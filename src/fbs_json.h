//Copyright 2014 David Yu
//------------------------------------------------------------------------
//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at 
//http://www.apache.org/licenses/LICENSE-2.0
//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.
//========================================================================

#ifndef NGX_HTTP_FBS_JSON_H
#define NGX_HTTP_FBS_JSON_H

extern "C" {

#include <ngx_core.h>
#include <ngx_http.h>

}

typedef struct {
    ngx_str_t schema;

} ngx_http_fbs_conf_t;

char * fbs_schema_parse(ngx_pool_t *pool, ngx_str_t schema);

ngx_int_t fbs_json_parse(ngx_http_request_t *r, ngx_str_t *res, ngx_http_variable_value_t *v, 
    ngx_http_fbs_conf_t *conf);

#endif /* NGX_HTTP_FBS_JSON_H */

