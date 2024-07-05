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

typedef void (*CJ_write_t)(FILE* sink, const char* fmt, ...);
typedef struct CJ CJ;

#ifndef CJ_MAX_SCOPES
    #define CJ_MAX_SCOPES 256
#endif

CJ* cj_new(FILE* sink, CJ_write_t write);
void cj_delete(CJ* cj);

const char* cj_get_error(const CJ* cj);

bool cj_begin_object(CJ* cj);
bool cj_end_object(CJ* cj);
bool cj_begin_array(CJ* cj);
bool cj_end_array(CJ* cj);

bool cj_key(CJ* cj, const char* cstr);
bool cj_key_sized(CJ* cj, size_t len, const char cstr[len]);

bool cj_string(CJ* cj, const char* cstr);
bool cj_string_sized(CJ* cj, size_t len, const char cstr[len]);
bool cj_number(CJ* cj, long long int n);
bool cj_float(CJ* cj, long double f, size_t precision);
bool cj_bool(CJ* cj, bool bol);
bool cj_null(CJ* cj);

#endif // CJ_H

#ifdef CJ_IMPLEMENTATION
#include <assert.h>
#include <string.h>

typedef enum {
    CJ_OBJECT,
    CJ_ARRAY
}CJScopeType;

typedef enum {
    CJ_SUCCESS,
    CJ_SYNTAX_ERROR,
    CJ_SCOPE_OVERFLOW,
    CJ_SCOPE_UNDERFLOW
}CJResult;

typedef struct {
    CJScopeType type;
    bool start;
    bool key;
}CJScope;

struct CJ {
    FILE* sink;
    CJ_write_t write;

    CJResult result;
    CJScope scopes[CJ_MAX_SCOPES];
    size_t scope_count;
};

const char* cj_get_error(const CJ* cj) {
    switch (cj->result) {
        case CJ_SYNTAX_ERROR: return "Syntax error";
        case CJ_SCOPE_OVERFLOW: return "Scope overflow";
        case CJ_SCOPE_UNDERFLOW: return "Scope underflow";
        case CJ_SUCCESS: return "No error";
        default: assert(0);
    }
}

static CJScope* cj_scope_top(CJ* cj) {
    if (cj->scope_count == 0) {
        cj->result = CJ_SCOPE_UNDERFLOW;
        return NULL;
    }

    return &cj->scopes[cj->scope_count - 1];
}

static bool cj_maybe_object_key_remove(CJ* cj, CJScope* scope) {
    if (scope->type == CJ_OBJECT) {
        if (!scope->key) {
            cj->result = CJ_SYNTAX_ERROR;
            return false;
        }
        scope->key = false;
    }

    return true;
}

static bool cj_has_error(const CJ* cj) {
    return cj->result != CJ_SUCCESS;
}

CJ* cj_new(FILE* sink, CJ_write_t write) {
    CJ* cj = calloc(1, sizeof(*cj));
    cj->sink = sink;
    cj->write = write;
    return cj;
}

void cj_delete(CJ* cj) {
    free(cj);
}

bool cj_begin_object(CJ* cj) {
    if (cj_has_error(cj)) return false;

    if (cj->scope_count >= CJ_MAX_SCOPES) {
        cj->result = CJ_SCOPE_OVERFLOW;
        return false;
    }
    
    if (cj->scope_count > 0) {
        CJScope* top = cj_scope_top(cj);
        assert(top != NULL);

        if (top->type == CJ_OBJECT) {
            if (!top->key) {
                cj->result = CJ_SYNTAX_ERROR;
                return false;
            }
        }
        else if (top->type == CJ_ARRAY) {
            if (!top->start) cj->write(cj->sink, ",");
            else top->start = false;
        }
        else assert(0);
    }

    cj->write(cj->sink, "{");
    cj->scopes[cj->scope_count++] = (CJScope) { .type = CJ_OBJECT, .start = true, .key = false };

    return true;
}

