# smoke

[smoke](https://github.com/papercompute/smoke-epoll) high performance low latency multi-threaded/processed udp/tcp/http/ws server built on top of linux epoll/pfring

current state: very experimental

design goals:

* no complexity, no external deps
* accepting thread(s) / round robin
* processing thread(s) / IO
* use event.data.fd w/ both IN | OUT

testing:

sudo sh -c "ulimit -n 65535 && exec su $LOGNAME"

./wrk -t32 -c10000 -d5m --timeout 60s --latency http://127.0.0.1:9090/
ab -n 1000000 -c 10000 http://localhost:9090/
curl -v http://localhost:9090/

