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
 * @brief Being an excuting manager, dispatching task to builtin, external, or pipeline
 * 
 * @return int
 * Return the status code
 */
int shell_execute(struct cmd *cmd) {
	if (cmd == NULL) return 1; // keep running

	struct cmd_node *temp = cmd->head;

	if(temp->next == NULL) {
		int idx = search_builtin(temp);
		if (idx != -1) {
			return execute_builtin_safe(idx, temp);
		}
		else {
			return execute_external(cmd->head);
		}
	}
	// multiple commands ( | )
	else {
		return execute_pipeline(cmd);
	}
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
	int args_cap = 10;
    struct cmd *new_cmd = (struct cmd *)malloc(sizeof(struct cmd));

	new_cmd->pipe_num 		= 0;
    new_cmd->head 			= (struct cmd_node *)malloc(sizeof(struct cmd_node));
    new_cmd->head->args 	= (char **)malloc(args_cap * sizeof(char *));
	for (int i = 0; i < args_cap; ++i)
		new_cmd->head->args[i] = NULL;
	new_cmd->head->in_file  = NULL;
    new_cmd->head->out_file = NULL;
    new_cmd->head->next 	= NULL;
	new_cmd->head->length   = 0;
	new_cmd->head->in 		= 0;
	new_cmd->head->out		= 1;

	struct cmd_node *temp = new_cmd->head;
    char *token = strtok(line, " ");

    while (token != NULL) {
        if (token[0] == '|') {
            struct cmd_node *new_pipe = (struct cmd_node *)malloc(sizeof(struct cmd_node));
			new_pipe->args = (char **)malloc(args_cap * sizeof(char *));
			for (int i = 0; i < args_cap; ++i)
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

		if (dup2(fd, STDIN_FILENO) == -1) {
			perror("dup2 input");
			close(fd);
			return -1;
		}
		close(fd);
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
		close(fd);
	}

	return 1;
}


/**
 * @brief 
 * Determine whether cmd is a built-in command
 * @param cmd Command structure
 * @return int 
 * If command is built-in command return function number
 * If command is external command return -1 
 */
int search_builtin(struct cmd_node *cmd) {
	if (!cmd->args[0]) return -1;
	for (int i = 0; i < num_builtins(); i++){
		if (strcmp(cmd->args[0], builtin_str[i]) == 0){
			return i;
		}
	}
	return -1;
}


/**
 * @brief Execute built-in command
 * 
 * @param index Choose which built-in command to execute
 * @param cmd Command structure
 * @return int 
 * Return execution result status
 */
int execute_builtin(int index, struct cmd_node *cmd) {
	return (*builtin_func[index])(cmd->args);
}

int execute_builtin_safe(int index, struct cmd_node *cmd) {
	int saved_in = dup(STDIN_FILENO);
	int saved_out = dup(STDOUT_FILENO);
	if(saved_in == -1 || saved_out == -1) {
		perror("dup");
		return -1;
	}

	if (setup_redirection(cmd) == -1) {
		perror("setup_redirection");
		// Must not execute the command
		dup2(saved_in, STDIN_FILENO);
		dup2(saved_out, STDOUT_FILENO);
		close(saved_in);
		close(saved_out);
		return -1;
	}

	int status = (*builtin_func[index])(cmd->args);

	// always restore, regardless of whether the command succeeded or failed
	dup2(saved_in, STDIN_FILENO);
	dup2(saved_out, STDOUT_FILENO);
	close(saved_in);
	close(saved_out);

	return status;
}


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
		return -1;
	}

	pid_t pid;
	int status;

	switch (pid = fork()) {
		case -1:
			perror("fork");
			return -1;

		case 0:
			if (setup_redirection(p) == -1) {
				_exit(1);
			}

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

  	return 1;
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
	struct cmd_node *curr = cmd->head;

	int in_fd = STDIN_FILENO; // read end of previous pipe
	int fd[2];

	while (curr != NULL) {

		if (curr->next != NULL) {
			if (pipe(fd) == -1) {
				perror("pipe");
				return -1;
			}
		}

		pid_t pid = fork();
		if (pid < 0) {
			perror("fork");
			return -1;
		}
		else if (pid == 0) { 
			
			// connect previous pipe to STDIN (if not the first command)
			if (in_fd != STDIN_FILENO) {
				dup2(in_fd, STDIN_FILENO);
				close(in_fd);
			}

			// connect current pipe to STDOUT (if not the last command)
			if (curr->next != NULL) {
				dup2(fd[1], STDOUT_FILENO);
				close(fd[1]);
				close(fd[0]);
			}

			int idx = search_builtin(curr);
			if (idx >= 0) {
				exit(execute_builtin(idx, curr));
			}
			else {
				// FIX: Don't call execute_external (it forks). 
				// We are already in a child. Just exec.
					execvp(curr->args[0], curr->args);
					perror("execvp");
					exit(127);
				}
			exit(-1); // Should not reach here
		}
		else { 
			// close previous pipe, we are done reading from it
			if (in_fd != STDIN_FILENO) close(in_fd);

			// set up for next loop
			if (curr->next != NULL) {

				// the read end of the current process becomes the "Input" for the next
				in_fd = fd[0];

				// parent dose not write
				close(fd[1]);
			}
			
		}

		curr = curr->next;
	}
	
	while (wait(NULL) > 0); // wait all children

	return 1;
}


void shell_loop() {
	int status = 1;
	
	do {		
		printf(">>> $ ");

		char* line = shell_read_line();
		if (line == NULL)
			continue;

		struct cmd* cmd = shell_split_line(line);
		
		status = shell_execute(cmd);

		free(line);
		free_cmd(cmd);
	} while(status);
}