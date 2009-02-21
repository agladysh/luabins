# luabins makefile

include Config.make

CFLAGS+=-O2 -Wall

all: $(INCDIR)/$(HNAME) $(LIBDIR)/$(SONAME)

clean: testclean
	$(MAKE) -f Library.make clean TARGET="$(LIBDIR)/$(SONAME)"
	$(RM) $(INCDIR)/$(HNAME)

# Note static library is not installed anywhere
install:
	$(CP) $(LIBDIR)/$(SONAME) $(LUA_LIBDIR)/$(SONAME)

test:
	$(MAKE) -f Test.make test-all LANGUAGE=c STANDARD=c89
	$(MAKE) -f Test.make test-all LANGUAGE=c STANDARD=c99
	$(MAKE) -f Test.make test-all LANGUAGE=c++ STANDARD=c++98 \
	  CC=$(CXX) LD=$(LDXX)
	$(ECHO) "===== TESTS PASSED ====="

testclean:
	$(MAKE) -f Test.make clean LANGUAGE=c STANDARD=c89
	$(MAKE) -f Test.make clean LANGUAGE=c STANDARD=c99
	$(MAKE) -f Test.make clean LANGUAGE=c++ STANDARD=c++98 CC=$(CXX) LD=$(LDXX)

testreset:
	$(MAKE) -f Test.make reset LANGUAGE=c STANDARD=c89
	$(MAKE) -f Test.make reset LANGUAGE=c STANDARD=c99
	$(MAKE) -f Test.make reset LANGUAGE=c++ STANDARD=c++98 CC=$(CXX) LD=$(LDXX)

.PHONY: all clean install test testclean

$(LIBDIR)/$(SONAME):
	$(MAKE) -f Library.make shared \
	CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" \
	TARGET="$(LIBDIR)/$(SONAME)"

$(INCDIR)/$(HNAME): $(SRCDIR)/$(HNAME)
	$(CP) $(SRCDIR)/$(HNAME) $(INCDIR)/$(HNAME)
