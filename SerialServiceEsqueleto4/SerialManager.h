
#define PORT_EMULADOR 1
#define PORT_BAUDRATE 115200
#define SERIAL_MAX_CHARS 20
#define INIT_CHAR_0 0
#define INIT_CHAR_1 1
#define INIT_CHAR_2 2
#define DATAIN_CHAR 4
#define DATAIN_ESTATE 6
#define SERIAL_OUTPUT_NUMBER_CHARS 15
#define SERIAL_OUTPUT_INIT ">OUT:0,0\r\n"
#define SERIAL_OUTPUT_NUMBER_LAMP 5
#define SERIAL_OUTPUT_ESTATE_LAMP 7
#define LAMP0 0 
#define LAMP1 1 
#define LAMP2 2 
#define TCP_MSG ">SW:X,Y\r\n"
#define TCP_MSG_LENGTH sizeof(TCP_MSG)
#define DATAOUT_CHAR 4
#define DATAOUT_ESTATE 6
#define TCP_MAX_CHARS 128
#define TCP_PORT 10000
#define SERVER_TCP_ADDRESS "127.0.0.1"
#define NUMBER_OF_LAMPS 3
#define INVALID_FD -1


int serial_open(int pn, int baudrate);
void serial_send(char *pData, int size);
void serial_close(void);
int serial_receive(char *buf, int size);



