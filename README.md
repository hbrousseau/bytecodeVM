# cLox
cLox is a modern c++ implementation heavily reliant upon *jlox* from Part II of [Crafting Interpreters](https://www.craftinginterpreters.com/) by Robert Nystrom. This project was developed as an assignment for CS 403 at the University of Alabama during Fall of 2023. Throughout the development of cLox, I attempted to stay as true to the original implementation as I could. For more information of my challenges, look under [Challenges](#challenges) or skip it if you don't want to hear me complain.

cLox has been tested on Windows 10 and 11 with Microsoft's VS Code. If you would like to test it on your own system, please follow the instructions found under the [Building](#building) header. If you have any issues, please email me at: hbrousseau@crimson.ua.edu

## Files and Reading the Code
I have submitted a zip file for the final project partitioned for each chapter that I have worked through (chapters 14-30, which is the entire clox portion of the book). I attempted to comment throughout all of the chapters what the code was attempting to do and when/where I was finding issues. 

## Building
Clone the repository to a location most convenient to you.
```
git clone https://github.com/hbrousseau/cLox.git
```

Navigate inside ch30Checkpoint to build the entire project.
```
cd ch30Checkpoint
```

To compile the entire interpreter, run `make`

## Usage
clox has two usages: either as a REPL (read, print, eval loop) or, given a source file, cLox will attempt to execute the code and exit the program. To run the program as a REPL:
```
./cLox.exe
```

Otherwise, to execute a Lox source file:
```
./cLox.exe [Lox script]
```

## Testing
I used the test cases from [Robert Nystrom's Lox unit tests](https://github.com/munificent/craftinginterpreters/tree/master/test) excluding the benchmark portion. I will point out that the runtime errors will pop up in the terminal window instead of the test_output.txt. There is a test case inside of limits that throws a stack_overflow as well. 

Test cases can run all at once by `make test-all`

**Note:** If you would like to generate your own output text, I would either make a copy of the test_output.txt file and delete the contents so the output would read directly inside that file. Or, you can modify the Makefile at the OUTPUT_FILE variable to save it inside a different folder. If you end up modifying the Makefile, you could also just change the filename. 

On line 22, you could modify the OUTPUT_FILE variable as this to change the text file's name:
```
OUTPUT_FILE := YessickOutput.txt
```

Or inside a different folder:
```
OUTPUT_FILE := testFolderName/test_output.txt
```

## Challenges 

Even though I followed the book to a T, I had to restart a few times as I was encountering strange behavior. For instance, I completely restarted when I got to chapter 21, because I kept running into errors that the variables were not being stored. But, in previous chapters it was. I believe the issue was with the hash table and the scanner. After restarting, I got stuck again in ch24 and it was another issue with the hash table. Then, I got stuck at ch28, with using 'this.' However, this one was not a problem with the hash table. I decided to finish the book and come back for this later, since I was almost to the end and everything else was working. The problem: After declaring 'this' as a variable inside my initCompiler function, I accidentally wrote what was in the else statement outside of the else statement as well. So, this is the correct way to implement:
```
if (type != TYPE_FUNCTION) {                            // added in ch28
        local->name.start = "this";
        local->name.length = 4;
    } 
    else {                                              // added in ch28
        local->name.start = "";
        local->name.length = 0;
    }
```

And I was implementing it like this by accident:
```
if (type != TYPE_FUNCTION) {                            // added in ch28
        local->name.start = "this";
        local->name.length = 4;
    } 
    else {                                              // added in ch28
        local->name.start = "";
        local->name.length = 0;
    }

    local->name.start = ""; // ??
    local->name.length = 0; // ??
```

See the problem? Yeah, took me way too long to see that. 
