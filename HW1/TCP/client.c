#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
	int sock = 0, port = 0, str_len = 0;
	char message[BUF_SIZE] = { 0, };
	char *server_ip = NULL;
	struct sockaddr_in sock_addr;

	if (argc != 3) {
		fprintf(stderr, "Usage %s <server_ip> <server_port>\n", argv[0]);
		exit(1);
	}

	server_ip = argv[1];
	port = atoi(argv[2]);
	if (port < 0 || port > 0xFFFF) {
		fprintf(stderr, "Usage valid port number\n");
		exit(1);
	}

	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error");
		exit(1);
	}

	// Bind the socket
	memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.sin_family = PF_INET;
	sock_addr.sin_port = htons(port);
	sock_addr.sin_addr.s_addr = inet_addr(server_ip);

	if (connect(sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == -1) {
		perror("connect error");
		exit(1);
	}
	
	while (1) {
		fputs("Input message (q to quit): ", stdout);
		fgets(message, BUF_SIZE - 1, stdin);

		if (!strcmp(message, "q\n"))
			break;

		write(sock, message, strlen(message));

		str_len = read(sock, message, BUF_SIZE - 1);
		message[str_len] = 0;
		printf("Message from server: %s\n", message);
	}

	close(sock);
	return 0;
}
