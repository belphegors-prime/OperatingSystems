CFLAGS = -c -g -w -std=gnu11

SOURCES= my_malloctest.c my_malloc.c my_malloc.h

OBJECTS=$(SOURCES:.c=.o)

EXECUTABLE=my_malloc

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	gcc $(OBJECTS) $(LDFLAGS) -o $@

.c.o:
	gcc $(CFLAGS) $< -o $@

clean:
	rm -rf *.o *~ $(EXECUTABLE)