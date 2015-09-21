#ifndef DBLIB_H
#define DBLIB_H

#include "common.h"

struct mc_accu;  // Needs forward declaration

mongoc_client_t *db_init();
mongoc_collection_t *db_connect(mongoc_client_t *client, char *db_name,
                                char *collection_name);
void db_disconnect(mongoc_collection_t *dbc);
void db_free(mongoc_client_t *client);
void db_update_watchdog(mongoc_collection_t *dbc, time_t ts);
void db_insert_sdd(mongoc_collection_t *dbc, size_t rx, const char *ns_name,
                   time_t ts, double esno);
void db_insert_mc(mongoc_collection_t *dbc, struct mc_accu *accu);
int db_get_esno_avg(mongoc_collection_t *dbc, char *rx_name, char *ns_name,
                    time_t ts_begin, double *esno, int *count);

#endif // DBLIB_H
