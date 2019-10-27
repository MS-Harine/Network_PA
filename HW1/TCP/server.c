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
	int serv_sock = 0, port = 0, buf_size = 0;
	int clnt_sock = 0, clnt_addr_size = 0, str_len = 0;
	int pid = 0, status = 0;
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

	buf_size = atoi(argv[2]);
	if (buf_size < 1) {
		fprintf(stderr, "Usage valid buf size\n");
		exit(1);
	}

	if ((serv_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
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

	if (listen(serv_sock, 5) < 0) {
		perror("Listen error");
		exit(1);
	}

	clnt_addr_size = sizeof(clnt_addr);
	signal(SIGCHLD, (void *)child_handler);
	
	while (1) {
		clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
		if (clnt_sock == -1) {
			perror("accept error");
			continue;
		}

		pid = fork();
		if (pid == -1) {
			perror("fork error");
			exit(1);
		}
		else if (pid == CHILD_PROCESS) {
			close(serv_sock);
			sleep(5);
			message = (char *)malloc((buf_size + 1) * sizeof(char));
			while((str_len = read(clnt_sock, message, buf_size)) > 0) {
				message[str_len] = 0;
				printf("%s: %s", inet_ntoa(clnt_addr.sin_addr), message);
				//write(clnt_sock, message, strlen(message));
			}
			free(message);
			close(clnt_sock);
			exit(0);
		}
		else {
			printf("Connect with %s (%d)\n", inet_ntoa(clnt_addr.sin_addr), pid);
			close(clnt_sock);
		}
	}

	return 0;
}
