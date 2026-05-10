CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -I./include

# Targets
.PHONY: all clean run test gen

all: nanodb gen_data test_runner

# Main NanoDB engine
nanodb: src/main.cpp include/*.h
	$(CXX) $(CXXFLAGS) -o nanodb src/main.cpp

# Data generator
gen_data: src/gen_data.cpp
	$(CXX) $(CXXFLAGS) -o gen_data src/gen_data.cpp

# Automated test runner
test_runner: tests/test_runner.cpp include/*.h
	$(CXX) $(CXXFLAGS) -o test_runner tests/test_runner.cpp

# Generate data, build everything, run test runner
run: all gen_data
	mkdir -p data
	./gen_data
	cp tests/queries.txt queries.txt
	./test_runner 2>&1 | tee nanodb_execution.log

# Just build and run main engine
demo: nanodb gen_data
	mkdir -p data
	./gen_data
	./nanodb 2>&1 | tee nanodb_execution.log

# Valgrind memory check
memcheck: nanodb
	valgrind --leak-check=full --track-origins=yes ./nanodb

clean:
	rm -f nanodb gen_data test_runner *.ndb *.log
	rm -f data/*.tbl
