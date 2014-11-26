#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <sys/types.h>

/* Module config */
typedef struct {
	ngx_str_t msg;	
} ngx_http_autohome_hello_loc_conf_t;

static char *ngx_http_autohome_hello(ngx_conf_t *cf, ngx_command_t *cmd, void *conf); 
static void *ngx_http_autohome_hello_create_loc_conf(ngx_conf_t *cf); 
static char *ngx_http_autohome_hello_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child); 
static void ngx_str_append(ngx_str_t *src, const ngx_str_t *append, const ngx_str_t *delimiter, ngx_pool_t *pool, ngx_log_t *log);

/* Directives */
static ngx_command_t ngx_http_autohome_hello_commands[] = {
	{ 	ngx_string("autohome_hello"),       
		NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS|NGX_CONF_TAKE1,       
		ngx_http_autohome_hello,       
		NGX_HTTP_LOC_CONF_OFFSET,       
		offsetof(ngx_http_autohome_hello_loc_conf_t, msg),       
		NULL },         
		
		ngx_null_command 
}; 

/* Http context of the module */
static ngx_http_module_t ngx_http_autohome_hello_module_ctx = {     
	NULL,                                  /* preconfiguration */    
	NULL,                                  /* postconfiguration */      
	
	NULL,                                  /* create main configuration */    
	NULL,                                  /* init main configuration */      
	
	NULL,                                  /* create server configuration */    
	NULL,                                  /* merge server configuration */      
	
	ngx_http_autohome_hello_create_loc_conf,         /* create location configration */    
	ngx_http_autohome_hello_merge_loc_conf           /* merge location configration */
}; 


/* Module */
ngx_module_t ngx_http_autohome_hello_module = {     
	NGX_MODULE_V1,     
	&ngx_http_autohome_hello_module_ctx,             /* module context */    
	ngx_http_autohome_hello_commands,                /* module directives */    
	NGX_HTTP_MODULE,                       /* module type */    
	NULL,                                  /* init master */    
	NULL,                                  /* init module */    
	NULL,                                  /* init process */    
	NULL,                                  /* init thread */    
	NULL,                                  /* exit thread */    
	NULL,                                  /* exit process */    
	NULL,                                  /* exit master */    
	NGX_MODULE_V1_PADDING
};

static void ngx_str_append(ngx_str_t *src, const ngx_str_t *append, const ngx_str_t *delimiter, ngx_pool_t *pool, ngx_log_t *log){
	size_t			size;
	u_char			*data;
	size_t			pos;
	ngx_int_t		using_delimiter;

	size = 0;
	if(src == NULL || src->data == NULL || src->len == 0){
		return;
	}
	size+=src->len;

	if(append == NULL || append->data == NULL || append->len == 0){
		return;
	}
	size+=append->len;

	using_delimiter = 1;
	if(delimiter == NULL || delimiter->data == NULL || delimiter->len == 0){
		using_delimiter = 0;
	}

	if(using_delimiter == 1){
		size+=delimiter->len;
	}

	if(pool){
		data = ngx_pcalloc(pool, size);
	}else{
		data = ngx_calloc(size, log);
	}

	if(data == NULL)
	{
		//memory alloc failed, direct return
		return;
	}

	//copy content
	pos=0;
	ngx_cpystrn(data+pos, src->data, src->len);
	pos+=src->len;
	if(using_delimiter == 1){
		//#ngx_cpystrn
		ngx_cpystrn(data+pos, delimiter->data, delimiter->len);
		pos+=delimiter->len;
	}
	ngx_cpystrn(data+pos, append->data, append->len);

	src->data = data;
	src->len = size;
}

/* Handler function */
static ngx_int_t ngx_http_autohome_hello_handler(ngx_http_request_t *r) {     
	ngx_int_t rc;     
	ngx_buf_t *b;     
	ngx_chain_t out;
	
	ngx_int_t content_len;
	ngx_flag_t has_conf;
	ngx_str_t final;
	ngx_str_t delimiter;
	char *delimiter_conf;	
	
	ngx_http_autohome_hello_loc_conf_t *elcf;     
	elcf = ngx_http_get_module_loc_conf(r, ngx_http_autohome_hello_module);       
	
	if(!(r->method & (NGX_HTTP_HEAD|NGX_HTTP_GET|NGX_HTTP_POST))){
		return NGX_HTTP_NOT_ALLOWED;
	}
	
	r->headers_out.content_type.len = sizeof("text/html") - 1;
	r->headers_out.content_type.data = (u_char *) "text/html";
		
	delimiter_conf = ": ";
	if(elcf->msg.len == 0){
		content_len = r->unparsed_uri.len;
		has_conf=0;
	}else{
		content_len = elcf->msg.len + r->unparsed_uri.len + ngx_strlen((u_char *)delimiter_conf);
		has_conf=1;
	}	
	
	r->headers_out.status = 200;
	r->headers_out.content_length_n = content_len;
	
	if(r->method == NGX_HTTP_HEAD){         
		rc = ngx_http_send_header(r);
		if(rc != NGX_OK) {
			return rc;
		}
	}

	b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));     
	if(b == NULL) {         
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Failed to allocate response buffer.");
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	out.buf = b;
	out.next = NULL;	
	
	if(has_conf){
		final.len = elcf->msg.len;
		final.data = elcf->msg.data;		
		delimiter.len = ngx_strlen((u_char *)delimiter_conf);
		delimiter.data = (u_char *)delimiter_conf;		
		ngx_str_append(&final, &r->unparsed_uri, &delimiter, r->pool, r->pool->log);
	}else{
		final.len = r->unparsed_uri.len;
		final.data = r->unparsed_uri.data;
	}		
	
    b->pos = final.data;
	b->last = final.data + final.len;
	b->memory = 1;
	b->last_buf = 1;
	rc = ngx_http_send_header(r);
	if(rc != NGX_OK){
		return rc;
	}
	
	return ngx_http_output_filter(r, &out);
}

static char *ngx_http_autohome_hello(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {     
	ngx_http_core_loc_conf_t  *clcf;     
	clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);     
	clcf->handler = ngx_http_autohome_hello_handler;     
	ngx_conf_set_str_slot(cf,cmd,conf);       
	return NGX_CONF_OK; 
}

static void *ngx_http_autohome_hello_create_loc_conf(ngx_conf_t *cf) {     
	ngx_http_autohome_hello_loc_conf_t  *conf;       
	conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_autohome_hello_loc_conf_t));     
	if (conf == NULL) {         
		return NGX_CONF_ERROR;     
	}     
	conf->msg.len = 0;     
	conf->msg.data = NULL;       
	return conf; 
}   

static char *ngx_http_autohome_hello_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child) {     
	ngx_http_autohome_hello_loc_conf_t *prev = parent;     
	ngx_http_autohome_hello_loc_conf_t *conf = child;       
	ngx_conf_merge_str_value(conf->msg, prev->msg, "");       
	return NGX_CONF_OK; 
} 
