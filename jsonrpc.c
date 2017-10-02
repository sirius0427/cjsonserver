/*
 * jsonrpc-c.c
 *
 *  Created on: Oct 11, 2012
 *      Author: hmng
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "jsonrpc.h"
#include "LOGS.h"

struct ev_loop *loop;

//static char* (*jrpc_json_print)(cJSON *item) = cJSON_Print;
static char* (*jrpc_json_print)(cJSON *item) = cJSON_PrintUnformatted;



const static char jrpc_http_header[] = "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=UTF-8\r\nAccept: application/json\t\n";

static char* jrpc_build_httpResponse(char* response)
{
	int header_len = strlen(jrpc_http_header);
	int reponse_len = strlen(response);
	char* tosend = (char*)malloc(header_len+25+reponse_len);
	sprintf(tosend,"%s",jrpc_http_header);
	sprintf(tosend+header_len,"Content-Length: %d\r\n\r\n%s",reponse_len, response);
	return tosend;
}

static int jrpc_http_parse(jrpc_parser_t* parser, char* text, int http_length)
{
	int len;
	long lenstart,lenend;
	char* pstart,*pend;
	if(!strncmp(text,"GET",3))
	{
		sprintf(parser->method,"GET");
		//pstart = strstr(text+4,"Host:");
		//pend = strstr(pstart,"\r\n");
		//snprintf(parser->host,pend-pstart,pstart);
		//pstart = strstr(text+4,"Content-Length:");
		//pend = strstr(pstart,"\r\n");
		//snprintf(parser->length,pend-pstart-16,pstart+16);
		return 0;
	}else if(!strncmp(text,"POST",4))
	{
		DEBUGLOGSG( "jrpc_http_parse POST" );
		//method
		sprintf(parser->method,"POST");
		
		//url
		pstart = strstr(text+5,"//");
		if( pstart == NULL )
			pstart = text;
		pstart = strstr(pstart,"/");
		pend = strstr(pstart," ");
		snprintf(parser->url,pend-pstart+1,pstart);

		//host
		pstart = strstr(text+5,"Host:");
		pend = strstr(pstart,"\r\n");
		snprintf(parser->host,pend-pstart+1,pstart);
		
		//Content-Length
		pstart = strstr(text+5,"Content-Length:");
		pend = strstr(pstart,"\r\n");
		snprintf(parser->length,pend-pstart-16+1,pstart+16);
		
		//Content
		pstart = strstr(text+5,"{");
		//pend = strstr(pstart, "\"method\":\"datainsert\"}" );
		//len = atoi(parser->length)+10;
		//len = strlen( text ) - indexOf( text+5,"{");
		//len = indexOf(text+5, "\"method\":\"datainsert\"}" ) - indexOf( text+5,"{") + 23;
		lenend = indexOf(text+5, "}\r" );
		lenstart = indexOf( text+5,"{");
		DEBUGLOGSG( "lenend[%ld]lenstart[%ld]", lenend, lenstart );
		if( lenend >= http_length || lenend == -1)
		{
			ERRORLOGSG( "找不到结束符，退出[%ld][%d]",indexOf(text+5, "}\r" ), http_length );
			printf( "--------text begin-------\n%s\n---------text end----------\n",text );
			return -1;
		}
		if( lenstart >= http_length || lenstart == -1)
		{
			ERRORLOGSG( "找不到开始符，退出[%ld][%d]",indexOf(text+5, "{" ), http_length );
			printf( "--------text begin-------\n%s\n---------text end----------\n",text );
			return -1;
		}
		
		len = lenend - lenstart + 2;
		if( len <= 0 )
		{
			ERRORLOGSG( "取得的长度错误，退出[%d]", len );
			printf( "--------text begin-------\n%s\n---------text end----------\n",text );
			return -1;
		}

		//printf("Content-Length=[%ld][%d][%d]\n", len, indexOf(text+5, "\"method\":\"datainsert\"}" )+23 , indexOf( text+5,"{") );
		DEBUGLOGSG("Content-Length=[%ld][%ld][%ld]", len, indexOf(text+5, "}\r" ) , indexOf( text+5,"{") );
		//printf("%s\n", text );
		parser->content = (char*)malloc(len+1);	
		DEBUGLOGSG( "malloc ok" );
		//pstart = strstr(text+5,"\r\n\r\n");
		snprintf(parser->content,len+1,pstart);

		while(1)
		{
			pstart = strstr( parser->content, "\r\n" );
			if( pstart == NULL )
			{
				break;
			}
			pend = strstr( pstart+2, "\r\n" );
			if( pend == NULL )
			{
				break;
			}
			snprintf(pstart, strlen(parser->content)-(pend-pstart)-2, pend+2 ); 
		}


		printf( "%s\n", parser->content );
	}else
	{
		sprintf(parser->method,"UNKNOW");
	}

	return 0;
}

static int jrpc_http_check_intact(char* text)
{
	long len;
	char* pstart,*pend;
	char length[16]={0};
	if(!strncmp(text,"GET",3))
	{
		DEBUGLOGSG( "http方法为GET" );
		pend = strstr(text+4,"\r\n\r\n");
		if(!pend)
		{
			return -1;
		}
		return pend+4-text;
	}else if(!strncmp(text,"POST",4))
	{
		DEBUGLOGSG( "http方法为POST" );
		pstart = strstr(text+5,"Content-Length:");
		if(!pstart)
		{
			ERRORLOGSG( "取不到Content-Length长度" );
			return -1;
		}
		pend = strstr(pstart,"\r\n");
		if(!pend)
		{
			ERRORLOGSG( "取不到结束符" );
			return -1;
		}
		snprintf(length,pend-pstart-16+1,pstart+16);
		len = atol(length);
		DEBUGLOGSG( "Content-Length:%ld", len );
		return len;
		//printf( "text=%s\n", text );
		
		/*pstart = strstr(text+5,"\r\n\r\n");
		DEBUGLOGSG( "pstart位置为[%d]", strlen(pstart));
		if(strlen(pstart+4) < len)
		{
			ERRORLOGSG( "取pstart错[%d][%d]",strlen(pstart+4), len );
			return -1;
		}else
		{
			printf("\r\nContent-Length:%d\r\n",len);
			printf("\r\npstart-text=%d\r\n",pstart-text);
			return pstart+4+len-text;
		}
		*/
	}else
	{
		DEBUGLOGSG( "http方法为空" );
		return -1;
	}
}

