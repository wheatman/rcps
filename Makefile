CFLAGS := -Wall -Wextra -O3 -g  -std=c++17   -march=native -fconstexpr-steps=100000000

ifeq ($(INFO), 1) 
# CFLAGS +=  -Rpass-missed="(inline|loop*)" 
#CFLAGS += -Rpass="(inline|loop*)" -Rpass-missed="(inline|loop*)" -Rpass-analysis="(inline|loop*)" 
CFLAGS += -Rpass=.* -Rpass-missed=.* -Rpass-analysis=.* 
endif

ifeq ($(SANITIZE),1)
CFLAGS += -fsanitize=undefined,address -fno-omit-frame-pointer
endif


all: rcps
 
rcps: rcps.cpp
	$(CXX) $(CFLAGS) -o $@ rcps.cpp

profile: rcps.cpp
	$(CXX) $(CFLAGS) -fprofile-instr-generate -o $@ rcps.cpp
	./profile 200 0 1000000 > /dev/null
	llvm-profdata-10 merge -output=code.profdata default.profraw
	

opt: profile
	$(CXX) $(CFLAGS) -fprofile-instr-use=code.profdata -o $@ rcps.cpp

clean:
	rm -f rcps