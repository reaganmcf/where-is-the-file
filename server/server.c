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
  int port = atoi(argv[1]);
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
  long addr = 0;

  if (!pointer) {
    pthread_exit(0);
  }
  connection = (wtf_connection *)pointer;

  /* Read length of message */
  read(connection->socket, &len, sizeof(int));
  if (len > 0) {
    addr = (long)((struct sockaddr_in *)&connection->address)->sin_addr.s_addr;
    buffer = malloc((len + 1) * sizeof(char));
    buffer[len] = 0;
    read(connection->socket, buffer, len);
    printf("%d.%d.%d.%d: %s\n", (addr)&0xFF, (addr >> 8) & 0xFF, (addr >> 16) & 0xFF, (addr >> 24) & 0xFF, buffer);
  }

  //Handle message sanitization and routing here
  int command_size = atoi(buffer);
  //Shift the start of the buffer until the char after the first : found which is our delim
  while (buffer[0] != ':')
    buffer++;
  buffer++;
  char *command = malloc(command_size + 1);
  command = strncpy(command, buffer, command_size);
  printf("\tcommand = %s\n", command);
  //advance buffer to char after next ':'
  while (buffer[0] != ':')
    buffer++;
  buffer++;

  //Check the command here and route it accordingly
  if (strcmp(command, COMMAND_CREATE_PROJECT) == 0) {
    //Create Project Command

    //Extract project name param from socket
    int project_name_size = atoi(buffer);
    while (buffer[0] != ':')
      buffer++;
    buffer++;
    char *project_name = malloc(project_name_size + 1);
    project_name = strncpy(project_name, buffer, project_name_size);
    int status = wtf_server_create_project(project_name);
    char *ret_buffer = malloc(4);
    sprintf(ret_buffer, "%d", (status + 1) + 100);
    printf("\tSending back {%s} to the client\n", ret_buffer);
    write(connection->socket, ret_buffer, 5);
  } else if (strcmp(command, COMMAND_CURRENT_VERSION_PROJECT) == 0) {
    //Current Version (of a Project) command

    //Extract project name param from socket
    int project_name_size = atoi(buffer);
    while (buffer[0] != ':') buffer++;
    buffer++;
    char *project_name = malloc(project_name_size + 1);
    project_name = strncpy(project_name, buffer, project_name_size);
    int status = wtf_server_get_current_version(project_name);
  }

  //Close socket and cleanup
  close(connection->socket);
  free(connection);
  free(command);
  // free(buffer);
  pthread_exit(0);
}

/**
 * wtf_server_create_project
 * 
 * Handler for create_project directive
 * 
 * Should check if the project name already exists in Projects/, and if it doesn't then create the manifest and send back the status code
 * 
 * Returns 0 if successful, and an E_ERROR_CODE enum otherwise
 */
int wtf_server_create_project(char *project_name) {
  //First we need to loop over the directory ./Projects/ and check if any of the file names already exist
  DIR *d;
  struct dirent *dir;
  d = opendir("./Projects/");
  int name_exists = 0;
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if (!isRegFile(dir->d_name)) {
        if (strcmp(dir->d_name, project_name) == 0)
          name_exists = 1;
      }
      // printf("%s\n", dir->d_name);
    }
    closedir(d);
  } else {
    wtf_perror(E_CANNOT_READ_OR_WRITE_PROJECT_DIR, 0);
    return E_CANNOT_READ_OR_WRITE_PROJECT_DIR;
  }

  if (name_exists == 1) {
    wtf_perror(E_PROJECT_ALREADY_EXISTS, 0);
    return E_CANNOT_READ_OR_WRITE_PROJECT_DIR;
  }

  //Ready to write project manifest to file here
  //First we need to create the directory
  char *path = malloc(100);
  sprintf(path, "./Projects/%s", project_name);
  mkdir(path, 0700);
  sprintf(path, "./Projects/%s/.Manifest", project_name);
  int fd = open(path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    wtf_perror(E_CANNOT_READ_OR_WRITE_PROJECT_DIR, 0);
    return E_CANNOT_READ_OR_WRITE_PROJECT_DIR;
  }

  //write version number 1
  char *init_data = malloc(150);
  sprintf(init_data, "%s\n1", project_name);
  printf("\t attemtping the write\n");
  int num_bytes = write(fd, init_data, strlen(init_data));
  printf("\t write finished \n");
  if (num_bytes <= 0) {
    wtf_perror(E_CANNOT_READ_OR_WRITE_PROJECT_DIR, 0);
    return E_CANNOT_READ_OR_WRITE_PROJECT_DIR;
  }

  printf("\tSuccessfully created .Manifest file for %s\n", project_name);
  free(path);
  close(fd);
  return 0;
}

/**
 * wtf_server_get_current_version
 * 
 * Handler for the get_current_version directive
 * 
 * Should check if the project exists, and if it does, returns a list of all of the files 

/**
 * Custom perror for our custom wtf_error
 * 
 * Prints out the error code and error description
 * 
 * If should_exit is 1 then also send exit() command
 */
void wtf_perror(wtf_error e, int should_exit) {
  printf("\033[0;31m");
  printf("\t[ Error Code %d ] %s\n", errordesc[e].code, errordesc[e].message);
  printf("\033[0m");

  if (should_exit == 1) {
    exit(errordesc[e].code);
  }
}

int isRegFile(const char *path) {
  struct stat statbuf;

  if (stat(path, &statbuf) != 0) {
    return 0;
  }

  return S_ISREG(statbuf.st_mode);
}