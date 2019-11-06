#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CHILD_PROCESS 0

static void child_handler(int sig) {
	int status = 0;
	pid_t pid = 0;
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
		fprintf(stderr, "Disconnect with %d\n", pid);
	}
}

int main(int argc, char *argv[]) {
	int serv_sock = 0, port = 0, BUFSIZ = 0;
	int clnt_sock = 0, clnt_addr_size = 0, data_len = 0;
	int num = 0;
	char *message = NULL;
	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;

	if (argc != 3) {
		fprintf(stderr, "Usage %s <port_number> <buffer_size>\n", argv[0]);
		exit(1);
	}

	port = atoi(argv[1]);
	if (port < 0 || port > 0xFFFF) {
		fprintf(stderr, "Usage valid port number\n");
		exit(1);
	}

	message = (char *)malloc(sizeof(char) * BUFSIZ);

	if ((serv_sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket error");
		exit(1);
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = PF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("bind error");
		exit(1);
	}

	while (1) {
		clnt_addr_size = sizeof(clnt_addr);
		data_len = recvfrom(serv_sock, message, BUFSIZ - 1, 0, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
		printf("%d\n", data_len);
		break;
	}

	free(message);
	return 0;
}
