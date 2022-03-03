#include "sqlite3ext.h"

SQLITE_EXTENSION_INIT1;

int
sqlite3_extension_init(sqlite3 *db, char **error, const sqlite3_api_routines *api)
{
    SQLITE_EXTENSION_INIT2(api);

    return SQLITE_OK;
}
