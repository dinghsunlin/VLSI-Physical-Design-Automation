CXX			= g++

CXXFLAGS	= -O3 -std=c++17

SRC			= main.cpp

RM			= rm

EXE			= ../bin/hw5

VER			= ../verifier/verify

OUT			= ../output/*.def

all :: opt
opt: $(SRC)
	make -s clean && $(CXX) $(CXXFLAGS) $(SRC) -o $(EXE)
clean:
	$(RM) -rf $(EXE) $(OUT)
test: opt
	@read -p "How many sources? " CASE; \
	echo "Running on $$CASE sources ..."; \
	$(EXE) $$CASE ../output/CS_$$CASE.def; \
	echo "Running verification on CS_$$CASE.def ..."; \
	$(VER) $$CASE ../output/CS_$$CASE.def
out: opt
	echo "Running on 4 sources ..."; \
	$(EXE) 4 ../output/CS_4.def; \
	echo "Running on 16 sources ..."; \
	$(EXE) 16 ../output/CS_16.def; \
	echo "Running on 36 sources ..."; \
	$(EXE) 36 ../output/CS_36.def; \
	echo "Running on 64 sources ..."; \
	$(EXE) 64 ../output/CS_64.def; \
	echo "Running on 100 sources ..."; \
	$(EXE) 100 ../output/CS_100.def; \
	echo "Running verification on CS_4.def ..."; \
	$(VER) 4 ../output/CS_4.def; \
	echo "Running verification on CS_16.def ..."; \
	$(VER) 16 ../output/CS_16.def; \
	echo "Running verification on CS_36.def ..."; \
	$(VER) 36 ../output/CS_36.def; \
	echo "Running verification on CS_64.def ..."; \
	$(VER) 64 ../output/CS_64.def; \
	echo "Running verification on CS_100.def ..."; \
	$(VER) 100 ../output/CS_100.def; \