#include "server.h"

#include <dirent.h>
#include <fcntl.h>
#include <linux/in.h>
#include <openssl/sha.h>
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

int SOCKET_FD;
pthread_mutex_t lock;

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

  SOCKET_FD = sock;

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

  //setup mutex
  if (pthread_mutex_init(&lock, NULL) != 0) {
    wtf_perror(E_CANNOT_INIT_MUTEX, 1);
  }

  printf("Server has started up Successfully. Listening for Connections...\n");

  atexit(wtf_server_exit_handler);

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

/**
 * Exit Handler for wtf_server
 * 
 * Close ports and sockets and free mem
 * 
 */
static void wtf_server_exit_handler(void) {
  printf("Handling exit safely. Freeing alloc'd memory...\n");
  close(SOCKET_FD);
  pthread_mutex_destroy(&lock);
  printf("Successfully handled exit.\n");
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
    memset(buffer, 0, len + 1);
    buffer[len] = 0;
    read(connection->socket, buffer, len);
    printf("%d.%d.%d.%d: %s\n", (addr)&0xFF, (addr >> 8) & 0xFF, (addr >> 16) & 0xFF, (addr >> 24) & 0xFF, buffer);
  }
  if (len == 0) pthread_exit(0);

  //Handle message sanitization and routing here
  int command_size = atoi(buffer);
  //Shift the start of the buffer until the char after the first : found which is our delim
  while (buffer[0] != ':')
    buffer++;
  buffer++;
  char *command = malloc(command_size + 1);
  memset(command, 0, command_size + 1);
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
    free(project_name);
  } else if (strcmp(command, COMMAND_CURRENT_VERSION_PROJECT) == 0) {
    //Current Version (of a Project) command

    //Extract project name param from socket
    int project_name_size = atoi(buffer);
    while (buffer[0] != ':') buffer++;
    buffer++;
    char *project_name = malloc(project_name_size + 1);
    project_name = strncpy(project_name, buffer, project_name_size);
    char *ret = wtf_server_get_current_version(project_name);
    if (ret[0] == '5') {
      wtf_perror(E_CANNOT_READ_OR_WRITE_PROJECT_DIR, 0);
    } else if (ret[0] == '7') {
      wtf_perror(E_PROJECT_DOESNT_EXIST, 0);
    }
    //Success! Write back the message
    write(connection->socket, ret, strlen(ret));
    memset(ret, 0, 500000);
    free(ret);
    free(project_name);
  } else if (strcmp(command, COMMAND_CREATE_COMMIT) == 0) {
    //Write incoming .Commit
    //Extract byte_size
    int project_name_size = atoi(buffer);
    while (buffer[0] != ':') buffer++;
    buffer++;
    char *project_name = malloc(project_name_size + 1);
    strncpy(project_name, buffer, project_name_size);
    while (buffer[0] != ':') buffer++;
    buffer++;
    int commit_buffer_size = atoi(buffer);
    while (buffer[0] != ':') buffer++;
    buffer++;
    char *commit_buffer = malloc(commit_buffer_size);
    strncpy(commit_buffer, buffer, commit_buffer_size);
    char *status = wtf_server_write_commit(project_name, commit_buffer);
    write(connection->socket, status, 3);

    //free
    free(status);
    free(project_name);
    free(commit_buffer);
  } else if (strcmp(command, COMMAND_CREATE_PUSH) == 0) {
    //Handle incoming commit

    int project_name_size = atoi(buffer);
    while (buffer[0] != ':') buffer++;
    buffer++;
    char *project_name = malloc(project_name_size + 1);
    strncpy(project_name, buffer, project_name_size);
    while (buffer[0] != ':') buffer++;
    buffer++;
    int commit_buffer_size = atoi(buffer);
    while (buffer[0] != ':') buffer++;
    buffer++;
    char *commit_buffer = malloc(commit_buffer_size);
    strncpy(commit_buffer, buffer, commit_buffer_size);
    while (buffer[0] != ':') buffer++;
    buffer++;
    int file_buffer_length = atoi(buffer);
    while (buffer[0] != ':') buffer++;
    buffer++;
    char *file_buffer = malloc(file_buffer_length);
    strncpy(file_buffer, buffer, file_buffer_length);

    printf("\tProject_name = %s\n", project_name);
    printf("\tCommit buffer = %s\n", commit_buffer);
    printf("\tFile buffer = %s\n", file_buffer);

    char *ret = wtf_server_push(project_name, commit_buffer, file_buffer);

    free(project_name);
    free(commit_buffer);
    free(file_buffer);
  }

  //Close socket and cleanup
  close(connection->socket);
  free(connection);
  free(command);
  len = 0;
  printf("\tFinished\n");
  pthread_exit(0);
}

