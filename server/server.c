#include "server.h"

#include <dirent.h>
#include <fcntl.h>
#include <linux/in.h>
#include <openssl/sha.h>
#include <pthread.h>
#include <signal.h>
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
RepositoryLock *HEAD_REPOSITORY_LOCK = NULL;

int main(int argc, char **argv) {
  //Check if only a port is passed in as a param as it should
  if (argc == 1 || argc > 2) {
    wtf_perror(E_IMPROPER_PARAMS, FATAL_ERROR);
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
    wtf_perror(E_ERROR_MAKING_SOCKET, FATAL_ERROR);
  }

  SOCKET_FD = sock;

  //Bind socket to port
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);
  if (bind(sock, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) < 0) {
    wtf_perror(E_ERROR_BINDING_SOCKET_TO_PORT, FATAL_ERROR);
  }

  if (listen(sock, 100) < 0) {
    wtf_perror(E_CANNOT_LISTEN_TO_PORT, FATAL_ERROR);
  }

  //exit handler
  atexit(wtf_server_exit_handler);
  signal(SIGINT, sigintHandler);

  //Initialize repo locks
  DIR *d;
  struct dirent *dir;
  d = opendir("./Projects/");
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if (!isRegFile(dir->d_name) && strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
        RepositoryLock *new_lock = malloc(sizeof(RepositoryLock));
        memset(new_lock, 0, sizeof(new_lock));
        new_lock->next = NULL;
        new_lock->project_name = malloc(strlen(dir->d_name) + 1);
        memset(new_lock->project_name, 0, strlen(dir->d_name) + 1);
        strcpy(new_lock->project_name, dir->d_name);
        pthread_mutex_init(&(new_lock->lock), NULL);
        RepositoryLock *curr = HEAD_REPOSITORY_LOCK;
        if (curr == NULL) {
          HEAD_REPOSITORY_LOCK = new_lock;
        } else {
          while (curr->next != NULL) {
            curr = curr->next;
          }
          curr->next = new_lock;
        }
      }
    }
    closedir(d);
  } else {
    wtf_perror(E_CANNOT_READ_OR_WRITE_PROJECT_DIR, NON_FATAL_ERROR);
    return E_CANNOT_READ_OR_WRITE_PROJECT_DIR;
  }

  printf("Server has started up Successfully. Listening for Connections...\n");

  printf("all repo locks:\n");
  RepositoryLock *curr = HEAD_REPOSITORY_LOCK;
  while (curr != NULL) {
    printf("\t%s %p\n", curr->project_name, &curr->lock);
    curr = curr->next;
  }

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
 * sigint handler, will call wtf_server_exit_handler
 */
