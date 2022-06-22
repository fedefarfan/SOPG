#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "SerialManager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>


/*Variable global donde se gurada el estados de las lamparas
posicion 0 LAMPARA UNO, posicion 1 LAMPARA DOS y posicion 2 
LAMPARA TRES 
Estos valores se modifican por informacion que viene del 
Emulador del puerto serie o la página
*/
char lampValueState[NUMBER_OF_LAMPS] = {0, 0, 0};

pthread_mutex_t mutexLineState = PTHREAD_MUTEX_INITIALIZER;
int newfd = INVALID_FD;
int checkOutSignal = 0;

void recibiSignal(int sig)
{
	if ((sig == SIGINT) || (sig = SIGTERM)){
		checkOutSignal = 1;
	}
	else{
		checkOutSignal = 0;
	}
}

void bloquearSign(void){
	sigset_t set;
	int s;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
}

void desbloquearSign(void){
	sigset_t set;
	int s;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);
}

void *tpcInterface(void *param){
	socklen_t addr_len;
	struct sockaddr_in clientaddr;
	struct sockaddr_in serveraddr;
	char buffer[TCP_MAX_CHARS];
	int n;

	/* Creamos socket*/
	int s = socket(PF_INET, SOCK_STREAM, 0);

	/* Cargamos los datos para configurar el IP:PORT del server*/
	bzero((char *)&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(TCP_PORT);
	serveraddr.sin_addr.s_addr = inet_addr(SERVER_TCP_ADDRESS);
	
	if (serveraddr.sin_addr.s_addr == INADDR_NONE){
		printf("%s: ", (const char *)param);
		fprintf(stderr, "ERROR invalid server IP\r\n");
		exit(EXIT_FAILURE);
	}
	else{
		printf("%s: ", (const char *)param);
		printf("Socket successfully created..\n");
	}

	/*Se abre el puerto con bind()*/
	if (bind(s, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1){
		close(s);
		perror("listener: bind\r\n");
		exit(EXIT_FAILURE);
	}
	else{
		printf("%s: ", (const char *)param);
		printf("Socket successfully binded..\n");
	}

	/* Seteamos socket en modo Listening*/
	if (listen(s, 10) == -1){ 
	
		perror("error en listen\r\n");
		exit(EXIT_FAILURE);
	}
	else{
		printf("%s: ", (const char *)param);
		printf("Server listening..\n");
	}

	while (1){
		/* Ejecutamos accept() para recibir conexiones entrantes*/
		addr_len = sizeof(struct sockaddr_in);
		if ((newfd = accept(s, (struct sockaddr *)&clientaddr, &addr_len)) == -1){
			perror("error en accept\r\n");
			exit(EXIT_FAILURE);
		}
		else{
			printf("%s: ", (const char *)param);
			printf("server accept the client...\n");
		}

		printf("%s: ", (const char *)param);
		printf("conexion desde:  %s -> ", inet_ntoa(clientaddr.sin_addr));
		printf("Puerto: %d -> ", TCP_PORT);
		printf("FileDescriptor: %d\r\n", newfd);

		do{
			/* Leemos mensaje del cliente bzero(buffer, TCP_MAX_CHARS);*/
			if ((n = read(newfd, buffer, TCP_MAX_CHARS)) < 0){
				perror("Error leyendo mensaje en socket\r\n");
				exit(EXIT_FAILURE);
			}
			else if (n == 0){
				break;
			}
			else{
				buffer[n] = 0;
				printf("%s: ", (const char *)param);
				printf("Int. Status Recibido: %s", buffer);
			}

			pthread_mutex_lock(&mutexLineState);
			/* Si existe un cambio en la web, modifica el estado de la variable
			global que guarda el estado de todas las lamparas ">OUT:0,0\r\n"*/
			if(buffer[SERIAL_OUTPUT_ESTATE_LAMP]=='0'){//SERIAL_OUTPUT_ESTATE_LED 7

				switch(buffer[SERIAL_OUTPUT_NUMBER_LAMP]){
				case '0':	
				lampValueState[LAMP0] = '0';
				break;
				case '1':
				lampValueState[LAMP1] = '0';
				break;
				case '2':
				lampValueState[LAMP2] = '0';
				break;
				default:
				break;
				}
				}else{
				switch(buffer[SERIAL_OUTPUT_NUMBER_LAMP]){//">OUT:0,0\r\n"
				case '0':	
				lampValueState[LAMP0] = '1';
				break;
				case '1':
				lampValueState[LAMP1] = '1';
				break;
				case '2':
				lampValueState[LAMP2] = '1';
				break;
				default:
				break;
				}
				}


			pthread_mutex_unlock(&mutexLineState);

		} while (buffer[0] != 0);

		// Cerramos conexion con cliente
		if (close(newfd) < 0){
			printf("%s: ", (const char *)param);
			printf("No se pudo cerrar el socket TCP\r\n");
			exit(EXIT_FAILURE);
		}
		else{
			newfd = INVALID_FD;
			printf("%s: ", (const char *)param);
			printf("Conexion con Cliente perdida, reintentando...\r\n");
		}
	}
	exit(EXIT_FAILURE);
}

void *serialInterface(void *param){

	int serialPort, serialStatus;
	int initPort = 0;
	char serialBufferIn[SERIAL_MAX_CHARS];
	char serialBufferOut[SERIAL_MAX_CHARS] = SERIAL_OUTPUT_INIT;
	
	/*Variable local donde se gurada el estados de las lamparas
	posicion 0 LAMPARA UNO, posicion 1 LAMPARA DOS y posicion 2 
	LAMPARA TRES 
	*/
	char lampState[NUMBER_OF_LAMPS] = {0, 0, 0};
	char tcpFrame[TCP_MAX_CHARS] = TCP_MSG;

	/* Abre el puerto serie del Emulador*/
	if ((serialPort = serial_open(PORT_EMULADOR, PORT_BAUDRATE))!=0){

	printf("Error al abrir puerto serie\r\n");
	}

	while (serialPort == 0){

		if (initPort == 0){

			printf("%s: ", (const char *)param);
			printf("Puerto serie inicializado -> ");
			printf("serialPort: %d\r\n", serialPort);
			initPort = 1;
		}

		/*En la variable serialBufferIn se recibe el comando enviado por 
		el Emulador con formato: “>SW:X,Y\r\n”*/

		serialStatus = serial_receive(serialBufferIn, SERIAL_MAX_CHARS);
		if (serialStatus != -1 && serialStatus != 0){

			/*Control de la variable que envia el Emulador*/
			if ((serialBufferIn[INIT_CHAR_0] == '>')&&(serialBufferIn[INIT_CHAR_1] == 'S')&&(serialBufferIn[INIT_CHAR_2] == 'W')){
				printf("%s: ", (const char *)param);
				printf("Comando valido recibido: %.*s -> ", serialStatus - 2, serialBufferIn);
				printf("serialStatus: %d\r\n", serialStatus);

				pthread_mutex_lock(&mutexLineState);
			
			/*Se modifica el estado de la lampara de la trama serialBufferOut 
			de acuerdo a serialBufferIn */
				if(serialBufferIn[DATAIN_ESTATE]=='0'){
					serialBufferOut[SERIAL_OUTPUT_ESTATE_LAMP] = '0';
				}
				else{
					serialBufferOut[SERIAL_OUTPUT_ESTATE_LAMP]='1';
				}
				pthread_mutex_unlock(&mutexLineState);

			/*Se modifica el numero de lampara de la trama serialBufferOut 
			de acuerdo a serialBufferIn */
				switch(serialBufferIn[DATAIN_CHAR]){
				case '0':	
				serialBufferOut[SERIAL_OUTPUT_NUMBER_LAMP] = '0';
				break;
				case '1':
				serialBufferOut[SERIAL_OUTPUT_NUMBER_LAMP] = '1';
				break;
				case '2':
				serialBufferOut[SERIAL_OUTPUT_NUMBER_LAMP] = '2';
				break;
				default:
				break;
				}


				serial_send(serialBufferOut, SERIAL_OUTPUT_NUMBER_CHARS);
				printf("%s: ", (const char *)param);
				printf("Comando enviado a EMULADOR de UART: %.*s", SERIAL_OUTPUT_NUMBER_CHARS, serialBufferOut);

				if (newfd != INVALID_FD){
				/*Se modifica en la trama el numero de lampara y su estado de acuerdo a
			    de acuerdo a serialBufferIn */
					tcpFrame[DATAOUT_CHAR] = serialBufferIn[DATAIN_CHAR];//>SW:X,Y
					tcpFrame[DATAOUT_ESTATE] = serialBufferIn[DATAIN_ESTATE];
					
					if (write(newfd, tcpFrame, TCP_MSG_LENGTH) == -1){
						perror("Error escribiendo mensaje en socket\r\n");
						exit(EXIT_FAILURE);
					}
					printf("%s: ", (const char *)param);
					printf("Comando enviado a Web originado en UART: %.*s", (int)TCP_MSG_LENGTH, tcpFrame);
				}
			}/*Si recibo otra informacion:*/
			else if (serialBufferIn[INIT_CHAR_1] == 'O'){
				printf("%s: ", (const char *)param);
				printf("ACK recibido: %.*s -> ", serialStatus - 2, serialBufferIn);
				printf("serialStatus: %d\r\n", serialStatus);
			}
		}
		else{
			/*Actualiza el estado de los leds al producirse cambios por accion de la 
			comunicacion TCP*/
			int change = 0;
			

			for (int i = 0; i < NUMBER_OF_LAMPS; i++){
				pthread_mutex_lock(&mutexLineState);
				if (lampState[i] != lampValueState[i])
				{
					lampState[i] = lampValueState[i];
					
				switch(i){
				case 0:	
				serialBufferOut[SERIAL_OUTPUT_NUMBER_LAMP] = '0';
				printf("Valor de los estados de las LAMPARAS %s\r\n",lampValueState);
				break;
				case 1:
				serialBufferOut[SERIAL_OUTPUT_NUMBER_LAMP] = '1';
				printf("Valor de los estados de las LAMPARAS %s\r\n",lampValueState);
				break;
				case 2:
				serialBufferOut[SERIAL_OUTPUT_NUMBER_LAMP] = '2';
				printf("Valor de los estados de las LAMPARAS %s\r\n",lampValueState);
				break;
				default:
				break;
				}
				serialBufferOut[SERIAL_OUTPUT_ESTATE_LAMP] = lampState[i] ;
					
					change = 1;
				}
				pthread_mutex_unlock(&mutexLineState);
			}
			
			if (change == 1){
				serial_send(serialBufferOut, SERIAL_OUTPUT_NUMBER_CHARS);
				printf("%s: ", (const char *)param);
				printf("Comando enviado a EMULADOR originado en TCP: %.*s", SERIAL_OUTPUT_NUMBER_CHARS, serialBufferOut);
			}
				
		}
		sleep(1);
	}

	printf("%s: ", (const char *)param);
	printf("Puerto serie no pudo ser inicializado -> ");
	printf("serialPort: %d\r\n", serialPort);
	exit(EXIT_FAILURE);
}

int main(void)
{
	int serialThreadCheck, tcpThreadCheck;
	const char *message1 = "Thread Serie";
	const char *message2 = "Thread TCP";
	struct sigaction sa;
	pthread_t serialThread, tcpThread;
	printf("Main: Inicio de Serial Service\r\n");

	
	sa.sa_handler = recibiSignal;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGINT, &sa, NULL) == -1){
		perror("Main: sigaction\r\n");
		exit(EXIT_FAILURE);
	}
	if (sigaction(SIGTERM, &sa, NULL) == -1){
		perror("Main: sigaction\r\n");
		exit(EXIT_FAILURE);
	}

	/* Creacion de threads para comunicacion serial*/
	bloquearSign();
	serialThreadCheck = pthread_create(&serialThread, NULL, serialInterface, (void *)message1);
	if (serialThreadCheck != 0){
		printf("Main: Error al crear el thread del puerto serie\r\n");
		exit(EXIT_FAILURE);
	}
	printf("Main: Thread del puerto serie creado\n\r");
	
	/* Creacion de threads para comunicacion TCP*/
	tcpThreadCheck = pthread_create(&tcpThread, NULL, tpcInterface, (void *)message2);
	if (tcpThreadCheck != 0){
		printf("Main: Error al crear el thread del TCP\r\n");
		exit(EXIT_FAILURE);
	}
	printf("Main: Thread TCP creado\r\n");
	desbloquearSign();

	
	while (1)
	{
		if (checkOutSignal == 1)
		{
			serial_close();
			printf("Main: Cierre del puerto Serie\r\n");

			if (newfd != INVALID_FD)
			{
				if (close(newfd) < 0)
				{
					printf("Main: No se pudo cerrar el socket TCP\r\n");
					exit(EXIT_FAILURE);
				}
				else
				{
					printf("Main: Se cierra el Socket TCP\r\n");
				}
			}

			if (pthread_cancel(serialThread) != 0)
			{
				printf("Main: No se pudo cancelar el thread Serie\r\n");
				exit(EXIT_FAILURE);
			}
			else
			{
				printf("Main: Cancela el thread Serie\r\n");
			}

			if (pthread_cancel(tcpThread) != 0)
			{
				printf("Main: No se puede cancelar el thread TCP\r\n");
				exit(EXIT_FAILURE);
			}
			else
			{
				printf("Main: Cancela el thread TCP\r\n");
			}

			if (pthread_join(serialThread, NULL) != 0)
			{
				printf("Main: No se pudo hacer el Join del thread Serie\r\n");
				exit(EXIT_FAILURE);
			}
			else
			{
				printf("Main: Join del thread Serie\r\n");
			}

			if (pthread_join(tcpThread, NULL) != 0)
			{
				printf("Main: No se pudo hacer el Join del thread TCP\r\n");
				exit(EXIT_FAILURE);
			}
			else
			{
				printf("Main: Join del thread TCP\r\n");
			}
			exit(EXIT_SUCCESS);
		}
		sleep(1);
	}
	exit(EXIT_FAILURE);
}
