#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv) {
  int PORT_NUMBER = 5203;

  char* buffer = malloc(500);

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
  sprintf(buffer, "touch ./project/file1");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "echo some text > ./project/file1");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF add project file1");
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

  free(buffer);

  return 0;
}