void sigintHandler(int signum) {
  wtf_server_exit_handler();
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

  //destroy and free locks
  RepositoryLock *curr = HEAD_REPOSITORY_LOCK;
  RepositoryLock *next;
  while (curr != NULL) {
    pthread_mutex_destroy(&(curr->lock));
    free(curr->project_name);
    next = curr->next;
    free(curr);
    curr = next;
  }

  printf("Successfully handled exit.\n");
  _exit(0);
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
    write(connection->socket, ret_buffer, 4);
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
      wtf_perror(E_CANNOT_READ_OR_WRITE_PROJECT_DIR, NON_FATAL_ERROR);
    } else if (ret[0] == '7') {
      wtf_perror(E_PROJECT_DOESNT_EXIST, NON_FATAL_ERROR);
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
    printf("commit_buffer_size is %d\n", commit_buffer_size);
    printf("buffer is %s\n", buffer);
    char *commit_buffer = malloc(commit_buffer_size + 1);
    memset(commit_buffer, 0, commit_buffer_size + 1);
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
    memset(project_name, 0, project_name_size + 1);
    strncpy(project_name, buffer, project_name_size);
    while (buffer[0] != ':') buffer++;
    buffer++;
    int commit_buffer_size = atoi(buffer);
    while (buffer[0] != ':') buffer++;
    buffer++;
    char *commit_buffer = malloc(commit_buffer_size + 1);
    memset(commit_buffer, 0, commit_buffer_size + 1);
    strncpy(commit_buffer, buffer, commit_buffer_size);
    while (buffer[0] != ':') buffer++;
    buffer++;
    int file_buffer_length = atoi(buffer);
    while (buffer[0] != ':') buffer++;
    buffer++;
    char *file_buffer = malloc(file_buffer_length + 1);
    memset(file_buffer, 0, file_buffer_length + 1);
    strncpy(file_buffer, buffer, file_buffer_length);

    printf("\tProject_name = %s\n", project_name);
    printf("\tCommit buffer = %s\n", commit_buffer);
    printf("\tFile buffer = %s\n", file_buffer);

    char *ret = wtf_server_push(project_name, commit_buffer, file_buffer);
    write(connection->socket, ret, 3);

    free(project_name);
    free(commit_buffer);
    free(file_buffer);
  } else if (strcmp(command, COMMAND_GET_HISTORY) == 0) {
    //Handle incoming message
    int project_name_size = atoi(buffer);
    while (buffer[0] != ':') buffer++;
    buffer++;
    char *project_name = malloc(project_name_size + 1);
    memset(project_name, 0, project_name_size + 1);
    strncpy(project_name, buffer, project_name_size);
    //go ahead and call function handler
    char *ret = wtf_server_get_history(project_name);
    write(connection->socket, ret, strlen(ret));

    free(ret);
    free(project_name);
  } else if (strcmp(command, COMMAND_DESTORY_PROJECT) == 0) {
    //Handle incoming message
    int project_name_size = atoi(buffer);
    while (buffer[0] != ':') buffer++;
    buffer++;
    char *project_name = malloc(project_name_size + 1);
    memset(project_name, 0, project_name_size + 1);
    strncpy(project_name, buffer, project_name_size);
    int status = wtf_server_destroy_project(project_name);
    char *ret_buffer = malloc(3);
    memset(ret_buffer, 0, 3);
    sprintf(ret_buffer, "%d", status);
    printf("\tSending back {%s} to the client\n", ret_buffer);
    write(connection->socket, ret_buffer, 3);
    free(project_name);
    free(ret_buffer);
  } else if (strcmp(command, COMMAND_ROLLBACK_PROJECT) == 0) {
    //Handle incoming message
    int project_name_size = atoi(buffer);
    while (buffer[0] != ':') buffer++;
    buffer++;
    char *project_name = malloc(project_name_size + 1);
    memset(project_name, 0, project_name_size + 1);
    strncpy(project_name, buffer, project_name_size);
    while (buffer[0] != ':') buffer++;
    buffer++;
    int version_number_size = atoi(buffer);
    while (buffer[0] != ':') buffer++;
    buffer++;
    char *version_number_string = malloc(version_number_size + 1);
    memset(version_number_string, 0, version_number_size + 1);
    strncpy(version_number_string, buffer, version_number_size);
    int version_number = atoi(version_number_string);

    printf("\trollback %s %d\n", project_name, version_number);
    int status = wtf_server_rollback_project(project_name, version_number);
    char *ret_buffer = malloc(3);
    memset(ret_buffer, 0, 3);
    sprintf(ret_buffer, "%d", status);
    write(connection->socket, ret_buffer, 3);
    free(ret_buffer);
    free(project_name);
    free(version_number_string);
  } else if (strcmp(command, COMMAND_GET_FILE_CONTENTS) == 0) {
    //Handle incoming message
    int file_path_size = atoi(buffer);
    while (buffer[0] != ':') buffer++;
    buffer++;
    char *file_path = malloc(file_path_size + 1);
    memset(file_path, 0, file_path_size + 1);
    strncpy(file_path, buffer, file_path_size);
    char *ret = wtf_server_get_file_contents(file_path);
    printf("\tSending back {%s} to the client\n", ret);
    write(connection->socket, ret, strlen(ret));
    free(file_path);
    free(ret);
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
 * wtf_server_get_file_contents
 * 
 * sends back the file contents of a specified file path
 * 
 * Returns:
 *  "1:<data>" = Success
 *  "5" = E_CANNOT_READ_OR_WRITE_PROJECT_DIR
 *  "11" = E_FILE_DOESNT_EXIST
 */
char *wtf_server_get_file_contents(char *file_path) {
  char *buffer = malloc(1000);
  char *ret_buffer = malloc(500000);
  char *file_buffer = malloc(450000);
  memset(buffer, 0, 1000);
  memset(ret_buffer, 0, 500000);
  memset(file_buffer, 0, 450000);
  sprintf(buffer, "./Projects/%s", file_path);

  //Check if the file exists
  if (access(buffer, F_OK) == -1) {
    free(buffer);
    wtf_perror(E_FILE_DOESNT_EXIST, NON_FATAL_ERROR);
    sprintf(ret_buffer, "%d", E_FILE_DOESNT_EXIST);
    return ret_buffer;
  }

  //Open the file
  int fd = open(buffer, O_RDONLY);
  if (fd == -1) {
    free(buffer);
    sprintf(ret_buffer, "%d", E_CANNOT_READ_OR_WRITE_PROJECT_DIR);
    return ret_buffer;
  }

  memset(buffer, 0, 1000);
  free(buffer);
  buffer = NULL;
  buffer = malloc(2);  //only need buffer to be 2 bytes long since we are going to do lots of memsets
  memset(buffer, 0, 2);
  int n = read(fd, buffer, 1);
  while (n != 0) {
    sprintf(file_buffer, "%s%c", file_buffer, buffer[0]);
    n = read(fd, buffer, 1);
  }

  //file is fully read, build buffer
  memset(ret_buffer, 0, 500000);
  sprintf(ret_buffer, "1:%d:%s", strlen(file_buffer), file_buffer);
  printf("\tSuccessfully read %d bytes from %s\n", strlen(file_buffer), file_path);
  return ret_buffer;
}

/**
 * wtf_server_rollback_project
 * 
 * Handler for the rollback_project directive
 * 
 * Rolls back the project to a particular version number by doing the following:
 *  - delete all other files besides <project_name>_<version_number>.gz
 *  - expand <project_name>_<version_number>.gz into the current dir
 * 
 * Returns
 *  1 = Success
 *  5 = E_CANNOT_READ_OR_WRITE_PROJECT_DIR
 *  7 = E_PROJECT_DOESNT_EXIST
 *  10 = E_PROJECT_VERSION_DOESNT_EXIST
 */
int wtf_server_rollback_project(char *project_name, int version_number) {
  lock_repository(project_name);
  //First loop over directory and check if the project actually exists
  DIR *d;
  struct dirent *dir;
  d = opendir("./Projects/");
  int name_exists = 0;
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if (!isRegFile(dir->d_name)) {
        if (strcmp(dir->d_name, project_name) == 0) {
          name_exists = 1;
        }
      }
    }
    closedir(d);
  } else {
    wtf_perror(E_CANNOT_READ_OR_WRITE_PROJECT_DIR, NON_FATAL_ERROR);
    unlock_repository(project_name);
    return E_CANNOT_READ_OR_WRITE_PROJECT_DIR;
  }

  if (name_exists == 0) {
    wtf_perror(E_PROJECT_DOESNT_EXIST, NON_FATAL_ERROR);
    unlock_repository(project_name);
    return E_PROJECT_DOESNT_EXIST;
  }

  //Check that the version number provided is > current version number
  Manifest *manifest = fetch_manifest(project_name);
  if (manifest->version_number <= version_number) {
    wtf_perror(E_PROJECT_VERSION_DOESNT_EXIST, NON_FATAL_ERROR);
    unlock_repository(project_name);
    free_manifest(manifest);
    return E_PROJECT_VERSION_DOESNT_EXIST;
  }

  //All good to go

  //move history out of the dir
  char *buffer = malloc(200);
  memset(buffer, 0, 200);
  sprintf(buffer, "mv ./Projects/%s/.History ./Projects", project_name);
  system(buffer);

  //move the backup out of the dir
  memset(buffer, 0, 200);
  sprintf(buffer, "mv ./Projects/%s/%s_%d.gz ./Projects/", project_name, project_name, version_number);
  system(buffer);

  //delete all files inside the project folder
  memset(buffer, 0, 200);
  sprintf(buffer, "rm -r ./Projects/%s", project_name);
  system(buffer);

  //create the project folder again
  memset(buffer, 0, 200);
  sprintf(buffer, "mkdir ./Projects/%s", project_name);
  system(buffer);

  //move back the backup
  memset(buffer, 0, 200);
  sprintf(buffer, "mv ./Projects/%s_%d.gz ./Projects/%s/", project_name, version_number, project_name);
  system(buffer);

  //untar the backup
  memset(buffer, 0, 200);
  sprintf(buffer, "tar -xf ./Projects/%s/%s_%d.gz -C ./Projects/", project_name, project_name, version_number, project_name);
  system(buffer);

  //move files in untar to current dir
  memset(buffer, 0, 200);
  sprintf(buffer, "cp -rf ./Projects/Projects/%s_%d/. ./Projects/%s/", project_name, version_number, project_name);
  system(buffer);

  //delete untar dirs
  memset(buffer, 0, 200);
  sprintf(buffer, "rm -r ./Projects/Projects/", buffer);
  system(buffer);

  //move .History back
  memset(buffer, 0, 200);
  sprintf(buffer, "mv ./Projects/.History ./Projects/%s/.History", project_name);
  system(buffer);

  //delete backup
  memset(buffer, 0, 200);
  sprintf(buffer, "rm ./Projects/%s/%s_%d.gz", project_name, project_name, version_number);
  system(buffer);

  //write to history
  memset(buffer, 0, 200);
  sprintf(buffer, "./Projects/%s/.History", project_name);
  int fd = open(buffer, O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    printf("\tcannot create or open .History for some reason\n");
  } else {
    memset(buffer, 0, 200);
    sprintf(buffer, "Rolled back to version %d\n", version_number);
    printf("Writing out to .History %s\n", buffer);
    write(fd, buffer, strlen(buffer));
  }
  close(fd);

  printf("Successfully rolled back project\n");
  free(buffer);
  unlock_repository(project_name);
  free_manifest(manifest);

  return 1;
}

