#define SQL_CREATE_META "CREATE TABLE metadata (" \
                        "id INTEGER PRIMARY KEY CHECK (id = 1), " \
                        "config TEXT, " \
                        "cache_size INTEGER, " \
                        "lfuda_aging INTEGER" \
                        ");"

#define SQL_CREATE_LRU "CREATE TABLE cacheLRU (" \
                       "key TEXT PRIMARY KEY, " \
                       "size INTEGER, " \
                       "download_time INTEGER, " \
                       "hash BLOB, " \
                       "sequence INTEGER" \
                       ");"

#define SQL_CREATE_LFUDA "CREATE TABLE cacheLFUDA (" \
                         "key TEXT PRIMARY KEY, " \
                         "size INTEGER, " \
                         "download_time INTEGER, " \
                         "hash BLOB, " \
                         "timestamp INTEGER, " \
                         "freq INTEGER, " \
                         "eff INTEGER" \
                         ");"

#define SQL_INSERT_META "INSERT INTO metadata " \
                        "(id, config, cache_size, lfuda_aging) " \
                        "VALUES (1, ?, ?, ?);"

#define SQL_INSERT_LRU "INSERT INTO cacheLRU " \
                       "(key, size, download_time, " \
                       "hash, sequence) " \
                       "VALUES (?, ?, ?, ?, ?);"

#define SQL_INSERT_LFUDA "INSERT INTO cacheLFUDA " \
                         "(key, size, download_time, " \
                         "hash, timestamp, freq, eff) " \
                         "VALUES (?, ?, ?, ?, ?, ?, ?);"

#define SQL_UPDATE_META "UPDATE metadata " \
                        "SET config = ?, cache_size = ?, lfuda_aging = ? " \
                        "WHERE id = 1;"

/*
#define SQL_UPDATE_LRU "UPDATE cacheLRU " \
                       "SET sequence = ? " \
                       "WHERE key = ?;"

#define SQL_UPDATE_LRU_CONTENT "UPDATE cacheLRU " \
                               "SET size = ?, hash = ? " \
                               "WHERE key = ?;"

#define SQL_UPDATE_LFUDA "UPDATE cacheLFUDA " \
                         "SET timestamp = ?, " \
                         "freq = ?, eff = ? " \
                         "WHERE key = ?;"

#define SQL_UPDATE_LFUDA_CONTENT "UPDATE cacheLFUDA " \
                                 "SET size = ?, hash = ? " \
                                 "WHERE key = ?;"

#define SQL_DELETE_LRU "DELETE FROM cacheLRU " \
                       "WHERE key = ?;"

#define SQL_DELETE_LFUDA "DELETE FROM cacheLFUDA " \
                         "WHERE key = ?;"
*/

#define SQL_CHECK_META "SELECT name FROM sqlite_master " \
                       "WHERE type = 'table' " \
                       "AND name = 'metadata';"

#define SQL_CHECK_LRU "SELECT name FROM sqlite_master " \
                      "WHERE type = 'table' " \
                      "AND name = 'cacheLRU';"

#define SQL_CHECK_LFUDA "SELECT name FROM sqlite_master " \
                        "WHERE type = 'table' " \
                        "AND name = 'cacheLFUDA';"

#define SQL_QUERY_META "SELECT * FROM metadata;"

#define SQL_QUERY_LRU "SELECT * FROM cacheLRU;"

#define SQL_QUERY_LFUDA "SELECT * FROM cacheLFUDA;"

#define SQL_DELETE_LRU "DELETE * FROM cacheLRU;"

#define SQL_DELETE_LFUDA "DELETE * FROM cacheLFUDA;"

