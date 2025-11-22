#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "../include/command.h"
#include "../include/builtin.h"

// ======================= requirement 2.3 =======================
/**
 * @brief 
 * Redirect command's stdin and stdout to the specified file descriptor
 * If you want to implement ( < , > ), use "in_file" and "out_file" included the cmd_node structure
 * If you want to implement ( | ), use "in" and "out" included the cmd_node structure.
 *
 * @param p cmd_node structure
 * 
 */
void redirection(struct cmd_node *p) {
	
}
// ===============================================================

// ======================= requirement 2.2 =======================
/**
 * @brief 
 * Execute external command
 * The external command is mainly divided into the following two steps:
 * 1. Call "fork()" to create child process
 * 2. Call "execvp()" to execute the corresponding executable file
 * @param p cmd_node structure
 * @return int 
 * Return execution status
 */
int spawn_proc(struct cmd_node *p) {
  	return 1;
}
// ===============================================================


// ======================= requirement 2.4 =======================
/**
 * @brief 
 * Use "pipe()" to create a communication bridge between processes
 * Call "spawn_proc()" in order according to the number of cmd_node
 * @param cmd Command structure  
 * @return int
 * Return execution status 
 */
int fork_cmd_node(struct cmd *cmd) {
	return 1;
}


void shell_loop() {
	int status = 1;
	
	do {
		char* line = NULL;
		struct cmd* cmd = NULL;
		
		printf(">>> $ ");

		line = shell_read_line();
		if (line == NULL)
			continue;

		cmd = shell_split_line(line);
		
		status = shell_execute(cmd);

		shell_cleanup(line, cmd);
	} while(status);
}