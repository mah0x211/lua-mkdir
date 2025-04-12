/**
 *  Copyright (C) 2022 Masatoshi Fukunaga
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 */

#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
// lua
#include <lauxhlib.h>
#include <lua_errno.h>

static size_t MKDIR_BUFSIZ = PATH_MAX;
static char *MKDIR_BUF     = NULL;

static int normalize(lua_State *L, char *path, size_t len)
{
    int top    = 0;
    char *head = path;
    char *tail = path + len;
    char *p    = head;

CHECK_FIRST:
    switch (*p) {
    case '/':
ADD_SEGMENT:
        // add segments
        luaL_checkstack(L, 2, NULL);
        if ((uintptr_t)head < (uintptr_t)p) {
            lua_pushlstring(L, head, (uintptr_t)p - (uintptr_t)head);
            top++;
        }
        // add '/'
        lua_pushliteral(L, "/");
        top++;

        // skip multiple slashes
        while (*p == '/') {
            p++;
        }
        head = p;
        if (*p != '.') {
            break;
        }

    case '.':
        // foud '.' segment
        if (p[1] == '/' || p[1] == 0) {
            p += 1;
        }
        // found '..' segment
        else if (p[1] == '.' && (p[2] == '/' || p[2] == 0)) {
            p += 2;
            switch (top) {
            case 1:
                // remove previous segment if it is not slash
                if (*lua_tostring(L, 1) != '/') {
                    lua_settop(L, 0);
                    top = 0;
                }
                break;

            default:
                // remove previous segment with trailing-slash
                if (top > 1 && strcmp(lua_tostring(L, -2), "..") != 0) {
                    lua_pop(L, 2);
                    top -= 2;
                    break;
                }

            case 0:
                // add '..' segment
                luaL_checkstack(L, 2, NULL);
                lua_pushliteral(L, "..");
                lua_pushliteral(L, "/");
                top += 2;
                break;
            }
        } else {
            // allow segments that started with '.' character
            break;
        }

        // skip multiple slashes
        while (*p == '/') {
            p++;
        }
        head = p;
        goto CHECK_FIRST;
    }

    // search '/' character
    while (*p) {
        if (*p == '/') {
            goto ADD_SEGMENT;
        }
        p++;
    }

    // found NULL before the end of the string
    if (p != tail) {
        errno = EILSEQ;
        return -1;
    }

    // add last-segment
    if ((uintptr_t)head < (uintptr_t)tail) {
        luaL_checkstack(L, 1, NULL);
        lua_pushlstring(L, head, (uintptr_t)tail - (uintptr_t)head);
        top++;
    } else if (!top) {
        errno = EINVAL;
        return -1;
    }

    // remove trailing-slash
    if (top > 1 && *(char *)lua_tostring(L, top) == '/') {
        lua_pop(L, 1);
    }

    // check a last segment
    path = (char *)lua_tostring(L, -1);
    if (strcmp(path, "/") == 0 || strcmp(path, "..") == 0) {
        errno = EINVAL;
        return -1;
    }

    return 0;
}

static inline int isdir(const char *path, int follow_symlink)
{
    struct stat buf = {0};
    int rv          = 0;

    if (follow_symlink) {
        rv = stat(path, &buf);
    } else {
        rv = lstat(path, &buf);
    }

    if (rv) {
        // -1 := error
        // 0 := not exist
        return -(errno != ENOENT);
    } else if (S_ISDIR(buf.st_mode)) {
        // directory exists
        return 1;
    }
    // file exist
    errno = EEXIST;
    return -1;
}

static int mkdir_lua(lua_State *L)
{
    size_t len         = 0;
    char *path         = (char *)lauxh_checklstring(L, 1, &len);
    mode_t mode        = 0777;
    int parents        = 0;
    int follow_symlink = 1;
    int top            = 0;

    errno = 0;
    // check path length
    if (len > MKDIR_BUFSIZ) {
        // got error
        errno = ENAMETOOLONG;
        lua_pushboolean(L, 0);
        lua_errno_new(L, errno, "mkdir");
        return 2;
    }

    // check mode value
    if (lua_type(L, 2) == LUA_TSTRING) {
        // conver mode string to integer
        char *endptr     = NULL;
        const char *nptr = lua_tostring(L, 2);
        uintmax_t n      = strtoumax(nptr, &endptr, 8);
        if (errno != 0) {
            return lauxh_argerror(L, 2, "%s", strerror(errno));
        } else if (*endptr) {
            errno = EINVAL;
            return lauxh_argerror(L, 2, "%s", strerror(errno));
        }
        mode = n;
    } else {
        mode = lauxh_optuint16(L, 2, mode);
    }
    if (mode & ~0777) {
        errno = ERANGE;
        return lauxh_argerror(L, 2, "%s", strerror(errno));
    }

    parents        = lauxh_optboolean(L, 3, parents);
    follow_symlink = lauxh_optboolean(L, 4, follow_symlink);

    // copy to buffer
    path      = memcpy(MKDIR_BUF, path, len);
    path[len] = 0;
    lua_settop(L, 0);
    if (normalize(L, path, len) != 0) {
        lua_settop(L, 0);
        lua_pushboolean(L, 0);
        lua_errno_new(L, errno, "mkdir");
        return 2;
    }

    top = lua_gettop(L);
    len = 0;
    for (int idx = 1; idx <= top; idx++) {
        size_t slen     = 0;
        const char *seg = lua_tolstring(L, idx, &slen);
        memcpy(path + len, seg, slen);
        len += slen;
        path[len] = 0;

        if (idx != top) {
            switch (slen) {
            case 2:
                if (seg[0] == '.' && seg[2] == '.') {
                    continue;
                }
                break;
            case 1:
                if (*seg == '/') {
                    continue;
                }
            }
        }

        switch (isdir(path, follow_symlink)) {
        case 1: // found directory
            break;

        case 0: // not found
            if (idx != top && !parents) {
                // parent directory creation is not enabled
                errno = ENOENT;
            } else if (mkdir(path, mode) == 0) {
                // directory has been created
                continue;
            }

        default:
            // got error
            lua_settop(L, 0);
            lua_pushboolean(L, 0);
            lua_errno_new(L, errno, "mkdir");
            return 2;
        }
    }

    lua_settop(L, 0);
    lua_pushboolean(L, 1);
    return 1;
}

LUALIB_API int luaopen_mkdir(lua_State *L)
{
    long pathmax = pathconf(".", _PC_PATH_MAX);

    lua_errno_loadlib(L);

    // set the maximum number of bytes in a pathname
    if (pathmax != -1) {
        MKDIR_BUFSIZ = pathmax;
    }
    // allocate the buffer for getcwd
    MKDIR_BUF = lua_newuserdata(L, MKDIR_BUFSIZ + 1);
    // holds until the state closes
    luaL_ref(L, LUA_REGISTRYINDEX);

    lua_pushcfunction(L, mkdir_lua);
    return 1;
}
