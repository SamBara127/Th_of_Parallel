EXE = app.out
FL = -fopenmp -lm

all: $(EXE)

$(EXE): Source.cpp
	@clear
	@echo "START COMPILING..."
	@g++ $(FL) $^ -o $@ -lm
	@echo "COMPILING HAS ENDED!"

run: 
	@echo "RUNNING ..."
	@./$(EXE) $(ARGS)
	@echo "PROGRAM HAS ENDED!"

clean:
	@clear
	@echo CLEARING EXE_FILES...
	@rm -f $(EXE)
	@echo "CLEARING HAS ENDED!"
