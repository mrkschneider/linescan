include Makefile.env
PROJNAME=linescan
INCLUDEDIRS=include 
LIBDIRS=
LIBNAMES=
OUTLIBDIR=lib
OUTLIBNAME_DEBUG=$(PROJNAME).debug
OUTLIBNAME_OPT=$(PROJNAME)
SRCDIR=src
TEST_SRCDIR=test
TEST_DEBUG_OUTDIR=test_out_debug
TEST_OPT_OUTDIR=test_out_opt
TEST_BIN=testrunner
OUTBIN=main.out
SHELL=/bin/bash
SRC_EXTENSION=.c

CC=gcc
CPP=g++
SHAREDFLAGS=-g3 -Wall -Werror -pedantic
CFLAGS=-std=c11 $(SHAREDFLAGS)
CPPFLAGS=-std=c++14 $(SHAREDFLAGS)
PROFILEFLAGS=--coverage
DEBUGFLAGS=-DLINESCAN_DEBUG -O0
OPTIMIZEFLAGS=-O3

TEST_INCLUDEDIRS=$(INCLUDEDIRS) $(CXXTESTDIR)
TEST_LIBDIRS=$(LIBDIRS) $(OUTLIBDIR)
TEST_DEBUG_LIBNAMES=$(OUTLIBNAME_DEBUG) $(LIBNAMES) 
TEST_OPT_LIBNAMES=$(OUTLIBNAME_OPT) $(LIBNAMES) 

OBJDIR_DEBUG=obj_debug
OBJDIR_OPT=obj
SOURCES=$(shell find $(SRCDIR) -type f -name "*$(SRC_EXTENSION)")
TEST_SOURCES=$(shell find $(TEST_SRCDIR) -type f -name "*test.hpp")

OBJECTS_DEBUG=$(addprefix $(OBJDIR_DEBUG)/,$(SOURCES:$(SRC_EXTENSION)=.o))
OBJECTS_OPT=$(addprefix $(OBJDIR_OPT)/,$(SOURCES:$(SRC_EXTENSION)=.o))
INCLUDELINE=$(addprefix -I,$(INCLUDEDIRS))
LIBLINE=$(addprefix -L,$(LIBDIRS))
LIBNAMELINE=$(addprefix -l,$(LIBNAMES))

TEST_INCLUDELINE=$(addprefix -I,$(TEST_INCLUDEDIRS))
TEST_LIBLINE=$(addprefix -L,$(TEST_LIBDIRS))
TEST_DEBUG_LIBNAMELINE=$(addprefix -l,$(TEST_DEBUG_LIBNAMES))
TEST_OPT_LIBNAMELINE=$(addprefix -l,$(TEST_OPT_LIBNAMES))

default: debug opt

$(OBJECTS_DEBUG): $(OBJDIR_DEBUG)/%.o: %$(SRC_EXTENSION)
	@[ -d $@ ] || mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $(DEBUGFLAGS) $(PROFILEFLAGS) $(INCLUDELINE) $(LIBLINE) -o $@ $< $(LIBNAMELINE)

$(OBJECTS_OPT): $(OBJDIR_OPT)/%.o: %$(SRC_EXTENSION)
	@[ -d $@ ] || mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $(OPTIMIZEFLAGS) $(INCLUDELINE) $(LIBLINE) -o $@ $< $(LIBNAMELINE)

debug: clean_profile $(OBJECTS_DEBUG)
	ar rcs $(OUTLIBDIR)/lib$(OUTLIBNAME_DEBUG).a $(OBJECTS_DEBUG)

opt: $(OBJECTS_OPT)
	ar rcs $(OUTLIBDIR)/lib$(OUTLIBNAME_OPT).a $(OBJECTS_OPT)

test: clean_test debug
	cxxtestgen --error-printer -o $(TEST_DEBUG_OUTDIR)/tests.cpp $(TEST_SOURCES)
	$(CPP) $(CPPFLAGS) $(DEBUGFLAGS) $(PROFILEFLAGS) $(TEST_INCLUDELINE) $(TEST_LIBLINE) -o $(TEST_DEBUG_OUTDIR)/tests.o $(TEST_DEBUG_OUTDIR)/tests.cpp $(TEST_DEBUG_LIBNAMELINE)
	$(TEST_DEBUG_OUTDIR)/tests.o -v

coverage: test
	lcov -q --capture --directory . --output-file coverage.info --no-external --exclude '*/$(TEST_DEBUG_OUTDIR)/*' --exclude '*/$(TEST_SRCDIR)/*'
	genhtml coverage.info -q --output-directory coverage

test_opt: clean_test opt
	cxxtestgen --error-printer -o $(TEST_OPT_OUTDIR)/tests.cpp $(TEST_SOURCES)
	$(CPP) $(CPPFLAGS) $(OPTIMIZEFLAGS) $(TEST_INCLUDELINE) $(TEST_LIBLINE) -o $(TEST_OPT_OUTDIR)/tests.o $(TEST_OPT_OUTDIR)/tests.cpp $(TEST_OPT_LIBNAMELINE)
	$(TEST_OPT_OUTDIR)/tests.o -v

clean_profile:
	rm -f *.info
	rm -rf coverage
	rm -f tests.gcda tests.gcno

clean_test: clean_profile
	rm -f $(TEST_DEBUG_OUTDIR)/*.o $(TEST_DEBUG_OUTDIR)/tests.cpp
	rm -f $(TEST_OPT_OUTDIR)/*.o $(TEST_OPT_OUTDIR)/tests.cpp

clean: clean_profile clean_test
	rm -rf $(OBJDIR_DEBUG) $(OBJDIR_OPT)
	rm -f lib/*.a

all: clean debug opt test coverage test_opt
