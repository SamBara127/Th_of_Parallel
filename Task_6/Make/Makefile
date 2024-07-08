EXE = app.out
CPP = Source.cpp
FL = -lm -lboost_program_options
CORES := $(shell grep -c ^processor /proc/cpuinfo)

RESOLUTION = 1024
ACCURACY = 0.000001 
ITERATION = 1000000

cpu: 
	@clear
	@echo "START COMPILING PROGRAM ON CPU..."
	@pgc++ $(CPP) -o $(EXE) $(FL) -w 
	@echo "COMPILING HAS ENDED!"

gpu:
	@clear
	@echo "START COMPILING PROGRAM ON GPU..."
	@pgc++ -acc -Minfo=accel $(CPP) -o $(EXE) $(FL) -w 
	@echo "COMPILING HAS ENDED!"

cpu_mult:
	@clear
	@echo "START COMPILING PROGRAM ON CPU_MULTICORE..."
	@pgc++ -acc=multicore -Minfo=accel $(CPP) -o $(EXE) $(FL) -w 
	@echo "COMPILING HAS ENDED!"

run: 
	@clear
	@echo "RUNNING ..."
	@./$(EXE) -g $(RESOLUTION) -a $(ACCURACY) -n $(ITERATION)
	@echo "PROGRAM HAS ENDED SUCCESSFULLY!"

profile:
	@clear
	@echo "PROFILING ..."
	@nsys profile -o ProfileReport_2 -t openacc ./$(EXE) -g $(RESOLUTION) -a $(ACCURACY) -n 1000
	@echo "PROGRAM HAS PROFILED SUCCESSFULLY!"

clean:
	@clear
	@echo CLEARING EXE_FILES...
	@rm -f $(EXE)
	@echo "CLEARING HAS ENDED!"