bool cj_end_object(CJ* cj) {
    if (cj_has_error(cj)) return false;

    CJScope* top = cj_scope_top(cj);
    if (top == NULL) return false;
    if (top->type != CJ_OBJECT) {
        cj->result = CJ_SYNTAX_ERROR;
        return false;
    }

    cj->scope_count--;
    cj->write(cj->sink, "}");

    if (cj->scope_count > 0) {
        CJScope* top = cj_scope_top(cj);
        assert(top != NULL);

        if (!cj_maybe_object_key_remove(cj, top)) return false;
    }

    return true;
}

bool cj_begin_array(CJ* cj) {
    if (cj_has_error(cj)) return false;

    if (cj->scope_count >= CJ_MAX_SCOPES) {
        cj->result = CJ_SCOPE_OVERFLOW;
        return false;
    }

    if (cj->scope_count > 0) {
        CJScope* top = cj_scope_top(cj);
        assert(top != NULL);

        if (top->type == CJ_OBJECT) {
            if (!top->key) {
                cj->result = CJ_SYNTAX_ERROR;
                return false;
            }
        } else if (top->type == CJ_ARRAY) {
            if (!top->start) cj->write(cj->sink, ",");
            else top->start = false;
        }
        else assert(0);
    }

    cj->write(cj->sink, "[");
    cj->scopes[cj->scope_count++] = (CJScope) { .type = CJ_ARRAY, .start = true, .key = false };

    return true;
}

bool cj_end_array(CJ* cj) {
    if (cj_has_error(cj)) return false;

    CJScope* top = cj_scope_top(cj);
    if (top == NULL) return false;
    else if (top->type != CJ_ARRAY) {
        cj->result = CJ_SYNTAX_ERROR;
        return false;
    }

    cj->write(cj->sink, "]");
    cj->scope_count--;

    if (cj->scope_count > 0) {
        CJScope* top = cj_scope_top(cj);
        assert(top != NULL);
        if (top->type == CJ_OBJECT) {
            if (!cj_maybe_object_key_remove(cj, top)) return false;
        }
    }
    
    return true;
}

bool cj_key(CJ* cj, const char* cstr) {
    if (cj_has_error(cj)) return false;

    CJScope* top = cj_scope_top(cj);
    if (top == NULL) return false;
    else if (top->type != CJ_OBJECT || top->key) {
        cj->result = CJ_SYNTAX_ERROR;
        return false;
    }

    if (!top->start) {
        cj->write(cj->sink, ",");
    } else {
        top->start = false;
    }

    cj->write(cj->sink, "\"%s\":", cstr);
    top->key = true;

    return true;
}

bool cj_key_sized(CJ* cj, size_t len, const char cstr[len]) {
    if (cj_has_error(cj)) return false;

    CJScope* top = cj_scope_top(cj);
    if (top == NULL) return false;
    else if (top->type != CJ_OBJECT || top->key) {
        cj->result = CJ_SYNTAX_ERROR;
        return false;
    }

    if (!top->start) {
        cj->write(cj->sink, ",");
    } else {
        top->start = false;
    }

    cj->write(cj->sink, "\"%.*s\"", (int)len, cstr);

    if (top->type == CJ_OBJECT) {
        cj->write(cj->sink, ":");
        top->key = true;
    }

    return true;
}

bool cj_bool(CJ* cj, bool bol) {
    if (cj_has_error(cj)) return false;

    CJScope* top = cj_scope_top(cj);
    if (top == NULL) return false;

    if (top->type == CJ_ARRAY) {
        if (!top->start) {
            cj->write(cj->sink, ",");
        } else {
            top->start = false;
        }
    }

    if (bol) {
        cj->write(cj->sink, "true");
    } else {
        cj->write(cj->sink, "false");
    }

    if (!cj_maybe_object_key_remove(cj, top)) return false;

    return true;
}

