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

#ifndef DDEBUG
#define DDEBUG 0
#endif

extern "C" {

#include "ddebug.h"

#include "ndk_set_var.h"

} // extern C

#include "fbs_json.h"

#define json_type "application/json"
#define json_type_len (sizeof(json_type) - 1)

ngx_flag_t ngx_http_fbs_used = 0;

typedef struct {
    unsigned          done:1;
    unsigned          waiting_more_body:1;
} ngx_http_fbs_ctx_t;

static char * ngx_http_fbs_schema(ngx_conf_t *cf, 
        ngx_command_t *cmd, void *conf);

static ngx_int_t ngx_http_set_fbs_from_json(ngx_http_request_t *r,
        ngx_str_t *res, ngx_http_variable_value_t *v);
    
static char * ngx_http_set_fbs_conf_handler(ngx_conf_t *cf,
        ngx_command_t *cmd, void *conf);

static ngx_int_t ngx_http_fbs_init(ngx_conf_t *cf);
static ngx_int_t ngx_http_fbs_handler(ngx_http_request_t *r);
static void ngx_http_fbs_post_read(ngx_http_request_t *r);


static void *ngx_http_fbs_create_conf(ngx_conf_t *cf);
static char *ngx_http_fbs_merge_conf(ngx_conf_t *cf, 
        void *parent, void *child);

static ngx_command_t  ngx_http_fbs_commands[] = {
    {
        ngx_string("fbs_schema"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
        ngx_http_fbs_schema,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    {
        ngx_string("set_fbs_from_json"),
        NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_TAKE12,
        ngx_http_set_fbs_conf_handler,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    
    ngx_null_command
};


static ngx_http_module_t  ngx_http_fbs_module_ctx = {
    NULL,                                    /* preconfiguration */
    ngx_http_fbs_init,                       /* postconfiguration */
    
    NULL,                                    /* create main configuration */
    NULL,                                    /* init main configuration */

    NULL,                                    /* create server configuration */
    NULL,                                    /* merge server configuration */
    
    ngx_http_fbs_create_conf,  /* create location configuration */
    ngx_http_fbs_merge_conf,   /* merge location configuration */
};

extern "C" {

ngx_module_t  ngx_http_fbs_module = {
    NGX_MODULE_V1,
    &ngx_http_fbs_module_ctx,  /* module context */
    ngx_http_fbs_commands,     /* module directives */
    NGX_HTTP_MODULE,                         /* module type */
    NULL,                                    /* init master */
    NULL,                                    /* init module */
    NULL,                                    /* init process */
    NULL,                                    /* init thread */
    NULL,                                    /* exit thread */
    NULL,                                    /* exit process */
    NULL,                                    /* exit master */
    NGX_MODULE_V1_PADDING
};

} // extern C


static ngx_int_t
ngx_http_set_fbs_from_json(ngx_http_request_t *r, 
        ngx_str_t *res, ngx_http_variable_value_t *v)
{
    ngx_http_fbs_ctx_t                  *ctx;
    
    ngx_http_fbs_conf_t      *conf;
    
    dd_enter();
    
    dd("set default return value");
    ngx_str_set(res, "");
    
    if (r->done)
    {
        dd("request done");
        return NGX_OK;
    }
    
    ctx = (ngx_http_fbs_ctx_t*)ngx_http_get_module_ctx(r, ngx_http_fbs_module);
    
    if (ctx == NULL)
    {
        dd("ndk handler:null ctx");
        return NGX_OK;
    }
    
    if (!ctx->done)
    {
        dd("ctx not done");
        return NGX_OK;
    }
    
    conf = (ngx_http_fbs_conf_t*)ngx_http_get_module_loc_conf(r, ngx_http_fbs_module);
    if (conf->schema.data == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "fbs: a schema is required to be defined by the fbs_schema directive");

        return NGX_ERROR;
    }
    
    return fbs_json_parse(r, res, v, conf);
}


static char *
ngx_http_fbs_schema(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_str_t                *value;
    ngx_http_fbs_conf_t      *llcf = (ngx_http_fbs_conf_t*)conf;
    
    if (llcf->schema.data != NULL)
        return (char *)"is duplicate";
    
    value = (ngx_str_t*)cf->args->elts;
    
    llcf->schema = value[1];
    
    ngx_http_fbs_used = 1;
    
    return fbs_schema_parse(cf->pool, llcf->schema);
}


static void *
ngx_http_fbs_create_conf(ngx_conf_t *cf)
{
    ngx_http_fbs_conf_t  *conf;
    
    conf = (ngx_http_fbs_conf_t*)ngx_palloc(cf->pool, sizeof(ngx_http_fbs_conf_t));
    if (conf == NULL)
        return NULL;
    
    return conf;
}


static char *
ngx_http_fbs_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_fbs_conf_t *prev = (ngx_http_fbs_conf_t*)parent;
    ngx_http_fbs_conf_t *conf = (ngx_http_fbs_conf_t*)child;
    
    //ngx_conf_merge_str_value(conf->schema, prev->schema, "");
    if (conf->schema.data == NULL && prev->schema.data != NULL)
        conf->schema = prev->schema;
    
    return NGX_CONF_OK;
}

static char *
ngx_http_set_fbs_conf_handler(ngx_conf_t *cf, 
        ngx_command_t *cmd, void *conf)
{
    ndk_set_var_t                            filter;
    ngx_str_t                               *value, s;
    u_char                                  *p;
    
#if defined(nginx_version) && nginx_version >= 8042 && nginx_version <= 8053
    return "does not work with " NGINX_VER;
#endif
    
    filter.type = NDK_SET_VAR_MULTI_VALUE;
    filter.size = 1;
    
    value = (ngx_str_t*)cf->args->elts;
    
    filter.func = (void *) ngx_http_set_fbs_from_json;
    
    value++;
    
    if (cf->args->nelts == 2)
    {
        p = value->data;
        p++;
        s.len = value->len - 1;
        s.data = p;
    }
    else if (cf->args->nelts == 3)
    {
        s.len = (value + 1)->len;
        s.data = (value + 1)->data;
    }
    
    return ndk_set_var_multi_value_core (cf, value,  &s, &filter);
}

/* register a new rewrite phase handler */
static ngx_int_t
ngx_http_fbs_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt             *h;
    ngx_http_core_main_conf_t       *cmcf;
    
    if (!ngx_http_fbs_used)
        return NGX_OK;
    
    cmcf = (ngx_http_core_main_conf_t*)ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    
    h = (ngx_http_handler_pt*)ngx_array_push(&cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers);
    
    if (h == NULL)
        return NGX_ERROR;
    
    *h = ngx_http_fbs_handler;
    
    return NGX_OK;
}


