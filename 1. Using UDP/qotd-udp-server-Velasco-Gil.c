// Practica tema 5, Velasco Gil Alvaro

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <string.h>
#include <unistd.h>

#define MAXQUOTESIZE 512
#define MAXLENGTH 100

// buffer donde se almacena la quote de Fortune
static char buffer[MAXLENGTH];
// Quote que vamos a enviar
static char finalQuote[MAXQUOTESIZE];

void generaQuote(void) {
    
    // Preparamos el encabezado del servidor
    char encabezado[MAXQUOTESIZE] = "Quote of the day from ";
    char hostname[MAXQUOTESIZE];
    gethostname(hostname, MAXQUOTESIZE);
    strcat(encabezado, hostname);
    strcat(encabezado,":\n");
    strcpy(finalQuote, encabezado);

 	// Formamos la cadena para entregar al cliente con Fortune
    system("/usr/games/fortune -s > /tmp/tt.txt");
    FILE *fich = fopen("/tmp/tt.txt","r");
    int nc = 0;
    do {
        buffer[nc++] = fgetc(fich);
    } while(nc < MAXLENGTH-1);
    fclose(fich);

    // Formamos el mensaje a enviar.
    strcat(finalQuote, buffer);
    strcat(finalQuote,"\n");
}

int main(int argc,char *argv[]){

    // Aseguramos los argumentos introducidos
    if(argc<1 || argc==2||argc>3) {
		printf("Ha introducido un número de argumentos no valido.\n");
		exit(1);
	}

    int socketS;
    int puerto;

    // Quote recibido
    char cadenaArbitraria[MAXQUOTESIZE]; //Almacenamos la cadena recibida por el cliente

    // --- SERVIDOR ---
    // Puerto
    struct servent *server;
    // Direccion
    struct sockaddr_in servidor;

    // --- Cliente ---
    // Direccion
    struct sockaddr_storage clienteIP;

    // Rellenamos la direccion del servidor
    servidor.sin_family = AF_INET;
    if(argc == 3){
        if(strcmp(argv[1],"-p") != 0){
			printf("Falta el tag para indicar el puerto: \'-p\' \n");
			exit(0);
		} else {
            // Guardamos el puerto
		    sscanf ( argv[2], "%d", &puerto);
            // Convertimos el puerto a NetworkByteOrder
		    servidor.sin_port = htons(puerto);
        }
    } else {
        // Obtenemos el puerto a traves del nombre
	    server = getservbyname ("qotd","udp");
		servidor.sin_port = server->s_port;
	}
    // Apuntamos la direccion donde se ejecuta el servidor
    servidor.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Iniciamos el socket del Servidor
    if ( (socketS = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
 		perror("socket()");
 		exit(1);
 	}
    // Relacionamos el socket del servidor con su estructura
    if(bind(socketS, (struct sockaddr*)&servidor,sizeof(servidor)) == -1) {
      	perror("bind()");
    	exit(0);
	}

    //Tamaño direccion del cliente.
    socklen_t clientSize = sizeof clienteIP;


    // Bucle infinito para que el servidor se quede esperando peticiones.
    while(1 != 0){
        // Esperamos la petición del cliente
        if( recvfrom(socketS, cadenaArbitraria, MAXQUOTESIZE, 0, (struct sockaddr*)&clienteIP, &clientSize) == -1){
  		    perror("recvfrom()");
            exit(0);
      	}
        // Generamos una nueva quote
        generaQuote();

        // Enviamos la QOTD al cliente
        if( sendto(socketS, (char *)&finalQuote, sizeof(finalQuote), 0, (struct sockaddr *)&clienteIP, clientSize)==-1){
            perror("sendto()");
            exit(0);
        }
    }
    
    // Cerramos el socket, aunque no saldremos del bucle.
    close(socketS);
    return 0;
}
