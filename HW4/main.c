#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "modbus.h"

int selectMenu();

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
			//writeMultipleCoils(sock);
			break;
		case 3:
			readHoldingRegisters(sock);
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
