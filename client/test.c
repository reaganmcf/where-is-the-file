#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const int PORT_NUMBER = 5000;

int main(int argc, char** argv) {
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
  sprintf(buffer, "./WTF create example_project");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "touch ./example_project/file1");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "echo some text > ./example_project/file1");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF add example_project example_project/file1");
  system(buffer);

  memset(buffer, 0, 500);
  sprintf(buffer, "./WTF commit example_project");

  free(buffer);

  return 0;
}