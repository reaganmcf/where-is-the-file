#include "client.h"

#include <dirent.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <zlib.h>

/** WTF Client
 * 
 * The WTF client will be handling all of the commands for this project, and will send each command to its proper
 * sub function from main to abstract out the code and make it more readable
 */

//Global Declarations
wtf_configuration *CONFIGURATION = NULL;

int main(int argc, char **argv) {
  atexit(wtf_exit_handler);
  //First, we need to check the params and flags
  if (argc == 1) {
    wtf_perror(E_IMPROPER_PARAMS_AND_FLAGS, FATAL_ERROR);
  }

  char *command = argv[1];

  //Error check params
  int i;
  int params_are_safe = 1;
  for (i = 1; i < argc; i++) {
    //check if the first or last char is a / ./
    char substr[100];
    memset(substr, 0, 100);
    strncpy(substr, argv[i], 2);
    if (strlen(substr) >= 2) {
      if (substr[0] == '.' && substr[1] == '/') {
        params_are_safe = 0;
      } else if (substr[0] == '/') {
        params_are_safe = 0;
      } else if (substr[strlen(substr) - 1] == '/') {
        params_are_safe = 0;
      } else if (substr[strlen(substr) - 1] == '.' && substr[strlen(substr) - 2] == '/') {
        params_are_safe = 0;
      }
    } else if (strlen(substr) == 1) {
      if (substr[0] == '/') {
        params_are_safe = 0;
      }
    }
  }

  if (params_are_safe == 0) {
    wtf_perror(E_IMPROPER_PATH_INPUT_PARAMS, FATAL_ERROR);
  }

  //Now, lets check send the command to the proper function
  if (strcmp(command, "configure") == 0) {
    //Check for params
    if (argc != 4) {
      wtf_perror(E_IMPROPER_CONFIGURATION_PARAMS, FATAL_ERROR);
    }

    char *hostname = argv[2];
    char *port = argv[3];
    if (strlen(hostname) == 0 || strlen(port) == 0) {
      wtf_perror(E_IMPROPER_CONFIGURATION_PARAMS, FATAL_ERROR);
    }

    int result = wtf_configure_host(hostname, port);
    if (result == 1) {
      printf("Succesfully configured client.\n");
      return 0;
    } else {
      wtf_perror(E_CONFIGURATION_WRITE_ERROR, FATAL_ERROR);
    }
  } else if (strcmp(command, "create") == 0) {
    //Check for params
    if (argc != 3) {
      wtf_perror(E_IMPROPER_CREATE_PARAMS, FATAL_ERROR);
    }
    char *project_name = argv[2];
    if (strlen(project_name) == 0 || strlen(project_name) >= 100) {
      wtf_perror(E_IMPROPER_CREATE_PROJECT_NAME_LENGTH, 1);
    }

    //Check that the project name does not contain : which is our delimeter
    char *temp = argv[2];
    int safe = 1;
    while (temp[0] != '\0') {
      if (temp[0] == ':')
        safe = 0;
      temp++;
    }

    if (safe == 0) {
      wtf_perror(E_IMPROPER_CREATE_PROJECT_NAME, FATAL_ERROR);
    }

    //All ready, create a connection handler and call server
    int result = wtf_create_project(project_name);

  } else if (strcmp(command, "add") == 0) {
    //Check for params
    if (argc != 4) {
      wtf_perror(E_IMPROPER_ADD_PARAMS, FATAL_ERROR);
    }

    char *project_name = argv[2];
    char *file = argv[3];
    if (strlen(project_name) == 0 || strlen(file) == 0) {
      wtf_perror(E_IMPROPER_ADD_PARAMS, FATAL_ERROR);
    }

    int result = wtf_add(project_name, file);
    if (result == 1) {
      printf("Successfully added %s to project manifest\n", file);
      return 0;
    } else {
      //Won't ever get here because errors will be handeled inside of wtf_add first
    }
  } else if (strcmp(command, "currentversion") == 0) {
    //Check for params
    if (argc != 3) {
      wtf_perror(E_IMPROPER_CURRENT_VERSION_PARAMS, FATAL_ERROR);
    }
    char *project_name = argv[2];

    //Check that the project name does not contain : which is our delimeter
    char *temp = argv[2];
    int safe = 1;
    while (temp[0] != '\0') {
      if (temp[0] == ':') safe = 0;
      temp++;
    }

    if (safe == 0) {
      wtf_perror(E_IMPROPER_CURRENT_VERSION_PROJECT_NAME, FATAL_ERROR);
    }

    //All ready, create a connection handler and call the server
    int result = wtf_get_current_version(project_name);
    if (result == 1) {
      printf("Successfully retrieved project information from the server.\n");
      return 0;
    } else {
      //Wont ever get here because errors will be handled inside of wtf_get_current_version first
    }
  } else if (strcmp(command, "commit") == 0) {
    //Check for params
    if (argc != 3) {
      wtf_perror(E_IMPROPER_COMMIT_PARAMS, FATAL_ERROR);
    }
    char *project_name = argv[2];
    //Check that the project name does not contain : which is our delimeter
    char *temp = argv[2];
    int safe = 1;
    while (temp[0] != '\0') {
      if (temp[0] == ':') safe = 0;
      temp++;
    }

    if (safe == 0) {
      wtf_perror(E_IMPROPER_COMMIT_PROJECT_NAME, FATAL_ERROR);
    }

    //All ready, create a connection handler and call server
    int result = wtf_commit(project_name);
    if (result == 1) {
      return 0;
    }
  } else if (strcmp(command, "remove") == 0) {
    //Check for params
    if (argc != 4) {
      wtf_perror(E_IMPROPER_REMOVE_PARAMS, FATAL_ERROR);
    }

    char *project_name = argv[2];
    char *file = argv[3];
    if (strlen(project_name) == 0 || strlen(file) == 0) {
      wtf_perror(E_IMPROPER_REMOVE_PARAMS, FATAL_ERROR);
    }

    int result = wtf_remove(project_name, file);
    if (result == 1) {
      printf("Successfully removed %s from project manifest\n", file);
      return 0;
    } else {
      //Won't ever get here because errors will be handled inside of wtf_add first
    }
  } else if (strcmp(command, "push") == 0) {
    //Check for params
    if (argc != 3) {
      wtf_perror(E_IMPROPER_PUSH_PARAMS, FATAL_ERROR);
    }
    char *project_name = argv[2];

    //Check that the project name doesn't contain : which is our delim
    char *temp = argv[2];
    int safe = 1;
    while (temp[0] != '\0') {
      if (temp[0] == ':') safe = 0;
      temp++;
    }

    if (safe == 0) {
      wtf_perror(E_IMPROPER_PUSH_PROJECT_NAME, FATAL_ERROR);
    }

    //All ready, create a connection handler and call the server
    int result = wtf_push(project_name);
    if (result == 1) {
      printf("Successfully retrieved project information from the server.\n");
      return 0;
    } else {
      //Wont ever get here because errors will be handled inside of wtf_push first
    }
  } else if (strcmp(command, "history") == 0) {
    //Check for params
    if (argc != 3) {
      wtf_perror(E_IMPROPER_HISTORY_PARAMS, FATAL_ERROR);
    }

    char *project_name = argv[2];

    //Check that the project name does not contain : which is our delimeter
    char *temp = argv[2];
    int safe = 1;
    while (temp[0] != '\0') {
      if (temp[0] == ':')
        safe = 0;
      temp++;
    }

    if (safe == 0) {
      wtf_perror(E_IMPROPER_CREATE_PROJECT_NAME, FATAL_ERROR);
    }

    //All ready, create a connection handler and call server
    int result = wtf_history(project_name);

  } else if (strcmp(command, "destroy") == 0) {
    //Check params
    if (argc != 3) {
      wtf_perror(E_IMPROPER_DESTROY_PARAMS, FATAL_ERROR);
    }

    char *project_name = argv[2];

    //Check that the project name does not contain : which is our delimeter
    char *temp = argv[2];
    int safe = 1;
    while (temp[0] != '\0') {
      if (temp[0] == ':') safe = 0;
      temp++;
    }

    if (safe == 0) {
      wtf_perror(E_IMPROPER_DESTROY_PROJECT_NAME, FATAL_ERROR);
    }

    //All ready, create a connection handler and call server
    int result = wtf_destroy(project_name);

  } else if (strcmp(command, "rollback") == 0) {
    //Check for params
    if (argc != 4) {
      wtf_perror(E_IMPROPER_ROLLBACK_PARAMS, FATAL_ERROR);
    }

    char *project_name = argv[2];

    //Check that the project name does not contain : which is our delimeter
    char *temp = argv[2];
    int safe = 1;
    while (temp[0] != '\0') {
      if (temp[0] == ':')
        safe = 0;
      temp++;
    }

    if (safe == 0) {
      wtf_perror(E_IMPROPER_ROLLBACK_PROJECT_NAME, FATAL_ERROR);
    }

    //Check that the version number casts to an int correctly. We can use atoi() since if atoi() fails it returns 0
    //But, the min version number is 1 so 0 == fails
    int version_num = atoi(argv[3]);
    if (version_num == 0) {
      //failed
      wtf_perror(E_IMPROPER_ROLLBACK_VERSION_NUMBER, FATAL_ERROR);
    }

    //All ready, create connection handler and call server
    int result = wtf_rollback(project_name, argv[3]);

  } else if (strcmp(command, "update") == 0) {
    //Check for params
    if (argc != 3) {
      wtf_perror(E_IMPROPER_UPDATE_PARAMS, FATAL_ERROR);
    }
    char *project_name = argv[2];
    //Check that the project name does not contain : which is our delimeter
    char *temp = argv[2];
    int safe = 1;
    while (temp[0] != '\0') {
      if (temp[0] == ':') safe = 0;
      temp++;
    }

    if (safe == 0) {
      wtf_perror(E_IMPROPER_UPDATE_PROJECT_NAME, FATAL_ERROR);
    }

    //All ready, create a connection handler and call server
    int result = wtf_update(project_name);

  } else if (strcmp(command, "upgrade") == 0) {
    //Check for params
    if (argc != 3) {
      wtf_perror(E_IMPROPER_UPGRADE_PARAMS, FATAL_ERROR);
    }
    char *project_name = argv[2];
    //Check that the project name does not contain : which is our delimeter
    char *temp = argv[2];
    int safe = 1;
    while (temp[0] != '\0') {
      if (temp[0] == ':') safe = 0;
      temp++;
    }

    if (safe == 0) {
      wtf_perror(E_IMPROPER_UPDATE_PROJECT_NAME, FATAL_ERROR);
    }

    //All ready, create a connection handler and call server
    int result = wtf_upgrade(project_name);

  } else if (strcmp(command, "checkout") == 0) {
    //Check for params
    if (argc != 3) {
      wtf_perror(E_IMPROPER_CHECKOUT_PARAMS, 1);
    }
    char *project_name = argv[2];
    //Check that the project name does not contain : which is our delimeter
    char *temp = argv[2];
    int safe = 1;
    while (temp[0] != '\0') {
      if (temp[0] == ':') safe = 0;
      temp++;
    }

    if (safe == 0) {
      wtf_perror(E_IMPROPER_UPDATE_PROJECT_NAME, FATAL_ERROR);
    }

    //All ready, create a connection handler and call server
    int result = wtf_checkout(project_name);
  } else {
    wtf_perror(E_NO_COMMAND_PROVIDED, FATAL_ERROR);
  }

  // wtf_connection *connection = wtf_connect();
  // char *testString = "hello gamers";
  // write(connection->socket, &testString, sizeof(int));
  // write(connection->socket, testString, strlen(testString));
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
 * Checkout command
 * 
 * Fetch the entire project from the server
 * 
 * Returns
 *  0 = Failure
 *  1 = Success
 */
int wtf_checkout(char *project_name) {
  //Check that the project does not exist on the client already
  DIR *dir = opendir(project_name);
  if (dir) {
    closedir(dir);
    wtf_perror(E_CANNOT_CHECKOUT_PROJECT_ALREADY_EXISTS, FATAL_ERROR);
  }
  //Fetch server manifest
  Manifest *server_manifest = fetch_server_manifest(project_name);

  //We need to loop over all of the files and create the file locally and write the contents
  char *buffer = malloc(1000);
  char *dir_path = malloc(1000);
  char *mid_buffer = malloc(1000);
  memset(dir_path, 0, 1000);

  wtf_connection *connection = wtf_connect();

  //create project dir
  memset(buffer, 0, 1000);
  sprintf(buffer, "mkdir ./%s", project_name);
  system(buffer);

  int i;
  int fd;
  int k;
  int slash_index;
  char *file_contents_buffer;
  for (i = 0; i < server_manifest->file_count; i++) {
    memset(buffer, 0, 1000);
    memset(dir_path, 0, 1000);
    char *file_path = server_manifest->files[i]->file_path;

    //extract dir path since we have to create sub dirs
    k = 0;
    slash_index = 0;
    while (file_path[k] != 0) {
      if (file_path[k] == '/')
        slash_index = k;
      k++;
    }

    strncpy(dir_path, file_path, slash_index + 1);
    printf("sub dirs are %s\n", dir_path);
    dir = opendir(buffer);
    if (dir) {
      closedir(dir);
    } else {
      memset(buffer, 0, 1000);
      sprintf(buffer, "mkdir -p %s &> /dev/null", dir_path);
      system(buffer);
    }

    //Fetch file contents
    memset(buffer, 0, 1000);
    sprintf(buffer, "%d:%s:%d:%s", strlen(COMMAND_GET_FILE_CONTENTS), COMMAND_GET_FILE_CONTENTS, strlen(file_path), file_path);
    int msg_size = strlen(buffer) + 1;
    write(connection->socket, &msg_size, sizeof(int));
    write(connection->socket, buffer, msg_size);

    memset(buffer, 0, 1000);
    read(connection->socket, buffer, 1);
    if (buffer[0] == '1') {
      //all good, read more
      read(connection->socket, buffer, 1);  //read past :
      memset(buffer, 0, 1);
      while (buffer[0] != ':') {  //read file content size
        sprintf(mid_buffer, "%s%c", mid_buffer, buffer[0]);
        read(connection->socket, buffer, 1);
      }
      int file_size = atoi(mid_buffer);
      file_contents_buffer = malloc(file_size + 1);
      memset(file_contents_buffer, 0, file_size + 1);
      printf("file size is %d\n", file_size);
      read(connection->socket, file_contents_buffer, file_size);
      printf("Retrieved file contents for %s\n", file_path);
      printf("\t%s\n", file_contents_buffer);

      fd = open(file_path, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
      if (fd == -1) {
        printf("Coulnd't open %s\n", file_path);
        free(buffer);
        free(dir_path);
        free_manifest(server_manifest);
        free(mid_buffer);
        free(file_contents_buffer);
        close(connection->socket);
        free(connection);
        wtf_perror(E_CANNOT_CHECKOUT_CANT_WRITE_FILE, FATAL_ERROR);
      }

      write(fd, file_contents_buffer, strlen(file_contents_buffer));
      free(file_contents_buffer);
    } else {
      free(buffer);
      free(dir_path);
      free_manifest(server_manifest);
      free(mid_buffer);
      free(file_contents_buffer);
      close(connection->socket);
      free(connection);
      wtf_perror(E_CANNOT_CHECKOUT_FAILED_TO_GET_FILE_CONTENTS, FATAL_ERROR);
    }
  }

  //Write out manifest
  write_manifest(server_manifest);
  free(buffer);
  free(dir_path);
  free(mid_buffer);
  free_manifest(server_manifest);
  close(connection->socket);
  free(connection);
  return 1;
}

/**
 * Upgrade command
 * 
 * Apply all of the operations listed in .Update
 * 
 * Returns
 *  0 = Failure
 *  1 = Success
 */
int wtf_upgrade(char *project_name) {
  wtf_connection *connection = wtf_connect();

  char *buffer = malloc(1000);
  memset(buffer, 0, 1000);

  //Check if there is no .Conflict on client
  sprintf(buffer, "./%s/.Conflict", project_name);
  if (access(buffer, F_OK) == 0) {
    free(buffer);
    close(connection->socket);
    free(connection);
    wtf_perror(E_CANNOT_UPGRADE_CONFLICT_EXISTS, FATAL_ERROR);
  }

  //Check that there is a .Update file on client
  memset(buffer, 0, 1000);
  sprintf(buffer, "./%s/.Update", project_name);
  if (access(buffer, F_OK) != 0) {
    free(buffer);
    close(connection->socket);
    free(connection);
    wtf_perror(E_CANNOT_UPGRADE_NO_UPDATE, FATAL_ERROR);
  }

  //Load .Update file into string
  char *update_raw = malloc(100000);
  memset(update_raw, 0, 100000);
  int fd = open(buffer, O_RDONLY);
  if (fd == -1) {
    free(update_raw);
    free(buffer);
    close(connection->socket);
    free(connection);
    wtf_perror(E_CANNOT_READ_UPDATE_FILE, FATAL_ERROR);
  }
  read(fd, update_raw, 100000);
  if (strlen(update_raw) == 0) {
    printf("Client is up to date.\n");
    memset(buffer, 0, 1000);
    sprintf(buffer, "rm ./%s/.Update", project_name);
    system(buffer);
    close(fd);
    free(buffer);
    close(connection->socket);
    free(connection);
    free(update_raw);
  }
  close(fd);

  //Load update_raw into custom UpdateOperation array
  int i = 0;
  int j = 0;
  char opcode;
  int upgrade_op_count = 0;
  int non_delete_upgrade_op_count = 0;
  char *update_copy = malloc(strlen(update_raw) + 1);
  memset(update_copy, 0, strlen(update_raw) + 1);
  strncpy(update_copy, update_raw, strlen(update_raw));
  UpgradeOperation **upgrade_ops = malloc(sizeof(UpgradeOperation *) * 1000);

  while (update_copy[0] != 0) {
    if (update_copy[0] == OPCODE_ADD || update_copy[0] == OPCODE_DELETE || update_copy[0] == OPCODE_MODIFY) {
      opcode = update_copy[0];
      if (opcode == OPCODE_ADD || opcode == OPCODE_MODIFY) non_delete_upgrade_op_count++;
      if (update_copy[1] == ' ') {
        //entry starts here
        update_copy += 2;
        upgrade_ops[upgrade_op_count] = malloc(sizeof(UpgradeOperation));
        upgrade_ops[upgrade_op_count]->op_code = opcode;
        memset(buffer, 0, 1000);
        while (update_copy[0] != ' ') {
          sprintf(buffer, "%s%c", buffer, update_copy[0]);
          update_copy++;
        }
        upgrade_ops[upgrade_op_count]->file_path = malloc(strlen(buffer) + 1);
        memset(upgrade_ops[upgrade_op_count]->file_path, 0, strlen(buffer) + 1);
        strcpy(upgrade_ops[upgrade_op_count]->file_path, buffer);

        //we have to fetch this from the server later
        upgrade_ops[upgrade_op_count]->contents = NULL;

        upgrade_op_count++;
      }
    }
    update_copy++;
  }
  free(update_raw);

  printf("upgrade_ops finished. total of %d\n", upgrade_op_count);
  //go over all upgrade ops and fetch from the server the contents of the file if the OPCODE is M or A

  char *mid_buffer = malloc(20);
  for (i = 0; i < upgrade_op_count; i++) {
    memset(mid_buffer, 0, 20);
    memset(buffer, 0, 1000);
    if (upgrade_ops[i]->op_code == OPCODE_ADD || upgrade_ops[i]->op_code == OPCODE_MODIFY) {
      //fetch file contents from server
      sprintf(buffer, "%d:%s:%d:%s", strlen(COMMAND_GET_FILE_CONTENTS), COMMAND_GET_FILE_CONTENTS, strlen(upgrade_ops[i]->file_path), upgrade_ops[i]->file_path);
      int msg_size = strlen(buffer) + 1;
      printf("Sending {%s} to the server (%d) bytes in total\n", buffer, msg_size);
      write(connection->socket, &msg_size, sizeof(int));
      write(connection->socket, buffer, strlen(buffer) + 1);
      memset(buffer, 0, 1000);
      read(connection->socket, buffer, 2);
      if (strlen(buffer) == 1 && buffer[0] == '1') {
        //all good, read more
        read(connection->socket, buffer, 1);  //read past :
        memset(buffer, 0, 1);
        while (buffer[0] != ':') {
          sprintf(mid_buffer, "%s%c", mid_buffer, buffer[0]);
          read(connection->socket, buffer, 1);
        }
        int file_size = atoi(mid_buffer);
        upgrade_ops[i]->contents = malloc(file_size + 1);
        memset(upgrade_ops[i]->contents, 0, file_size + 1);
        read(connection->socket, upgrade_ops[i]->contents, file_size);
        printf("Retrieved file contents for %s\n", upgrade_ops[i]->file_path);
      } else {
        //error happened when grabbing file from server
        for (j = 0; j < 1000; j++) {
          if (upgrade_ops[j] != NULL) {
            free(upgrade_ops[j]->contents);
            free(upgrade_ops[j]->file_path);
            free(upgrade_ops[j]);
          }
        }
        free(upgrade_ops);
        free(buffer);
        free(mid_buffer);
        close(connection->socket);
        free(connection);
        wtf_perror(E_SERVER_CANNOT_FIND_FILE, FATAL_ERROR);
      }
    }

    printf("%c %s %s\n", upgrade_ops[i]->op_code, upgrade_ops[i]->file_path, upgrade_ops[i]->contents);

    //Apply the operation
    if (upgrade_ops[i]->op_code == OPCODE_ADD || upgrade_ops[i]->op_code == OPCODE_MODIFY) {
      //create the file and write to it
      //we need to pull out the dir and tree path first

      char *dir_paths = malloc(1000);
      memset(dir_paths, 0, 1000);
      int k = 0;
      int slash_index = 0;
      while (upgrade_ops[i]->file_path[k] != 0) {
        if (upgrade_ops[i]->file_path[k] == '/')
          slash_index = k;
        k++;
      }
      strncpy(dir_paths, upgrade_ops[i]->file_path, slash_index + 1);
      printf("\tdir subtree is %s\n", dir_paths);

      memset(buffer, 0, 1000);
      sprintf(buffer, "./%s", dir_paths);
      DIR *dir = opendir(buffer);
      if (dir) {
        closedir(dir);
      } else {
        memset(buffer, 0, 1000);
        sprintf(buffer, "mkdir -p %s &> /dev/null", dir_paths);
        system(buffer);
      }
      closedir(dir);
      free(dir_paths);

      fd = open(upgrade_ops[i]->file_path, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
      if (fd == -1) {
        printf("couldn't open %s\n", upgrade_ops[i]->file_path);
        for (j = 0; j < 1000; j++) {
          if (upgrade_ops[j] != NULL) {
            free(upgrade_ops[j]->contents);
            free(upgrade_ops[j]->file_path);
            free(upgrade_ops[j]);
          }
        }
        free(upgrade_ops);
        free(buffer);
        free(mid_buffer);
        close(connection->socket);
        free(connection);
        wtf_perror(E_CANNOT_UPGRADE_CANT_OPEN_OR_CREATE_FILE, FATAL_ERROR);
      }

      write(fd, upgrade_ops[i]->contents, strlen(upgrade_ops[i]->contents));
      close(fd);
    } else {
      //Delete operation
      // memset(buffer, 0, 1000);
      // sprintf(buffer, "rm %s &> /dev/null", upgrade_ops[i]->file_path);
      // system(buffer);

      //sanitize project
      sanitize_project(project_name);
    }
  }

  //Delete .Update
  memset(buffer, 0, 1000);
  sprintf(buffer, "rm ./%s/.Update &> /dev/null", project_name);
  system(buffer);

  Manifest *server_manifest = fetch_server_manifest(project_name);
  write_manifest(server_manifest);

  printf("Successfully performed operations listed in .Update\n");

  for (j = 0; j < 1000; j++) {
    if (upgrade_ops[j] != NULL) {
      free(upgrade_ops[j]->contents);
      free(upgrade_ops[j]->file_path);
    }
    free(upgrade_ops[j]);
  }
  free(upgrade_ops);
  free(buffer);
  free(mid_buffer);
  free_manifest(server_manifest);
  close(connection->socket);
  free(connection);
  return 1;
}

/**
 * Update command
 * 
 * Basically is commit but the other way around
 * 
 * Returns
 *  0 = Failure
 *  1 = Success
 */
int wtf_update(char *project_name) {
  Manifest *server_manifest = fetch_server_manifest(project_name);
  Manifest *client_manifest = fetch_client_manifest(project_name);
  //If we get here then project exists on client and server, and also means we can establish a connection

  char *buffer = malloc(1000);
  memset(buffer, 0, 1000);

  //Full Success Case
  //server and client .Manifests are the same version
  if (server_manifest->version_number == client_manifest->version_number) {
    printf("Up To Date\n");
    //We need to write a blank .Update
    sprintf(buffer, "./%s/.Update", project_name);
    if (access(buffer, F_OK) != -1) {
      memset(buffer, 0, 1000);
      sprintf(buffer, "rm ./%s/.Update", project_name);
      system(buffer);
    }
    sprintf(buffer, "touch ./%s/.Update", project_name);
    system(buffer);
    memset(buffer, 0, 1000);
    sprintf(buffer, "./%s/.Conflict", project_name);
    if (access(buffer, F_OK) != -1) {
      memset(buffer, 0, 1000);
      sprintf(buffer, "rm ./%s/.Conflict", project_name);
      system(buffer);
    }
    free(buffer);
    free_manifest(client_manifest);
    free_manifest(server_manifest);
    return 1;
  }

  //calculate live hash for all of the client's files

  int i;
  int j;
  char *hash;
  for (i = 0; i < client_manifest->file_count; i++) {
    hash = hash_file(client_manifest->files[i]->file_path);
    client_manifest->files[i]->new_hash = malloc(SHA_DIGEST_LENGTH * 2 + 1);
    memset(client_manifest->files[i]->new_hash, 0, SHA_DIGEST_LENGTH * 2 + 1);
    strncpy(client_manifest->files[i]->new_hash, hash, strlen(hash));
    free(hash);
  }

  char *conflict_buffer = malloc(500000);
  memset(conflict_buffer, 0, 500000);
  int total_writes = 0;

  //Failure Case: server and client are different version, and client has a livehash that doesn't exist on either the server or client
  for (i = 0; i < client_manifest->file_count; i++) {
    for (j = 0; j < server_manifest->file_count; j++) {
      if (strcmp(client_manifest->files[i]->file_path, server_manifest->files[j]->file_path) == 0) {
        //Same file, now check if live hash is different then stored hash on client
        if (strcmp(client_manifest->files[i]->new_hash, client_manifest->files[i]->hash) != 0) {
          //Modified on client, now check if it's different then the server hash
          if (strcmp(client_manifest->files[i]->new_hash, server_manifest->files[i]->hash) != 0) {
            //Modified on both client and server, this is a conflict
            if (total_writes != 0) {
              sprintf(conflict_buffer, "%s\n");
            }
            total_writes++;
            sprintf(conflict_buffer, "%s%c %s %s", conflict_buffer, OPCODE_CONFLICT, client_manifest->files[i]->file_path, client_manifest->files[i]->new_hash);
            printf("%c %s\n", OPCODE_CONFLICT, client_manifest->files[i]->file_path);
          }
        }
      }
    }
  }

  //Add Code Partial Success
  char *update_buffer = malloc(500000);
  memset(update_buffer, 0, 500000);
  total_writes = 0;

  //Modify Code case: Server has modifications for the client
  //  server manifest has files whose version and hash are different then the server
  //  and the live hash of those files MATCH the hash in the client's .Manifest

  for (i = 0; i < server_manifest->file_count; i++) {
    for (j = 0; j < client_manifest->file_count; j++) {
      if (strcmp(server_manifest->files[i]->file_path, client_manifest->files[j]->file_path) == 0) {
        //Same file, now lets check if hashes are different on client side
        if (strcmp(server_manifest->files[i]->hash, client_manifest->files[j]->new_hash) != 0) {
          //Client has different file, now make sure that the client hash == server hash
          if (strcmp(server_manifest->files[i]->hash, client_manifest->files[j]->new_hash) == 0) {
            //This is a match. We need to append a MODIFY opcode to .Update
            if (total_writes != 0) {
              sprintf(update_buffer, "%s\n", update_buffer);
            }
            total_writes++;
            printf("%c %s\n", OPCODE_MODIFY, server_manifest->files[i]->file_path);
            sprintf(update_buffer, "%s%c %s %s", update_buffer, OPCODE_MODIFY, server_manifest->files[i]->file_path, server_manifest->files[i]->hash);
          }
        }
      }
    }
  }

  //Add Code case: server has the file but client doesn't
  int client_has_file;
  for (i = 0; i < server_manifest->file_count; i++) {
    client_has_file = 0;
    for (j = 0; j < client_manifest->file_count; j++) {
      if (strcmp(server_manifest->files[i]->file_path, client_manifest->files[j]->file_path) == 0) {
        client_has_file = 1;
      }
    }
    if (client_has_file == 0) {
      printf("%c %s\n", OPCODE_ADD, server_manifest->files[i]->file_path);
      if (total_writes != 0) {
        sprintf(update_buffer, "%s\n", update_buffer);
      }
      total_writes++;
      sprintf(update_buffer, "%s%c %s %s", update_buffer, OPCODE_ADD, server_manifest->files[i]->file_path, server_manifest->files[i]->hash);
    }
  }

  //Delete Code case: client has the file but server doesn't
  int server_has_file;
  for (i = 0; i < client_manifest->file_count; i++) {
    server_has_file = 0;
    for (j = 0; j < server_manifest->file_count; j++) {
      if (strcmp(client_manifest->files[i]->file_path, server_manifest->files[j]->file_path) == 0) {
        server_has_file = 1;
      }
    }
    if (server_has_file == 0) {
      printf("%c %s\n", OPCODE_DELETE, client_manifest->files[i]->file_path);
      if (total_writes != 0) {
        sprintf(update_buffer, "%s\n", update_buffer);
      }
      total_writes++;
      sprintf(update_buffer, "%s%c %s %s", update_buffer, OPCODE_DELETE, client_manifest->files[i]->file_path, client_manifest->files[i]->hash);
    }
  }

  //Conflict exists
  if (strlen(conflict_buffer) != 0) {
    //need to delete .Update
    memset(buffer, 0, 1000);
    sprintf(buffer, "./%s/.Update", project_name);
    if (access(buffer, F_OK) == 0) {
      memset(buffer, 0, 1000);
      sprintf(buffer, "rm ./%s/.Update", project_name);
      system(buffer);
    }

    //write .Conflict
    memset(buffer, 0, 1000);
    sprintf(buffer, "./%s/.Conflict", project_name);
    int fd = open(buffer, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd == -1) {
      free(update_buffer);
      free(conflict_buffer);
      free(buffer);
      free_manifest(server_manifest);
      free_manifest(client_manifest);
      wtf_perror(E_CANNOT_WRITE_CONFLICT, FATAL_ERROR);
    }
    write(fd, conflict_buffer, strlen(conflict_buffer));
    close(fd);

    free(update_buffer);
    free(conflict_buffer);
    free(buffer);
    free_manifest(server_manifest);
    free_manifest(client_manifest);
    wtf_perror(E_CANNOT_UPDATE_CONFLICT_EXISTS, FATAL_ERROR);
  }

  //Nothing to update
  if (strlen(update_buffer) == 0) {
    printf("Nothing to update\n");
    free(update_buffer);
    free(conflict_buffer);
    free(buffer);
    free_manifest(server_manifest);
    free_manifest(client_manifest);
    return 1;
  }

  //Write .Update
  memset(buffer, 0, 1000);
  sprintf(buffer, "./%s/.Update", project_name);
  int fd = open(buffer, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    free(update_buffer);
    free(conflict_buffer);
    free(buffer);
    free_manifest(server_manifest);
    free_manifest(client_manifest);
    wtf_perror(E_CANNOT_WRITE_UPDATE, FATAL_ERROR);
  }

  write(fd, update_buffer, strlen(update_buffer));

  printf("Successfully wrote .Update\n");
  free(update_buffer);
  free(conflict_buffer);
  free(buffer);
  free_manifest(server_manifest);
  free_manifest(client_manifest);
  close(fd);
  return 1;
}

/**
 * Rollback command
 * 
 * Send the server the project name and version number and retrieve back the status code of the operation
 * 
 * Most of the work is done server side
 * 
 * Returns:
 *  0 = Failure
 *  1 = Success
 */
int wtf_rollback(char *project_name, char *version_number) {
  //Make a connection
  wtf_connection *connection = wtf_connect();

  //already confirmed that version_number is an int, but we want it as a string so we can format protocol correctly

  char *buffer = malloc(200);
  memset(buffer, 0, 200);
  sprintf(buffer, "%d:%s:%d:%s:%d:%s", strlen(COMMAND_ROLLBACK_PROJECT), COMMAND_ROLLBACK_PROJECT, strlen(project_name), project_name, strlen(version_number), version_number);
  int msg_size = strlen(buffer) + 1;
  printf("Sending {%s} to the server (%d) bytes in total\n", buffer, msg_size);
  write(connection->socket, &msg_size, sizeof(int));
  write(connection->socket, buffer, strlen(buffer) + 1);

  //handle callback
  memset(buffer, 0, 200);
  read(connection->socket, buffer, 3);
  int ret_status = atoi(buffer);
  printf("ret_status = %d\n", ret_status);
  if (ret_status == 1) {
    printf("Successfully reverted project to version %d on the server.\n", atoi(version_number));
    free(buffer);
    close(connection->socket);
    free(connection);
    return 1;
  } else {
    //error'd
    free(buffer);
    close(connection->socket);
    free(connection);
    if (ret_status == 5) {
      wtf_perror(E_SERVER_IMPROPER_PERMISSIONS, FATAL_ERROR);
    } else if (ret_status == 7) {
      wtf_perror(E_SERVER_PROJECT_DOESNT_EXIST, FATAL_ERROR);
    } else {
      wtf_perror(E_SERVER_PROJECT_VERSION_DOESNT_EXIST, FATAL_ERROR);
    }
  }

  return 1;
}

/**
 * Destroy command
 * 
 * Send the server the project name and retrieve back the status code whether the operation was performed successfully
 * 
 * Most of the work is done server side
 * 
 * Returns:
 *  0 = Failure
 *  1 = Success
 */
int wtf_destroy(char *project_name) {
  //Make a connection
  wtf_connection *connection = wtf_connect();

  char *buffer = malloc(200);
  memset(buffer, 0, 200);
  sprintf(buffer, "%d:%s:%d:%s", strlen(COMMAND_DESTORY_PROJECT), COMMAND_DESTORY_PROJECT, strlen(project_name), project_name);
  int msg_size = strlen(buffer) + 1;
  printf("Sending {%s} to the server (%d) bytes in total\n", buffer, msg_size);
  write(connection->socket, &msg_size, sizeof(int));
  write(connection->socket, buffer, strlen(buffer) + 1);

  //handle callback
  memset(buffer, 0, 200);
  read(connection->socket, buffer, 3);
  int ret_status = atoi(buffer);
  if (ret_status == 1) {
    printf("Successfully destoryed the project '%s' on the server.\n", project_name);
    free(buffer);
    close(connection->socket);
    free(connection);
    return 1;
  } else {
    //error'd
    if (ret_status == 5) {
      free(buffer);
      close(connection->socket);
      free(connection);
      wtf_perror(E_SERVER_IMPROPER_PERMISSIONS, FATAL_ERROR);
    } else {
      free(buffer);
      close(connection->socket);
      free(connection);
      wtf_perror(E_SERVER_PROJECT_DOESNT_EXIST, FATAL_ERROR);
    }
  }

  return 1;
}

/**
 * History command
 * 
 * Send the server the project name and retrieve back the full length of all operations performed on the repo
 * 
 * Most of the work is done server side
 * 
 * Returns:
 *  0 = Failure
 *  1 = Success
 */
int wtf_history(char *project_name) {
  wtf_connection *connection = wtf_connect();
  char *buffer = malloc(200);
  char *ret_buffer = malloc(100000);
  memset(buffer, 0, 200);
  memset(ret_buffer, 0, 100000);
  sprintf(buffer, "%d:%s:%d:%s", strlen(COMMAND_GET_HISTORY), COMMAND_GET_HISTORY, strlen(project_name), project_name);
  int msg_size = strlen(buffer) + 1;
  printf("Sending {%s} to the server (%d) bytes in total\n", buffer, msg_size);
  write(connection->socket, &msg_size, sizeof(int));
  write(connection->socket, buffer, strlen(buffer) + 1);

  //Handle callback
  memset(buffer, 0, 200);
  read(connection->socket, ret_buffer, 100000);
  if (ret_buffer[0] == '1' && ret_buffer[1] == ':') {
    int i = 0;
    while (ret_buffer[i] != ':') i++;
    i++;
    while (ret_buffer[i] != ':') i++;
    i++;
    //print starting from this index
    while (ret_buffer[i] != 0) {
      printf("%c", ret_buffer[i]);
      i++;
    }
    printf("\n");
    free(ret_buffer);
    free(buffer);
    close(connection->socket);
    free(connection);
    return 1;
  } else {
    //error checks
    free(buffer);
    close(connection->socket);
    free(connection);
    if (ret_buffer[0] == '2') {
      free(ret_buffer);
      wtf_perror(E_SERVER_IMPROPER_PERMISSIONS, FATAL_ERROR);
    } else if (ret_buffer[0] == '3') {
      free(ret_buffer);
      wtf_perror(E_SERVER_MANIFEST_ALREADY_EXISTS, FATAL_ERROR);
    }
    return 0;
  }
}

/**
 * Push project command
 * 
 * Send the server our .Commit as well as all files we have. Server will handle most of the operation here.
 * 
 * We need to check if we have a .Commit, if we can establish a valid wtf_connection, and the project exists on the server
 * 
 */
int wtf_push(char *project_name) {
  //First we need to fetch the server .Manifest (this will check if the project exists on the server
  //We also want to fetch the client .Manifest because we need to use it later
  Manifest *server_manifest = fetch_server_manifest(project_name);
  Manifest *client_manifest = fetch_client_manifest(project_name);

  //Check if we have a .Commit on client
  char *buffer = malloc(1000);
  memset(buffer, 0, 1000);
  sprintf(buffer, "%s/.Commit", project_name);
  if (access(buffer, F_OK) != 0) {
    free_manifest(server_manifest);
    free_manifest(client_manifest);
    free(buffer);
    wtf_perror(E_CANNOT_PUSH_NO_COMMIT_ON_CLIENT, FATAL_ERROR);
  }

  char *commit_buffer = malloc(10000);
  memset(commit_buffer, 0, 10000);
  int commit_fd = open(buffer, O_RDONLY);
  if (commit_fd < 0) {
    free_manifest(client_manifest);
    free_manifest(server_manifest);
    free(buffer);
    free(commit_buffer);
    wtf_perror(E_CANNOT_READ_COMMIT, FATAL_ERROR);
  }

  int n = read(commit_fd, buffer, 1);
  if (n != 1) {
    free_manifest(client_manifest);
    free_manifest(server_manifest);
    free(buffer);
    free(commit_buffer);
    close(commit_fd);
    wtf_perror(E_CANNOT_READ_COMMIT, FATAL_ERROR);
  }
  while (n != 0) {
    sprintf(commit_buffer, "%s%c", commit_buffer, buffer[0]);
    n = read(commit_fd, buffer, 1);
  }
  close(commit_fd);
  printf("commit buffer loaded %s\n", commit_buffer);

  //Go over all of the files that were A and M opcode's and add them to the file_buffer which we will compress later
  int i;
  int fd = 0;
  char *file_buffer = malloc(1000000);
  char *mid_buffer = malloc(10000);
  memset(mid_buffer, 0, 10000);
  int t_size = 0;
  int total_files_to_send = 0;
  memset(buffer, 0, 1000);
  for (i = 0; i < strlen(commit_buffer) - 1; i++) {
    t_size = 0;
    memset(mid_buffer, 0, 10000);
    memset(buffer, 0, 1000);
    if ((commit_buffer[i] == OPCODE_ADD || commit_buffer[i] == OPCODE_MODIFY) && commit_buffer[i + 1] == ' ') {
      total_files_to_send++;
      i += 2;
      while (commit_buffer[i] != ' ') {
        sprintf(buffer, "%s%c", buffer, commit_buffer[i]);
        i++;
      }
      printf("we need to add file name: %s\n", buffer);
      strcat(file_buffer, buffer);
      strcat(file_buffer, ":");
      fd = open(buffer, O_RDONLY);
      if (fd == -1) {
        free(file_buffer);
        free(mid_buffer);
        free_manifest(client_manifest);
        free_manifest(server_manifest);
        free(buffer);
        free(commit_buffer);
        wtf_perror(E_CANNOT_READ_FILES_IN_COMMIT, FATAL_ERROR);
      }

      n = read(fd, buffer, 1);
      memset(mid_buffer, 0, 10000);
      if (n == -1) {
        free(file_buffer);
        free(mid_buffer);
        free_manifest(client_manifest);
        free_manifest(server_manifest);
        free(buffer);
        free(commit_buffer);
        wtf_perror(E_CANNOT_READ_FILES_IN_COMMIT, FATAL_ERROR);
      }

      while (n != 0) {
        sprintf(mid_buffer, "%s%c", mid_buffer, buffer[0]);
        t_size++;
        n = read(fd, buffer, 1);
      }

      close(fd);

      //finished reading file here
      sprintf(file_buffer, "%s%d:", file_buffer, t_size);
      strcat(file_buffer, mid_buffer);
      strcat(file_buffer, "\n");
    }
  }

  // printf("file buffer is finished here %s\n", file_buffer);

  //combine commit buffer and file buffer
  char *final_buffer = malloc(1000000);
  memset(final_buffer, 0, 1000000);
  //command_length:command_name:project_length:project_name:commit_length:commit_string:file_buffer_length:file_buffer
  sprintf(final_buffer, "%d:%s:%d:%s:%d:%s:%d:%s", strlen(COMMAND_CREATE_PUSH), COMMAND_CREATE_PUSH, strlen(project_name), project_name, strlen(commit_buffer), commit_buffer, strlen(file_buffer), file_buffer);
  printf("final buffer is %s\n", final_buffer);

  //establish connection
  wtf_connection *connection = wtf_connect();

  //write buffer to server
  printf("Sending %s (%d) bytes to the server\n", COMMAND_CREATE_PUSH, strlen(final_buffer));
  int msg_size = strlen(final_buffer) + 1;
  write(connection->socket, &msg_size, sizeof(int));
  write(connection->socket, final_buffer, strlen(final_buffer));
  memset(buffer, 0, 1000);
  n = read(connection->socket, buffer, 4);
  int ret_status = atoi(buffer);
  if (ret_status == 101) {
    printf("Successfully pushed\n");
    client_manifest->version_number += 1;
    for (i = 0; i < client_manifest->file_count; i++) {
      client_manifest->files[i]->seen_by_server = 1;
      client_manifest->files[i]->op_code = OPCODE_NONE;
    }
    free_manifest(server_manifest);
    server_manifest = fetch_server_manifest(project_name);
    write_manifest(server_manifest);
    memset(buffer, 0, 1000);
    sprintf(buffer, "./%s/.Commit", project_name);
    remove(buffer);

  } else {
    //Shit hit the fan, don't update manifest
    free(final_buffer);
    free(mid_buffer);
    free(buffer);
    free_manifest(client_manifest);
    free_manifest(server_manifest);
    wtf_perror(E_SERVER_FAILED_PUSH, FATAL_ERROR);
  }

  free(final_buffer);
  free(mid_buffer);
  free(buffer);
  free_manifest(client_manifest);
  free_manifest(server_manifest);
}

/**
 * Commit project command
 * 
 * Will fetch the server's .Manifest and check if the version match. If they don't, tell client to update project.
 * If they do, then run through the client .Manifest and compute live hash for each file listed. For each hash that is different then the stored hash in the client's .Manifest,
 * write out an entry to a .Commit with an incremented file version number
 *
 * Returns
 *  0 = Failure
 *  1 = Success
 * 
 */
int wtf_commit(char *project_name) {
  //First we need to fetch the server .Manifest and parse it
  Manifest *server_manifest = fetch_server_manifest(project_name);
  Manifest *client_manifest = fetch_client_manifest(project_name);
  printf("\n");
  print_manifest(server_manifest, SERVER, 1);
  printf("\n");
  print_manifest(client_manifest, CLIENT, 1);

  wtf_connection *connection = wtf_connect();

  //Most of the error checks are handled inside of fetch_server_manifest and fetch_client_manifest, which is why there aren't any checks above this line

  //Check if there is a NON-EMPTY .Update file
  char *buffer = malloc(1000);
  memset(buffer, 0, 1000);

  //.Update exists and is non-empty check
  sprintf(buffer, "%s/.Update", project_name);
  int n = 0;
  if (access(buffer, F_OK) != -1) {
    int fd = open(buffer, O_RDONLY);
    if (fd <= 0) {
      free(buffer);
      free(client_manifest);
      free(server_manifest);
      close(connection->socket);
      free(connection);
      wtf_perror(E_CANNOT_READ_UPDATE_FILE, FATAL_ERROR);
    }
    n = read(fd, buffer, 1);
    if (n != 0) {
      close(fd);
      free(buffer);
      free(client_manifest);
      free(server_manifest);
      close(connection->socket);
      free(connection);
      close(fd);
      wtf_perror(E_CANNOT_COMMIT_NON_EMPTY_UPDATE_EXISTS, FATAL_ERROR);
    }
    close(fd);
  }

  //.Conflict exists check
  memset(buffer, 0, 1000);
  sprintf(buffer, "%s/.Conflict", project_name);
  if (access(buffer, F_OK) != -1) {
    free(buffer);
    free(client_manifest);
    free(server_manifest);
    close(connection->socket);
    free(connection);
    wtf_perror(E_CANNOT_COMMIT_CONFLICT_EXISTS, FATAL_ERROR);
  }

  //Manifest Version Check
  if (server_manifest->version_number != client_manifest->version_number) {
    free(buffer);
    free(client_manifest);
    free(server_manifest);
    close(connection->socket);
    free(connection);
    wtf_perror(E_CANNOT_COMMIT_MISMATCHED_MANIFEST_VERSIONS, FATAL_ERROR);
  }

  //Rehash client's files
  int i;
  char *hash;
  for (i = 0; i < client_manifest->file_count; i++) {
    // printf("Old hash: %s\n", client_manifest->files[i]->hash);
    hash = hash_file(client_manifest->files[i]->file_path);
    client_manifest->files[i]->new_hash = malloc(SHA_DIGEST_LENGTH * 2 + 1);
    memset(client_manifest->files[i]->new_hash, 0, SHA_DIGEST_LENGTH * 2 + 1);
    strncpy(client_manifest->files[i]->new_hash, hash, strlen(hash));
    // printf("New hash: %s\n", client_manifest->files[i]->new_hash);
    free(hash);
    //if they are not equal then increment the version number
    if (strcmp(client_manifest->files[i]->hash, client_manifest->files[i]->new_hash) != 0) client_manifest->files[i]->version_number++;
  }

  //rewrite client manifest
  int res = write_manifest(client_manifest);
  if (res == 0) {
    free(buffer);
    free(hash);
    close(connection->socket);
    free(connection);
    free_manifest(server_manifest);
    free_manifest(client_manifest);
    return 0;
  }

  char *commit_buffer = malloc(500000);
  memset(commit_buffer, 0, 500000);
  int total_writes = 0;

  //Check if client needs to sync first
  int j;
  for (i = 0; i < client_manifest->file_count; i++) {
    for (j = 0; j < server_manifest->file_count; j++) {
      if (strcmp(client_manifest->files[i]->file_path, server_manifest->files[j]->file_path) == 0) {
        //same file, check if hashes are different
        if (strcmp(client_manifest->files[i]->hash, server_manifest->files[j]->hash) != 0) {
          //different hashes, check if server version number is greater or equal to clients
          if (server_manifest->files[j]->version_number >= client_manifest->files[i]->version_number) {
            //SHOULD FAIL NEED TO UPDATE
            free(commit_buffer);
            free(buffer);
            free(hash);
            free_manifest(server_manifest);
            free_manifest(client_manifest);
            close(connection->socket);
            free(connection);
            wtf_perror(E_CANNOT_COMMIT_MUST_SYNCH_FIRST, FATAL_ERROR);
          }
        }
      }
    }
  }

  //Modify Code case: server and client have the same file and same hash, but client new_hash != client hash
  for (i = 0; i < client_manifest->file_count; i++) {
    for (j = 0; j < server_manifest->file_count; j++) {
      if (strcmp(client_manifest->files[i]->file_path, server_manifest->files[j]->file_path) == 0) {
        //Same file, now lets check if hashes are different on client side
        if (strcmp(client_manifest->files[i]->new_hash, server_manifest->files[j]->hash) != 0) {
          //Client has different file, now make sure that the client hash == server hash
          if (strcmp(client_manifest->files[i]->hash, server_manifest->files[j]->hash) == 0) {
            //This is a match. We need to append a MODIFY opcode to .Commit
            if (total_writes != 0) {
              sprintf(commit_buffer, "%s\n", commit_buffer);
            }
            total_writes++;
            printf("%c %s\n", OPCODE_MODIFY, client_manifest->files[i]->file_path);
            sprintf(commit_buffer, "%s%c %s %s", commit_buffer, OPCODE_MODIFY, client_manifest->files[i]->file_path, server_manifest->files[j]->hash);
          }
        }
      }
    }
  }

  //Add Code case: server doesn't have the file but client does
  int server_has_file;
  for (i = 0; i < client_manifest->file_count; i++) {
    server_has_file = 0;
    for (j = 0; j < server_manifest->file_count; j++) {
      if (strcmp(client_manifest->files[i]->file_path, server_manifest->files[j]->file_path) == 0) {
        server_has_file = 1;
      }
    }
    if (server_has_file == 0) {
      printf("%c %s\n", OPCODE_ADD, client_manifest->files[i]->file_path);
      if (total_writes != 0) {
        sprintf(commit_buffer, "%s\n", commit_buffer);
      }
      total_writes++;
      sprintf(commit_buffer, "%s%c %s %s", commit_buffer, OPCODE_ADD, client_manifest->files[i]->file_path, client_manifest->files[i]->new_hash);
    }
  }

  //Delete Code case: server has the file but client doesn't
  int client_has_file;
  for (i = 0; i < server_manifest->file_count; i++) {
    client_has_file = 0;
    for (j = 0; j < client_manifest->file_count; j++) {
      if (strcmp(server_manifest->files[i]->file_path, client_manifest->files[j]->file_path) == 0) {
        client_has_file = 1;
      }
    }
    if (client_has_file == 0) {
      printf("%c %s\n", OPCODE_DELETE, server_manifest->files[i]->file_path);
      if (total_writes != 0) {
        sprintf(commit_buffer, "%s\n", commit_buffer);
      }
      total_writes++;
      sprintf(commit_buffer, "%s%c %s %s", commit_buffer, OPCODE_DELETE, server_manifest->files[i]->file_path, server_manifest->files[i]->hash);
    }
  }

  //Nothing to commit
  if (strlen(commit_buffer) == 0) {
    printf("Nothing to commit\n");
    free(commit_buffer);
    free(buffer);
    free(hash);
    free_manifest(server_manifest);
    free_manifest(client_manifest);
    close(connection->socket);
    free(connection);
    return 1;
  }

  //Write .Commit
  memset(buffer, 0, 1000);
  sprintf(buffer, "%s/.Commit", project_name);
  int fd = open(buffer, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    free(buffer);
    free(commit_buffer);
    free(hash);
    close(connection->socket);
    free(connection);
    free_manifest(server_manifest);
    free_manifest(client_manifest);
    wtf_perror(E_CANNOT_WRITE_COMMIT, FATAL_ERROR);
  }
  n = write(fd, commit_buffer, strlen(commit_buffer));
  if (n <= 0) {
    free(buffer);
    free(commit_buffer);
    free(hash);
    close(connection->socket);
    free(connection);
    free_manifest(server_manifest);
    free_manifest(client_manifest);
    close(fd);
    wtf_perror(E_CANNOT_WRITE_COMMIT, FATAL_ERROR);
  }

  //.Commit written, send it to the server
  char *server_buffer = malloc(500200);
  memset(server_buffer, 0, 500200);
  sprintf(server_buffer, "%d:%s:%d:%s:%d:%s", strlen(COMMAND_CREATE_COMMIT), COMMAND_CREATE_COMMIT, strlen(project_name), project_name, strlen(commit_buffer), commit_buffer);
  printf("Attempting to send (%d) bytes to the server\n", strlen(server_buffer));
  int msg_size = strlen(server_buffer) + 1;
  write(connection->socket, &msg_size, sizeof(int));
  write(connection->socket, server_buffer, strlen(server_buffer));
  memset(buffer, 0, 1000);
  n = read(connection->socket, buffer, 4);
  printf("return buffer is %s\n", buffer);
  int ret_status = atoi(buffer);
  if (ret_status == 101) {
    printf("Successfully sent .Commit to the Server\n");
  } else {
    //error checks handling here
    free(buffer);
    free(commit_buffer);
    free_manifest(server_manifest);
    free_manifest(client_manifest);
    close(fd);
    close(connection->socket);
    free(connection);
    free(server_buffer);
    wtf_perror(E_SERVER_CANNOT_READ_OR_WRITE_NEW_COMMIT, NON_FATAL_ERROR);
  }

  //free and return
  free(buffer);
  free(commit_buffer);
  free_manifest(server_manifest);
  free_manifest(client_manifest);
  close(fd);
  close(connection->socket);
  free(connection);
  free(server_buffer);
  return 1;
}

/**
 * 
 * wtf_remove
 * 
 * Removes a given file from the project's .Manifest
 */
int wtf_remove(char *project_name, char *file_path) {
  Manifest *client_manifest = fetch_client_manifest(project_name);

  char *file = malloc(1000);
  memset(file, 0, 1000);
  sprintf(file, "%s/%s", project_name, file_path);

  //Check that the file exists in the manifest
  int i;
  int index = -1;  //index where file is in client_manifest->files
  for (i = 0; i < client_manifest->file_count; i++) {
    printf("comparing %s and %s\n", client_manifest->files[i]->file_path, file);
    if (strcmp(client_manifest->files[i]->file_path, file) == 0)
      index = i;
  }
  if (index == -1) {
    //Doesn't exist in manifest
    free_manifest(client_manifest);
    wtf_perror(E_REMOVE_PROVIDED_FILE_NOT_IN_MANIFEST, FATAL_ERROR);
  }

  free(client_manifest->files[index]->file_path);
  client_manifest->files[index]->file_path = NULL;
  write_manifest(client_manifest);
  free(file);
  free_manifest(client_manifest);  // why does this cause a crash?
  return 1;
}

/**
 * write_manifest
 * 
 * Writes out Manifest object to the local .Manifest
 *  
 * Returns:
 *  0 = Failure
 *  1 = Success
 */
int write_manifest(Manifest *manifest) {
  char *buffer = malloc(500);
  sprintf(buffer, "%s/.Manifest", manifest->project_name);

  //remove file and create it again. We are going to write out the entire .Manifest anyway
  remove(buffer);
  int fd = open(buffer, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    wtf_perror(E_CANNOT_WRITE_TO_MANIFEST, NON_FATAL_ERROR);
    free(buffer);
    return 0;
  }

  //check if we can write
  int n = write(fd, manifest->project_name, strlen(manifest->project_name));
  if (n <= 0) {
    wtf_perror(E_CANNOT_WRITE_TO_MANIFEST, NON_FATAL_ERROR);
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
    if (manifest->files[i]->file_path == NULL) continue;  //skip over this file, means it is being deleted from the .Manifest
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
 * Helper function for fetching the server manifest and populating a Manifest data structure
 * 
 * Returns
 *  NULL = Failure
 *  Manifest* = Success
 */
Manifest *fetch_server_manifest(char *project_name) {
  wtf_connection *connection = wtf_connect();
  char *buffer = malloc(200);
  char *ret_string = malloc(500000);
  sprintf(buffer, "19:get_current_version:%d:%s", strlen(project_name) + 1, project_name);
  int msg_size = strlen(buffer) + 1;
  printf("Sending {%s} to the server (%d) bytes total\n", buffer, msg_size);
  write(connection->socket, &msg_size, sizeof(int));
  write(connection->socket, buffer, strlen(buffer) + 1);

  //Handle call back here
  memset(buffer, 0, 200);
  memset(ret_string, 0, 500000);
  read(connection->socket, ret_string, 500000);
  if (ret_string[0] == '1' && ret_string[1] == ':') {
    Manifest *server_manifest = malloc(sizeof(Manifest));
    server_manifest->project_name = malloc(120);
    strcpy(server_manifest->project_name, project_name);

    //Success, trim off first 2 chars to extract the string
    ret_string += 2;

    //String format is the following
    //    <project_version_number>:<file_count>:<file1_name_length>:<file1_name>:<file1_version_number>:<file2_name_length>:<file2_name>:<file2_version_number>...

    int i = 0;
    while (ret_string[i] != ':') {  // version number
      sprintf(buffer, "%s%c", buffer, ret_string[i]);
      i++;
    }
    i++;
    server_manifest->version_number = atoi(buffer);

    memset(buffer, 0, 200);
    while (ret_string[i] != ':') {  // total files
      sprintf(buffer, "%s%c", buffer, ret_string[i]);
      i++;
    }
    i++;
    server_manifest->file_count = atoi(buffer);
    server_manifest->files = malloc(sizeof(ManifestFileEntry *) * server_manifest->file_count);

    memset(buffer, 0, 200);
    int j = 0;
    for (j = 0; j < server_manifest->file_count; j++) {
      server_manifest->files[j] = malloc(sizeof(ManifestFileEntry));
      memset(buffer, 0, 200);
      while (ret_string[i] != ':') {  //file_path length
        sprintf(buffer, "%s%c", buffer, ret_string[i]);
        i++;
      }
      i++;
      server_manifest->files[j]->file_path = malloc(atoi(buffer));

      memset(buffer, 0, 200);
      while (ret_string[i] != ':') {  //file_path
        sprintf(buffer, "%s%c", buffer, ret_string[i]);
        i++;
      }
      i++;
      strcpy(server_manifest->files[j]->file_path, buffer);

      memset(buffer, 0, 200);
      while (ret_string[i] != ':') {  //file_version_number
        sprintf(buffer, "%s%c", buffer, ret_string[i]);
        i++;
      }
      i++;
      server_manifest->files[j]->version_number = atoi(buffer);

      memset(buffer, 0, 200);
      while (ret_string[i] != ':') {  //hash length
        sprintf(buffer, "%s%c", buffer, ret_string[i]);
        i++;
      }
      i++;
      server_manifest->files[j]->hash = malloc(atoi(buffer) + 1);

      memset(buffer, 0, 200);
      while (ret_string[i] != ':') {  //hash
        sprintf(buffer, "%s%c", buffer, ret_string[i]);
        i++;
      }
      i++;
      strcpy(server_manifest->files[j]->hash, buffer);

      server_manifest->files[j]->new_hash = NULL;
      server_manifest->files[j]->seen_by_server = 1;
      server_manifest->files[j]->op_code = OPCODE_NONE;
    }

    //Move pointer back so we can free the block correctly
    ret_string -= 2;

    free(buffer);
    free(ret_string);
    close(connection->socket);
    free(connection);
    return server_manifest;

  } else if (ret_string[0] == '7') {
    free(ret_string);
    free(buffer);
    close(connection->socket);
    free(connection);
    wtf_perror(E_SERVER_PROJECT_DOESNT_EXIST, FATAL_ERROR);
  } else {
    free(ret_string);
    free(buffer);
    close(connection->socket);
    free(connection);
    wtf_perror(E_SERVER_IMPROPER_PERMISSIONS, FATAL_ERROR);
  }
}

/**
 * Helper function for fetching the client manifest and populating a Manifest data structure
 * 
 * Returns
 *  NULL = Failure
 *  Manifest* = Success
 */
Manifest *fetch_client_manifest(char *project_name) {
  //First check that the project exists on the client
  DIR *dir = opendir(project_name);
  if (!dir) wtf_perror(E_PROJECT_DOESNT_EXIST_ON_CLIENT, FATAL_ERROR);

  //Fetch manifest
  char *buffer = malloc(200);
  char *builder = malloc(200);  //used when we need to pull out certain substrings of unkown length while reading from the buffer

  sprintf(buffer, "%s/.Manifest", project_name);
  int manifest_fd = open(buffer, O_RDONLY);
  if (manifest_fd < 0) {
    free(buffer);
    free(builder);
    wtf_perror(E_CANNOT_READ_MANIFEST, FATAL_ERROR);
  }

  //Count number of lines in file
  int n = read(manifest_fd, buffer, FATAL_ERROR);
  if (n <= 0) {
    free(buffer);
    free(builder);
    close(manifest_fd);
    wtf_perror(E_CANNOT_READ_MANIFEST, FATAL_ERROR);
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
    printf("\tFile path: %s\n", builder);
    client_manifest->files[j]->file_path = malloc(strlen(builder) + 1);
    memset(client_manifest->files[j]->file_path, 0, strlen(builder));
    strncpy(client_manifest->files[j]->file_path, builder, strlen(builder));

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
  free(builder);
  close(manifest_fd);
  closedir(dir);
  return client_manifest;
}

/**
 * 
 * Get current version of a project on the server
 * 
 * Contacts the server and attempts to retrieve all of the information about the project and prints it out
 * 
 * Returns:
 *  0 = Failure
 *  1 = Success
 */
int wtf_get_current_version(char *project_name) {
  Manifest *m = fetch_server_manifest(project_name);
  if (m == NULL) return 0;
  print_manifest(m, SERVER, 1);

  //free manifest
  free_manifest(m);

  return 1;
}

/**
 * Add file to project
 * 
 * Adds a file with a given pathname to a given project's .Manifest
 * 
 * Signify that the server has not seen this file yet also
 * 
 * Returns:
 *  0 = Failure
 *  1 = Success
 */
int wtf_add(char *project_name, char *file_name) {
  //First check that the project exists on the client
  DIR *dir = opendir(project_name);
  if (!dir) wtf_perror(E_PROJECT_DOESNT_EXIST_ON_CLIENT, FATAL_ERROR);
  //append the project name to the file
  char *file = malloc(strlen(project_name) + strlen(file_name) + 10);
  memset(file, 0, strlen(project_name) + strlen(file_name) + 10);
  sprintf(file, "%s/%s", project_name, file_name);

  //Check if the file exists in the project
  if (!isRegFile(file)) wtf_perror(E_FILE_DOESNT_EXIST_TO_ADD, FATAL_ERROR);

  //check if file already exists in the manifest
  char *buffer = malloc(150);
  char *manifest_header_buffer = malloc(120);
  char **file_entries = malloc(sizeof(char *) * 1000);
  file_entries[0] = malloc(200);
  sprintf(buffer, "%s/.Manifest", project_name);
  int manifest_fd = open(buffer, O_RDWR);
  if (manifest_fd < 0) wtf_perror(E_CANNOT_WRITE_TO_MANIFEST, FATAL_ERROR);

  int n = 1;
  memset(buffer, 0, 150);
  int curr_idx = 0;
  while (buffer[0] != '~' && n != 0) {
    n = read(manifest_fd, buffer, 1);
  }
  strncpy(manifest_header_buffer, buffer, strlen(buffer));
  //We only need to check if there are file entries in the .Manifest
  if (buffer[0] == '~') {
    while (n != 0) {
      n = read(manifest_fd, buffer, 1);
      if (n == 0) continue;
      if (buffer[0] == '\n') {
        curr_idx++;
        // file_entries[curr_idx] = "\0";
      } else if (buffer[0] == '~') {
        file_entries[curr_idx] = malloc(200);
      } else if (buffer[0] != '~' && buffer[0] != '\n' && buffer[0] != ' ') {
        strcat(file_entries[curr_idx], buffer);
      }
    }
  }

  //loop over file_entries and see if there is an entry with the same filename as the one provided
  int i;
  int j;
  char *temp_name_buffer = malloc(100);
  for (i = 0; i < 1000; i++) {
    j = 0;
    if (file_entries[i] != NULL && strlen(file_entries[i]) != 0) {
      // printf("[%d] - %s\n", i, file_entries[i]);
      while (file_entries[i][j + 2] != ':') {
        // printf("\t file_entries[i][j+2] = %c\n", file_entries[i][j + 2]);
        temp_name_buffer[j] = file_entries[i][j + 2];
        j++;
      }
      temp_name_buffer[j + 1] = '\0';

      printf("temp_name extracted = '%s'\n", temp_name_buffer);
      if (strcmp(temp_name_buffer, file) == 0) {
        wtf_perror(E_FILE_ALREADY_ADDED_TO_MANIFEST, FATAL_ERROR);
      }
    }
  }

  printf("here, lets add the new file to the manifest\n");

  //If we have made it here, then we can safely add the new entry to the end of the manifest

  memset(buffer, 0, 150);
  char *hash = malloc(SHA_DIGEST_LENGTH * 2 + 1);
  memset(hash, 0, SHA_DIGEST_LENGTH * 2 + 1);
  sprintf(hash, "%s", hash_file(file));
  sprintf(buffer, "\n~ A:%s:%d:%s:%s", file, 1, hash, "!");
  n = write(manifest_fd, buffer, strlen(buffer));
  if (n == 0) wtf_perror(E_CANNOT_WRITE_TO_MANIFEST, FATAL_ERROR);

  free(file);
  closedir(dir);
  free(hash);
  free(buffer);
  free(temp_name_buffer);
  return 1;
}

/**
 * Helper method to hash the contents of a file
 * 
 * Returns:
 *  NULL = Error
 *  char* = Hash string
 */
char *hash_file(char *path) {
  if (access(path, F_OK) == -1) wtf_perror(E_FILE_DOESNT_EXIST, FATAL_ERROR);
  char *file_contents_buffer = malloc(100000);
  memset(file_contents_buffer, 0, 100000);
  char *char_buffer = malloc(1);
  int file_fd = open(path, O_RDONLY);
  if (file_fd < 0) wtf_perror(E_CANNOT_READ_FILE, FATAL_ERROR);
  read(file_fd, file_contents_buffer, 100000);

  // printf("file contents buffer is %s, strlen is %d\n", file_contents_buffer, strlen(file_contents_buffer));
  file_contents_buffer[strlen(file_contents_buffer)] = '\0';

  unsigned char tmphash[SHA_DIGEST_LENGTH];
  memset(tmphash, 0, SHA_DIGEST_LENGTH);
  SHA1(file_contents_buffer, strlen(file_contents_buffer), tmphash);

  unsigned char *hash = malloc((SHA_DIGEST_LENGTH * 2) + 1);
  memset(hash, 0, SHA_DIGEST_LENGTH * 2);
  int i = 0;
  for (i = 0; i < SHA_DIGEST_LENGTH; i++)
    sprintf((char *)&(hash[i * 2]), "%02x", tmphash[i]);

  free(file_contents_buffer);
  free(char_buffer);
  close(file_fd);
  return hash;
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
int wtf_create_project(char *project_name) {
  //Establish connection to the server
  wtf_connection *connection = wtf_connect();
  char *buffer = malloc(100);
  memset(buffer, 0, 100);

  sprintf(buffer, "14:create_project:%d:%s", strlen(project_name) + 1, project_name);
  int msg_size = strlen(buffer) + 1;
  printf("Sending {%s} to the server (%d) bytes total\n", buffer, msg_size);
  write(connection->socket, &msg_size, sizeof(int));
  write(connection->socket, buffer, strlen(buffer) + 1);
  int n = read(connection->socket, buffer, 4);
  int ret_status = atoi(buffer);
  if (ret_status == 101) {
    printf("Successfully created Project Manifest on server.\n");

    //Now we need to create the project on the client side (including .Manifest);
    mkdir(project_name, 0700);
    char *path = malloc(200);
    sprintf(path, "./%s/.Manifest", project_name);
    if (access(path, F_OK) != -1)
      remove(path);
    int fd = open(path, O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR);
    if (fd == -1) {
      free(buffer);
      free(path);
      wtf_perror(E_CANNOT_WRITE_TO_MANIFEST, FATAL_ERROR);
    }
    write(fd, project_name, strlen(project_name));
    n = write(fd, "\n1", 2);
    if (n < 1) {
      free(buffer);
      free(path);
      wtf_perror(E_CANNOT_WRITE_TO_MANIFEST, FATAL_ERROR);
    }

    free(path);
    close(fd);
    free(connection);
    free(buffer);
    printf("Successfully created Project Manifest on Client\n");
    return 1;
  } else {
    if (ret_status == 105) {
      wtf_perror(E_SERVER_IMPROPER_PERMISSIONS, NON_FATAL_ERROR);
    } else {
      wtf_perror(E_SERVER_MANIFEST_ALREADY_EXISTS, NON_FATAL_ERROR);
    }
  }

  free(connection);
  free(buffer);
  return 0;
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
wtf_connection *wtf_connect() {
  //If configuration is not created, we need to read in the configuration file and load it into the struct
  if (CONFIGURATION == NULL) {
    char *buffer = malloc(200);
    int fd = -1;
    fd = open(WTF_CONFIGURATION_FILE_PATH, O_RDONLY);
    if (fd == -1) {
      free(buffer);
      wtf_perror(E_INVALID_CONFIGURATION, FATAL_ERROR);
    }
    int num_bytes = 0;
    num_bytes = read(fd, buffer, 200);
    // printf("read %d bytes from .configuration\n", num_bytes);
    if (num_bytes <= 0) {
      free(buffer);
      wtf_perror(E_INVALID_CONFIGURATION, FATAL_ERROR);
    }

    //Load the configuration file's contents into 2 variables, port_buffer and hostname_buffer
    CONFIGURATION = malloc(sizeof(wtf_configuration));
    char *port_buffer = strstr(buffer, "\n");  //this now stores \n<port>
    char *hostname_buffer = malloc(100);
    strncpy(hostname_buffer, buffer, port_buffer - buffer);  //load the first substring into hostname_buffer
    port_buffer++;                                           //trim off \n

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
  if (socket <= 0) {
    close(connection->socket);
    free(connection);
    wtf_perror(E_CANNOT_CREATE_SOCKET, FATAL_ERROR);
  }
  connection->address.sin_family = AF_INET;
  connection->address.sin_port = htons(CONFIGURATION->port);
  connection->host = gethostbyname(CONFIGURATION->hostname);
  if (connection->host == NULL) {
    close(connection->socket);
    free(connection);
    wtf_perror(E_UNKNOWN_HOST, FATAL_ERROR);
  }
  memcpy(&connection->address.sin_addr, connection->host->h_addr_list[0], connection->host->h_length);

  if (connect(connection->socket, (struct sockaddr *)&connection->address, sizeof(connection->address))) {
    close(connection->socket);
    free(connection);
    wtf_perror(E_CANNOT_CONNECT_TO_HOST, FATAL_ERROR);
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

/**
 * 
 * isRegFile
 * 
 * Checks if the path provided is a normal file and not a directory
 */
int isRegFile(const char *path) {
  struct stat statbuf;

  if (stat(path, &statbuf) != 0) {
    return 0;
  }

  return S_ISREG(statbuf.st_mode);
}

/**
 * 
 * print_manifest
 * 
 * Prints manifest as readable string
 */
void print_manifest(Manifest *m, int client_or_server, int verbose) {
  if (client_or_server == CLIENT) printf("[ CLIENT MANIFEST ] \n");
  if (client_or_server == SERVER) printf("[ SERVER MANIFEST ] \n");
  printf("Project: %s\n", m->project_name);
  printf("Version: %d\n", m->version_number);
  printf("%d Total Files:\n", m->file_count);
  int i;
  for (i = 0; i < m->file_count; i++) {
    if (client_or_server == SERVER) {
      if (!verbose) {
        printf("\t%s Version: %d\n", m->files[i]->file_path, m->files[i]->version_number);
      } else {
        printf("\t%s Version: %d; Hash: %s\n", m->files[i]->file_path, m->files[i]->version_number, m->files[i]->hash);
      }
    } else {
      printf("\t%s Version: %d; OPCode: %c; Hash: %s; Seen by server: %d\n", m->files[i]->file_path, m->files[i]->version_number, m->files[i]->op_code, m->files[i]->hash, m->files[i]->seen_by_server);
    }
  }
}

/**
 * sanitize_project
 * 
 * Go through all directories and delete them if they don't contain any files
 */
void sanitize_project(char *project) {
  char *buffer = malloc(1000);
  memset(buffer, 0, 1000);
  sprintf(buffer, "cd ./%s/ && find . -type d -empty -delete", project);
  // system(buffer);
  printf("Sanitized project\n");
  free(buffer);
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
  free(m->project_name);
  free(m->files);
  free(m);
}