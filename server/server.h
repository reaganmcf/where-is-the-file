#include <linux/in.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>

#ifndef SERVER_H
#define SERVER_H

//Possible Error Codes for the Server
enum _error_codes {
  E_IMPROPER_PARAMS = 1,
  E_ERROR_MAKING_SOCKET = 2,
  E_ERROR_BINDING_SOCKET_TO_PORT = 3,
  E_CANNOT_LISTEN_TO_PORT = 4,
};

typedef enum _error_codes wtf_error;

struct _error_desc {
  int code;
  char* message;
} errordesc[] = {
    {},
    {E_IMPROPER_PARAMS, "Improper params. Please make sure to run the server with the format of ./WTFServer <port>"},
    {E_ERROR_MAKING_SOCKET, "There was an issue while creating the socket"},
    {E_ERROR_BINDING_SOCKET_TO_PORT, "There was an error while attempting to bind the socket to the port provided."},
    {E_CANNOT_LISTEN_TO_PORT, "Unable to have the socket listen on the provided port."}

};

typedef struct {
  int socket;
  struct sockaddr address;
  int addr_len;
} wtf_connection;

//Function Prototype for multithreaded connection handler
void* wtf_process(void*);

//Function Prototype for printing custom errors
void wtf_perror(wtf_error e, int should_exit);

#endif