/*
** libzbxpgsql - A PostgreSQL monitoring module for Zabbix
** Copyright (C) 2015 - Ryan Armstrong <ryan@cavaliercoder.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**/

/*
 * See:
 *     LibPQ:       http://www.postgresql.org/docs/9.4/static/libpq.html
 *     Statistics:  http://www.postgresql.org/docs/9.4/static/monitoring-stats.html
 */

#include "libzbxpgsql.h"

// Define custom keys
static ZBX_METRIC keys[] =
/*      KEY                         FLAG            FUNCTION                        TEST PARAMETERS */
{
    {"pg.modver",                   0,              MODVER,                         NULL},
    {"pg.connect",                  CF_HAVEPARAMS,  PG_CONNECT,                     NULL},
    {"pg.version",                  CF_HAVEPARAMS,  PG_VERSION,                     NULL},
    {"pg.starttime",                CF_HAVEPARAMS,  PG_STARTTIME,                   NULL},
    {"pg.uptime",                   CF_HAVEPARAMS,  PG_UPTIME,                      NULL},

    {"pg.setting",                  CF_HAVEPARAMS,  PG_SETTING,                     ",,data_directory"},
    {"pg.setting.discovery",        CF_HAVEPARAMS,  PG_SETTING_DISCOVERY,           NULL},

    // User queries
    {"pg.query.string",             CF_HAVEPARAMS,  PG_QUERY,                       ",,SELECT 'Lorem ipsum dolor';"},
    {"pg.query.integer",            CF_HAVEPARAMS,  PG_QUERY,                       ",,SELECT pg_backend_pid();"},
    {"pg.query.double",             CF_HAVEPARAMS,  PG_QUERY,                       ",,SELECT CAST(1234 AS double precision);"},
    {"pg.query.discovery",          CF_HAVEPARAMS,  PG_QUERY,                       ",,SELECT * FROM pg_database;"},
    
    // Client connection statistics
    {"pg.backends.count",           CF_HAVEPARAMS,  PG_BACKENDS_COUNT,              NULL},
    {"pg.queries.longest",          CF_HAVEPARAMS,  PG_QUERIES_LONGEST,             NULL},

    // Server statistics (as per pg_stat_bgwriter)
    {"pg.checkpoints_timed",        CF_HAVEPARAMS,  PG_STAT_BGWRITER,               NULL},
    {"pg.checkpoints_req",          CF_HAVEPARAMS,  PG_STAT_BGWRITER,               NULL},
    {"pg.checkpoint_write_time",    CF_HAVEPARAMS,  PG_STAT_BGWRITER,               NULL},
    {"pg.checkpoint_sync_time",     CF_HAVEPARAMS,  PG_STAT_BGWRITER,               NULL},
    {"pg.checkpoint_avg_interval",  CF_HAVEPARAMS,  PG_BG_AVG_INTERVAL,             NULL},
    {"pg.checkpoint_time_perc",     CF_HAVEPARAMS,  PG_BG_TIME_PERC,                NULL},
    {"pg.buffers_checkpoint",       CF_HAVEPARAMS,  PG_STAT_BGWRITER,               NULL},
    {"pg.buffers_clean",            CF_HAVEPARAMS,  PG_STAT_BGWRITER,               NULL},
    {"pg.maxwritten_clean",         CF_HAVEPARAMS,  PG_STAT_BGWRITER,               NULL},
    {"pg.buffers_backend",          CF_HAVEPARAMS,  PG_STAT_BGWRITER,               NULL},
    {"pg.buffers_backend_fsync",    CF_HAVEPARAMS,  PG_STAT_BGWRITER,               NULL},
    {"pg.buffers_alloc",            CF_HAVEPARAMS,  PG_STAT_BGWRITER,               NULL},
    {"pg.stats_reset",              CF_HAVEPARAMS,  PG_STAT_BGWRITER,               NULL},
    {"pg.stats_reset_interval",     CF_HAVEPARAMS,  PG_BG_STATS_RESET_INTERVAL,     NULL},
    
    // Asset discovery
    {"pg.db.discovery",             CF_HAVEPARAMS,  PG_DB_DISCOVERY,                NULL},
    {"pg.namespace.discovery",      CF_HAVEPARAMS,  PG_NAMESPACE_DISCOVERY,         NULL},
    {"pg.schema.discovery",         CF_HAVEPARAMS,  PG_NAMESPACE_DISCOVERY,         NULL}, // Alias for pg.namespace.discovery
    {"pg.tablespace.discovery",     CF_HAVEPARAMS,  PG_TABLESPACE_DISCOVERY,        NULL},
    {"pg.table.discovery",          CF_HAVEPARAMS,  PG_TABLE_DISCOVERY,             NULL},
    {"pg.table.children.discovery", CF_HAVEPARAMS,  PG_TABLE_CHILDREN_DISCOVERY,    ",,pg_proc"},
    {"pg.index.discovery",          CF_HAVEPARAMS,  PG_INDEX_DISCOVERY,             NULL},
    
    // Asset class sizes
    {"pg.db.size",                  CF_HAVEPARAMS,  PG_DB_SIZE,                     NULL},
    {"pg.table.size",               CF_HAVEPARAMS,  PG_TABLE_SIZE,                  NULL},
    {"pg.table.rows",               CF_HAVEPARAMS,  PG_TABLE_ROWS,                  NULL},
    {"pg.table.children",           CF_HAVEPARAMS,  PG_TABLE_CHILDREN,              ",,pg_database"},
    {"pg.table.children.size",      CF_HAVEPARAMS,  PG_TABLE_CHILDREN_SIZE,         ",,pg_database"},
    {"pg.table.children.rows",      CF_HAVEPARAMS,  PG_TABLE_CHILDREN_ROWS,         ",,pg_database"},
    {"pg.index.size",               CF_HAVEPARAMS,  PG_INDEX_SIZE,                  NULL},
    {"pg.index.rows",               CF_HAVEPARAMS,  PG_INDEX_ROWS,                  NULL},
    {"pg.tablespace.size",          CF_HAVEPARAMS,  PG_TABLESPACE_SIZE,             ",,pg_default"},
    {"pg.namespace.size",           CF_HAVEPARAMS,  PG_NAMESPACE_SIZE,              ",,pg_catalog"},
    {"pg.schema.size",              CF_HAVEPARAMS,  PG_NAMESPACE_SIZE,              ",,pg_catalog"}, // Alias for pg.namespace.size
    
    // Database statistics (as per pg_stat_database)
    {"pg.db.numbackends",           CF_HAVEPARAMS,  PG_STAT_DATABASE,               NULL},
    {"pg.db.xact_commit",           CF_HAVEPARAMS,  PG_STAT_DATABASE,               NULL},
    {"pg.db.xact_rollback",         CF_HAVEPARAMS,  PG_STAT_DATABASE,               NULL},
    {"pg.db.blks_read",             CF_HAVEPARAMS,  PG_STAT_DATABASE,               NULL},
    {"pg.db.blks_hit",              CF_HAVEPARAMS,  PG_STAT_DATABASE,               NULL},
    {"pg.db.blks_perc",             CF_HAVEPARAMS,  PG_DB_BLKS_PERC,                NULL},
    {"pg.db.tup_returned",          CF_HAVEPARAMS,  PG_STAT_DATABASE,               NULL},
    {"pg.db.tup_fetched",           CF_HAVEPARAMS,  PG_STAT_DATABASE,               NULL},
    {"pg.db.tup_inserted",          CF_HAVEPARAMS,  PG_STAT_DATABASE,               NULL},
    {"pg.db.tup_updated",           CF_HAVEPARAMS,  PG_STAT_DATABASE,               NULL},
    {"pg.db.tup_deleted",           CF_HAVEPARAMS,  PG_STAT_DATABASE,               NULL},
    {"pg.db.conflicts",             CF_HAVEPARAMS,  PG_STAT_DATABASE,               NULL},
    {"pg.db.temp_files",            CF_HAVEPARAMS,  PG_STAT_DATABASE,               NULL},
    {"pg.db.temp_bytes",            CF_HAVEPARAMS,  PG_STAT_DATABASE,               NULL},
    {"pg.db.deadlocks",             CF_HAVEPARAMS,  PG_STAT_DATABASE,               NULL},
    {"pg.db.blk_read_time",         CF_HAVEPARAMS,  PG_STAT_DATABASE,               NULL},
    {"pg.db.blk_write_time",        CF_HAVEPARAMS,  PG_STAT_DATABASE,               NULL},
    {"pg.db.stats_reset",           CF_HAVEPARAMS,  PG_STAT_DATABASE,               ",,postgres,,,"},
    
    // Table statistics (as per pg_stat_all_tables)
    {"pg.table.seq_scan",           CF_HAVEPARAMS,  PG_STAT_ALL_TABLES,             NULL},
    {"pg.table.seq_tup_read",       CF_HAVEPARAMS,  PG_STAT_ALL_TABLES,             NULL},
    {"pg.table.idx_scan",           CF_HAVEPARAMS,  PG_STAT_ALL_TABLES,             NULL},
    {"pg.table.idx_scan_perc",      CF_HAVEPARAMS,  PG_TABLE_IDX_SCAN_PERC,         NULL},
    {"pg.table.idx_tup_fetch",      CF_HAVEPARAMS,  PG_STAT_ALL_TABLES,             NULL},
    {"pg.table.n_tup_ins",          CF_HAVEPARAMS,  PG_STAT_ALL_TABLES,             NULL},
    {"pg.table.n_tup_upd",          CF_HAVEPARAMS,  PG_STAT_ALL_TABLES,             NULL},
    {"pg.table.n_tup_del",          CF_HAVEPARAMS,  PG_STAT_ALL_TABLES,             NULL},
    {"pg.table.n_tup_hot_upd",      CF_HAVEPARAMS,  PG_STAT_ALL_TABLES,             NULL},
    {"pg.table.n_live_tup",         CF_HAVEPARAMS,  PG_STAT_ALL_TABLES,             NULL},
    {"pg.table.n_dead_tup",         CF_HAVEPARAMS,  PG_STAT_ALL_TABLES,             NULL},
    {"pg.table.n_mod_since_analyze",CF_HAVEPARAMS,  PG_STAT_ALL_TABLES,             NULL},
    {"pg.table.last_vacuum",        CF_HAVEPARAMS,  PG_STAT_ALL_TABLES,             ",,pg_database"},
    {"pg.table.last_autovacuum",    CF_HAVEPARAMS,  PG_STAT_ALL_TABLES,             ",,pg_database"},
    {"pg.table.last_analyze",       CF_HAVEPARAMS,  PG_STAT_ALL_TABLES,             ",,pg_database"},
    {"pg.table.last_autoanalyze",   CF_HAVEPARAMS,  PG_STAT_ALL_TABLES,             ",,pg_database"},
    {"pg.table.vacuum_count",       CF_HAVEPARAMS,  PG_STAT_ALL_TABLES,             NULL},
    {"pg.table.autovacuum_count",   CF_HAVEPARAMS,  PG_STAT_ALL_TABLES,             NULL},
    {"pg.table.analyze_count",      CF_HAVEPARAMS,  PG_STAT_ALL_TABLES,             NULL},
    {"pg.table.autoanalyze_count",  CF_HAVEPARAMS,  PG_STAT_ALL_TABLES,             NULL},
    
    // Table IO Statistics (as per pg_statio_all_tables)
    {"pg.table.heap_blks_read",     CF_HAVEPARAMS,  PG_STATIO_ALL_TABLES,           NULL},
    {"pg.table.heap_blks_hit",      CF_HAVEPARAMS,  PG_STATIO_ALL_TABLES,           NULL},
    {"pg.table.heap_blks_perc",     CF_HAVEPARAMS,  PG_TABLE_HEAP_BLKS_PERC,        NULL},
    {"pg.table.idx_blks_read",      CF_HAVEPARAMS,  PG_STATIO_ALL_TABLES,           NULL},
    {"pg.table.idx_blks_hit",       CF_HAVEPARAMS,  PG_STATIO_ALL_TABLES,           NULL},
    {"pg.table.idx_blks_perc",      CF_HAVEPARAMS,  PG_TABLE_IDX_BLKS_PERC,         NULL},
    {"pg.table.toast_blks_read",    CF_HAVEPARAMS,  PG_STATIO_ALL_TABLES,           NULL},
    {"pg.table.toast_blks_hit",     CF_HAVEPARAMS,  PG_STATIO_ALL_TABLES,           NULL},
    {"pg.table.toast_blks_perc",    CF_HAVEPARAMS,  PG_TABLE_TOAST_BLKS_PERC,       NULL},
    {"pg.table.tidx_blks_read",     CF_HAVEPARAMS,  PG_STATIO_ALL_TABLES,           NULL},
    {"pg.table.tidx_blks_hit",      CF_HAVEPARAMS,  PG_STATIO_ALL_TABLES,           NULL},
    {"pg.table.tidx_blks_perc",     CF_HAVEPARAMS,  PG_TABLE_TIDX_BLKS_PERC,        NULL},
    
    // Index statistics (as per pg_stat_all_indexes)
    {"pg.index.idx_scan",           CF_HAVEPARAMS,  PG_STAT_ALL_INDEXES,            NULL},
    {"pg.index.idx_tup_read",       CF_HAVEPARAMS,  PG_STAT_ALL_INDEXES,            NULL},
    {"pg.index.idx_tup_fetch",      CF_HAVEPARAMS,  PG_STAT_ALL_INDEXES,            NULL},
    
    // Index IO statistics (as per pg_statio_all_indexes)
    {"pg.index.idx_blks_read",      CF_HAVEPARAMS,  PG_STATIO_ALL_INDEXES,          NULL},
    {"pg.index.idx_blks_hit",       CF_HAVEPARAMS,  PG_STATIO_ALL_INDEXES,          NULL},
    {"pg.index.idx_blks_perc",      CF_HAVEPARAMS,  PG_INDEX_IDX_BLKS_PERC,         NULL},
    
    // Null terminator
    {NULL}
};

