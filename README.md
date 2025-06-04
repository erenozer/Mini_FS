# MiniFS Project - Eren Ozer 150240719

# Compile & Run
- Use "make" to compile the code and run it directly.

# How to Compile
To compile the project:

- Run "make compile", mini_fs executable will be created.

# How to Run Demo Sequence in Main
To run the sequence in main and see the logs in run_log.txt, use "make run"

Demo main sequence will:
- Format the disk.
- Create a directory.
- Create/write a file.
- Read the file.
- List directory contents.
- Delete file and directory.
- Demonstrate bitmap/inode reuse.

# Command Line Interface
After compiling, use ./mini_fs <command> [argument] to execute commands within the terminal to modify the existing disk. 

# Automated Tests
- Run `make check`
- This executes the commands in `tests/commands.txt`, creates an output.txt file and compares it to `tests/expected_output.txt`, as explained in the homework document.

# Files Implemented
- fs.h / fs.c - File system implementation
- disk.h - Constants and disk layout
- main.c - Command Line Interface & Demo Sequence
- tests/commands.txt - Test command script
- tests/expected_output.txt - Static expected output for the current commands.txt
- tests/output.txt - Actual output from "make check" run to compare.
- run_log.txt - Auto-generated log of main from "make run"