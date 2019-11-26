#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int selectMenu();
void readCoils(int sock);

int main(int argc, char *argv[]) {
	int sock = 0, str_len = 0;
	int port = 502;
	char message[BUFSIZ] = { 0, };
	char *server_ip = NULL;
	struct sockaddr_in sock_addr;

	int menu = 0, flag = 0;

	if (argc != 2) {
		fprintf(stderr, "Usage %s <server_ip>\n", argv[0]);
		exit(1);
	}

	server_ip = argv[1];
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

	while (flag != 1) {
		menu = selectMenu();
		switch (menu) {
		case 1:
			readCoils(sock);
			break;
		case 2:
			break;
		case 3:
			break;
		case 4:
			break;
		case 100:
			flag = 1;
			break;
		}
	}

	close(sock);
	return 0;
}

int selectMenu() {
	int select = 0;

	while (1) {
		printf("Which function do you want?\n");
		printf("[1]: Read Coils\n");
		printf("[2]: Write multiple Coils\n");
		printf("[3]: Read Holding Registers\n");
		printf("[4]: Write multiple (Holding) Registers\n");
		printf("[100]: Quit\n");
		printf("Select a function [1, 2, 3, 4, 100]: ");
		scanf("%d", &select);
		
		if ((select >= 1 && select <= 4) || select == 100)
			break;
	}

	return select;
}

void _readCoils(int sock, int s_address, int n_coils) {
	char data[20] = { 0, };
	char *receive_buffer = NULL;
	int data_len = 0, i = 0;
	
	int slave_address = 17;
	int function_code = 1;
	int start_number = s_address;
	int number_coils = n_coils;
	int CRC = 3716;

	sprintf(data, "%02x%02x%04x%04x%04x", slave_address, function_code, start_number, number_coils, CRC);
	for (i = 0; i < 16; i++)
		data[i] = isdigit(data[i]) ? data[i] - '0' : data[i] - 'A' + 10;
	for (i = 0; i < 16; i++) {
		printf("%c", data[i] < 10 ? data[i] + '0' : data[i] - 10 + 'A');
		if (i % 2 == 1)
			printf(" ");
	}
	printf("\n");
	send(sock, data, 16, 0);

	memset(data, 0, 20);
	data_len = recv(sock, data, 20, 0);
	for (i = 0; i < data_len; i++)
		printf("%d ", data[i]);
/*	
	recv(sock, data, 6, 0);
	data_len = (data[4] - '0') * 16 + (data[5] - '0');
	receive_buffer = malloc(sizeof(char) * data_len);
	
	recv(sock, receive_buffer, data_len, 0);
	recv(sock, data + 6, 4, 0);

	printf("DATA Rxed in HEX\n");
	printf("------------------------\n");
	printf("[FC] [BC]: %c%c %02x\n", data[2] + '0', data[3] + '0', data_len);
	printf("[DATA]: ");
	for (i = 0; i < data_len; i++) {
		printf("%c", receive_buffer[i]);
		if (i % 2 == 1)
			printf(" ");
	}
	printf("\n");
	printf("------------------------\n");
	
	free(receive_buffer);
	*/
}

void readCoils(int sock) {
	int start_address = 0, number_of_coils = 0;

	printf("Enter the Start Address: ");
	scanf("%d", &start_address);

	printf("Enter the number of coils to be read: ");
	scanf("%d", &number_of_coils);

	_readCoils(sock, start_address, number_of_coils);
}
