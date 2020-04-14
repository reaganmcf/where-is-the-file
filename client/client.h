
#ifndef CLIENT_H
#define CLIENT_H

#define WTF_CONFIGURATION_FILE_PATH "./.configuration"

//Possible Error Codes for the Client
enum _error_codes {
  E_INVALID_CONFIGURATION = 1,
  E_CONFIGURATION_WRITE_ERROR = 2,
  E_SERVER_NOT_FOUND = 3,
  E_IMPROPER_PARAMS_AND_FLAGS = 4,
  E_IMPROPER_CONFIGURATION_PARAMS = 5
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

};

//Struct for .configuration
typedef struct _configuration {
  char *hostname;
  int port;
} wtf_configuration;

//Function Prototype for freeing all memory after exit
static void wtf_exit_handler(void);

//Function Prototype to handle attemtping connection and returning the socket FD

//Function Prototype for printing custom errors
void wtf_perror(wtf_error e, int should_exit);

//Function Prototype for writing out .configuration
int wtf_configure_host(char *hostname, char *port);

#endif