#include "prelude.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "jobDictionary.h"
#include "prelude.h"

job_node_t *originalNode(char *name, job_dict_t *JobD) {
  job_node_t *n;
  n = JobD->head;
  while (n != NULL && strcmp(name,n->nickname) != 0) {
    n = n->next;
  }
  return n;
}

void insertJobD(char *nick, job_dict_t *JobD, int jobID) {
  job_node_t *n;
  job_node_t *ogNode;

  
  if (!containsJobD(nick,JobD)) {
    n = (job_node_t *)malloc(sizeof(job_node_t));
    n->nickname = nick;
    n->next = JobD->head;
    JobD->head = n;
    n->jobID = jobID;
  } else {  //Here if the word is already in the dictionary, the jobID will be replaced with the newly-defined one
    ogNode = originalNode(nick, JobD);
    ogNode->jobID = jobID;
  }
}

boolean containsJobD(char *name, job_dict_t *JobD) {
  job_node_t *n;
  n = JobD->head;
  while (n != NULL && strcmp(name,n->nickname) != 0) {
    n = n->next;
  }
  return (n != NULL);
}

job_dict_t *newJobD(int initial_size) {
  // suggested size is ignored
  job_dict_t *JobD;
  JobD = (job_dict_t *)malloc(sizeof(job_dict_t));
  JobD->head = NULL;
  return JobD;
}

int getJobID(char *nick,job_dict_t *JobD) {
  job_node_t *n;
  n = JobD->head;
  while (n != NULL && strcmp(nick,n->nickname) != 0) {
    n = n->next;
  }
  return (n->jobID);
}


void outputJobD(job_dict_t *JobD) {
  job_node_t *n;
  job_node_t *branch;
  boolean first;
  n = JobD->head;
  printf("{");
  for (n = JobD->head, first = TRUE; n != NULL; n = n->next) {
    if (!first) printf(", ");
    printf("%s : %d",n->nickname,n->jobID);
    first = FALSE;
    branch = n;
  }
  printf("}");
}

