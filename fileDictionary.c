#include "prelude.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "userDictionary.h"
#include "prelude.h"

file_node_t *originalNode(char *name, file_dict_t *FileD) {
  file_node_t *n;
  n = FileD->head;
  while (n != NULL && strcmp(name,n->nickname) != 0) {
    n = n->next;
  }
  return n;
}

void insertFileD(char *nick, file_dict_t *FileD, int fileID) {
  file_node_t *n;
  file_node_t *ogNode;
  userEntry *branch;

  
  if (!containsFileD(userName,FileD)) {
    n = (node_t *)malloc(sizeof(node_t));
    n->nickname = nick;
    n->next = FileD->head;
    FileD->head = n;
    n->fileID = fileID;
  } else {  //Here if the word is already in the dictionary, the fileID will be replaced with the newly-defined one
    ogNode = originalNode(userName, FileD);
    ogNode->fileID = fileID;
  }
}

boolean containsFileD(char *name, file_dict_t *FileD) {
  node_t *n;
  n = FileD->head;
  while (n != NULL && strcmp(name,n->nickname) != 0) {
    n = n->next;
  }
  return (n != NULL);
}

file_dict_t *newFileD(int initial_size) {
  // suggested size is ignored
  file_dict_t *FileD;
  FileD = (file_dict_t *)malloc(sizeof(file_dict_t));
  FileD->head = NULL;
  return FileD;
}

int getFileID(char *nick,file_dict_t *FileD) {
  node_t *n;
  n = FileD->head;
  while (n != NULL && strcmp(nick,n->nickname) != 0) {
    n = n->next;
  }
  return (n->fileID);
}


void outputFileD(file_dict_t *FileD) {
  node_t *n;
  node_t *branch;
  boolean first;
  n = FileD->head;
  printf("{");
  for (n = FileD->head, first = TRUE; n != NULL; n = n->next) {
    if (!first) printf(", ");
    printf("%s : %d",n->nickname,n->fileID);
    first = FALSE;
    branch = n;
  }
  printf("}");
}

