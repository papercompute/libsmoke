# smoke

[smoke](https://github.com/papercompute/smoke-epoll) high performance low latency multi-threaded udp/tcp/http/ws server built on top of linux epoll

current state: very experimental

design:

* accepting thread(s) / round robin
* processing thread(s) / IO


./wrk -t32 -c10000 -d5m --timeout 60s --latency http://127.0.0.1:9090/


