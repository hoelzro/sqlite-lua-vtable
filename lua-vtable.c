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

struct script_module_data {
    lua_State *L;
    sqlite3_module *mod;

    int create_ref;
    int connect_ref;
    int best_index_ref;
    int close_ref;
    int rowid_ref;
    int column_ref;
    int next_ref;
    int eof_ref;
    int filter_ref;
    int disconnect_ref;
    int destroy_ref;
    int open_ref;
    int update_ref;
    int begin_ref;
    int sync_ref;
    int commit_ref;
    int rollback_ref;
    int rename_ref;
    int find_function_ref;
};

struct script_module_vtab {
    sqlite3_vtab vtab;
    struct script_module_data *aux;
    int vtab_ref;
};

struct script_module_cursor {
    sqlite3_vtab_cursor cursor;
    struct script_module_data *aux;
    int cursor_ref;
};

static int
lua_vtable_create(sqlite3 *db, void *aux, int argc, const char * const *argv, sqlite3_vtab **vtab_out, char **err_out)
{
    *err_out = sqlite3_mprintf("Module method %s not yet implemented", __func__);
    NYI();
}

static void
push_db(lua_State *L, sqlite3 *db)
{
    sqlite3 **lua_db = lua_newuserdata(L, sizeof(sqlite3 *));
    *lua_db = db;
    luaL_getmetatable(L, "sqlite3*");
    lua_setmetatable(L, -2);
}

static void
push_arg_strings(lua_State *L, int argc, const char * const *argv)
{
    int i;

    lua_createtable(L, argc, 0);
    for(i = 0; i < argc; i++) {
        lua_pushstring(L, argv[i]);
        lua_rawseti(L, -2, i + 1);
    }
}

static void
push_vtab(lua_State *L, struct script_module_vtab *vtab)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, vtab->vtab_ref);
}

static sqlite3_vtab *
pop_vtab(lua_State *L, struct script_module_data *aux)
{
    struct script_module_vtab *vtab;

    vtab = sqlite3_malloc(sizeof(struct script_module_vtab));
    vtab->aux = aux;
    vtab->vtab_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    return (sqlite3_vtab *) vtab;
}

static int
lua_vtable_connect(sqlite3 *db, void *_aux, int argc, const char * const *argv, sqlite3_vtab **vtab_out, char **err_out)
{
    struct script_module_data *aux = (struct script_module_data *) _aux;
    lua_State *L = aux->L;
    int status;

    lua_rawgeti(L, LUA_REGISTRYINDEX, aux->connect_ref);
    push_db(L, db);
    push_arg_strings(L, argc, argv);
    status = lua_pcall(L, 2, 2, 0);

    if(status == LUA_OK) {
        if(lua_isnil(L, -2)) { // XXX general falsiness instead?
            // XXX what if no values are returned?
            *err_out = sqlite3_mprintf("%s", lua_tostring(L, -1));
            lua_pop(L, 2);
            return SQLITE_ERROR;
        } else {
            lua_pop(L, 1);
            *vtab_out = pop_vtab(L, aux);
            return SQLITE_OK;
        }
    }

    *err_out = sqlite3_mprintf("%s", lua_tostring(L, -1));
    lua_pop(L, 1);

    return SQLITE_ERROR;
}

static void
push_index_info(lua_State *L, sqlite3_index_info *info)
{
    // XXX IMPL ME
    lua_pushnil(L);
}

static void
pop_index_info(lua_State *L, struct script_module_data *data, void *aux)
{
    // XXX IMPL ME
    lua_pop(L, 1);
}

#define VTAB_STATE(vtab)\
    ((struct script_module_vtab *) vtab)->aux->L

