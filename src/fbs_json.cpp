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

} // extern C

#include "fbs_json.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"


static flatbuffers::Parser parser;

char *
fbs_schema_parse(ngx_pool_t *pool, ngx_str_t schema)
{
    u_char last_char;
    u_char* last;
    
    last = schema.data + schema.len;
    last_char = *last;
    *last = '\0';
    
    // clear structs
    parser.structs_.vec.clear();
    parser.structs_.dict.clear();
    // clear enums
    parser.enums_.vec.clear();
    parser.enums_.dict.clear();
    
    if (!parser.Parse((char*)schema.data, "schema/"))
    {
        *last = last_char;
        //ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, jsongen.c_str());
        return (char*)parser.error_.c_str();
    }
    
    *last = last_char;
    
    return NGX_CONF_OK;
}

static ngx_int_t
get_body(ngx_http_request_t *r, 
        ngx_str_t *value, u_char** buf_out, size_t* len_out)
{
    u_char              *p, *last, *buf;
    ngx_chain_t         *cl;
    size_t               len = 0;
    //ngx_array_t         *array = NULL;
    //ngx_str_t           *s;
    ngx_buf_t           *b;
    
    // set default
    ngx_str_set(value, "");
    
    // we read data from r->request_body->bufs
    if (r->request_body == NULL || r->request_body->bufs == NULL)
    {
        dd("empty rb or empty rb bufs");
        return NGX_OK;
    }
    
    if (r->request_body->bufs->next != NULL)
    {
        // more than one buffer...we should copy the data out...
        len = 0;
        for (cl = r->request_body->bufs; cl; cl = cl->next)
        {
            b = cl->buf;
            
            if (b->in_file)
            {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                        "fbs: in-file buffer found. aborted. "
                        "consider increasing your client_body_buffer_size "
                        "setting");
                
                return NGX_OK;
            }
            
            len += b->last - b->pos;
        }
        
        dd("len=%d", (int) len);
        
        if (len == 0)
            return NGX_OK;
        
        buf = (u_char*)ngx_palloc(r->pool, len);
        if (buf == NULL)
            return NGX_ERROR;
        
        *buf_out = buf;
        *len_out = len;
        
        p = buf;
        last = p + len;
        
        for (cl = r->request_body->bufs; cl; cl = cl->next)
            p = ngx_copy(p, cl->buf->pos, cl->buf->last - cl->buf->pos);
        
        //dd("p - buf = %d, last - buf = %d", (int) (p - buf),
        //   (int) (last - buf));
        
        //dd("copied buf (len %d): %.*s", (int) len, (int) len,
        //   buf);

    }
    else
    {
        dd("XXX one buffer only");
        
        b = r->request_body->bufs->buf;
        if (ngx_buf_size(b) == 0)
            return NGX_OK;
        
        buf = b->pos;
        last = b->last;
        
        *buf_out = buf;
        *len_out = last - buf;
    }
    
    return NGX_OK;
}

ngx_int_t
fbs_json_parse(ngx_http_request_t *r, 
        ngx_str_t *res, ngx_http_variable_value_t *v, ngx_http_fbs_conf_t *conf)
{
    ngx_int_t rc;
    u_char last_char;
    u_char* last;
    u_char* buf = nullptr;
    size_t len = 0;
    
    if (!parser.SetRootType((const char*)v->data))
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Unknown root_type: %s", v->data);
        return NGX_OK;
    }
    
    if (parser.root_struct_def->fixed)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Non-table root_type: %s", v->data);
        return NGX_OK;
    }
    
    rc = get_body(r, res, &buf, &len);
    if (rc != NGX_OK)
        return rc;
    
    if (len == 0 || *buf != '{')
    {
        //ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "empty body");
        return NGX_OK;
    }
    
    /* ================================================== */
    
    // temporary null delim
    last = buf + len;
    last_char = *last;
    *last = '\0';
    
    if (parser.ParseJson((char*)buf))
    {
        // restore
        *last = last_char;
        
        // TODO allocate ngx_palloc and copy?
        res->data = parser.builder_.GetBufferPointer();
        res->len = parser.builder_.GetSize();
        //ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "success: %d %s %s", len, v->data, buf);
    }
    else
    {
        // restore
        *last = last_char;
        
        res->data = nullptr;
        res->len = 0;
        //ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "fail: %d %s %s %s", len, v->data, parser.error_.c_str(), buf);
    }
    
    return NGX_OK;
}
