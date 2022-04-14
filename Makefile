# sample Makefile, feel free to make any modification.
RES = src/main.cpp
EXE = pa2
all:
	g++ -std=c++11 $(RES) -o $(EXE)
clean:
	rm $(EXE)