#ifndef __DATA_H__
#define __DATA_H__

#include <sys/types.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef struct datagram_header {
	u_short client_id;
	u_short c_say_id;
	u_char flags;
	u_int window_size;
	u_int data_length;
} DGRAM_HEADER;

u_char MSK_FIN = 1 << 0;
u_char MSK_SYN = 1 << 1;
u_char MSK_CLR = 0;

#endif