#define CALL_METHOD_VTAB(vtab, method_name, nargs, popper, popper_aux)\
    call_method_vtab((struct script_module_vtab *) vtab, ((struct script_module_vtab *) vtab)->aux->method_name##_ref, nargs, popper, popper_aux)

static int
call_method_vtab(struct script_module_vtab *vtab, int method_ref, int nargs, void (*popper)(lua_State *, struct script_module_data *, void *), void *popper_aux)
{
    struct script_module_data *aux = vtab->aux;
    lua_State *L = aux->L;
    int status;

    lua_rawgeti(L, LUA_REGISTRYINDEX, method_ref);
    push_vtab(L, vtab);
    lua_rotate(L, -2 - nargs, 2);
    status = lua_pcall(L, nargs + 1, 2, 0);

    if(status == LUA_OK) {
        if(lua_isnil(L, -2) && !lua_isnil(L, -1)) { // XXX general falsiness instead?
            if(vtab->vtab.zErrMsg) {
                sqlite3_free(vtab->vtab.zErrMsg);
            }
            // XXX what if the value at the top of the stack isn't a string?
            vtab->vtab.zErrMsg = sqlite3_mprintf("%s", lua_tostring(L, -1));
            lua_pop(L, 2);
            return SQLITE_ERROR;
        } else {
            lua_pop(L, 1);
            popper(L, aux, popper_aux);
            return SQLITE_OK;
        }
    }

    if(vtab->vtab.zErrMsg) {
        sqlite3_free(vtab->vtab.zErrMsg);
    }
    // XXX what if the value at the top of the stack isn't a string?
    vtab->vtab.zErrMsg = sqlite3_mprintf("%s", lua_tostring(L, -1));
    lua_pop(L, 1);

    return SQLITE_ERROR;
}

static int
lua_vtable_best_index(sqlite3_vtab *vtab, sqlite3_index_info *info)
{
    lua_State *L = VTAB_STATE(vtab);
    push_index_info(L, info);
    return CALL_METHOD_VTAB(vtab, best_index, 1, pop_index_info, info);
}

static void
pop_nothing(lua_State *L, struct script_module_data *data, void *aux)
{
}

static int
lua_vtable_disconnect(sqlite3_vtab *vtab)
{
    lua_State *L = VTAB_STATE(vtab);
    int status = CALL_METHOD_VTAB(vtab, disconnect, 0, pop_nothing, NULL);
    luaL_unref(L, LUA_REGISTRYINDEX, ((struct script_module_vtab *) vtab)->vtab_ref);
    return status;
}

static int
lua_vtable_destroy(sqlite3_vtab *vtab)
{
    NYI();
}

static void
pop_cursor(lua_State *L, struct script_module_data *data, void *aux)
{
    sqlite3_vtab_cursor **cursor_out = aux;
    struct script_module_cursor *cursor;

    cursor = sqlite3_malloc(sizeof(struct script_module_cursor));
    cursor->aux = data;
    cursor->cursor_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    *cursor_out = (sqlite3_vtab_cursor *) cursor;
}

static int
lua_vtable_open(sqlite3_vtab *_vtab, sqlite3_vtab_cursor **cursor_out)
{
    struct script_module_vtab *vtab = (struct script_module_vtab *) _vtab;

    return CALL_METHOD_VTAB(vtab, open, 0, pop_cursor, cursor_out);
}

static void
push_cursor(lua_State *L, struct script_module_cursor *cursor)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, cursor->cursor_ref);
}

static void
push_arg_values(lua_State *L, int argc, sqlite3_value **argv)
{
    int i;

    lua_createtable(L, argc, 0);
    for(i = 0; i < argc; i++) {
        switch(sqlite3_value_type(argv[i])) {
            case SQLITE_INTEGER:
                lua_pushinteger(L, sqlite3_value_int(argv[i]));
                break;
            case SQLITE_FLOAT:
                lua_pushnumber(L, sqlite3_value_double(argv[i]));
                break;
            case SQLITE_NULL:
                lua_pushnil(L);
                break;
            case SQLITE_BLOB:
            case SQLITE_TEXT: {
                size_t length;

                length = sqlite3_value_bytes(argv[i]);
                lua_pushlstring(L, (const char *) sqlite3_value_text(argv[i]), length);
                break;
            }
        }
        lua_rawseti(L, -2, i + 1);
    }
}