/**
 * wtf_server_destroy_project
 * 
 * Handler for the destroy_project directive
 * 
 * deletes project folder and all subfolders, etc
 *
 * Returns
 *  1 = Success
 *  5 = E_CANNOT_READ_OR_WRITE_PROJECT_DIR
 *  7 = E_PROJECT_DOESNT_EXIST
 */
int wtf_server_destroy_project(char *project_name) {
  //lock mutex
  lock_repository(project_name);

  //First loop over directory and check if the project actually exists
  DIR *d;
  struct dirent *dir;
  d = opendir("./Projects/");
  int name_exists = 0;
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if (!isRegFile(dir->d_name)) {
        if (strcmp(dir->d_name, project_name) == 0) {
          name_exists = 1;
        }
      }
    }
    closedir(d);
  } else {
    wtf_perror(E_CANNOT_READ_OR_WRITE_PROJECT_DIR, NON_FATAL_ERROR);
    unlock_repository(project_name);
    return E_CANNOT_READ_OR_WRITE_PROJECT_DIR;
  }

  if (name_exists == 0) {
    wtf_perror(E_PROJECT_DOESNT_EXIST, NON_FATAL_ERROR);
    unlock_repository(project_name);
    return E_PROJECT_DOESNT_EXIST;
  }

  //Confirmed project actually exists, time to load buffer and run command
  char *buffer = malloc(200);
  memset(buffer, 0, 200);
  sprintf(buffer, "rm -r ./Projects/%s", project_name);
  system(buffer);

  free(buffer);
  unlock_repository(project_name);
  return 1;
}

/**
 * wtf_server_get_history
 * 
 * Handler for the get_history directive
 * 
 * fetches the project's .History file and returns a string containing its contents
 */
