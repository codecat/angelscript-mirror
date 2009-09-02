# Angelscript makefile for linux (based on MingW makefile)
# Type 'make' then 'make install' to complete the installation of the library

# For 'make install' to work, set LOCAL according to your system configuration
LOCAL = /usr/local

# If you want to build a shared library, then run make with SHARED=1 and VERSION=version
ifdef SHARED
LIB = libangelscript-$(VERSION).so
DEVLIB = libangelscript.so
else
LIB = libangelscript.a
endif
INC = angelscript.h

SRCDIR = ../../source
LIBDIR = ../../lib
INCDIR = ../../include
OBJDIR = obj

CXX = gcc

# On i686 architecture you may need to add -march=i686 if you get 
# an undefined symbol for __sync_sub_and_fetch_4 in as_atomic.cpp.
CXXFLAGS = -g -Wall -fPIC
DELETER = rm -f
COPIER = cp -a

# If you're targetting an ARM processor, you will need to 
# include the as_callfunc_armasm.s file in the source files.

SRCNAMES = \
  as_arrayobject.cpp \
  as_atomic.cpp \
  as_builder.cpp  \
  as_bytecode.cpp \
  as_callfunc.cpp \
  as_callfunc_mips.cpp \
  as_callfunc_ppc.cpp \
  as_callfunc_ppc_64.cpp \
  as_callfunc_sh4.cpp \
  as_callfunc_x86.cpp \
  as_callfunc_x64_gcc.cpp \
  as_compiler.cpp \
  as_context.cpp \
  as_configgroup.cpp \
  as_datatype.cpp \
  as_generic.cpp \
  as_gc.cpp \
  as_memory.cpp \
  as_module.cpp \
  as_objecttype.cpp \
  as_outputbuffer.cpp \
  as_parser.cpp \
  as_restore.cpp \
  as_scriptcode.cpp \
  as_scriptengine.cpp \
  as_scriptfunction.cpp \
  as_scriptnode.cpp \
  as_scriptobject.cpp \
  as_string.cpp \
  as_string_util.cpp \
  as_thread.cpp \
  as_tokenizer.cpp \
  as_typeinfo.cpp \
  as_variablescope.cpp \

OBJ = $(addprefix $(OBJDIR)/, $(notdir $(SRCNAMES:.cpp=.o)))
BIN = $(LIBDIR)/$(LIB)

all: $(BIN)

$(BIN): $(OBJ)
ifdef SHARED
	$(CXX) -shared -Wl,-soname,$(LIB) -o $(BIN) $(OBJ)
	( cd $(LIBDIR); ln -s $(LIB) $(DEVLIB) )
else
	ar r $(BIN) $(OBJ)
	ranlib $(BIN)
endif
	@echo -------------------------------------------------------------------
	@echo Done. As root, type 'make install' to install the library.

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<


clean:
	$(DELETER) $(OBJ) $(BIN) $(LIBDIR)/$(DEVLIB)

install: $(BIN)
	@echo Installing to: $(LOCAL)/lib and $(LOCAL)/include...
	@echo -------------------------------------------------------------------
ifdef SHARED
	$(COPIER) $(LIBDIR)/$(DEVLIB) $(LOCAL)/lib
endif
	$(COPIER) $(BIN) $(LOCAL)/lib
	$(COPIER) $(INCDIR)/$(INC) $(LOCAL)/include
	@echo -------------------------------------------------------------------
	@echo Angelscript library installed. Enjoy!

uninstall:
	$(DELETER) $(LOCAL)/include/$(INC) $(LOCAL)/lib/$(LIB) $(LOCAL)/lib/$(DEVLIB)
	@echo -------------------------------------------------------------------
	@echo Angelscript library uninstalled.

.PHONY: all clean install uninstall
