#include "client.h"

#include <dirent.h>
#include <fcntl.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/** WTF Client
 * 
 * The WTF client will be handling all of the commands for this project, and will send each command to its proper
 * sub function from main to abstract out the code and make it more readable
 */

//Global Declarations
int socket = -1;
wtf_configuration *CONFIGURATION;

int main(int argc, char **argv) {
  atexit(wtf_exit_handler);
  //First, we need to check the params and flags
  if (argc == 1) {
    wtf_perror(E_IMPROPER_PARAMS_AND_FLAGS, 1);
  }

  //Now, lets check send the command to the proper function
  if (strcmp(argv[1], "configure") == 0) {
    //Check for params
    if (argc != 4) {
      wtf_perror(E_IMPROPER_CONFIGURATION_PARAMS, 1);
    } else if (strlen(argv[2]) == 0 || strlen(argv[3]) == 0) {
      wtf_perror(E_IMPROPER_CONFIGURATION_PARAMS, 1);
    }

    int result = wtf_configure_host(argv[2], argv[3]);
    if (result == 1) {
      printf("Succesfully configured client.\n");
      return 0;
    } else {
      wtf_perror(E_CONFIGURATION_WRITE_ERROR, 1);
    }
  }

  int s = wtf_connect();
  return 0;
}

static void wtf_exit_handler(void) {
  printf("Handling exit safely. Freeing alloc'd memory...\n");
  if (CONFIGURATION != NULL) {
    free(CONFIGURATION->hostname);
    free(CONFIGURATION);
  }
  printf("Successfully handled exit.\n");
}

/**
 * Connect to server
 * 
 * Checks if the global CONFIGURATION struct has been populated, if it has not, then loads the contents into the global
 * 
 * After this, it attempts to connect to the server based on the hostname:port given in the .configuration file
 * 
 * Returns:
 *  0 = failure
 *  1 = success
 */
int wtf_connect() {
  //If configuration is not created, we need to read in the configuration file and load it into the struct
  if (CONFIGURATION == NULL) {
    char *buffer = malloc(200);
    int fd = open(WTF_CONFIGURATION_FILE_PATH, O_RDONLY);
    if (fd == -1) {
      wtf_perror(E_INVALID_CONFIGURATION, 1);
    }
    int num_bytes = 0;
    num_bytes = read(fd, buffer, 200);
    // printf("read %d bytes from .configuration\n", num_bytes);
    if (num_bytes <= 0) {
      wtf_perror(E_INVALID_CONFIGURATION, 1);
    }

    //Load the configuration file's contents into 2 variables, port_buffer and hostname_buffer
    CONFIGURATION = malloc(sizeof(wtf_configuration));
    char *port_buffer = strstr(buffer, "\n");  //this now stores \n<port>
    char *hostname_buffer = malloc(100);
    strncpy(hostname_buffer, buffer, port_buffer - buffer);  //load the first substring into hostname_buffer
    port_buffer++;                                           //trim off \n

    CONFIGURATION->hostname = (char *)malloc(strlen(hostname_buffer));
    strncpy(CONFIGURATION->hostname, hostname_buffer, strlen(hostname_buffer));
    CONFIGURATION->port = atoi(port_buffer);

    //free
    close(fd);
    free(buffer);
    free(hostname_buffer);

    printf("Configuration loaded into global: {hostname: '%s', port: %d}\n", CONFIGURATION->hostname, CONFIGURATION->port);
  }

  printf("Attempting to connect to %s:%d\n", CONFIGURATION->hostname, CONFIGURATION->port);
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
int wtf_configure_host(char *hostname, char *port) {
  /**
     * The .configure format is the following  example of my.server.com with port 2503
     * 
     * myserver.com
     * 4433
     * 
     */

  //if .configure already exists, delete it and lets create a new one (to override)
  if (access(WTF_CONFIGURATION_FILE_PATH, F_OK) != -1) {
    remove(WTF_CONFIGURATION_FILE_PATH);
  }
  int fd = open(WTF_CONFIGURATION_FILE_PATH, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
  char *tStr = malloc(500);
  sprintf(tStr, "%s\n%s", hostname, port);
  int num_bytes = write(fd, tStr, strlen(tStr));

  //free memory
  close(fd);
  free(tStr);

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
void wtf_perror(wtf_error e, int should_exit) {
  printf("\033[0;31m");
  printf("[ Error Code %d ] %s\n", errordesc[e].code, errordesc[e].message);
  printf("\033[0m");

  if (should_exit == 1) {
    exit(errordesc[e].code);
  }
}
