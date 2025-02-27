# x-cache-manager
A nginx proxy_cache manager for caching replacement
(This project is not finished yet)

## Use case
It was designed for PyPi mirror initially.
And is also suitable for other static content caching replacement.

```                           
 | upsteam-server |  <---  | nginx |  <----  | client |         
               streaming log  / \  purging cache                
               via socket    /   \ by ngx_proxy_purge (a community nginx mudule)                                   
                            /     \                             
                           /       \                            
     | file log | ---> | x-cache-manager |  <---> | SQLite3/Redis |
```               

## Features
- Using unix domain socket to collect access log (nginx should be configured with json log format)
- Based on LRU/LFUDA or [WGDSF](https://ieeexplore.ieee.org/document/8258989) algorithm to determine the popularity of the resource
- Multiple groups of caching profiles for different kind of **static** content
- Using ngx_cache_purge to purge the unused cache (thirdparty)
- Fast caching replacememt for tightly limited disk space
- SQLite3 database, also support Redis for large instance

## TODO
- Basic function
- Automatically gathering Project/Package info for better evaluation (for PyPi/Anaconda mirror)
- Support purging file directly from filesystem (not relying on ngx_cache_purge)
- Develop a more stable purger
- Automatically check the validity of cached resource
- Support multiple use cases and webserver
- Dynamic loading configuration
