LDFLAGS+= -DUSE_LIBPNG -lpng

image_operations: image_operations.o
	g++ -I /usr/include -I/usr/include/libpng16 -L/usr/lib/x86_64-linux-gnu -o image_operations image_operations.o -lpng -lfftw

image_operations.o: image_operations.cpp
	g++ -c image_operations.cpp

.phony clean:
	rm image_operations image_operations.o
