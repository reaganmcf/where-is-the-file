#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>

#ifndef CLIENT_H
#define CLIENT_H

#define WTF_CONFIGURATION_FILE_PATH "./.configuration"

//Possible Error Codes for the Client
enum _error_codes
{
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
  E_CANNOT_WRITE_TO_MANIFEST = 14
};

typedef enum _error_codes wtf_error;

struct _error_desc
{
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
    {E_CANNOT_WRITE_TO_MANIFEST, "Improper permissions to write .Manifest."}

};

//Struct for a wtf_connection
typedef struct _wtf_connection
{
  int socket;
  struct sockaddr_in address;
  struct hostent *host;
  int port;
  int len;
} wtf_connection;

//Struct for .configuration
typedef struct _configuration
{
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

#endif