// get sockaddr, IPv4 or IPv6:
static void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*) sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

static int send_response(struct jrpc_connection * conn, char *response) {
	int fd = conn->fd;
	if (conn->debug_level > 1)
		printf("JSON Response--->:\n%s\n", response);
	write(fd, response, strlen(response));
	write(fd, "\n", 1);
	return 0;
}

static int send_error(struct jrpc_connection * conn, int code, char* message, cJSON * id, int isHttp) 
{
	int return_value = 0;
	char *tmp;
	cJSON *result_root = cJSON_CreateObject();
	cJSON *error_root = cJSON_CreateObject();
	cJSON_AddNumberToObject(error_root, "code", code);
	cJSON_AddStringToObject(error_root, "message", message);
	cJSON_AddStringToObject(result_root, "jsonrpc", "2.0");		//added by wangzugang
	cJSON_AddItemToObject(result_root, "error", error_root);
	cJSON_AddItemToObject(result_root, "id", id);
	char * str_result = jrpc_json_print(result_root);
	if(isHttp)
	{
		tmp = jrpc_build_httpResponse(str_result);
		free(str_result);
		str_result = tmp;
	}
	return_value = send_response(conn, str_result);
	free(str_result);
	cJSON_Delete(result_root);
	free(message);
	return return_value;
}


static int send_result(struct jrpc_connection * conn, cJSON * result, cJSON * id,int isHttp) 
{
	int return_value = 0;
	char *tmp;
	cJSON *result_root = cJSON_CreateObject();
	cJSON_AddStringToObject(result_root, "jsonrpc", "2.0");		//added by wangzugang
	if (result)
		cJSON_AddItemToObject(result_root, "result", result);
	cJSON_AddItemToObject(result_root, "id", id);

	char * str_result = jrpc_json_print(result_root);
	if(isHttp)
	{
		tmp = jrpc_build_httpResponse(str_result);
		free(str_result);
		str_result = tmp;
	}
	
	return_value = send_response(conn, str_result);
	free(str_result);
	cJSON_Delete(result_root);
	return return_value;
}


