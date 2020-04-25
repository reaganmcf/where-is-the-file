#include <linux/in.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>

#ifndef SERVER_H
#define SERVER_H

//Possible Command Strings for the Server
const char *COMMAND_CREATE_PROJECT = "create_project";
const char *COMMAND_CURRENT_VERSION_PROJECT = "get_current_version";

//Possible Error Codes for the Server
enum _error_codes {
  E_IMPROPER_PARAMS = 1,
  E_ERROR_MAKING_SOCKET = 2,
  E_ERROR_BINDING_SOCKET_TO_PORT = 3,
  E_CANNOT_LISTEN_TO_PORT = 4,
  E_CANNOT_READ_OR_WRITE_PROJECT_DIR = 5,
  E_PROJECT_ALREADY_EXISTS = 6
};

typedef enum _error_codes wtf_error;

struct _error_desc {
  int code;
  char *message;
} errordesc[] = {
    {},
    {E_IMPROPER_PARAMS, "Improper params. Please make sure to run the server with the format of ./WTFServer <port>"},
    {E_ERROR_MAKING_SOCKET, "There was an issue while creating the socket"},
    {E_ERROR_BINDING_SOCKET_TO_PORT, "There was an error while attempting to bind the socket to the port provided."},
    {E_CANNOT_LISTEN_TO_PORT, "Unable to have the socket listen on the provided port."},
    {E_CANNOT_READ_OR_WRITE_PROJECT_DIR, "Unable to read or write to ./Manifests/ directory."},
    {E_PROJECT_ALREADY_EXISTS, "Project already exists with this name."}

};

typedef struct
{
  int socket;
  struct sockaddr address;
  int addr_len;
} wtf_connection;

//Function Prototype for multithreaded connection handler
void *wtf_process(void *);

//Function Prototype for creating the Project Manifest
int wtf_server_create_project(char *);

//Function Prototype for printing custom errors
void wtf_perror(wtf_error e, int should_exit);

//Function Prototype for checking if path is a regular file
int isRegFile(const char *path);

#endif