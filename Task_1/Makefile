EXE = app.out
FL = 

ifdef FLOAT
    FLAGS += -DFLOAT
	FL = FLOAT
endif

ifdef DOUBLE
    FLAGS += -DDOUBLE
	FL = DOUBLE
endif

all: $(EXE)

$(EXE): Source.c
	@clear
	@echo "START COMPILING WITH DATA_TYPE -> $(FL) ..."
	@gcc $(FLAGS) $^ -o $@ -lm
	@echo "COMPILING HAS ENDED!"

run: 
	@echo "RUNNING ..."
	@./app.out
	@echo "PROGRAM HAS ENDED!"

clean:
	@clear
	@echo CLEARING EXE_FILES...
	@rm -f $(EXE)
	@echo "CLEARING HAS ENDED!"