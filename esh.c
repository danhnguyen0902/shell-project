/*
 * esh - the 'pluggable' shell.
 *
 * Developed by Godmar Back for CS 3214 Fall 2009
 * Virginia Tech.
 */
#include <stdio.h>
#include <readline/readline.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>
#include <fcntl.h>
#include "esh-sys-utils.h"
#include "esh.h"

/*
 * Functions you may use for your shell: give_terminal_to, sigchld_handler and wait_for_job
 * See FAQ Question 4.
 *
 * CS 3214 Fall 2011, Godmar Back.
 */

//Global variables:
struct list /* <esh_pipeline> */ job_list;  // List of the current jobs
int jid;                                    // to keep track of the job ids
struct esh_command_line * cline;            // The command line after parsing
struct termios *initial_terminal;           // The initial state of the terminal
bool hasPipe;                               // true if the pipeline contains pipes
int firstPipe[2], secondPipe[2];

static void
usage(char *progname)
{
    printf("Usage: %s -h\n"
           " -h            print this help\n"
           " -p  plugindir directory from which to load plug-ins\n",
           progname);

    exit(EXIT_SUCCESS);
}

/* Build a prompt by assembling fragments from loaded plugins that
 * implement 'make_prompt.'
 *
 * This function demonstrates how to iterate over all loaded plugins.
 */
static char *
build_prompt_from_plugins(void)
{
    char *prompt = NULL;
    struct list_elem * e = list_begin(&esh_plugin_list);

    for (; e != list_end(&esh_plugin_list); e = list_next(e)) {
        struct esh_plugin *plugin = list_entry(e, struct esh_plugin, elem);

        if (plugin->make_prompt == NULL) {
            continue;
        }

        /* append prompt fragment created by plug-in */
        char * p = plugin->make_prompt();
        if (prompt == NULL) {
            prompt = p;
        } else {
            prompt = realloc(prompt, strlen(prompt) + strlen(p) + 1);
            strcat(prompt, p);
            free(p);
        }
    }

    /* default prompt */
    if (prompt == NULL) {
        prompt = strdup("esh> ");
    }

    return prompt;
}

/**
 * Assign ownership of ther terminal to process group
 * pgrp, restoring its terminal state if provided.
 *
 * Before printing a new prompt, the shell should
 * invoke this function with its own process group
 * id (obtained on startup via getpgrp()) and a
 * sane terminal state (obtained on startup via
 * esh_sys_tty_init()).
 * 
 * In other words, this function will restore the initial 
 * state of the terminal before the actual command is executed
 */
static void
give_terminal_to(pid_t pgrp, struct termios *pg_tty_state)
{
    esh_signal_block(SIGTTOU);
    int rc = tcsetpgrp(esh_sys_tty_getfd(), pgrp);
    if (rc == -1)
        esh_sys_fatal_error("tcsetpgrp: ");

    if (pg_tty_state)
        esh_sys_tty_restore(pg_tty_state);
    esh_signal_unblock(SIGTTOU);
}

/*
 * Return the list of the current jobs
 */
static struct list * get_jobs(void) {
    return &job_list;
}

/*
 * Return a job/pipeline corresponding to the given process group id
 */
static struct esh_pipeline * get_job_from_pgrp(pid_t pgrp) {
    struct list_elem *e;

    for (e = list_begin(&job_list); e != list_end(&job_list); e = list_next(e)) {
        struct esh_pipeline *pipeline = list_entry(e, struct esh_pipeline, elem);
    
        if (pipeline->pgrp == pgrp) {
            return pipeline;
        }
    }

    return NULL;
}

/*
 * Return a command/process corresponding to the given pid
 */
static struct esh_command * get_cmd_from_pid(pid_t pid) {
    struct list_elem *p, *c; 
    struct esh_pipeline *pipeline;  // a single pipeline in a list of pipelines
    struct esh_command *command;    // a single command in a list of commands (of a pipeline)   

