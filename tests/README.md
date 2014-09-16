## Test setup for [tengine](http://tengine.taobao.org)

On your nginx.conf (/opt/tengine/conf/nginx.conf), add:

```
dso {
    load ngx_http_fbs_module.so;
}
```

Inside the server {} block, add:
```
include /absolute/path/to/fbs-nginx-module/tests/conf/fbs.conf
```

Start tengine:
```
/opt/tengine/sbin/nginx
```

Run the tests (requires curl):
```
# specify the port configured in your nginx.conf
./foo_success.sh 8080
./bar_success.sh 8080
```

If it does not print anything to stdout, then the test is successful.