//added by wangzugang, this is added for jsonrpc-c-2.0
static int send_batch(struct jrpc_connection *conn, int count, jrpc_context *ctx, cJSON **returned, int *procedure_found, cJSON **id, int isHttp) 
{
	int i,return_value = 0;
	char* tmp;
	cJSON *array = cJSON_CreateArray();
	cJSON *result_root = NULL;
	cJSON *error_root = NULL;
	for(i = 0; i < count; i++)
	{
		if (!procedure_found[i] || (procedure_found[i] && ctx[i].error_code))
		{
			//return send_error(conn, JRPC_METHOD_NOT_FOUND, strdup("Method not found."), id[i]);
			//return send_error(conn, ctx[i].error_code, ctx[i].error_message, id[i]);
			result_root = cJSON_CreateObject();
			error_root = cJSON_CreateObject();
			if(procedure_found[i])
			{
				cJSON_AddNumberToObject(error_root, "code", ctx[i].error_code);
				cJSON_AddStringToObject(error_root, "message", ctx[i].error_message);
			}else
			{
				cJSON_AddNumberToObject(error_root, "code", JRPC_METHOD_NOT_FOUND);
				cJSON_AddStringToObject(error_root, "message", strdup("Method not found."));
			}
			cJSON_AddStringToObject(result_root, "jsonrpc", "2.0");
			cJSON_AddItemToObject(result_root, "error", error_root);
			cJSON_AddItemToObject(result_root, "id", id[i]);
			cJSON_AddItemToArray(array,result_root);
		}
		else 
		{
			//return send_result(conn, returned[i], id[i]);
			result_root = cJSON_CreateObject();
			cJSON_AddStringToObject(result_root, "jsonrpc", "2.0");		//added by wangzugang
			if (returned[i])
			{
				cJSON_AddItemToObject(result_root, "result", returned[i]);
			}
			cJSON_AddItemToObject(result_root, "id", id[i]);
			cJSON_AddItemToArray(array,result_root);
		}
	}
	
	char * str_result = jrpc_json_print(array);
	if(isHttp)
	{
		tmp = jrpc_build_httpResponse(str_result);
		free(str_result);
		str_result = tmp;
	}
	
	return_value = send_response(conn, str_result);
	free(str_result);
	cJSON_Delete(array);	
	free(ctx);
	free(returned);
	free(procedure_found);
	free(id);
	return return_value;
}

static int invoke_procedure(struct jrpc_server *server, struct jrpc_connection * conn, char *name, cJSON *params, cJSON *id, int isHttp) 
{
	cJSON *returned = NULL;
	int procedure_found = 0;
	jrpc_context ctx;
	ctx.error_code = 0;
	ctx.error_message = NULL;
	int i = server->procedure_count;
	while (i--) {
		if (!strcmp(server->procedures[i].name, name)) {
			procedure_found = 1;
			ctx.data = server->procedures[i].data;
			returned = server->procedures[i].function(&ctx, params, id);
			break;
		}
	}
	if (!procedure_found)
		return send_error(conn, JRPC_METHOD_NOT_FOUND, strdup("Method not found."), id, isHttp);
	else {
		if (ctx.error_code)
			return send_error(conn, ctx.error_code, ctx.error_message, id, isHttp);
		else
			return send_result(conn, returned, id, isHttp);
	}
}

//added by wangzugang, this is added for jsonrpc-c-2.0
static int invoke_procedure_batch(struct jrpc_server *server, struct jrpc_connection * conn, int count, cJSON ** method, cJSON **params, cJSON **id, int isHttp) 
{
	cJSON **returned = NULL;
	int *procedure_found = NULL;
	jrpc_context *ctx = NULL;
	int i, j; 
	
	returned = (cJSON**)malloc(sizeof(cJSON*)*count);
	procedure_found = (int*)malloc(sizeof(int)*count);
	ctx = (jrpc_context*)malloc(sizeof(jrpc_context)*count);
	
	for(i = 0; i < count; i++)
	{
		returned[i] = NULL;
		procedure_found[i] = 0;	
		ctx[i].error_code = 0;
		ctx[i].error_message = NULL;
	}
	
	i = count;
	while(i--)
	{
		j = server->procedure_count; 
		while (j--) 
		{
			if (!strcmp(server->procedures[j].name, method[i]->valuestring)) 
			{
				procedure_found[i] = 1;
				ctx[i].data = server->procedures[j].data;
				returned[i] = server->procedures[j].function(&ctx[i], params[i], id[i]);
				break;
			}
		}
	}
	
	free(method);
	free(params);
	return send_batch(conn, count, ctx, returned, procedure_found, id,isHttp);
}

