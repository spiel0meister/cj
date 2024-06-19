/*
 * Copyright 2024 Žan Sovič <soviczan7@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef CJ_H
#define CJ_H
#include <stdio.h>
#include <stdbool.h>

typedef struct CJ CJ;

#ifndef CJ_MAX_SCOPES
    #define CJ_MAX_SCOPES 256
#endif

CJ* cj_new(FILE* sink);
void cj_delete(CJ* cj);

void cj_begin_object(CJ* cj);
void cj_end_object(CJ* cj);
void cj_start_array(CJ* cj);
void cj_end_array(CJ* cj);

void cj_key(CJ* cj, const char* cstr);
void cj_key_sized(CJ* cj, size_t n, const char cstr[n]);

void cj_string(CJ* cj, const char* cstr);
void cj_string_sized(CJ* cj, size_t n, const char cstr[n]);
void cj_number(CJ* cj, long long int n);
void cj_float(CJ* cj, long double f, size_t precision);
void cj_bool(CJ* cj, bool bol);
void cj_null(CJ* cj);

#endif // CJ_H

#ifdef CJ_IMPLEMENTATION
#include <assert.h>

typedef enum {
    CJ_OBJECT,
    CJ_ARRAY
}CJScopeType;

typedef struct {
    CJScopeType type;
    bool start;
    bool key;
}CJScope;

typedef struct CJ {
    FILE* sink;

    CJScope scopes[CJ_MAX_SCOPES];
    size_t scope_count;
}CJ;

static CJScope* cj_scope_top(CJ* cj) {
    assert(cj->scope_count > 0);
    return &cj->scopes[cj->scope_count - 1];
}

CJ* cj_new(FILE* sink) {
    CJ* cj = calloc(1, sizeof(*cj));
    cj->sink = sink;
    return cj;
}

void cj_delete(CJ* cj) {
    free(cj);
}

void cj_begin_object(CJ* cj) {
    assert(cj->scope_count < CJ_MAX_SCOPES);

    fprintf(cj->sink, "{");
    cj->scopes[cj->scope_count++] = (CJScope) { .type = CJ_OBJECT, .start = true, .key = false };
}

void cj_end_object(CJ* cj) {
    CJScope* top = cj_scope_top(cj);
    assert(top->type == CJ_OBJECT);

    fprintf(cj->sink, "}");

    if (cj->scope_count > 0) {
        CJScope* top = cj_scope_top(cj);
        if (top->type == CJ_OBJECT) {
            top->key = false;
        }
    }
}

void cj_start_array(CJ* cj) {
    assert(cj->scope_count < CJ_MAX_SCOPES);

    fprintf(cj->sink, "[");
    cj->scopes[cj->scope_count++] = (CJScope) { .type = CJ_OBJECT, .start = true, .key = false };
}

void cj_end_array(CJ* cj) {
    CJScope* top = cj_scope_top(cj);
    assert(top->type == CJ_ARRAY);

    fprintf(cj->sink, "]");

    if (cj->scope_count > 0) {
        CJScope* top = cj_scope_top(cj);
        if (top->type == CJ_OBJECT) {
            top->key = false;
        }
    }
}

void cj_key(CJ* cj, const char* cstr) {
    CJScope* top = cj_scope_top(cj);
    assert(top->type == CJ_OBJECT);
    assert(!top->key);

    if (!top->start) {
        fprintf(cj->sink, ",");
    } else {
        top->start = false;
    }

    fprintf(cj->sink, "\"%s\":", cstr);
    top->key = true;
}

void cj_key_sized(CJ* cj, size_t n, const char cstr[n]) {
    CJScope* top = cj_scope_top(cj);
    assert(top->type == CJ_OBJECT);
    assert(!top->key);

    if (!top->start) {
        fprintf(cj->sink, ",");
    } else {
        top->start = false;
    }

    fprintf(cj->sink, "\"%.*s\"", (int)n, cstr);

    if (top->type == CJ_OBJECT) {
        fprintf(cj->sink, ":");
        top->key = true;
    }
}

void cj_bool(CJ* cj, bool bol) {
    CJScope* top = cj_scope_top(cj);

    if (top->type == CJ_ARRAY && !top->start) {
        fprintf(cj->sink, ",");
    }

    if (bol) {
        fprintf(cj->sink, "true");
    } else {
        fprintf(cj->sink, "false");
    }

    if (top->type == CJ_OBJECT) {
        top->key = false;
    }
}

void cj_string(CJ* cj, const char* cstr) {
    CJScope* top = cj_scope_top(cj);

    if (top->type == CJ_ARRAY && !top->start) {
        fprintf(cj->sink, ",");
    }

    fprintf(cj->sink, "\"%s\"", cstr);

    if (top->type == CJ_OBJECT) {
        top->key = false;
    }
}

void cj_string_sized(CJ* cj, size_t n, const char cstr[n]) {
    CJScope* top = cj_scope_top(cj);

    if (top->type == CJ_ARRAY && !top->start) {
        fprintf(cj->sink, ",");
    }

    fprintf(cj->sink, "\"%.*s\"", (int)n, cstr);

    if (top->type == CJ_OBJECT) {
        top->key = false;
    }
}

void cj_number(CJ* cj, long long int n) {
    CJScope* top = cj_scope_top(cj);

    if (top->type == CJ_ARRAY) {
        if (!top->start) {
            fprintf(cj->sink, ",");
        } else {
            top->start = false;
        }
    }

    fprintf(cj->sink, "%lld", n);

    if (top->type == CJ_OBJECT) {
        top->key = false;
    }
}

void cj_float(CJ* cj, long double f, size_t precision) {
    CJScope* top = cj_scope_top(cj);

    if (top->type == CJ_ARRAY) {
        if (!top->start) {
            fprintf(cj->sink, ",");
        } else {
            top->start = false;
        }
    }

    fprintf(cj->sink, "%.*Lf", (int)precision, f);

    if (top->type == CJ_OBJECT) {
        top->key = false;
    }
}

void cj_null(CJ* cj) {
    CJScope* top = cj_scope_top(cj);

    if (top->type == CJ_ARRAY) {
        if (!top->start) {
            fprintf(cj->sink, ",");
        } else {
            top->start = false;
        }
    }

    fprintf(cj->sink, "null");

    if (top->type == CJ_OBJECT) {
        top->key = false;
    }
}


#endif