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
Open file descriptors are preserved across a call to exec


but the first cmd should have output then it can pipe to second's input, will this cause problem
Sequential / Blocking
```
while (curr != NULL) {
    fork();
    // ... setup pipes ...
    wait(NULL); // <--- ERROR! 
    // If you wait here, the parent stops. 
    // Only ONE child runs at a time.
    // The pipe will break because no one is reading the output!
    curr = curr->next;
}
```
Concurrent / Streaming
```
while (curr != NULL) {
    fork();
    // ... setup pipes ...
    // Do NOT wait here. Keep looping to start the next process!
    curr = curr->next;
}

// Wait for everyone ONLY after everyone has started
while (wait(NULL) > 0);

Also, your execute_builtin_safe has a Critical Logic Bug (Sequence Error). You are restoring the file descriptors before you run the command, which means the command will ignore the redirection.
```
Reference:

https://github.com/brenns10/lsh

The coding style apply the Old C89, make all variables on top of a function.
https://news.ycombinator.com/item?id=32525817


https://stackoverflow.com/questions/262439/create-a-wrapper-function-for-malloc-and-free-in-c


execvp `file` if the file has `/` than thoguht to be absolute, else considered `relative` and would search from current dir
execvp expects the `argv` array to look like this: ["ls", "-l", NULL]