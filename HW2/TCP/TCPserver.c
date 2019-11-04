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

#ifdef DEBUG
#define DPRINT(func) func
#else
#define DPRINT(func) ;
#endif

int main(int argc, char *argv[]) {
	int serv_sock = 0, port = 0, is_filename = 1;
	int clnt_sock = 0, clnt_addr_size = 0, data_len = 0;
	char *message = NULL, filename[FILENAME_MAX] = { 0, };
	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	FILE *fp;

	if (argc != 2) {
		fprintf(stderr, "Usage %s <port_number>\n", argv[0]);
		exit(1);
	}

	port = atoi(argv[1]);
	if (port < 0 || port > 0xFFFF) {
		fprintf(stderr, "Usage valid port number\n");
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
	while (1) {
		clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
		if (clnt_sock == -1) {
			perror("accept error");
			continue;
		}
		printf("Connect with %s\n", inet_ntoa(clnt_addr.sin_addr));

		message = (char *)malloc(BUFSIZ * sizeof(char));
		while((data_len = read(clnt_sock, message, BUFSIZ)) > 0) {
			if (is_filename) {
				strncpy(filename, message, strlen(message) + 1);
				strncpy(message, message + strlen(message) + 1, data_len - strlen(message) - 1);
				data_len -= (strlen(message) + 1);
				is_filename = !is_filename;
				DPRINT(printf("Filename: %s\n", filename));
				fp = fopen(filename, "wb");
				if (fp == NULL) {
					perror("Fail to open file");
					break;
				}
			}
			fwrite(message, sizeof(char), data_len, fp);
		}
		fclose(fp);
		free(message);
		close(clnt_sock);
	}

	return 0;
}
