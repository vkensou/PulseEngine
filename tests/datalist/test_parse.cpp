#include "pulse_datalist.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void test_map(void) {
    static const char text[] = "x : 1\ny : 2\n100 : number\n";
    PulseDatalist* v = pulse_datalist_create_from_text(text, sizeof(text) - 1);
    assert(v != nullptr);
    assert(pulse_datalist_get_type(v, nullptr) == PULSE_DATALIST_TYPE_MAP);
    assert(pulse_datalist_get_int(v, "x", 0) == 1);
    assert(pulse_datalist_get_int(v, "y", 0) == 2);
    assert(strcmp(pulse_datalist_get_string(v, "100", ""), "number") == 0);
    assert(pulse_datalist_has(v, "x"));
    assert(!pulse_datalist_has(v, "missing"));
    assert(pulse_datalist_object_count(v) == 3);
    assert(strcmp(pulse_datalist_object_key(v, 0), "x") == 0);
    assert(strcmp(pulse_datalist_object_key(v, 1), "y") == 0);
    assert(strcmp(pulse_datalist_object_key(v, 2), "100") == 0);
    assert(pulse_datalist_get_int(pulse_datalist_object_value(v, 0), nullptr, -1) == 1);
    assert(pulse_datalist_get_int(pulse_datalist_object_value(v, 1), nullptr, -1) == 2);
    assert(pulse_datalist_get_type(v, "missing") == PULSE_DATALIST_TYPE_NIL);
    pulse_datalist_release(v);
}

static void test_list(void) {
    static const char text[] = "hello \"world\"\n0x1p+0\n2\n0x3\nnil\ntrue\nfalse\n";
    PulseDatalist* v = pulse_datalist_create_from_text(text, sizeof(text) - 1);
    assert(v != nullptr);
    assert(pulse_datalist_get_type(v, nullptr) == PULSE_DATALIST_TYPE_LIST);
    assert(pulse_datalist_count(v) == 8);
    assert(pulse_datalist_get_type(pulse_datalist_get(v, 0), nullptr) == PULSE_DATALIST_TYPE_STRING);
    assert(strcmp(pulse_datalist_get_string(pulse_datalist_get(v, 0), nullptr, ""), "hello") == 0);
    assert(strcmp(pulse_datalist_get_string(pulse_datalist_get(v, 1), nullptr, ""), "world") == 0);
    assert(fabs(pulse_datalist_get_double(pulse_datalist_get(v, 2), nullptr, 0.0) - 1.0) < 1e-12);
    assert(pulse_datalist_get_int(pulse_datalist_get(v, 3), nullptr, -1) == 2);
    assert(pulse_datalist_get_int(pulse_datalist_get(v, 4), nullptr, -1) == 3);
    assert(pulse_datalist_get_type(pulse_datalist_get(v, 5), nullptr) == PULSE_DATALIST_TYPE_NIL);
    assert(pulse_datalist_get_bool(pulse_datalist_get(v, 6), nullptr, false));
    assert(!pulse_datalist_get_bool(pulse_datalist_get(v, 7), nullptr, true));
    assert(pulse_datalist_get(v, 8) == nullptr);
    pulse_datalist_release(v);
}

static void test_sections(void) {
    static const char text[] = "---\nx : hello\ny : world\n---\n1 2 3\n";
    PulseDatalist* v = pulse_datalist_create_from_text(text, sizeof(text) - 1);
    assert(v != nullptr);
    assert(pulse_datalist_get_type(v, nullptr) == PULSE_DATALIST_TYPE_LIST);
    assert(pulse_datalist_count(v) == 2);
    PulseDatalist* s0 = pulse_datalist_get(v, 0);
    PulseDatalist* s1 = pulse_datalist_get(v, 1);
    assert(pulse_datalist_get_type(s0, nullptr) == PULSE_DATALIST_TYPE_MAP);
    assert(strcmp(pulse_datalist_get_string(s0, "x", ""), "hello") == 0);
    assert(strcmp(pulse_datalist_get_string(s0, "y", ""), "world") == 0);
    assert(pulse_datalist_get_type(s1, nullptr) == PULSE_DATALIST_TYPE_LIST);
    assert(pulse_datalist_count(s1) == 3);
    assert(pulse_datalist_get_int(pulse_datalist_get(s1, 0), nullptr, -1) == 1);
    pulse_datalist_release(v);
}

