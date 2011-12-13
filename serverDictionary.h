#ifndef _SERVER_DICT_H
#define _SERVER_DICT_H
#include "prelude.h"

//TODO: ensure this header matches the definitions in serverDictionary.h -TL
void insertServerD(dict_t *D, serverId *serverId);
void insertJobD(dict_t *D, jobDescriptor *job);

#endif
