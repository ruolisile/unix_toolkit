# unix_toolkit
This is a simple unix systems toolkit for process and filesystem management. </br>

## Functions

### myexit
**Description:** This is an internal function. This function terminate teh toolkit. </br>
**Syntax:** `exit` </br>

### mycd
**Description:** This is an internal function. This function change the current working directory of the toolkit. </br> </br>
**Syntax:** `mycd [dir]`. If dir is not specified, the current working directory will be changed to $HOME. </br>

### mypwd:
**Description:** This is an internal fucntion. This functin print the absolute path of current working directory. </br>
**Syntax:** `mypwd`</br>

### mytree
**Description:** This is an external function. This function prints the directories and files in a tree-like format.</br>
**Syntax:** `mytree <dir>`, if `dir` is not specified, the command will use the current working directory as teh begining directory.</br>

### mytime
**Description:** This is an internal function. This function report the following three values: : 1) user CPU time; 2) system CPU time; and 3) elapsed wall-clock time for running the command `cmd`. </br>
**Syntax:** `mytime cmd [arguments]`, where `cmd` is the command to run,`[arguments[` is the optional arguments for `cmd` </br>

### mymtimes
**Description:** This is an external function. This function reports the hourly number of files last modified in the last 24 hours given a directory. </br>
**Syntax:** `mymtimes [dir]`. If `dir` is not specified, this command will use the current working directory. </br>

### mytimeout
**Description:** This is an external function. This function will run a command with time limit. </br>
**Syntax:** `mytimeout <seconds> <command> [arguments]` where `<seconds>` is the time limit in second, `<command>` is the command to run and `arguments` is optional argument list for the command. </br>

## Note
* Commands, arguments, and I/O symbols and pipe symbols should be space separated, otherwise the toolkit might behave abonormaly. </br>
* The toolkit support all existing external unix command and functions within the same directory of this mytoolkit function. </br>
* The toolkit support I/O redirection and pipe, however, I/O redirection and pipe in the same command line is not supported. </br>

## Know Bugs
* BUffer overflow might occur. 




