all : client server_seq server_hp server_php
.PHONY : all

client : client.c
	gcc client.c -lpng -lpthread -o client

server_seq : server_seq.c
	gcc server_seq.c -lpng -o server_seq

server_hp : server_hp.c
	gcc server_hp.c -lpng -o server_hp

server_php : server_php.c
	gcc server_php.c -lpng -lpthread -o server_php

clean:
	rm server_hp && rm server_php && rm server_seq && rm client
	rm -rf sequential && rm -rf heavy_process && rm -rf pre_heavy_process