// a rewrite phase handler
static ngx_int_t
ngx_http_fbs_handler(ngx_http_request_t *r)
{
    ngx_http_fbs_ctx_t              *ctx;
    ngx_str_t                        value;
    ngx_int_t                        rc;
    
    dd_enter();
    
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "http fbs rewrite phase handler");
    
    ctx = (ngx_http_fbs_ctx_t*)ngx_http_get_module_ctx(r, ngx_http_fbs_module);
    
    if (ctx != NULL)
    {
        if (ctx->done)
        {
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                    "http fbs rewrite phase handler done");
            
            return NGX_DECLINED;
        }
        
        return NGX_DONE;
    }
    
    if (r->method != NGX_HTTP_POST && r->method != NGX_HTTP_PUT)
        return NGX_DECLINED;
    
    if (r->headers_in.content_type == NULL || r->headers_in.content_type->value.data == NULL)
    {
        dd("content_type is %s", r->headers_in.content_type == NULL ? "NULL": "NOT NULL");
        
        return NGX_DECLINED;
    }
    
    value = r->headers_in.content_type->value;
    
    dd("r->headers_in.content_length_n:%d", (int) r->headers_in.content_length_n);
    
    // just focus on application/json
    if (value.len < json_type_len || 
            ngx_strncasecmp(value.data, (u_char *) json_type, json_type_len) != 0)
    {
        dd("not application/json");
        return NGX_DECLINED;
    }
    
    dd("content type is application/json");
    
    dd("create new ctx");
    
    ctx = (ngx_http_fbs_ctx_t*)ngx_pcalloc(r->pool, sizeof(ngx_http_fbs_ctx_t));
    if (ctx == NULL)
        return NGX_ERROR;
    
    /**
     * set by ngx_pcalloc:
     *      ctx->done = 0;
     *      ctx->waiting_more_body = 0;
     */
    ngx_http_set_ctx(r, ctx, ngx_http_fbs_module);
    
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "http fbs start to read client request body");
    
    rc = ngx_http_read_client_request_body(r, ngx_http_fbs_post_read);
    
    if (rc == NGX_ERROR || rc >= NGX_HTTP_SPECIAL_RESPONSE)
    {
        
#if (nginx_version < 1002006) || (nginx_version >= 1003000 && nginx_version < 1003009)
        r->main->count--;
#endif
        
        return rc;
    }
    
    if (rc == NGX_AGAIN)
    {
        ctx->waiting_more_body = 1;
        
        return NGX_DONE;
    }
    
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "http fbs has read the request body in one run");
    
    return NGX_DECLINED;
}


static void
ngx_http_fbs_post_read(ngx_http_request_t *r)
{
    ngx_http_fbs_ctx_t     *ctx;
    
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "http fbs post read request body");
    
    ctx = (ngx_http_fbs_ctx_t*)ngx_http_get_module_ctx(r, ngx_http_fbs_module);
    
    ctx->done = 1;
    
#if defined(nginx_version) && nginx_version >= 8011
    dd("count--");
    r->main->count--;
#endif
    
    dd("waiting more body: %d", (int) ctx->waiting_more_body);
    
    // waiting_more_body my rewrite phase handler
    if (ctx->waiting_more_body)
    {
        ctx->waiting_more_body = 0;
        
        ngx_http_core_run_phases(r);
    }
}
