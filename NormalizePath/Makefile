CXX=g++
LXX=ar
INCLUDES=-I"."
CPPFLAGS=-O3 -std=c++17 -pthread -Wall -Wextra -Wno-unknown-pragmas -Wno-missing-field-initializers -pedantic -flto
LIB=-lrt
BINDIR=.
OBJDIR=./obj

$(shell mkdir -p $(BINDIR) $(OBJDIR) >/dev/null)

all: normalize_path

normalize_path: normalize_path.cpp
	$(CXX) $(CPPFLAGS) $(DBGFLAGS) $(CXXFLAGS) normalize_path.cpp -o $(BINDIR)/normalize_path $(LIB) $(INCLUDES)

.PHONY: clean

clean:
	@rm -f $(OBJDIR)/*.o
	@rm -f $(BINDIR)/normalize_path
