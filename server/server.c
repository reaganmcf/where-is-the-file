#include "server.h"

#include <dirent.h>
#include <fcntl.h>
#include <linux/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * WTF Server
 * 
 * Multithreaded Socket server to handle multiple connections at a time
 */

int main(int argc, char **argv) {
  //Check if only a port is passed in as a param as it should
  if (argc == 1 || argc > 2) {
    wtf_perror(E_IMPROPER_PARAMS, 1);
  }

  //Now, let's do some setup and start the server
  int sock = -1;
  struct sockaddr_in address;
  int port = argv[1];
  wtf_connection *connection;
  pthread_t thread;

  //Create socket
  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock <= 0) {
    wtf_perror(E_ERROR_MAKING_SOCKET, 1);
  }

  //Bind socket to port
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);
  if (bind(sock, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) < 0) {
    wtf_perror(E_ERROR_BINDING_SOCKET_TO_PORT, 1);
  }

  if (listen(sock, 100) < 0) {
    wtf_perror(E_CANNOT_LISTEN_TO_PORT, 1);
  }

  printf("Server has started up Successfully. Listening for Connections...\n");

  while (1) {
    //Listen for incoming connections
    connection = malloc(sizeof(wtf_connection));
    connection->socket = accept(sock, &connection->address, &connection->addr_len);
    if (connection->socket <= 0) {
      free(connection);
    } else {
      //Start a new thread but do not wait for it
      pthread_create(&thread, 0, wtf_process, (void *)connection);
      pthread_detach(thread);
    }
  }

  return 0;
}

void *wtf_process(void *pointer) {
  char *buffer;
  int len;
  wtf_connection *connection;
  int addr = 0;

  if (!pointer) {
    pthread_exit(0);
  }
  connection = (wtf_connection *)pointer;

  /* Read length of message */
  read(connection->socket, &len, sizeof(int));
  if (len > 0) {
    addr = ((struct sockaddr_in *)&connection->address)->sin_addr.s_addr;
    buffer = malloc((len + 1) * sizeof(char));
    buffer[len] = 0;
    read(connection->socket, buffer, len);
    printf("%d.%d.%d.%d: %s\n", (addr)&0xFF, (addr >> 8) & 0xFF, (addr >> 16) & 0xFF, (addr >> 24) & 0xFF, buffer);
    free(buffer);
  }

  //Close socket and cleanup
  close(connection->socket);
  free(connection);
  pthread_exit(0);
}

/**
 * Custom perror for our custom wtf_error
 * 
 * Prints out the error code and error description
 * 
 * If should_exit is 1 then also send exit() command
 */
void wtf_perror(wtf_error e, int should_exit) {
  printf("\033[0;31m");
  printf("[ Error Code %d ] %s\n", errordesc[e].code, errordesc[e].message);
  printf("\033[0m");

  if (should_exit == 1) {
    exit(errordesc[e].code);
  }
}
