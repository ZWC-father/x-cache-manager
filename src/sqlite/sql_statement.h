#define SQL_CREATE_METALRU "CREATE TABLE metaLRU (" \
                           "id INTEGER PRIMARY KEY CHECK (id = 1), " \
                           "cache_size INTEGER NOT NULL, " \
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

#define SQL_UPDATE_SEQ_LRU "UPDATE cacheLRU " \
                           "SET sequence = ? " \
                           "WHERE key = ?;"

#define SQL_UPDATE_CONTENT_LRU "UPDATE cacheLRU " \
                               "SET size = ? WHERE key = ?;"

#define SQL_QUERY_COUNT_LRU "SELECT COUNT(*) FROM cacheLRU " \
                            "WHERE key = ?;"

#define SQL_QUERY_SINGLE_LRU "SELECT * FROM cacheLRU " \
                             "WHERE key = ?;"

#define SQL_QUERY_OLD_LRU "SELECT * FROM cacheLRU " \
                          "ORDER BY sequence ASC LIMIT 1;"

#define SQL_QUERY_NEW_LRU "SELECT * FROM cacheLRU " \
                          "ORDER BY sequence DESC LIMIT 1;"

#define SQL_QUERY_ALL_LRU "SELECT * FROM cacheLRU " \
                          "ORDER BY sequence ASC;"

#define SQL_DELETE_SINGLE_LRU "DELETE FROM cacheLRU WHERE key = ? " \
                              "RETURNING key, size, download_time, " \
                              "hash, sequence;"

#define SQL_DELETE_OLD_LRU "DELETE FROM cacheLRU " \
                           "WHERE key = (" \
                           "SELECT key FROM cacheLRU " \
                           "ORDER BY sequence ASC " \
                           "LIMIT 1) " \
                           "RETURNING key, size, download_time, " \
                           "hash, sequence;"

/*------------------------------------------------------------------*/
#define SQL_CREATE_METALFUDA "CREATE TABLE metaLFUDA (" \
                             "id INTEGER PRIMARY KEY CHECK (id = 1), " \
                             "cache_size INTEGER NOT NULL, " \
                             "max_size INTEGER NOT NULL, " \
                             "global_aging INTEGER NOT NULL" \
                             ");"

#define SQL_CHECK_METALFUDA "SELECT name FROM sqlite_master " \
                            "WHERE type = 'table' " \
                            "AND name = 'metaLFUDA';"

#define SQL_INSERT_METALFUDA "INSERT INTO metaLFUDA " \
                             "(id, cache_size, max_size, " \
                             "global_aging) VALUES (1, ?, ?, ?);"

#define SQL_UPDATE_METALFUDA "UPDATE metaLFUDA " \
                             "SET cache_size = ?, max_size = ?, " \
                             "global_aging = ? WHERE id = 1;"

#define SQL_QUERY_METALFUDA "SELECT cache_size, max_size, " \
                            "global_aging FROM metaLFUDA;"

/*------------------------------------------------------------------*/
#define SQL_CREATE_LFUDA "CREATE TABLE cacheLFUDA (" \
                         "key TEXT PRIMARY KEY, " \
                         "size INTEGER NOT NULL, " \
                         "download_time INTEGER, " \
                         "hash BLOB, " \
                         "timestamp INTEGER NOT NULL, " \
                         "freq INTEGER NOT NULL, " \
                         "eff INTEGER NOT NULL" \
                         ");"

#define SQL_CREATE_INDEX_LFUDA "CREATE INDEX IF NOT EXISTS indexLFUDA " \
                               "ON cacheLFUDA(eff ASC, freq ASC, " \
                               "timestamp ASC, size ASC);"

#define SQL_CHECK_LFUDA "SELECT name FROM sqlite_master " \
                        "WHERE type = 'table' " \
                        "AND name = 'cacheLFUDA';"

#define SQL_INSERT_LFUDA "INSERT INTO cacheLFUDA " \
                         "(key, size, download_time, " \
                         "hash, timestamp, freq, eff) " \
                         "VALUES (?, ?, ?, ?, ?, ?, ?);"

//timestamp + freq + eff = TFE
#define SQL_UPDATE_TFE_LFUDA "UPDATE cacheLFUDA " \
                             "SET timestamp = ?, " \
                             "freq = ?, eff = ? " \
                             "WHERE key = ?;" \

#define SQL_UPDATE_CONTENT_LFUDA "UPDATE cacheLFUDA " \
                                 "SET size = ? WHERE key = ?;"

#define SQL_QUERY_COUNT_LFUDA "SELECT COUNT(*) FROM cacheLFUDA " \
                              "WHERE key = ?;"

#define SQL_QUERY_SINGLE_LFUDA "SELECT * FROM cacheLFUDA " \
                               "WHERE key = ?;"

#define SQL_QUERY_TFE_LFUDA  "SELECT key, timestamp, freq, eff " \
                             "FROM cacheLFUDA WHERE key = ?;"

#define SQL_QUERY_WORST_LFUDA "SELECT * FROM cacheLFUDA " \
                              "ORDER BY eff ASC, freq ASC, " \
                              "timestamp ASC, size ASC LIMIT 1;"

#define SQL_QUERY_ALL_LFUDA "SELECT * FROM cacheLFUDA " \
                            "ORDER BY eff ASC, freq ASC, " \
                            "timestamp ASC, size ASC;"

#define SQL_DELETE_SINGLE_LFUDA "DELETE FROM cacheLFUDA WHERE key = ? " \
                                "RETURNING key, size, download_time, " \
                                "hash, timestamp, freq, eff;"

#define SQL_DELETE_WORST_LFUDA "DELETE FROM cacheLFUDA " \
                               "WHERE key = (" \
                               "SELECT key FROM cacheLFUDA " \
                               "ORDER BY eff ASC, freq ASC, " \
                               "timestamp ASC, size ASC " \
                               "LIMIT 1) " \
                               "RETURNING key, size, download_time, " \
                               "hash, timestamp, freq, eff;"

