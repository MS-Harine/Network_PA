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

#include "data.h"

struct client_list {
	u_short client_id;
	u_short c_say_id;
	u_int window_size;
	struct sockaddr_in clnt_addr;
};

char * make_header(u_short cid, int isFIN, int isSYN, u_int w_sz, u_int len);
int receive_data(int sock, char *message, int size, struct sockaddr_in *clnt_addr, int *c_num);
DGRAM_HEADER * get_header(char *message);
void get_content(char *content, char *message, int length);
void connect_with(int client_num, int sockfd);
void disconnect_with(int client_num, char *message, int len);
void work_with(int client_num, char *message, int len, const char *filename);

int client_count = 1;
int want_to_disconnect = -1;
struct client_list *clnt_list = NULL;

void disconnect_handler() {
	printf("Disconnect with (%d)%s\n", want_to_disconnect, inet_ntoa(clnt_list[want_to_disconnect].clnt_addr.sin_addr));
	clnt_list[want_to_disconnect].client_id = 0;
}

int main(int argc, char *argv[]) {
	int sock = 0, port = 0;
	int clnt_num = 0, data_len = 0;
	char message[BUFSIZ] = { 0, }, filename[FILENAME_MAX] = { 0, };
	struct sockaddr_in serv_addr, clnt_addr;
	DGRAM_HEADER *header = NULL;
	clnt_list = malloc(sizeof(struct client_list) * client_count);

	if (argc != 2) {
		fprintf(stderr, "Usage %s <port_number>\n", argv[0]);
		exit(1);
	}

	port = atoi(argv[1]);
	if (port < 0 || port > 0xFFFF) {
		fprintf(stderr, "Usage valid port number\n");
		exit(1);
	}

	if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket error");
		exit(1);
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = PF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("bind error");
		exit(1);
	}

	signal(SIGALRM, disconnect_handler);

	while (1) {
		data_len = receive_data(sock, message, BUFSIZ, &clnt_addr, &clnt_num);
		header = get_header(message);
		if (header->flags & MSK_SYN) {
			if (data_len > 0) get_content(filename, message, data_len);
			connect_with(clnt_num, sock);
		}
		else if (header->flags & MSK_FIN) {
			disconnect_with(clnt_num, message, data_len);
		}
		else {
			work_with(clnt_num, message, data_len, filename);
		}
		free(header);
	}

	return 0;
}

char * make_header(u_short cid, int isFIN, int isSYN, u_int w_sz, u_int len) {
	DGRAM_HEADER *header = malloc(sizeof(DGRAM_HEADER));
	header->client_id = cid;
	header->flags = 0;
	if (isFIN) header->flags |= MSK_FIN;
	if (isSYN) header->flags |= MSK_SYN;
	header->window_size = w_sz;
	header->data_length = len;
	
	char *h_message = malloc(sizeof(DGRAM_HEADER));
	memcpy(h_message, header, sizeof(DGRAM_HEADER));
	free(header);

	return h_message;
}

int receive_data(int sock, char *message, int size, struct sockaddr_in *clnt_addr, int *c_num) {
	int data_len = 0, addr_size = 0;
	int i = 0;
	DGRAM_HEADER *header = NULL;

	addr_size = sizeof(*clnt_addr);
	data_len = recvfrom(sock, message, size, 0, (struct sockaddr *)clnt_addr, &addr_size);
	header = get_header(message);
	
	if (header->client_id == 0) {
		for (i = 0; i < client_count; i++) {
			if (clnt_list[i].c_say_id == header->c_say_id)
				return -1;

			if (clnt_list[i].client_id == 0)
				break;
		}

		if ((i == client_count) && (client_count % 2 == 0))
			clnt_list = realloc(clnt_list, sizeof(struct client_list) * client_count * 2);

		clnt_list[i].client_id = client_count;
		clnt_list[i].c_say_id = header->c_say_id;
		clnt_list[i].window_size = header->window_size;
		memcpy(&clnt_list[i].clnt_addr, clnt_addr, sizeof(struct sockaddr));
		*c_num = i;
		client_count++;
	}
	else
		*c_num = header->client_id;

	free(header);

	return data_len;
}

DGRAM_HEADER * get_header(char *message) {
	DGRAM_HEADER *header = malloc(sizeof(DGRAM_HEADER));
	memcpy(header, message, sizeof(DGRAM_HEADER));
	return header;
}

void get_content(char *content, char *message, int length) {
	memcpy(content, message + sizeof(DGRAM_HEADER), length - sizeof(DGRAM_HEADER));
}

void connect_with(int client_num, int sockfd) {
	struct sockaddr_in *clnt_addr = &clnt_list[client_num].clnt_addr;
	char *header = make_header(clnt_list[client_num].client_id, TRUE, FALSE, clnt_list[client_num].window_size, 0);
	sendto(sockfd, header, sizeof(DGRAM_HEADER), 0, (struct sockaddr *)clnt_addr, sizeof(*clnt_addr));
	free(header);
	printf("Try to connect with (%d)%s\n", client_num, inet_ntoa(clnt_addr->sin_addr));
}

void disconnect_with(int client_num, char *message, int len) {
	char buf[BUFSIZ] = { 0, };

	alarm(0);
	get_content(buf, message, len);
	int time = atoi(buf);
	want_to_disconnect = client_num;
	alarm(time);
}

void work_with(int client_num, char *message, int len, const char *filename) {
	FILE *fp = NULL;
	char buf[BUFSIZ] = { 0, };
	DGRAM_HEADER *header = NULL;

	printf("Work with (%d)\n", client_num);
	fp = fopen(filename, "ab");
	
	header = get_header(message);
	get_content(buf, message, len);
	fwrite(buf, sizeof(char), header->data_length, fp);
	fclose(fp);
}
