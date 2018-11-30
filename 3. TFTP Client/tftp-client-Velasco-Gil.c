// Practica tema 7, Velasco Gil Alvaro

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

#define MAXDATASIZE 512 // Tamaño máximo de cada bloque
#define MAXFILENAME 100 // Tamaño máximo de nombre del fichero

int main(int argc, char *argv[])
{

    // COMPROBACION 1: Numero de parametros aceptable.
    // argV = {1 : activado, 2 : desactivado}
    int argV = 0;

    if (argc < 4 || argc > 5) {
        printf("Número de argumentos no válido.\n");
        exit(1);
    } else {
        if (argc == 5)  {
            // Si hay un cuarto argumento, debe ser V.
            if (strcmp(argv[4], "-v") == 0) {
                argV = 1;
            } else {
                printf("El cuarto argumento, en caso de haber, debe ser [-v].\n");
                exit(1);
            }
        }
    }

    // -- CLIENT SIDE --
    int socketClient;               // Socket del cliente.
    struct sockaddr_in clientID;    // Direccion del cliente.

    clientID.sin_family = AF_INET;
    clientID.sin_port = 0;
    clientID.sin_addr.s_addr    = INADDR_ANY;

    // Configuramos el socket del cliente.
	if ( (socketClient = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
 		perror("FAIL: No se pudo iniciar el socket del cliente.\n");
 		exit(1);
 	}

    // Enlazamos la direccion del cliente con su socket.
	if (bind(socketClient, (struct sockaddr*) &clientID, sizeof(clientID)) == -1) {
      		perror("FAIL: No se pudo enlazar el socket del Cliente.\n");
      		exit(0);
    }

    // -- SERVER SIDE --
    struct sockaddr_in serverID;
    struct servent *server;
    // Obtenemos la direccion IP del servidor
    if (inet_aton(argv[1], &serverID.sin_addr) == -1){
        perror("FAIL: No se pudo obtener la direccion IP del servidor.\n");
        exit(0);
    }
    server = getservbyname("tftp", "udp");
    serverID.sin_port   = server->s_port;
    serverID.sin_family = AF_INET;


    // COMPROBACION 2: Si estamos en lectura o escritura.
    // row = {0 : lectura, 1 : escritura}
    int row = -1;
    if (strcmp(argv[2], "-r") == 0){
        row = 0;
    } else if (strcmp(argv[2], "-w") == 0) {
        row = 1;
    } else {
        printf("FAIL: No se ha especificado bien si es lectura [-r] o escritura [-w]");
        exit(0);
    }




}
// --------------------------------------------

// El servidor está situado en 10.0.25.250
// datos-cortos.dat = 100 Bytes
// datos-justos.dat = 512 Bytes
// datos-largos.dat = 12 345 678 Bytes

//  servidor UDP
//  tftp-client ip-servidor {-r|-w} archivo [-v]
//  Si se incluye un modo visual, hay que mostrar por consola
// todos los pasos de enviar y recibir paquetes y ACKs

// -----    LECTURA DEL FICHERO     -----
// datagrama de lectura:    RRQ
//                          01filename0octet0

// operation + filename0 + (octet)

// Los nombres de los ficheros, como máx 100 caracteres.
// El cliente solicita un fichero, el servidor comprueba si existe,
// lo abre, y va enviando bloques.
// Los paquetes tienen un numero de bloque, el primero es 1,
// y cada paquete es de hasta 512 bytes.
//          03numblockData
// Cuando el cliente recibe paquete, responde un ACK con el número de bloque.
//                                              04numblock
// Cuando el servidor recibe el ACK X, envía paquete X+1
// El último bloque tiene que ser menos a 512 bytes.
// El servidor puede mandar un código de error: 05errcodeErrstring0

// -----        ESCRITURA           -----
// datagrama de escritura:  WRQ
//                          02filename0octet0
// Si esta permitida la escritura, envía al cliente un ACK con el bloque 0.
// al recibirla el cliente, se pone a enviar paquetillos.
// Cada paquete, el servidor responde con un ACK y el numero del paquete,
// que al recibirlo el cliente, envia el siguiente paquete
