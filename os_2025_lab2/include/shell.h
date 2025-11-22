#ifndef SHELL_H
#define SHELL_H

#include "command.h"

int launch_cmd(struct cmd_node *);
int execute_pipeline(struct cmd *cmd);
void setup_redirection(struct cmd_node *cmd);
void shell_loop();

#endif