    for (p = list_begin(&cline->pipes); p != list_end(&cline->pipes); p = list_next(p)) {
        pipeline = list_entry(p, struct esh_pipeline, elem);
        
        for (c = list_begin(&pipeline->commands); c != list_end(&pipeline->commands); c = list_next(c)) {
            command = list_entry(c, struct esh_command, elem);

            if (command->pid == pid) {
                return command;
            }
        }
    }

    return NULL;
}

/*
 * Print all the commands of a job/pipeline
 */
static void print_commands(struct esh_pipeline *pipeline)
{
    struct list_elem *e;
    struct esh_command *command;

    printf("(");
    int count = 0;
    for (e = list_begin(&pipeline->commands); e != list_end(&pipeline->commands); e = list_next(e)) {
        command = list_entry(e, struct esh_command, elem);

        ++count;
        if (count > 1) {
            printf("| ");
        }

        char **c = command->argv;
        while (*c) {
            printf("%s ", *c);
            ++c;
        }
        
        if (pipeline->bg_job) {
            printf("& ");
        }
    }

    printf(")\n");
}

/*
 * Change the status of a given job id
 */
static void child_status_change(pid_t pid, int status)
{
    struct list_elem *e;

    if (pid < 0) {
        esh_sys_fatal_error("Error in waitpid\n");
    }

    if (pid > 0) {
        for (e = list_begin(&job_list); e != list_end(&job_list); e = list_next(e)) {
            struct esh_pipeline *pipeline = list_entry(e, struct esh_pipeline, elem);

            if (pipeline->pgrp != pid) {
                continue;
            }

            // If the child process was terminated by a signal.
            if (WIFSIGNALED(status)) {
                list_remove(e);

                // Ctrl-C, SIGINT
                if (WTERMSIG(status) == 2) {
                    printf("\n");
                }

                // SIGKILL
                if (WTERMSIG(status) == 9) {
                    list_remove(e);
                }
            }

            // If the child process was stopped by a signal
            if (WIFSTOPPED(status)) {
                // Ctrl-Z, SIGTSTP
                if (WSTOPSIG(status) == 20) {
                    pipeline->status = STOPPED;

                    printf("\n[%d]+    Stopped                      ", pipeline->jid);
                    print_commands(pipeline);                        
                }

                // SIGTTIN
                if (WSTOPSIG(status) == 21) {
                    pipeline->status = STOPPED;
                }

                // SIGTTOU
                if (WSTOPSIG(status) == 22) {
                    pipeline->status = STOPPED;
                }
            }

            // Other cases
            if (WIFEXITED(status) || WIFCONTINUED(status)) {
                list_remove(e);
            }
        }
    }

    if (list_empty(&job_list)) {
        jid = 0;
    }
}

/*
 * SIGCHLD handler.
 * Call waitpid() to learn about any child processes that
 * have exited or changed status (been stopped, needed the
 * terminal, etc.)
 * Just record the information by updating the job list
 * data structures.  Since the call may be spurious (e.g.
 * an already pending SIGCHLD is delivered even though
 * a foreground process was already reaped), ignore when
 * waitpid returns -1.
 * Use a loop with WNOHANG since only a single SIGCHLD 
 * signal may be delivered for multiple children that have 
 * exited.
 */
static void
sigchld_handler(int sig, siginfo_t *info, void *_ctxt)
{
    pid_t child;
    int status;

    assert(sig == SIGCHLD);

    while ((child = waitpid(-1, &status, WUNTRACED|WNOHANG)) > 0) {
        child_status_change(child, status);
    }
}

/*
 * Return a job/pipeline corresponding to the given job id
 */
static struct esh_pipeline * get_job_from_jid(int jid) {
    struct list_elem *e;

    for (e = list_begin(&job_list); e != list_end(&job_list); e = list_next(e)) {
        struct esh_pipeline *pipeline = list_entry(e, struct esh_pipeline, elem);

        if (pipeline->jid == jid) {
            return pipeline;
        }
    }

