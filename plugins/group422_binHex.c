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
    printf("Plugin 'binHex' initialized...\n");
    return true;
}

/* Implement binary built-in.
 * Returns true if handled, false otherwise. */
static bool
binHex_builtin(struct esh_command *cmd)
{
	if (strcmp(cmd->argv[0], "binHex")) {
	   return false;   
	}

	//Check if the number is valid
	int i = 0;

	if (cmd->argv[1] == NULL)
	{
		printf("%s\n", "Invalid input");
		return true;	
	}

	for (i = 0; i < strlen(cmd->argv[1]); ++i)
	{
		if ((cmd->argv[1])[i] != '1' && (cmd->argv[1])[i] != '2' && 
			(cmd->argv[1])[i] != '3' && (cmd->argv[1])[i] != '4' && 
			(cmd->argv[1])[i] != '5' && (cmd->argv[1])[i] != '6' &&
			(cmd->argv[1])[i] != '7' && (cmd->argv[1])[i] != '8' &&
			(cmd->argv[1])[i] != '9' && (cmd->argv[1])[i] != '0')
		{
			printf("%s\n", "Invalid input");
			return true;
		}
	}

	int number = atoi(cmd->argv[1]);

	int tmp = number;
	int arr[40];
	int n = 0;

	// Conver to binary number
	while (tmp > 0)
	{
		arr[n] = tmp % 2;
		++n;
		tmp = tmp / 2;
	}

	printf("%s\n", "The number in binary: ");
	i = 0;
	for (i = n - 1; i >= 0; --i)
	{
		printf("%d", arr[i]);
	}
	printf("\n");

	// Convert to hexadecimal number
	tmp = number;
	n = 0;
	while (tmp > 0)
	{
		arr[n] = tmp % 16;
		++n;
		tmp = tmp / 16;
	}

	printf("%s\n", "The number in hexadecimal: ");
	i = 0;
	for (i = n - 1; i >= 0; --i)
	{
		if (arr[i] > 9)
		{
			switch (arr[i])
			{
				case 10: 
					printf("%s", "A");
					break;
				case 11:
					printf("%s", "B");
					break;
				case 12:
					printf("%s", "C");
					break;
				case 13:
					printf("%s", "D");
					break;
				case 14:
					printf("%s", "E");
					break;
				case 15:
					printf("%s", "F");
					break;
			}
		}
		else
			printf("%d", arr[i]);
	}
	printf("\n");	

	return true;
}

struct esh_plugin esh_module = {
  .rank = 5,
  .init = init_plugin,
  .process_builtin = binHex_builtin
};
