#include "pulse_datalist.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool same_value(const PulseDatalist* a, const PulseDatalist* b) {
    EPulseDatalistType t = pulse_datalist_get_type(a, nullptr);
    if (t != pulse_datalist_get_type(b, nullptr))
        return false;
    switch (t) {
    case PULSE_DATALIST_TYPE_NIL:
        return true;
    case PULSE_DATALIST_TYPE_BOOL:
        return pulse_datalist_get_bool(a, nullptr, false) == pulse_datalist_get_bool(b, nullptr, false);
    case PULSE_DATALIST_TYPE_INT:
        return pulse_datalist_get_int(a, nullptr, 0) == pulse_datalist_get_int(b, nullptr, 0);
    case PULSE_DATALIST_TYPE_DOUBLE:
        return fabs(pulse_datalist_get_double(a, nullptr, 0.0) - pulse_datalist_get_double(b, nullptr, 0.0)) < 1e-12;
    case PULSE_DATALIST_TYPE_STRING:
        return strcmp(pulse_datalist_get_string(a, nullptr, ""), pulse_datalist_get_string(b, nullptr, "")) == 0;
    default:
        break;
    }
    if (pulse_datalist_count(a) != pulse_datalist_count(b))
        return false;
    size_t i;
    for (i = 0; i < pulse_datalist_count(a); i++) {
        if (!same_value(pulse_datalist_get(a, i), pulse_datalist_get(b, i)))
            return false;
    }
    if (pulse_datalist_object_count(a) != pulse_datalist_object_count(b))
        return false;
    for (i = 0; i < pulse_datalist_object_count(a); i++) {
        if (strcmp(pulse_datalist_object_key(a, i), pulse_datalist_object_key(b, i)) != 0)
            return false;
        if (!same_value(pulse_datalist_object_value(a, i), pulse_datalist_object_value(b, i)))
            return false;
    }
    return true;
}

static void test_roundtrip(const char* text) {
    PulseDatalist* a = pulse_datalist_create_from_text(text, strlen(text));
    assert(a != nullptr);
    size_t len = 0;
    char* out = pulse_datalist_to_text(a, &len);
    assert(out != nullptr);
    assert(len == strlen(out));
    PulseDatalist* b = pulse_datalist_create_from_text(out, len);
    assert(b != nullptr);
    assert(same_value(a, b));
    pulse_datalist_release(b);
    pulse_datalist_free_string(out);
    pulse_datalist_release(a);
}

static void test_to_text_exact(void) {
    PulseDatalist* a = pulse_datalist_create_from_text("x : 1\ny : true\n", 15);
    assert(a != nullptr);
    size_t len = 0;
    char* out = pulse_datalist_to_text(a, &len);
    assert(out != nullptr);
    assert(strcmp(out, "x : 1 y : true") == 0);
    pulse_datalist_free_string(out);
    pulse_datalist_release(a);

    a = pulse_datalist_create_from_text("", 0);
    assert(a != nullptr);
    out = pulse_datalist_to_text(a, nullptr);
    assert(out != nullptr);
    assert(strcmp(out, "") == 0);
    pulse_datalist_free_string(out);
    pulse_datalist_release(a);
}

static void test_quote(void) {
    char* q = pulse_datalist_quote("hello\\\tworld\n\1\0", 15);
    assert(q != nullptr);
    assert(strcmp(q, "\"hello\\\\\\tworld\\n\\x01\\0\"") == 0);
    pulse_datalist_free_string(q);
    q = pulse_datalist_quote("plain", 5);
    assert(q != nullptr);
    assert(strcmp(q, "\"plain\"") == 0);
    pulse_datalist_free_string(q);
}

static void test_file_roundtrip(void) {
    PulseDatalist* a = pulse_datalist_create_from_text_file("tests/datalist/data/sample.dl");
    assert(a != nullptr);
    size_t len = 0;
    char* out = pulse_datalist_to_text(a, &len);
    assert(out != nullptr);
    PulseDatalist* b = pulse_datalist_create_from_text(out, len);
    assert(b != nullptr);
    assert(same_value(a, b));
    pulse_datalist_release(b);
    pulse_datalist_free_string(out);
    pulse_datalist_release(a);
}

int main() {
    test_roundtrip("x : 1\ny : 2\n");
    test_roundtrip("hello \"world\"\n0x1p+0\n2\n0x3\nnil\ntrue\nfalse\n");
    test_roundtrip("---\nx : hello\ny : world\n---\n1 2 3\n");
    test_roundtrip("x :\n\t1 2 3\ny :\n\tdict : \"hello world\"\nz : { foobar }\n");
    test_roundtrip("multi : { x : 1 }\nmulti : { x : 2 }\nmulti : { x : 3 }\n");
    test_roundtrip("--- $obj\nx : 1\n---\ny : [ 1, 2, 3 ]\n");
    test_roundtrip("x : \"a b\"\ny : { 1 2 { a : 3 } }\n");
    test_to_text_exact();
    test_quote();
    test_file_roundtrip();
    printf("test_serialize ok\n");
    return 0;
}