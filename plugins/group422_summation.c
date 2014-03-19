

/*
 * An example plug-in, which implements the 'cd' command.
 */
#include <stdbool.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>
#include "../esh.h"
#include <signal.h>
#include "../esh-sys-utils.h"

static bool 
init_plugin(struct esh_shell *shell)
{
    printf("Plugin 'summation' initialized...\n");
    return true;
}

/* Implement summation built-in.
 * Returns true if handled, false otherwise. */
static bool
summation_builtin(struct esh_command *cmd)
{
	if (strcmp(cmd->argv[0], "sum")) {
	   return false;   
	}
	int result = 0;
	int no1 = atoi(cmd->argv[1]);
	if(cmd->argv[1] == NULL)
		return false;
	
	if(cmd->argv[2] != NULL	)
	{
		int no2 = atoi(cmd->argv[2]);
		result = no1 + no2;
		printf("The Sum of %d and %d is %d.", no1, no2, result);
	}
	else
	{
		result = ((no1)*(no1 + 1))/2;
		printf("The Sum from 1 to %d is %d.", no1, result);
	}
	
	
	printf("\n");


	return true;
}

struct esh_plugin esh_module = {
  .rank = 3,
  .init = init_plugin,
  .process_builtin = summation_builtin
};