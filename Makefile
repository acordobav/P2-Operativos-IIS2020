all : client server server_hp
.PHONY : all

client : client.c
	gcc client.c -lpng -lpthread -o client

server : server_seq.c
	gcc server_seq.c -lpng -o server_seq

server_hp : server_hp.c
	gcc server_hp.c -lpng -o server_hp