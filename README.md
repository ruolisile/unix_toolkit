# unix_toolkit
This is a simple unix systems toolkit for process and filesystem management. </br>

## Build
* Ensure Makefile, mytoolkit.c, mytree.c and mymtimes.c are all in the same directory. </br>
* Run `make` to build the program. </br>
* Use `./mytoolkit` to run the program. </br> 

## Note
* Commands, arguments, I/O symbols and pipe symbols should be space separated, otherwise the toolkit might behave abonormaly. </br>
* The toolkit support all existing external unix command and functions within the same directory of this mytoolkit function. </br>
* The toolkit support I/O redirection and pipe, however, I/O redirection and pipe in the same command line is not supported. </br>
* Even though there are some error handling, enterring incorrect command name might cause the function behave abnormally. </br>

## Know Bugs
* Buffer overflow might occur. 
* Pipe doesn't work with internal command `mycd`, `mypwd`, `mytime`, `mytimeout`.</br>

## Functions

### myexit
**Description:** This is an internal function. This function terminate teh toolkit. </br>
**Syntax:** `exit` </br>

### mycd
**Description:** This is an internal function. This function change the current working directory of the toolkit. </br> 
**Syntax:** `mycd [dir]`. If dir is not specified, the current working directory will be changed to $HOME. </br>

### mypwd:
**Description:** This is an internal fucntion. This functin print the absolute path of current working directory. </br>
**Syntax:** `mypwd`</br>

### mytree
**Description:** This is an external function. This function prints the directories and files in a tree-like format.</br>
**Syntax:** `mytree <dir>`, if `dir` is not specified, the command will use the current working directory as teh begining directory.</br>

### mytime
**Description:** This is an internal function. This function report the following three values in seconds: </br>
- user CPU time: the sum of the tms_utime and tms_cutime values in a struct tms as returned by times(2)
- system CPU time: the system CPU time (the sum of thetms_stime and tms_cstime values in a struct tms as returned by times(2)).
- elapsed wall-clock time for running the command `cmd`: the elapsed real time between invocation and termination. </br>
**Syntax:** `mytime cmd [arguments]`, where `cmd` is the command to run,`[arguments[` is the optional arguments for `cmd` </br>

### mymtimes
**Description:** This is an external function. This function reports the hourly number of files last modified in the last 24 hours given a directory. </br>
**Syntax:** `mymtimes [dir]`. If `dir` is not specified, this command will use the current working directory. </br>

### mytimeout
**Description:** This is an internal function. This function will run a command with time limit. </br>
**Syntax:** `mytimeout <seconds> <command> [arguments]` where `<seconds>` is the time limit in second, `<command>` is the command to run and `arguments` is optional argument list for the command. </br>






