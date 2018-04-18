# Source lists
LIBNAME=GC_monitoring
SOURCES=GC_monitoring.c
J2SDK=/usr/lib/jvm/java-8-openjdk-amd64
DACAPO=/home/gvaumour/Dev/benchmark/dacapo/dacapo-9.12-bach.jar

# GNU Compiler options needed to build it
COMMON_FLAGS=-fno-strict-aliasing -fPIC -fno-omit-frame-pointer
# Options that help find errors
COMMON_FLAGS+= -W -Wall  -Wno-unused -Wno-parentheses

ifeq ($(OPT), true)
	CFLAGS=-O2 $(COMMON_FLAGS) 
else
	CFLAGS=-g $(COMMON_FLAGS) 
endif

# Object files needed to create library
OBJECTS=$(SOURCES:%.c=%.o)
# Library name and options needed to build it
LIBRARY=lib$(LIBNAME).so
LDFLAGS=-Wl,-soname=$(LIBRARY) -static-libgcc #-mimpure-text
# Libraries we are dependent on
LIBRARIES=-lc
# Building a shared library
LINK_SHARED=$(LINK.c) -shared -o $@

# Common -I options
CFLAGS += -I.
CFLAGS += -I$(J2SDK)/include -I$(J2SDK)/include/linux

# Default rule
all: $(LIBRARY)

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $^

# Build native library
$(LIBRARY): $(OBJECTS)
	$(LINK_SHARED) $(OBJECTS) $(LIBRARIES)



# Cleanup the built bits
clean:
	rm -f $(LIBRARY) $(OBJECTS)

# Simple tester
test: all
	LD_LIBRARY_PATH=`pwd` $(J2SDK)/bin/java -agentlib:$(LIBNAME) -jar $(DACAPO) -v avrora -s large  
