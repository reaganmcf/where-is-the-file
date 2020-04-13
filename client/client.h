
#ifndef CLIENT_H
#define CLIENT_H

//Possible Error Codes for the Client
enum _error_codes
{
    E_INVALID_CONFIGURATION = 1,
    E_CONFIGURATION_WRITE_ERROR = 2,
    E_SERVER_NOT_FOUND = 3,
    E_IMPROPER_PARAMS_AND_FLAGS = 4,
    E_IMPROPER_CONFIGURATION_PARAMS = 5
};

typedef enum _error_codes wtf_error;

struct _error_desc
{
    int code;
    char *message;
} errordesc[] = {
    {},
    {E_INVALID_CONFIGURATION, "Invalid configuration parameters."},
    {E_CONFIGURATION_WRITE_ERROR, "Failed to write .configuration. Please ensure you have proper permissions."},
    {E_SERVER_NOT_FOUND, "Failed to connect to the server provided in the .configuration file"},
    {E_IMPROPER_PARAMS_AND_FLAGS, "Improper params and flags. Please make sure to enter a valid command with all required params"},
    {E_IMPROPER_CONFIGURATION_PARAMS, "Improper params for configure command. Please follow the format of ./WTF configure <IP/hostname> <port>"}};

//Function Prototype for printing custom errors
void wtf_perror(wtf_error e);

//Function Prototype for configuring connection to server
int configure_host(char *hostname, char* port);

#endif