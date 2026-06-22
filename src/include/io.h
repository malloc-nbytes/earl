#ifndef IO_H_INCLUDED
#define IO_H_INCLUDED

#include "array.h"

int   file_exists(const char *fp);
int   create_file(const char *fp, int force_overwrite);
int   is_dir(const char *path);
int   write_file(const char *fp, const char *content);
char *load_file(const char *path);

cstr_ar lsdir(const char *path);
cstr_ar walkdir(const char *path);

const char *gethome(void);
char       *get_realpath(const char *fp);
const char *get_basename(const char *name);

#endif // IO_H_INCLUDED
