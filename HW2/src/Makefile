CPP		=g++

SRC		=main.cpp

RM		=rm

EXE		=../bin/hw2

VER		=../verifier/verify

OUT		=../output/*.out

all :: opt
noopt:
	make clean && $(CPP) -O0 -std=c++17 $(SRC) -o $(EXE)
opt:
	date && make clean && $(CPP) -O3 -std=c++17 $(SRC) -o $(EXE)
out:
	make tc_1 && make tc_2 && make tc_3 && make tc_4 && make tc_5
ver:
	make ts_1 && make ts_2 && make ts_3 && make ts_4 && make ts_5
loop:
	make opt && make out && make ver
clean:
	$(RM) -f $(EXE) $(OUT)
tc_1:
	$(EXE) ../testcases/p2-1.cells ../testcases/p2-1.nets ../output/p2-1.out
tc_2:
	$(EXE) ../testcases/p2-2.cells ../testcases/p2-2.nets ../output/p2-2.out
tc_3:
	$(EXE) ../testcases/p2-3.cells ../testcases/p2-3.nets ../output/p2-3.out
tc_4:
	$(EXE) ../testcases/p2-4.cells ../testcases/p2-4.nets ../output/p2-4.out
tc_5:
	$(EXE) ../testcases/p2-5.cells ../testcases/p2-5.nets ../output/p2-5.out
ts_1:
	$(VER) ../testcases/p2-1.cells ../testcases/p2-1.nets ../output/p2-1.out
ts_2:
	$(VER) ../testcases/p2-2.cells ../testcases/p2-2.nets ../output/p2-2.out
ts_3:
	$(VER) ../testcases/p2-3.cells ../testcases/p2-3.nets ../output/p2-3.out
ts_4:
	$(VER) ../testcases/p2-4.cells ../testcases/p2-4.nets ../output/p2-4.out
ts_5:
	$(VER) ../testcases/p2-5.cells ../testcases/p2-5.nets ../output/p2-5.out