char *wtf_server_get_history(char *project_name) {
  lock_repository(project_name);
  char *buffer = malloc(200);
  char *mid_buffer = malloc(99999);
  char *ret = malloc(100000);
  memset(ret, 0, 100000);
  memset(mid_buffer, 0, 99999);
  memset(buffer, 0, 200);
  sprintf(buffer, "./Projects/%s", project_name);

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
    wtf_perror(E_CANNOT_READ_OR_WRITE_PROJECT_DIR, NON_FATAL_ERROR);
    sprintf(ret, "2");
    free(buffer);
    free(mid_buffer);
    unlock_repository(project_name);
    return ret;
  }

  if (name_exists == 0) {
    wtf_perror(E_PROJECT_DOESNT_EXIST, NON_FATAL_ERROR);
    sprintf(ret, "3");
    free(buffer);
    free(mid_buffer);
    unlock_repository(project_name);
    return ret;
  }

  memset(buffer, 0, 200);
  sprintf(buffer, "./Projects/%s/.History", project_name);
  if (isRegFile(buffer) == 0) {
    //no history is fine, we just need to return a string saying no history
    char *t = "Repository has no history yet";
    sprintf(ret, "1:%d:%s", strlen(t), t);
    free(mid_buffer);
    free(buffer);
    unlock_repository(project_name);
    return ret;
  }

  printf("\tattemtping to read %s\n", buffer);
  int fd = open(buffer, O_RDWR);

  if (fd == -1) {
    wtf_perror(E_CANNOT_READ_OR_WRITE_PROJECT_DIR, NON_FATAL_ERROR);
    sprintf(ret, "2");
    free(buffer);
    free(mid_buffer);
    unlock_repository(project_name);
    return ret;
  }
  memset(buffer, 0, 2);
  int n = read(fd, buffer, 1);
  while (n != 0) {
    sprintf(mid_buffer, "%s%c", mid_buffer, buffer[0]);
    n = read(fd, buffer, 1);
  }

  sprintf(ret, "1:%d:%s", strlen(mid_buffer), mid_buffer);
  free(buffer);
  free(mid_buffer);
  close(fd);
  unlock_repository(project_name);
  return ret;
}

/**
 * wtf_server_push
 * 
 * Handler for push directive
 */
