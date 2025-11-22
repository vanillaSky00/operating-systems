#ifndef SHELL_H
#define SHELL_H

#include "command.h"

// Main Lifecycle Management
void shell_loop();
char *shell_read_line();
struct cmd *shell_split_line(char *);
int shell_execute(struct cmd *);
void shell_cleanup();

// Ececution Engine
int execute_builtin(struct cmd_node *);
int execute_pipeline(struct cmd *cmd);

// Utilities
void setup_redirection(struct cmd_node *cmd);


#endif
