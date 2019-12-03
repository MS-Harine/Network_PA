#ifndef __MODBUS_H__
#define __MODBUS_H__

#ifdef DEBUG
#define DPRINT(func) func
#else
#define DPRINT(func) ;
#endif

#define HEADER_LENGTH 8

enum FC { READ_COILS = 1, READ_HOLDING_REGISTERS = 3, WRITE_MULTIPLE_COILS = 15, WRITE_MULTIPLE_REGISTERS = 16 };

typedef struct _header {
	int transaction_id;
	int protocol_id;
	int length;
	int unit_id;
	int function_code;
} HEADER;

void readCoils(int sock);
void readHoldingRegisters(int sock);
void writeMultipleCoils(int sock);
void writeMultipleRegisters(int sock);

#endif
