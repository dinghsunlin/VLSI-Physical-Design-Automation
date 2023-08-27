CXX			= g++

CXXFLAGS	= -O3 -std=c++17

SRC			= main.cpp

RM			= rm

EXE			= ../bin/hw3

VER			= ../verifier/verifier

OUT			= ../output/*.floorplan

all :: opt
opt: $(SRC)
	make -s clean && $(CXX) $(CXXFLAGS) $(SRC) -o $(EXE)
clean:
	$(RM) -rf $(EXE) $(OUT)
test: opt
	@read -n4 -p "Which testcase to run? " CASE; \
	echo ""; \
	read -n4 -p "What is the dead space ratio? " RATIO; \
	echo ""; \
	echo "Running testcase $$CASE with dead space ratio $$RATIO..."; \
	$(EXE) ../testcases/$$CASE.hardblocks ../testcases/$$CASE.nets ../testcases/$$CASE.pl ../output/$$CASE.floorplan $$RATIO; \
	$(VER) ../testcases/$$CASE.hardblocks ../testcases/$$CASE.nets ../testcases/$$CASE.pl ../output/$$CASE.floorplan $$RATIO
ratio_tests: opt
	@read -n4 -p "What is the dead space ratio? " RATIO; \
	for CASE in n100 n200 n300 ; do \
		echo "Running testcase $$CASE with dead space ratio $$RATIO..."; \
		$(EXE) ../testcases/$$CASE.hardblocks ../testcases/$$CASE.nets ../testcases/$$CASE.pl ../output/$$CASE.floorplan $$RATIO; \
		$(VER) ../testcases/$$CASE.hardblocks ../testcases/$$CASE.nets ../testcases/$$CASE.pl ../output/$$CASE.floorplan $$RATIO; \
	done