    return NULL;
}

/* Wait for all processes in this pipeline to complete, or for
 * the pipeline's process group to no longer be the foreground 
 * process group. 
 * You should call this function from a) where you wait for
 * jobs started without the &; and b) where you implement the
 * 'fg' command.
 * 
 * Implement child_status_change such that it records the 
 * information obtained from waitpid() for pid 'child.'
 * If a child has exited or terminated (but not stopped!)
 * it should be removed from the list of commands of its
 * pipeline data structure so that an empty list is obtained
 * if all processes that are part of a pipeline have 
 * terminated.  If you use a different approach to keep
 * track of commands, adjust the code accordingly.
 */
static void wait_for_job()
{
    pid_t pid;
    int status;

    if ((pid = waitpid(-1, &status, WUNTRACED)) > 0) {
        give_terminal_to(getpgrp(), initial_terminal);
        child_status_change(pid, status);
    }
}

/*
 * Determine which kind of command it is
 */
static int command_type(char *cmd)
{
    if (strcmp(cmd, "jobs") == 0) {
        return 1;
    }

    if (strcmp(cmd, "fg") == 0) {
        return 2;
    }

    if (strcmp(cmd, "bg") == 0) {
        return 3;
    }

    if (strcmp(cmd, "kill") == 0) {
        return 4;
    }

    if (strcmp(cmd, "stop") == 0) {
        return 5;
    }

    if (strcmp(cmd, "exit") == 0) {
        return 6;
    }

    return 7;   // any other useful commands, pipes; also where the job_list is queued up
}

/*
 * If the current command is a plugin, execute it and return true,
 * otherwise return false
 */
static bool plugin_execute(struct esh_command *command)
{
    bool isPlugin = false;
    struct list_elem *e;
    struct esh_plugin *plugin;  // a single plugin in a list of plugins

    for (e = list_begin(&esh_plugin_list); e != list_end(&esh_plugin_list); e = list_next(e)) {
         plugin = list_entry(e, struct esh_plugin, elem);
               
        // If the current command's name matches up with one of the plugins in the plugin list
        if (plugin->process_builtin(command)) { 
            isPlugin = true;
            continue;   // don't break, in case of there are many plugins with the same name
        }
    }

    return isPlugin;    
}

/*
 * Execute the jobs command
 */
static void jobs_execute()
{
    struct list_elem *e;
    
    for (e = list_begin(&job_list); e != list_end(&job_list); e = list_next(e)) {
        struct esh_pipeline *pipeline = list_entry(e, struct esh_pipeline, elem);
        
        if (pipeline->status == FOREGROUND || pipeline->status == BACKGROUND)
        {
            printf("[%d]    %s                   ", pipeline->jid, "Running");
            print_commands(pipeline);
        }

        if (pipeline->status == STOPPED)
        {
            printf("[%d]    %s                   ", pipeline->jid, "Stopped");
            print_commands(pipeline);
        }
    }
}

/*
 * Get job id from the parameter of a command 
 */
static int get_jid(char *parameter)
{
    struct list_elem *e;
    struct esh_pipeline *pipeline;

    if (parameter == NULL) {
        e = list_back(&job_list);
        pipeline = list_entry(e, struct esh_pipeline, elem);
    
        return pipeline->jid;
    }
    else {
        return atoi(parameter);
    }
}

/*
 * Execute the fg command
 */
static void fg_execute(char *parameter)
{
    struct esh_pipeline *pipeline;
    int jid = get_jid(parameter);

    if (list_empty(&job_list)) {
        printf("fg: %d: No such job\n", jid);
    }
    else {
        pipeline = get_job_from_jid(jid);
        pipeline->status = FOREGROUND;

        esh_signal_block(SIGCHLD);

        if (killpg(pipeline->pgrp, SIGCONT) < 0) {
            esh_sys_fatal_error("SIGCONT error\n");
        }

        give_terminal_to(pipeline->pgrp, initial_terminal);

        print_commands(pipeline);

        wait_for_job();
        
        esh_signal_unblock(SIGCHLD);
    }
}