// Required Zabbix module functions
int         zbx_module_api_version()                { return ZBX_MODULE_API_VERSION_ONE; }
void        zbx_module_item_timeout(int timeout)    { return; }
ZBX_METRIC  *zbx_module_item_list()                 { return keys; }
int         zbx_module_uninit()                     { return ZBX_MODULE_OK; }

int         zbx_module_init() { 
    // log version on startup
    zabbix_log(LOG_LEVEL_INFORMATION, "Starting agent module %s", STRVER);
    return ZBX_MODULE_OK; 
}

/*
 * Custom key: pg.modver
 *
 * Returns the version string of the libzbxpgsql module.
 *
 * Parameters:
 *
 * Returns: s
 */
 int    MODVER(AGENT_REQUEST *request, AGENT_RESULT *result)
 {
    int         ret = SYSINFO_RET_FAIL;         // Request result code
    const char  *__function_name = "MODVER";    // Function name for log file
    
    zabbix_log(LOG_LEVEL_DEBUG, "In %s", __function_name);
    
    // Set result
    SET_STR_RESULT(result, strdup(STRVER));
    ret = SYSINFO_RET_OK;
    
    zabbix_log(LOG_LEVEL_DEBUG, "End of %s", __function_name);
    return ret;
}

/* 
 * Function: pg_exec
 *
 * Wrapper for PQexecParams. Only supports text parameters as binary parameters
 * are not possible in Zabbix item keys.
 *
 * Returns: PGresult
 */
