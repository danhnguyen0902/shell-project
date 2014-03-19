#!/usr/bin/python
#
# io_append_test
#
# Test that the shell correctly makes use of the
# ">>" command to append input to a file
#
#       Requires the use of the following commands:
#
#       echo, cat
#
# ******** NOTE: Make sure you delete any out_append files before running this test *******
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

# run a command that appends to the file
c.sendline("echo Im trying to append this! >> dump")

# print the content of the test file
c.sendline("cat dump")

# should output the contents of the text file
assert c.expect_exact("Im trying to append this!\r\n") == 0, "Shell did not print the expected prompt"

# run a second command that appends to the file
c.sendline("echo lets add more to this! >> dump")

# print the content of the test file
c.sendline("cat dump")

# should output the new content of the text file
assert c.expect_exact("Im trying to append this!\r\nlets add more to this!\r\n") == 0, "Shell did not print the expected prompt"

#exit
c.sendline("exit")
assert c.expect_exact("exit") == 0, "Shell output extraneous characters"


shellio.success()
