#include <linux/in.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>

#ifndef SERVER_H
#define SERVER_H

#define OPCODE_ADD 'A'
#define OPCODE_DELETE 'D'
#define OPCODE_MODIFY 'M'
#define OPCODE_NONE 'N'
#define NON_FATAL_ERROR 0
#define FATAL_ERROR 1

//Possible Command Strings for the Server
const char *COMMAND_CREATE_PROJECT = "create_project";
const char *COMMAND_CURRENT_VERSION_PROJECT = "get_current_version";
const char *COMMAND_CREATE_COMMIT = "create_commit";
const char *COMMAND_CREATE_PUSH = "create_push";
const char *COMMAND_GET_HISTORY = "get_history";
const char *COMMAND_DESTORY_PROJECT = "destroy_project";
const char *COMMAND_ROLLBACK_PROJECT = "rollback_project";

//Possible Error Codes for the Server
enum _error_codes {
  E_IMPROPER_PARAMS = 1,
  E_ERROR_MAKING_SOCKET = 2,
  E_ERROR_BINDING_SOCKET_TO_PORT = 3,
  E_CANNOT_LISTEN_TO_PORT = 4,
  E_CANNOT_READ_OR_WRITE_PROJECT_DIR = 5,
  E_PROJECT_ALREADY_EXISTS = 6,
  E_PROJECT_DOESNT_EXIST = 7,
  E_CANNOT_INIT_MUTEX = 8,
  E_CANNOT_READ_OR_WRITE_NEW_COMMIT = 9
};

typedef enum _error_codes wtf_error;

struct _error_desc {
  int code;
  char *message;
} errordesc[] = {
    {},
    {E_IMPROPER_PARAMS, "Improper params. Please make sure to run the server with the format of ./WTFServer <port>"},
    {E_ERROR_MAKING_SOCKET, "There was an issue while creating the socket"},
    {E_ERROR_BINDING_SOCKET_TO_PORT, "There was an error while attempting to bind the socket to the port provided."},
    {E_CANNOT_LISTEN_TO_PORT, "Unable to have the socket listen on the provided port."},
    {E_CANNOT_READ_OR_WRITE_PROJECT_DIR, "Unable to read or write to ./Projects/ directory."},
    {E_PROJECT_ALREADY_EXISTS, "Project already exists with this name."},
    {E_PROJECT_DOESNT_EXIST, "Project doesn't exist on the server."},
    {E_CANNOT_INIT_MUTEX, "Cannot Initialize mutex lock."},
    {E_CANNOT_READ_OR_WRITE_NEW_COMMIT, "Unable to read new .Commit to the project directory. Please check your permissions to this directory"}

};

//Struct for handling complicated file entries in .Manifest
typedef struct _manifest_file {
  char op_code;
  char *file_path;
  int version_number;
  char *hash;
  char *new_hash;  //used when commiting
  int seen_by_server;
} ManifestFileEntry;

//Struct for handling complicated .Manifest
typedef struct _manifest {
  char *project_name;
  int version_number;
  int file_count;
  int new_file_count;
  ManifestFileEntry **files;
  ManifestFileEntry **new_files;
} Manifest;

//Struct for handling complicated commit operations
typedef struct _commit_op {
  char op_code;
  char *file_path;
  char *contents;
} CommitOperation;

//connection struct
typedef struct
{
  int socket;
  struct sockaddr address;
  int addr_len;
} wtf_connection;

//Function Prototype for multithreaded connection handler
void *wtf_process(void *);

//Function Prototype for exit handler
static void wtf_server_exit_handler(void);

//Function Prototype for creating the Project Manifest
int wtf_server_create_project(char *);

//Function Prototype for retrieving the state of the Project
char *wtf_server_get_current_version(char *project_name);

//Function Prototype for printing custom errors
void wtf_perror(wtf_error e, int should_exit);

//Function Prototype for writing incoming .Commit
char *wtf_server_write_commit(char *, char *);

//Function Prototype for handling push
char *wtf_server_push(char *, char *, char *);

//Function Prototype for handling get_history command
char *wtf_server_get_history(char *);

//Function Prototype for handling destory_project command
int wtf_server_destroy_project(char *);

//Function Prototype for fetching manifest on server side
Manifest *fetch_manifest(char *);

//Function Prototype for printing manifest
void print_manifest(Manifest *, int);

//Function Prototype for writing manifest
int write_manifest(Manifest *);

//Function Prototype for freeing manifest
void free_manifest(Manifest *);

//Function Prototype for hashing string helper function
char *hash_string(char *);

//Function Prototype for checking if path is a regular file
int isRegFile(char *path);

#endif