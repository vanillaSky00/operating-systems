#include <stdlib.h>
#include "include/shell.h"
#include "include/command.h"

int history_count;
char *history[MAX_RECORD_NUM];

int main(int argc, char *argv[]) {

	// Optional: Load Configuration files

	// Run shell command loop
	history_count = 0;
	for (int i = 0; i < MAX_RECORD_NUM; ++i)
    	history[i] = (char *)malloc(BUF_SIZE * sizeof(char));

	shell_loop();

	for (int i = 0; i < MAX_RECORD_NUM; ++i)
    	free(history[i]);

	// Shutdown and cleanup
	return 0;
}
