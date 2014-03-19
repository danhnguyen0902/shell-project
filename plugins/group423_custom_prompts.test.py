#!/usr/bin/python
#
# Tests the custom_prompts plugin.
#
#
import sys, imp, atexit
sys.path.append("/home/courses/cs3214/software/pexpect-dpty/");
import pexpect, shellio, signal, time, os, re, proc_check, getpass

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
c = pexpect.spawn(def_module.shell + plugin_dir, drainpty=True, logfile=logfile)

atexit.register(force_shell_termination, shell_process=c)
c.timeout = 2
c.logfile = sys.stdout


# Ensure that shell prints default prompt first
assert c.expect('esh> ') == 0, "Shell did not print default prompt (1)"

# Change the prompt to the username
c.sendline('prompt 1')

# Ensure that shell prints the right username
assert c.expect(getpass.getuser() + '> ' ) == 0, "Shell did not print username prompt (1)"

# Change the prompt to the time
c.sendline('prompt 2')

# Ensure that shell prints something that matches the time format
assert c.expect("\w{3} \w{3} \d+ \d{2}:\d{2}:\d{2} \d{4}") == 0, "Shell did not print time prompt (1a)"
# Also need to see the "> " at the end
assert c.expect('> ') == 0, "Shell did not print time prompt (1b)"


shellio.success()