bool cj_string(CJ* cj, const char* cstr) {
    if (cj_has_error(cj)) return false;

    CJScope* top = cj_scope_top(cj);
    if (top == NULL) return false;

    if (top->type == CJ_ARRAY) {
        if (!top->start) {
            cj->write(cj->sink, ",");
        } else {
            top->start = false;
        }
    }
    if (!cj_maybe_object_key_remove(cj, top)) return false;

    size_t len = strlen(cstr);
    char buf[len * 2] = {};
    memset(buf, 0, len * 2);
    size_t buf_len = 0;
    for (size_t i = 0; buf_len < len * 2 && i < len; ++i) {
        switch (cstr[i]) {
            case '\n':
                buf[buf_len++] = '\\';
                buf[buf_len++] = 'n';
                break;
            case '"':
                buf[buf_len++] = '\\';
                buf[buf_len++] = '"';
                break;
            case '\t':
                buf[buf_len++] = '\\';
                buf[buf_len++] = 't';
                break;
            case '\r':
                buf[buf_len++] = '\\';
                buf[buf_len++] = 'r';
                break;
            case '\\':
                buf[buf_len++] = '\\';
                buf[buf_len++] = '\\';
                break;
            default:
                buf[buf_len++] = cstr[i];
                break;
        }
    }
    cj->write(cj->sink, "\"%s\"", buf);

    return true;
}

bool cj_string_sized(CJ* cj, size_t len, const char cstr[len]) {
    if (cj_has_error(cj)) return false;

    CJScope* top = cj_scope_top(cj);
    if (top == NULL) return false;

    if (top->type == CJ_ARRAY) {
        if (!top->start) {
            cj->write(cj->sink, ",");
        } else {
            top->start = false;
        }
    }

    char buf[len * 2] = {};
    memset(buf, 0, len * 2);
    size_t buf_len = 0;
    for (size_t i = 0; buf_len < len * 2 && i < len; ++i) {
        switch (cstr[i]) {
            case '\n':
                buf[buf_len++] = '\\';
                buf[buf_len++] = 'n';
                break;
            case '"':
                buf[buf_len++] = '\\';
                buf[buf_len++] = '"';
                break;
            case '\t':
                buf[buf_len++] = '\\';
                buf[buf_len++] = 't';
                break;
            case '\r':
                buf[buf_len++] = '\\';
                buf[buf_len++] = 'r';
                break;
            case '\\':
                buf[buf_len++] = '\\';
                buf[buf_len++] = '\\';
                break;
            default:
                buf[buf_len++] = cstr[i];
                break;
        }
    }
    cj->write(cj->sink, "\"%s\"", buf);

    if (!cj_maybe_object_key_remove(cj, top)) return false;

    return true;
}

bool cj_number(CJ* cj, long long int n) {
    if (cj_has_error(cj)) return false;

    CJScope* top = cj_scope_top(cj);
    if (top == NULL) return false;

    if (top->type == CJ_ARRAY) {
        if (!top->start) {
            cj->write(cj->sink, ",");
        } else {
            top->start = false;
        }
    }

    cj->write(cj->sink, "%lld", n);

    if (!cj_maybe_object_key_remove(cj, top)) return false;

    return true;
}

bool cj_float(CJ* cj, long double f, size_t precision) {
    if (cj_has_error(cj)) return false;

    CJScope* top = cj_scope_top(cj);
    if (top == NULL) return false;

    if (top->type == CJ_ARRAY) {
        if (!top->start) {
            cj->write(cj->sink, ",");
        } else {
            top->start = false;
        }
    }

    cj->write(cj->sink, "%.*Lf", (int)precision, f);

    if (!cj_maybe_object_key_remove(cj, top)) return false;

    return true;
}

bool cj_null(CJ* cj) {
    if (cj_has_error(cj)) return false;

    CJScope* top = cj_scope_top(cj);
    if (top == NULL) {
        cj->result = CJ_SCOPE_UNDERFLOW;
        return false;
    }

    if (top->type == CJ_ARRAY) {
        if (!top->start) {
            cj->write(cj->sink, ",");
        } else {
            top->start = false;
        }
    }

    cj->write(cj->sink, "null");

    if (!cj_maybe_object_key_remove(cj, top)) return false;

    return true;
}

#endif
