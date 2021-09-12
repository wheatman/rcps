CFLAGS := -Wall -Wextra -O3 -g  -std=c++17   -march=native

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

clean:
	rm -f rcps