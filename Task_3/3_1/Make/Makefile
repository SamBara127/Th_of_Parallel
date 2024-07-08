EXE = app.out
FL = -lm

all: $(EXE)

$(EXE): Source_mod.cpp
	@clear
	@echo "START COMPILING PROGRAM..."
	@g++ $^ -o $@ $(FL)
	@echo "COMPILING HAS ENDED!"

run: 
	@clear
	@echo "RUNNING ..."
	@./app.out
	@echo "PROGRAM HAS ENDED!"

clean:
	@clear
	@echo CLEARING EXE_FILES...
	@rm -f $(EXE)
	@echo "CLEARING HAS ENDED!"