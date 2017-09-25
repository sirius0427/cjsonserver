/*
 * server.c
 *
 *  Created on: Oct 9, 2012
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
#include <sys/wait.h>
#include <signal.h>
#include "jsonrpc.h"

#define PORT 1234  // the port users will be connecting to

struct jrpc_server my_server;


cJSON * data_insert(jrpc_context * ctx, cJSON * params, cJSON *id) 
{
	//int i,count;
	//count = cJSON_GetArraySize(params);
	//printf( "params count[%d]\n", count );
	printf( "params[%d]\n", params->type);
	cJSON *child,*content,*createTime,*cid,*talker;
	child = params->child;
	while( child != NULL )
	{
		content = cJSON_GetObjectItem( child , "content");
		createTime = cJSON_GetObjectItem( child , "createTime");
		cid = cJSON_GetObjectItem( child , "id");
		talker = cJSON_GetObjectItem( child , "talker");

		printf( "content[%s]\ncreateTime[%.0lf]\nid[%d]\ntalker[%s]\n", content->valuestring, createTime->valuedouble, cid->valueint, talker->valuestring );
		printf( "--------------------\n" );
		child = child->next;
	}


	return cJSON_CreateNumber(0);
}

cJSON * say_hello(jrpc_context * ctx, cJSON * params, cJSON *id) 
{
	return cJSON_CreateString("Hello!");
}

cJSON * exit_server(jrpc_context * ctx, cJSON * params, cJSON *id) 
{
	jrpc_server_stop(&my_server);
	return cJSON_CreateString("Bye!");
}

cJSON * error_cb(jrpc_context * ctx, cJSON * params, cJSON *id) 
{
	char* msg = (char*)malloc(256);
	char* msgc = (char*)malloc(256);
	strcpy(msg,"error message.");
	strcpy(msgc,"error message content.");
	ctx->error_code = 1;
	ctx->error_message = msg;
	ctx->data = msgc;
	return NULL;
}

cJSON * error_cb2(jrpc_context * ctx, cJSON * params, cJSON *id) 
{
	char* msg = (char*)malloc(256);
	char* msgc = (char*)malloc(256);
	strcpy(msg,"error message.");
	ctx->error_code = 1;
	ctx->error_message = msg;
	return NULL;
}

cJSON * data_cb(jrpc_context * ctx, cJSON * params, cJSON *id) 
{
	return cJSON_CreateString("data");
}

int main(void) 
{
	jrpc_server_init(&my_server, PORT);
	my_server.debug_level=2;
	my_server.type = JRPC_TYPE_HTTP;
	//my_server.debug_level=1;
	jrpc_register_procedure(&my_server, say_hello, "sayHello", NULL );
	jrpc_register_procedure(&my_server, exit_server, "exit", NULL );
	jrpc_register_procedure(&my_server, error_cb, "error", NULL );
	jrpc_register_procedure(&my_server, error_cb2, "error2", NULL );
	jrpc_register_procedure(&my_server, data_cb, "data", NULL );
	jrpc_register_procedure(&my_server, data_insert, "datainsert", NULL );
	jrpc_server_run(&my_server);
	jrpc_server_destroy(&my_server);
	return 0;
}