static void test_indent_and_braces(void) {
    static const char text[] = "x :\n\t1 2 3\ny :\n\tdict : \"hello world\"\nz : { foobar }\n";
    PulseDatalist* v = pulse_datalist_create_from_text(text, sizeof(text) - 1);
    assert(v != nullptr);
    assert(pulse_datalist_get_type(v, nullptr) == PULSE_DATALIST_TYPE_MAP);
    PulseDatalist* x = pulse_datalist_get_obj(v, "x");
    PulseDatalist* y = pulse_datalist_get_obj(v, "y");
    PulseDatalist* z = pulse_datalist_get_obj(v, "z");
    assert(x != nullptr && y != nullptr && z != nullptr);
    assert(pulse_datalist_get_type(x, nullptr) == PULSE_DATALIST_TYPE_LIST);
    assert(pulse_datalist_count(x) == 3);
    assert(pulse_datalist_get_int(pulse_datalist_get(x, 2), nullptr, -1) == 3);
    assert(pulse_datalist_object_count(y) == 1);
    assert(strcmp(pulse_datalist_get_string(y, "dict", ""), "hello world") == 0);
    assert(pulse_datalist_get_type(z, nullptr) == PULSE_DATALIST_TYPE_LIST);
    assert(pulse_datalist_count(z) == 1);
    assert(strcmp(pulse_datalist_get_string(pulse_datalist_get(z, 0), nullptr, ""), "foobar") == 0);
    pulse_datalist_release(v);
}

static void test_tag_shared(void) {
    static const char text[] = "--- &1\nx : 1\n--- *1\n";
    PulseDatalist* v = pulse_datalist_create_from_text(text, sizeof(text) - 1);
    assert(v != nullptr);
    assert(pulse_datalist_count(v) == 2);
    PulseDatalist* a = pulse_datalist_get(v, 0);
    PulseDatalist* b = pulse_datalist_get(v, 1);
    assert(a == b);
    assert(pulse_datalist_get_type(a, nullptr) == PULSE_DATALIST_TYPE_MAP);
    assert(pulse_datalist_get_int(a, "x", -1) == 1);
    pulse_datalist_release(v);
}

static void test_tag_forward(void) {
    static const char text[] = "--- *1\n--- &1\nx : 1\n";
    PulseDatalist* v = pulse_datalist_create_from_text(text, sizeof(text) - 1);
    assert(v != nullptr);
    assert(pulse_datalist_count(v) == 2);
    PulseDatalist* a = pulse_datalist_get(v, 0);
    PulseDatalist* b = pulse_datalist_get(v, 1);
    assert(a == b);
    assert(pulse_datalist_get_type(a, nullptr) == PULSE_DATALIST_TYPE_MAP);
    assert(pulse_datalist_get_int(a, "x", -1) == 1);
    pulse_datalist_release(v);
}

static void test_tag_cycle(void) {
    static const char text[] = "--- &1\nx : *1\n";
    PulseDatalist* v = pulse_datalist_create_from_text(text, sizeof(text) - 1);
    assert(v != nullptr);
    PulseDatalist* a = pulse_datalist_get(v, 0);
    assert(pulse_datalist_get_type(a, nullptr) == PULSE_DATALIST_TYPE_MAP);
    assert(pulse_datalist_get_obj(a, "x") == a);
    pulse_datalist_release(v);
}

static void test_multi_key(void) {
    static const char text[] = "multi : { x : 1 }\nmulti : { x : 2 }\nmulti : { x : 3 }\n";
    PulseDatalist* v = pulse_datalist_create_from_text(text, sizeof(text) - 1);
    assert(v != nullptr);
    PulseDatalist* multi = pulse_datalist_get_obj(v, "multi");
    assert(multi != nullptr);
    assert(pulse_datalist_get_type(multi, nullptr) == PULSE_DATALIST_TYPE_MIXED);
    assert(pulse_datalist_get_int(multi, "x", -1) == 1);
    assert(pulse_datalist_count(multi) == 2);
    assert(pulse_datalist_get_int(pulse_datalist_get(multi, 0), "x", -1) == 2);
    assert(pulse_datalist_get_int(pulse_datalist_get(multi, 1), "x", -1) == 3);
    assert(strcmp(pulse_datalist_object_key(multi, 0), "x") == 0);
    pulse_datalist_release(v);
}

static void test_parse_list(void) {
    static const char text[] = "x : 1\ny : 2\n";
    PulseDatalist* v = pulse_datalist_create_from_text_list(text, sizeof(text) - 1);
    assert(v != nullptr);
    assert(pulse_datalist_get_type(v, nullptr) == PULSE_DATALIST_TYPE_LIST);
    assert(pulse_datalist_count(v) == 4);
    assert(strcmp(pulse_datalist_get_string(pulse_datalist_get(v, 0), nullptr, ""), "x") == 0);
    assert(pulse_datalist_get_int(pulse_datalist_get(v, 1), nullptr, -1) == 1);
    assert(strcmp(pulse_datalist_get_string(pulse_datalist_get(v, 2), nullptr, ""), "y") == 0);
    assert(pulse_datalist_get_int(pulse_datalist_get(v, 3), nullptr, -1) == 2);
    pulse_datalist_release(v);
}