PGresult    *pg_exec(PGconn *conn, const char *command, PGparams params) {
    PGresult *res = NULL;
    int      i = 0, nparams = 0;

    // count parameters
    nparams = param_len(params);

    // log the query
    zabbix_log(LOG_LEVEL_DEBUG, "Executing query with %i parameters: %s", nparams, command);
    for (i = 0; i < nparams; i++)
        zabbix_log(LOG_LEVEL_DEBUG, "  $%i: %s", i, params[i]);

    // execute query with escaped parameters
    res = PQexecParams(conn, command, nparams, NULL, (const char * const*) params, NULL, NULL, 0);

    //free up the params array which would have been alloc'ed for this request
    param_free(params);

    return res;
}

/* 
 * Function: pg_version
 *
 * Returns a comparable version number (e.g 80200 or 90400) for the connected
 * PostgreSQL server version.
 *
 * Returns: int
 */
long int pg_version(AGENT_REQUEST *request) {
    const char  *__function_name = "pg_version"; // Function name for log file

    PGconn      *conn = NULL;
    PGresult    *res = NULL;
    long int    version = 0;

    zabbix_log(LOG_LEVEL_DEBUG, "In %s", __function_name);

    // connect to PostgreSQL
    conn = pg_connect_request(request);
    if (NULL == conn)
        goto out;

    // get server version
    res = pg_exec(conn, "SELECT setting FROM pg_settings WHERE name='server_version_num'", NULL);
    if(0 == PQntuples(res)) {
        zabbix_log(LOG_LEVEL_ERR, "Failed to get PostgreSQL server version");
        goto out;
    }

    // convert to integer
    version = atol(PQgetvalue(res, 0, 0));
    zabbix_log(LOG_LEVEL_DEBUG, "PostgreSQL server version: %lu", version);

out:
    PQclear(res);
    PQfinish(conn);
    
    zabbix_log(LOG_LEVEL_DEBUG, "End of %s", __function_name);

    return version;
}

