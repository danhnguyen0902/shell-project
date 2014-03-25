Extensible Shell
=============

Student Information
-------------------
Austin Moore, aemoore

Danh Nguyen, danh0902

How to install lex and yacc in Ubuntu
-------------------------------------
Open the terminal (CTRL+ALT+T)

Type the followings:
- sudo apt-get update
- sudo apt-get install bison
- sudo apt-get install flex
- sudo apt-get install byacc

How to execute the shell
------------------------
0. Run the Makefile: make

1. To run without plugins, simply run : esh

2. If you would like to run with plugins, run: esh -p plugins



Important Notes
---------------
To test the plugins:
./stdriver.py -p plugins plugins.tst

To run the basic tests:
./stdriver.py -b

To run the advanced tests:
./stdriver.py advanced.tst


Description of Base Functionality
---------------------------------
IMPLEMENTATION of the following commands:

jobs:
for the job function we loop through the job list and get a reference to the pipeline.
Then, based on the job status of the pipeline (stopped, background, foreground), print out the results.

fg:
For the foreground function, we get the job id from the command line, then we get a reference
to the pipeline, then set the status to foreground. After that, we give the terminal to the command
and send a continue signal

bg: 
For the background function, we get the job id from the command line, then we get a reference
to the pipeline, then set the status to background. After that, we give the signal to continue

kill: 
For the kill function, we get the job id from the command line, then we get a reference
to the pipeline, then send the kill signal

stop:
For the stop function, we get the job id from the command line, then we get a reference
to the pipeline, then send the stop signal

^C:
When hit, we send the stop signal to the process running in fg

^Z: 
When hit, we send the kill signal to the process running in fg

These are handled by the child_status_changed function to alert for signals.
The killpg is used to send the signals



Description of Extended Functionality
-------------------------------------
IMPLEMENTATION of the following functionality:

I/O:
While piping, we check if the ioinput or iooutput flags are set on the commands.
If they are set then the process opens up the file and sets the repective input or output
file descriptor to that file stream

Pipes:
For piping, we iterate through the list of commands. If the command requires using pipes, then we copy the 
first commands output descriptor to a pipe. Then for the next command, we use that pipe to read from. If the command is in the 
middle, then we also copy the output to a pipe. Then moving on to the next command, if it is at the end of the command
list, then we only copy the output to standard out.

Exclusive Access:
This is handled via the child_status change. We keep track of the foreground and background signals and processes 
to give access to the terminal when needed. 




List of Plugins Implemented
---------------------------

PLUGINS:

GROUP 422, OUR TEAM:

summation
- given one number sums all numbers from one to that number and displays result
- given two numbers it adds them together and displays the results
	
converter
-given a numerical value and a descriptor( F or C) will convert to celsius or Farheinheit
	
binHex
-given a number will show binary and hexadecimal of number

reverseString
-given a string will print out the reverse of it

chdir
-will change current working directory to the one specified

Written by other teams:

group415
- harrypotter
- war
- zodiac

group455
- duel
- morse
- random

group423
- custom_prompts

group407
- starwars

group416
- dom
- intToBinary
- startedFromTheBottom
- timer
- iHeartSystems
- ohHello
- stringToBinary
