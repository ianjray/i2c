CXX      = clang++
SANITIZE = -fsanitize=address -fsanitize=undefined-trap -fsanitize-undefined-trap-on-error
COVERAGE = -fprofile-instr-generate -fcoverage-mapping
OPTS     = -std=c++17 $(SANITIZE) $(COVERAGE) -O2 -Weverything -Wno-c++98-compat -Wno-padded -Wno-poison-system-directories -Wno-weak-vtables -Wno-global-constructors -Wno-exit-time-destructors
XCRUN    = xcrun

.PHONY : all
all : test_i2c.coverage

%.coverage : %.profdata
	$(XCRUN) llvm-cov show $*.unittest -instr-profile=$< $*.cpp > $@
	! grep " 0|" $@
	echo PASS $@

%.profdata : %.profraw
	$(XCRUN) llvm-profdata merge -sparse $< -o $@

%.profraw : %.unittest
	LLVM_PROFILE_FILE=$@ ./$<

test_i2c.unittest : test_i2c.cpp bus.cpp controllerbase.cpp line.cpp log.cpp node.cpp target.cpp targetbase.cpp
	$(CXX) $(OPTS) $^ -o $@

.PHONY : clean
clean :
	rm -rf *.coverage *.profdata *.profraw *.unittest*