static void test_converter(void) {
    static const char text[] = "--- $obj\nx : 1\n---\ny : [ 1, 2, 3 ]\n";
    PulseDatalist* v = pulse_datalist_create_from_text(text, sizeof(text) - 1);
    assert(v != nullptr);
    assert(pulse_datalist_get_type(v, nullptr) == PULSE_DATALIST_TYPE_LIST);
    assert(pulse_datalist_count(v) == 2);
    PulseDatalist* conv = pulse_datalist_get(v, 0);
    assert(pulse_datalist_get_type(conv, nullptr) == PULSE_DATALIST_TYPE_LIST);
    assert(pulse_datalist_count(conv) == 2);
    assert(strcmp(pulse_datalist_get_string(pulse_datalist_get(conv, 0), nullptr, ""), "obj") == 0);
    PulseDatalist* inner = pulse_datalist_get(conv, 1);
    assert(pulse_datalist_get_type(inner, nullptr) == PULSE_DATALIST_TYPE_MAP);
    assert(pulse_datalist_get_int(inner, "x", -1) == 1);
    PulseDatalist* bracket = pulse_datalist_get(v, 1);
    assert(pulse_datalist_get_type(bracket, nullptr) == PULSE_DATALIST_TYPE_MAP);
    PulseDatalist* arr = pulse_datalist_get_obj(bracket, "y");
    assert(arr != nullptr);
    assert(pulse_datalist_get_type(arr, nullptr) == PULSE_DATALIST_TYPE_LIST);
    assert(pulse_datalist_count(arr) == 3);
    assert(pulse_datalist_get_int(pulse_datalist_get(arr, 1), nullptr, -1) == 2);
    pulse_datalist_release(v);
}

static void test_errors(void) {
    const char* bad[] = {
        "x : 1\nx : 2\n",
        "--- *1\nhello\n",
        "x : { 1 2 }\ny : ]\n",
        "\"unterminated\n",
        "1 2 3\n---\nx : 1\n",
    };
    size_t i;
    for (i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        PulseDatalist* v = pulse_datalist_create_from_text(bad[i], strlen(bad[i]));
        assert(v == nullptr);
        assert(strlen(pulse_datalist_last_error()) > 0);
    }
}

static void test_getters(void) {
    static const char text[] = "i : 3\nd : 1.5\nb : true\ns : hi\n";
    PulseDatalist* v = pulse_datalist_create_from_text(text, sizeof(text) - 1);
    assert(v != nullptr);
    assert(pulse_datalist_get_int(v, "d", 0) == 1);
    assert(fabs(pulse_datalist_get_double(v, "i", 0.0) - 3.0) < 1e-12);
    assert(pulse_datalist_get_bool(v, "i", false));
    assert(pulse_datalist_get_int(v, "missing", 7) == 7);
    assert(strcmp(pulse_datalist_get_string(v, "missing", "def"), "def") == 0);
    assert(pulse_datalist_get_obj(v, "missing") == nullptr);
    assert(pulse_datalist_get_type(v, "s") == PULSE_DATALIST_TYPE_STRING);
    pulse_datalist_release(v);
}

static void test_file(void) {
    PulseDatalist* v = pulse_datalist_create_from_text_file("tests/datalist/data/sample.dl");
    assert(v != nullptr);
    assert(pulse_datalist_get_type(v, nullptr) == PULSE_DATALIST_TYPE_MAP);
    assert(strcmp(pulse_datalist_get_string(v, "name", ""), "pulse") == 0);
    assert(pulse_datalist_get_int(v, "version", -1) == 1);
    PulseDatalist* levels = pulse_datalist_get_obj(v, "levels");
    assert(levels != nullptr);
    assert(pulse_datalist_count(levels) == 3);
    PulseDatalist* tags = pulse_datalist_get_obj(v, "tags");
    assert(tags != nullptr);
    assert(strcmp(pulse_datalist_get_string(pulse_datalist_get(tags, 0), nullptr, ""), "a b") == 0);
    assert(strcmp(pulse_datalist_get_string(pulse_datalist_get(tags, 1), nullptr, ""), "c") == 0);
    pulse_datalist_release(v);
    v = pulse_datalist_create_from_text_file("tests/datalist/data/not_exist.dl");
    assert(v == nullptr);
}

static void test_refcount(void) {
    static const char text[] = "x : 1\n";
    PulseDatalist* v = pulse_datalist_create_from_text(text, sizeof(text) - 1);
    assert(v != nullptr);
    pulse_datalist_addref(v);
    pulse_datalist_release(v);
    pulse_datalist_release(v);
}

int main() {
    test_map();
    test_list();
    test_sections();
    test_indent_and_braces();
    test_tag_shared();
    test_tag_forward();
    test_tag_cycle();
    test_multi_key();
    test_parse_list();
    test_converter();
    test_errors();
    test_getters();
    test_file();
    test_refcount();
    printf("test_parse ok\n");
    return 0;
}