char *wtf_server_push(char *project_name, char *commit_contents, char *files_string) {
  char *buffer = malloc(1000);
  char *mid_buffer = malloc(1000);
  char *target_commit_file_name = malloc(500);
  char *buff = malloc(4);
  memset(buff, 0, 4);
  memset(buffer, 0, 1000);
  memset(mid_buffer, 0, 1000);
  memset(target_commit_file_name, 0, 500);
  sprintf(target_commit_file_name, ".Commit_%s", hash_string(commit_contents));
  sprintf(buffer, "./Projects/%s/%s", project_name, target_commit_file_name);
  printf("\tChecking if commit doesn't exist\n");

  //lock repo
  lock_repository(project_name);

  printf("\tServer should have %s\n", target_commit_file_name);
  if (access(buffer, F_OK) == -1) {
    printf("\tCommit doesn't exist, can't push\n");
    sprintf(buff, "101");
    unlock_repository(project_name);
    return buff;
  }

  //expire all other commits
  DIR *d;
  struct dirent *dir;
  memset(buffer, 0, 1000);
  sprintf(buffer, "./Projects/%s", project_name);
  d = opendir(buffer);
  int name_exists = 0;
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      memset(buffer, 0, 1000);
      sprintf(mid_buffer, "./Projects/%s/%s", project_name, dir->d_name);
      memset(buffer, 0, 1000);
      if (isRegFile(mid_buffer)) {
        memset(buffer, 0, 1000);
        strncpy(buffer, dir->d_name, strlen(".Commit"));
        if (strcmp(buffer, ".Commit") == 0 && strcmp(dir->d_name, target_commit_file_name) != 0) {
          printf("\tShould expire %s\n", dir->d_name);
          remove(mid_buffer);
        }
      }
    }
    closedir(d);
  } else {
    free(mid_buffer);
    free(buffer);
    unlock_repository(project_name);
    wtf_perror(E_CANNOT_READ_OR_WRITE_PROJECT_DIR, NON_FATAL_ERROR);
    sprintf(buff, "10%d", E_CANNOT_READ_OR_WRITE_PROJECT_DIR);
  }

  Manifest *manifest = fetch_manifest(project_name);
  print_manifest(manifest, 0);
  int version_number = manifest->version_number;

  //delete current commit now that commit ops are made
  memset(buffer, 0, 1000);
  sprintf(buffer, "rm ./Projects/%s/%s", project_name, target_commit_file_name);
  printf("%s\n", buffer);
  system(buffer);

  //Now that all other commits are expired, lets duplicate the repo and append the version number to it so we can still access it later in rollback

  //Duplicate repo
  sprintf(buffer, "cp -R ./Projects/%s/ ./Projects/%s_%d", project_name, project_name, version_number);
  printf("%s\n", buffer);
  system(buffer);

  //Tar copy
  memset(buffer, 0, 1000);
  sprintf(buffer, "tar -cf ./Projects/%s_%d.gz ./Projects/%s_%d", project_name, version_number, project_name, version_number);
  printf("%s\n", buffer);
  system(buffer);

  //Remove copy
  memset(buffer, 0, 1000);
  sprintf(buffer, "rm -r ./Projects/%s_%d", project_name, version_number);
  printf("%s\n", buffer);
  system(buffer);

  //Move tar back into project dir
  memset(buffer, 0, 1000);
  sprintf(buffer, "mv ./Projects/%s_%d.gz ./Projects/%s/", project_name, version_number, project_name, project_name);
  printf("%s\n", buffer);
  system(buffer);

  //Now that copy is made, read over commit message and construct message out of it

  int i = 0;
  int j = 0;
  char opcode;
  char *commit_copy = malloc(strlen(commit_contents) + 1);
  char *files_copy = malloc(strlen(files_string) + 1);
  strcpy(commit_copy, commit_contents);
  strcpy(files_copy, files_string);
  int commit_op_count = 0;
  int non_delete_commit_op_count = 0;
  CommitOperation **commit_ops = malloc(sizeof(CommitOperation *) * 1000);

  while (commit_copy[0] != 0) {
    if (commit_copy[0] == OPCODE_ADD || commit_copy[0] == OPCODE_DELETE || commit_copy[0] == OPCODE_MODIFY) {
      opcode = commit_copy[0];
      if (opcode == OPCODE_ADD || opcode == OPCODE_MODIFY) non_delete_commit_op_count++;
      if (commit_copy[1] == ' ') {
        //entry starts here
        commit_copy += 2;
        commit_ops[commit_op_count] = malloc(sizeof(CommitOperation));
        commit_ops[commit_op_count]->op_code = opcode;
        memset(buffer, 0, 1000);
        while (commit_copy[0] != ' ') {
          sprintf(buffer, "%s%c", buffer, commit_copy[0]);
          commit_copy++;
        }
        commit_ops[commit_op_count]->file_path = malloc(strlen(buffer) + 1);
        memset(commit_ops[commit_op_count]->file_path, 0, strlen(buffer) + 1);
        strcpy(commit_ops[commit_op_count]->file_path, buffer);

        if (strlen(files_copy) != 0) {
          files_copy += strlen(commit_ops[commit_op_count]->file_path) + 1;  //advance files string until number designating length of file contents
          memset(buffer, 0, 1000);
          while (files_copy[0] != ':') {
            sprintf(buffer, "%s%c", buffer, files_copy[0]);
            files_copy++;
          }
          files_copy++;
        }
        int file_contents_length = atoi(buffer);
        commit_ops[commit_op_count]->contents = malloc(file_contents_length + 1);
        memset(commit_ops[commit_op_count]->contents, 0, file_contents_length + 1);
        for (i = 0; i < file_contents_length; i++) {
          sprintf(commit_ops[commit_op_count]->contents, "%s%c", commit_ops[commit_op_count]->contents, files_copy[0]);
          files_copy++;
        }
        files_copy++;
        commit_op_count++;
      }
    }

    commit_copy++;
  }

  printf("\tall commit ops made\n");

  memset(buffer, 0, 1000);
  sprintf(buffer, "./Projects/%s/.History", project_name);
  int history_fd = open(buffer, O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR);
  memset(buffer, 0, 1000);
  sprintf(buffer, "%d\n", manifest->version_number + 1);
  write(history_fd, buffer, strlen(buffer));

  //create new manifest that will overwrite old one
  Manifest *new_manifest = malloc(sizeof(Manifest));
  new_manifest->version_number = manifest->version_number + 1;
  new_manifest->project_name = malloc(strlen(manifest->project_name) + 1);
  strcpy(new_manifest->project_name, manifest->project_name);
  new_manifest->file_count = non_delete_commit_op_count;
  new_manifest->files = malloc(sizeof(ManifestFileEntry *) * non_delete_commit_op_count);

  //CommitOperations built here and ready to be applied
  int fd;
  for (i = 0; i < commit_op_count; i++) {
    printf("%c %s %s\n", commit_ops[i]->op_code, commit_ops[i]->file_path, commit_ops[i]->contents);
    memset(buffer, 0, 1000);
    sprintf(buffer, "%c %s\n", commit_ops[i]->op_code, commit_ops[i]->file_path);
    write(history_fd, buffer, strlen(buffer));
    memset(buffer, 0, 1000);
    sprintf(buffer, "./Projects/%s", commit_ops[i]->file_path);
    if (commit_ops[i]->op_code == OPCODE_ADD || commit_ops[i]->op_code == OPCODE_MODIFY) {
      printf("\tadding/modifying\n");
      new_manifest->files[i] = malloc(sizeof(ManifestFileEntry));
      new_manifest->files[i]->file_path = malloc(strlen(commit_ops[i]->file_path) + 1);
      strcpy(new_manifest->files[i]->file_path, commit_ops[i]->file_path);
      new_manifest->files[i]->hash = malloc(SHA_DIGEST_LENGTH * 2 + 1);
      memset(new_manifest->files[i]->hash, 0, SHA_DIGEST_LENGTH * 2 + 1);
      printf("going to be hasing %s\n", commit_ops[i]->contents);
      new_manifest->files[i]->hash = hash_string(commit_ops[i]->contents);
      new_manifest->files[i]->op_code = OPCODE_NONE;
      new_manifest->files[i]->seen_by_server = 1;

      //if modify, delete the file and we will write it again
      if (commit_ops[i]->op_code == OPCODE_MODIFY) {
        remove(buffer);
      }
      //create the file and write to it
      //we need to pull out the dir tree path first
      char *dir_paths = malloc(1000);
      memset(dir_paths, 0, 1000);
      int k = 0;
      int slash_index = 0;
      while (commit_ops[i]->file_path[k] != 0) {
        if (commit_ops[i]->file_path[k] == '/')
          slash_index = k;
        k++;
      }
      strncpy(dir_paths, commit_ops[i]->file_path, slash_index + 1);
      printf("\tdir subtree is %s\n", dir_paths);

      memset(mid_buffer, 0, 1000);
      sprintf(mid_buffer, "mkdir -p ./Projects/%s", dir_paths);
      system(mid_buffer);
      free(dir_paths);
      memset(mid_buffer, 0, 1000);
      sprintf(mid_buffer, 0, "touch %s", commit_ops[i]->file_path);
      system(mid_buffer);

      fd = open(buffer, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
      if (fd == -1) {
        printf("couldn't create file %s\n", commit_ops[i]->file_path);
      }
      write(fd, commit_ops[i]->contents, strlen(commit_ops[i]->contents));
      close(fd);
    } else {
      //has to be delete
      memset(buffer, 0, 1000);
      sprintf(buffer, "rm %s", commit_ops[i]->file_path);
      system(buffer);

      //sanitize project
      sanitize_project(project_name);
    }
  }

  close(history_fd);

  //write new manifest
  write_manifest(new_manifest);

  free_manifest(manifest);
  free(mid_buffer);
  free(buffer);
  unlock_repository(project_name);
  sprintf(buff, "101");
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
  printf("\tcommit is %s\n", commit);
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
    wtf_perror(E_CANNOT_READ_OR_WRITE_NEW_COMMIT, NON_FATAL_ERROR);
    sprintf(buff, "10%d", E_CANNOT_READ_OR_WRITE_NEW_COMMIT);
    return buff;
  }
  int n = write(fd, commit, strlen(commit));
  if (n <= 0) {
    free(buffer);
    close(fd);
    wtf_perror(E_CANNOT_READ_OR_WRITE_NEW_COMMIT, NON_FATAL_ERROR);
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
  lock_repository(project_name);

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
    wtf_perror(E_CANNOT_READ_OR_WRITE_PROJECT_DIR, NON_FATAL_ERROR);
    unlock_repository(project_name);
    return E_CANNOT_READ_OR_WRITE_PROJECT_DIR;
  }

  if (name_exists == 1) {
    wtf_perror(E_PROJECT_ALREADY_EXISTS, NON_FATAL_ERROR);
    unlock_repository(project_name);
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
    wtf_perror(E_CANNOT_READ_OR_WRITE_PROJECT_DIR, NON_FATAL_ERROR);
    free(path);
    close(fd);
    //unlock
    unlock_repository(project_name);
    return E_CANNOT_READ_OR_WRITE_PROJECT_DIR;
  }

  //write version number 1
  char *init_data = malloc(150);
  sprintf(init_data, "%s\n1", project_name);
  printf("\t attemtping the write\n");
  int num_bytes = write(fd, init_data, strlen(init_data));
  printf("\t write finished \n");
  if (num_bytes <= 0) {
    wtf_perror(E_CANNOT_READ_OR_WRITE_PROJECT_DIR, NON_FATAL_ERROR);
    free(path);
    close(fd);
    //unlock
    unlock_repository(project_name);
    return E_CANNOT_READ_OR_WRITE_PROJECT_DIR;
  }

  printf("\tSuccessfully created .Manifest file for %s\n", project_name);
  free(path);
  close(fd);

  //unlock
  unlock_repository(project_name);
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
  lock_repository(project_name);

  char *path = malloc(150);
  char *ret_string = malloc(500000);
  sprintf(path, "./Projects/%s/.Manifest", project_name);
  if (access(path, F_OK) == -1) {
    sprintf(ret_string, "%d", E_PROJECT_DOESNT_EXIST);
    unlock_repository(project_name);
    free(path);
    return ret_string;
  }

  int manifest_fd = open(path, O_RDONLY);
  if (manifest_fd < 0) {
    close(manifest_fd);
    free(path);
    sprintf(ret_string, "%d", E_CANNOT_READ_OR_WRITE_PROJECT_DIR);
    unlock_repository(project_name);
    return ret_string;
  }

  //now we need to read from the manifest
  char *buffer = malloc(10000);
  int n = read(manifest_fd, buffer, strlen(project_name) + 1);
  if (n <= 0) {
    close(manifest_fd);
    free(buffer);
    sprintf(ret_string, "%d", E_CANNOT_READ_OR_WRITE_PROJECT_DIR);
    unlock_repository(project_name);
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

  unlock_repository(project_name);

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
  // printf("\thashing %s strlen is %d\n", string, strlen(string));
  unsigned char tmphash[SHA_DIGEST_LENGTH];
  memset(tmphash, 0, SHA_DIGEST_LENGTH);
  SHA1(string, strlen(string), tmphash);
  int i = 0;
  unsigned char *hash = malloc((SHA_DIGEST_LENGTH * 2) + 1);
  memset(hash, 0, SHA_DIGEST_LENGTH * 2);
  for (i = 0; i < SHA_DIGEST_LENGTH; i++) {
    sprintf((char *)&(hash[i * 2]), "%02x", tmphash[i]);
  }

  // printf("final hash is %s\n", hash);

  return hash;
}

