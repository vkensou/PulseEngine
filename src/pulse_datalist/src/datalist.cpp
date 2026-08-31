#include "pulse_datalist.h"

#include <assert.h>
#include <inttypes.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DEPTH 256
#define SHORT_STRING 1024
#define CONVERTER 2
#define REF_CACHE 3
#define REF_UNSOLVED 4
#define TAB_SPACE 4
#define ERROR_SIZE 512
#define BLOCK_SIZE 65536

typedef uintptr_t objectid;

#if defined(_MSC_VER)
#define TLS_STATIC __declspec(thread)
#else
#define TLS_STATIC _Thread_local
#endif

TLS_STATIC char g_last_error[ERROR_SIZE];

typedef struct Block {
	struct Block *next;
	size_t used;
	size_t cap;
} Block;

typedef struct {
	Block *head;
} Arena;

static void set_error(const char *msg) {
	if (msg == NULL)
		msg = "";
	snprintf(g_last_error, sizeof(g_last_error), "%s", msg);
}

static void *arena_alloc(Arena *arena, size_t size) {
	size = (size + 15u) & ~(size_t)15u;
	Block *b = arena->head;
	if (b == NULL || b->cap - b->used < size) {
		size_t cap = size > BLOCK_SIZE ? size : BLOCK_SIZE;
		Block *nb = (Block *)calloc(1, sizeof(Block) + cap);
		if (nb == NULL)
			return NULL;
		nb->cap = cap;
		nb->next = b;
		arena->head = nb;
		b = nb;
	}
	void *p = (char *)(b + 1) + b->used;
	memset(p, 0, size);
	b->used += size;
	return p;
}

static void arena_free_all(Arena *arena) {
	Block *b = arena->head;
	while (b != NULL) {
		Block *next = b->next;
		free(b);
		b = next;
	}
	arena->head = NULL;
}

typedef struct Entry {
	char *key;
	PulseDatalist *value;
} Entry;

struct PulseDatalist {
	EPulseDatalistType type;
	PulseDatalist *root;
	int32_t ref_count;
	Arena arena;
	bool b;
	int64_t i;
	double d;
	char *s;
	size_t s_len;
	Entry *entries;
	size_t entry_count;
	size_t entry_cap;
	PulseDatalist **items;
	size_t item_count;
	size_t item_cap;
};

enum token_type {
	TOKEN_OPEN,	// 0 { [
	TOKEN_CLOSE,	// 1 } ]
	TOKEN_CONVERTER, // 2 $ ( $name something == [name, something] )
	TOKEN_MAP,	// 3 = :
	TOKEN_LIST,	// 4 ---
	TOKEN_STRING,	// 5
	TOKEN_ESCAPESTRING,	// 6
	TOKEN_ATOM,	// 7
	TOKEN_NEWLINE,	// 8 space \t
	TOKEN_TAG,	// 9	&badf00d  (64bit hex number)
	TOKEN_REF,	// 10	*badf00d
	TOKEN_EOF,	// 11 end of file
};

struct token {
	enum token_type type;
	ptrdiff_t from;
	ptrdiff_t to;
};

struct lex_state {
	const char *source;
	size_t sz;
	ptrdiff_t position;
	struct token c;
	struct token n;
	int newline;
	int aslist;
};

typedef struct TagEntry {
	uint64_t tag;
	PulseDatalist *node;
	int unsolved;
} TagEntry;

struct BuildState {
	Arena arena;
	PulseDatalist *root;
	TagEntry *tags;
	size_t tag_count;
	size_t tag_cap;
	jmp_buf errjmp;
	char error[ERROR_SIZE];
};

static void invalid(struct BuildState *B, struct lex_state *LS, const char *err);

static inline int
inset(const char *set, char c) {
	return c != '\0' && strchr(set, c) != NULL;
}

static const char *
skip_line_comment(struct lex_state *LS) {
	const char * ptr = LS->source + LS->position;
	const char * endptr = LS->source + LS->sz;
	while (ptr < endptr) {
		if (*ptr == '\r' || *ptr == '\n') {
			LS->position = ptr - LS->source;
			LS->newline = 1;
			return ptr;
		}
		++ptr;
	}
	return ptr;
}

static const char *
parse_ident(struct lex_state *LS) {
	const char * ptr = LS->source + LS->position;
	const char * endptr = LS->source + LS->sz;
	while (ptr < endptr) {
		switch (*ptr) {
		case '\r':
		case '\n':
			LS->newline = 1;
			return ptr+1;
		case '#':
			// empty line
			return ptr;
		case ' ':
		case '\t':
			break;
		default:
			LS->n.type = TOKEN_NEWLINE;
			LS->n.from = LS->position;
			LS->n.to = ptr - LS->source;
			LS->position = LS->n.to;
			return NULL;
		}
		++ptr;
	}
	return ptr;
}

static int
is_hexnumber(struct lex_state *LS) {
	const char * ptr = LS->source + LS->n.from + 1;
	const char * endptr = LS->source + LS->n.to;
	if (ptr == endptr)
		return 0;
	do {
		char c = *ptr;
		if (!((c >= '0' && c <= '9')
			|| (c >= 'a' && c <= 'f')
			|| (c >= 'A' && c <= 'F')))
			return 0;
	} while (++ptr < endptr);
	return 1;
}

static void
parse_atom(struct lex_state *LS) {
	static const char * separator = " \t\r\n,{}[]$:\"'";
	const char * ptr = LS->source + LS->position;
	const char * endptr = LS->source + LS->sz;
	char head = *ptr;
	LS->n.from = LS->position;
	while (ptr < endptr) {
		if (inset(separator, *ptr)) {
			break;
		}
		++ptr;
	}
	LS->n.to = ptr - LS->source;
	LS->position = LS->n.to;
	switch (head) {
	case '&':
		if (is_hexnumber(LS)) {
			LS->n.type = TOKEN_TAG;
			return;
		}
		break;
	case '*':
		if (is_hexnumber(LS)) {
			LS->n.type = TOKEN_REF;
			return;
		}
		break;
	default:
		break;
	}
	LS->n.type = TOKEN_ATOM;
}

static int
parse_string(struct lex_state *LS) {
	const char * ptr = LS->source + LS->position;
	const char * endptr = LS->source + LS->sz;
	char open_string = *ptr++;
	LS->n.type = TOKEN_STRING;
	LS->n.from = LS->position + 1;
	while (ptr < endptr) {
		char c = *ptr;
		if (c == open_string) {
			LS->n.to = ptr - LS->source;
			LS->position = ptr - LS->source + 1;
			return 1;
		}
		if (c == '\r' || c == '\n') {
			return 0;
		}
		if (c == '\\') {
			LS->n.type = TOKEN_ESCAPESTRING;
			++ptr;
		}
		++ptr;
	}
	return 0;
}

// 0 : invalid source
// 1 : ok
static int
next_token(struct lex_state *LS) {
	const char * ptr = LS->source + LS->position;
	const char * endptr = LS->source + LS->sz;
	while (ptr < endptr) {
		LS->position = ptr - LS->source;
		if (LS->newline) {
			// line head
			LS->newline = 0;
			const char * nextptr = parse_ident(LS);
			if (nextptr == NULL)
				return 1;
			// empty line
			ptr = nextptr;
			continue;
		}

		switch (*ptr) {
		case '#':
			// comment
			ptr = skip_line_comment(LS);
			continue;
		case '\r':
		case '\n':
			LS->newline = 1;
			++ptr;
			continue;
		case ' ':
		case '\t':
		case ',':
			break;
		case '{':
		case '[':
			LS->n.type = TOKEN_OPEN;
			LS->n.from = LS->position;
			LS->n.to = ++LS->position;
			return 1;
		case '$':
			LS->n.type = TOKEN_CONVERTER;
			LS->n.from = LS->position;
			LS->n.to = ++LS->position;
			return 1;
		case '}':
		case ']':
			LS->n.type = TOKEN_CLOSE;
			LS->n.from = LS->position;
			LS->n.to = ++LS->position;
			return 1;
		case '-':
			do ++ptr; while (ptr < endptr && *ptr == '-');
			if (ptr >= endptr || inset(" \t\r\n", *ptr)) {
				LS->n.type = TOKEN_LIST;
				LS->n.from = LS->position;
				LS->n.to = ptr - LS->source;
				LS->position = LS->n.to;
			} else {
				// negative number
				parse_atom(LS);
			}
			return 1;
		case ':':
			LS->n.type = TOKEN_MAP;
			LS->n.from = LS->position;
			LS->n.to = ++LS->position;
			return 1;
		case '"':
		case '\'':
			return parse_string(LS);
		default:
			parse_atom(LS);
			return 1;
		}
		++ptr;
	}
	LS->n.type = TOKEN_EOF;
	LS->position = LS->sz;
	return 1;
}

