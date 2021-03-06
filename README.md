# Name

**fbs-nginx-module**
- decode json from request body (application/json) into a flatbuffer message.

*This module is not distributed with the Nginx source.* See the
[install](#install) instructions.

# Usecase

Suppose your upstream/backend is a json rpc/rest api server and you're load-balancing your requests
via nginx (or simply using it as [ssl termination](http://nginx.com/resources/admin-guide/nginx-ssl-termination/)).  

This module allows you to:
- pre-validate the structure and size of the json request/payload before sending it upstream
- send a binary flatbuffer to upstream instead (json payload is converted to a flatbuffer binary)

In the future, more validation will be added (fields with validation attributes in the fbs_schema).

# Status
This module is in active development.

# Synopsis
```
dso {
    load ngx_http_fbs_module.so;
}

http {
    #...

    server {
        #...
        
        fbs_schema "
            namespace foo;

            table Foo {
                id: uint = 0 (id:  0);
                name: string (id: 1);
                description: string (id:  2);
            }
            
            struct Baz {
                kind: ubyte = 0;
            }
            
            table Bar (original_order) {
                title: string (id: 0);
                baz: Baz (id: 1);
                foo: Foo (id: 2);
            }
        ";

        location = /foo {
            set_fbs_from_json $Foo;
            
            if ( $Foo = "" ) { return 400; }
            
            return 200;
        }

        location = /bar {
            # limit the json payload
            client_max_body_size 16k;
            client_body_buffer_size 16k;
        
            # can be set/assigned to a different var
            set_fbs_from_json $hello Bar;
            
            # returns 400 if the json payload is too large or invalid.
            if ( $hello = "" ) { return 400; }
            
            # sucessful ... proceed to upstream (for example: uwsgi)
            
            # send the binary flatbuffer as a param
            uwsgi_param message $hello;
            
            # we've already processed the request body (converted json to a flatbuffer binary)
            uwsgi_pass_request_body off;
            
            uwsgi_pass unix:/tmp/uwsgi.socket;
        }
    }
}
```

## Install

I have **not** figured out how to compile this (c++11) with vanilla nginx .

At the moment, you'll have to compile this module as a dynamic shared lib/object ([DSO](http://tengine.taobao.org/document/dso.html)) with [tengine](http://tengine.taobao.org).

Note that a c++11 compiler is required to build this module (gcc >= 4.7 would be fine).

Steps:
- [Install tengine](http://tengine.taobao.org/document/install.html) (--prefix=/opt/tengine)
- Clone and setup the repo: 
  - ```git clone https://github.com/dyu/fbs-nginx-module && cd fbs-nginx-module && git clone https://github.com/dyu/flatbuffers && cd flatbuffers && git checkout dev```
- Install the module:
  - ```/opt/tengine/sbin/dso_tool -a=/path/to/fbs-nginx-module```

To verify that it works, follow the instructions in [tests](tests/README.md)

## Todo

Further optimize json parsing (make flatbuffers use nginx's allocator - request context)

Remove the limitation of 64 fields per table (Or increase the limit to 128).
 - having this artifical limit allows the code to check for duplicate json fields more efficiently.


## License
Apache v2

Copyright 2014 David Yu

## Credits
The code was largely based from:
- [form-input-nginx-module](http://github.com/calio/form-input-nginx-module) by [calio](http://github.com/calio), [agentzh](http://github.com/agentzh)
- [encrypted-session-nginx-module](http://github.com/openresty/encrypted-session-nginx-module) by [agentzh](http://github.com/agentzh)

This repo contains code (modified to remove c++11 warnings) from [ngx_devel_kit](http://github.com/simpl/ngx_devel_kit), namely:
- ``` ndk_rewrite ```
- ``` ndk_set_var ```
