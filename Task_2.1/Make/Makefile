EXE = app.out
FL = -fopenmp

all: $(EXE)

$(EXE): Source.c
	@clear
	@echo "START COMPILING..."
	@gcc $(FL) $^ -o $@ -lm
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
