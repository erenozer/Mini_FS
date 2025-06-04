all: compile run

compile: main.c fs.c fs.h disk.h
	@echo "-----------------------------------------"
	@echo "Compiling..."
	@gcc -o mini_fs main.c fs.c -pthread
	@echo "Compilation completed."

run: mini_fs
	@echo "-----------------------------------------"
	@echo "Running the program..."
	@echo "======================================================================="
	@./mini_fs 2>&1 | tee run_log.txt
	@echo "======================================================================="
	@echo "Program completed."

check: mini_fs
	./mini_fs < tests/commands.txt > tests/output.txt 2> /dev/null
	diff -u tests/expected_output.txt tests/output.txt \
	&& echo "Output matches expected." \
	|| { echo "Output mismatch."; exit 1; }

clean:
	@echo "-----------------------------------------"
	@echo "Removing compiled files..."
	@rm -f mini_fs
	@echo "Removed compiled files."