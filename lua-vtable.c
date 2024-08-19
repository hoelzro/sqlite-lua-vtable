#include "sqlite3ext.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <stdio.h>
#include <string.h>

#define START_STACK_CHECK\
    int __top_start = lua_gettop(L)

#define FINISH_STACK_CHECK\
    if(__top_start != lua_gettop(L)) {\
        fprintf(stderr, "%s:%d Stack check failed in function %s (initial: %d, current: %d)\n", __FILE__, __LINE__, __FUNCTION__, __top_start, lua_gettop(L));\
        luaL_error(L, "stack check failed");\
    }

#define FINISH_STACK_CHECK_METHOD\
    if(__top_start != lua_gettop(L)) {\
        fprintf(stderr, "%s:%d Stack check failed calling method %s (initial: %d, current: %d)\n", __FILE__, __LINE__, method_name, __top_start, lua_gettop(L));\
        luaL_error(L, "stack check failed");\
    }

SQLITE_EXTENSION_INIT1;

static void *
sqlite_lua_allocator(void *ud, void *ptr, size_t osize, size_t nsize)
{
    (void) ud;
    (void) osize;

    return sqlite3_realloc(ptr, nsize);
}

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
lua_vtable_create(sqlite3 *db, void *_aux, int argc, const char * const *argv, sqlite3_vtab **vtab_out, char **err_out)
{
    struct script_module_data *aux = (struct script_module_data *) _aux;
    lua_State *L = aux->L;
    START_STACK_CHECK;

    int status;

    lua_rawgeti(L, LUA_REGISTRYINDEX, aux->create_ref);
    push_db(L, db);
    push_arg_strings(L, argc, argv);
    status = lua_pcall(L, 2, 2, 0);

    if(status == LUA_OK) {
        if(lua_isnil(L, -2)) { // XXX general falsiness instead?
            // XXX what if no values are returned?
            *err_out = sqlite3_mprintf("%s", lua_tostring(L, -1));
            lua_pop(L, 2);
            FINISH_STACK_CHECK;
            return SQLITE_ERROR;
        } else {
            lua_pop(L, 1);
            *vtab_out = pop_vtab(L, aux);
            FINISH_STACK_CHECK;
            return SQLITE_OK;
        }
    }

    *err_out = sqlite3_mprintf("%s", lua_tostring(L, -1));
    lua_pop(L, 1);

    FINISH_STACK_CHECK;
    return SQLITE_ERROR;
}

static int
lua_vtable_connect(sqlite3 *db, void *_aux, int argc, const char * const *argv, sqlite3_vtab **vtab_out, char **err_out)
{
    struct script_module_data *aux = (struct script_module_data *) _aux;
    lua_State *L = aux->L;
    START_STACK_CHECK;

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
            FINISH_STACK_CHECK;
            return SQLITE_ERROR;
        } else {
            lua_pop(L, 1);
            *vtab_out = pop_vtab(L, aux);
            FINISH_STACK_CHECK;
            return SQLITE_OK;
        }
    }

    *err_out = sqlite3_mprintf("%s", lua_tostring(L, -1));
    lua_pop(L, 1);

    FINISH_STACK_CHECK;
    return SQLITE_ERROR;
}

static void
push_index_info(lua_State *L, sqlite3_index_info *info)
{
    int i;

    lua_createtable(L, 0, 2);
    lua_createtable(L, info->nConstraint, 0);
    for(i = 0; i < info->nConstraint; i++) {
        lua_createtable(L, 0, 3);

        lua_pushinteger(L, info->aConstraint[i].iColumn);
        lua_setfield(L, -2, "column");

        switch(info->aConstraint[i].op) {
            case SQLITE_INDEX_CONSTRAINT_EQ:
                lua_pushliteral(L, "=");
                break;
            case SQLITE_INDEX_CONSTRAINT_GT:
                lua_pushliteral(L, ">");
                break;
            case SQLITE_INDEX_CONSTRAINT_LE:
                lua_pushliteral(L, "<=");
                break;
            case SQLITE_INDEX_CONSTRAINT_LT:
                lua_pushliteral(L, "<");
                break;
            case SQLITE_INDEX_CONSTRAINT_GE:
                lua_pushliteral(L, ">=");
                break;
            case SQLITE_INDEX_CONSTRAINT_MATCH:
                lua_pushliteral(L, "match");
                break;
            case SQLITE_INDEX_CONSTRAINT_LIKE:
                lua_pushliteral(L, "like");
                break;
            case SQLITE_INDEX_CONSTRAINT_GLOB:
                lua_pushliteral(L, "glob");
                break;
            case SQLITE_INDEX_CONSTRAINT_REGEXP:
                lua_pushliteral(L, "regexp");
                break;
            case SQLITE_INDEX_CONSTRAINT_NE:
                lua_pushliteral(L, "<>");
                break;
            case SQLITE_INDEX_CONSTRAINT_ISNOT:
                lua_pushliteral(L, "is not");
                break;
            case SQLITE_INDEX_CONSTRAINT_ISNOTNULL:
                lua_pushliteral(L, "is not null");
                break;
            case SQLITE_INDEX_CONSTRAINT_ISNULL:
                lua_pushliteral(L, "is null");
                break;
            case SQLITE_INDEX_CONSTRAINT_IS:
                lua_pushliteral(L, "is");
                break;
            case SQLITE_INDEX_CONSTRAINT_LIMIT:
                lua_pushliteral(L, "limit");
                break;
            case SQLITE_INDEX_CONSTRAINT_OFFSET:
                lua_pushliteral(L, "offset");
                break;
            default:
                // XXX SQLITE_INDEX_CONSTRAINT_FUNCTION or higher
                lua_pushinteger(L, info->aConstraint[i].op);
                break;
        }
        lua_setfield(L, -2, "op");

        lua_pushboolean(L, info->aConstraint[i].usable);
        lua_setfield(L, -2, "usable");

        lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "constraints");

    lua_createtable(L, info->nOrderBy, 0);
    for(i = 0; i < info->nOrderBy; i++) {
        lua_createtable(L, 0, 2);

        lua_pushinteger(L, info->aOrderBy[i].iColumn);
        lua_setfield(L, -2, "column");

        lua_pushboolean(L, info->aOrderBy[i].desc);
        lua_setfield(L, -2, "desc");

        lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "order_by");
}

