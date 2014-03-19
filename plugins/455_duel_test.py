#!/usr/bin/python
#
# random_test: tests random plugin
#
# Test random plugin, assert that it returns number and that the 
# plugin follows expected behavior

import sys, imp, atexit
sys.path.append("/home/courses/cs3214/software/pexpect-dpty/");
import pexpect, shellio, signal, time, os, re, proc_check

child = pexpect.spawn("./esh -p plugins")

child.timeout = 2

with open("log2.txt", "w") as log_file:
	child.logfile = log_file
	child.send("duel\r")
	assert child.expect("Welcome to DUEL") == 0, "Unexpected output"

shellio.success()
