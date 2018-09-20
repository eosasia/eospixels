.PHONY: test fuzz

test: test.bin
	./test.bin

test.bin: *.hpp *.cpp
	g++ -DGCC -std=c++14 -o test.bin test.cpp

fuzz: fuzz.bin
	fuzz.bin

fuzz.bin: fuzz.cpp
	g++ -DGCC -std=c++14 -o fuzz.bin fuzz.cpp