static void
invalid(struct BuildState *B, struct lex_state *LS, const char * err) {
	int line = 1;
	if (LS != NULL) {
		ptrdiff_t index;
		ptrdiff_t position = LS->n.from;
		for (index = 0; index < position ; index ++) {
			if (LS->source[index] == '\n')
				++line;
		}
	}
	snprintf(B->error, sizeof(B->error), "Line %d : %s", line, err);
	longjmp(B->errjmp, 1);
}

static inline int
read_token(struct BuildState *B, struct lex_state *LS) {
	if (LS->c.type == TOKEN_EOF) {
		invalid(B, LS, "End of data");
	}
	LS->c = LS->n;
	if (!next_token(LS))
		invalid(B, LS, "Invalid token");
//	printf("token %d %.*s\n", LS->c.type, (int)(LS->c.to-LS->c.from), LS->source + LS->c.from);
	return LS->c.type;
}

static inline int
to_hex(char c) {
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

static PulseDatalist *new_table_0(struct BuildState *B);

static PulseDatalist *
push_token_string(struct BuildState *B, const char *ptr, size_t sz) {
	PulseDatalist *n = new_table_0(B);
	n->type = PULSE_DATALIST_TYPE_STRING;
	char *buffer = (char *)arena_alloc(&B->arena, sz + 1);
	if (buffer == NULL) {
		invalid(B, NULL, "Out of memory");
		return NULL;
	}

	size_t i, m;
	for (m=i=0;i<sz;++i,++ptr,++m) {
		if (*ptr != '\\') {
			buffer[m] = *ptr;
		} else {
			++ptr;
			++i;
			assert(i < sz);
			char c = *ptr;
			if (c == '0') {
				buffer[m] = '\0';
			} else if (c >= '1' && c <= '9') {
				// escape dec ascii
				int dec = c - '0';
				if (i+1 < sz) {
					int c2 = ptr[1];
					if (c2 >= '0' && c2 <= '9') {
						dec = dec * 10 + c2 - '0';
						++ptr;
						++i;
					}
				}
				if (i+1 < sz) {
					int c2 = ptr[1];
					if (c2 >= '0' && c2 <= '9') {
						int tmp = dec * 10 + c2 - '0';
						if (tmp <= 255) {
							dec = tmp;
							++ptr;
							++i;
						}
					}
				}
				buffer[m] = dec;
			} else {
				switch(*ptr) {
				case 'x':
				case 'X': {
					// escape hex ascii
					if (i+2 >= sz) {
						invalid(B, NULL, "Invalid quote string");
						return NULL;
					}
					++ptr;
					++i;
					int hex = to_hex(*ptr);
					if (hex < 0) {
						invalid(B, NULL, "Invalid quote string");
						return NULL;
					}
					++ptr;
					++i;
					int hex2 = to_hex(*ptr);
					if (hex2 >= 0) {
						hex = hex * 16 + hex2;
					}
					buffer[m] = hex;
					break;
				}
				case 'n':
					buffer[m] = '\n';
					break;
				case 'r':
					buffer[m] = '\r';
					break;
				case 't':
					buffer[m] = '\t';
					break;
				case 'a':
					buffer[m] = '\a';
					break;
				case 'b':
					buffer[m] = '\b';
					break;
				case 'v':
					buffer[m] = '\v';
					break;
				case '\'':
				case '"':
				case '\n':
				case '\r':
					buffer[m] = *ptr;
					break;
				default:
					invalid(B, NULL, "Invalid quote string");
					return NULL;
				}
			}
		}
	}
	n->s = buffer;
	n->s_len = m;
	return n;
}

#define IS_KEYWORD(ptr, sz, str) (sizeof(str "") == sz+1 && (memcmp(ptr, str, sz) == 0))

static PulseDatalist *make_string_raw(struct BuildState *B, const char *ptr, size_t sz);

static PulseDatalist *
push_token(struct BuildState *B, struct lex_state *LS, struct token *t) {
	const char * ptr = LS->source + t->from;
	size_t sz = t->to - t->from;

	switch(t->type) {
	case TOKEN_STRING:
		return make_string_raw(B, ptr, sz);
	case TOKEN_ESCAPESTRING:
		return push_token_string(B, ptr, sz);
	case TOKEN_ATOM:
		break;
	default:
		invalid(B, LS, "Invalid atom");
		return NULL;
	}

	if (inset("0123456789+-.", ptr[0])) {
		if (sz == 1) {
			char c = *ptr;
			if (c >= '0' && c <='9') {
				PulseDatalist *n = new_table_0(B);
				n->type = PULSE_DATALIST_TYPE_INT;
				n->i = c - '0';
				return n;
			} else {
				return make_string_raw(B, ptr, 1);
			}
		}

		if (sz >=3 && ptr[0] == '0' && (ptr[1] == 'x' || ptr[1] == 'X')) {
			// may be a hex integer
			uint64_t v = 0;
			int hex = 1;
			size_t i;
			for (i=2;i<sz;i++) {
				char c = ptr[i];
				v = v * 16;
				if (c >= '0' && c <='9') {
					v += c - '0';
				} else if (c >= 'a' && c <= 'f') {
					v += c - 'a' + 10;
				} else if (c >= 'A' && c <= 'F') {
					v += c - 'A' + 10;
				} else {
					hex = 0;
					break;
				}
			}
			if (hex) {
				PulseDatalist *n = new_table_0(B);
				n->type = PULSE_DATALIST_TYPE_INT;
				n->i = (int64_t)v;
				return n;
			}
		}

		// may be a number
		// lua string always has \0 at the end, so strto* is safe
		char *temp = (char *)arena_alloc(&B->arena, sz + 1);
		if (temp == NULL) {
			invalid(B, LS, "Out of memory");
			return NULL;
		}
		memcpy(temp, ptr, sz);
		temp[sz] = '\0';
		char *endptr = NULL;
		uint64_t uv = strtoull(temp, &endptr, 10);
		if (endptr - temp == (ptrdiff_t)sz) {
			PulseDatalist *n = new_table_0(B);
			n->type = PULSE_DATALIST_TYPE_INT;
			n->i = (int64_t)uv;
			return n;
		}

		endptr = NULL;
		double f = strtod(temp, &endptr);
		if (endptr - temp == (ptrdiff_t)sz) {
			PulseDatalist *n = new_table_0(B);
			n->type = PULSE_DATALIST_TYPE_DOUBLE;
			n->d = f;
			return n;
		}
	}

	if (t->type == TOKEN_ATOM) {
		if (IS_KEYWORD(ptr, sz, "true")) {
			PulseDatalist *n = new_table_0(B);
			n->type = PULSE_DATALIST_TYPE_BOOL;
			n->b = true;
			return n;
		} else if (IS_KEYWORD(ptr, sz, "false")) {
			PulseDatalist *n = new_table_0(B);
			n->type = PULSE_DATALIST_TYPE_BOOL;
			return n;
		} else if (IS_KEYWORD(ptr, sz, "nil")) {
			PulseDatalist *n = new_table_0(B);
			n->type = PULSE_DATALIST_TYPE_NIL;
			return n;
		}
	}

	return make_string_raw(B, ptr, sz);
}

static inline int
token_length(struct token *t) {
	return (int)(t->to - t->from);
}

static inline int
token_ident(struct lex_state *LS) {
	struct token *t = &LS->c;
	const char * ptr = LS->source + t->from;
	const char * endptr = LS->source + t->to;
	int ident = token_length(t);
	while (endptr > ptr) {
		if (*ptr == '\t')
			ident += TAB_SPACE - 1;
		++ptr;
	}
	return ident;
}

static inline char
token_symbol(struct lex_state *LS) {
	return LS->source[LS->c.from];
}

static inline void
push_key(struct lex_state *LS, ptrdiff_t *from, ptrdiff_t *to) {
	*from = LS->c.from;
	*to = LS->c.to;
}

static PulseDatalist *
new_table_0(struct BuildState *B) {
	PulseDatalist *n = (PulseDatalist *)arena_alloc(&B->arena, sizeof(PulseDatalist));
	if (n == NULL) {
		invalid(B, NULL, "Out of memory");
		return NULL;
	}
	n->type = PULSE_DATALIST_TYPE_LIST;
	n->ref_count = 1;
	n->root = B->root;
	if (B->root == NULL)
		B->root = n;
	if (n->root == NULL)
		n->root = n;
	return n;
}

static PulseDatalist *
new_table(struct BuildState *B, int layer, objectid ref) {
	if (layer >= MAX_DEPTH)
		invalid(B, NULL, "too many layers");
	if (ref == 0) {
		return new_table_0(B);
	} else {
		size_t i;
		for (i = 0; i < B->tag_count; i++) {
			if (B->tags[i].tag == (uint64_t)ref)
				return B->tags[i].node;
		}
		invalid(B, NULL, "Unknown tag");
		return NULL;
	}
}

static objectid
read_tag(struct lex_state *LS) {
	const char * ptr = LS->source + LS->c.from + 1;
	const char * endptr = LS->source + LS->c.to;
	objectid x = 0;
	while (ptr < endptr) {
		char c = *ptr;
		int n;
		if (c >= '0' && c <= '9')
			n = c - '0';
		else if (c >= 'a' && c <= 'f')
			n = c - 'a' + 10;
		else
			n = c - 'A' + 10;
		x = x * 16 + n;
		++ptr;
	}
	if (x == 0) {
		x = ~(objectid)0;
	}
	return x;
}

static void
cache_tag(struct BuildState *B, objectid tag, PulseDatalist *n, int unsolved) {
	if (B->tag_count >= B->tag_cap) {
		size_t ncap = B->tag_cap == 0 ? 4 : B->tag_cap * 2;
		TagEntry *np = (TagEntry *)arena_alloc(&B->arena, ncap * sizeof(TagEntry));
		if (np == NULL) {
			invalid(B, NULL, "Out of memory");
			return;
		}
		if (B->tags != NULL)
			memcpy(np, B->tags, B->tag_cap * sizeof(TagEntry));
		B->tags = np;
		B->tag_cap = ncap;
	}
	B->tags[B->tag_count].tag = (uint64_t)tag;
	B->tags[B->tag_count].node = n;
	B->tags[B->tag_count].unsolved = unsolved;
	B->tag_count++;
}

static objectid
parse_tag(struct BuildState *B, struct lex_state *LS) {
	objectid tag = read_tag(LS);
	size_t i;
	for (i = 0; i < B->tag_count; i++) {
		if (B->tags[i].tag == (uint64_t)tag) {
			if (!B->tags[i].unsolved) {
				invalid(B, LS, "Duplicate tag");
			}
			// clear unsolved
			B->tags[i].unsolved = 0;
			return tag;
		}
	}
	PulseDatalist *n = new_table_0(B);
	cache_tag(B, tag, n, 0);
	return tag;
}

static PulseDatalist *
parse_ref(struct BuildState *B, struct lex_state *LS) {
	objectid tag = read_tag(LS);
	read_token(B, LS);	//	consume ref tag
	size_t i;
	for (i = 0; i < B->tag_count; i++) {
		if (B->tags[i].tag == (uint64_t)tag)
			return B->tags[i].node;
	}
	PulseDatalist *n = new_table_0(B);		// Create a table for future
	cache_tag(B, tag, n, 1);
	return n;
}

static PulseDatalist *parse_bracket(struct BuildState *B, struct lex_state *LS, int layer, objectid tag);
static PulseDatalist *parse_converter(struct BuildState *B, struct lex_state *LS, int layer, int ident);

static int
closed_bracket(struct BuildState *B, struct lex_state *LS, int bracket) {
	for (;;) {
		switch (LS->c.type) {
		case TOKEN_CLOSE:
			if (token_symbol(LS) != bracket) {
				invalid(B, LS, "Invalid close bracket");
			}
			read_token(B, LS);
			return 1;
		case TOKEN_NEWLINE:
			read_token(B, LS);
			break;
		default:
			return 0;
		}
	}
}

// table key value
static void
set_keyvalue(struct BuildState *B, struct lex_state *LS, PulseDatalist *n, ptrdiff_t kfrom, ptrdiff_t kto, PulseDatalist *v) {
	char *kbuf = (char *)arena_alloc(&B->arena, (size_t)(kto - kfrom) + 1);
	if (kbuf == NULL) {
		invalid(B, LS, "Out of memory");
		return;
	}
	memcpy(kbuf, LS->source + kfrom, (size_t)(kto - kfrom));
	kbuf[kto - kfrom] = '\0';
	size_t i;
	PulseDatalist *old = NULL;
	for (i = 0; i < n->entry_count; i++) {
		if (strcmp(n->entries[i].key, kbuf) == 0) {
			old = n->entries[i].value;
			break;
		}
	}
	if (v->type == PULSE_DATALIST_TYPE_NIL) {
		if (old != NULL) {
			memmove(&n->entries[i], &n->entries[i + 1], (n->entry_count - i - 1) * sizeof(Entry));
			n->entry_count--;
			if (n->entry_count == 0 && n->type == PULSE_DATALIST_TYPE_MAP)
				n->type = n->item_count > 0 ? PULSE_DATALIST_TYPE_MIXED : PULSE_DATALIST_TYPE_LIST;
		}
		return;
	}
	if (old != NULL) {
		if (old->type == PULSE_DATALIST_TYPE_MAP || old->type == PULSE_DATALIST_TYPE_LIST || old->type == PULSE_DATALIST_TYPE_MIXED) {
			// append to the old table
			if (old->item_count + 1 > old->item_cap) {
				size_t ncap = old->item_cap == 0 ? 4 : old->item_cap * 2;
				PulseDatalist **np = (PulseDatalist **)arena_alloc(&B->arena, ncap * sizeof(PulseDatalist *));
				if (np == NULL) {
					invalid(B, LS, "Out of memory");
					return;
				}
				if (old->items != NULL)
					memcpy(np, old->items, old->item_cap * sizeof(PulseDatalist *));
				old->items = np;
				old->item_cap = ncap;
			}
			old->items[old->item_count++] = v;
			if (old->type == PULSE_DATALIST_TYPE_MAP)
				old->type = PULSE_DATALIST_TYPE_MIXED;
		} else {
			char msg[ERROR_SIZE];
			snprintf(msg, sizeof(msg), "Multi-key (%s) should be a table", kbuf);
			invalid(B, LS, msg);
		}
		return;
	}
	if (n->entry_count + 1 > n->entry_cap) {
		size_t ncap = n->entry_cap == 0 ? 4 : n->entry_cap * 2;
		Entry *np = (Entry *)arena_alloc(&B->arena, ncap * sizeof(Entry));
		if (np == NULL) {
			invalid(B, LS, "Out of memory");
			return;
		}
		if (n->entries != NULL)
			memcpy(np, n->entries, n->entry_cap * sizeof(Entry));
		n->entries = np;
		n->entry_cap = ncap;
	}
	n->entries[n->entry_count].key = kbuf;
	n->entries[n->entry_count].value = v;
	n->entry_count++;
	if (n->type == PULSE_DATALIST_TYPE_LIST) {
		if (n->item_count > 0)
			n->type = PULSE_DATALIST_TYPE_MIXED;
		else
			n->type = PULSE_DATALIST_TYPE_MAP;
	}
}

static void
parse_bracket_map(struct BuildState *B, struct lex_state *LS, int layer, PulseDatalist *n, int bracket) {
	int i = 1;
	int aslist = LS->aslist;
	do {
		if (LS->c.type != TOKEN_ATOM) {
			invalid(B, LS, "Invalid key");
		}
		ptrdiff_t kfrom = LS->c.from;
		ptrdiff_t kto = LS->c.to;
		if (read_token(B, LS) != TOKEN_MAP) {
			invalid(B, LS, "Need a : or =");
		}
		objectid tag = 0;
		int t = read_token(B, LS);
		if (t == TOKEN_TAG) {
			tag = parse_tag(B, LS);
			t = read_token(B, LS);
		}
		PulseDatalist *v;
		switch (LS->c.type) {
		case TOKEN_REF:
			if (tag != 0) {
				invalid(B, LS, "Invalid ref in bracket map");
			}
			v = parse_ref(B, LS);
			break;
		case TOKEN_OPEN:
			v = parse_bracket(B, LS, layer+1, tag);
			break;
		case TOKEN_CONVERTER:
			v = parse_converter(B, LS, layer+1, -1);
			break;
		default:
			v = push_token(B, LS, &LS->c);
			read_token(B, LS);
			break;
		}
		if (aslist) {
			if (n->item_count + 1 > n->item_cap) {
				size_t ncap = n->item_cap == 0 ? 4 : n->item_cap * 2;
				PulseDatalist **np = (PulseDatalist **)arena_alloc(&B->arena, ncap * sizeof(PulseDatalist *));
				if (np == NULL) {
					invalid(B, LS, "Out of memory");
					return;
				}
				if (n->items != NULL)
					memcpy(np, n->items, n->item_cap * sizeof(PulseDatalist *));
				n->items = np;
				n->item_cap = ncap;
			}
			n->items[n->item_count++] = make_string_raw(B, LS->source + kfrom, (size_t)(kto - kfrom));
			if (n->item_count + 1 > n->item_cap) {
				size_t ncap = n->item_cap == 0 ? 4 : n->item_cap * 2;
				PulseDatalist **np = (PulseDatalist **)arena_alloc(&B->arena, ncap * sizeof(PulseDatalist *));
				if (np == NULL) {
					invalid(B, LS, "Out of memory");
					return;
				}
				if (n->items != NULL)
					memcpy(np, n->items, n->item_cap * sizeof(PulseDatalist *));
				n->items = np;
				n->item_cap = ncap;
			}
			n->items[n->item_count++] = v;
			if (n->type == PULSE_DATALIST_TYPE_MAP)
				n->type = PULSE_DATALIST_TYPE_MIXED;
		} else {
			set_keyvalue(B, LS, n, kfrom, kto, v);
		}
		(void)i;
	} while (!closed_bracket(B, LS, bracket));
}

static PulseDatalist *
parse_bracket_sequence(struct BuildState *B, struct lex_state *LS, int layer, PulseDatalist *n, int bracket) {
	for (;;) {
		switch (LS->c.type) {
		case TOKEN_NEWLINE:
			read_token(B, LS);
			continue;	// skip ident
		case TOKEN_CLOSE:
			if (token_symbol(LS) != bracket) {
				invalid(B, LS, "Invalid close bracket");
			}
			read_token(B, LS);	// consume }
			return n;
		case TOKEN_REF:
			return parse_ref(B, LS);
		case TOKEN_OPEN:
			// No tag in sequence
			if (n->item_count + 1 > n->item_cap) {
				size_t ncap = n->item_cap == 0 ? 4 : n->item_cap * 2;
				PulseDatalist **np = (PulseDatalist **)arena_alloc(&B->arena, ncap * sizeof(PulseDatalist *));
				if (np == NULL) {
					invalid(B, LS, "Out of memory");
					return NULL;
				}
				if (n->items != NULL)
					memcpy(np, n->items, n->item_cap * sizeof(PulseDatalist *));
				n->items = np;
				n->item_cap = ncap;
			}
			n->items[n->item_count++] = parse_bracket(B, LS, layer, 0);
			break;
		case TOKEN_CONVERTER:
			if (n->item_count + 1 > n->item_cap) {
				size_t ncap = n->item_cap == 0 ? 4 : n->item_cap * 2;
				PulseDatalist **np = (PulseDatalist **)arena_alloc(&B->arena, ncap * sizeof(PulseDatalist *));
				if (np == NULL) {
					invalid(B, LS, "Out of memory");
					return NULL;
				}
				if (n->items != NULL)
					memcpy(np, n->items, n->item_cap * sizeof(PulseDatalist *));
				n->items = np;
				n->item_cap = ncap;
			}
			n->items[n->item_count++] = parse_converter(B, LS, layer, -1);
			break;
		default:
			if (n->item_count + 1 > n->item_cap) {
				size_t ncap = n->item_cap == 0 ? 4 : n->item_cap * 2;
				PulseDatalist **np = (PulseDatalist **)arena_alloc(&B->arena, ncap * sizeof(PulseDatalist *));
				if (np == NULL) {
					invalid(B, LS, "Out of memory");
					return NULL;
				}
				if (n->items != NULL)
					memcpy(np, n->items, n->item_cap * sizeof(PulseDatalist *));
				n->items = np;
				n->item_cap = ncap;
			}
			n->items[n->item_count++] = push_token(B, LS, &LS->c);
			read_token(B, LS);
			break;
		}
	}
}

static PulseDatalist *
parse_bracket_(struct BuildState *B, struct lex_state *LS, int layer, PulseDatalist *n, int bracket) {
again:
	switch (read_token(B, LS)) {
	case TOKEN_NEWLINE:
		goto again;
	case TOKEN_CLOSE:
		if (token_symbol(LS) != bracket) {
			invalid(B, LS, "Invalid close bracket");
		}
		read_token(B, LS);	// consume }
		return n;
	case TOKEN_ATOM:
		if (LS->n.type == TOKEN_MAP) {
			parse_bracket_map(B, LS, layer, n, bracket);
			return n;
		}
		break;
	default:
		break;
	}
	return parse_bracket_sequence(B, LS, layer, n, bracket);
}

static PulseDatalist *
parse_bracket(struct BuildState *B, struct lex_state *LS, int layer, objectid tag) {
	char bracket = token_symbol(LS);
	if (bracket == '[') {
		if (tag) {
			invalid(B, LS, "[] can't has a tag");
		}
		PulseDatalist *n = new_table_0(B);
		return parse_bracket_(B, LS, layer, n, ']');
	} else {
		PulseDatalist *n = new_table(B, layer, tag);
		return parse_bracket_(B, LS, layer, n, '}');
	}
}

static void parse_section(struct BuildState *B, struct lex_state *LS, int layer);

static void
parse_section_at(struct BuildState *B, struct lex_state *LS, int layer, PulseDatalist *container) {
	PulseDatalist *saved = B->root;
	B->root = container;
	parse_section(B, LS, layer);
	B->root = saved;
}

static PulseDatalist *
parse_converter(struct BuildState *B, struct lex_state *LS, int layer, int ident) {
	PulseDatalist *n = new_table_0(B);
	if (read_token(B, LS) != TOKEN_ATOM) {
		invalid(B, LS, "$ need an atom");
	}
	if (n->item_count + 1 > n->item_cap) {
		size_t ncap = n->item_cap == 0 ? 4 : n->item_cap * 2;
		PulseDatalist **np = (PulseDatalist **)arena_alloc(&B->arena, ncap * sizeof(PulseDatalist *));
		if (np == NULL) {
			invalid(B, LS, "Out of memory");
			return NULL;
		}
		if (n->items != NULL)
			memcpy(np, n->items, n->item_cap * sizeof(PulseDatalist *));
		n->items = np;
		n->item_cap = ncap;
	}
	n->items[n->item_count++] = make_string_raw(B, LS->source + LS->c.from, (size_t)(LS->c.to - LS->c.from));	// $atom xxx === [ "atom",  xxx ]

	read_token(B, LS);

	PulseDatalist *v;
	switch (LS->c.type) {
	case TOKEN_NEWLINE:
		if (ident < 0)
			invalid(B, LS, "Invalid newline , Use { } for a struct instead");
		{
			int next_ident = token_ident(LS);
			if (next_ident < ident) {
				invalid(B, LS, "Invalid new section ident");
			}
			PulseDatalist *sub = new_table(B, layer+1, 0);
			parse_section_at(B, LS, layer+1, sub);
			v = sub;
		}
		break;
	case TOKEN_CLOSE:
		invalid(B, LS, "Invalid close bracket");
		v = NULL;
		break;
	case TOKEN_REF:
		v = parse_ref(B, LS);
		break;
	case TOKEN_OPEN:
		v = parse_bracket(B, LS, layer, 0);
		break;
	case TOKEN_CONVERTER:
		v = parse_converter(B, LS, layer, ident+1);
		break;
	default:
		v = push_token(B, LS, &LS->c);
		read_token(B, LS);
		break;
	}

	if (n->item_count + 1 > n->item_cap) {
		size_t ncap = n->item_cap == 0 ? 4 : n->item_cap * 2;
		PulseDatalist **np = (PulseDatalist **)arena_alloc(&B->arena, ncap * sizeof(PulseDatalist *));
		if (np == NULL) {
			invalid(B, LS, "Out of memory");
			return NULL;
		}
		if (n->items != NULL)
			memcpy(np, n->items, n->item_cap * sizeof(PulseDatalist *));
		n->items = np;
		n->item_cap = ncap;
	}
	n->items[n->item_count++] = v;
	return n;
}

static int
next_item(struct BuildState *B, struct lex_state *LS, int ident) {
	int t = LS->c.type;
	if (t == TOKEN_NEWLINE) {
		int next_ident = token_ident(LS);
		if (next_ident == ident) {
			if (LS->n.type == TOKEN_LIST)
				return 0;
			read_token(B, LS);
			return 1;
		} else if (next_ident > ident) {
			invalid(B, LS, "Invalid ident");
		} else {
			// end of sequence
			return 0;
		}
	} else if (t == TOKEN_EOF) {
		return 0;
	}
	return 1;
}

static PulseDatalist *
make_string_raw(struct BuildState *B, const char *ptr, size_t sz) {
	PulseDatalist *n = new_table_0(B);
	n->type = PULSE_DATALIST_TYPE_STRING;
	n->s = (char *)arena_alloc(&B->arena, sz + 1);
	if (n->s == NULL) {
		invalid(B, NULL, "Out of memory");
		return NULL;
	}
	memcpy(n->s, ptr, sz);
	n->s[sz] = '\0';
	n->s_len = sz;
	return n;
}

static void
parse_section_map(struct BuildState *B, struct lex_state *LS, int ident, int layer) {
	PulseDatalist *n = B->root;
	int i = 1;
	int aslist = LS->aslist;
	do {
		if (LS->c.type != TOKEN_ATOM)
			invalid(B, LS, "Invalid key");
		ptrdiff_t kfrom, kto;
		push_key(LS, &kfrom, &kto);
		if (read_token(B, LS) != TOKEN_MAP) {
			invalid(B, LS, "Need a : or =");
		}
		objectid tag = 0;
		int t = read_token(B, LS);
		if (t == TOKEN_TAG) {
			tag = parse_tag(B, LS);
			t = read_token(B, LS);
		}

		PulseDatalist *v;
		switch (t) {
		case TOKEN_REF:
			if (tag != 0) {
				invalid(B, LS, "Invalid ref after tag");
			}
			v = parse_ref(B, LS);
			break;
		case TOKEN_OPEN:
			v = parse_bracket(B, LS, layer+1, tag);
			break;
		case TOKEN_CONVERTER:
			v = parse_converter(B, LS, layer+1, ident+1);
			break;
		case TOKEN_NEWLINE: {
			int next_ident = token_ident(LS);
			if (next_ident <= ident) {
				invalid(B, LS, "Invalid new section ident");
			}
			PulseDatalist *sub = new_table(B, layer+1, tag);
			parse_section_at(B, LS, layer+1, sub);
			v = sub;
			break;
		}
		default:
			v = push_token(B, LS, &LS->c);
			read_token(B, LS);
			break;
		}
		if (aslist) {
			if (n->item_count + 1 > n->item_cap) {
				size_t ncap = n->item_cap == 0 ? 4 : n->item_cap * 2;
				PulseDatalist **np = (PulseDatalist **)arena_alloc(&B->arena, ncap * sizeof(PulseDatalist *));
				if (np == NULL) {
					invalid(B, LS, "Out of memory");
					return;
				}
				if (n->items != NULL)
					memcpy(np, n->items, n->item_cap * sizeof(PulseDatalist *));
				n->items = np;
				n->item_cap = ncap;
			}
			n->items[n->item_count++] = make_string_raw(B, LS->source + kfrom, (size_t)(kto - kfrom));
			if (n->item_count + 1 > n->item_cap) {
				size_t ncap = n->item_cap == 0 ? 4 : n->item_cap * 2;
				PulseDatalist **np = (PulseDatalist **)arena_alloc(&B->arena, ncap * sizeof(PulseDatalist *));
				if (np == NULL) {
					invalid(B, LS, "Out of memory");
					return;
				}
				if (n->items != NULL)
					memcpy(np, n->items, n->item_cap * sizeof(PulseDatalist *));
				n->items = np;
				n->item_cap = ncap;
			}
			n->items[n->item_count++] = v;
			if (n->type == PULSE_DATALIST_TYPE_MAP)
				n->type = PULSE_DATALIST_TYPE_MIXED;
		} else {
			set_keyvalue(B, LS, n, kfrom, kto, v);
		}
		(void)i;
	} while (next_item(B, LS, ident));
}

static void
parse_section_sequence(struct BuildState *B, struct lex_state *LS, int ident, int layer) {
	PulseDatalist *n = B->root;
	int i = 1;
	do {
		switch (LS->c.type) {
		case TOKEN_REF:
			if (n->item_count + 1 > n->item_cap) {
				size_t ncap = n->item_cap == 0 ? 4 : n->item_cap * 2;
				PulseDatalist **np = (PulseDatalist **)arena_alloc(&B->arena, ncap * sizeof(PulseDatalist *));
				if (np == NULL) {
					invalid(B, LS, "Out of memory");
					return;
				}
				if (n->items != NULL)
					memcpy(np, n->items, n->item_cap * sizeof(PulseDatalist *));
				n->items = np;
				n->item_cap = ncap;
			}
			n->items[n->item_count++] = parse_ref(B, LS);
			break;
		case TOKEN_OPEN:
			if (n->item_count + 1 > n->item_cap) {
				size_t ncap = n->item_cap == 0 ? 4 : n->item_cap * 2;
				PulseDatalist **np = (PulseDatalist **)arena_alloc(&B->arena, ncap * sizeof(PulseDatalist *));
				if (np == NULL) {
					invalid(B, LS, "Out of memory");
					return;
				}
				if (n->items != NULL)
					memcpy(np, n->items, n->item_cap * sizeof(PulseDatalist *));
				n->items = np;
				n->item_cap = ncap;
			}
			n->items[n->item_count++] = parse_bracket(B, LS, layer+1, 0);
			break;
		case TOKEN_CONVERTER:
			if (n->item_count + 1 > n->item_cap) {
				size_t ncap = n->item_cap == 0 ? 4 : n->item_cap * 2;
				PulseDatalist **np = (PulseDatalist **)arena_alloc(&B->arena, ncap * sizeof(PulseDatalist *));
				if (np == NULL) {
					invalid(B, LS, "Out of memory");
					return;
				}
				if (n->items != NULL)
					memcpy(np, n->items, n->item_cap * sizeof(PulseDatalist *));
				n->items = np;
				n->item_cap = ncap;
			}
			n->items[n->item_count++] = parse_converter(B, LS, layer+1, ident+1);
			break;
		case TOKEN_LIST:
			// end of this section
			return;
		default:
			if (n->item_count + 1 > n->item_cap) {
				size_t ncap = n->item_cap == 0 ? 4 : n->item_cap * 2;
				PulseDatalist **np = (PulseDatalist **)arena_alloc(&B->arena, ncap * sizeof(PulseDatalist *));
				if (np == NULL) {
					invalid(B, LS, "Out of memory");
					return;
				}
				if (n->items != NULL)
					memcpy(np, n->items, n->item_cap * sizeof(PulseDatalist *));
				n->items = np;
				n->item_cap = ncap;
			}
			n->items[n->item_count++] = push_token(B, LS, &LS->c);
			read_token(B, LS);
			break;
		}
		(void)i;
	} while(next_item(B, LS, ident));
}

static int
next_list(struct BuildState *B, struct lex_state *LS, int ident) {
	int t = LS->c.type;
	if (t == TOKEN_NEWLINE) {
		int next_ident = token_ident(LS);
		if (next_ident == ident) {
			switch (read_token(B, LS)) {
			case TOKEN_EOF:
				return 0;
			case TOKEN_LIST:
				// next list
				return 1;
			default:
				invalid(B, LS, "Invalid list");
				break;
			}
		} else if (next_ident < ident) {
			// end of sequence
			return 0;
		}
	} else if (t == TOKEN_EOF)
		return 0;
	invalid(B, LS, "Invalid list");
	return 0;
}

static PulseDatalist *
empty_list(struct BuildState *B, PulseDatalist *tag) {
	if (tag != NULL)
		return tag;
	return new_table_0(B);
}

static void
parse_section_list(struct BuildState *B, struct lex_state *LS, int ident, int layer) {
	PulseDatalist *n = B->root;
	int i = 1;
	do {
		int t = read_token(B, LS);
		objectid tag = 0;
		if (t == TOKEN_TAG) {
			tag = parse_tag(B, LS);
			t = read_token(B, LS);
		}
		PulseDatalist *v;
		switch (t) {
		case TOKEN_REF:
			if (tag != 0) {
				invalid(B, LS, "Invalid ref after tag");
			}
			v = parse_ref(B, LS);
			break;
		case TOKEN_OPEN:
			v = parse_bracket(B, LS, layer+1, tag);
			break;
		case TOKEN_CONVERTER:
			v = parse_converter(B, LS, layer+1, ident);
			break;
		case TOKEN_NEWLINE: {
			int next_ident = token_ident(LS);
			if (next_ident >= ident) {
				PulseDatalist *sub = new_table(B, layer + 1, tag);
				if (LS->n.type != TOKEN_LIST || next_ident > ident) {
					// not an empty list
					parse_section_at(B, LS, layer + 1, sub);
				}
				v = sub;
			} else {
				// end of list
				v = empty_list(B, new_table(B, 0, tag));
				if (n->item_count + 1 > n->item_cap) {
					size_t ncap = n->item_cap == 0 ? 4 : n->item_cap * 2;
					PulseDatalist **np = (PulseDatalist **)arena_alloc(&B->arena, ncap * sizeof(PulseDatalist *));
					if (np == NULL) {
						invalid(B, LS, "Out of memory");
						return;
					}
					if (n->items != NULL)
						memcpy(np, n->items, n->item_cap * sizeof(PulseDatalist *));
					n->items = np;
					n->item_cap = ncap;
				}
				n->items[n->item_count++] = v;
				return;
			}
			break;
		}
		case TOKEN_EOF:
			// empty list
			v = empty_list(B, new_table(B, 0, tag));
			if (n->item_count + 1 > n->item_cap) {
				size_t ncap = n->item_cap == 0 ? 4 : n->item_cap * 2;
				PulseDatalist **np = (PulseDatalist **)arena_alloc(&B->arena, ncap * sizeof(PulseDatalist *));
				if (np == NULL) {
					invalid(B, LS, "Out of memory");
					return;
				}
				if (n->items != NULL)
					memcpy(np, n->items, n->item_cap * sizeof(PulseDatalist *));
				n->items = np;
				n->item_cap = ncap;
			}
			n->items[n->item_count++] = v;
			return;
		default:
			v = push_token(B, LS, &LS->c);
			read_token(B, LS);
			break;
		}
		if (n->item_count + 1 > n->item_cap) {
			size_t ncap = n->item_cap == 0 ? 4 : n->item_cap * 2;
			PulseDatalist **np = (PulseDatalist **)arena_alloc(&B->arena, ncap * sizeof(PulseDatalist *));
			if (np == NULL) {
				invalid(B, LS, "Out of memory");
				return;
			}
			if (n->items != NULL)
				memcpy(np, n->items, n->item_cap * sizeof(PulseDatalist *));
			n->items = np;
			n->item_cap = ncap;
		}
		n->items[n->item_count++] = v;
		(void)i;
	} while(next_list(B, LS, ident));
}

static void
parse_section(struct BuildState *B, struct lex_state *LS, int layer) {
	int ident = token_ident(LS);
	switch (read_token(B, LS)) {
	case TOKEN_ATOM:
		if (LS->n.type == TOKEN_MAP) {
			parse_section_map(B, LS, ident, layer);
			return;
		}
		break;
	case TOKEN_EOF:
		return;
	case TOKEN_STRING:
	case TOKEN_ESCAPESTRING:
	case TOKEN_OPEN:
	case TOKEN_REF:
	case TOKEN_CONVERTER:
		break;
	case TOKEN_LIST:
		parse_section_list(B, LS, ident, layer);
		return;
	default:
		invalid(B, LS, "Invalid section");
	}
	// a sequence
	parse_section_sequence(B, LS, ident, layer);
}

static void
init_lex(struct BuildState *B, struct lex_state *LS, const char *text, size_t len, int aslist) {
	LS->source = text;
	LS->sz = len;
	LS->position = 0;
	LS->newline = 1;
	LS->aslist = aslist;
	if (!next_token(LS))
		invalid(B, LS, "Invalid token");
}

static void
parse_all(struct BuildState *B, struct lex_state *LS) {
	PulseDatalist *root = new_table_0(B);	// top level
	B->root = root;
	int tt = read_token(B, LS);
	if (tt == TOKEN_EOF)
		return;
	assert(tt == TOKEN_NEWLINE);
	parse_section(B, LS, 0);
	if (LS->c.type != TOKEN_EOF) {
		invalid(B, LS, "not end");
	}
	// check unsolved
	{
		size_t i;
		for (i = 0; i < B->tag_count; i++) {
			if (B->tags[i].unsolved) {
				char msg[ERROR_SIZE];
				snprintf(msg, sizeof(msg), "Unsolved tag %" PRIx64, B->tags[i].tag);
				invalid(B, LS, msg);
			}
		}
	}
}

static PulseDatalist *
parse_text_internal(const char *text, size_t len, int aslist) {
	struct BuildState B;
	struct lex_state LS;
	memset(&B, 0, sizeof(B));
	memset(&LS, 0, sizeof(LS));
	if (setjmp(B.errjmp)) {
		set_error(B.error);
		arena_free_all(&B.arena);
		return NULL;
	}
	init_lex(&B, &LS, text, len, aslist);
	parse_all(&B, &LS);
	B.root->arena = B.arena;
	return B.root;
}

static inline int
utf8_trail(unsigned char c) {
	return c >= 0x80 && c <= 0xbf;
}

static size_t
valid_utf8(const char *str, size_t sz) {
	unsigned char c = *str;
	if (c >= 0xc0 && c <= 0xdf) {
		// 110xxxxx  10zzzzzz (0x80 - 0xbf)
		if (sz < 2)
			return 0;
		if (!utf8_trail(str[1]))
			return 0;
		return 2;
	} else if (c >= 0xe0 && c <= 0xef) {
		// 1110xxxx  10zzzzzz 10zzzzzz
		if (sz < 3)
			return 0;
		if (!utf8_trail(str[1]) || !utf8_trail(str[2]))
			return 0;
		return 3;
	} else if (c >= 0xf0 && c <= 0xf7) {
		// 11110xxx  10zzzzzz 10zzzzzz 10zzzzzz
		if (sz < 4)
			return 0;
		if (!utf8_trail(str[1]) || !utf8_trail(str[2]) || !utf8_trail(str[3]))
			return 0;
		return 4;
	} else if (c < 128)
		return 1;
	return 0;
}

typedef struct SBuf {
	char *data;
	size_t len;
	size_t cap;
} SBuf;

static int
sb_reserve(SBuf *sb, size_t need) {
	if (sb->len + need <= sb->cap)
		return 0;
	size_t ncap = sb->cap == 0 ? 256 : sb->cap * 2;
	while (ncap < sb->len + need)
		ncap *= 2;
	char *nd = (char *)realloc(sb->data, ncap);
	if (nd == NULL)
		return -1;
	sb->data = nd;
	sb->cap = ncap;
	return 0;
}

static int
sb_append(SBuf *sb, const char *p, size_t n) {
	if (sb_reserve(sb, n) != 0)
		return -1;
	memcpy(sb->data + sb->len, p, n);
	sb->len += n;
	return 0;
}

static void
add_hex(SBuf *b, unsigned char c) {
	char tmp[5];
	snprintf(tmp, sizeof(tmp), "\\x%02X", c);
	sb_append(b, tmp, 4);
}

static char *
lquote(const char *str, size_t sz) {
	SBuf b = { 0 };
	size_t i;
	sb_append(&b, "\"", 1);
	for (i=0;i<sz;i++) {
		unsigned char c = (unsigned char)str[i];
		if (c < 32) {
			switch (c) {
			case 0:
				sb_append(&b, "\\0", 2);
				break;
			case '\t':
				sb_append(&b, "\\t", 2);
				break;
			case '\n':
				sb_append(&b, "\\n", 2);
				break;
			case '\r':
				sb_append(&b, "\\r", 2);
				break;
			default:
				add_hex(&b, c);
				break;
			}
		} else if (c == '"') {
			sb_append(&b, "\\\"", 2);
		} else if (c == '\\') {
			sb_append(&b, "\\\\", 2);
		} else if (c >= 128) {
			// check utf-8
			int n = valid_utf8(str+i, sz-i);
			if (n == 0) {
				add_hex(&b, c);
			} else {
				sb_append(&b,str+i,n);
				i += n-1;
			}
		} else {
			sb_append(&b, (char *)&c, 1);
		}
	}
	sb_append(&b, "\"", 1);
	sb_append(&b, "", 1);
	return b.data;
}

static int
serialize_node(const PulseDatalist *n, SBuf *sb, int depth, const PulseDatalist **path, int path_len, int entries_only);

static int
serialize_scalar(const PulseDatalist *n, SBuf *sb) {
	switch (n->type) {
	case PULSE_DATALIST_TYPE_NIL:
		return sb_append(sb, "nil", 3);
	case PULSE_DATALIST_TYPE_BOOL:
		return sb_append(sb, n->b ? "true" : "false", n->b ? 4 : 5);
	case PULSE_DATALIST_TYPE_INT: {
		char tmp[32];
		int r = snprintf(tmp, sizeof(tmp), "%" PRId64, n->i);
		return sb_append(sb, tmp, (size_t)r);
	}
	case PULSE_DATALIST_TYPE_DOUBLE: {
		char tmp[64];
		snprintf(tmp, sizeof(tmp), "%g", n->d);
		if (strchr(tmp, '.') == NULL && strchr(tmp, 'e') == NULL && strchr(tmp, 'E') == NULL
			&& strcmp(tmp, "inf") != 0 && strcmp(tmp, "-inf") != 0
			&& strcmp(tmp, "nan") != 0 && strcmp(tmp, "-nan") != 0) {
			sb_append(sb, tmp, strlen(tmp));
			return sb_append(sb, ".0", 2);
		}
		return sb_append(sb, tmp, strlen(tmp));
	}
	case PULSE_DATALIST_TYPE_STRING: {
		char *q = lquote(n->s, n->s_len);
		if (q == NULL)
			return -1;
		int r = sb_append(sb, q, strlen(q));
		free(q);
		return r;
	}
	default:
		return -1;
	}
}

static int
in_path(const PulseDatalist **path, int path_len, const PulseDatalist *n) {
	int i;
	for (i = 0; i < path_len; i++) {
		if (path[i] == n)
			return 1;
	}
	return 0;
}

static int
emit_entry_list(const PulseDatalist *n, SBuf *sb, int depth, const PulseDatalist **path, int path_len, int *sep) {
	size_t i;
	for (i = 0; i < n->entry_count; i++) {
		if (*sep && sb_append(sb, " ", 1) != 0)
			return -1;
		*sep = 1;
		const char *key = n->entries[i].key;
		if (sb_append(sb, key, strlen(key)) != 0)
			return -1;
		if (sb_append(sb, " : ", 3) != 0)
			return -1;
		const PulseDatalist *ev = n->entries[i].value;
		if (serialize_node(ev, sb, depth + 1, path, path_len + 1, ev->type == PULSE_DATALIST_TYPE_MIXED ? 1 : 0) != 0)
			return -1;
		if (ev->type == PULSE_DATALIST_TYPE_MIXED && ev->item_count > 0) {
			size_t j;
			for (j = 0; j < ev->item_count; j++) {
				if (sb_append(sb, " ", 1) != 0)
					return -1;
				if (sb_append(sb, key, strlen(key)) != 0)
					return -1;
				if (sb_append(sb, " : ", 3) != 0)
					return -1;
				if (serialize_node(ev->items[j], sb, depth + 1, path, path_len + 1, 0) != 0)
					return -1;
			}
		}
	}
	return 0;
}

static int
emit_item_list(const PulseDatalist *n, SBuf *sb, int depth, const PulseDatalist **path, int path_len, int *sep) {
	size_t i;
	for (i = 0; i < n->item_count; i++) {
		if (*sep && sb_append(sb, " ", 1) != 0)
			return -1;
		*sep = 1;
		if (serialize_node(n->items[i], sb, depth + 1, path, path_len + 1, 0) != 0)
			return -1;
	}
	return 0;
}

static int
serialize_node(const PulseDatalist *n, SBuf *sb, int depth, const PulseDatalist **path, int path_len, int entries_only) {
	if (n->type != PULSE_DATALIST_TYPE_LIST && n->type != PULSE_DATALIST_TYPE_MAP && n->type != PULSE_DATALIST_TYPE_MIXED)
		return serialize_scalar(n, sb);
	if (in_path(path, path_len, n)) {
		set_error("Cycle in datalist tree");
		return -1;
	}
	if (depth > MAX_DEPTH) {
		set_error("too many layers");
		return -1;
	}
	path[path_len] = n;
	if (sb_append(sb, "{", 1) != 0)
		return -1;
	int sep = 0;
	if (emit_entry_list(n, sb, depth, path, path_len + 1, &sep) != 0)
		return -1;
	if (!entries_only) {
		if (emit_item_list(n, sb, depth, path, path_len + 1, &sep) != 0)
			return -1;
	}
	return sb_append(sb, "}", 1);
}

static int
serialize_root(const PulseDatalist *n, SBuf *sb) {
	if (n->type != PULSE_DATALIST_TYPE_LIST && n->type != PULSE_DATALIST_TYPE_MAP && n->type != PULSE_DATALIST_TYPE_MIXED)
		return serialize_scalar(n, sb);
	const PulseDatalist **path = (const PulseDatalist **)malloc(sizeof(const PulseDatalist *) * (MAX_DEPTH + 1));
	if (path == NULL) {
		set_error("Out of memory");
		return -1;
	}
	path[0] = n;
	int sep = 0;
	int r = emit_entry_list(n, sb, 0, path, 1, &sep);
	if (r == 0)
		r = emit_item_list(n, sb, 0, path, 1, &sep);
	free(path);
	return r;
}

static int
is_container(const PulseDatalist *n) {
	return n->type == PULSE_DATALIST_TYPE_LIST
		|| n->type == PULSE_DATALIST_TYPE_MAP
		|| n->type == PULSE_DATALIST_TYPE_MIXED;
}

PulseDatalist *
pulse_datalist_create_from_text(const char *text, size_t len) {
	if (text == NULL) {
		set_error("Invalid argument");
		return NULL;
	}
	return parse_text_internal(text, len, 0);
}

PulseDatalist *
pulse_datalist_create_from_text_list(const char *text, size_t len) {
	if (text == NULL) {
		set_error("Invalid argument");
		return NULL;
	}
	return parse_text_internal(text, len, 1);
}

PulseDatalist *
pulse_datalist_create_from_text_file(const char *path) {
	if (path == NULL) {
		set_error("Invalid argument");
		return NULL;
	}
	FILE *f = fopen(path, "rb");
	if (f == NULL) {
		set_error("Cannot open file");
		return NULL;
	}
	if (fseek(f, 0, SEEK_END) != 0) {
		fclose(f);
		set_error("Cannot read file");
		return NULL;
	}
	long fsz = ftell(f);
	if (fsz < 0) {
		fclose(f);
		set_error("Cannot read file");
		return NULL;
	}
	rewind(f);
	char *buf = (char *)malloc(fsz > 0 ? (size_t)fsz : 1);
	if (buf == NULL) {
		fclose(f);
		set_error("Out of memory");
		return NULL;
	}
	size_t rd = fread(buf, 1, (size_t)fsz, f);
	fclose(f);
	PulseDatalist *n = parse_text_internal(buf, rd, 0);
	free(buf);
	return n;
}

void
pulse_datalist_addref(PulseDatalist *node) {
	if (node == NULL)
		return;
	node->root->ref_count++;
}

void
pulse_datalist_release(PulseDatalist *node) {
	if (node == NULL)
		return;
	PulseDatalist *root = node->root;
	if (--root->ref_count <= 0)
		arena_free_all(&root->arena);
}

void
pulse_datalist_free_string(char *str) {
	free(str);
}

static const PulseDatalist *
resolve(const PulseDatalist *node, const char *key) {
	if (node == NULL)
		return NULL;
	if (key == NULL)
		return node;
	size_t i;
	for (i = 0; i < node->entry_count; i++) {
		if (strcmp(node->entries[i].key, key) == 0)
			return node->entries[i].value;
	}
	return NULL;
}

EPulseDatalistType
pulse_datalist_get_type(const PulseDatalist *node, const char *key) {
	const PulseDatalist *v = resolve(node, key);
	return v != NULL ? v->type : PULSE_DATALIST_TYPE_NIL;
}

bool
pulse_datalist_has(const PulseDatalist *node, const char *key) {
	if (node == NULL)
		return false;
	if (key == NULL)
		return is_container(node);
	return resolve(node, key) != NULL;
}

bool
pulse_datalist_get_bool(const PulseDatalist *node, const char *key, bool default_value) {
	const PulseDatalist *v = resolve(node, key);
	if (v == NULL)
		return default_value;
	switch (v->type) {
	case PULSE_DATALIST_TYPE_BOOL:
		return v->b;
	case PULSE_DATALIST_TYPE_INT:
		return v->i != 0;
	case PULSE_DATALIST_TYPE_DOUBLE:
		return v->d != 0.0;
	default:
		return default_value;
	}
}

int64_t
pulse_datalist_get_int(const PulseDatalist *node, const char *key, int64_t default_value) {
	const PulseDatalist *v = resolve(node, key);
	if (v == NULL)
		return default_value;
	switch (v->type) {
	case PULSE_DATALIST_TYPE_INT:
		return v->i;
	case PULSE_DATALIST_TYPE_DOUBLE:
		return (int64_t)v->d;
	case PULSE_DATALIST_TYPE_BOOL:
		return v->b ? 1 : 0;
	default:
		return default_value;
	}
}

double
pulse_datalist_get_double(const PulseDatalist *node, const char *key, double default_value) {
	const PulseDatalist *v = resolve(node, key);
	if (v == NULL)
		return default_value;
	switch (v->type) {
	case PULSE_DATALIST_TYPE_DOUBLE:
		return v->d;
	case PULSE_DATALIST_TYPE_INT:
		return (double)v->i;
	case PULSE_DATALIST_TYPE_BOOL:
		return v->b ? 1.0 : 0.0;
	default:
		return default_value;
	}
}

const char *
pulse_datalist_get_string(const PulseDatalist *node, const char *key, const char *default_value) {
	const PulseDatalist *v = resolve(node, key);
	if (v == NULL)
		return default_value;
	if (v->type != PULSE_DATALIST_TYPE_STRING)
		return default_value;
	return v->s;
}

PulseDatalist *
pulse_datalist_get_obj(const PulseDatalist *node, const char *key) {
	const PulseDatalist *v = resolve(node, key);
	if (v == NULL)
		return NULL;
	if (!is_container(v))
		return NULL;
	return (PulseDatalist *)v;
}

size_t
pulse_datalist_count(const PulseDatalist *node) {
	return node != NULL ? node->item_count : 0;
}

PulseDatalist *
pulse_datalist_get(const PulseDatalist *node, size_t index) {
	if (node == NULL || index >= node->item_count)
		return NULL;
	return node->items[index];
}

size_t
pulse_datalist_object_count(const PulseDatalist *node) {
	return node != NULL ? node->entry_count : 0;
}

const char *
pulse_datalist_object_key(const PulseDatalist *node, size_t index) {
	if (node == NULL || index >= node->entry_count)
		return NULL;
	return node->entries[index].key;
}

PulseDatalist *
pulse_datalist_object_value(const PulseDatalist *node, size_t index) {
	if (node == NULL || index >= node->entry_count)
		return NULL;
	return node->entries[index].value;
}

char *
pulse_datalist_quote(const char *str, size_t len) {
	if (str == NULL) {
		set_error("Invalid argument");
		return NULL;
	}
	char *q = lquote(str, len);
	if (q == NULL) {
		set_error("Out of memory");
		return NULL;
	}
	return q;
}

char *
pulse_datalist_to_text(const PulseDatalist *node, size_t *out_len) {
	if (node == NULL) {
		set_error("Invalid argument");
		return NULL;
	}
	SBuf sb = { 0 };
	int r;
	if (is_container(node)) {
		r = serialize_root(node, &sb);
	} else {
		const PulseDatalist **path = (const PulseDatalist **)malloc(sizeof(const PulseDatalist *) * (MAX_DEPTH + 1));
		if (path == NULL) {
			set_error("Out of memory");
			free(sb.data);
			return NULL;
		}
		r = serialize_node(node, &sb, 0, path, 0, 0);
		free(path);
	}
	if (r != 0) {
		free(sb.data);
		return NULL;
	}
	if (sb_append(&sb, "", 1) != 0) {
		free(sb.data);
		set_error("Out of memory");
		return NULL;
	}
	if (out_len != NULL)
		*out_len = sb.len - 1;
	return sb.data;
}

const char *
pulse_datalist_last_error(void) {
	return g_last_error;
}
