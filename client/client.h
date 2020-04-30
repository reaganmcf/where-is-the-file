#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>

#ifndef CLIENT_H
#define CLIENT_H

#define WTF_CONFIGURATION_FILE_PATH "./.configuration"
#define OPCODE_ADD 'A'
#define OPCODE_DELETE 'D'
#define OPCODE_MODIFY 'M'
#define OPCODE_NONE 'N'
#define CLIENT 0
#define SERVER 1

//Possible Command Strings for the Server
const char *COMMAND_CREATE_PROJECT = "create_project";
const char *COMMAND_CURRENT_VERSION_PROJECT = "get_current_version";
const char *COMMAND_CREATE_COMMIT = "create_commit";
const char *COMMAND_CREATE_PUSH = "create_push";
const char *COMMAND_GET_HISTORY = "get_history";
const char *COMMAND_DESTORY_PROJECT = "destroy_project";

//Possible Error Codes for the Client
enum _error_codes {
  E_INVALID_CONFIGURATION = 1,
  E_CONFIGURATION_WRITE_ERROR = 2,
  E_SERVER_NOT_FOUND = 3,
  E_IMPROPER_PARAMS_AND_FLAGS = 4,
  E_IMPROPER_CONFIGURATION_PARAMS = 5,
  E_CANNOT_CREATE_SOCKET = 6,
  E_UNKNOWN_HOST = 7,
  E_CANNOT_CONNECT_TO_HOST = 8,
  E_IMPROPER_CREATE_PARAMS = 9,
  E_IMPROPER_CREATE_PROJECT_NAME_LENGTH = 10,
  E_IMPROPER_CREATE_PROJECT_NAME = 11,
  E_SERVER_MANIFEST_ALREADY_EXISTS = 12,
  E_SERVER_IMPROPER_PERMISSIONS = 13,
  E_CANNOT_WRITE_TO_MANIFEST = 14,
  E_PROJECT_DOESNT_EXIST_ON_CLIENT = 15,
  E_IMPROPER_ADD_PARAMS = 16,
  E_FILE_DOESNT_EXIST_TO_ADD = 17,
  E_FILE_ALREADY_ADDED_TO_MANIFEST = 18,
  E_FILE_DOESNT_EXIST = 19,
  E_CANNOT_READ_FILE = 20,
  E_FILE_MAX_LENGTH = 21,
  E_IMPROPER_CURRENT_VERSION_PARAMS = 22,
  E_IMPROPER_CURRENT_VERSION_PROJECT_NAME = 23,
  E_SERVER_PROJECT_DOESNT_EXIST = 24,
  E_IMPROPER_COMMIT_PARAMS = 25,
  E_IMPROPER_COMMIT_PROJECT_NAME = 26,
  E_CANNOT_READ_MANIFEST = 27,
  E_CANNOT_READ_UPDATE_FILE = 28,
  E_CANNOT_COMMIT_NON_EMPTY_UPDATE_EXISTS = 29,
  E_CANNOT_COMMIT_CONFLICT_EXISTS = 30,
  E_CANNOT_COMMIT_MISMATCHED_MANIFEST_VERSIONS = 31,
  E_IMPROPER_REMOVE_PARAMS = 32,
  E_REMOVE_PROVIDED_FILE_NOT_IN_MANIFEST = 33,
  E_CANNOT_COMMIT_MUST_SYNCH_FIRST = 34,
  E_CANNOT_WRITE_COMMIT = 35,
  E_SERVER_CANNOT_READ_OR_WRITE_NEW_COMMIT = 36,
  E_IMPROPER_PUSH_PARAMS = 37,
  E_IMPROPER_PUSH_PROJECT_NAME = 38,
  E_CANNOT_PUSH_NO_COMMIT_ON_CLIENT = 39,
  E_CANNOT_READ_COMMIT = 40,
  E_CANNOT_READ_FILES_IN_COMMIT = 41,
  E_NO_COMMAND_PROVIDED = 42,
  E_SERVER_FAILED_PUSH = 43,
  E_IMPROPER_HISTORY_PARAMS = 44,
  E_IMPROPER_DESTROY_PARAMS = 45,
  E_IMPROPER_DESTROY_PROJECT_NAME = 46
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
    {E_CANNOT_CREATE_SOCKET, "Failed to create socket.\n"},
    {E_UNKNOWN_HOST, "Unknwon host provided\n"},
    {E_CANNOT_CONNECT_TO_HOST, "Failed to connect to the host and port provided"},
    {E_IMPROPER_CREATE_PARAMS, "Imporper params for create command. Please follow the format of ./WTF create <project-name>"},
    {E_IMPROPER_CREATE_PROJECT_NAME_LENGTH, "Improper project name for create command. Please ensure your project name length is between 1 and 100 characters long"},
    {E_IMPROPER_CREATE_PROJECT_NAME, "Improper project name for create command. Please make sure the project name provided does not contain ':'."},
    {E_SERVER_MANIFEST_ALREADY_EXISTS, "Please create a project name that is unique."},
    {E_SERVER_IMPROPER_PERMISSIONS, "The server could not read and/or write to the Manifests directory. Please make sure the server is correctly configured."},
    {E_CANNOT_WRITE_TO_MANIFEST, "Improper permissions to write .Manifest."},
    {E_PROJECT_DOESNT_EXIST_ON_CLIENT, "Project doesn't exist on the client."},
    {E_IMPROPER_ADD_PARAMS, "Improper Params for add command. Please follow the format of ./WTF add <project-name> <file-path>"},
    {E_FILE_DOESNT_EXIST_TO_ADD, "Provided file path for add command not found. Please follow the following format example: ./WTF add project project/myfile.txt"},
    {E_FILE_ALREADY_ADDED_TO_MANIFEST, "File already exists in .Manifest for the client. Aborting add command"},
    {E_FILE_DOESNT_EXIST, "Provided file path does not exist"},
    {E_CANNOT_READ_FILE, "Provided file path does not have proper read permissions"},
    {E_FILE_MAX_LENGTH, "Provided file is very large. Only reading the first 10000 characters"},
    {E_IMPROPER_CURRENT_VERSION_PARAMS, "Improper params for currentversion command. Please follow the format of ./WTF currentversion <project-name>"},
    {E_IMPROPER_CURRENT_VERSION_PROJECT_NAME, "Improper project name provided for currentversion. Project names cannot contain ':'."},
    {E_SERVER_PROJECT_DOESNT_EXIST, "Provided project name doesn't exist on the server."},
    {E_IMPROPER_COMMIT_PARAMS, "Improper params for commit command. Please follow the format of ./WTF commit <project-name>"},
    {E_IMPROPER_COMMIT_PROJECT_NAME, "Improper project name provided for commit. Project names cannot contain ':'"},
    {E_CANNOT_READ_MANIFEST, "Improper permissions to read .Manifest"},
    {E_CANNOT_READ_UPDATE_FILE, "Improper permissions to read .Update"},
    {E_CANNOT_COMMIT_NON_EMPTY_UPDATE_EXISTS, "Unable to commit because non-empty .Update exists. Please update the project first."},
    {E_CANNOT_COMMIT_CONFLICT_EXISTS, "Unable to commit because .Conflict exists"},
    {E_CANNOT_COMMIT_MISMATCHED_MANIFEST_VERSIONS, "Unable to commit because server .Manifest and client .Manifest have mismatched version numbers. Please update your local project first"},
    {E_IMPROPER_REMOVE_PARAMS, "Improper params for remove command. Please follow the format of ./WTF remove <project-name> <file-path>"},
    {E_REMOVE_PROVIDED_FILE_NOT_IN_MANIFEST, "Provided file path doesn't exist in the project's Manifest"},
    {E_CANNOT_COMMIT_MUST_SYNCH_FIRST, "Cannot commit because the client must synch with repository before committing changes."},
    {E_CANNOT_WRITE_COMMIT, "Improper permissions to write to .Commit"},
    {E_SERVER_CANNOT_READ_OR_WRITE_NEW_COMMIT, "Unable to read new .Commit to the project directory. Please check your permissions to this directory"},
    {E_IMPROPER_PUSH_PARAMS, "Improper params for push command. Please follow the format of ./WTF push <project-name>"},
    {E_IMPROPER_PUSH_PROJECT_NAME, "Improper project name provided for currentversion. Project names cannot contain ':'."},
    {E_CANNOT_PUSH_NO_COMMIT_ON_CLIENT, "Unable to push because client does not have a .Commit. Please run ./WTF commit <project-name> first."},
    {E_CANNOT_READ_COMMIT, "Improper permissions to read .Commit"},
    {E_CANNOT_READ_FILES_IN_COMMIT, "Unable to read files listed in the .Commit"},
    {E_NO_COMMAND_PROVIDED, "Provided input params didn't match any command patterns. Please enter a valid command according to the README"},
    {E_SERVER_FAILED_PUSH, "Couldn't successfully run push command because the server encountered an error. Please check server output to see what happened."},
    {E_IMPROPER_HISTORY_PARAMS, "Improper params for history command. Please follow the format of ./WTF history <project-name>"},
    {E_IMPROPER_DESTROY_PARAMS, "Improper params for destroy command. Please follow the format of ./WTF destroy <project-name>"},
    {E_IMPROPER_DESTROY_PROJECT_NAME, "Improper project name provided for destroy. Project names cannot contain ':'."}

};

