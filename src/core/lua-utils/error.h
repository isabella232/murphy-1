/*
 * Copyright (c) 2012, 2013, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Intel Corporation nor the names of its contributors
 *     may be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MURPHY_LUA_ERROR_H__
#define __MURPHY_LUA_ERROR_H__

#include <lua.h>

#include "murphy/common/debug.h"

/*
 * basic error handling
 */

/** Macro to declare an error buffer for mrp_lua_{error, throw}. */
#define MRP_LUA_ERRBUF(...)                                             \
    size_t _ela[] = { 256, __VA_ARGS__ };                               \
    size_t _errl  = MRP_ARRAY_SIZE(_ela) == 1 ? _ela[0] : _ela[1];      \
    char   _errb[_errl]

/** Macro to mark arguments to be used by mrp_lua_{error, throw}. */
#define MRP_LUA_ERRUSE(_ebuf, _esize)                                   \
    size_t  _errl = _esize;                                             \
    char   *_errb = _ebuf

/** Macro to pass on error buffer argument to a function call. */
#define MRP_LUA_ERRPASS _errb, _errl

/** Macro to omit error buffer from a function call. */
#define MRP_LUA_ERRNONE NULL, 0

/** Active error buffer. */
#define MRP_LUA_ERR _errb

/** Macro to pass up, throw, or print an error. */
#define mrp_lua_error(_retval, _L, ...)                                 \
    mrp_lua_set_error(_L, _errb, _errl, __VA_ARGS__),                   \
        _retval

/** Macro to throw an error/exception. */
#define mrp_lua_throw(_L, ...)                                          \
    mrp_lua_set_error(_L, _errb, _errl, __VA_ARGS__)

/** The low-level error formatting/throwing/printing routine. */
int mrp_lua_set_error(lua_State *L, char *errbuf, size_t size, ...);


/*
 * debugging
 */

/** Workhorse macro to produce stack dumps */
#define __mrp_lua_stackdump(_fl, _ln, _fn, _L, _fmt, _args...) do {         \
    int _i, _depth, _t;                                                     \
                                                                            \
    if (_fmt && *_fmt)                                                      \
        mrp_debug_at(_fl, _ln, _fn, _fmt, ## _args);                        \
                                                                            \
    if ((_depth = lua_gettop(_L)) > 0)                                      \
        mrp_debug_at(_fl, _ln, _fn, "   Lua stack (depth: %d)", _depth);    \
    else                                                                    \
        mrp_debug_at(_fl, _ln, _fn, "   Lua stack: empty");                 \
                                                                            \
    for (_i = -1; _i > -1 - _depth; _i--) {                                 \
        switch ((_t = lua_type(_L, _i))) {                                  \
        case LUA_TSTRING:                                                   \
            mrp_debug_at(_fl, _ln, _fn, "      [#%d:'%s']", _i,             \
                         lua_tostring(_L, _i));                             \
            break;                                                          \
        case LUA_TNUMBER:                                                   \
            if (((int)lua_tointeger(_L,_i) == (double)lua_tonumber(_L,_i))) \
                mrp_debug_at(_fl, _ln, _fn, "      [#%d: %d]", _i,          \
                          (int)lua_tointeger(_L, _i));                      \
            else                                                            \
                mrp_debug_at(_fl, _ln, _fn, "      [#%d: %f]", _i,          \
                          (double)lua_tonumber(_L, _i));                    \
            break;                                                          \
        case LUA_TTABLE:                                                    \
            mrp_debug_at(_fl, _ln, _fn, "      [#%d: {%p}]", _i,            \
                         lua_topointer(_L, _i));                            \
            break;                                                          \
        default:                                                            \
            mrp_debug_at(_fl, _ln, _fn, "      [#%d: %s]", _i,              \
                         lua_typename(_L, _t));                             \
        }                                                                   \
    }                                                                       \
} while (0)


/** Macro to produce a simple debug dump of the Lua stack. */
#define mrp_lua_stackdump(L, fmt, args...) \
    __mrp_lua_stackdump(__FILE__, __LINE__, __FUNCTION__, L, fmt, ## args)


#endif /* __MURPHY_LUA_ERROR_H__ */
