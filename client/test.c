#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv) {
  int lower = 2000, upper = 10000;
  int PORT_NUMBER = 5030;

  char* buffer = malloc(500);

  //find an open port
  int n = 1;
  while (n >= 1) {
    memset(buffer, 0, 500);
    sprintf(buffer, "grep -w %d /etc/services > .tempresult", PORT_NUMBER);
    system(buffer);

    memset(buffer, 0, 500);
    sprintf(buffer, ".tempresult");
    int fd = open(buffer, O_RDONLY);
    if (fd == -1) {
      printf("Unable to write temporary output to check if port was already in use\n");
      free(buffer);
      return 0;
    }
    close(fd);

    n = read(fd, buffer, 20);
    printf("read %d bytes from .tempresult\n", n);

    // remove(".tempresult");
    printf("random port already taken... trying another port in 3 seconds\n");
    sleep(3);
  }

  memset(buffer, 0, 500);
  sprintf(buffer, ".tempresult");
  remove(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "cd ./../server && make && ./WTFserver %d &", PORT_NUMBER);
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "make");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF configure localhost %d", PORT_NUMBER);
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF destroy project");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF create project");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "mkdir ./project/folder && touch ./project/folder/file1 && echo some text > ./project/folder/file1");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF add project folder/file1");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF commit project");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF push project");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "touch ./project/file2");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "echo some text > ./project/file2");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF add project file2");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF commit project");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF push project");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "echo more text again > ./project/file3");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "cp ./project/.Manifest ./");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF add project file3");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF commit project");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF push project");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "mv ./.Manifest ./project/.Manifest");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF update project");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF upgrade project");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF rollback project 3");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF history project");
  system(buffer);

  //find the pid of the server process
  memset(buffer, 0, 500);
  sprintf(buffer, "lsof -i :%d | awk '{system(\"kill -SIGINT \" $2)}' &> /dev/null", PORT_NUMBER);
  system(buffer);

  printf("Killed server. All done testing!\n");

  free(buffer);

  return 0;
}