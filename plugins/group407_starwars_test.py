#group407_starwars_eshoutput.py
#!/usr/bin/python
#
# Stephen Fenton sfentonx@vt.edu
# Andrew Weckstein awex@vt.edu
#
# This test ensures that our plugin is working by verifying the output of the -info tab, as well as just the 'starwars' argument.
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
c = pexpect.spawn(def_module.shell + plugin_dir, drainpty=True, logfile=logfile)

atexit.register(force_shell_termination, shell_process=c)

child = pexpect.spawn("./esh -p plugins")
child.logfile = open("StarWarsLog", "w+")
child.expect(".*")

child.sendline("starwars -quiz")
child.expect(".*")
child.sendline("3")
child.expect(".*")
child.sendline("2")
child.expect(".*")
child.sendline("4")
child.expect(".*")
child.sendline("2")
child.expect(".*")
child.sendline("3")
child.expect(".*")
child.sendline("4")
child.expect(".*")
child.sendline("1")
child.expect(".*")
child.sendline("4")
child.expect(".*")
child.sendline("4")
child.expect(".*")
child.sendline("2")
child.expect(".*")
child.sendline("starwars")

logger = open("StarWarsLog").read()
case1 = logger.find("You are a Jedi Master!  10/10!") != -1
case2 = logger.find("What type of empire *is* the Empire?") != -1
case3 = logger.find("What kind of farmers were the Lars'?") != -1
case4 = logger.find("What things do Jawas trade in?") != -1
case5 = logger.find("What kind of star system does Tatooine have?") != -1
case6 = logger.find("Over what planet's moon was there a massive battle?") != -1
case7 = logger.find("What is the Millenium Falcoln captured by?") != -1
case8 = logger.find("What was the name of George Lucas's dog that inspired Chewbacca, that later inspired another film?") != -1
case9 = logger.find("What was Luke's original last name?") != -1
case10 = logger.find("Who composed the score for Star Wars?") != -1
case11 = logger.find("PLAY MOVIE")

assert case1
assert case2
assert case3
assert case4
assert case5
assert case6
assert case7
assert case8
assert case9
assert case10
assert case11

os.remove("StarWarsLog")
	
shellio.success()
