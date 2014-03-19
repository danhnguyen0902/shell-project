#!/usr/bin/python
#
# David Keimig, Sayed Shah
#
#
import sys, imp, atexit
sys.path.append("/home/courses/cs3214/software/pexpect-dpty/");
import pexpect, shellio, signal, time, os, re, proc_check

#Ensure the shell process is terminated
def force_shell_termination(shell_process):
	c.close(force=True)

#pulling in the regular expression and other definitions
definitions_scriptname = sys.argv[1]
plugin_dir = sys.argv[2]
def_module = imp.load_source('', definitions_scriptname)
logfile = None
if hasattr(def_module, 'logfile'):
    logfile = def_module.logfile

#spawn an instance of the shell
c = pexpect.spawn(def_module.shell, drainpty=True, logfile=logfile, args= ['-p', 'plugins'])

atexit.register(force_shell_termination, shell_process=c)

#set expect time to 2 seconds
c.timeout = 2

#run plugin and check for output

c.sendline("war play")
assert c.expect("-------") == 0


#exit
c.sendline("exit")
assert c.expect("exit\r\n") == 0, "Shell output extraneous characters"

shellio.success()
