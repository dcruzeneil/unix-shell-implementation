# Unix Shell Implementation
This project features a C program that implements a Unix-like shell, supporting I/O redirection and piping.

### Compiling the Code
To compile the program, use the provided `Makefile`:
```sh
make
```

### Using the Shell
To start the shell (assuming you are in the same directory):
```sh
./mysh
```

This will bring you to a prompt that looks like this:
```sh
$
```

You can now use this shell similarly to the default UNIX shell. Below are some demonstrations of the I/O redirection and piping support. These operations can be combined, as shown in the piping examples.

##### Input Redirection
To read input from a file called `input.txt`:
```sh
$ cat < input.txt
```
<i> Note: This is not a typical use of the `cat` command but is done for demonstration purposes.</i>

##### Output Redirection
To write the output of `ls -al` to a file called `output.txt` and to overwrite everything previously written in `output.txt`:
```sh
$ ls -al > output.txt
```

However, to append to `output.txt`:
```sh
$ ls >> output.txt
```
##### Piping Support
This shell supports multiple piping, allowing the output of one program to be passed as input to another. This is useful in various scenarios.

For example, to read a file's content, count the number of lines, and store the count in another file:
```sh
$ cat output.txt | wc -l > line_count
```

To read a file's contents, find a specific keyword, count the number of lines where the keyword appears, and store the output in a new file:
```sh
$ cat output.txt | grep -i mysh | wc -l > grepped_line_count
```

To test multiple piping support, you can chain several `cat` commands:
```sh
$ cat output.txt | grep -i mysh | wc -l | cat | cat | cat | cat > grepped_line_count
```
### Exiting the Shell
To exit the shell, type the `exit` command or press `CTRL + D`.



