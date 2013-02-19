CC=gcc
CFLAGS=-O3
LDFLAGS=-lv4l2 
OBJECTS=Camera.o Serial.o Timer.o carl.o
LIBRARY_PATH=lib
LIBRARY=$(LIBRARY_PATH)/libcarl.a

all: $(LIBRARY) 

.c.o:
	$(CC) -c $(CFLAGS) $<

$(LIBRARY): $(OBJECTS)
	mkdir -p $(LIBRARY_PATH)
	ar rcs $(LIBRARY) $(OBJECTS)

clean:
	rm -rf --preserve-root $(LIBRARY) $(LIBRARY_PATH) $(OBJECTS)