//Struct for a wtf_connection
typedef struct _wtf_connection {
  int socket;
  struct sockaddr_in address;
  struct hostent *host;
  int port;
  int len;
} wtf_connection;

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
  ManifestFileEntry **files;
} Manifest;

//Struct for .configuration
typedef struct _configuration {
  char *hostname;
  int port;
} wtf_configuration;

//Function Prototype for freeing all memory after exit
static void wtf_exit_handler(void);

//Function Prototype to handle attemtping connection and returning a valid wtf_connection struct
wtf_connection *wtf_connect();

//Function Prototype for printing custom errors
void wtf_perror(wtf_error e, int should_exit);

//Function Prototype for writing out .configuration
int wtf_configure_host(char *hostname, char *port);

//Function Prototype for adding file to .Manifest on client
int wtf_add(char *, char *);

//Function Prototype for fetching current state of a project on the server
int wtf_get_current_version(char *);

//Function Prototype for commit
int wtf_commit(char *);

//Function Prototype for fetching the server .Manifest and populating Manifest struct
Manifest *fetch_server_manifest(char *);

//Function Prototype for fetching the client .Manifest and populating Manifest struct
Manifest *fetch_client_manifest(char *);

//Function Prototype for writing out Manifest object to .Manifest
int write_manifest(Manifest *);

//Function Prototype for removing entry from .Manifest
int wtf_remove(char *, char *);

//Function Prototype for push a commit to server
int wtf_push(char *);

//Function Prototype for retrieving history of repo
int wtf_history(char *);

//Function Prototype for destroying project
void wtf_destory(char *);

//Function Prototype for printing Manifest out as string
void print_manifest(Manifest *, int, int);

//Function Prototype for freeing Manifest
void free_manifest(Manifest *);

//Function Prototype for hashing a file helper function
char *hash_file(char *path);

//Function Prototype for checking if a path is a normal file and not a directory
int isRegFile(const char *);

#endif