#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
	int sock = 0, port = 0, data_len = 0, data_total_len = 0;
	int file_size = 0, str_len = 0, total_str_len = 0;
	int i = 0;
	char message[BUFSIZ] = { 0, }, *filename = NULL;
	char *server_ip = NULL;
	struct sockaddr_in sock_addr;
	FILE *fp = NULL;

	if (argc != 4) {
		fprintf(stderr, "Usage %s <server_ip> <server_port> <filename>\n", argv[0]);
		exit(1);
	}

	server_ip = argv[1];
	port = atoi(argv[2]);
	filename = argv[3];
	if (port < 0 || port > 0xFFFF) {
		fprintf(stderr, "Usage valid port number\n");
		exit(1);
	}

	fp = fopen(filename, "r");
	if (fp == NULL) {
		perror("Fail to open file: ");
		exit(1);
	}

	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error");
		exit(1);
	}

	memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.sin_family = PF_INET;
	sock_addr.sin_port = htons(port);
	sock_addr.sin_addr.s_addr = inet_addr(server_ip);

	if (connect(sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == -1) {
		perror("connect error");
		exit(1);
	}

	printf("Connected with server...\n");
	printf("Try to sending file %s\n", filename);

	strcpy(message, filename);
	write(sock, message, strlen(filename) + 1);

	while (feof(fp) == 0) {
		data_len = fread(message, sizeof(char), BUFSIZ, fp);
		data_total_len += data_len;
		str_len = write(sock, message, data_len);
		total_str_len += str_len;
		printf("Sending >>>>>>>>>>>>> (%d/%d)\r", total_str_len, file_size);
	}
	printf("\n");
	printf("Success to send file to server!\n");

	fclose(fp);
	close(sock);
	return 0;
}
