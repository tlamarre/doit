#include "prelude.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/errno.h>
#include "prelude.h"

#define error(...) fprintf (stderr, __VA_ARGS__)

int main(int argc, char **argv) {
 
  FILE *fp;
  char ch[26];
  char *awesome = "awesome";
  int i = 0;
  printf("Yo.\n");
  fp = fopen("test.txt","r+w");
  if(fp == NULL) {
    printf("You got no file bro.\n");
    error("File open failed with %d\n",errno);
  }

  while (1) {
    ch[i] = fgetc (fp);
    if(ch[i] == EOF) {
      break;
    }
    else {
      printf("%d",ch[i]);
    }
    i++;
  }
  printf("\n%s\n",ch);
  //fprintf(fp,"%s",awesome);
  if (argc == 2) {
    fp = fopen(argv[1],"w");
    fprintf(fp, "This is a pretty dope file yo.");
  }
}
