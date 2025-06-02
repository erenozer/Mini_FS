all: compile run

compile: main.c fs.c fs.h
	@echo "-----------------------------------------"
	@echo "Compiling..."
	@gcc -o fs_simulator main.c fs.c -pthread
	@echo "Compilation completed."

run: mini_fs
	@echo "-----------------------------------------"
	@echo "Running the program..."
	@echo "======================================================================="
	@./mini_fs
	@echo "======================================================================="
	@echo "Program completed."

clean:
	@echo "-----------------------------------------"
	@echo "Removing compiled files..."
	@rm -f mini_fs
	@echo "Removed compiled files."