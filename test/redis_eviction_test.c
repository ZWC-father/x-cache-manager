#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>

#define VALUE_SIZE    1024     // 每个 value 占 1KB
#define LOG_INTERVAL 100000    // 每写入这么多键，打印一次进度

int main(void) {
    // 1. 同步连接到 Redis
    redisContext *ctx = redisConnect("127.0.0.1", 6379);
    if (!ctx || ctx->err) {
        fprintf(stderr, "Connection error: %s\n",
                ctx ? ctx->errstr : "allocation failure");
        return 1;
    }

    // 2. 准备固定大小的 value
    char *value = (char*)malloc(VALUE_SIZE);
    if (!value) {
        fprintf(stderr, "Failed to allocate value buffer\n");
        redisFree(ctx);
        return 1;
    }
    memset(value, 'x', VALUE_SIZE);

    // 3. 无限循环写入 key:0, key:1, key:2, …
    for (long long i = 0; ; ++i) {
        redisReply *r = (redisReply*)redisCommand(ctx,
                                     "SET key:%lld %b",
                                     i,
                                     value, VALUE_SIZE);
        if (!r) {
            fprintf(stderr, "SET error: %s\n", ctx->errstr);
            break;
        }
        freeReplyObject(r);

        if ((i % LOG_INTERVAL) == 0) {
            printf("Written %lld keys so far\n", i);
            fflush(stdout);
        }
    }

    // 4. 清理
    free(value);
    redisFree(ctx);
    return 0;
}