/*
 * Execute the bg command
 */
static void bg_execute(char *parameter)
{
    struct esh_pipeline *pipeline;
    int jid = get_jid(parameter);

    if (list_empty(&job_list)) {
        printf("bg: %d: No such job\n", jid);
    }
    else {
        pipeline = get_job_from_jid(jid);
        pipeline->status = BACKGROUND;

        if (killpg(pipeline->pgrp, SIGCONT) < 0) {
            esh_sys_fatal_error("SIGCONT error\n");
        }

        print_commands(pipeline);
        printf("\n");
    }
}

/*
 * Execute the kill command
 */
static void kill_execute(char *parameter)
{
    struct esh_pipeline *pipeline;
    int jid = get_jid(parameter);

    if (list_empty(&job_list)) {
        printf("kill: %d: No such job\n", jid);
    }
    else {
        pipeline = get_job_from_jid(jid);
        
        if (killpg(pipeline->pgrp, SIGKILL) < 0) {   //or: SIGINT
            esh_sys_fatal_error("SIGKILL error\n");
        }
    }
}

/*
 * Execute the stop command
 */
static void stop_execute(char *parameter)
{
    struct esh_pipeline *pipeline;
    int jid = get_jid(parameter);

    if (list_empty(&job_list)) {
        printf("stop: %d: No such process\n", jid);
    }
    else {
        pipeline = get_job_from_jid(jid);

        if (killpg(pipeline->pgrp, SIGSTOP) < 0) {
            esh_sys_fatal_error("SIGSTOP error\n");
        }
    }
}

/*
 * Some pre-processings for case 7, returns true if the pipeline has pipes,
 * otherwis returns false
 */
static bool pipeline_pre_process(struct esh_pipeline *pipeline)
{
    if (pipeline->bg_job) {
        pipeline->status = BACKGROUND;
    } 
    else {
        pipeline->status = FOREGROUND;
    }
    
    if (list_empty(&job_list)) {
        jid = 0;
    }
    ++jid;

    pipeline->jid = jid;
    pipeline->pgrp = -1;

    if (list_size(&pipeline->commands) >= 2) {
        return true;
    }
    return false;
}

/*
 * Advanced functionality: I/O redirection
 */
static void io_redirection(struct esh_command *command)
{
    if (command->iored_input != NULL) {
        int inputFile;

        if ((inputFile = open(command->iored_input, O_RDONLY)) == - 1) {
            esh_sys_fatal_error("Can't open file to read\n");    
        }

        if (dup2(inputFile, 0) < 0) {
            esh_sys_fatal_error("dup2 error\n");
        }

        close(inputFile);
    }

    if (command->iored_output != NULL) {
        int outputFile;
        
        if (command->append_to_output) {
            if ((outputFile = open(command->iored_output, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR)) == -1) {
                esh_sys_fatal_error("Can't open file to append\n");    
            }   
        }
        else {
            if ((outputFile = open(command->iored_output, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR)) == -1) {
                esh_sys_fatal_error("Can't open file to write\n");    
            }
        }

        if (dup2(outputFile, 1) < 0) {
            esh_sys_fatal_error("dup2 error\n");
        }

        close(outputFile);
    }
}

/*
 * Set the process group id of the child with the given pid to the
 * process group id of the pipeline/job
 */
static void setpgid_child(pid_t pid, struct esh_pipeline *pipeline)
{
    if (pipeline->pgrp == -1) {
        pipeline->pgrp = pid;
    }

    if (setpgid(pid, pipeline->pgrp) < 0) { // Set the pgid of the child to pipeline->pgrp
        esh_sys_fatal_error("Error in setting up the Process Group ID\n");
    }
}

/*
 * Process pipeline in case 7
 */
