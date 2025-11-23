#ifndef SHELL_H
#define SHELL_H

#include "command.h"

// Main Lifecycle Management
void shell_loop();
char *shell_read_line();
struct cmd *shell_parse_line(char *);
int shell_execute(struct cmd *);
void shell_cleanup();

// Execution Engine
int search_builtin(struct cmd_node *cmd);
int execute_builtin(int index, struct cmd_node *cmd);
int execute_builtin_safe(int index, struct cmd_node *cmd);
int execute_external(struct cmd_node *);
int execute_pipeline(struct cmd *cmd);

// Utilities
int setup_redirection(struct cmd_node *cmd);


#endif
