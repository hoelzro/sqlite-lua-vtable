#include "sqlite3ext.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <stdio.h>
#include <string.h>

SQLITE_EXTENSION_INIT1;

static void *
sqlite_lua_allocator(void *ud, void *ptr, size_t osize, size_t nsize)
{
    (void) ud;
    (void) osize;

    return sqlite3_realloc(ptr, nsize);
}

#define NYI()\
    fprintf(stderr, "Module method %s not yet implemented\n", __func__);\
    return SQLITE_ERROR;

static int
lua_vtable_create(sqlite3 *db, void *aux, int argc, const char * const *argv, sqlite3_vtab **vtab_out, char **err_out)
{
    *err_out = sqlite3_mprintf("Module method %s not yet implemented", __func__);
    NYI();
}

static int
lua_vtable_connect(sqlite3 *db, void *aux, int argc, const char * const *argv, sqlite3_vtab **vtab_out, char **err_out)
{
    *err_out = sqlite3_mprintf("Module method %s not yet implemented", __func__);
    NYI();
}

static int
lua_vtable_best_index(sqlite3_vtab *vtab, sqlite3_index_info *info)
{
    NYI();
}

static int
lua_vtable_disconnect(sqlite3_vtab *vtab)
{
    NYI();
}

static int
lua_vtable_destroy(sqlite3_vtab *vtab)
{
    NYI();
}

static int
lua_vtable_open(sqlite3_vtab *vtab, sqlite3_vtab_cursor **cursor_out)
{
    NYI();
}

static int
lua_vtable_close(sqlite3_vtab_cursor *cursor)
{
    NYI();
}

static int
lua_vtable_filter(sqlite3_vtab_cursor *cursor, int idx_num, const char *idx_str, int argc, sqlite3_value **argv)
{
    NYI();
}

static int
lua_vtable_next(sqlite3_vtab_cursor *cursor)
{
    NYI();
}

static int
lua_vtable_eof(sqlite3_vtab_cursor *cursor)
{
    NYI();
}

static int
lua_vtable_column(sqlite3_vtab_cursor *cursor, sqlite3_context *ctx, int n)
{
    NYI();
}

static int
lua_vtable_rowid(sqlite3_vtab_cursor *cursor, sqlite_int64 *rowid_out)
{
    NYI();
}

static int
lua_vtable_update(sqlite3_vtab *vtab, int argc, sqlite3_value **argv, sqlite_int64 *rowid)
{
    NYI();
}

static int
lua_vtable_begin(sqlite3_vtab *vtab)
{
    NYI();
}

static int
lua_vtable_sync(sqlite3_vtab *vtab)
{
    NYI();
}

static int
lua_vtable_commit(sqlite3_vtab *vtab)
{
    NYI();
}

static int
lua_vtable_rollback(sqlite3_vtab *vtab)
{
    NYI();
}

static int
lua_vtable_find_function(sqlite3_vtab *vtab, int argc, const char *name, void (**func_out)(sqlite3_context*,int,sqlite3_value**), void **argv)
{
    NYI();
}

static int
lua_vtable_rename(sqlite3_vtab *vtab, const char *new_name)
{
    NYI();
}

static sqlite3_module lua_vtable_module = {
    iVersion: 1,
    xCreate: lua_vtable_create,
    xConnect: lua_vtable_connect,
    xBestIndex: lua_vtable_best_index,
    xDisconnect: lua_vtable_disconnect,
    xDestroy: lua_vtable_destroy,
    xOpen: lua_vtable_open,
    xClose: lua_vtable_close,
    xFilter: lua_vtable_filter,
    xNext: lua_vtable_next,
    xEof: lua_vtable_eof,
    xColumn: lua_vtable_column,
    xRowid: lua_vtable_rowid,
    xUpdate: lua_vtable_update,
    xBegin: lua_vtable_begin,
    xSync: lua_vtable_sync,
    xCommit: lua_vtable_commit,
    xRollback: lua_vtable_rollback,
    xFindFunction: lua_vtable_find_function,
    xRename: lua_vtable_rename,
};

int
sqlite3_extension_init(sqlite3 *db, char **error, const sqlite3_api_routines *api)
{
    SQLITE_EXTENSION_INIT2(api);

    lua_State *L;

    L = lua_newstate(sqlite_lua_allocator, NULL);
    luaL_openlibs(L);

    return sqlite3_create_module_v2(
        db,
        "lua",
        &lua_vtable_module,
        L,
        (void (*)(void*)) lua_close);
}
