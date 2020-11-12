all : client server server_hp
.PHONY : all

client : client.c
	gcc client.c -lpng -lpthread -o client

server : server.c
	gcc server.c -lpng -o server

server_hp : server_hp.c
	gcc server_hp.c -lpng -o server_hp