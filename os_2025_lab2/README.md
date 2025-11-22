command.h (The Model): Only defines data structures and functions that manipulate those structures (create, free, print).

builtin.h (The Module): Only defines the built-in command functions.

shell.h (The Controller): Defines the main lifecycle (Loop, Read, Parse, Execute) and the process management.


Overall 
```
Load_config_files()

Run_commands_loop()

Shutdown()
```

What is the key difference between redirection and pipe?
A redirect is make `fd` attach to different source, eg. output to STDOUT change to a txt file
A pipe separates two commands, also change to fd, but the output is piped as arguements to next command


Reference:

https://github.com/brenns10/lsh

The coding style apply the Old C89, make all variables on top of a function.
https://news.ycombinator.com/item?id=32525817


https://stackoverflow.com/questions/262439/create-a-wrapper-function-for-malloc-and-free-in-c


execvp `file` if the file has `/` than thoguht to be absolute, else considered `relative` and would search from current dir
execvp expects the `argv` array to look like this: ["ls", "-l", NULL]