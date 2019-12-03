#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "modbus.h"

static int trans_id = 0;

void string2hex(char *str) {
	int i = 0, len = strlen(str);
	for (i = 0; i < len; i++)
		str[i] = isdigit(str[i]) ? str[i] - '0' : str[i] - 'A' + 10;
	
	for (i = 0; i < len / 2; i++)
		str[i] = str[i * 2] * 16 + str[i * 2 + 1];
}

int hex2int(char *arr, int n) {
	int i = 0, result = 0;
	for (i = 0; i < n; i++)
		result += arr[i] * (int)pow(256.0, n - i - 1);
	return result;
}

HEADER * make_header(int length, enum FC function_code) {
	HEADER *header = malloc(sizeof(HEADER));
	header->transaction_id = trans_id;
	header->protocol_id = 0;
	header->length = length;
	header->unit_id = 0;
	header->function_code = function_code;
	return header;
}

char * make_data(HEADER *header, const char *data) {
	char *data_with_header = NULL;

	data_with_header = malloc(sizeof(char) * ((HEADER_LENGTH + header->length) * 2 + 1));
	memset(data_with_header, 0, sizeof(char) * ((HEADER_LENGTH + header->length) * 2 + 1));
	sprintf(data_with_header, "%04X%04X%04X%02X%02X", header->transaction_id,
													  header->protocol_id,
													  header->length + 2,
													  header->unit_id,
													  header->function_code);
	strcat(data_with_header, data);
	return data_with_header;
}

void _readCoils(int sock, int start_address, int num_of_coils) {
	HEADER *header = NULL;
	char *data = NULL, *data_with_header = NULL, *receive_buffer = NULL;
	int is_coil_on = 0, code = 0, data_len = 4, i = 0;

	header = make_header(data_len, READ_COILS);
	data = malloc(sizeof(char) * (data_len * 2 + 1));
	sprintf(data, "%04X%04X", start_address, num_of_coils);
	
	data_with_header = make_data(header, data);
	data_len = strlen(data_with_header) / 2;
	string2hex(data_with_header);

	DPRINT(
		printf("====== Sending data ======\n");
		for (i = 0; i < data_len; i++) {
			printf("%d ", data_with_header[i]);
			if ((i + 1) % 4 == 0)
				printf("\n");
		}
		if (i % 4 != 0) printf("\n");
		printf("==========================\n");
	)

	send(sock, data_with_header, data_len, 0);
	free(data);
	free(data_with_header);
	free(header);
	
	data = malloc(sizeof(char) * (HEADER_LENGTH + 1));
	recv(sock, data, HEADER_LENGTH + 1, 0);

	trans_id = hex2int(data, 2);
	code = hex2int(data + HEADER_LENGTH - 1, 1);
	data_len = hex2int(data + HEADER_LENGTH, 1);

	if (code != READ_COILS) {
		printf("\nExceptions on Read Coils\n");
		printf("Exception code: %d\n\n", data_len);
		free(data);
		return;
	}
	
	receive_buffer = malloc(sizeof(char) * data_len);
	recv(sock, receive_buffer, data_len, 0);
	
	printf("\n");
	for (i = 0; i < num_of_coils; i++) {
		is_coil_on = receive_buffer[i / 8] & (1 << i % 8);
		printf("Coil #%d: %s\n", start_address + 1 + i, is_coil_on ? "On" : "Off");
	}
	printf("\n");

	DPRINT(
		printf("====== Receiving data ======\n");
		for (i = 0; i < HEADER_LENGTH + 1; i++) {
			printf("%d ", data[i]);
			if ((i + 1) % 4 == 0)
				printf("\n");
		}
		for (; i < HEADER_LENGTH + data_len + 1; i++) {
			printf("%d ", receive_buffer[i - (HEADER_LENGTH + 1)]);
			if ((i + 1) % 4 == 0)
				printf("\n");
		}
		if (i % 4 != 0) printf("\n");
		printf("============================\n");
	)
	
	free(data);
	free(receive_buffer);
}

void readCoils(int sock) {
	int start_address = 0, num_of_coils = 0;
	
	printf("Enter the start address: ");
	scanf("%d", &start_address);

	printf("Enter the number of coils to be read: ");
	scanf("%d", &num_of_coils);

	_readCoils(sock, start_address, num_of_coils);
}

