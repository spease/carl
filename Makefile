CC=gcc
CFLAGS=-O3
LDFLAGS=-lv4l2 
OBJECTS=Camera.o Serial.o Timer.o carl.o
LIBRARY=lib/libcarl.a

all: $(LIBRARY) 

.c.o:
	$(CC) -c $(CFLAGS) $<

$(LIBRARY): $(OBJECTS)
	ar rcs $(LIBRARY) $(OBJECTS)

clean:
	rm -rf --preserve-root $(LIBRARY) $(OBJECTS)
