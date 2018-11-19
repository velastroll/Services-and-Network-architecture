// Practica tema 6, Velasco Gil Alvaro

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <unistd.h>
#include <string.h>

// Tamanno maximo a recibir
#define MAXQUOTESIZE 512

int main(int argc, char *argv[]){

	// Aseguramos los argumentos introducidos
	if(argc<2||argc==3||argc>4) {
		printf("Ha introducido un número de argumentos no valido.\n");
		exit(1);
	}

    // Quote recibido
	char buffer[MAXQUOTESIZE];

    // --- LOCAL ---  
    // puerto y socket
	int puerto, socketC;


    // --- REMOTO ---
    // Puerto
    struct servent *portS;
    // Direccion del servidor
	struct sockaddr_in servidor;

	// Iniciamos socket local
	if ( (socketC = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
 		perror("socket()");
 		exit(1);
 	}
	// Direccion del servidor
	servidor.sin_family = AF_INET;
	if (argc == 4){
        // Comprobamos que se indica el puerto
		if(strcmp(argv[2],"-p") != 0){
			printf("Falta el tag para indicar el puerto: \'-p\' \n");
			exit(0);
		}
		else{
            // Conectamos con el puerto del servidor
			sscanf ( argv[3],"%d",&puerto);
            // Convertimos el puerto en NetworkByteOrder
			servidor.sin_port = htons(puerto);
		}
	} else {	
        // En caso de no tener el puerto, lo obtenemos.
		portS = getservbyname ("qotd", "tcp");
		servidor.sin_port = portS->s_port;
	}
	// Iniciamos una estructura de direccion para almacenar la IP
	struct in_addr direccionIP;
    // Obtenemos la direccion IP del servidor
	if (inet_aton(argv[1], &direccionIP) == -1){
		perror("Error en inet_aton\n");
		exit(0);
	}

    // Asignamos la direccion IP al servidor
	servidor.sin_addr=direccionIP;
    // Establecemos la conexión del socket cliente con el servidor
	if (connect(socketC, (struct sockaddr*) &servidor, sizeof(struct sockaddr) )== -1) {
        perror("FAIL: No se ha podido iniciar la conexión con el servidor.");
        exit(0);
	}

	// Recibimos el quote del servidor
	if(recv(socketC, buffer, MAXQUOTESIZE, 0) == -1){
		perror("FALLO: Error al recibir la Quote Of The Day");
        exit(0);
	}
	
	// Mostramos lo recibido y cerramos el socket
	printf("%s", buffer);
	close(socketC);

	return 0;
}