void _readHoldingRegisters(int sock, int start_address, int num_of_registers) {
	HEADER *header = NULL;
	char *data = NULL, *data_with_header = NULL, *receive_buffer = NULL;
	int code = 0, data_len = 4, i = 0, j = 0;
	unsigned int value = 0;

	header = make_header(data_len, READ_HOLDING_REGISTERS);
	data = malloc(sizeof(char) * (data_len * 2 + 1));
	sprintf(data, "%04X%04X", start_address, num_of_registers);
	
	data_with_header = make_data(header, data);
	data_len = strlen(data_with_header) / 2;
	string2hex(data_with_header);

	DPRINT(
		printf("====== Sending data ======\n");
		for (i = 0; i < data_len; i++) {
			printf("%d ", data_with_header[i]);
			if ((i + 1) % 4 == 0)
				printf("\n");
		}
		if (i % 4 != 0) printf("\n");
		printf("==========================\n");
	)

	send(sock, data_with_header, data_len, 0);
	free(data);
	free(data_with_header);
	free(header);
	
	data = malloc(sizeof(char) * (HEADER_LENGTH + 1));
	recv(sock, data, HEADER_LENGTH + 1, 0);

	trans_id = hex2int(data, 2);
	code = hex2int(data + HEADER_LENGTH - 1, 1);
	data_len = hex2int(data + HEADER_LENGTH, 1);

	if (code != READ_HOLDING_REGISTERS) {
		printf("\nExceptions on Read Holding Registers\n");
		printf("Exception code: %d\n\n", data_len);
		free(data);
		return;
	}
	
	receive_buffer = malloc(sizeof(char) * data_len);
	recv(sock, receive_buffer, data_len, 0);
	
	printf("\n");
	for (i = 0; i < num_of_registers; i++) {
		value = 0;
		for (j = 0; j < 8; j++)
			value += receive_buffer[i * 2] & (1 << j);
		value *= 256;
		for (j = 0; j < 8; j++)
			value += receive_buffer[i * 2 + 1] & (1 << j);

		printf("Register #%d: %d\n", start_address + 1 + i, value);
	}
	printf("\n");

	DPRINT(
		printf("====== Receiving data ======\n");
		for (i = 0; i < HEADER_LENGTH + 1; i++) {
			printf("%d ", data[i]);
			if ((i + 1) % 4 == 0)
				printf("\n");
		}
		for (; i < HEADER_LENGTH + data_len + 1; i++) {
			printf("%d ", receive_buffer[i - (HEADER_LENGTH + 1)]);
			if ((i + 1) % 4 == 0)
				printf("\n");
		}
		if (i % 4 != 0) printf("\n");
		printf("============================\n");
	)
	
	free(data);
	free(receive_buffer);
}

void readHoldingRegisters(int sock) {
	int start_address = 0, num_of_registers = 0;

	printf("Enter the start address: ");
	scanf("%d", &start_address);

	printf("Enter the number of registers to be read: ");
	scanf("%d", &num_of_registers);

	_readHoldingRegisters(sock, start_address, num_of_registers);
}

void _writeMultipleCoils(int sock, int start_address, int num_of_coils, char *values) {
	HEADER *header = NULL;
	char *data = NULL, *data_with_header = NULL;
	char coils_value = 0;
	int byte_count = 0, data_len = 0, code = 0, i = 0, j = 0;

	byte_count = (num_of_coils + 7) / 8;

	header = make_header(5 + byte_count, WRITE_MULTIPLE_COILS);
	data = malloc(sizeof(char) * ((5 + byte_count) * 2 + 1));
	memset(data, 0, sizeof(char) * ((5 + byte_count) * 2));
	sprintf(data, "%04X%04X%02X", start_address, num_of_coils, byte_count);
	
	for (i = 0; i < (strlen(values) + 7) / 8; i++) {
		coils_value = 0;
		for (j = 0; j < 8 && (i * 8 + j) < strlen(values); j++)
			coils_value |= (values[i * 8 + j] - '0' ? 1 : 0) << j;
		sprintf(data + strlen(data), "%02X", coils_value);
	}

	data_with_header = make_data(header, data);
	data_len = strlen(data_with_header) / 2;
	string2hex(data_with_header);
	
	DPRINT(
		printf("====== Sending data ======\n");
		for (i = 0; i < data_len; i++) {
			printf("%d ", data_with_header[i]);
			if ((i + 1) % 4 == 0)
				printf("\n");
		}
		if (i % 4 != 0) printf("\n");
		printf("==========================\n");
	)
	
	send(sock, data_with_header, data_len, 0);
	free(data);
	free(data_with_header);
	free(header);

	data = malloc(sizeof(char) * (HEADER_LENGTH + 4));
	memset(data, 0, sizeof(char) * (HEADER_LENGTH + 4));
	recv(sock, data, sizeof(char) * (HEADER_LENGTH + 4), 0);
	
	trans_id = hex2int(data, 2);
	code = hex2int(data + HEADER_LENGTH - 1, 1);
	if (code != WRITE_MULTIPLE_COILS) {
		data_len = hex2int(data + HEADER_LENGTH, 1);
		printf("\nExceptions on Write Multiple Coils\n");
		printf("Exception code: %d\n\n", data_len);
		free(data);
		return;
	}
	
	start_address = hex2int(data + HEADER_LENGTH, 2);
	num_of_coils = hex2int(data + HEADER_LENGTH + 2, 2);
	
	printf("\nCoils are updated from #%d to #%d\n\n", start_address + 1, start_address + num_of_coils);

	DPRINT(
		printf("====== Receiving data ======\n");
		for (i = 0; i < HEADER_LENGTH + 4; i++) {
			printf("%d ", data[i]);
			if ((i + 1) % 4 == 0)
				printf("\n");
		}
		if (i % 4 != 0) printf("\n");
		printf("============================\n");
	)
	
	free(data);
}

