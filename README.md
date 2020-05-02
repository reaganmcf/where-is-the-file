# AsstLast


## Implementation (Server and Client)
Since this project is complex, many of the building blocks need to be very robust for it to work as intended. Because of this, we made sure to create a detailed `.Manifest` structure for the client, as well as Protocol to communicate back and forth between the `CLIENT` and `SERVER` as reliably as possible. We go into more depth about how these are structured below.  

Since we had to make the server multi-threaded, we made sure to keep in mind the issues that can arise from this type of design. Most importantly, 2 threads operating at the same time. We needed the functionality of locking the repository at any given time for any given thread, but locking the entire project directory is too overkill. So, we decided to create a linked list of `RepositoryLocks` that give us a way to assign a lock to a particular repository.

```c
typedef struct _repository_lock {
  pthread_mutex_t lock;
  char *project_name;
  struct _repository_lock *next;
} RepositoryLock;
```

These `RepositoryLocks` get init'd at the start, and get locked accordingly throughout the codebase, whenever a particular function needs to lock the project. 

Since the server doesn't exit when it finishes handling a connection, memory allocation has to be tracked carefully. Due to the modularization of how we structured our project, as described in the next few paragraphs, this was much easier to debug since we made sure to keep it in mind from the start.

When the server does exit, crash, or recieves SIGINT, we also made sure to properly intercept these by using `atexit()` and `signal()`. If any of these happen, it will make sure to `pthread_mutex_destory` all `RepositoryLocks` and stop listening for incoming connections before terminating the process.

---

Our client had to be modular to allow for a readable codebase when the project is as complex and large as this one. For this, we made sure to abstract out common functions, such as `hash_file(char* file_path)`, `fetch_client_manifest(char* project_name)`, `fetch_server_manifest(char* project_name)`, and more. This greatly reduces the amount of code we have to write on the client side and allows for a much more stable program as we don't have to make the same change in multiple locations.

Another aspect of the client that we made sure to get down perfect was how we handle connections. Almost every function has to communicate with the server, so writing a function handler for creating and retrieving a connection, while handling all errors and edge cases that can happen, was essential.

We accomplised this by creating a custom `wtf_connection` data structure that stores the information we need to read and write from the server at any time.

```c
typedef struct _wtf_connection {
  int socket;
  struct sockaddr_in address;
  struct hostent *host;
  int port;
  int len;
} wtf_connection;
```

Even better, we created a function called `wtf_connect` that will read from the `.configuration` file, attempt to establish a connection, and error check the entire process printing errors to STDOUT when needed. If the connection was successful, it returns a `wtf_connection*` which we can use anywhere. Example:

```c
wtf_connection* new_connection = wtf_connect()
char buffer[50];
memset(buffer, 0, 50);
sprintf(buffer, "18:my message to send");
write(new_connection->socket, buffer, strlen(buffer));
close(new_connection->socket);
free(new_connection);
```

Now that we had connections and generalized functions sorted out, we needed to create a plan for handling each function input. We modularized this as much as we could by making sure `int main()` doesn't `malloc()` or do any heavy lifting, but just checks the input commands, and passes the values in `argv` to the specific function. We split up each comamnd into its own function with the format of `wtf_push(char* project_name)`, `wtf_commit(char* project_name)`, etc. This made `free()`ing any memory we allocated much easier to track, and allowed us to free all of our memory without much debugging.

### Protocol

Since networking has reading unknown length of bytes as a core fundamental of networking operations, we made sure to design our protocol with a way to properly handle this in mind. We accomplished this by sending the length of the information before the information itself, allowing the server to allocate exactly the space it needs.

1) Send length of command
2) Send command name
3) For the rest of the params, send length followed by data

Example:
`14:create_project:10:project1.1`
`19:get_current_version:8:project`


### Manifest Structure
Manifests are in the following format
```
<project_name>
<version_number>
~ <A/D>:<file_path:name>:<file_current_version>:<file_hash>:!
~ <A/D>:<file_path:name>:<file_current_version>:<file_hash>:
~ <A/D>:<file_path:name>:<file_current_version>:<file_hash>:  
```
If a file entry has `!` at the end, then it means that the server has not seen it.

We compute the `file_hash` using `SHA1()` from the  OpenSSL Library.