static void pipeline_process(struct esh_pipeline *pipeline, struct esh_command *command, struct list_elem *c)
{
    pid_t pid;
    
    esh_signal_block(SIGCHLD);

    /*----------------------------Pipes----------------------------*/
    if (hasPipe && list_next(c) != list_end(&pipeline->commands)) {
        pipe(firstPipe);
    }
    /*-------------------------------------------------------------*/

    if ((pid = fork()) < 0) {
        esh_sys_fatal_error("fork error\n");
    }
    else {
        // Child
        if (pid == 0) {     
            pid = getpid();
            command->pid = pid;

            // Set the pgid of the child to the pipeline's pgid
            setpgid_child(pid, pipeline);

            if (pipeline->status == FOREGROUND) {
                give_terminal_to(pipeline->pgrp, initial_terminal);
            }

            /*------------------------------I/O----------------------------*/
            io_redirection(command);
           
            /*----------------------------Pipes----------------------------*/
            if (hasPipe) {
                // at the beginning
                if (c == list_begin(&pipeline->commands)) {
                    //close(1);
                    if (dup2(firstPipe[1], 1) < 0) {
                        esh_sys_fatal_error("dup2 error\n");
                    }

                    close(firstPipe[0]);
                    close(firstPipe[1]);
                }
                else {
                    // at the end
                    if (list_next(c) == list_end(&pipeline->commands)) {
                        //close(0);
                        if (dup2(secondPipe[0], 0) < 0) {
                            esh_sys_fatal_error("dup2 error\n");
                        }

                        close(secondPipe[0]);
                        close(secondPipe[1]);       
                    }
                    // in the middle
                    else {
                        //close(0);
                        if (dup2(secondPipe[0], 0) < 0) {
                            esh_sys_fatal_error("dup2 error\n");       
                        }

                        close(secondPipe[0]);
                        close(secondPipe[1]);

                        //close(1);
                        if (dup2(firstPipe[1], 1) < 0) {
                            esh_sys_fatal_error("dup2 error\n");       
                        }

                        close(firstPipe[0]);
                        close(firstPipe[1]);
                    }
                }
            }   
            /*-------------------------------------------------------------*/

            if (execvp(command->argv[0], &command->argv[0]) < 0) {
                esh_sys_fatal_error("execvp error\n");
            }
        }
        // Parent
        else {     
            // Set the pgid of the child to the pipeline's pgid
            setpgid_child(pid, pipeline);

            /*----------------------------Pipes----------------------------*/
            if (hasPipe) {
                if (c != list_begin(&pipeline->commands)) {
                    close(secondPipe[0]);
                    close(secondPipe[1]);
                }

                if (list_next(c) == list_end(&pipeline->commands)) {
                    close(secondPipe[0]);
                    close(secondPipe[1]);    
                    close(firstPipe[0]);
                    close(firstPipe[1]);
                }
                else {
                    secondPipe[0] = firstPipe[0];
                    secondPipe[1] = firstPipe[1];
                }
            }
            /*-------------------------------------------------------------*/
        }
    }
}

/*
 * Some post-processings for case 7
 */
static void pipeline_post_process(struct esh_pipeline *pipeline, struct list_elem *p)
{
    if (pipeline->status == BACKGROUND) {
        printf("[%d] %d\n", pipeline->jid, pipeline->pgrp);
    }

    list_push_back(&job_list, p);   // add the current job to the list of jobs

    if (pipeline->status == FOREGROUND) {
        wait_for_job();
    }

    esh_signal_unblock(SIGCHLD);       
}

/*
 * Initialize everything beforehand
 */
static void shell_initialize()
{
    // Set a signal handler for when a child changes its status, we only need to set it once
    esh_signal_sethandler(SIGCHLD, sigchld_handler);

    // Save the initial state of the terminal, tty: terminal
    initial_terminal = esh_sys_tty_init();

    // Create a new group with the shell as the leader
    setpgid(0, 0);

    // Give the terminal to the created group
    give_terminal_to(getpgrp(), initial_terminal);

    // Create an empty list of plugins
    list_init(&esh_plugin_list);    // esh_plugin_list is a global variable declared in esh.h

    // Create an empty list of jobs
    list_init(&job_list);    

    // There is no job yet
    jid = 0; 
}

