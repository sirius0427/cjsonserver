all: jsonserver

client: client.c jsonrpc.c cjson.c
	gcc $^ -o $@ -lm -lev -g
	
jsonserver: server.c jsonrpc.c cjson.c mystr.c log.c
	gcc $^ -o $@ -lm -lev -g -I/usr/local/include/iLOG3/ -liLOG3
	
	
.PHONY:clean
clean:
	@rm -rf client server jsonserver
	