static int eval_request(struct jrpc_server *server, struct jrpc_connection * conn, cJSON *root) 
{
	cJSON *method, *params, *id;
	//We have to copy ID because using it on the reply and deleting the response Object will also delete ID
	cJSON * id_copy = NULL;
	method = cJSON_GetObjectItem(root, "method");
	if (method != NULL && method->type == cJSON_String && !strcmp( method->valuestring, "datainsert" )) {
		params = cJSON_GetObjectItem(root, "params");
		if (params == NULL|| params->type == cJSON_Array || params->type == cJSON_Object) {
			id = cJSON_GetObjectItem(root, "id");
			if (id == NULL|| id->type == cJSON_String || id->type == cJSON_Number) {
				if (id != NULL)
					id_copy = (id->type == cJSON_String) ? cJSON_CreateString( id->valuestring) : cJSON_CreateNumber(id->valueint);
				if (server->debug_level)
				{
					printf("Method Invoked: %s\n", method->valuestring);
					printf("message Invoked: %s\n", params->string);
				}
				return invoke_procedure(server, conn, method->valuestring, params, id_copy, server->type);
			}
		}
	}
	else if (method != NULL && method->type == cJSON_String && !strcmp( method->valuestring, "contactinsert" )) {
		params = cJSON_GetObjectItem(root, "params");
		if (params == NULL|| params->type == cJSON_Array || params->type == cJSON_Object) {
				return invoke_procedure(server, conn, method->valuestring, params, id_copy, server->type);
		}
	}
	send_error(conn, JRPC_INVALID_REQUEST, strdup("The JSON sent is not a valid Request object."), NULL, server->type);
	return -1;
}

//added by wangzugang, this is added for jsonrpc-c-2.0
static int eval_request_batch(struct jrpc_server *server, struct jrpc_connection * conn, cJSON *root) 
{
	cJSON *object, **method, **params,**idcopy, *id;
	int i,count;
	count = cJSON_GetArraySize(root);
	printf("cJSON_GetArraySize=%d\n",count);
	method = (cJSON**)malloc(sizeof(cJSON*)*count);
	params = (cJSON**)malloc(sizeof(cJSON*)*count);
	idcopy = (cJSON**)malloc(sizeof(cJSON*)*count);
	for(i = 0; i < count; i++)
	{
		object = cJSON_GetArrayItem(root,i);
		if (object->type == cJSON_Object) 
		{
			//eval_request(server, conn, object);
			method[i] = cJSON_GetObjectItem(object, "method");
			if (method[i] != NULL && method[i]->type == cJSON_String) 
			{
				params[i] = cJSON_GetObjectItem(object, "messages");
				if (params[i] == NULL || params[i]->type == cJSON_Array || params[i]->type == cJSON_Object) 
				{
					id = cJSON_GetObjectItem(object, "id");
					if (id == NULL|| id->type == cJSON_String || id->type == cJSON_Number) 
					{
						//We have to copy ID because using it on the reply and deleting the response Object will also delete ID
						idcopy[i] = NULL;
						if (id != NULL)
						{
							idcopy[i] = (id->type == cJSON_String) ? cJSON_CreateString(id->valuestring) : cJSON_CreateNumber(id->valueint);
						}
						if (server->debug_level)
						{
							printf("Method Invoked: %s\n", method[i]->valuestring);
						}
					}else
					{				
						return -1;
					}
				}else
				{				
					return -1;
				}
			}else
			{				
				return -1;
			}
		}else
		{				
			return -1;
		}
	}
	return invoke_procedure_batch(server, conn, count, method, params, idcopy, server->type);
}

static void close_connection(struct ev_loop *loop, ev_io *w) {
	ev_io_stop(loop, w);
	close(((struct jrpc_connection *) w)->fd);
	free(((struct jrpc_connection *) w)->buffer);
	free(((struct jrpc_connection *) w));
}

