
PIN_ROOT=/home/gvaumour/Dev/apps/pin/pin-3.6-97554-g31f0a167d-gcc-linux/

CPP   = g++
FLAGS = -DBIGARRAY_MULTIPLIER=1 -Wno-unknown-pragmas -D__PIN__=1 -DPIN_CRT=1 -fno-stack-protector -fno-exceptions -funwind-tables -fasynchronous-unwind-tables -fno-rtti -DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_LINUX -fabi-version=2  -I$(PIN_ROOT)source/include/pin -I$(PIN_ROOT)source/include/pin/gen -isystem $(PIN_ROOT)extras/stlport/include -isystem $(PIN_ROOT)extras/libstdc++/include -isystem $(PIN_ROOT)extras/crt/include -isystem $(PIN_ROOT)extras/crt/include/arch-x86_64 -isystem $(PIN_ROOT)extras/crt/include/kernel/uapi -isystem $(PIN_ROOT)extras/crt/include/kernel/uapi/asm-x86 -I$(PIN_ROOT)extras/components/include -I$(PIN_ROOT)extras/xed-intel64/include/xed -I$(PIN_ROOT)source/tools/InstLib -O3 -fomit-frame-pointer -fno-strict-aliasing -lz -std=c++11 -I./src/ 

LDFLAGS=-shared -Wl,--hash-style=sysv $(PIN_ROOT)intel64/runtime/pincrt/crtbeginS.o -Wl,-Bsymbolic -Wl,--version-script=$(PIN_ROOT)source/include/pin/pintool.ver -fabi-version=2 -L$(PIN_ROOT)intel64/runtime/pincrt -L$(PIN_ROOT)intel64/lib -L./bin/ -L$(PIN_ROOT)intel64/lib-ext -L$(PIN_ROOT)extras/xed-intel64/lib -lpin -lxed $(PIN_ROOT)intel64/runtime/pincrt/crtendS.o -lpin3dwarf  -ldl-dynamic -nostdlib -lstlport-dynamic -lm-dynamic -lc-dynamic -lunwind-dynamic


PINTOOLS=gc_watch

all : Makefile
	$(foreach tool,$(PINTOOLS), $(MAKE) ./obj-intel64/$(tool).so;)


obj-intel64/%.so : obj-intel64/%.o Makefile
	$(CPP) -o $@ $< $(LDFLAGS)
	
	
obj-intel64/%.o :  %.cc Makefile
	$(CPP) $(FLAGS) -c $< -o $@
	
clean :	
	$(foreach tool,$(PINTOOLS), rm ./obj-intel64/$(tool).*;)

