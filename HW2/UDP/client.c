#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>

#include "data.h"

#define TIMEOUT 1
#define DISCON_WAIT_PLZ 3

#ifdef DEBUG
#define DPRINT(func) func
#else
#define DPRINT(func) ;
#endif

int set_blocking(int sockfd, int blocking);
char * make_header(u_short cid, u_short csi, int isFIN, int isSYN, u_int w_sz, u_int len);
DGRAM_HEADER * get_header(char *message);
void get_content(char *content, char *message, int length);
char * concat_header_message(char *header, char *message, int len);

int main(int argc, char *argv[]) {
	int sock = 0, port = 0, data_len = 0, whole_data = 0;
	int flag = 0, addr_len = 0, file_len = 0;
	char *message = NULL, buffer[BUFSIZ] = { 0, }, filename[FILENAME_MAX] = { 0, };
	char *server_ip = NULL, *header = NULL;
	int p_start = 0, p_end = 0;
	struct sockaddr_in sock_addr;
	DGRAM_HEADER *t_header = NULL;
	u_short my_id = 0;
	FILE *fp = NULL;

	if (argc != 4) {
		fprintf(stderr, "Usage %s <server_ip> <server_port> <filename>\n", argv[0]);
		exit(1);
	}

	server_ip = argv[1];
	port = atoi(argv[2]);
	if (port < 0 || port > 0xFFFF) {
		fprintf(stderr, "Usage valid port number\n");
		exit(1);
	}
	strcpy(filename, argv[3]);
	
	srand(time(NULL));

	if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket error");
		exit(1);
	}

	memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.sin_family = PF_INET;
	sock_addr.sin_port = htons(port);
	sock_addr.sin_addr.s_addr = inet_addr(server_ip);
	
	addr_len = sizeof(sock_addr);
	my_id = rand();

	p_start = time(NULL);

	int i = 0;
	header = make_header(0, my_id, FALSE, TRUE, BUFSIZ, sizeof(char) * (strlen(filename) + 1));
	message = concat_header_message(header, filename, strlen(filename) + 1);
	free(header);

	printf("Try to send filename to server...\n");
	data_len = reliable_sendto(sock, message, sizeof(DGRAM_HEADER) + strlen(filename) + 1, 0, (struct sockaddr *)&sock_addr, &addr_len);
	free(message);
	printf("Filename was sent successfully!\n");

	t_header = get_header(buffer);
	my_id = t_header->client_id;
	free(t_header);
	
	fp = fopen(filename, "rb");
	fseek(fp, 0, SEEK_END);
	file_len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	while (feof(fp) == 0) {
		data_len = fread(buffer, sizeof(char), BUFSIZ, fp);
		whole_data += data_len;
		header = make_header(my_id, 0, FALSE, FALSE, BUFSIZ, data_len);
		message = concat_header_message(header, buffer, data_len);
		free(header);
		sendto(sock, message, sizeof(DGRAM_HEADER) + data_len, 0, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
		free(message);
		printf("Sending >>>>>>>>>>>>>>>>> (%d/%d)\r", whole_data, file_len);
	}
	printf("\n");

	fclose(fp);

	sprintf(buffer, "%d\0", DISCON_WAIT_PLZ);
	header = make_header(my_id, 0, TRUE, FALSE, BUFSIZ, strlen(buffer) + 1);
	message = concat_header_message(header, buffer, strlen(buffer) + 1);
	free(header);

	printf("Try to disconnect with server...\n");
	data_len = reliable_sendto(sock, &message, sizeof(DGRAM_HEADER) + strlen(buffer) + 1, 0, (struct sockaddr *)&sock_addr, &addr_len);
	printf("Disconnected with server successfully!\n");
	free(message);

	p_end = time(NULL);
	printf("Data transmission time: %dsec\n", p_end - p_start);
	return 0;
}

int reliable_sendto(int sock, char *message, int len, int flags, struct sockaddr *addr, int *addr_size) {
	int start = 0, end = 0;
	char buffer[BUFSIZ] = { 0, };
	int data_len = 0, count = 0;

	while (1) {
		start = time(NULL);
		sendto(sock, message, len, flags, addr, sizeof(*addr));
		DPRINT(printf("Try to connect..... %d\r", count++); setbuf(stdout, NULL);)
		set_blocking(sock, FALSE);
		for (end = time(NULL); end - start < TIMEOUT; end = time(NULL)) {
			data_len = recvfrom(sock, buffer, BUFSIZ, 0, addr, addr_size);
			if (data_len >= 0)
				break;
		}
		set_blocking(sock, TRUE);
		
		if (data_len >= 0)
			break;
	}
	DPRINT(printf("\n"));
	return data_len;
}

int set_blocking(int sockfd, int blocking) {
	int flag = fcntl(sockfd, F_GETFL, 0);
	if (blocking)
		flag &= ~O_NONBLOCK;
	else
		flag |= O_NONBLOCK;
	return fcntl(sockfd, F_SETFL, flag) != -1;
}

char * make_header(u_short cid, u_short csi, int isFIN, int isSYN, u_int w_sz, u_int len) {
	DGRAM_HEADER *header = malloc(sizeof(DGRAM_HEADER));
	header->client_id = cid;
	header->c_say_id = csi;
	header->flags &= MSK_CLR;
	if (isFIN) header->flags |= MSK_FIN;
	if (isSYN) header->flags |= MSK_SYN;
	header->window_size = w_sz;
	header->data_length = len;
	
	char *h_message = malloc(sizeof(DGRAM_HEADER));
	memcpy(h_message, header, sizeof(DGRAM_HEADER));
	free(header);

	return h_message;
}

DGRAM_HEADER * get_header(char *message) {
	DGRAM_HEADER *header = malloc(sizeof(DGRAM_HEADER));
	memcpy(header, message, sizeof(DGRAM_HEADER));
	return header;
}

void get_content(char *content, char *message, int length) {
	memcpy(content, message + sizeof(DGRAM_HEADER), length - sizeof(DGRAM_HEADER));
}

char * concat_header_message(char *header, char *message, int len) {
	char *m = malloc(len + sizeof(DGRAM_HEADER));
	memcpy(m, header, sizeof(DGRAM_HEADER));
	memcpy(m + sizeof(DGRAM_HEADER), message, len);
	return m;
}
