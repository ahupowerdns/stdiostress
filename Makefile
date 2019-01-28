CXXFLAGS:=-std=gnu++17 -Wall -O2 -MMD -MP -ggdb -Iext/ -pthread  -Iext/simplesocket/ext/fmt-5.2.1/include/ -Wno-reorder -Iext/simplesocket
CFLAGS:= -Wall -O2 -MMD -MP -ggdb 

PROGRAMS = stdiostress

all: $(PROGRAMS)

clean:
	rm -f *~ *.o *.d test $(PROGRAMS)

-include *.d


stdiostress: stdiostress.o ext/simplesocket/comboaddress.o ext/simplesocket/sclasses.o ext/simplesocket/swrappers.o ext/simplesocket/ext/fmt-5.2.1/src/format.o
	g++ -std=gnu++17 $^ -o $@ -pthread 

