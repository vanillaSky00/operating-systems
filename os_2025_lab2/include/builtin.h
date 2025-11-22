#ifndef BUILTIN_H
#define BUILTIN_H
#include "../include/command.h"

int pwd(char **args);
int help(char **args);
int cd(char **args);
int echo(char **args);
int exit_shell(char **args);
int record(char **args);

extern const char *builtin_str[];

extern int (* const builtin_func[]) (char **); // int is status code of a funct

extern int num_builtins();

#endif