static int
call_method_cursor(struct script_module_cursor *cursor, int method_ref, int nargs, void (*popper)(lua_State *, struct script_module_data *, void *), void *popper_aux)
{
    struct script_module_data *aux = cursor->aux;
    lua_State *L = aux->L;
    int status;

    lua_rawgeti(L, LUA_REGISTRYINDEX, method_ref);
    push_cursor(L, cursor);
    lua_rotate(L, -2 - nargs, 2); // I *think* this is right?
    status = lua_pcall(L, nargs + 1, 2, 0);

    if(status == LUA_OK) {
        if(lua_isnil(L, -2) && !lua_isnil(L, -1)) { // XXX general falsiness instead?
            if(cursor->cursor.pVtab->zErrMsg) {
                sqlite3_free(cursor->cursor.pVtab->zErrMsg);
            }
            // XXX what if the value at the top of the stack isn't a string?
            cursor->cursor.pVtab->zErrMsg = sqlite3_mprintf("%s", lua_tostring(L, -1));
            lua_pop(L, 2);
            return SQLITE_ERROR;
        } else {
            lua_pop(L, 1);
            popper(L, aux, popper_aux);
            return SQLITE_OK;
        }
    }

    if(cursor->cursor.pVtab->zErrMsg) {
        sqlite3_free(cursor->cursor.pVtab->zErrMsg);
    }
    // XXX what if the value at the top of the stack isn't a string?
    cursor->cursor.pVtab->zErrMsg = sqlite3_mprintf("%s", lua_tostring(L, -1));
    lua_pop(L, 1);

    return SQLITE_ERROR;
}

#define CURSOR_STATE(cursor)\
    ((struct script_module_cursor *) cursor)->aux->L

