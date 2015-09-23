#include "dblib.h"

static void db_insert(mongoc_collection_t *dbc, bson_t *doc);

/**
 * Initialize database connection
 */
mongoc_client_t *db_init()
{
	mongoc_init();

	// Create db connection
	return mongoc_client_new("mongodb://localhost:27017");
}

/**
 * Initialize specific collection connection
 */
mongoc_collection_t *db_connect(mongoc_client_t *client, char *db_name, char *collection_name)
{
	// Use existing client to connect to collection
	return mongoc_client_get_collection(client, db_name, collection_name);
}

/**
 * Disconnect from collection
 */
void db_disconnect(mongoc_collection_t *dbc)
{
	mongoc_collection_destroy(dbc);
}

/**
 * Disconnect from database
 */
void db_free(mongoc_client_t *client)
{
	mongoc_client_destroy(client);
	mongoc_cleanup();
}

/**
 * Helper function to insert new record in MongoDB database
 */
static void db_insert(mongoc_collection_t *dbc, bson_t *doc)
{
	bson_error_t error;

	if (!mongoc_collection_insert(dbc, MONGOC_INSERT_NONE, doc, NULL, &error)) {
		fprintf(stderr, "MongoDB insert failed: %s\n", error.message);
	}
}

/**
 * Update watchdog timer
 */
void db_update_watchdog(mongoc_collection_t *dbc, time_t ts)
{
	bson_t *query, *update;
	bson_error_t error;

	// Construct query
	query = BCON_NEW("key", "watchdog_ts");

	// Construct the new document
	update = bson_new();
	bson_append_utf8(update, "key", -1, "watchdog_ts", -1);
	bson_append_time_t(update, "val", -1, ts);

	// Upsert new document into database
	if (!mongoc_collection_update(dbc, MONGOC_UPDATE_UPSERT, query, update, NULL, &error)) {
		printf("Watchdog update failed: %s\n", error.message);
	}

	// Free memory
	bson_destroy(query);
	bson_destroy(update);
}

/**
 * Wrapper to insert new SDD record into database
 */
void db_insert_sdd(mongoc_collection_t *dbc, size_t rx, const char *ns_name,
                   time_t ts, double esno)
{
	bson_oid_t oid;
	bson_t *doc;

	char rx_name[4];
	switch (rx) {
	case RX1: strncpy(rx_name, "RX1", 4); break;
	case RX2: strncpy(rx_name, "RX2", 4); break;
	}

	doc = bson_new();
	bson_oid_init(&oid, NULL);
	bson_append_oid(doc, "_id", -1, &oid);
	bson_append_utf8(doc, "rx", -1, rx_name, -1);
	bson_append_utf8(doc, "ns", -1, ns_name, -1);
	bson_append_time_t(doc, "ts", -1, ts);
	bson_append_double(doc, "esno", -1, esno);

	db_insert(dbc, doc);

	bson_destroy(doc);
}

/**
 * Wrapper to insert new MODCOD record into database
 */
void db_insert_mc(mongoc_collection_t *dbc, struct mc_accu *accu)
{
	bson_oid_t oid;
	bson_t *doc;
	bson_t *arr;
	uint64_t *modcods;
	char i_to_string[3];

	modcods = accu->perc + 1;

	arr = bson_new();
	for (int i = 0; i < 28; ++i) {
		snprintf(i_to_string, 3, "%d", i);
		bson_append_int32(arr, i_to_string, -1, modcods[i]);
	}

	doc = bson_new();
	bson_oid_init(&oid, NULL);
	bson_append_oid(doc, "_id", -1, &oid);
	bson_append_time_t(doc, "ts", -1, accu->ts);
	bson_append_double(doc, "bit_rate", -1, accu->bit_rate / 1000000.0);
	bson_append_int64(doc, "total", -1, accu->curr[0]);
	bson_append_array(doc, "arr", -1, arr);

	db_insert(dbc, doc);

	bson_destroy(arr);
	bson_destroy(doc);
}

/**
 * Get the average EsNo for specific [rx, ns] from 'ts_begin' to now. Uses the
 * Mongo aggregation framework. May be quite heavyweight. The results are given
 * through the referenced esno/count variables
 *
 * @return 1 on success, else 0
 */
int db_get_esno_avg(mongoc_collection_t *dbc, char *rx_name, char *ns_name,
                    time_t ts_begin, double *esno, int *count)
{
	bson_t *pipeline;
	mongoc_cursor_t *cursor = NULL;
	bson_iter_t iter;
	const bson_t *res;

	ts_begin *= 1000;

	pipeline = BCON_NEW(
		"pipeline", "[",
		  "{", "$match",
		    "{",
		      "rx", BCON_UTF8(rx_name),
		      "ns", BCON_UTF8(ns_name),
		      "ts", "{", "$gt", BCON_DATE_TIME(ts_begin), "}",
		    "}",
		  "}",
		  "{", "$group",
		    "{",
		      "_id", "$ns",
		      "esno_avg", "{", "$avg", "$esno", "}",
		      "count", "{", "$sum", BCON_INT32(1), "}",
		    "}",
		  "}",
		"]");

	cursor = mongoc_collection_aggregate(dbc, MONGOC_QUERY_NONE, pipeline,
	                                     NULL, NULL);

	bson_destroy(pipeline);

	if (!mongoc_cursor_next(cursor, &res)) {
		return 0;
	}

	if (!(bson_iter_init_find(&iter, res, "esno_avg") &&
	     (BSON_ITER_HOLDS_DOUBLE(&iter)))) {
		mongoc_cursor_destroy(cursor);
		return 0;
	}

	*esno = bson_iter_double(&iter);

	if (!(bson_iter_init_find(&iter, res, "count") &&
	     (BSON_ITER_HOLDS_INT32(&iter)))) {
		mongoc_cursor_destroy(cursor);
		return 0;
	}

	*count = bson_iter_int32(&iter);

	mongoc_cursor_next(cursor, &res);
	if (mongoc_cursor_more(cursor)) {
		fprintf(stderr, "EsNo monitor: DB cursor not exhausted!\n");
		mongoc_cursor_destroy(cursor);
		return 0;
	}

	mongoc_cursor_destroy(cursor);

	return 1;
}
