# Tutoral GCC Makefile

CXX = g++
CXXFLAGS = -ggdb -I../../../../angelscript/include -D_LINUX_
SRCDIR = ../../source
OBJDIR = obj

SRCNAMES = \
  main.cpp

OBJ = $(addprefix $(OBJDIR)/, $(notdir $(SRCNAMES:.cpp=.o))) obj/scriptstring.o
BIN = ../../bin/console
DELETER = rm -f

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) -o $(BIN) $(OBJ) -langelscript -L ../../../../angelscript/lib
	@echo -------------------------------------------------------------------
	@echo Done.

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/scriptstring.o: ../../../../add_on/scriptstring/scriptstring.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	$(DELETER) $(OBJ) $(BIN)

.PHONY: all clean