static void connection_cb(struct ev_loop *loop, ev_io *w, int revents) 
{
	struct jrpc_connection *conn;
	struct jrpc_server *server = (struct jrpc_server *) w->data;
	jrpc_parser_t parser;
	size_t bytes_read = 0;
	size_t http_length = 0;
	char *pstart, *pend;
	char length[16]={0};
	//get our 'subclassed' event watcher
	conn = (struct jrpc_connection *) w;
	int fd = conn->fd;
	//conn->buffer_size += 10;
	if (conn->pos == (conn->buffer_size - 1)) {
		char * new_buffer = realloc(conn->buffer, conn->buffer_size *= 2);
		if (new_buffer == NULL) {
			perror("Memory error");
			return close_connection(loop, w);
		}
		conn->buffer = new_buffer;
		memset(conn->buffer + conn->pos, 0, conn->buffer_size - conn->pos);
	}
	// can not fill the entire buffer, string must be NULL terminated
	int max_read_size = conn->buffer_size - conn->pos - 1 ;
	DEBUGLOGSG( "max_read_size[%d][%ld][%d]", max_read_size , conn->buffer_size, conn->pos );
	if ((bytes_read = read(fd, conn->buffer + conn->pos, 65535)) == -1) {
		perror("read");
		return close_connection(loop, w);
	}
	if (!bytes_read) {
		// client closed the sending half of the connection
		if (server->debug_level)
			printf("Client closed connection.\n");
		return close_connection(loop, w);
	} 
	pstart = strstr(conn->buffer+5,"Content-Length:");
	if(!pstart)
	{
		ERRORLOGSG( "取不到Content-Length长度" );
		return close_connection(loop, w);
	}
	pend = strstr(pstart,"\r\n");
	if(!pend)
	{
		ERRORLOGSG( "取不到结束符" );
		return close_connection(loop, w);
	}
	snprintf(length,pend-pstart-16+1,pstart+16);
	http_length = atol(length);
	DEBUGLOGSG( "http_length = [%ld]", http_length );

	conn->pos = strlen( conn->buffer );
	if( conn->pos <= http_length )
	{
		while((bytes_read = read(fd, conn->buffer + conn->pos, 65535 )) )
		{
			//DEBUGLOGSG( "conn->pos[%d]", conn->pos );
			conn->pos = strlen( conn->buffer );
			if( conn->pos >= http_length )
				break;
		}
		DEBUGLOGSG( "接收完成conn->pos[%d]", conn->pos );
	}
	//printf( "connbuffer=%s\n", conn->buffer );
	//printf( "connbuffer print end\n" );

	if (!bytes_read) {
		// client closed the sending half of the connection
		if (server->debug_level)
			printf("Client closed connection.\n");
		return close_connection(loop, w);
	} else {
		cJSON *root;
		char *end_ptr;
		conn->pos += bytes_read;

		//解析http服务
		if(server->type == JRPC_TYPE_HTTP)
		{
			if((http_length = jrpc_http_check_intact(conn->buffer)) < 0)
			{
				return;
			}
			printf( "-------------------------------------------http_length[%ld]-----------------------------------------------------------\n", http_length);
			if(jrpc_http_parse(&parser,conn->buffer,conn->pos) < 0 )
			{
				return close_connection(loop, w);
			}
			DEBUGLOGSG( "jrpc_http_parse ok" );
			//printf("\r\n-----------------------------\r\n%s\r\n----------------------------------,http_length=%ld\r\n",conn->buffer,http_length);
			memmove(conn->buffer, &conn->buffer[http_length], strlen(&conn->buffer[http_length])+1);
			DEBUGLOGSG( "memmove ok" );
			conn->pos = strlen(&conn->buffer[http_length]);
			memset(conn->buffer + conn->pos, 0, conn->buffer_size - conn->pos - 1);
			replace( conn->buffer, "\r\n", "" );
			//printf("\r\n---------------json-content--------------\r\n%s\r\n----------------------------------\r\n",parser.content);
			if ((root = cJSON_Parse_Stream(parser.content, &end_ptr)) != NULL) {
				if (server->debug_level > 1) {
					char * str_result = jrpc_json_print(root);
					printf("Valid JSON Received<---:\n%s\n", str_result);
					free(str_result);
				}
				DEBUGLOGSG( "printf str_result" );

				if (root->type == cJSON_Object) {
					DEBUGLOGSG( "root is cJSON_Object, begin eval_request" );
					eval_request(server, conn, root);
				}
				//added by wangzugang
				DEBUGLOGSG("root->type=%d",root->type);
				if (root->type == cJSON_Array) {
					DEBUGLOGSG( "root is cJSON_Array, begin eval_request_batch" );
					eval_request_batch(server, conn, root);
				}
				DEBUGLOGSG("eval_request done" );
				free(parser.content);
				parser.content = NULL;

				cJSON_Delete(root);
			} else {
				if (cJSON_GetErrorPtr() != (parser.content+atoi(parser.length))) 
				{
					if (server->debug_level) 
					{
						//printf("INVALID JSON Received<---:\n---\n%s\n---\n", parser.content);
					}
					send_error(conn, JRPC_PARSE_ERROR, strdup("Http Parse error. Invalid JSON was received by the server."), NULL, server->type);
					return close_connection(loop, w);
				}
			}
		}
		else
		{
			if ((root = cJSON_Parse_Stream(conn->buffer, &end_ptr)) != NULL) 
			{
				if (server->debug_level > 1) {
					char * str_result = jrpc_json_print(root);
					printf("Valid JSON Received<---:\n%s\n", str_result);
					free(str_result);
				}

				if (root->type == cJSON_Object) {
					eval_request(server, conn, root);
				}
				//added by wangzugang
				//printf("root->type=%d\n",root->type);
				if (root->type == cJSON_Array) {
					eval_request_batch(server, conn, root);
				}
				//added end
				//shift processed request, discarding it
				memmove(conn->buffer, end_ptr, strlen(end_ptr) + 2);

				conn->pos = strlen(end_ptr);
				memset(conn->buffer + conn->pos, 0, conn->buffer_size - conn->pos - 1);

				cJSON_Delete(root);
			} else {
				// did we parse the all buffer? If so, just wait for more.
				// else there was an error before the buffer's end
				if (cJSON_GetErrorPtr() != (conn->buffer + conn->pos)) 
				{
					if (server->debug_level) 
					{
						printf("INVALID JSON Received<---:\n---\n%s\n---\n", conn->buffer);
					}
					send_error(conn, JRPC_PARSE_ERROR, strdup("No Http Parse error. Invalid JSON was received by the server."), NULL, server->type);
					return close_connection(loop, w);
				}
			}
		}
	}

}

