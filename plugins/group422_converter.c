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
    printf("Plugin 'converter' initialized...\n");
    return true;
}

/* Implement chdir built-in.
 * Returns true if handled, false otherwise. */
static bool
converter_builtin(struct esh_command *cmd)
{
	if (strcmp(cmd->argv[0], "convertDegrees")) {
	   return false;   
	}

	//Check if the number is valid
	int i = 0;
	int count = 0;
	//printf("%s\n", cmd->argv[1]);
	for (i = 0; i < strlen(cmd->argv[1]); ++i)
	{
		if ((cmd->argv[1])[i] != '1' && (cmd->argv[1])[i] != '2' && 
			(cmd->argv[1])[i] != '3' && (cmd->argv[1])[i] != '4' && 
			(cmd->argv[1])[i] != '5' && (cmd->argv[1])[i] != '6' &&
			(cmd->argv[1])[i] != '7' && (cmd->argv[1])[i] != '8' &&
			(cmd->argv[1])[i] != '9' && (cmd->argv[1])[i] != '0' &&
			(cmd->argv[1])[i] != '.' && (cmd->argv[1])[i] != '-')
		{
			printf("%s\n", "Invalid input");
			return true;
		}

		if ((cmd->argv[1])[i] == '.')
		{
			++count;
			if (count > 1)
			{
				printf("%s\n", "Invalid input");
				return true;
			}
		}
	}

	double degree = atof(cmd->argv[1]);


	if (cmd->argv[2] == NULL)
	{
		printf("%s\n", "Invalid input");
		return true;
	}

	char *type = cmd->argv[2];
	double result = 0.0;

	if (type[0] == 'F')
	{
		result = (degree - 32) * 5 / 9;
		printf("%lf C\n", result);
	}
	else
	{
		result = (degree * 9 / 5) + 32;
		printf("%lf F\n", result);
	}

	return true;
}

struct esh_plugin esh_module = {
  .rank = 2,
  .init = init_plugin,
  .process_builtin = converter_builtin
};
