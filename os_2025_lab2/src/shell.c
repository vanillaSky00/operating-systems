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
#include "../include/shell.h"

/**
 * @brief Only execute a single command(built-in or external) 
 * 
 * @return int
 * Return the status code
 */
int shell_execute(struct cmd *cmd) {

	struct cmd_node *temp = cmd->head;
	int status = 0;

	if(temp->next == NULL) {
		status = search_builtin(temp);
		if (status != -1) {
			int in = dup(STDIN_FILENO);
			int out = dup(STDOUT_FILENO);
			if( in == -1 | out == -1) perror("dup");

			if (setup_redirection(temp) == -1) perror("setup_redirection");
			status = execute_builtin(status, temp);

			// recover shell stdin and stdout
			if (temp->in_file)  dup2(in, 0);
			if (temp->out_file) dup2(out, 1);
			
			close(in);
			close(out);
		}
		else {
			//external command
			status = execute_external(cmd->head);
		}
	}
	// There are multiple commands ( | )
	else {
		status = execute_pipeline(cmd);
	}

	return status;
}


void shell_cleanup() {

}


/**
 * @brief Read the user's input string
 * 
 * @return char* 
 * Return string
 */
char *shell_read_line() {
	// TODO: reallocate if execeed
    char *buffer = (char *)malloc(BUF_SIZE * sizeof(char));
    if (buffer == NULL) {
        perror("Unable to allocate buffer");
        exit(1);
    }

	if (fgets(buffer, BUF_SIZE, stdin) != NULL) {
		if (buffer[0] == '\n' || buffer[0] == ' ' || buffer[0] == '\t') {
			free(buffer);
			buffer = NULL;
		} 
		else {
			buffer[strcspn(buffer, "\n")] = 0;
			strncpy(history[history_count % MAX_RECORD_NUM], buffer, BUF_SIZE);
			++history_count;
		}
	}

	return buffer;
}


/**
 * @brief Parse the user's command
 * 
 * @param line User input command
 * @return struct cmd* 
 * Return the parsed cmd structure
 */
struct cmd *shell_split_line(char *line) {
	int args_length = 10;
    struct cmd *new_cmd = (struct cmd *)malloc(sizeof(struct cmd));
    new_cmd->head = (struct cmd_node *)malloc(sizeof(struct cmd_node));
    new_cmd->head->args = (char **)malloc(args_length * sizeof(char *));

	for (int i = 0; i < args_length; ++i)
		new_cmd->head->args[i] = NULL;
    new_cmd->head->length = 0;
    new_cmd->head->next = NULL;
	new_cmd->pipe_num = 0;

	struct cmd_node *temp = new_cmd->head;
	temp->in_file 	= NULL;
	temp->out_file 	= NULL;
	temp->in       	= 0;
	temp->out 		= 1;

    char *token = strtok(line, " ");
    while (token != NULL) {
        if (token[0] == '|') {
            struct cmd_node *new_pipe = (struct cmd_node *)malloc(sizeof(struct cmd_node));
			new_pipe->args = (char **)malloc(args_length * sizeof(char *));
			for (int i = 0; i < args_length; ++i)
				new_pipe->args[i] = NULL;
			new_pipe->length = 0;
			new_pipe->next = NULL;
			temp->next = new_pipe;
			temp = new_pipe;
        } else if (token[0] == '<') {
			token = strtok(NULL, " ");
            temp->in_file = token;
        } else if (token[0] == '>') {
			token = strtok(NULL, " ");
            temp->out_file = token;
        } else {
			temp->args[temp->length] = token;
			temp->length++;
        }
        token = strtok(NULL, " ");
		new_cmd->pipe_num++;

    }

    return new_cmd;
}


/**
 * @brief 
 * Redirect command's stdin and stdout to the specified file descriptor
 * If you want to implement ( < , > ), use "in_file" and "out_file" included the cmd_node structure
 * If you want to implement ( | ), use "in" and "out" included the cmd_node structure.
 *
 * @param p cmd_node structure
 * 
 */
int setup_redirection(struct cmd_node *p) {
	
	if (p->in_file != NULL) {
		int fd = open(p->in_file, O_RDONLY);
		if (fd == -1) { 
			perror("open input file");
			return -1;
		}

		if (dup2(fd, STDOUT_FILENO) == -1) {
			perror("dup2 input");
			close(fd);
			return -1;
		}
	}

	if (p->out_file != NULL) {
		int fd = open(p->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd == -1) {
			perror("open output file");
			return -1;
		}

		if (dup2(fd, STDOUT_FILENO) == -1) {
			perror("dup2 output");
			close(fd);
			return -1;
		}
	}

	return 0;
}
// ===============================================================


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
int execute_external(struct cmd_node *p) {
	if (p == NULL) {
		fprintf(stderr, "command is null\n");
		return 0;
	}

	pid_t pid;
	int status;

	switch (pid = fork()) {
		case -1:
			perror("fork");
			return 0;

		case 0:
			execvp(p->args[0], p->args);
			perror("execvp");
			_exit(127);

		default :
			if (waitpid(pid, &status, 0) == -1) {
				perror("waitpid");
			}
			else if (WIFEXITED(status)) {
				// child finish normally, check the exit code
				int exit_code = WEXITSTATUS(status);

				if (exit_code != 0) {
					fprintf(stderr, "Command failed with exit code: %d", exit_code);
				}
			}
			else if (WIFSIGNALED(status)){
				// the child crash or was killed
				int sig = WTERMSIG(status);
				fprintf(stderr, "Child process terminated by signal: %d\n", sig);
			}
	}

  	return 0;
}


/**
 * @brief 
 * Use "pipe()" to create a communication bridge between processes
 * Call "execute_builtin()" in order according to the number of cmd_node
 * @param cmd Command structure  
 * @return int
 * Return execution status 
 */
int execute_pipeline(struct cmd *cmd) {
	
	struct cmd_node *temp = cmd->head;
	int idx = 0;

	while (temp != NULL) {
		int idx = search_builtin(temp);
		
		if (status >= 0) {
			status = setup_redirection(temp);
			execute_builtin(idx, temp);
		}
		else if (status > 0){
			status = setup_redirection(temp);
			execute_external(temp);
		}

		temp = temp->next;
	}
	
	return 0;
}


void shell_loop() {
	int status = 0;
	
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
	} while(!status);
}