void writeMultipleCoils(int sock) {
	int start_address = 0, num_of_coils = 0, i = 0;
	char *value = NULL;

	printf("Enter the start_address: ");
	scanf("%d", &start_address);

	printf("Enter the number of coils to be write: ");
	scanf("%d", &num_of_coils);

	value = malloc(sizeof(char) * (num_of_coils + 1));
	memset(value, 0, sizeof(char) * (num_of_coils + 1));
	for (i = 0; i < num_of_coils; i++) {
		printf("Enter the value of coil #%d (0 or 1): ", start_address + 1 + i);
		scanf(" %c", &value[i]);
	}

	_writeMultipleCoils(sock, start_address, num_of_coils, value);
	free(value);
}

void _writeMultipleRegisters(int sock, int start_address, int num_of_registers, int *values) {
	HEADER *header = NULL;
	int byte_count = 0, data_len = 0, code = 0, i = 0;
	char *data = NULL, *data_with_header = NULL;

	byte_count = num_of_registers * 2;
	header = make_header(5 + byte_count, WRITE_MULTIPLE_REGISTERS);
	
	data = malloc(sizeof(char) * ((5 + byte_count) * 2 + 1));
	memset(data, 0, sizeof(char) * ((5 + byte_count) * 2 + 1));
	sprintf(data, "%04X%04X%02X", start_address, num_of_registers, byte_count);
	for (i = 0; i < num_of_registers; i++)
		sprintf(data + strlen(data), "%04X", values[i]);

	data_with_header = make_data(header, data);
	data_len = strlen(data_with_header) / 2;
	string2hex(data_with_header);
	
	DPRINT(
		printf("====== Sending data ======\n");
		for (i = 0; i < data_len; i++) {
			printf("%d ", data_with_header[i]);
			if ((i + 1) % 4 == 0)
				printf("\n");
		}
		if (i % 4 != 0) printf("\n");
		printf("==========================\n");
	)

	send(sock, data_with_header, data_len, 0);
	free(data);
	free(data_with_header);
	free(header);

	data = malloc(sizeof(char) * (HEADER_LENGTH + 4));
	memset(data, 0, sizeof(char) * (HEADER_LENGTH + 4));
	recv(sock, data, sizeof(char) * (HEADER_LENGTH + 4), 0);

	trans_id = hex2int(data, 2);
	code = hex2int(data + HEADER_LENGTH - 1, 1);
	if (code != WRITE_MULTIPLE_REGISTERS) {
		data_len = hex2int(data + HEADER_LENGTH, 1);
		printf("\nExceptions on Write Multiple Registers\n");
		printf("Exception code: %d\n\n", data_len);
		free(data);
		return;
	}
	
	start_address = hex2int(data + HEADER_LENGTH, 2);
	num_of_registers = hex2int(data + HEADER_LENGTH + 2, 2);
	
	printf("\nRegisters are updated from #%d to #%d\n\n", start_address + 1, start_address + num_of_registers);

	DPRINT(
		printf("====== Receiving data ======\n");
		for (i = 0; i < HEADER_LENGTH + 4; i++) {
			printf("%d ", data[i]);
			if ((i + 1) % 4 == 0)
				printf("\n");
		}
		if (i % 4 != 0) printf("\n");
		printf("============================\n");
	)
	
	free(data);
}

void writeMultipleRegisters(int sock) {
	int start_address = 0, num_of_registers = 0, i = 0;
	int *values = NULL;
	
	printf("Enter the start_address: ");
	scanf("%d", &start_address);
	
	printf("Enter the number of registers: ");
	scanf("%d", &num_of_registers);

	values = malloc(sizeof(int) * num_of_registers);
	for (i = 0; i < num_of_registers; i++) {
		printf("Enter the value of register #%d: ", start_address + 1 + i);
		scanf("%d", &values[i]);
	}

	_writeMultipleRegisters(sock, start_address, num_of_registers, values);
	free(values);
}