/**
 * wtf_server_push
 * 
 * Handler for push directive
 */
char *wtf_server_push(char *project_name, char *commit_contents, char *files_string) {
  char *buffer = malloc(1000);
  char *buff = malloc(4);
  memset(buffer, 0, 4);
  memset(buffer, 0, 1000);
  sprintf(buffer, "./Projects/%s/.Commit_%s", project_name, hash_string(commit_contents));
  printf("\tChecking if commit doesn't exist\n");

  //lock repo
  pthread_mutex_lock(&lock);

  if (access(buffer, F_OK) == -1) {
    printf("\tCommit doesn't exist, can't push\n");
    sprintf(buff, "101");
    pthread_mutex_unlock(&lock);
    return buff;
  }

  //expire all other commits

  free(buffer);
  pthread_mutex_unlock(&lock);
  return buff;
}

/**
 * wtf_server_write_commit
 * 
 * Handler for create_commit directive
 * 
 * Takes incoming commit buffer and writes it out a .Commit in the project dir.
 * 
 * Returns
 *  0 = Failure
 *  1 = Success
 */
char *wtf_server_write_commit(char *project_name, char *commit) {
  //We need to hash the contents of the commit
  char *buffer = malloc(1000);
  char *buff = malloc(4);
  memset(buff, 0, 4);
  memset(buffer, 0, 1000);
  sprintf(buffer, "./Projects/%s/.Commit_%s", project_name, hash_string(commit));
  printf("\tAttemtping to write %s\n", buffer);

  if (access(buffer, F_OK) != -1) {
    printf("\tFile already exists. No need to write again\n");
    sprintf(buff, "101");
    return buff;
  }

  int fd = open(buffer, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    free(buffer);
    wtf_perror(E_CANNOT_READ_OR_WRITE_NEW_COMMIT, 0);
    sprintf(buff, "10%d", E_CANNOT_READ_OR_WRITE_NEW_COMMIT);
    return buff;
  }
  int n = write(fd, commit, strlen(commit));
  if (n <= 0) {
    free(buffer);
    close(fd);
    wtf_perror(E_CANNOT_READ_OR_WRITE_NEW_COMMIT, 0);
    sprintf(buff, "10%d", E_CANNOT_READ_OR_WRITE_NEW_COMMIT);
    return buff;
  }
  printf("\tSuccessfully created new .Commit\n");

  free(buffer);
  close(fd);
  sprintf(buff, "%d", 101);
  return buff;
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
  //lock mutex
  pthread_mutex_lock(&lock);

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
    pthread_mutex_unlock(&lock);
    return E_CANNOT_READ_OR_WRITE_PROJECT_DIR;
  }

  if (name_exists == 1) {
    wtf_perror(E_PROJECT_ALREADY_EXISTS, 0);
    pthread_mutex_unlock(&lock);
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
    free(path);
    close(fd);
    //unlock
    pthread_mutex_unlock(&lock);
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
    free(path);
    close(fd);
    //unlock
    pthread_mutex_unlock(&lock);
    return E_CANNOT_READ_OR_WRITE_PROJECT_DIR;
  }

  printf("\tSuccessfully created .Manifest file for %s\n", project_name);
  free(path);
  close(fd);

  //unlock
  pthread_mutex_unlock(&lock);
  return 0;
}

/**
 * wtf_server_get_current_version
 * 
 * Handler for the get_current_version directive
 * 
 * Should check if the project exists, and if it does, returns a list of all of the files under the project's name, including their version number
 * 
 * Returns:
 *  "1:<return_string>" = success
 *  "7" = E_PROJECT_DOESNT_EXIST
 *  "5" = E_CANNOT_READ_OR_WRITE_PROJECT_DIR
 */
