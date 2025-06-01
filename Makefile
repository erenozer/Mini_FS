all: compile run

compile: studentID_market_sim.c market_sim.h
	@echo "-----------------------------------------"
	@echo "Compiling..."
	@gcc -o market_sim studentID_market_sim.c -pthread
	@echo "Compilation completed."

run: market_sim
	@echo "-----------------------------------------"
	@echo "Running the program..."
	@echo "======================================================================="
	@./market_sim
	@echo "======================================================================="
	@echo "Program completed."

clean:
	@echo "-----------------------------------------"
	@echo "Removing compiled files..."
	@rm -f market_sim
	@echo "Removed compiled files."