static void accept_cb(struct ev_loop *loop, ev_io *w, int revents) {
	char s[INET6_ADDRSTRLEN];
	struct jrpc_connection *connection_watcher;
	connection_watcher = malloc(sizeof(struct jrpc_connection));
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	sin_size = sizeof their_addr;
	connection_watcher->fd = accept(w->fd, (struct sockaddr *) &their_addr, &sin_size);
	if (connection_watcher->fd == -1) {
		perror("accept");
		free(connection_watcher);
	} else {
		if (((struct jrpc_server *) w->data)->debug_level) {
			inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *) &their_addr), s, sizeof s);
			DEBUGLOGSG("server: got connection from %s", s);
		}
		DEBUGLOGSG( "开始接收数据" );
		ev_io_init(&connection_watcher->io, connection_cb, connection_watcher->fd, EV_READ);
		//copy pointer to struct jrpc_server
		connection_watcher->io.data = w->data;
		connection_watcher->buffer_size = JRPC_BUFFER_SIZE;
		connection_watcher->buffer = malloc(JRPC_BUFFER_SIZE);
		memset(connection_watcher->buffer, 0, JRPC_BUFFER_SIZE);
		connection_watcher->pos = 0;
		//copy debug_level, struct jrpc_connection has no pointer to struct jrpc_server
		connection_watcher->debug_level = ((struct jrpc_server *) w->data)->debug_level;
		ev_io_start(loop, &connection_watcher->io);
	}
}

int jrpc_server_init(struct jrpc_server *server, int port_number) {
	loop = EV_DEFAULT;
	return jrpc_server_init_with_ev_loop(server, port_number, loop);
}

