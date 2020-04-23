#include "client.h"

#include <fcntl.h>
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
wtf_configuration *CONFIGURATION = NULL;

int main(int argc, char **argv)
{
  atexit(wtf_exit_handler);
  //First, we need to check the params and flags
  if (argc == 1)
  {
    wtf_perror(E_IMPROPER_PARAMS_AND_FLAGS, 1);
  }

  char *command = argv[1];

  //Now, lets check send the command to the proper function
  if (strcmp(command, "configure") == 0)
  {
    //Check for params
    if (argc != 4)
    {
      wtf_perror(E_IMPROPER_CONFIGURATION_PARAMS, 1);
    }

    char *hostname = argv[2];
    char *port = argv[3];
    if (strlen(hostname) == 0 || strlen(port) == 0)
    {
      wtf_perror(E_IMPROPER_CONFIGURATION_PARAMS, 1);
    }

    int result = wtf_configure_host(hostname, port);
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
  else if (strcmp(command, "create") == 0)
  {
    //Check for params
    if (argc != 3)
    {
      wtf_perror(E_IMPROPER_CREATE_PARAMS, 1);
    }
    char *project_name = argv[2];
    if (strlen(project_name) == 0 || strlen(project_name) >= 100)
    {
      wtf_perror(E_IMPROPER_CREATE_PROJECT_NAME_LENGTH, 1);
    }

    //Check that the project name does not contain : which is our delimeter
    char *temp = argv[2];
    int safe = 1;
    while (temp[0] != '\0')
    {
      if (temp[0] == ':')
        safe = 0;
      temp++;
    }

    if (safe == 0)
    {
      wtf_perror(E_IMPROPER_CREATE_PROJECT_NAME, 1);
    }

    //All ready, create a connection handler and call server
    int result = wtf_create_project(project_name);
  }

  // wtf_connection *connection = wtf_connect();
  // char *testString = "hello gamers";
  // write(connection->socket, &testString, sizeof(int));
  // write(connection->socket, testString, strlen(testString));
  return 0;
}

static void wtf_exit_handler(void)
{
  printf("Handling exit safely. Freeing alloc'd memory...\n");
  if (CONFIGURATION != NULL)
  {
    free(CONFIGURATION->hostname);
    free(CONFIGURATION);
  }
  printf("Successfully handled exit.\n");
}

/**
 * Create project
 * 
 * Creates a project on the server, and writes the appropriate .Manifest that the server sends back
 *
 * All errors are handled in here, no need to throw errors if it returns 0
 * 
 * Returns:
 *  0 = Failure
 *  1 = Success
 */
int wtf_create_project(char *project_name)
{
  //Establish connection to the server
  wtf_connection *connection = wtf_connect();
  char *buffer = malloc(sizeof(100));
  sprintf(buffer, "14:create_project:%d:%s", strlen(project_name) + 1, project_name);
  int msg_size = strlen(buffer) + 1;
  printf("Sending {%s} to the client (%d) bytes total\n", buffer, msg_size);
  write(connection->socket, &msg_size, sizeof(int));
  write(connection->socket, buffer, strlen(buffer) + 1);
  int n = read(connection->socket, buffer, 4);
  int ret_status = atoi(buffer);
  if (ret_status == 101)
  {
    printf("Successfully created Project Manifest on server.\n");

    //Now we need to create the manifest on the client side
    int fd = open(".Manifest", O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
      wtf_perror(E_CANNOT_WRITE_TO_MANIFEST, 1);
    }
    write(fd, project_name, strlen(project_name));
    n = write(fd, "\n1.0", 4);
    if (n < 1)
    {
      wtf_perror(E_CANNOT_WRITE_TO_MANIFEST, 1);
    }

    close(fd);
    printf("Successfully created .Manifest\n");
  }
  else
  {
    if (ret_status == 105)
    {
      wtf_perror(E_SERVER_IMPROPER_PERMISSIONS, 0);
    }
    else
    {
      wtf_perror(E_SERVER_MANIFEST_ALREADY_EXISTS, 0);
    }
  }

  free(connection);
  //TODO free buffer without crashing
}

/**
 * Connect to server
 * 
 * Checks if the global CONFIGURATION struct has been populated, if it has not, then loads the contents into the global
 * 
 * After this, it attempts to connect to the server based on the hostname:port given in the .configuration file
 * 
 * Returns:
 *  wtf_connection* = success
 *  Failure will throw an exit-provoking wtf_error, so you can expect return to be valid 
 */
wtf_connection *wtf_connect()
{
  //If configuration is not created, we need to read in the configuration file and load it into the struct
  if (CONFIGURATION == NULL)
  {
    char *buffer = malloc(200);
    int fd = -1;
    fd = open(WTF_CONFIGURATION_FILE_PATH, O_RDONLY);
    if (fd == -1)
    {
      wtf_perror(E_INVALID_CONFIGURATION, 1);
    }
    int num_bytes = 0;
    num_bytes = read(fd, buffer, 200);
    // printf("read %d bytes from .configuration\n", num_bytes);
    if (num_bytes <= 0)
    {
      wtf_perror(E_INVALID_CONFIGURATION, 1);
    }

    //Load the configuration file's contents into 2 variables, port_buffer and hostname_buffer
    CONFIGURATION = malloc(sizeof(wtf_configuration));
    char *port_buffer = strstr(buffer, "\n"); //this now stores \n<port>
    char *hostname_buffer = malloc(100);
    strncpy(hostname_buffer, buffer, port_buffer - buffer); //load the first substring into hostname_buffer
    port_buffer++;                                          //trim off \n

    CONFIGURATION->hostname = (char *)malloc(strlen(hostname_buffer) + 1);
    strncpy(CONFIGURATION->hostname, hostname_buffer, strlen(hostname_buffer) + 1);
    CONFIGURATION->port = atoi(port_buffer);

    //free
    close(fd);
    free(buffer);
    free(hostname_buffer);

    printf("Configuration loaded into global: {hostname: '%s', port: %d}\n", CONFIGURATION->hostname, CONFIGURATION->port);
  }

  printf("Attempting to connect to %s:%d\n", CONFIGURATION->hostname, CONFIGURATION->port);

  wtf_connection *connection = malloc(sizeof(wtf_connection));
  connection->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socket <= 0)
  {
    wtf_perror(E_CANNOT_CREATE_SOCKET, 1);
  }
  connection->address.sin_family = AF_INET;
  connection->address.sin_port = htons(CONFIGURATION->port);
  connection->host = gethostbyname(CONFIGURATION->hostname);
  if (connection->host == NULL)
  {
    wtf_perror(E_UNKNOWN_HOST, 1);
  }
  memcpy(&connection->address.sin_addr, connection->host->h_addr_list[0], connection->host->h_length);

  if (connect(connection->socket, (struct sockaddr *)&connection->address, sizeof(connection->address)))
  {
    wtf_perror(E_CANNOT_CONNECT_TO_HOST, 1);
  }

  //Socket is connected and everything, return it.
  return connection;
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
     * The .configure format is the following  example of my.server.com with port 2503
     * 
     * myserver.com
     * 4433
     * 
     */

  //if .configure already exists, delete it and lets create a new one (to override)
  if (access(WTF_CONFIGURATION_FILE_PATH, F_OK) != -1)
  {
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