/**
 * Helper function for fetching the manifest and populating a Manifest data structure
 * 
 * Returns
 *  NULL = Failure
 *  Manifest* = Success
 */
Manifest *fetch_manifest(char *project_name) {
  //First check that the project exists on the client
  char *path = malloc(150);
  sprintf(path, "./Projects/%s", project_name);
  DIR *dir = opendir(path);
  if (!dir) wtf_perror(E_PROJECT_DOESNT_EXIST, NON_FATAL_ERROR);

  //Fetch manifest
  char *buffer = malloc(200);
  char *builder = malloc(200);  //used when we need to pull out certain substrings of unkown length while reading from the buffer

  sprintf(buffer, "%s/.Manifest", path);
  int manifest_fd = open(buffer, O_RDONLY);
  if (manifest_fd < 0) {
    free(buffer);
    free(builder);
    wtf_perror(E_CANNOT_READ_OR_WRITE_PROJECT_DIR, NON_FATAL_ERROR);
  }

  //Count number of lines in file
  int n = read(manifest_fd, buffer, 1);
  if (n <= 0) {
    free(buffer);
    free(builder);
    close(manifest_fd);
    wtf_perror(E_CANNOT_READ_OR_WRITE_PROJECT_DIR, NON_FATAL_ERROR);
  }

  Manifest *client_manifest = malloc(sizeof(Manifest));
  client_manifest->project_name = malloc(strlen(project_name) + 1);
  strcpy(client_manifest->project_name, project_name);

  int lines = 0;
  while (n != 0) {
    if (buffer[0] == '\n') lines++;
    n = read(manifest_fd, buffer, 1);
  }
  lines++;  //always off by 1 since EOF isnt a new line

  lines -= 2;  // we will use lines from here as the file count, and the first 2 lines are always never files
  // printf("total of %d files\n", lines);
  client_manifest->file_count = lines;
  client_manifest->files = malloc(sizeof(ManifestFileEntry *) * lines);
  lseek(manifest_fd, 0, SEEK_SET);                         //move back to front of file
  lseek(manifest_fd, strlen(project_name) + 1, SEEK_SET);  //move to second line because we don't care about reading the name of the project again

  memset(buffer, 0, 200);
  memset(builder, 0, 200);
  n = 1;
  while (buffer[0] != '\n' && n != 0) {
    sprintf(builder, "%s%c", builder, buffer[0]);
    n = read(manifest_fd, buffer, 1);
  }
  client_manifest->version_number = atoi(builder);

  //Now loop over all of the remaining lines building a ManifestFileEntry for each
  int j;
  for (j = 0; j < client_manifest->file_count; j++) {
    memset(buffer, 0, 200);
    memset(builder, 0, 200);
    client_manifest->files[j] = malloc(sizeof(ManifestFileEntry));

    //advance 2 bytes which will put the cursor to the start of the entry
    lseek(manifest_fd, 2, SEEK_CUR);

    //We need to check if there is either A/D opcode at the front
    read(manifest_fd, buffer, 2);
    // printf("Starting at %c\n", buffer[0]);
    if ((buffer[0] == OPCODE_ADD || buffer[0] == OPCODE_DELETE) && buffer[1] == ':') {
      //There is an op code
      client_manifest->files[j]->op_code = buffer[0];
      memset(buffer, 0, 200);
      memset(builder, 0, 200);
    } else {
      sprintf(builder, "%c%c", buffer[0], buffer[1]);
      client_manifest->files[j]->op_code = OPCODE_NONE;
    }
    // printf("\tOPCode = %c\n", client_manifest->files[j]->op_code);

    memset(buffer, 0, 200);
    while (buffer[0] != ':') {  //filename
      sprintf(builder, "%s%c", builder, buffer[0]);
      read(manifest_fd, buffer, 1);
    }
    // printf("\tFile path: %s\n", builder);
    client_manifest->files[j]->file_path = malloc(strlen(builder));
    strcpy(client_manifest->files[j]->file_path, builder);

    memset(buffer, 0, 200);
    memset(builder, 0, 200);
    while (buffer[0] != ':') {  //version number
      sprintf(builder, "%s%c", builder, buffer[0]);
      read(manifest_fd, buffer, 1);
    }
    // printf("\tVersion number: %d\n", atoi(builder));
    client_manifest->files[j]->version_number = atoi(builder);

    memset(buffer, 0, 200);
    memset(builder, 0, 200);
    while (buffer[0] != ':') {  //hash code
      sprintf(builder, "%s%c", builder, buffer[0]);
      read(manifest_fd, buffer, 1);
    }
    // printf("\tHash code: %s\n", builder);
    client_manifest->files[j]->hash = malloc(SHA_DIGEST_LENGTH * 2 + 1);
    memset(client_manifest->files[j]->hash, 0, SHA_DIGEST_LENGTH * 2 + 1);
    strcpy(client_manifest->files[j]->hash, builder);

    memset(buffer, 0, 200);
    memset(builder, 0, 200);
    //If there is a `!` meaning the server has not seen this entry yet
    read(manifest_fd, buffer, 1);
    if (buffer[0] == '!') {
      client_manifest->files[j]->seen_by_server = 0;
      lseek(manifest_fd, 1, SEEK_CUR);
    } else {
      client_manifest->files[j]->seen_by_server = 1;
    }
    // printf("\tSeen by Server = %d\n", client_manifest->files[j]->seen_by_server);

    client_manifest->files[j]->new_hash = NULL;
  }

  //free mem and return
  free(buffer);
  free(path);
  free(builder);
  close(manifest_fd);
  closedir(dir);
  return client_manifest;
}