static void
pop_index_info(lua_State *L, struct script_module_data *data, void *aux)
{
    sqlite3_index_info *info = (sqlite3_index_info *) aux;
    int i;

    lua_getfield(L, -1, "constraint_usage");
    lua_len(L, -1);
    int returned_constraint_usage_len = lua_tointeger(L, -1);
    lua_pop(L, 1);

    // XXX test that returning a short table is OK
    for(i = 0; i < info->nConstraint && (i+1) <= returned_constraint_usage_len; i++) {
        lua_rawgeti(L, -1, i + 1);

        lua_getfield(L, -1, "argv_index");
        if(!lua_isnil(L, -1)) {
            info->aConstraintUsage[i].argvIndex = lua_tointeger(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, -1, "omit");
        info->aConstraintUsage[i].omit = lua_toboolean(L, -1);
        lua_pop(L, 1);

        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "index_num");
    if(!lua_isnil(L, -1)) {
        info->idxNum = lua_tointeger(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "index_str");
    if(!lua_isnil(L, -1)) {
        info->idxStr = sqlite3_mprintf("%s", lua_tostring(L, -1));
        info->needToFreeIdxStr = 1;
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "order_by_consumed");
    info->orderByConsumed = lua_toboolean(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "estimated_cost");
    if(!lua_isnil(L, -1)) {
        info->estimatedCost = lua_tonumber(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "estimated_rows");
    if(!lua_isnil(L, -1)) {
        info->estimatedRows = lua_tointeger(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "index_flags");
    if(!lua_isnil(L, -1)) {
        info->idxFlags = lua_tointeger(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "column_used");
    if(!lua_isnil(L, -1)) {
        info->colUsed = lua_tointeger(L, -1);
    }
    lua_pop(L, 1);

    lua_pop(L, 1);
}

#define VTAB_STATE(vtab)\
    ((struct script_module_vtab *) vtab)->aux->L

#define CALL_METHOD_VTAB(vtab, method_name, nargs, popper, popper_aux)\
    call_method_vtab((struct script_module_vtab *) vtab, ((struct script_module_vtab *) vtab)->aux->method_name##_ref, nargs, popper, popper_aux, #method_name)

static int
call_method_vtab(struct script_module_vtab *vtab, int method_ref, int nargs, void (*popper)(lua_State *, struct script_module_data *, void *), void *popper_aux, const char *method_name)
{
    struct script_module_data *aux = vtab->aux;
    lua_State *L = aux->L;
    START_STACK_CHECK;
    __top_start -= nargs; // adjust stack check to compensate for arguments the caller left on the stack
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
            FINISH_STACK_CHECK_METHOD;
            return SQLITE_ERROR;
        } else {
            lua_pop(L, 1);
            popper(L, aux, popper_aux);
            FINISH_STACK_CHECK_METHOD;
            return SQLITE_OK;
        }
    }

    if(vtab->vtab.zErrMsg) {
        sqlite3_free(vtab->vtab.zErrMsg);
    }
    // XXX what if the value at the top of the stack isn't a string?
    vtab->vtab.zErrMsg = sqlite3_mprintf("%s", lua_tostring(L, -1));
    lua_pop(L, 1);

    FINISH_STACK_CHECK_METHOD;
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
    lua_pop(L, 1);
}

static int
lua_vtable_disconnect(sqlite3_vtab *vtab)
{
    lua_State *L = VTAB_STATE(vtab);
    START_STACK_CHECK;

    int status = CALL_METHOD_VTAB(vtab, disconnect, 0, pop_nothing, NULL);
    luaL_unref(L, LUA_REGISTRYINDEX, ((struct script_module_vtab *) vtab)->vtab_ref);
    FINISH_STACK_CHECK;
    return status;
}

static int
lua_vtable_destroy(sqlite3_vtab *vtab)
{
    lua_State *L = VTAB_STATE(vtab);
    START_STACK_CHECK;

    int status = CALL_METHOD_VTAB(vtab, destroy, 0, pop_nothing, NULL);
    luaL_unref(L, LUA_REGISTRYINDEX, ((struct script_module_vtab *) vtab)->vtab_ref);
    FINISH_STACK_CHECK;
    return status;
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
call_method_cursor(struct script_module_cursor *cursor, int method_ref, int nargs, void (*popper)(lua_State *, struct script_module_data *, void *), void *popper_aux, const char *method_name)
{
    struct script_module_data *aux = cursor->aux;
    lua_State *L = aux->L;
    START_STACK_CHECK;
    __top_start -= nargs; // adjust stack check to compensate for arguments the caller left on the stack
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
            FINISH_STACK_CHECK_METHOD;
            return SQLITE_ERROR;
        } else {
            lua_pop(L, 1);
            popper(L, aux, popper_aux);
            FINISH_STACK_CHECK_METHOD;
            return SQLITE_OK;
        }
    }

    if(cursor->cursor.pVtab->zErrMsg) {
        sqlite3_free(cursor->cursor.pVtab->zErrMsg);
    }
    // XXX what if the value at the top of the stack isn't a string?
    cursor->cursor.pVtab->zErrMsg = sqlite3_mprintf("%s", lua_tostring(L, -1));
    lua_pop(L, 1);

    FINISH_STACK_CHECK_METHOD;
    return SQLITE_ERROR;
}

#define CURSOR_STATE(cursor)\
    ((struct script_module_cursor *) cursor)->aux->L

#define CALL_METHOD_CURSOR(cursor, method_name, nargs, popper, popper_aux)\
    call_method_cursor((struct script_module_cursor *) cursor, ((struct script_module_cursor *)cursor)->aux->method_name##_ref, nargs, popper, popper_aux, #method_name)

static int
lua_vtable_close(sqlite3_vtab_cursor *cursor)
{
    lua_State *L = CURSOR_STATE(cursor);
    START_STACK_CHECK;

    int status = CALL_METHOD_CURSOR(cursor, close, 0, pop_nothing, NULL);
    luaL_unref(L, LUA_REGISTRYINDEX, ((struct script_module_cursor *)cursor)->cursor_ref);
    FINISH_STACK_CHECK;
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
            if(lua_isinteger(L, -1)) {
                sqlite3_result_int64(ctx, lua_tointeger(L, -1));
            } else {
                sqlite3_result_double(ctx, lua_tonumber(L, -1));
            }
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

static void
pop_int64(lua_State *L, struct script_module_data *data, void *aux)
{
    sqlite_int64 *out = (sqlite_int64 *) aux;

    *out = lua_tointeger(L, -1);
    lua_pop(L, 1);
}

static int
lua_vtable_rowid(sqlite3_vtab_cursor *cursor, sqlite_int64 *rowid_out)
{
    return CALL_METHOD_CURSOR(cursor, rowid, 0, pop_int64, rowid_out);
}

static int
lua_vtable_update(sqlite3_vtab *vtab, int argc, sqlite3_value **argv, sqlite_int64 *rowid_out)
{
    lua_State *L = VTAB_STATE(vtab);
    push_arg_values(L, argc, argv);
    return CALL_METHOD_VTAB(vtab, update, 1, pop_int64, rowid_out);
}

static int
lua_vtable_begin(sqlite3_vtab *vtab)
{
    return CALL_METHOD_VTAB(vtab, begin, 0, pop_nothing, NULL);
}

static int
lua_vtable_sync(sqlite3_vtab *vtab)
{
    return CALL_METHOD_VTAB(vtab, sync, 0, pop_nothing, NULL);
}

static int
lua_vtable_commit(sqlite3_vtab *vtab)
{
    return CALL_METHOD_VTAB(vtab, commit, 0, pop_nothing, NULL);
}

static int
lua_vtable_rollback(sqlite3_vtab *vtab)
{
    return CALL_METHOD_VTAB(vtab, rollback, 0, pop_nothing, NULL);
}

static void
pop_function(lua_State *L, struct script_module_data *data, void *aux)
{
    int *ref = aux;

    *ref = LUA_REFNIL;
    if(lua_isfunction(L, -1)) {
        *ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    lua_pop(L, 1);
}

struct caller_args {
    lua_State *L;
    int function_ref;
};

static void
lua_function_caller(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    struct caller_args *args = sqlite3_user_data(ctx);
    lua_State *L = args->L;
    START_STACK_CHECK;

    int i;
    int status;

    if(!lua_checkstack(L, argc + 1)) {
        sqlite3_result_error_nomem(ctx);
        FINISH_STACK_CHECK;
        return;
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, args->function_ref);

    // XXX duplication with push_arg_values
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
            default:
            }
        }
    }

    status = lua_pcall(L, argc, 1, 0);
    if(status != LUA_OK) {
        sqlite3_result_error(ctx, lua_tostring(L, -1), -1);
        lua_pop(L, 1);
        FINISH_STACK_CHECK;
        return;
    }

    // XXX duplication with pop_sqlite_value
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
    FINISH_STACK_CHECK;
}

static int
lua_vtable_find_function(sqlite3_vtab *vtab, int argc, const char *name, void (**func_out)(sqlite3_context*,int,sqlite3_value**), void **arg_out)
{
    lua_State *L = VTAB_STATE(vtab);
    START_STACK_CHECK;

    int function_ref;

    lua_pushinteger(L, argc);
    lua_pushstring(L, name);
    int status = CALL_METHOD_VTAB(vtab, find_function, 2, pop_function, &function_ref);

    if(status != SQLITE_OK) {
        // XXX we have no way to signal an error, do we?
        FINISH_STACK_CHECK;
        return 0;
    }

    if(function_ref == LUA_REFNIL) {
        FINISH_STACK_CHECK;
        return 0;
    }

    struct caller_args *args = sqlite3_malloc(sizeof(struct caller_args));
    args->L = L;
    args->function_ref = function_ref;

    *func_out = lua_function_caller;
    *arg_out = args;
    FINISH_STACK_CHECK;
    return 1;
}

static int
lua_vtable_rename(sqlite3_vtab *vtab, const char *new_name)
{
    lua_State *L = VTAB_STATE(vtab);
    lua_pushstring(L, new_name);
    return CALL_METHOD_VTAB(vtab, rename, 1, pop_nothing, NULL);
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

static int
lua_sqlite3_get_ptr(lua_State *L)
{
    // XXX how can you be sure the SQLite versions match?
    lua_pushlightuserdata(L, to_sqlite3(L, 1));
    return 1;
}

static luaL_Reg lua_sqlite3_methods[] = {
    {"declare_vtab", lua_sqlite3_declare_vtab},
    {"get_ptr", lua_sqlite3_get_ptr},
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

static void
create_module(sqlite3_context *ctx, const char *source, int is_source_file)
{
    lua_State *L;
    int status;

    sqlite3 *db = sqlite3_context_db_handle(ctx);

    L = lua_newstate(sqlite_lua_allocator, NULL);
    luaL_openlibs(L);

    set_up_metatables(L);

    if(is_source_file) {
        status = luaL_dofile(L, source);
    } else {
        status = luaL_dostring(L, source);
    }

    if(status) {
        sqlite3_result_error(ctx, lua_tostring(L, -1), -1);
        lua_close(L);
        return;
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

    if(script_module_aux->update_ref == LUA_REFNIL) {
        script_module->xUpdate = NULL;
    }

    if(script_module_aux->begin_ref == LUA_REFNIL) {
        script_module->xBegin = NULL;
    }

    if(script_module_aux->sync_ref == LUA_REFNIL) {
        script_module->xSync = NULL;
    }

    if(script_module_aux->commit_ref == LUA_REFNIL) {
        script_module->xCommit = NULL;
    }

    if(script_module_aux->rollback_ref == LUA_REFNIL) {
        script_module->xRollback = NULL;
    }

    if(script_module_aux->rename_ref == LUA_REFNIL) {
        script_module->xRename = NULL;
    }

    if(script_module_aux->find_function_ref == LUA_REFNIL) {
        script_module->xFindFunction = NULL;
    }

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
        sqlite3_result_error_code(ctx, status);
        sqlite3_free(script_module);
        sqlite3_free(script_module_aux);
        lua_close(L);
    }
}

static void
create_module_from_script(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    const char *filename = sqlite3_value_text(argv[0]);
    create_module(ctx, filename, 1);
}

int
sqlite3_extension_init(sqlite3 *db, char **error, const sqlite3_api_routines *api)
{
    SQLITE_EXTENSION_INIT2(api);
    sqlite3_initialize();

    sqlite3_create_function(db, "lua_create_module_from_file", 1, SQLITE_UTF8,
        NULL, create_module_from_script, NULL, NULL);

    return SQLITE_OK;
}