/* The shell object plugins use.
 * Some methods are set to defaults.
 */
struct esh_shell shell = {
    .build_prompt = build_prompt_from_plugins,
    .readline = readline,       /* GNU readline(3) */
    .parse_command_line = esh_parse_command_line, /* Default parser */
    .get_jobs = get_jobs,
    .get_job_from_jid = get_job_from_jid,
    .get_job_from_pgrp = get_job_from_pgrp,
    .get_cmd_from_pid = get_cmd_from_pid
};

int
main(int ac, char *av[])
{
    int opt;

    // Prepare everything beforehand
    shell_initialize();

    /* Process command-line arguments. See getopt(3) */
    while ((opt = getopt(ac, av, "hp:")) > 0) {
        switch (opt) {
        case 'h':
            usage(av[0]);
            break;

        case 'p':
            esh_plugin_load_from_directory(optarg); // Load the plugins into the created list
            esh_plugin_initialize(&shell);  // Initialize the loaded plugins
            break;
        }
    }

    /* Read/eval loop. */
    for (;;) {

        /* Do not output a prompt unless shell's stdin is a terminal */
        char * prompt = isatty(0) ? shell.build_prompt() : NULL;
        char * cmdline = shell.readline(prompt);
        free (prompt);

        if (cmdline == NULL) { /* User typed EOF */
            break;
        }

        cline = shell.parse_command_line(cmdline);
        free (cmdline);
        if (cline == NULL) {                /* Error in command line */
            continue;
        }

        if (list_empty(&cline->pipes)) {    /* User hit enter */
            esh_command_line_free(cline);
            continue;
        }

        /* Take cline and deal with it  */
        struct list_elem *p, *c;
        struct esh_pipeline *pipeline;  // a single pipeline in a list of pipelines
        struct esh_command *command;    // a single command in a list of commands (of a pipeline)
        bool flag;                      // true if the pipeline is a plugin or jobs, fg, bg, kill, stop, exit

        // Iterate through the command line's pipelines, in each pipeline, iterate through its commands
        //for (p = list_begin(&cline->pipes); p != list_end(&cline->pipes); p = list_next(p)) { // SEG FAULT
        while (!list_empty(&cline->pipes)) {
            p = list_pop_front(&cline->pipes);

            pipeline = list_entry(p, struct esh_pipeline, elem);

            // Pre-process pipeline
            hasPipe = pipeline_pre_process(pipeline);

            for (c = list_begin(&pipeline->commands); c != list_end(&pipeline->commands); c = list_next(c)) {
                command = list_entry(c, struct esh_command, elem);

                flag = false;

                // Check if the current command is a plugin, if it is, execute it
                if (plugin_execute(command)) {
                    flag = true;
                    break;
                }
                
                // Check if the current command is jobs, fg, bg, kill,stop, or exit
                switch (command_type(command->argv[0]))
                {
                    case 1:
                        flag = true;
                        jobs_execute();
                        break;
                    case 2:
                        flag = true;
                        fg_execute(command->argv[1]);
                        break;
                    case 3:
                        flag = true;
                        bg_execute(command->argv[1]);
                        break;
                    case 4:
                        flag = true;
                        kill_execute(command->argv[1]);
                        break;
                    case 5:
                        flag = true;
                        stop_execute(command->argv[1]);
                        break;
                    case 6:
                        exit(EXIT_SUCCESS);
                        break;
                    case 7: // Process pipeline
                        pipeline_process(pipeline, command, c);
                        break;
                } // end switch

                if (flag) {
                    break;
                }
            } // end command loop       

            // Post-process pipeline
            if (!flag)
            {
                pipeline_post_process(pipeline, p);
            }
        } // end pipeline loop

        esh_command_line_free(cline);
    }

    return 0;
}