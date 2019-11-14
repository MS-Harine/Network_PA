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
#define DPRINT(func) func; setbuf(stdout, NULL);
#else
#define DPRINT(func) ;
#endif

#define CHILD_PROCESS 0

int work(int clnt_sock);

enum PROTOCOL { GET, POST };
enum PAGE { INDEX, QUERY, SAMPLE, NOT_FOUND };

char *make_data(const char *content, int status);
int send_response(int sockfd, int status, const char *content, int flag);
char *read_file(const char *filename);

static void child_handler(int sig) {
	int status = 0;
	pid_t pid = 0;
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
		fprintf(stderr, "Disconnect with %d\n", pid);
	}
}

int main(int argc, char *argv[]) {
	int serv_sock = 0, port = 0;
	int clnt_sock = 0, clnt_addr_size = 0, str_len = 0;
	int pid = 0, status = 0;
	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;

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
			work(clnt_sock);
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

int work(int clnt_sock) {
	char *message = NULL, *more_data = NULL, *parsed = NULL, *temp = NULL;
	int cur_size = 0, whole_size = 0, data_len = 0, content_length = 0, i = 0;
	enum PROTOCOL protocol, page;
	char host_str[] = "Host: ", c_length_str[] = "Content-Length: ";

	cur_size = BUFSIZ;
	message = (char *)malloc(sizeof(char) * cur_size);

	while((data_len = recv(clnt_sock, message + whole_size, 1, 0)) > 0) {
		if (whole_size == cur_size) {
			message = realloc(message, sizeof(char) * cur_size * 2);
			cur_size *=2;
		}
		whole_size++;
		if (whole_size > 4) {
			if (message[whole_size - 4] == '\r' &&
					message[whole_size - 3] == '\n' &&
					message[whole_size - 2] == '\r' &&
					message[whole_size - 1] == '\n')
				break;
		}
	}
	DPRINT(printf("Header\n%s", message));

	// Protocol
	parsed = strtok(message, " ");
	if (strcmp(parsed, "GET") == 0)
		protocol = GET;
	else if (strcmp(parsed, "POST") == 0)
		protocol = POST;
	else {
		fprintf(stderr, "Wrong protocol");
		return -1;
	}
	DPRINT(printf("Protocol: %s\n", protocol == GET ? "GET" : protocol == POST ? "POST" : "ELSE"));

	// Request page
	parsed = strtok(NULL, " ");
	if (strcmp(parsed, "/") == 0)
		page = INDEX;
	else if (strcmp(parsed, "/index.html") == 0)
		page = INDEX;
	else if (strcmp(parsed, "/query.html") == 0)
		page = QUERY;
	else if (strcmp(parsed, "/sample") == 0)
		page = SAMPLE;
	else
		page = NOT_FOUND;
	DPRINT(printf("Page: %s\n", page == INDEX ? "index.html" : page == QUERY ? "query.html" : page == SAMPLE ? "sample" : "NOT_FOUNT"));

	// HTTP version
	parsed = strtok(NULL, " \n");
	DPRINT(printf("HTTP version: %s\n", parsed));

	switch (protocol) {
		case GET:
			if (page == INDEX) {
				free(message);
				
				message = read_file("index.html");
				DPRINT(printf("Content: \n%s\n", message));
				send_response(clnt_sock, 200, message, 0);
				free(message);
			}
			else if (page == QUERY) {
				// Host
				while ((parsed = strtok(NULL, "\n"))) {
					parsed = strstr(parsed, host_str);
					if (parsed == NULL)
						continue;

					parsed += strlen(host_str);
					temp = strchr(parsed, '\r');
					more_data = (char *)malloc(sizeof(char) * (temp - parsed + 1));
					strncpy(more_data, parsed, (temp - parsed));
					DPRINT(printf("Host: %s\n", more_data));
					break;
				}
				free(message);

				message = read_file("query.html");

				message = realloc(message, sizeof(char) * (strlen(message) + strlen(more_data) + 16));
				parsed = strstr(message, "action");
				temp = malloc(sizeof(char) * (strlen(parsed) + strlen(more_data) + 1));
				strcpy(temp, "http://");
				strcat(temp, more_data);
				strcat(temp, "/sample");
				strcat(temp, parsed + 8);
				strcpy(parsed + 8, temp);
				free(more_data);

				DPRINT(printf("Content: \n%s\n", message));
				send_response(clnt_sock, 200, message, 0);
				free(message);
			}
			else {
				free(message);
				send_response(clnt_sock, 404, message, 0);
			}
			break;

		case POST:
			// Content-Length
			while ((parsed = strtok(NULL, "\n"))) {
				parsed = strstr(parsed, c_length_str);
				if (parsed == NULL)
					continue;

				parsed += strlen(c_length_str);
				content_length = atoi(parsed);
				DPRINT(printf("Content-Length: %d\n", content_length));
				break;
			}
			free(message);
			
			more_data = (char *)malloc(sizeof(char) * (content_length + 1));
			recv(clnt_sock, more_data, content_length, 0);
			more_data[content_length] = 0;
			DPRINT(printf("More_data: %s\n", more_data));
			send_response(clnt_sock, 200, more_data, 0);
			free(more_data);
			break;
	}
	return 0;
}

char *read_file(const char *filename) {
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		fprintf(stderr, "No file : %s\n", filename);
		exit(0);
	}

	int cur_size = BUFSIZ, data_len = 0, whole_size = 0;
	char *message = malloc(sizeof(char) * cur_size);

	while(!feof(fp)) {
		data_len = fread(message, sizeof(char), cur_size, fp);
		whole_size += data_len;
		if (whole_size > cur_size) {
			message = realloc(message, sizeof(char) * cur_size * 2);
			cur_size *= 2;
		}
	}

	return message;
}

int send_response(int sockfd, int status, const char *content, int flag) {
	char *message = make_data(content, status);
	int s_len = 0;

	DPRINT(printf("\nData to send\n\n%s\n", message));
	s_len = send(sockfd, message, strlen(message) + 1, flag);
	free(message);
	return s_len;
}

char *make_data(const char *content, int status) {
	char *message = malloc(sizeof(char) * (strlen(content) + 200));
	char content_len[20] = { 0, };

	strcpy(message, "HTTP/1.1 ");
	switch (status) {
		case 200:
			strcat(message, "200 OK\r\n");
			break;
		case 404:
			strcat(message, "404 NOT FOUND\r\n");
			break;
	}

	strcat(message, "Content-Length: ");
	sprintf(content_len, "%d\r\n", status == 200 ? strlen(content) : 0);
	strcat(message, content_len);

	strcat(message, "Content-Type: text/html\r\n");
	strcat(message, "Connection: Closed\r\n\r\n");

	if (status == 200)
		strcat(message, content);

	return message;
}