#define CALL_METHOD_CURSOR(cursor, method_name, nargs, popper, popper_aux)\
    call_method_cursor((struct script_module_cursor *) cursor, ((struct script_module_cursor *)cursor)->aux->method_name##_ref, nargs, popper, popper_aux)

static int
lua_vtable_close(sqlite3_vtab_cursor *cursor)
{
    lua_State *L = CURSOR_STATE(cursor);
    int status = CALL_METHOD_CURSOR(cursor, close, 0, pop_nothing, NULL);
    luaL_unref(L, LUA_REGISTRYINDEX, ((struct script_module_cursor *)cursor)->cursor_ref);
    return status;
}

static int
lua_vtable_filter(sqlite3_vtab_cursor *cursor, int idx_num, const char *idx_str, int argc, sqlite3_value **argv)
{
    lua_State *L = CURSOR_STATE(cursor);

    lua_pushinteger(L, idx_num);
    lua_pushstring(L, idx_str);
    push_arg_values(L, argc, argv);

    return CALL_METHOD_CURSOR(cursor, filter, 3, pop_nothing, NULL);
}

static int
lua_vtable_next(sqlite3_vtab_cursor *cursor)
{
    return CALL_METHOD_CURSOR(cursor, next, 0, pop_nothing, NULL);
}

static void
pop_bool(lua_State *L, struct script_module_data *data, void *aux)
{
    int *result_out = (int *) aux;
    *result_out = lua_toboolean(L, -1);
    lua_pop(L, 1);
}

static int
lua_vtable_eof(sqlite3_vtab_cursor *cursor)
{
    int result;
    CALL_METHOD_CURSOR(cursor, eof, 0, pop_bool, &result);
    return result;
}

static void
pop_sqlite_value(lua_State *L, struct script_module_data *data, void *aux)
{
    sqlite3_context *ctx = (sqlite3_context *) aux;
    switch(lua_type(L, -1)) {
        case LUA_TSTRING: {
            size_t length;
            const char *value;

            value = lua_tolstring(L, -1, &length);

            sqlite3_result_text(ctx, value, length, SQLITE_TRANSIENT);
            break;
        }
        case LUA_TNUMBER:
            sqlite3_result_double(ctx, lua_tonumber(L, -1));
            break;
        case LUA_TBOOLEAN:
            sqlite3_result_int(ctx, lua_toboolean(L, -1));
            break;
        case LUA_TNIL:
            sqlite3_result_null(ctx);
            break;

        case LUA_TTABLE:
        case LUA_TFUNCTION:
        case LUA_TTHREAD:
        case LUA_TUSERDATA: {
            char *error = NULL;

            error = sqlite3_mprintf("Invalid return type from lua(): %s", lua_typename(L, lua_type(L, -1)));

            sqlite3_result_error(ctx, error, -1);
            sqlite3_free(error);
        }
    }
    lua_pop(L, 1);
}

static int
lua_vtable_column(sqlite3_vtab_cursor *cursor, sqlite3_context *ctx, int n)
{
    lua_State *L = CURSOR_STATE(cursor);

    lua_pushinteger(L, n);
    return CALL_METHOD_CURSOR(cursor, column, 1, pop_sqlite_value, ctx);
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

static void
cleanup_script_module(void *_aux)
{
    struct script_module_data *aux = (struct script_module_data *) _aux;
    lua_close(aux->L);
    sqlite3_free(aux->mod);
}

static sqlite3 *
to_sqlite3(lua_State *L, int idx)
{
    return *((sqlite3 **) luaL_checkudata(L, idx, "sqlite3*"));
}

static int
lua_sqlite3_declare_vtab(lua_State *L)
{
    sqlite3 *db;
    const char *sql;
    int status;

    db = to_sqlite3(L, 1);
    sql = luaL_checkstring(L, 2);

    status = sqlite3_declare_vtab(db, sql);

    if(status == SQLITE_OK) {
        lua_pushboolean(L, 1);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushstring(L, sqlite3_errstr(status));
        return 2;
    }
}

static luaL_Reg lua_sqlite3_methods[] = {
    {"declare_vtab", lua_sqlite3_declare_vtab},
    {NULL, NULL},
};

static void
set_up_metatables(lua_State *L)
{
    luaL_newmetatable(L, "sqlite3*");
    luaL_newlib(L, lua_sqlite3_methods);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

static int
create_module_from_script(sqlite3 *db, const char *filename, char **err_out)
{
    lua_State *L;
    int status;

    L = lua_newstate(sqlite_lua_allocator, NULL);
    luaL_openlibs(L);

    set_up_metatables(L);

    status = luaL_dofile(L, filename);
    if(status) {
        if(err_out) {
            *err_out = sqlite3_mprintf("%s", lua_tostring(L, -1));
        }
        lua_close(L);
        return SQLITE_ERROR;
    }

    lua_getfield(L, -1, "name");
    const char *module_name = lua_tostring(L, -1);

    sqlite3_module *script_module = sqlite3_malloc(sizeof(sqlite3_module));
    struct script_module_data *script_module_aux = sqlite3_malloc(sizeof(struct script_module_data));

    memcpy(script_module, &lua_vtable_module, sizeof(sqlite3_module));

    lua_getfield(L, -2, "connect");
    lua_getfield(L, -3, "create");

    // if create is nil, use NULL for xCreate to get that special behavior
    if(lua_isnil(L, -1)) {
        script_module->xCreate = NULL;
    } else if(lua_rawequal(L, -1, -2)) {
        // if create and connect are the same function, use the same C function
        // pointer here to get that special behavior
        script_module->xCreate = script_module->xConnect;
    }

    lua_pop(L, 2);

#define METHOD_REF(name)\
    lua_getfield(L, -2, #name);\
    script_module_aux->name##_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    METHOD_REF(create);
    METHOD_REF(connect);
    METHOD_REF(best_index);
    METHOD_REF(close);
    METHOD_REF(rowid);
    METHOD_REF(column);
    METHOD_REF(next);
    METHOD_REF(eof);
    METHOD_REF(filter);
    METHOD_REF(disconnect);
    METHOD_REF(destroy);
    METHOD_REF(open);
    METHOD_REF(update);
    METHOD_REF(begin);
    METHOD_REF(sync);
    METHOD_REF(commit);
    METHOD_REF(rollback);
    METHOD_REF(rename);
    METHOD_REF(find_function);

#undef METHOD_REF

    script_module_aux->L = L;
    script_module_aux->mod = script_module;

    status = sqlite3_create_module_v2(
        db,
        module_name,
        script_module,
        script_module_aux,
        cleanup_script_module);

    if(status == SQLITE_OK) {
        lua_settop(L, 0); // clear Lua stack after create_module to make sure module_name is a valid pointer
    } else {
        sqlite3_free(script_module);
        sqlite3_free(script_module_aux);
        lua_close(L);
    }

    return status;
}

int
sqlite3_extension_init(sqlite3 *db, char **error, const sqlite3_api_routines *api)
{
    SQLITE_EXTENSION_INIT2(api);

    return create_module_from_script(db, "counter.lua", error);
}