/*
 * Function: pg_get_result
 *
 * Executes a PostgreSQL Query using connection details from a Zabbix agent
 * request structure and updates the agent result structure with the value of
 * the first column of the first row returned.
 *
 * type may be 
 *
 * Query parameters may be provided as a NULL terminated sequence of *char
 * values in the ... parameter.
 *
 * Parameter [request]: Zabbix agent request structure.
 *          Passed to pg_connect_request to fetch as valid PostgreSQL
 *          server connection
 *
 * Parameter [result]:  Zabbix agent result structure
 *
 * Parameter [type]:    Result type to set. May be one of AR_STRING, AR_UINT64
 *          or AR_DOUBLE.
 *
 * Paramater [query]:   PostgreSQL query to execute. Query should return a
 *          single scalar string value. Parameters defined using PostgreSQL's
 *          '$n' notation will be replaced with the corresponding variadic
 *          argument provided in ...
 *
 * Returns: SYSINFO_RET_OK or SYSINFO_RET_FAIL on error
 */
int    pg_get_result(AGENT_REQUEST *request, AGENT_RESULT *result, int type, const char *query, PGparams params)
{
    int         ret = SYSINFO_RET_FAIL;             // Request result code
    const char  *__function_name = "pg_get_result"; // Function name for log file
    
    PGconn      *conn = NULL;
    PGresult    *res = NULL;
    char        *value = NULL;
    
    zabbix_log(LOG_LEVEL_DEBUG, "In %s(%s)", __function_name, request->key);
    
    // Connect to PostreSQL
    if(NULL == (conn = pg_connect_request(request)))
        goto out;
    
    // Execute a query
    res = pg_exec(conn, query, params);

    if(PQresultStatus(res) != PGRES_TUPLES_OK) {
        zabbix_log(LOG_LEVEL_ERR, "Failed to execute PostgreSQL query in %s(%s) with: %s", __function_name, request->key, PQresultErrorMessage(res));
        goto out;
    }
    
    if(0 == PQntuples(res)) {
        zabbix_log(LOG_LEVEL_DEBUG, "No results returned for query \"%s\" in %s(%s)", query, __function_name, request->key);
        goto out;
    }
    
    // get scalar value (freed later by PQclear)
    value = PQgetvalue(res, 0, 0);

    // Set result
    switch(type) {
        case AR_STRING:
            // string result (zabbix will clean the strdup'd buffer)
            SET_STR_RESULT(result, strdup(value));
            break;

        case AR_UINT64:
            // integer result
            // Convert E Notation
            if(1 < strlen(value) && '.' == value[1]) {
                double dbl = strtod(value, NULL);
                SET_UI64_RESULT(result, (unsigned long long) dbl);
            } else {
                SET_UI64_RESULT(result, strtoull(value, NULL, 10));
            }
            break;

        case AR_DOUBLE:
            // double result
            SET_DBL_RESULT(result, strtold(value, NULL));
            break;

        default:
            // unknown result type
            zabbix_log(LOG_LEVEL_ERR, "Unsupported result type: 0x%0X in %s", type, __function_name);
            goto out;
    }

    ret = SYSINFO_RET_OK;
    
out:
    PQclear(res);
    PQfinish(conn);
    
    zabbix_log(LOG_LEVEL_DEBUG, "End of %s(%s)", __function_name, request->key);
    return ret;
}

