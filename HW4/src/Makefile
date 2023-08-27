CXX			= g++

CXXFLAGS	= -O3 -std=c++17

SRC			= main.cpp

RM			= rm

EXE			= ../bin/hw4

VER			= ../verifier/verify

OUT			= ../output/*.result

all :: opt
opt: $(SRC)
	make -s clean && $(CXX) $(CXXFLAGS) $(SRC) -o $(EXE)
clean:
	$(RM) -rf $(EXE) $(OUT)
test: opt
	@read -p "Which testcase to run? (adaptec1/3, ibm01/07/09): " CASE; \
	echo "Running testcase $$CASE ..."; \
	$(EXE) ../testcase/$$CASE/$$CASE.aux ../output/$$CASE.result; \
	echo "Running verification on $$CASE ..."; \
	$(VER) ../testcase/$$CASE/$$CASE.aux ../output/$$CASE.result
out: opt
	echo "Running testcase adaptec1 ..."; \
	$(EXE) ../testcase/adaptec1/adaptec1.aux ../output/adaptec1.result; \
	echo "Running testcase adaptec3 ..."; \
	$(EXE) ../testcase/adaptec3/adaptec3.aux ../output/adaptec3.result; \
	echo "Running testcase ibm01 ..."; \
	$(EXE) ../testcase/ibm01/ibm01.aux ../output/ibm01.result; \
	echo "Running testcase ibm07 ..."; \
	$(EXE) ../testcase/ibm07/ibm07.aux ../output/ibm07.result; \
	echo "Running testcase ibm09 ..."; \
	$(EXE) ../testcase/ibm09/ibm09.aux ../output/ibm09.result; \
	echo "Running verification on adaptec1 ..."; \
	$(VER) ../testcase/adaptec1/adaptec1.aux ../output/adaptec1.result; \
	echo "Running verification on adaptec3 ..."; \
	$(VER) ../testcase/adaptec3/adaptec3.aux ../output/adaptec3.result; \
	echo "Running verification on ibm01 ..."; \
	$(VER) ../testcase/ibm01/ibm01.aux ../output/ibm01.result; \
	echo "Running verification on ibm07 ..."; \
	$(VER) ../testcase/ibm07/ibm07.aux ../output/ibm07.result; \
	echo "Running verification on ibm09 ..."; \
	$(VER) ../testcase/ibm09/ibm09.aux ../output/ibm09.result; \