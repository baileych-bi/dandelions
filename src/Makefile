# -------------------------------------------------------------------------
# These are configurable options:
# -------------------------------------------------------------------------

# C++ compiler 
CXX = clang++

# Standard flags for C++ 
CXXFLAGS ?= -Wno-everything -std=c++20

# Standard preprocessor flags (common for CC and CXX) 
CPPFLAGS ?= 

# Standard linker flags 
LDFLAGS ?= -pthread -stdlib=libstdc++

# Location and arguments of wx-config script 
WX_CONFIG ?= wx-config

# Port of the wx library to build against [gtk1,gtk2,msw,x11,motif,osx_cocoa,osx_carbon,dfb]
WX_PORT ?= $(shell $(WX_CONFIG) --query-toolkit)

# Use DLL build of wx library to use? [0,1]
WX_SHARED ?= $(shell if test -z `$(WX_CONFIG) --query-linkage`; then echo 1; else echo 0; fi)

# Compile Unicode build of wxWidgets? [0,1]
WX_UNICODE ?= $(shell $(WX_CONFIG) --query-chartype | sed 's/unicode/1/;s/ansi/0/')

# Version of the wx library to build against. 
WX_VERSION ?= $(shell $(WX_CONFIG) --query-version | sed -e 's/\([0-9]*\)\.\([0-9]*\)/\1\2/')

### Variables: ###

CPPDEPS = -MT$@ -MF`echo $@ | sed -e 's,\.o$$,.d,'` -MD -MP
WX_VERSION_MAJOR = $(shell echo $(WX_VERSION) | cut -c1,1)
WX_VERSION_MINOR = $(shell echo $(WX_VERSION) | cut -c2,2)
WX_CONFIG_FLAGS = $(WX_CONFIG_UNICODE_FLAG) $(WX_CONFIG_SHARED_FLAG) \
	--toolkit=$(WX_PORT) --version=$(WX_VERSION_MAJOR).$(WX_VERSION_MINOR)
MINIMAL_CXXFLAGS = -I. `$(WX_CONFIG) --cxxflags $(WX_CONFIG_FLAGS)` $(CPPFLAGS) \
	$(CXXFLAGS)
MINIMAL_OBJECTS =  \
	minimal_minimal.o

### Conditionally set variables: ###

ifeq ($(WX_UNICODE),0)
WX_CONFIG_UNICODE_FLAG = --unicode=no
endif
ifeq ($(WX_UNICODE),1)
WX_CONFIG_UNICODE_FLAG = --unicode=yes
endif
ifeq ($(WX_SHARED),0)
WX_CONFIG_SHARED_FLAG = --static=yes
endif
ifeq ($(WX_SHARED),1)
WX_CONFIG_SHARED_FLAG = --static=no
endif

### Targets: ###

#all: test_for_selected_wxbuild minimal
#
#install: 
#
#uninstall: 
#
#clean: 
#	rm -f ./*.o
#	rm -f ./*.d
#	rm -f minimal
#
#test_for_selected_wxbuild: 
#	@$(WX_CONFIG) $(WX_CONFIG_FLAGS)
#
#minimal: $(MINIMAL_OBJECTS)
#	$(CXX) -o $@ $(MINIMAL_OBJECTS)   $(LDFLAGS)  `$(WX_CONFIG) $(WX_CONFIG_FLAGS) --libs core,base`
#
#minimal_minimal.o: ./minimal.cpp
#	$(CXX) -c -o $@ $(MINIMAL_CXXFLAGS) $(CPPDEPS) $<
#
#.PHONY: all install uninstall clean
#
#
## Dependencies tracking:
#-include ./*.d


#CXXFLAGS = -Wno-everything -std=c++20 -stdlib=libc++ -pthread $(shell wx-config --cxxflags --unicode=yes --static=no --toolkit=gtk3 --version=3.0 --libs core,base)

# Project files
SRCDIR = .
SRCS = main.cpp canvas.cpp main_frame.cpp network.cpp style.cpp tree.cpp main.cpp muttable.cpp parsers.cpp style_editor.cpp util.cpp
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)
EXE = dandelions

# Debug settings
DBGDIR = debug
DBGEXE = $(DBGDIR)/$(EXE)
DBGOBJS = $(addprefix $(DBGDIR)/, $(OBJS))
DBGDEPS = $(addprefix $(DBGDIR)/, $(DEPS))
DBGCXXFLAGS = -g -O0 -DDEBUG

# Release settings
RELDIR = bin
RELEXE = $(RELDIR)/$(EXE)
RELOBJS = $(addprefix $(RELDIR)/, $(OBJS))
RELDEPS = $(addprefix $(RELDIR)/, $(DEPS))
RELCXXFLAGS = -O3 -DNDEBUG

.PHONY: all clean debug doc prep release remake

# Default to release build
all: prep release

# Debug build
debug: $(DBGEXE)

$(DBGEXE): $(DBGOBJS)
	$(CXX) $(LDFLAGS) $^ -o $(DBGEXE) `$(WX_CONFIG) $(WX_CONFIG_FLAGS) --libs core,base,propgrid`

-include $(DBGDEPS)

$(DBGDIR)/%.o: $(SRCDIR)/%.cpp Makefile
	$(CXX) $(MINIMAL_CXXFLAGS) $(CPPDEPS) $(DBGCXXFLAGS) -MMD -MP -c $< -o $@

# Release build
release: $(RELEXE)

$(RELEXE): $(RELOBJS)
	$(CXX) $(LDFLAGS) $^ -o $(RELEXE) `$(WX_CONFIG) $(WX_CONFIG_FLAGS) --libs core,base,propgrid`

-include $(RELDEPS)

$(RELDIR)/%.o: $(SRCDIR)/%.cpp Makefile
	$(CXX) $(MINIMAL_CXXFLAGS) $(CPPDEPS) $(RELCXXFLAGS) -MMD -MP -c $< -o $@

# Misc rules
doc:
	@mkdir -p doc
	doxygen Doxyfile

prep:
	@mkdir -p $(DBGDIR) $(RELDIR)

remake: clean all

clean:
	rm -rf doc/html
	rm -f $(DBGEXE) $(DBGOBJS) $(DBGDEPS) $(RELEXE) $(RELOBJS) $(RELDEPS)
