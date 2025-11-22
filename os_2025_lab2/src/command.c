#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "../include/command.h"


void free_cmd(struct cmd *cmd) {

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
