CXXFLAGS = -std=c++26 -O2 -Wall -Wextra -I include -fmax-errors=4

%.v1.o: %.cpp include/simd include/bits/*.h
	$(CXX) $(CXXFLAGS) -march=x86-64 -c $<

%.v2.o: %.cpp include/simd include/bits/*.h
	$(CXX) $(CXXFLAGS) -march=x86-64-v2 -c $<

%.v3.o: %.cpp include/simd include/bits/*.h
	$(CXX) $(CXXFLAGS) -march=x86-64-v3 -c $<

%.v4.o: %.cpp include/simd include/bits/*.h
	$(CXX) $(CXXFLAGS) -march=x86-64-v4 -c $<

%.avx.o: %.cpp include/simd include/bits/*.h
	$(CXX) $(CXXFLAGS) -march=ivybridge -c $<

v1: test.v1.o constexpr_tests.v1.o
v2: test.v2.o constexpr_tests.v2.o
v3: test.v3.o constexpr_tests.v3.o
v4: test.v4.o constexpr_tests.v4.o
avx: test.avx.o constexpr_tests.avx.o

all: v1 v2 v3 v4 avx
	$(CXX) $(CXXFLAGS) -c test.cpp
	$(CXX) $(CXXFLAGS) -march=skylake -c test.cpp
	$(CXX) $(CXXFLAGS) -march=skylake-avx512 -c test.cpp

help:
	@echo "... all"
	@echo "... v1"
	@echo "... v2"
	@echo "... v3"
	@echo "... v4"
	@echo "... avx"

clean:
	rm *.o

.PHONY: all clean v1 v2 v3 v4 avx help