#define PGSQL_PERCENTAGE    "\
SELECT \
    CASE \
        WHEN (%s) = 0 THEN 1 \
        ELSE (%s)::float / (%s) \
    END \
FROM %s"

int pg_get_percentage(AGENT_REQUEST *request, AGENT_RESULT *result, char *table, char *col1, char *col2, char *colFilter, char *filter)
{
    int         ret = SYSINFO_RET_FAIL;                 // Request result code
    const char  *__function_name = "pg_get_percentage"; // Function name for log file
    
    int         qlen = 0;
    char        query[MAX_STRING_LEN], *c = NULL;

    zabbix_log(LOG_LEVEL_DEBUG, "In %s(%s)", __function_name, request->key);

    zbx_snprintf(query, sizeof(query), PGSQL_PERCENTAGE, col2, col1, col2, table);
    if (!strisnull(colFilter)) {
        qlen = strlen(query);
        c = &query[qlen];
        zbx_snprintf(c, (sizeof(query) / sizeof(char)) - qlen, " WHERE %s = $1", colFilter);
    }

    ret = pg_get_dbl(request, result, query, param_new(filter));

    zabbix_log(LOG_LEVEL_DEBUG, "End of %s(%s)", __function_name, request->key);
    return ret; 
}

/*
 * Function: is_oid
 * 
 * Returns: 1 if the specified string is a valid PostgreSQL OID
 *
 * See also: http://www.postgresql.org/docs/9.4/static/datatype-oid.html
 */
int is_oid(char *str)
{
    char *p = NULL;
    int res = 0;

    for(p = str; '\0' != *p; p++) {
        if (0 == isdigit(*p))
            return 0;
        res = 1;
    }

    return res;
}

/* 
 * Function: is_valid_ip
 * 
 * Returns: 1 if the specified string is a valid IPv4 or IPv6 address
 */
int is_valid_ip(char *str)
{
    struct in6_addr in;
    int res = 0;

    // test for valid IPv4 address
    if(1 == inet_pton(AF_INET, str, &(in)))
        res = 1;

    // test for valid IPv6 address
    if(1 == inet_pton(AF_INET6, str, &(in)))
        res = 1;
    
    return res;
}

/*
 * Function: strcat2
 *
 * An attempt to improve the performance and usability of strcat.
 * Buffer sizing is the responsibility of the caller.
 *
 * Returns: pointer to the last character of the updated destination string
 */
char *strcat2(char *dest, const char *src)
{
    if (NULL == dest || NULL == src)
        return dest;

    // seek to the end of the dest string
    while (*dest) dest++;

    // copy one char at a time from source
    while (*dest++ == *src++);
    
    // return the last character
    return --dest;
}
