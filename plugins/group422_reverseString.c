

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
    printf("Plugin 'reverseString' initialized...\n");
    return true;
}

/* Implement chdir built-in.
 * Returns true if handled, false otherwise. */
static bool
reverseString_builtin(struct esh_command *cmd)
{
	if (strcmp(cmd->argv[0], "reverse")) {
	   return false;   
	}
	
	char* strToReverse = cmd->argv[1];
	char* result = malloc(1000*sizeof(char));
	if(strToReverse != NULL	)
	{
		int length = strlen(strToReverse);
		int last_pos = length-1;
		//char* result = malloc((length + 1)*sizeof(char));
		int i = 0;
		while(i < length / 2)
		{	
			char tmp = strToReverse[i];
   			strToReverse[i] = strToReverse[last_pos - i];
			strToReverse[last_pos - i] = tmp;
			
			i++;
		}

	}
	
	printf(strToReverse);
	printf("\n");

	free(result);
	return true;
}

struct esh_plugin esh_module = {
  .rank = 4,
  .init = init_plugin,
  .process_builtin = reverseString_builtin
};