/**
 * 
 * print_manifest
 * 
 * Prints manifest as readable string
 */
void print_manifest(Manifest *m, int verbose) {
  printf("[ MANIFEST]\n");
  printf("Project: %s\n", m->project_name);
  printf("Version: %d\n", m->version_number);
  printf("%d Total Files:\n", m->file_count);
  int i;
  for (i = 0; i < m->file_count; i++) {
    if (!verbose) {
      printf("\t%s Version: %d\n", m->files[i]->file_path, m->files[i]->version_number);
    } else {
      printf("\t%s Version: %d; Hash: %s\n", m->files[i]->file_path, m->files[i]->version_number, m->files[i]->hash);
    }
  }
  for (i = 0; i < m->new_file_count; i++) {
    if (!verbose) {
      printf("\t%s Version: %d\n", m->new_files[i]->file_path, m->new_files[i]->version_number);
    } else {
      printf("\t%s Version: %d; Hash: %s\n", m->new_files[i]->file_path, m->new_files[i]->version_number, m->files[i]->hash);
    }
  }
}

/**
 * lock_repository
 * 
 * lock a specific mutex given the project name
 */
void lock_repository(char *project_name) {
  RepositoryLock *curr = HEAD_REPOSITORY_LOCK;
  while (curr != NULL) {
    if (strcmp(curr->project_name, project_name) == 0) {
      pthread_mutex_lock(&(curr->lock));
      return;
    }
    curr = curr->next;
  }
}

