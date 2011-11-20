CXXFLAGS=-g -Wall -I$(LLVM)/tools/clang/include
LDFLAGS=-L$(LLVM)/Release+Asserts/lib -lclang

sclang: sclang.o
