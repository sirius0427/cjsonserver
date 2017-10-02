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
#include "LOGS.h"

#define PORT 1234  // the port users will be connecting to

struct jrpc_server my_server;


cJSON * data_insert(jrpc_context * ctx, cJSON * params, cJSON *id) 
{
	cJSON *child,*content,*createTime,*cid,*talker;
	char wxid[256], smscontent[18010],chatroom[256],actalker[256];
	char sqlinsert[3000];
	int istart,iend;
	child = params->child;
	child = child->child;
	while( child != NULL )
	{
		content = cJSON_GetObjectItem( child , "content");
		if (content == NULL)
		{
			ERRORLOGSG( "取不到元素content,取下一个" );
			child = child->next;
			continue;
		}
		createTime = cJSON_GetObjectItem( child , "createTime");
		if (createTime == NULL)
		{
			ERRORLOGSG( "取不到元素createTime,取下一个" );
			child = child->next;
			continue;
		}
		cid = cJSON_GetObjectItem( child , "id");
		if (cid == NULL)
		{
			ERRORLOGSG( "取不到元素cid,取下一个" );
			child = child->next;
			continue;
		}
		talker = cJSON_GetObjectItem( child , "talker");
		if (talker == NULL)
		{
			ERRORLOGSG( "取不到元素talker,取下一个" );
			child = child->next;
			continue;
		}

		istart = 0;
		iend = 0;
		memset( wxid, 0x00, sizeof( wxid ));
		memset( smscontent, 0x00, sizeof( smscontent ));
		memset( chatroom, 0x00, sizeof( chatroom ));
		memset( actalker, 0x00, sizeof( actalker ));
		memset( sqlinsert, 0x00, sizeof( sqlinsert ));

		replace( content->valuestring, "\n", "" );
		replace( content->valuestring, "\r", "" );
		//DEBUGLOGSG( "content[%s]", content->valuestring );
		DEBUGLOGSG( "createTime[%.0lf]", createTime->valuedouble );
		DEBUGLOGSG( "id[%d]", cid->valueint );
		DEBUGLOGSG( "talker[%s]", talker->valuestring);
		
		istart = indexOf( content->valuestring, ":" );
		if( istart > 0 )
		{
			strncpy( wxid, content->valuestring, istart );
			strncpy( smscontent, content->valuestring + istart + 1, strlen(content->valuestring)-istart-1 );
		}
		else
		{
			strncpy( smscontent, content->valuestring + istart + 1, strlen(content->valuestring)-istart-1 );
		}
		istart = indexOf( talker->valuestring, ":" );
		if( istart > 0 )
		{
			strncpy( chatroom, talker->valuestring, istart );
			strncpy( actalker, talker->valuestring + istart + 1, strlen(talker->valuestring)-istart-1 );
		}
		else
		{
			strcpy( chatroom, talker->valuestring );
		}
		sprintf( sqlinsert, "insert into weixin_sms values ('%.0lf','%d','%s','%s','%s','%s')",
				createTime->valuedouble, cid->valueint, wxid, smscontent, chatroom, actalker );
		DEBUGLOGSG( "wxid[%s]", wxid );
		printf( "chatroom[%s]wxid[%s]smscontent[%s]\n",chatroom, wxid, smscontent );
		DEBUGLOGSG( "chatroom[%s]", chatroom );
		DEBUGLOGSG( "actalker[%s]", actalker );
		
		child = child->next;
		DEBUGLOGSG( "----------------------" );
	}

	return cJSON_CreateNumber(0);
}

cJSON * contact_insert(jrpc_context * ctx, cJSON * params, cJSON *id) 
{
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
	BEGINLOG( );
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
	jrpc_register_procedure(&my_server, contact_insert, "contactinsert", NULL );
	jrpc_server_run(&my_server);
	jrpc_server_destroy(&my_server);
	return 0;
}

