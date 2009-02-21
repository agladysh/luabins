include Config.make

TESTNAME= test-$(PROJECTNAME)-$(STANDARD)
SODIR= $(TMPDIR)/$(STANDARD)
ANAME= lib$(PROJECTNAME).a
OBJSUFFIX=-$(STANDARD)

LUATESTSPASSED=$(SODIR)/.luatestspassed
CTESTSPASSED=$(SODIR)/.ctestspassed

CFLAGS+= -Werror -Wall -Wextra -pedantic -x $(LANGUAGE) -std=$(STANDARD)
TESTCFLAGS= -I$(SRCDIR)
TESTLDFLAGS= -lm -llua -l$(PROJECTNAME) -L$(SODIR)

OBJECTS=\
  $(OBJDIR)/test$(OBJSUFFIX).o

clean: reset
	$(MAKE) -f Library.make clean \
	  TARGET="$(SODIR)/$(SONAME)" OBJSUFFIX="$(OBJSUFFIX)"
	$(MAKE) -f Library.make clean \
	  TARGET="$(SODIR)/$(ANAME)" OBJSUFFIX="$(OBJSUFFIX)"
	$(RM) $(OBJECTS)
	$(RMDIR) $(SODIR)
	$(RM) $(BINDIR)/$(TESTNAME)

reset:
	$(RM) $(LUATESTSPASSED)
	$(RM) $(CTESTSPASSED)

test-all: test-lua test-c

test-lua: $(LUATESTSPASSED)

test-c: $(CTESTSPASSED)

$(LUATESTSPASSED): $(SODIR)/$(SONAME) $(TSTDIR)/$(TESTLUA)
	$(ECHO) "===== Running Lua tests for $(STANDARD) ====="
	$(LUA) \
	  -e "package.cpath='$(SODIR)/$(SONAME);'..package.cpath" \
	  $(TSTDIR)/$(TESTLUA)
	$(TOUCH) $(LUATESTSPASSED)
	$(ECHO) "===== Lua tests for $(STANDARD) PASSED ====="

$(CTESTSPASSED): $(BINDIR)/$(TESTNAME)
	$(ECHO) "===== Running C tests for $(STANDARD) ====="
	$(BINDIR)/$(TESTNAME)
	$(TOUCH) $(CTESTSPASSED)
	$(ECHO) "===== C tests for $(STANDARD) PASSED ====="

$(BINDIR)/$(TESTNAME): $(OBJECTS) $(SODIR)/$(ANAME)
	$(LD) $(LDFLAGS) $(TESTLDFLAGS) -o $(BINDIR)/$(TESTNAME) $(OBJECTS)

$(SODIR)/$(SONAME):
	$(MKDIR) $(SODIR)
	$(MAKE) -f Library.make shared \
    CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" \
    TARGET="$(SODIR)/$(SONAME)" OBJSUFFIX="$(OBJSUFFIX)"

$(SODIR)/$(ANAME):
	$(MKDIR) $(SODIR)
	$(MAKE) -f Library.make static \
    CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" \
    TARGET="$(SODIR)/$(ANAME)" OBJSUFFIX="$(OBJSUFFIX)"

.PHONY: clean reset test-all test-lua test-c

-include $(OBJECTS:%.o=%.d)

$(OBJDIR)/test$(OBJSUFFIX).o: \
  $(TSTDIR)/test.c \
  $(SRCDIR)/luabins.h
	$(CC) $(CFLAGS) $(TESTCFLAGS) -o $@ -c $(TSTDIR)/test.c
