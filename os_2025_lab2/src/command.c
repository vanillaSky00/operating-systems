#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "../include/command.h"


void free_cmd(struct cmd *cmd) {
    struct cmd_node *curr = cmd->head;
    struct cmd_node *next;

    while (curr != NULL) {
        next = curr->next;
		
        // Only free the container
        if (curr->args != NULL) free(curr->args);
        free(curr);
        
        curr = next;
    }
    
    if (cmd != NULL) free(cmd); 
}

struct cmd *create_cmd() {
	struct cmd *new_cmd = (struct cmd *)malloc(sizeof(struct cmd));
	if (!new_cmd) {
		perror("malloc in create_cmd");
		return NULL;
	} 

	new_cmd->head = NULL;
	new_cmd->pipe_num = 0;
	return new_cmd;
}

struct cmd_node *create_cmd_node(){
	struct cmd_node *node = (struct cmd_node *)calloc(1, sizeof(struct cmd_node));
	if (!node) {
		perror("malloc in crete_cmd_node");
		return NULL;
	}

	node->args = (char **) calloc(ARGS_CAP, sizeof(char *));

	node->out = 1;
	return node;
}
/**
 * @brief Information used to test the cmd structure
 * 
 * @param cmd Command structure
 */
void test_cmd_struct(struct cmd *cmd) {
	struct cmd_node *temp = cmd->head;
	int pipe_count = 0;
	printf("============ COMMAND INFO ============\n");
	while (temp != NULL) {
		printf("cmd_node %d: ", pipe_count);
		for (int i = 0; i < temp->length; ++i) {
			printf("%s ", temp->args[i]);
		}
		printf("\n");
		temp = temp->next;
		++pipe_count;
	}
	printf("============ COMMAND INFO END ============\n");
}


/**
 * @brief Information used to test the cmd_node structure
 * 
 * @param temp cmd_node structure
 */
void test_pipe_struct(struct cmd_node *temp) {
	printf("============ CMD_NODE INFO ============\n");
	
	for (int i = 0; i < temp->length; ++i) {
		printf("temp->args[%d] :%s \n",i, temp->args[i]);
	}
	printf(" in-file: %s\n", temp->in_file ? temp->in_file : "none");
	printf("out-file: %s\n", temp->out_file ? temp->out_file : "none");
	printf(" in: %d\n", temp->in );
	printf("out: %d\n", temp->out);
	printf("============ CMD_NODE END ============\n");
}
