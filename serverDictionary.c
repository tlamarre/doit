#include "prelude.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "userDictionary.h"
#include "prelude.h"

node_t *originalNode(char *name, dict_t *D) {
  node_t *n;
  n = D->head;
  while (n != NULL && strcmp(name,n->entry) != 0) {
    n = n->next;
  }
  return n;
}

void insertD(char *userName, dict_t *D, char *userAdress, int port) {
  node_t *n;
  node_t *ogNode;
  userEntry *entry;
  userEntry *branch;

  entry = (userEntry *)malloc(sizeof(userEntry));
  entry->address = userAdress;
  entry->port = port;
  
  pthread_mutex_lock(D->lock);
  if (!containsD(userName,D)) {
    n = (node_t *)malloc(sizeof(node_t));
    n->entry = userName;
    n->next = D->head;
    D->head = n;
    n->down = entry;
  } else {  //Here if the word is already in the dictionary, it will start branching down.
    ogNode = originalNode(userName, D);
    branch = ogNode->down;
    while (branch->nextEntry != NULL) {
      branch = branch->nextEntry;
    }
    branch->nextEntry = entry;
  }
  pthread_mutex_unlock(D->lock);
}

boolean containsD(char *name, dict_t *D) {
  node_t *n;
  n = D->head;
  while (n != NULL && strcmp(name,n->entry) != 0) {
    n = n->next;
  }
  return (n != NULL);
}

dict_t *newD(int initial_size) {
  // suggested size is ignored
  dict_t *D;
  D = (dict_t *)malloc(sizeof(dict_t));
  D->head = NULL;
  D->lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(D->lock,NULL);
  return D;
}

char *getLocation(char *entry,dict_t *D) {
  node_t *n;
  userEntry *user;
  n = D->head;
  while (n != NULL && strcmp(entry,n->entry) != 0) {
    n = n->next;
  }
  user = n->down;
  return (user->address);
}


void outputD(dict_t *D) {
  node_t *n;
  node_t *branch;
  boolean first;
  n = D->head;
  printf("{");
  for (n = D->head, first = TRUE; n != NULL; n = n->next) {
    if (!first) printf(", ");
    printf("%s",n->entry);
    first = FALSE;
    branch = n;
  }
  printf("}");
}

