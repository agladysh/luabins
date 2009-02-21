include Config.make

OBJSUFFIX=

OBJECTS=\
  $(OBJDIR)/luabins$(OBJSUFFIX).o \
  $(OBJDIR)/luainternals$(OBJSUFFIX).o \
  $(OBJDIR)/load$(OBJSUFFIX).o \
  $(OBJDIR)/save$(OBJSUFFIX).o

shared: $(OBJECTS)
	$(LD) $(LDFLAGS) $(SOFLAGS) -o $(TARGET) $(OBJECTS)

static: $(OBJECTS)
	$(AR) $(TARGET) $(OBJECTS)
	$(RANLIB) $(TARGET)

clean:
	$(RM) $(OBJECTS)
	$(RM) $(TARGET)

.PHONY: objclean

-include $(OBJECTS:%.o=%.d)

$(OBJDIR)/load$(OBJSUFFIX).o: \
  $(SRCDIR)/load.c \
  $(SRCDIR)/luabins.h \
  $(SRCDIR)/saveload.h \
  $(SRCDIR)/luaheaders.h \
  $(SRCDIR)/luainternals.h
	$(CC) $(CFLAGS) -o $@ -c $(SRCDIR)/load.c

$(OBJDIR)/luabins$(OBJSUFFIX).o: \
  $(SRCDIR)/luabins.c \
  $(SRCDIR)/luaheaders.h \
  $(SRCDIR)/luabins.h
	$(CC) $(CFLAGS) -o $@ -c $(SRCDIR)/luabins.c

$(OBJDIR)/luainternals$(OBJSUFFIX).o: \
  $(SRCDIR)/luainternals.c \
  $(SRCDIR)/luaheaders.h \
  $(SRCDIR)/luainternals.h
	$(CC) $(CFLAGS) -o $@ -c $(SRCDIR)/luainternals.c

$(OBJDIR)/save$(OBJSUFFIX).o: \
  $(SRCDIR)/save.c \
  $(SRCDIR)/luaheaders.h \
  $(SRCDIR)/saveload.h
	$(CC) $(CFLAGS) -o $@ -c $(SRCDIR)/save.c
