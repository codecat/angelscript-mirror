# Test Framework MingW makefile

CXX = g++
CXXFLAGS = -ggdb -I../../../angelscript/include
SRCDIR = ../../source
OBJDIR = obj

SRCNAMES = \
  main.cpp \
  test_2func.cpp \
  test_circularimport.cpp \
  test_condition.cpp \
  test_constproperty.cpp \
  test_constructor.cpp \
  test_discard.cpp \
  test_funcoverload.cpp \
  test_import.cpp \
  test_multiassign.cpp \
  test_nested.cpp \
  test_nevervisited.cpp \
  test_return_with_cdecl_objfirst.cpp \
  test_saveload.cpp \
  test_suspend.cpp \
  test2modules.cpp \
  testbstr.cpp \
  testbstr2.cpp  \
  testcdecl_class.cpp \
  testcdecl_class_a.cpp \
  testcdecl_class_c.cpp \
  testcdecl_class_d.cpp \
  testcreateengine.cpp \
  testenumglobvar.cpp \
  testexecute.cpp \
  testexecute1arg.cpp \
  testexecute2args.cpp \
  testexecute32args.cpp \
  testexecute32mixedargs.cpp \
  testexecute4args.cpp \
  testexecute4argsf.cpp \
  testexecutemixedargs.cpp \
  testexecutescript.cpp \
  testexecutestring.cpp \
  testexecutethis32mixedargs.cpp \
  testglobalvar.cpp \
  testint64.cpp \
  testlongtoken.cpp \
  testmoduleref.cpp \
  testmultipleinheritance.cpp \
  testnegateoperator.cpp \
  testnotcomplexstdcall.cpp \
  testnotcomplexthiscall.cpp \
  testoutput.cpp \
  testreturn.cpp \
  testreturnd.cpp \
  testreturnf.cpp \
  teststdcall4args.cpp \
  teststdstring.cpp \
  teststack.cpp \
  testswitch.cpp \
  testtempvar.cpp \
  testvirtualinheritance.cpp \
  testvirtualmethod.cpp \
  utils.cpp \
 

OBJ = $(addprefix $(OBJDIR)/, $(notdir $(SRCNAMES:.cpp=.o))) bstr.o stdstr.o
BIN = ../../bin/mingw.exe
OBJ_D = $(subst /,\,$(OBJ))
BIN_D = $(subst /,\,$(BIN))
DELETER = del /f

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) -o $(BIN) $(OBJ) -langelscript -L ../../../angelscript/lib
	@echo -------------------------------------------------------------------
	@echo Done. 

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

bstr.o: ../../../add_on/bstr/bstr.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<
stdstr.o: ../../../add_on/stdstr/stdstr.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	$(DELETER) $(OBJ_D) $(BIN_D)

.PHONY: all clean