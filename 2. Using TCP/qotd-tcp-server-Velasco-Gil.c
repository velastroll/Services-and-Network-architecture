// Practica tema 6, Velasco Gil lvaro

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <string.h>
#include <unistd.h>

#define MAXQUOTESIZE 512
#define MAXLENGTH 100
#define NUMMAXCLIENT 5

int socketServer; // Socket del servidor
int socketClient; // Socket del cliente.

// Nombramos las funciones a implementar
void generaQuote(void);
void signal_handler(int);

// buffer donde se almacena la quote de Fortune
static char buffer[MAXLENGTH];
// Quote que vamos a enviar
static char finalQuote[MAXQUOTESIZE];

int main(int argc, char *argv[])
{

  // Nos aseguramos que haya solo un argumento
  if (argc < 1 || argc > 3 || argc == 2)
  {
    printf("Número de argumentos no válido.\n");
    exit(1);
  }

  // -- SERVER SIDE --
  int socketServer;            // Socket del servidor.
  struct servent *portServer;  // Puerto servidor
  struct sockaddr_in serverID; // direccion servidor

  // -- CLIENT SIDE --
  int socketClient;            // Socket del cliente.
  struct sockaddr_in clientID; // direccion cliente
  int portClient;              // Puerto protocolo
  int pid;                     // Identidad del proceso

  // Llamamos para liberar el socket al cerrar el servidor.
  signal(SIGINT, signal_handler);

  //Rellenamos la direccion del servidor.
  serverID.sin_family = AF_INET;

  // Comprobamos la entrada de argumentos.
  if (argc == 3)
  {
    // Se indica un posible puerto, comprobamos validez.
    if (strcmp(argv[1], "-p") != 0)
    {
      printf("El primer argumento no es válido");
      exit(0);
    }
    else
    {
      sscanf(argv[2], "%d", &portClient);
      serverID.sin_port = htons(portClient);
    }
  }
  else
  {
    // No se indica el puerto, lo obtenemos:
    portServer = getservbyname("qotd", "tcp");
    serverID.sin_port = portServer->s_port;
  }

  // Establecemos la direccion del servidor:
  serverID.sin_addr.s_addr = inet_addr("127.0.0.1");

  // Iniciamos el socket del servidor
  if ((socketServer = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("FALLO: No se pudo iniciar el socket del servidor.");
    exit(1);
  }

  // Enlazamos la dirección del servidor con el socket:
  if (bind(socketServer, (struct sockaddr *)&serverID, sizeof(serverID)) == -1)
  {
    perror("FALLO: No se pudo enlazar el socket con la direccion del servidor.");
    exit(0);
  }

  // Escuchamos si un cliente se quiere conectar
  if (listen(socketServer, NUMMAXCLIENT) == -1)
  {
    perror("listen()");
    exit(0);
  }

  while (1 == 1)
  {

    // Tamanno de la direccion del cliente
    socklen_t sizeClient = sizeof(clientID);

    // Aceptamos una nueva conexion y guardamos socket de ese cliente
    if ((socketClient = accept(socketServer, (struct sockaddr *)&clientID, &sizeClient)) == -1)
    {
      perror("FALLO: No se pudo aceptar la conexión");
      exit(0);
    }

    // Nuevo proceso para poder recibir varias peticiones
    pid = fork();

    // El hilo padre se cierra, mientras que el hijo envia la cadena y pasan a ser padres
    if (pid == 0)
    {
      // Obtenemos un nuevo mensaje con Fortune en finalQuote.
      generaQuote();

      // Se lo enviamos al cliente.
      if (send(socketClient, (char *)&finalQuote, sizeof(finalQuote), 0) == -1)
      {
        perror("FALLO: El servidor no pudo enviar el mensaje.");
      }
      exit(0);
    }
    else
    {
      close(socketClient);
    }
  }
  return 0;
}

// Metodo que genera un mensaje con Fortune y lo guarda en finalQuote
void generaQuote(void)
{

  // Preparamos el encabezado del servidor
  char encabezado[MAXQUOTESIZE] = "Quote of the day from ";
  char hostname[MAXQUOTESIZE];
  gethostname(hostname, MAXQUOTESIZE);
  strcat(encabezado, hostname);
  strcat(encabezado, ":\n");
  strcpy(finalQuote, encabezado);

  // Formamos la cadena para entregar al cliente con Fortune
  system("/usr/games/fortune -s > /tmp/tt.txt");
  FILE *fich = fopen("/tmp/tt.txt", "r");
  int nc = 0;
  do
  {
    buffer[nc++] = fgetc(fich);
  } while (nc < MAXLENGTH - 1);
  fclose(fich);

  // Formamos el mensaje a enviar.
  strcat(finalQuote, buffer);
  strcat(finalQuote, "\n");
}

// Captura la señal Ctrl+C y cierra los sockets
void signal_handler(int sig)
{
  signal(sig, SIG_IGN);
  char confirmationBuffer1[MAXQUOTESIZE] = "";
  char confirmationBuffer2[MAXQUOTESIZE] = "";

  // Avisamos el cliente
  printf("\n > Solicitando cerrar los sockets... ");
  shutdown(socketServer, SHUT_RDWR);
  shutdown(socketClient, SHUT_RDWR);
  printf("HECHO. \n");

  // Recibimos confirmacion del cliente

  printf(" > Recibiendo confirmación... ");
  recv(socketClient, confirmationBuffer1, MAXQUOTESIZE, 0);
  recv(socketServer, confirmationBuffer2, MAXQUOTESIZE, 0);
  printf("RECIBIDA.\n");

  // Cerramos los sockets

  printf(" > Cerrando los sockets... ");
  close(socketServer);
  close(socketClient);
  printf("CERRADOS.\n");

  exit(0);
}