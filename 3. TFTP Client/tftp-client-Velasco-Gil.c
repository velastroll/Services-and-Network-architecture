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
    } else if (argc == 5)  {
        // Si hay un cuarto argumento, debe ser V.
        if (strcmp(argv[4], "-v") == 0) {
            argV = 1;
        } else {
            printf("El cuarto argumento, en caso de haber, debe ser [-v].\n");
            exit(1);
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
    // Obtenemos la direccion IP del servidor.
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

    // Cadenas y puntero necesarios:
    char pack[MAXDATASIZE]; // Datagrama para enviar y recibir bloques.
    FILE *file; // Fichero recibido o enviado.


    // Variables de control:
    int block       = 1;    // Bloque usado.
    int rcvBlock;           // Bloque recibido.
    int operationCode;      // Número de operacion = {01, 02, 03, 04, 05}
    int errorCode;          // Número de error = {1, ..., 7}
    int packSize;           // Tamaño de paquete a enviar.

    // -----  Iniciamos el modo lectura   -----
    if (row==0){
        // Construimos la pedición de lectura RRQ:
        // peticion = 01filenameoctet
        pack[0] = 0;    pack[1] = 1;    // 01
        strcpy(pack + 2, argv[3]);      // 01filename
        strcpy(pack + (strlen(argv[3]) + 3), "0");      // 01filename0
        strcpy(pack + (strlen(argv[3]) + 4), "octet");  // 01filename0octet
        strcpy(pack + (strlen(argv[3]) + strlen("octet") + 5), "0");     // 01filename0octet0

        // Abrimos el archivo de datos a enviar:
        if ((file = fopen(argv[3], "w")) == NULL){
            perror("FAIL: No se pudo abrir el fichero.\n");
            exit(0);
        }

        // Calculamos el tamaño del paquete con la petición:
        packSize = 2 + strlen(argv[3]) + 1 + strlen("octet") + 1;

        // Enviamos la petición:
        if (sendto(socketClient, pack, sizeof(char)*packSize, 0, (struct sockaddr*) &serverID, sizeof(serverID)) == -1) {
            perror("FAIL: No se pudo enviar la petición al servidor.\n");
            exit(0);
        }

        // Si se solicitó [-v], imprimimos información sobre la petición:
        if (argV == 1) {
            printf("Enviada solicitud de lectura de %s a servidor tftp en %s.", argv[3], argv[1]);
        }

        // Manejamos al respuesta por parte del servidor:
        int rcvEnd = 0; // Controlador de bucle, al recibir el último, rcvEnd = 1;
        int size;       // Tamaño del paquete recibido.
        while(rcvEnd==0){
            socklen_t serverSize = sizeof(serverID);
            if ( (size = recvfrom(socketClient, pack, MAXDATASIZE, 0, (struct sockaddr *) &serverID, &serverSize)) == -1){
                perror("FAIL: Error recibiendo el paquete del Servidor.\n");
                exit(0);
            } else if (argV == 1) {
                printf("Recibido bloque del servidor tftp.\n");
            }

            // Comprobanos el número de bloque:
            if (argV == 1) {
                if (block == 1) {
                    printf("Es el primer bloque (numero de bloque %d).\n", block);
                } else {
                    printf("Es el bloque con codigo $d.", block);
                }
            }

            // Comprobamos que el número del bloque recibido en el paquete es el mismo que el esperado.
            rcvBlock = (unsigned char) pack[2] * 256 + (unsigned char) pack[3];
            if (rcvBlock != block) {
                printf("FAIL: Se recibió el bloque %d, mientras se esperaba el %d", rcvBlock, block);
                exit(0);
            }

            // Comprobamos el tipo de operacion: 5 = error.
            operationCode = pack[1];
            if (operationCode == 5) {   // [05][errorCode][errorString][0]
                errorCode = pack[3];
                switch(errorCode){
                    case 0 :
                        printf("No definido: %s\n", &pack[4]);
                        exit(0);
                        break;
                    case 1 :
                        printf("Fichero no encontrado.\n");
                        exit(0);
                        break;
                    case 2 :
                        printf("Violación de acceso.\n");
                        exit(0);
                        break;
                    case 3 :
                        printf("Espacio de almacenamiento lleno.\n");
                        exit(0);
                        break;
                    case 4 :
                        printf("Operación TFTP ilegal.\n");
                        exit(0);
                        break;
                    case 5 :
                        printf("Identificador de transferencia desconocido.\n");
                        exit(0);
                        break;
                    case 6 :
                        printf("El fichero ya existe.\n");
                        exit(0);
                        break;
                    case 7 :
                        printf("Usuario desconocido.\n");
                        exit(0);
                        break;
                }
            }


            // Enviamos el ACK de 
        }


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
