// Practica tema 8, Velasco Gil Alvaro

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

// Estructuras de IP, ICMP y PING
#include "ip-icmp-ping.h"

int sockfd, numBytes;
int arg_V = 0; // 1: informacion visible
char ip[20];  // IP de la meta

struct in_addr addr;
struct sockaddr_in client, goal;
socklen_t addr_size;


int main(int argc, char *argv[]){

    // Comprobamos los argumentos introducidos.
    if (argc != 2 && argc != 3) {
        printf("FAIL: Se debe meter 1 o 2 argumentos - <ip> [-v].\n");
        exit(EXIT_FAILURE);
    } else {
        if (inet_aton(argv[1], &addr) == 0){
            fprintf(stderr, "FAIL: La direccion ip no es valida.\n");
            exit(EXIT_FAILURE);
        }
        // Direccion valida, la guardamos.
        strcpy(ip, argv[1]);

        if (strcmp(ip, "127.0.0.1") == 0) {
            printf("FAIL: No se puede usar esa direccion.\n");
            exit(EXIT_FAILURE);
        }

        // Si hay 2 argumentos, el segundo debe ser -v
        if (argc == 3) {
            if (strcmp(argv[2],"-v") == 0) {
                arg_V=1;
            } else {
                printf("FAIL: El segundo argumento debe ser [-v], o no ponerlo.\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Estructuras de la peticion y la respuesta.
    ECHORequest echoRequest;
    ECHOResponse echoResponse;

    // Creamos el socket.
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd == -1) {
        printf("FAIL: No se pudo crear el socket.\n");
        exit(EXIT_FAILURE);
    }

    // --- LOCAL ---
    client.sin_family = AF_INET;
    client.sin_port = 0;
    client.sin_addr.s_addr = INADDR_ANY;

    // --- OBJETIVO ---
    goal.sin_family = AF_INET;
    goal.sin_port = 0;
    goal.sin_addr.s_addr = inet_addr(ip);

    // Asignamos el socket a la direccion local.
	if (bind(sockfd, (struct sockaddr *) &client, sizeof(goal)) == -1) {
		perror("FAIL: No se ha podido asignar bien el socket.\n");
		exit(EXIT_FAILURE);
	}

    //--- Construimos la cabecera del paquete ---
    if(arg_V == 1){
        printf("-> Generando cabecera ICMP.\n");
    }

    // Ponemos la petición a 0 para evitar errores.
    bzero(&echoRequest, sizeof(echoRequest));

    // Creamos la peticion
    echoRequest.icmpHeader.Type = 8;
    echoRequest.icmpHeader.Code = 0;
    echoRequest.ID = getpid();
    echoRequest.SeqNumber = 0;
    strcpy(echoRequest.payload, "Este es el payload");
    echoRequest.icmpHeader.Checksum = 0;

    if (arg_V == 1) {
        printf("-> Type: %d\n", echoRequest.icmpHeader.Type);
        printf("-> Code: %d\n", echoRequest.icmpHeader.Code);
        printf("-> Identifier (pid): %d.\n", echoRequest.ID);
        printf("-> Seq. number: %d\n", echoRequest.SeqNumber);
        printf("-> Cadena a enviar: %s.\n", echoRequest.payload);
    }

    // Calculamos el checksum basandonos en los apuntes:
    int numShorts = sizeof(echoRequest) / 2;
    unsigned short int *puntero;
    unsigned int acumulador = 0;
    puntero = (unsigned short int *) &echoRequest;

    int i;
    for( i = 0; i < numShorts; i++) {
        acumulador = acumulador + (unsigned int) *puntero;
        puntero++;
    }

    acumulador = (acumulador>>16) + (acumulador & 0x0000ffff);
    acumulador = (acumulador>>16) + (acumulador & 0x0000ffff);
    acumulador = ~acumulador;

    // Guardamos el checksum.
    echoRequest.icmpHeader.Checksum = (unsigned int short) acumulador;

    // Comprobamos de nuevo, por si hemos calculado mal.
    puntero = (unsigned short int *) &echoRequest;
    acumulador = 0;
    for( i = 0; i < numShorts; i++) {
        acumulador = acumulador + (unsigned int) *puntero;
        puntero++;
    }

    acumulador = (acumulador>>16) + (acumulador & 0x0000ffff);
    acumulador = (acumulador>>16) + (acumulador & 0x0000ffff);
    acumulador = ~acumulador;

    // Si el checksum no es 0, es que ha habido un error.
    if ((unsigned short int) acumulador != 0) {
        printf("FAIL: No se ha calculado el checksum de forma correcta.\n");
        exit(EXIT_FAILURE);
    }

    if (arg_V == 1) {
        printf("-> Checksum: 0x%x.\n", echoRequest.icmpHeader.Checksum);
        printf("-> Tamaño total del paquete ICMP: %ld.\n", sizeof(echoRequest));
    }

    // --- Enviamos el paquete ICMP ---
    if (sendto(sockfd, &echoRequest, sizeof(echoRequest), 0, (struct sockaddr *) &goal, sizeof(struct sockaddr_in)) == -1) {
        printf("FAIL: No se pudo enviar el paquete.\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Paquete ICMP enviado a %s\n", ip);
    }

    // --- Recibimos la respuesta de nuestro objetivo ---
    socklen_t addrlen = sizeof(goal);
    numBytes = recvfrom(sockfd, &echoResponse, sizeof(echoResponse), 0, (struct sockaddr *) &goal, &addrlen);
    if (numBytes < 0) {
        printf("FAIL: No se pudo recibir el paquete correctamente.\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Respuesta recibida desde %s\n", ip);
    }

    if (arg_V == 1) {
        printf("-> Tamaño de la respuesta: %d\n", numBytes);
        printf("-> Cadena recibida: %s.\n", echoResponse.payload);
        printf("-> Identifier (pid): %d.\n", echoResponse.ID);
        printf("-> TTL: %d.\n", echoResponse.ipHeader.TTL);
    }

    // Mostramos la respuesta basandonos en la Wikipedia en ingles: https://en.wikipedia.org/wiki/Internet_Control_Message_Protocol
    printf("Descripcion de la respuesta: ");

    int type = echoResponse.icmpHeader.Type;
    int code = echoResponse.icmpHeader.Code;

    switch(type){
        case 0:
            printf("echo reply (type %d, code %d).\n", type, code);
            break;
        case 1 ... 2:
            printf("reserved (type %d, code %d).\n", type, code);
            break;
        case 3:
            switch (code) {
                case 0:
                    printf("destination network unreachable (type %d, code %d).\n", type, code);
                    break;
                case 1:
                    printf("destination host unreachable (type %d, code %d).\n", type, code);
                    break;
                case 2:
                    printf("destination protocol unreachable (type %d, code %d).\n", type, code);
                    break;
                case 3:
                    printf("destination port unreachable (type %d, code %d).\n", type, code);
                    break;
                case 4:
                    printf("fragmentation required, and DF flag set (type %d, code %d).\n", type, code);
                    break;
                case 5:
                    printf("source route failed (type %d, code %d).", type, code);
                    break;
                case 6:
                    printf("destination network unknown (type %d, code %d).\n", type, code);
                    break;
                case 7:
                    printf("destination host unknown (type %d, code %d).\n", type, code);
                    break;
                case 8:
                    printf("source host isolated (type %d, code %d).\n", type, code);
                    break;
                case 9:
                    printf("network administratively prohibited (type %d, code %d).\n", type, code);
                    break;
                case 10:
                    printf("host administratively prohibited (type %d, code %d).\n", type, code);
                    break;
                case 11:
                    printf("network unreachable for ToS (type %d, code %d).\n", type, code);
                    break;
                case 12:
                    printf("host unreachable for ToS (type %d, code %d).\n", type, code);
                    break;
                case 13:
                    printf("communication administratively prohibited (type %d, code %d).\n", type, code);
                    break;
                case 14:
                    printf("host Precedence Violation (type %d, code %d).\n", type, code);
                    break;
                case 15:
                    printf("precedence cutoff in effect (type %d, code %d).\n", type, code);
                    break;
            }
        case 4:
            printf("source quench (congestion control) (type %d, code %d).\n", type, code);
            break;
        case 5:
            switch (code) {
                case 0:
                    printf("redirect Datagram for the Network (type %d, code %d).\n", type, code);
                    break;
                case 1:
                    printf("redirect Datagram for the Host (type %d, code %d).\n", type, code);
                    break;
                case 2:
                    printf("redirect Datagram for the ToS & network (type %d, code %d).\n", type, code);
                    break;
                case 3:
                    printf("redirect Datagram for the ToS & host (type %d, code %d).\n", type, code);
                    break;
            }
        case 6:
            printf("alternate Host Address (type %d, code %d).\n", type, code);
            break;
        case 7:
            printf("reserved (type %d, code %d).", type, code);
            break;
        case 8:
            printf("echo request (used to ping) (type %d, code %d).", type, code);
            break;
        case 9:
            printf("router Advertisement (type %d, code %d).", type, code);
            break;
        case 10:
            printf("router discovery/selection/solicitation (type %d, code %d).\n", type, code);
            break;
        case 11:
            switch (code) {
                case 0:
                    printf("TTL expired in transit (type %d, code %d).\n", type, code);
                    break;
                case 1:
                    printf("fragment reassembly time exceeded (type %d, code %d).\n", type, code);
                    break;
            }
        case 12:
            switch (code) {
                case 0:
                    printf("pointer indicates the error (type %d, code %d).\n", type, code);
                    break;
                case 1:
                    printf("missing a required option (type %d, code %d).\n", type, code);
                    break;
                case 2:
                    printf("bad length (type %d, code %d).\n", type, code);
                    break;
            }
        case 13:
            printf("timestamp (type %d, code %d).\n", type, code);
            break;
        case 14:
            printf("timestamp reply (type %d, code %d).\n", type, code);
            break;
        case 15:
            printf("information Request (type %d, code %d).\n", type, code);
            break;
        case 16:
            printf("information Reply (type %d, code %d).\n", type, code);
            break;
        case 17:
            printf("address Mask Request (type %d, code %d).\n", type, code);
            break;
        case 18:
            printf("address Mask Reply (type %d, code %d).\n", type, code);
            break;
        case 19:
            printf("reserved for security (type %d, code %d).\n", type, code);
            break;
        case 20 ... 29:
            printf("reserved for robustness experiment (type %d, code %d).\n", type, code);
            break;
        case 30:
            printf("information Request (type %d, code %d).\n", type, code);
            break;
        case 31:
            printf("datagram Conversion Error (type %d, code %d).\n", type, code);
            break;
        case 32:
            printf("mobile Host Redirect (type %d, code %d).\n", type, code);
            break;
        case 33:
            printf("Where-Are-You (originally meant for IPv6) (type %d, code %d).\n", type, code);
            break;
        case 34:
            printf("Here-I-Am (originally meant for IPv6) (type %d, code %d).\n", type, code);
            break;
        case 35:
            printf("Mobile Registration Request (type %d, code %d).\n", type, code);
            break;
        case 36:
            printf("Mobile Registration Reply (type %d, code %d).\n", type, code);
            break;
        case 37:
            printf("Domain Name Request (type %d, code %d).\n", type, code);
            break;
        case 38:
            printf("Domain Name Reply (type %d, code %d).\n", type, code);
            break;
        case 39:
            printf("SKIP Algorithm Discovery Protocol, Simple Key-Management for Internet Protocol (type %d, code %d).\n", type, code);
            break;
        case 40:
            printf("Photuris, Security failures || Type %d || Code %d\n", type, code);
            break;
        case 41:
            printf("ICMP for experimental mobility protocols such as Seamoby [RFC4065] (type %d, code %d).\n", type, code);
            break;
        case 42 ... 252:
            printf("reserved (type %d, code %d).\n", type, code);
            break;
        case 253:
            printf("RFC3692-style Experiment 1 (type %d, code %d).\n", type, code);
            break;
        case 254:
            printf("RFC3692-style Experiment 2 (type %d, code %d).\n", type, code);
            break;
        case 255:
            printf("Reserved (type %d, code %d).\n", type, code);
            break;

    }

    // Finalizamos
    return(0);
}