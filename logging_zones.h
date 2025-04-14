#ifndef LOGGING_ZONES_H
#define LOGGING_ZONES_H

#define ZONE_NUM 16

enum{
     LOG_ZONE_MAIN,
     LOG_ZONE_REVEIVER,
     LOG_ZONE_PARSER,
     LOG_ZONE_CONF_PARSER,
     LOG_ZONE_CLEANER,
     LOG_ZONE_DELETER,
     LOG_ZONE_PURGER,
     LOG_ZONE_MONITOR,
     LOG_ZONE_SQLITE,
     LOG_ZONE_REDIS,
     LOG_ZONE_LRU,
     LOG_ZONE_LFUDA,
     LOG_ZONE_LRU_SQLITE,
     LOG_ZONE_LFUDA_SQLITE,
     LOG_ZONE_LRU_REDIS,
     LOG_ZONE_LFUDA_REDIS,
};

constexpr char* logging_prefix[] = {
    "[     main     ] ",
    "[   receiver   ] ",
    "[    parser    ] ",
    "[  conf_parser ] ",
    "[    cleaner   ] ",
    "[    deleter   ] ",
    "[    purger    ] ",
    "[    monitor   ] ",
    "[    sqlite    ] ",
    "[    redis     ] ",
    "[     lru      ] ",
    "[    lfuda     ] ",
    "[ lru(sqlite)  ] ",
    "[lfuda(sqlite) ] ",
    "[  lru(redis)  ] ",
    "[ lfuda(redis) ] "
};

#endif
