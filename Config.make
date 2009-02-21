LUA_DIR= /usr/local
LUA_LIBDIR= $(LUA_DIR)/lib/lua/5.1
LUA_INCDIR= $(LUA_DIR)/include

OSX= $(shell sw_vers | grep -q 'Mac OS X')

PROJECTNAME= luabins

SONAME= $(PROJECTNAME).so
HNAME= $(PROJECTNAME).h
TESTLUA=test.lua

LUA= lua
CP= cp
RM= rm -f
RMDIR= rm -df
MKDIR= mkdir -p
CC= gcc
LD= gcc
AR= ar rcu
RANLIB= ranlib
ECHO= @echo
TOUCH= touch

# Needed for tests only
CXX= g++
LDXX= g++

SRCDIR= ./src
TSTDIR= ./test
OBJDIR= ./obj
TMPDIR= ./tmp
INCDIR= ./include
LIBDIR= ./lib
BINDIR= ./bin

SOFLAGS=
ifeq ($(OSX),)
  SOFLAGS+= -dynamiclib -undefined dynamic_lookup
endif
