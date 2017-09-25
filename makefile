all: client server

client: client.c jsonrpc.c cjson.c
	gcc $^ -o $@ -lm -lev -g
	
server: server.c jsonrpc.c cjson.c
	gcc $^ -o $@ -lm -lev -g
	
	
.PHONY:clean
clean:
	@rm -rf client server
	
