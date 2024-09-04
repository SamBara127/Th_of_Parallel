EXE = app.out
FL = 
THREADS = 16

ifdef FIRST
    FLAGS += -DFIRST
	FL = FIRST
endif

ifdef SECOND
    FLAGS += -DSECOND
	FL = SECOND
endif

all: $(EXE)

$(EXE): Source.cpp
	@clear
	@echo "START COMPILING PROGRAM VERSION -> $(FL) ..."
	@g++ $(FLAGS) $^ -o $@ -lm -fopenmp
	@echo "COMPILING HAS ENDED!"

run: 
	@clear
	@echo "RUNNING ..."
	@./app.out $(THREADS)
	@echo "PROGRAM HAS ENDED!"

clean:
	@clear
	@echo CLEARING EXE_FILES...
	@rm -f $(EXE)
	@echo "CLEARING HAS ENDED!"