char *wtf_server_get_current_version(char *project_name) {
  //lock
  pthread_mutex_lock(&lock);

  char *path = malloc(150);
  char *ret_string = malloc(500000);
  sprintf(path, "./Projects/%s/.Manifest", project_name);
  if (access(path, F_OK) == -1) {
    sprintf(ret_string, "%d", E_PROJECT_DOESNT_EXIST);
    pthread_mutex_unlock(&lock);
    free(path);
    return ret_string;
  }

  int manifest_fd = open(path, O_RDONLY);
  if (manifest_fd < 0) {
    close(manifest_fd);
    free(path);
    sprintf(ret_string, "%d", E_CANNOT_READ_OR_WRITE_PROJECT_DIR);
    pthread_mutex_unlock(&lock);
    return ret_string;
  }

  //now we need to read from the manifest
  char *buffer = malloc(10000);
  int n = read(manifest_fd, buffer, strlen(project_name) + 1);
  if (n <= 0) {
    close(manifest_fd);
    free(buffer);
    sprintf(ret_string, "%d", E_CANNOT_READ_OR_WRITE_PROJECT_DIR);
    pthread_mutex_unlock(&lock);
    return ret_string;
  }
  memset(buffer, 0, 10000);
  char *t_number = malloc(100);
  memset(t_number, 0, 100);
  while (buffer[0] != '\n' && n != 0) {
    sprintf(t_number, "%s%c", t_number, buffer[0]);
    n = read(manifest_fd, buffer, 1);
  }
  int project_version = atoi(t_number);
  memset(t_number, 0, 100);
  memset(buffer, 0, 10000);
  memset(ret_string, 0, 500000);
  int file_count = 0;
  char *temp_file_name = malloc(100);
  char *temp_hash = malloc(20 * 2 + 1);  //SHA1 digest length + 1
  int temp_file_version_number = 0;
  while (n != 0) {
    n = read(manifest_fd, buffer, 1);
    if (buffer[0] == '~') {
      memset(temp_file_name, 0, 100);
      temp_file_version_number = 0;
      read(manifest_fd, buffer, 1);
      memset(buffer, 0, 10000);
      while (buffer[0] != ':') {
        strncat(temp_file_name, buffer, 1);
        n = read(manifest_fd, buffer, 1);
      }
      memset(buffer, 0, 10000);
      memset(t_number, 0, 100);
      while (buffer[0] != ':') {
        strncat(t_number, buffer, 1);
        n = read(manifest_fd, buffer, 1);
      }
      temp_file_version_number = atoi(t_number);

      memset(buffer, 0, 10000);
      memset(temp_hash, 0, 20 * 2 + 1);
      while (buffer[0] != ':') {
        strncat(temp_hash, buffer, 1);
        n = read(manifest_fd, buffer, 1);
      }
      // printf("\ttemp_file_name = %s\n", temp_file_name);
      // printf("\ttemp_file_version_number = %d\n", temp_file_version_number);
      // printf("\ttemp_hash = %s\n", temp_hash);
      memset(buffer, 0, 10000);

      sprintf(buffer, "%d:%s:%d:", strlen(temp_file_name), temp_file_name, temp_file_version_number);
      sprintf(buffer, "%s%d:%s:", buffer, strlen(temp_hash), temp_hash);
      // printf("buffer is %s\n", buffer);
      sprintf(ret_string, "%s%s", ret_string, buffer);
      // printf("ret_string is %s\n", ret_string);
      file_count++;
    }
  }
  if (file_count == 0) printf("NO FILES\n");
  char *final_ret_string = malloc(500000);
  memset(final_ret_string, 0, 500000);
  sprintf(final_ret_string, "1:%d:%d:%s", project_version, file_count, ret_string);  //Add 1 to front as we check for success code
  printf("\tSuccessfully built return string\n");
  printf("\tReturn string is: %s\n", final_ret_string);

  free(t_number);
  free(temp_hash);
  free(ret_string);
  free(temp_file_name);
  free(buffer);
  free(path);
  close(manifest_fd);

  pthread_mutex_unlock(&lock);

  return final_ret_string;
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
  printf("\t[ Error Code %d ] %s\n", errordesc[e].code, errordesc[e].message);
  printf("\033[0m\n");

  if (should_exit == 1) {
    exit(errordesc[e].code);
  }
}

/**
 * hash_string
 * 
 * Hashes the given string and returns a string of length SHA_DIGEST_LENGTH * 2 + 1 containing the hash string
 * 
 */
char *hash_string(char *string) {
  SHA_CTX ctx;
  SHA1_Init(&ctx);
  SHA1_Update(&ctx, string, strlen(string));
  unsigned char tmphash[SHA_DIGEST_LENGTH];
  memset(tmphash, 0, SHA_DIGEST_LENGTH);
  SHA1_Final(tmphash, &ctx);
  int i = 0;
  unsigned char *hash = malloc((SHA_DIGEST_LENGTH * 2) + 1);
  memset(hash, 0, SHA_DIGEST_LENGTH * 2);
  for (i = 0; i < SHA_DIGEST_LENGTH; i++) {
    sprintf((char *)&(hash[i * 2]), "%02x", tmphash[i]);
  }

  return hash;
}

/**
 * 
 * isRegFile
 * 
 * checks if the path provided is pointing to a normal file and not a directory
 */
int isRegFile(const char *path) {
  struct stat statbuf;

  if (stat(path, &statbuf) != 0) {
    return 0;
  }

  return S_ISREG(statbuf.st_mode);
}