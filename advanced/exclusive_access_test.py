#!/usr/bin/python
#
# exclusive_terminal_access_test:
#
# Test that the shell hands access over to a program
# correctly.
#
#       Requires the use of the following commands:
#
#       vim, ctrl-z control, fg
#

import sys, imp, atexit
sys.path.append("/home/courses/cs3214/software/pexpect-dpty/");
import pexpect, shellio, signal, time, os, re, proc_check

#Ensure the shell process is terminated
def force_shell_termination(shell_process):
        c.close(force=True)

#pulling in the regular expression and other definitions
definitions_scriptname = sys.argv[1]

def_module = imp.load_source('', definitions_scriptname)
logfile = None
if hasattr(def_module, 'logfile'):
    logfile = def_module.logfile

#spawn an instance of the shell
c = pexpect.spawn(def_module.shell, drainpty=True, logfile=logfile)
atexit.register(force_shell_termination, shell_process=c)

# set timeout for all following 'expect*' calls to 2 seconds
c.timeout = 2

# ensure that shell prints expected prompt
assert c.expect(def_module.prompt) == 0, "Shell did not print expected prompt"



# vim needs exclusive access
c.sendline("vim testvim")

# make sure vim is in the foreground in order to test sigtstp
proc_check.wait_until_child_is_in_foreground(c)


# send SIGTSTP to vim
c.sendcontrol('z')


# check for default prompt
assert c.expect(def_module.prompt) == 0, "Shell did not print expected prompt"



#exit
c.sendline("exit")
assert c.expect_exact("exit") == 0, "Shell output extraneous characters"


shellio.success()
