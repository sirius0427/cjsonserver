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

#define JRPC_HOST			"192.168.199.209"
#define JRPC_PORT			1024

#define JRPC_POST			"POST http://sql.wangxiaotian.com:1234/%s HTTP/1.1\r\nHost: sql.wangxiaotian.com:%d\r\nConnection: keep-alive\r\nContent-Length: %d\r\n\r\n%s"
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
	int i = 0;
    struct ev_loop* reactor=ev_loop_new(EVFLAG_AUTO);  
    int fd = new_tcp_client(JRPC_HOST,JRPC_PORT);
	
	for(i=0;i<=1;i++)
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



