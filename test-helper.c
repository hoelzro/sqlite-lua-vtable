#include "sqlite3ext.h"

#include <stdlib.h>

SQLITE_EXTENSION_INIT1;

static void
dummy_func(sqlite3_context *, int, sqlite3_value **)
{
}

int
sqlite3_extension_init(sqlite3 *db, char **error, const sqlite3_api_routines *api)
{
    SQLITE_EXTENSION_INIT2(api);
    sqlite3_initialize();

    sqlite3_create_function(db, "test_fn", 1, SQLITE_UTF8 | SQLITE_RESULT_SUBTYPE, NULL, dummy_func, NULL, NULL);

    return SQLITE_OK;
}
