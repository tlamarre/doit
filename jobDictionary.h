//New implementation of a dictionary for user info
#ifndef _JOB_DICT_H
#define _JOB_DICT_H
#include "prelude.h"

typedef struct _job_dict_t {
  int size;
  int elements; //I added this because it seemed to be easier to just keep track of how many
                //elements were in the dict_t rather than iterating through to check when we
                //are inserting elements.
  struct _job_node_t *head;
} job_dict_t;

typedef struct _job_node_t {
  char *nickname;
  int jobID; // TODO: Make job dictionaries actually contain jobDescriptor structs -TL
  struct _job_node_t *next;
} job_node_t;


job_dict_t *newJobD(int initial_size);
boolean containsJobD(char *nick, job_dict_t *D);
void insertJobD(char *nick, job_dict_t *D,int jobID);
void outputJobD(job_dict_t *D);
int getJobID(char *nick,job_dict_t *JobD);

#endif
