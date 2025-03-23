#define SQL_CREATE_METALRU "CREATE TABLE metaLRU (" \
                           "id INTEGER PRIMARY KEY CHECK (id = 1), " \
                           "cache_size INTEGER, " \
                           "max_size INTEGER NOT NULL, " \
                           "max_sequence INTEGER NOT NULL" \
                           ");"

#define SQL_CHECK_METALRU "SELECT name FROM sqlite_master " \
                          "WHERE type = 'table' " \
                          "AND name = 'metaLRU';"

#define SQL_INSERT_METALRU "INSERT INTO metaLRU " \
                           "(id, cache_size, max_size, " \
                           "max_sequence) VALUES (1, ?, ?, ?);"

#define SQL_UPDATE_METALRU "UPDATE metaLRU " \
                           "SET cache_size = ?, max_size = ?, " \
                           "max_sequence = ? WHERE id = 1;"

#define SQL_QUERY_METALRU "SELECT cache_size, max_size, " \
                          "max_sequence FROM metaLRU;"

/*------------------------------------------------------------------*/
#define SQL_CREATE_LRU "CREATE TABLE cacheLRU (" \
                       "key TEXT PRIMARY KEY, " \
                       "size INTEGER NOT NULL, " \
                       "download_time INTEGER, " \
                       "hash BLOB, " \
                       "sequence INTEGER NOT NULL UNIQUE" \
                       ");"

#define SQL_CREATE_INDEX_LRU "CREATE INDEX IF NOT EXISTS indexLRU " \
                             "ON cacheLRU(sequence ASC);"

#define SQL_CHECK_LRU "SELECT name FROM sqlite_master " \
                      "WHERE type = 'table' " \
                      "AND name = 'cacheLRU';"

#define SQL_INSERT_LRU "INSERT INTO cacheLRU " \
                       "(key, size, download_time, " \
                       "hash, sequence) " \
                       "VALUES (?, ?, ?, ?, ?);"

#define SQL_UPDATE_LRU_CONTENT "UPDATE cacheLRU " \
                               "SET size = ? WHERE key = ?;"

#define SQL_QUERY_NUM_LRU "SELECT COUNT(*) FROM cacheLRU " \
                          "WHERE key = ?;"

#define SQL_QUERY_OLD_LRU "SELECT * FROM cacheLRU " \
                          "ORDER BY sequence DESC LIMIT 1;"

#define SQL_QUERY_ALL_LRU "SELECT * FROM cacheLRU;"

#define SQL_DELETE_OLD_LRU "DELETE FROM cacheLRU " \
                           "WHERE key = ( " \
                           "SELECT key FROM cacheLRU " \
                           "ORDER BY sequence ASC " \
                           "LIMIT 1) " \
                           "RETURNING key, size, download_time " \
                           "hash, sequence;"

#define SQL_DELETE_ANY_LRU "DELETE FROM cacheLRU WHERE key = ? " \
                           "RETURNING key, size, download_time, " \
                           "hash, sequence;"

/*------------------------------------------------------------------*/
#define SQL_CREATE_LFUDA "CREATE TABLE cacheLFUDA (" \
                         "key TEXT PRIMARY KEY, " \
                         "size INTEGER, " \
                         "download_time INTEGER, " \
                         "hash BLOB, " \
                         "timestamp INTEGER, " \
                         "freq INTEGER, " \
                         "eff INTEGER" \
                         ");"

#define SQL_CHECK_LFUDA "SELECT name FROM sqlite_master " \
                        "WHERE type = 'table' " \
                        "AND name = 'cacheLFUDA';"

#define SQL_INSERT_LFUDA "INSERT INTO cacheLFUDA " \
                         "(key, size, download_time, " \
                         "hash, timestamp, freq, eff) " \
                         "VALUES (?, ?, ?, ?, ?, ?, ?);"

#define SQL_UPDATE_LFUDA_CONTENT "UPDATE cacheLFUDA " \
                                 "SET size = ?, hash = ? " \
                                 "WHERE key = ?;"

#define SQL_QUERY_ALL_LFUDA "SELECT * FROM cacheLFUDA;"