/**
 * unlock_repository
 * 
 * unlock a specific mutex given the project name
 */
void unlock_repository(char *project_name) {
  RepositoryLock *curr = HEAD_REPOSITORY_LOCK;
  while (curr != NULL) {
    if (strcmp(curr->project_name, project_name) == 0) {
      pthread_mutex_unlock(&(curr->lock));
      return;
    }
    curr = curr->next;
  }
}

/**
 * 
 * free_manifest
 * 
 * Frees a manifest data structure
 */
void free_manifest(Manifest *m) {
  int i;
  for (i = 0; i < m->file_count; i++) {
    free(m->files[i]->file_path);
    free(m->files[i]->hash);
    if (m->files[i]->new_hash != NULL) free(m->files[i]->new_hash);
    free(m->files[i]);
  }
  for (i = 0; i < m->new_file_count; i++) {
    free(m->new_files[i]->file_path);
    free(m->new_files[i]->hash);
    free(m->new_files[i]);
  }
  free(m->project_name);
  free(m->files);
  free(m);
}

/**
 * write_manifest
 * 
 * Writes out Manifest object to the local .Manifest
 * 
 * Taken from ./client/client.c but needed for complex manifest operations (such as inside push())
 *  
 * Returns:
 *  0 = Failure
 *  1 = Success
 */
int write_manifest(Manifest *manifest) {
  char *buffer = malloc(500);
  sprintf(buffer, "./Projects/%s/.Manifest", manifest->project_name);

  //remove file and create it again. We are going to write out the entire .Manifest anyway
  remove(buffer);
  int fd = open(buffer, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    wtf_perror(E_CANNOT_READ_OR_WRITE_PROJECT_DIR, NON_FATAL_ERROR);
    free(buffer);
    return 0;
  }

  //check if we can write
  int n = write(fd, manifest->project_name, strlen(manifest->project_name));
  if (n <= 0) {
    wtf_perror(E_CANNOT_READ_OR_WRITE_PROJECT_DIR, NON_FATAL_ERROR);
    free(buffer);
    return 0;
  }
  //write headers
  memset(buffer, 0, 500);
  sprintf(buffer, "\n", 1);
  write(fd, buffer, 1);
  memset(buffer, 0, 500);
  sprintf(buffer, "%d", manifest->version_number);
  write(fd, buffer, strlen(buffer));

  //write all file entries
  int i;
  for (i = 0; i < manifest->file_count; i++) {
    //if (strlen(manifest->files[i]->file_path) == 0) continue;  //skip over this file, means it is being deleted from the .Manifest
    //Construct entry string
    memset(buffer, 0, 500);
    sprintf(buffer, "\n~ ");
    if (manifest->files[i]->op_code != OPCODE_NONE) {
      sprintf(buffer, "%s%c:", buffer, manifest->files[i]->op_code);
    }
    sprintf(buffer, "%s%s:%d:%s", buffer, manifest->files[i]->file_path, manifest->files[i]->version_number, manifest->files[i]->hash);
    if (manifest->files[i]->seen_by_server == 0) {
      sprintf(buffer, "%s:!", buffer);
    }
    sprintf(buffer, "%s:", buffer);
    write(fd, buffer, strlen(buffer));
  }

  close(fd);
  free(buffer);
  return 1;
}

/**
 * sanitize_project
 * 
 * Go through all directories and delete them if they don't contain any files
 */
void sanitize_project(char *project) {
  printf("\tSanitizing project\n");
  char *buffer = malloc(1000);
  memset(buffer, 0, 1000);
  sprintf(buffer, "cd ./%s/ && find . -type d -empty print");
  system(buffer);
  printf("Sanitized project\n");
  free(buffer);
}

/**
 * 
 * isRegFile
 * 
 * checks if the path provided is pointing to a normal file and not a directory
 */
int isRegFile(char *path) {
  struct stat statbuf;

  if (stat(path, &statbuf) != 0) {
    return 0;
  }

  return S_ISREG(statbuf.st_mode);
}