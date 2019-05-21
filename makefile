UNAME := $(shell uname)

EXE                    = hypernomad.exe

COMPILATOR             = g++

COMPILATOR_OPTIONS     = -std=c++14  

LIB_DIR                = $(NOMAD_HOME)/lib
LIB_NOMAD              = libnomad.so 

CXXFLAGS               =           
ifeq ($(UNAME), Linux)
CXXFLAGS              += -Wl,-rpath,'$(LIB_DIR)'
CXXFLAGS              += -ansi
endif


LDLIBS                 = -lm -lnomad

INCLUDE                = -I$(NOMAD_HOME)/src -I$(NOMAD_HOME)/ext/sgtelib/src -I.

COMPILE                = $(COMPILATOR) $(COMPILATOR_OPTIONS) $(INCLUDE) -c

SRC		       = src/nomad_optimizer

OBJS                   = hypernomad.o hyperParameters.o


ifndef NOMAD_HOME
define ECHO_NOMAD
	@echo Please set NOMAD_HOME environment variable!
	@false
endef
endif


$(EXE): $(OBJS)
	$(ECHO_NOMAD)
	@echo "   building the scalar version ..."
	@echo "   exe file : "$(EXE)
	@$(COMPILATOR) -o $(EXE) $(OBJS) $(LDLIBS) $(CXXFLAGS) -L$(LIB_DIR) 
ifeq ($(UNAME), Darwin)
	@install_name_tool -change $(LIB_NOMAD) $(NOMAD_HOME)/lib/$(LIB_NOMAD) $(EXE)
endif
	@echo "   cleaning obj files"
	@rm -f $(OBJS)

hypernomad.o: $(SRC)/hypernomad.cpp $(SRC)/hyperParameters.hpp
	$(ECHO_NOMAD)
	@$(COMPILE) $(SRC)/hypernomad.cpp

hyperParameters.o: $(SRC)/hyperParameters.cpp $(SRC)/hyperParameters.hpp
	$(ECHO_NOMAD)
	@$(COMPILE) $(SRC)/hyperParameters.cpp

all: $(EXE)

clean: ;
	@echo "   cleaning obj files"
	@rm -f $(OBJS)

del: ;
	@echo "   cleaning trash files"
	@rm -f core *~
	@echo "   cleaning obj files"
	@rm -f $(OBJS) 
	@echo "   cleaning exe file"
	@rm -f $(EXE) 


