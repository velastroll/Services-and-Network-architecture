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
        printf("FAIL: No se ha especificado bien si es lectura [-r] o escritura [-w].\n");
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
    //Se guarda el nombre del fichero
    char filename[100];
    strcpy(filename, argv[3]);

    // -----    Iniciamos el modo LECTURA   -----
    if (row == 0) {
        // Construimos la petición de lectura RRQ.
        pack[0]=0;  pack[1]=1;  // Read = 01
        strcpy(&pack[2], filename);                 // Se añade el nombre del fichero
        strcpy(&pack[strlen(filename)+3],"octet");  // Se añade el modo "octet"

        // Abrimos el archivo de datos a enviar:
        if ((file = fopen(argv[3], "w")) == NULL){
            perror("FAIL: No se pudo abrir el fichero.\n");
            exit(0);
        }

        // Calculamos el tamaño del datagrama UDP como petición:
        packSize = 2 + strlen(argv[3]) + 1 + strlen("octet") + 1;

        // Enviamos la petición:
        if (sendto(socketClient, pack, sizeof(char)*packSize, 0, (struct sockaddr*) &serverID, sizeof(serverID)) == -1) {
            perror("FAIL: No se pudo enviar la petición al servidor.\n");
            exit(0);
        }

        // Si se solicitó [-v], imprimimos información sobre la petición:
        if (argV == 1) {
            printf("Enviada solicitud de lectura de %s a servidor tftp en %s.\n", argv[3], argv[1]);
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
                    printf("Es el bloque con codigo %d.\n", block);
                }
            }

            // Comprobamos que el número del bloque recibido en el paquete es el mismo que el esperado.
            rcvBlock = (unsigned char) pack[2] * 256 + (unsigned char) pack[3];
            if (rcvBlock != block) {
                printf("FAIL: Se recibió el bloque %d, mientras se esperaba el %d.\n", rcvBlock, block);
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

            // Escribimos los datos recibidos en el fichero
            fwrite(pack + 4, sizeof(char), size - 4, file);

            // Respondemos al servidor con un ACK con el número de bloque.
            // Como el paquete recibido, incluye en los bytes 2 y 3 el número de bloque,
            // lo reutilizamos como ACK cambiando solo el número de operacion:
            pack[0] = 0;    pack[1] = 4;
            int sizeACK = 4;
            // Respondemos al servidor con el ACK
            if (sendto(socketClient, pack, sizeACK, 0, (struct sockaddr*) &serverID, sizeof(serverID)) == -1) {
                perror("FAIL: No se pudo enviar el ACK al servidor.\n");
                exit(0);
            }
            if (argV == 1) {
                printf("Enviamos el ACK del bloque %d.\n", block);
            }
            
            // En caso de haber recibido el último bloque, finalizamos:
            if (size < MAXDATASIZE) {
                if (argV == 1) {
                    printf("El bloque %d era el ultimo: cerramos el fichero.\n", block);
                }

                // Cambiamos al condición de bucle y cerramos el fichero:
                rcvEnd = 1;
                if (fclose(file) == 1) {
                    perror("FAIL: No se pudo cerrar el fichero.\n");
                    exit(0);
                }
            }

            // Cambiamos el número de bloque esperado en el siguiente ciclo:
            block++;
        }  
    }

    // -----   Iniciamos el modo ESCRITURA  -----
    else if (row == 1) {
        // Construimos la petición de escritura WRQ:
        pack[0] = 0;    pack[1] = 2;
        strcpy(&pack[2], filename);                  // Se añade el nombre del fichero
        strcpy(&pack[strlen(filename)+3], "octet");  // Se añade el modo "octet"

        // tamaño del datagrama
        int datSize = 2 + strlen(argv[3]) + 1 + strlen("octet") + 1;
        
        // Enviamos la petición:
        if (sendto(socketClient, pack, sizeof(char) * datSize, 0, (struct sockaddr*) &serverID, sizeof(serverID)) == -1) {
            perror("FAIL: No se pudo enviar la petición al servidor.\n");
            exit(0);
        } else if (argV == 1) {
            // Si se solicitó [-v], imprimimos información sobre la petición:
            printf("Enviada solicitud de escritura de %s a servidor tftp en %s.\n", argv[3], argv[1]);
        }

        // Abrimos el archivo de datos como lectura:
        if ((file = fopen(argv[3], "w")) == NULL) {
            perror("FAIL: No se pudo abrir el fichero.\n");
            exit(0);
        }

        // Variables de control:
        int rcvEnd = 0, size = 0;
        block=0;

        while (rcvEnd == 0){      //Recibimos ACK del server
            socklen_t longserv = sizeof(serverID);
            if((size = recvfrom(socketClient, pack, MAXDATASIZE, 0, (struct sockaddr *) &serverID, &longserv)) == -1) {
                perror("recvfrom()");
                exit(0);
            }
            if (argV==1){
                printf("Recibido ACK del servidor tftp.\n");

            }
            //Es el primer ACK que recibimos
            if(block==0 && argV==1){
                printf("Es el primer ACK (numero de bloque 0). \n");
            } else if(argV == 1){
                //No es el primer ACK recibido
                printf("Es el ACK con codigo %d.\n", block);
            }
            //Calculamos el numero de bloque del ACK recibido
            rcvBlock = (unsigned char) pack[2] * 256 + (unsigned char) pack[3];
            if(rcvBlock != block){
                printf("Numero de bloque recibido descolocado, tocaba el bloque %d y se recibio %d \n", block, rcvBlock);
                exit(0);
            }
            //Aumentamos en 1 el bloque esperado.
            block++;
            //Obtenemos el codigo de operacion del datagrama recibido.
            operationCode = pack[1];
            //Comprobacion de error
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
            // Formamos el paquete con los datos del archivo.
            if (feof(file)) {

                // Se ha terminado de recibir el archivo.
                block--;

                if (argV == 1){
                    printf("El bloque %d era el ultimo: cerramos el fichero.\n", block);
                }

                //Cerramos fichero.
                if(fclose(file)!=0){
                    perror("FAIL: No se ha podido cerrar el fichero.\n");
                    exit(0);
                }

                // Cerramos el bucle:
                rcvEnd = 1;

            } else {
                // Todavía no se ha terminado de leer el archivo.
                size = fread(pack + 4, sizeof(char), 512, file);
            }

            // Si no se ha acabado el bucle, enviamos el datagrama UDP.
            if(rcvEnd != 1) {

                // Formamos el datagrama a enviar:
                pack[1] = 3;
                pack[2] = (rcvBlock + 1) / 256;
                pack[3] = (rcvBlock + 1) % 256;
                datSize = 2 + 2 + size;

                // Enviamos el datagrama al Servidor.
                if(sendto(socketClient, pack, sizeof(char) * datSize, 0, (struct sockaddr*) &serverID, sizeof(serverID)) == -1) {
                    perror("FAIL: No se pudo enviar el datagrama al servidor");
                    exit(0);
                } else if (argV == 1) {
                    printf("Enviamos el bloque %d del fichero.\n", block);
                }
            }

            // Preparamos las variables para el siguiente bucle:
            block++;
        // fin de bucle
        }

    // fin de escritura    
    }

    close(socketClient);
    return 0;

// fin del main
}