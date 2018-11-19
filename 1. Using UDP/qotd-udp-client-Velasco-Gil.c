// Practica 5, Velasco Gil Alvaro

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

// Tamanyo maximo a recibir
#define MAXQUOTESIZE 512

int main(int argc, char *argv[]){

	// Aseguramos los argumentos introducidos
	if(argc<2||argc==3||argc>4) {
		printf("Ha introducido un numero de argumentos no valido.\n");
		exit(1);
	}

    // Quote enviado para conectar con el servidor
    char cadenaArbitraria[MAXQUOTESIZE] = "Cadena Arbitraria";
    // Quote recibido
	char buffer[MAXQUOTESIZE];

    // --- LOCAL ---
    // Puerto y socket
	int puerto, socketC;
    // Direccion del cliente
    struct sockaddr_in cliente;

    // --- REMOTO ---
    // Puerto
	struct servent *server;
    // Direccion del ervidor
	struct sockaddr_in servidor;

	// Estructura del cliente
	cliente.sin_family = AF_INET;
	cliente.sin_port = 0;
	cliente.sin_addr.s_addr = INADDR_ANY;
	// Iniciamos socket local
	if ( (socketC = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
 		perror("Error en el socket del Cliente\n");
 		exit(1);
 	}

    // Direccion del servidor
	servidor.sin_family = AF_INET;
	if(argc==4){
        // Comprobamos que se indica el puerto
		if(strcmp(argv[2], "-p") != 0){
			printf("Falta el tag para indicar el puerto: \'-p\' \n");
			exit(0);
		} else {
            // Conectamos con el puerto del servidor
			sscanf(argv[3], "%d", &puerto);
            // Convertimos el puerto en NetworkByteOrder
			servidor.sin_port = htons(puerto);
		}
	} else {		
        // En caso de no tener el puerto, lo obtenemos.
		server = getservbyname("qotd", "udp");
		servidor.sin_port = server->s_port;
    }
    // Iniciamos estructura de direccion para almacenar la IP
    struct in_addr direccionIP;
    // obtenemos la direccion IP del servidor
	if(inet_aton(argv[1], &direccionIP) == -1){
		perror("Error en inet_aton\n");
		exit(0);
	}

    // Asignamos la nueva direccion IP
	servidor.sin_addr = direccionIP;
	// Relacionamos el socket del cliente con su estructura
	if( bind(socketC, (struct sockaddr*)&cliente, sizeof(cliente)) == -1) {
        perror("Error al enlazar: bind(socket, estructura, tamano)\n");
        exit(0);
	}

	// Lanzamos la peticion al servidor con la cadena arbitraria
	// int sendto(int sockfd, char *buff, int nbytes,
	//				 int flags, struct sockaddr_in *dest_addr,
	// 				 socklen_t addrlen);
	if( sendto(socketC, cadenaArbitraria, MAXQUOTESIZE, 0, (struct sockaddr*)&servidor, sizeof(servidor) ) == -1){
		perror("Error al enviar la cadena arbitraria.\n");
        exit(0);
	}
	// Recibimos la cadena por parte del servidor
	if( recvfrom(socketC, buffer, MAXQUOTESIZE, 0, NULL, NULL) == -1){
		perror("Error al recibir la Quote Of The Day");
      	exit(0);
	}

    // Mostramos lo recibido y cerramos el socket
	printf("%s", buffer);
	close (socketC);

	return 0;
}
