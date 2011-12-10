//New implementation of a dictionary for user info
#ifndef _FILE_DICT_H
#define _FILE_DICT_H
#include "prelude.h"

typedef struct _file_dict_t {
  int size;
  int elements; //I added this because it seemed to be easier to just keep track of how many
                //elements were in the dict_t rather than iterating through to check when we
                //are inserting elements.
  struct _file_node_t *head;
} file_dict_t;

typedef struct _file_node_t {
  char *nickname;
  int fileID;
  struct _file_node_t *next;
} file_node_t;


dict_t *newFileD(int initial_size);
boolean containsFileD(char *nick, file_dict_t *D);
void insertFileD(char *nick, file_dict_t *D,int fileID);
void outputFileD(file_dict_t *D);
int getFileID(char *nick,file_dict_t *FileD);

#endif
