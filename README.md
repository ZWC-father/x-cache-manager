# x-cache-manager
A nginx proxy_cache manager for caching replacement
(This project is not finished yet)

## Usecase
It was designed for PyPi mirror initially.
And is also suitable for other static resource caching replacement.

```                           
 | upsteam-server |  <---  | nginx |  <----  | client |         
               streaming log  / \  purging cache                
               via socket    /   \ by ngx_proxy_purge                                   \
                            /     \                             
                           /       \                            
     | file log | ---> | x-cache-manager |  <---> | SQLite3/Redis |
```               

## features
- Dynamic caching replacement for limited 