int jrpc_server_init_with_ev_loop(struct jrpc_server *server, 
		int port_number, struct ev_loop *loop) {
	memset(server, 0, sizeof(struct jrpc_server));
	server->loop = loop;
	server->port_number = port_number;
	char * debug_level_env = getenv("JRPC_DEBUG");
	if (debug_level_env == NULL)
		server->debug_level = 0;
	else {
		server->debug_level = strtol(debug_level_env, NULL, 10);
		printf("JSONRPC-C Debug level %d\n", server->debug_level);
	}
	return __jrpc_server_start(server);
}

static int __jrpc_server_start(struct jrpc_server *server) {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_in sockaddr;
	int len;
	int yes = 1;
	int rv;
	char PORT[6];
	sprintf(PORT, "%d", server->port_number);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
				== -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		len = sizeof(sockaddr);
		if (getsockname(sockfd, (struct sockaddr *) &sockaddr, &len) == -1) {
			close(sockfd);
			perror("server: getsockname");
			continue;
		}
		server->port_number = ntohs( sockaddr.sin_port );

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, 5) == -1) {
		perror("listen");
		exit(1);
	}
	if (server->debug_level)
		printf("server: waiting for connections...\n");

	ev_io_init(&server->listen_watcher, accept_cb, sockfd, EV_READ);
	server->listen_watcher.data = server;
	ev_io_start(server->loop, &server->listen_watcher);
	return 0;
}

// Make the code work with both the old (ev_loop/ev_unloop)
// and new (ev_run/ev_break) versions of libev.
#ifdef EVUNLOOP_ALL
#define EV_RUN ev_loop
#define EV_BREAK ev_unloop
#define EVBREAK_ALL EVUNLOOP_ALL
#else
#define EV_RUN ev_run
#define EV_BREAK ev_break
#endif

void jrpc_server_run(struct jrpc_server *server){
	EV_RUN(server->loop, 0);
}

int jrpc_server_stop(struct jrpc_server *server) {
	EV_BREAK(server->loop, EVBREAK_ALL);
	return 0;
}

void jrpc_server_destroy(struct jrpc_server *server){
	/* Don't destroy server */
	int i;
	for (i = 0; i < server->procedure_count; i++){
		jrpc_procedure_destroy( &(server->procedures[i]) );
	}
	free(server->procedures);
}

static void jrpc_procedure_destroy(struct jrpc_procedure *procedure){
	if (procedure->name){
		free(procedure->name);
		procedure->name = NULL;
	}
	if (procedure->data){
		free(procedure->data);
		procedure->data = NULL;
	}
}

int jrpc_register_procedure(struct jrpc_server *server,
		jrpc_function function_pointer, char *name, void * data) {
	int i = server->procedure_count++;
	if (!server->procedures)
		server->procedures = malloc(sizeof(struct jrpc_procedure));
	else {
		struct jrpc_procedure * ptr = realloc(server->procedures,
				sizeof(struct jrpc_procedure) * server->procedure_count);
		if (!ptr)
			return -1;
		server->procedures = ptr;

	}
	if ((server->procedures[i].name = strdup(name)) == NULL)
		return -1;
	server->procedures[i].function = function_pointer;
	server->procedures[i].data = data;
	return 0;
}

int jrpc_deregister_procedure(struct jrpc_server *server, char *name) {
	/* Search the procedure to deregister */
	int i;
	int found = 0;
	if (server->procedures){
		for (i = 0; i < server->procedure_count; i++){
			if (found)
				server->procedures[i-1] = server->procedures[i];
			else if(!strcmp(name, server->procedures[i].name)){
				found = 1;
				jrpc_procedure_destroy( &(server->procedures[i]) );
			}
		}
		if (found){
			server->procedure_count--;
			if (server->procedure_count){
				struct jrpc_procedure * ptr = realloc(server->procedures,
					sizeof(struct jrpc_procedure) * server->procedure_count);
				if (!ptr){
					perror("realloc");
					return -1;
				}
				server->procedures = ptr;
			}else{
				server->procedures = NULL;
			}
		}
	} else {
		fprintf(stderr, "server : procedure '%s' not found\n", name);
		return -1;
	}
	return 0;
}
