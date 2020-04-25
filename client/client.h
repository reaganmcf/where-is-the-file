#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>

#ifndef CLIENT_H
#define CLIENT_H

#define WTF_CONFIGURATION_FILE_PATH "./.configuration"

//Possible Error Codes for the Client
enum _error_codes {
  E_INVALID_CONFIGURATION = 1,
  E_CONFIGURATION_WRITE_ERROR = 2,
  E_SERVER_NOT_FOUND = 3,
  E_IMPROPER_PARAMS_AND_FLAGS = 4,
  E_IMPROPER_CONFIGURATION_PARAMS = 5,
  E_CANNOT_CREATE_SOCKET = 6,
  E_UNKNOWN_HOST = 7,
  E_CANNOT_CONNECT_TO_HOST = 8,
  E_IMPROPER_CREATE_PARAMS = 9,
  E_IMPROPER_CREATE_PROJECT_NAME_LENGTH = 10,
  E_IMPROPER_CREATE_PROJECT_NAME = 11,
  E_SERVER_MANIFEST_ALREADY_EXISTS = 12,
  E_SERVER_IMPROPER_PERMISSIONS = 13,
  E_CANNOT_WRITE_TO_MANIFEST = 14,
  E_PROJECT_DOESNT_EXIST_ON_CLIENT = 15,
  E_IMPROPER_ADD_PARAMS = 16,
  E_FILE_DOESNT_EXIST_TO_ADD = 17,
  E_FILE_ALREADY_ADDED_TO_MANIFEST = 18,
  E_FILE_DOESNT_EXIST = 19,
  E_CANNOT_READ_FILE = 20,
  E_FILE_MAX_LENGTH = 21
};

typedef enum _error_codes wtf_error;

struct _error_desc {
  int code;
  char *message;
} errordesc[] = {
    {},
    {E_INVALID_CONFIGURATION, ".configuration is either missing or invalid or cannot be read. Please re-run ./WTF configure <hostname> <port>."},
    {E_CONFIGURATION_WRITE_ERROR, "Failed to write .configuration. Please ensure you have proper permissions."},
    {E_SERVER_NOT_FOUND, "Failed to connect to the server provided in the .configuration file"},
    {E_IMPROPER_PARAMS_AND_FLAGS, "Improper params and flags. Please make sure to enter a valid command with all required params"},
    {E_IMPROPER_CONFIGURATION_PARAMS, "Improper params for configure command. Please follow the format of ./WTF configure <IP/hostname> <port>"},
    {E_CANNOT_CREATE_SOCKET, "Failed to create socket.\n"},
    {E_UNKNOWN_HOST, "Unknwon host provided\n"},
    {E_CANNOT_CONNECT_TO_HOST, "Failed to connect to the host and port provided"},
    {E_IMPROPER_CREATE_PARAMS, "Imporper params for create command. Please follow the format of ./WTF create <project-name>"},
    {E_IMPROPER_CREATE_PROJECT_NAME_LENGTH, "Improper project name for create command. Please ensure your project name length is between 1 and 100 characters long"},
    {E_IMPROPER_CREATE_PROJECT_NAME, "Improper project name for create command. Please make sure the project name provided does not contain ':'."},
    {E_SERVER_MANIFEST_ALREADY_EXISTS, "Please create a project name that is unique."},
    {E_SERVER_IMPROPER_PERMISSIONS, "The server could not read and/or write to the Manifests directory. Please make sure the server is correctly configured."},
    {E_CANNOT_WRITE_TO_MANIFEST, "Improper permissions to write .Manifest."},
    {E_PROJECT_DOESNT_EXIST_ON_CLIENT, "Project doesn't exist on the client."},
    {E_IMPROPER_ADD_PARAMS, "Improper Params for add command. Please follow the format of ./WTF add <project-name> <file-path>"},
    {E_FILE_DOESNT_EXIST_TO_ADD, "Provided file path for add command not found. Please follow the following format example: ./WTF add project project/myfile.txt"},
    {E_FILE_ALREADY_ADDED_TO_MANIFEST, "File already exists in .Manifest for the client. Aborting add command"},
    {E_FILE_DOESNT_EXIST, "Provided file path does not exist"},
    {E_CANNOT_READ_FILE, "Provided file path does not have proper read permissions"},
    {E_FILE_MAX_LENGTH, "Provided file is very large. Only reading the first 10000 characters"}

};

//Struct for a wtf_connection
typedef struct _wtf_connection {
  int socket;
  struct sockaddr_in address;
  struct hostent *host;
  int port;
  int len;
} wtf_connection;

//Struct for .configuration
typedef struct _configuration {
  char *hostname;
  int port;
} wtf_configuration;

//Function Prototype for freeing all memory after exit
static void wtf_exit_handler(void);

//Function Prototype to handle attemtping connection and returning a valid wtf_connection struct
wtf_connection *wtf_connect();

//Function Prototype for printing custom errors
void wtf_perror(wtf_error e, int should_exit);

//Function Prototype for writing out .configuration
int wtf_configure_host(char *hostname, char *port);

//Function Prototype for adding file to .Manifest on client
int wtf_add(char *, char *);

//Function Prototype for hashing a file helper function
char *hash_file(char *path);

#endif