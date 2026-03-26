#ifndef FILE_H
#define FILE_H

#include "editor.h"

int file_open(Editor *ed, const char *filename);
int file_save(Editor *ed, const char *filename);

#endif