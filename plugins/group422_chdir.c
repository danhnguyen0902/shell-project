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
    printf("Plugin 'cd' initialized...\n");
    return true;
}

static char*
home_directory()
{
	struct passwd *pw = getpwuid(getuid());
	if (pw == NULL) {
	    esh_sys_error("Could not obtain home directory.\n"
	                  "getpwuid(%d) failed: ", getuid());
	    return NULL;
	} else {
	    return pw->pw_dir;	// it's /home/ugrads/majors/danh0902
	}
}

/* Implement chdir built-in.
 * Returns true if handled, false otherwise. */
static bool
chdir_builtin(struct esh_command *cmd)
{
	if (strcmp(cmd->argv[0], "cd")) {
	   //printf("%s\n", "test cd");
	   return false;   
	}

	char *dir = cmd->argv[1];
	//printf("%s\n", dir);

	// if no argument is given, default to home directory
	if (dir == NULL || (dir[0] == '~' && strlen(dir) == 1)) {
		dir = home_directory();

		if (chdir(dir) != 0)
	   		esh_sys_error("chdir: ");

	   	return true;
	}

	if (dir[0] == '~' && strlen(dir) != 1)
	{
		char tmp[1000];

		//printf("%s\n", dir);
		if (dir[1] == '/')
		{
			strcpy(tmp, home_directory());
			strcat(tmp, strndup(dir + 1, strlen(dir) - 1));
			dir = tmp;
		}
		else
		{
			strcpy(tmp, "/home/courses/");
			strcat(tmp, strndup(dir + 1, strlen(dir) - 1));
			dir = tmp;
		}

		if (chdir(dir) != 0)
	   		esh_sys_error("chdir: ");	

	   	return true;
	}

	// cd ..
	/*
	if (dir[0] == '.' && dir[1] == '.' && strlen(dir) == 2)
	{

	}
	*/

	return true;
}

struct esh_plugin esh_module = {
  .rank = 1,
  .init = init_plugin,
  .process_builtin = chdir_builtin
};
