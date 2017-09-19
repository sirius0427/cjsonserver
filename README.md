<!--
author:sirius0427 
date: 2017-09-19
title: C语言实现jsonrpc
tags: cjsonserver 
category:作品
status: publish
summary:C语言实现jsonrpc，采用2.0协议并兼容1.x，支持http。
-->

```

20170919 修改了一个URL地址导致的错误，并在server.c增加了方法datainsert。

程序说明:该代码是在jsonrpc-c的基础上，添加jsonrpc-2.0支持（版本信息，批处理等），添加http协议支持，增加客户端代码得到，
如果使用过程中发现有bug以后会再加以修改。

编译使用:
cd third_party
tar -zxvf libev-4.22.tar.gz 
cd libev-4.22
./configure && make && make install

cd ..

make

服务端server.c:
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
	strcpy(msg,"error message content.");
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
	jrpc_server_run(&my_server);
	jrpc_server_destroy(&my_server);
	return 0;
}

客户端client.c:
#include <unistd.h>
#include <fcntl.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>  
#include <stdlib.h>  
#include <stdio.h>  
#include <stddef.h>  
#include <string.h>
#include <ev.h>

#define JRPC_HOST			"121.40.96.112"
#define JRPC_PORT			1234

#define JRPC_POST			"POST http://121.40.96.112:1234/%s HTTP/1.1\r\nHost: 121.40.96.112:%d\r\nConnection: keep-alive\r\nContent-Length: %d\r\n\r\n%s"
//#define JRPC_POST			"POST http://121.40.96.112:1234/%s HTTP/1.1\r\nHost: 121.40.96.112:%d\r\nConnection: keep-alive\r\nContent-Length: %d\r\nAccept: */*\r\n\r\n%s"

#define D(exp,fmt,...) do {                     \
        if(!(exp)){                             \
            fprintf(stderr,fmt,##__VA_ARGS__);  \
            abort();                            \
        }                                       \
    }while(0) 

static void setnonblock(int fd)
{  
    fcntl(fd,F_SETFL,fcntl(fd,F_GETFL) | O_NONBLOCK);  
}  
static void setreuseaddr(int fd)
{  
    int ok=1;  
    setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&ok,sizeof(ok));  
}  
  
static void setaddress(const char* ip,int port,struct sockaddr_in* addr)
{  
    bzero(addr,sizeof(*addr));  
    addr->sin_family=AF_INET;  
    inet_pton(AF_INET,ip,&(addr->sin_addr));  
    addr->sin_port=htons(port);  
}  
  
static void address_to_string(struct sockaddr_in* addr,char* ip, char* port)
{  
    inet_ntop(AF_INET,&(addr->sin_addr),ip,sizeof(ip));  
    snprintf(port,sizeof(port),"%d",ntohs(addr->sin_port)); 
}  
  
static int new_tcp_server(int port)
{  
    int fd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);  
    D(fd>0,"socket failed(%m)\n");  
    setnonblock(fd);  
    setreuseaddr(fd);  
    struct sockaddr_in addr;  
    setaddress("0.0.0.0",port,&addr);  
    bind(fd,(struct sockaddr*)&addr,sizeof(addr));  
    listen(fd,64); // backlog = 64  
    return fd;  
}  
  
static int new_tcp_client(const char* ip,int port){  
    int fd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);  
    setnonblock(fd);  
    struct sockaddr_in addr;  
    setaddress(ip,port,&addr);  
    connect(fd,(struct sockaddr*)(&addr),sizeof(addr));  
    return fd;  
} 

  
int main()
{  
	char wbuff[2048]={0};
	char rbuff[2048]={0};
    struct ev_loop* reactor=ev_loop_new(EVFLAG_AUTO);  
    int fd = new_tcp_client(JRPC_HOST,JRPC_PORT);
	
	while(1)
	{
		sprintf(wbuff,JRPC_POST,"sayHello",JRPC_PORT,strlen("{\"jsonrpc\":\"2.0\",\"method\":\"sayHello\"}"),"{\"jsonrpc\":\"2.0\",\"method\":\"sayHello\"}");
		//sprintf(wbuff,JRPC_POST,"error",JRPC_PORT,strlen("{\"jsonrpc\":\"2.0\",\"method\":\"error\",\"id\":1}"),"{\"jsonrpc\":\"2.0\",\"method\":\"error\",\"id\":1}");
		send(fd,wbuff,strlen(wbuff)+1,0);
		printf("Send--->:\r\n%s\r\n",wbuff);
		recv(fd,rbuff,sizeof(rbuff),0);
		printf("Recv<---:\r\n%s\r\n",rbuff);
		usleep(1000*200);
		memset(wbuff,0,sizeof(wbuff));
		memset(rbuff,0,sizeof(rbuff));	
	}
	close(fd);
	
    return 0;  
}

```



# cjsonserver
