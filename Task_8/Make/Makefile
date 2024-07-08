EXE = app.out
CPP = Source.cu
FL = -lm -lboost_program_options -arch=sm_70

RESOLUTION = 1024
ACCURACY = 0.000001 
ITERATION = 1000000
MODE = 0

gpu:
	@clear
	@echo "START COMPILING PROGRAM ON GPU_CUDA..."
	@nvcc $(CPP) -o $(EXE) $(FL) -w
	@echo "COMPILING HAS ENDED!"

run: 
	@clear
	@echo "RUNNING ..."
	@./$(EXE) -g $(RESOLUTION) -a $(ACCURACY) -n $(ITERATION) -m $(MODE)
	@echo "PROGRAM HAS ENDED SUCCESSFULLY!"

profile:
	@clear
	@echo "PROFILING ..."
	@nsys profile -o ProfileReport -t cuda ./$(EXE) -g $(RESOLUTION) -a $(ACCURACY) -n 1000 -m 0

clean:
	@clear
	@echo CLEARING EXE_FILES...
	@rm -f $(EXE)
	@echo "CLEARING HAS ENDED!"