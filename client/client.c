#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "client.h"

/** WTF Client
 * 
 * The WTF client will be handling all of the commands for this project, and will send each command to its proper
 * sub function from main to abstract out the code and make it more readable
 */

int main(int argc, char **argv)
{

    //First, we need to check the params and flags
    if (argc == 1)
    {
        wtf_perror(E_IMPROPER_PARAMS_AND_FLAGS, 1);
    }

    //Now, lets check send the command to the proper function
    if (strcmp(argv[1], "configure") == 0)
    {
        //Check for params
        if (argc != 4)
        {
            wtf_perror(E_IMPROPER_CONFIGURATION_PARAMS, 1);
        }
        else if (strlen(argv[2]) == 0 || strlen(argv[3]) == 0)
        {
            wtf_perror(E_IMPROPER_CONFIGURATION_PARAMS, 1);
        }

        int result = wtf_configure_host(argv[2], argv[3]);
        if (result == 1)
        {
            printf("Succesfully configured client.\n");
            return 0;
        }
        else
        {
            wtf_perror(E_CONFIGURATION_WRITE_ERROR, 1);
        }
    }
    return 0;
}

/**
 * Connect to server
 * 
 * Checks if there is a .configuration file, if so, then attempts to connect to the server
 * 
 * Returns:
 *  0 = failure
 *  1 = success
 */
int wtf_connect()
{

    return 0;
}

/**
 * Configure Host
 * 
 * Takes in the hostname and port and writes out to .configure for later use
 * 
 * Throws wtf_error.E_CONFIGURATION_WRITE_ERROR if was unable to write .configuration
 * 
 * Returns:
 *  0 = failure
 *  1 = success
 */
int wtf_configure_host(char *hostname, char *port)
{
    /**
     * The .configure format is the following, example of my.server.com with port 2503
     * 
     * hostname:my.server.com|port:2503
     * 
     */

    //if .configure already exists, delete it and lets create a new one (to override)
    if (access("./.configure", F_OK) != -1)
    {
        remove("./.configure");
    }
    int fd = open("./.configure", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    char *tStr = malloc(500);
    sprintf(tStr, "hostname:%s|port:%s", hostname, port);
    int num_bytes = write(fd, tStr, strlen(tStr));
    if (num_bytes == -1)
        return 0;
    return 1;
}

/**
 * Custom perror for our custom wtf_error
 * 
 * Prints out the error code and error description
 * 
 * If should_exit is 1 then also send exit() command
 */
void wtf_perror(wtf_error e, int should_exit)
{
    printf("\033[0;31m");
    printf("[ Error Code %d ] %s\n", errordesc[e].code, errordesc[e].message);
    printf("\033[0m");

    if (should_exit == 1)
    {
        exit(errordesc[e].code);
    }
}
