all : client server
.PHONY : all

client : client.c
	gcc client.c -lpng -o client

server : server.c
	gcc server.c -lpng -o server