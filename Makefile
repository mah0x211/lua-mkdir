TARGET=mkdir.$(LIB_EXTENSION)
SRCS=$(wildcard src/*.c)
OBJS=$(SRCS:.c=.o)
INSTALL?=install

ifdef MKDIR_COVERAGE
COVFLAGS=--coverage
endif

.PHONY: all install

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $(WARNINGS) $(COVFLAGS) $(CPPFLAGS) -o $@ -c $<

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS) $(PLATFORM_LDFLAGS) $(COVFLAGS)

install:
	$(INSTALL) $(TARGET) $(INST_LIBDIR)
	rm -f ./src/*.o
	rm -f ./src/*.so
