
CXXFLAGS := -Wall -Wextra -O3 -g  -std=c++20   -march=native 



ifeq ($(CXX),g++)
  CXXFLAGS += -DGCC=1
else ifeq ($(CXX),clang++)
  CXXFLAGS += -fconstexpr-steps=100000000 -DCLANG=1
else
  $(error has only been tested with clang++ and g++)
endif

ifeq ($(INFO), 1) 
# CFLAGS +=  -Rpass-missed="(inline|loop*)" 
#CFLAGS += -Rpass="(inline|loop*)" -Rpass-missed="(inline|loop*)" -Rpass-analysis="(inline|loop*)" 
CXXFLAGS += -Rpass=.* -Rpass-missed=.* -Rpass-analysis=.* 
endif

ifeq ($(SANITIZE),1)
CXXFLAGS += -fsanitize=undefined,address -fno-omit-frame-pointer
endif


all: state_finder dust_finder
 
state_finder: state_finder.cpp state_score.hpp
	$(CXX) $(CXXFLAGS) -o $@ state_finder.cpp

dust_finder: dust_finder.cpp state_score.hpp
	$(CXX) $(CXXFLAGS) -o $@ dust_finder.cpp

profile: dust_finder.cpp
	$(CXX) $(CXXFLAGS) -fprofile-instr-generate -o $@ dust_finder.cpp
	./profile 200 0 1000000 > /dev/null
	llvm-profdata-10 merge -output=code.profdata default.profraw
	

opt: profile
	$(CXX) $(CXXFLAGS) -fprofile-instr-use=code.profdata -o $@ dust_finder.cpp

clean:
	rm -f dust_finder state_finder profile opt perf.data perf.data.old code.profdata default.profraw