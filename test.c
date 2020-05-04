#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv) {
  int lower = 2000, upper = 10000;
  int PORT_NUMBER = 5823;

  char* buffer = malloc(500);

  //find an open port
  int n = 0;
  while (n >= 1) {
    PORT_NUMBER = (rand() % (upper - lower + 1)) + lower;
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

    n = read(fd, buffer, 1);
    printf("read %d bytes from .tempresult\n", n);

    remove(".tempresult");
    // printf("random port already taken... trying another port in 3 seconds\n");
    // printf("%d written \n", n);

    close(fd);
  }

  printf("we are going for port %d\n", PORT_NUMBER);

  memset(buffer, 0, 500);
  sprintf(buffer, ".tempresult");
  remove(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "make");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "make WTFserver && ./WTFserver %d &", PORT_NUMBER);
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "make WTF");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF configure localhost %d", PORT_NUMBER);
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF destroy project &> /dev/null");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF create project");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "mkdir ./project/folder &> /dev/null && touch ./project/folder/file1 &> /dev/null && echo some text > ./project/folder/file1");
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

  memset(buffer, 0, 500);
  sprintf(buffer, "lsof -i | grep WTFserver > .tempresult");
  system(buffer);

  memset(buffer, 0, 500);
  int fd = open(".tempresult", O_RDONLY);
  read(fd, buffer, 50);
  char* pid = malloc(50);
  memset(pid, 0, 50);
  int i = 0;
  int offset = 0;
  if (strlen(buffer) == 0) return;
  while (buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' && buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8' && buffer[i] != '9') {
    i++;
    offset++;
  }
  i = 0;
  while (buffer[i + offset] != ' ') {
    sprintf(pid, "%s%c", pid, buffer[i + offset]);
    i++;
  }
  //find the pid of the server process
  printf("pid is %s\n", pid);
  sprintf(buffer, "kill -2 %s", pid);
  system(buffer);

  printf("Killed server. All done testing!\n");

  free(buffer);
  free(pid);
  return 0;
}