#ifndef COMMAND_H
#define COMMAND_H

#define MAX_RECORD_NUM 16
#define BUF_SIZE 1024
#define ARGS_CAP 10
#include <stdbool.h>

struct cmd {
	struct cmd_node *head;
	int pipe_num;
};

struct cmd_node {
	char **args;
	int length;
	char *in_file; 
	char *out_file;
	int in;
	int out;
	struct cmd_node *next;
};


extern char *history[MAX_RECORD_NUM];
extern int history_count;

struct cmd *create_cmd();
struct cmd_node *create_cmd_node();
void free_cmd(struct cmd *cmd);
void test_cmd_struct(struct cmd *);
void test_pipe_struct(struct cmd_node *pipe);

#endif
