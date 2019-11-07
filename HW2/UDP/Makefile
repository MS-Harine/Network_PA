all:
	gcc server.c -o UDPserver
	gcc client.c -o UDPclient

debug:
	gcc server.c -o UDPserver -DDEBUG
	gcc client.c -o UDPclient -DDEBUG

clean:
	rm UDPserver UDPclient
