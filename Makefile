CC=gcc

CFLAGS=-O3 -Wall -pedantic -std=c99 -D _BSD_SOURCE
LDFLAGS=-lv4l2 -lbsd-compat
OBJECTS=Camera Serial Timer carl time
LIBRARY=$(LIBRARY_PATH)/libcarl.a

INCLUDE_PATH=inc/carl
OBJECT_PATH=obj
SOURCE_PATH=src
LIBRARY_PATH=lib

#----- Automatic machinery -----#
LIBRARY=$(LIBRARY_PATH)/lib$(LIBRARY_NAME).a
OBJECT_FILEPATHS=$(addprefix $(OBJECT_PATH)/, $(addsuffix .o, $(OBJECTS)))

all: $(LIBRARY) 

$(OBJECT_PATH)/%.o: $(SOURCE_PATH)/%.c
	mkdir -p $(OBJECT_PATH)
	$(CC) -c $(CFLAGS) $< -o $@

$(LIBRARY): $(OBJECT_FILEPATHS)
	mkdir -p $(LIBRARY_PATH)
	ar rcs $(LIBRARY) $(OBJECT_FILEPATHS)
	mkdir -p $(INCLUDE_PATH)
	cp $(SOURCE_PATH)/*.h $(INCLUDE_PATH)

clean:
	rm -rf --preserve-root $(LIBRARY) $(LIBRARY_PATH) $(OBJECTS) $(OBJECT_PATH)
