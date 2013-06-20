CC=gcc
CFLAGS=-O3
LDFLAGS=-lv4l2
OBJECT_PATH=obj
OBJECTS=Camera Serial Timer carl
LIBRARY_PATH=lib
LIBRARY=$(LIBRARY_PATH)/libcarl.a
SOURCE_PATH=src

OBJECT_FILEPATHS=$(addprefix $(OBJECT_PATH)/, $(addsuffix .o, $(OBJECTS)))

all: $(LIBRARY) 

$(OBJECT_PATH)/%.o: $(SOURCE_PATH)/%.c
	mkdir -p $(OBJECT_PATH)
	$(CC) -c $(CFLAGS) $< -o $@

$(LIBRARY): $(OBJECT_FILEPATHS)
	mkdir -p $(LIBRARY_PATH)
	ar rcs $(LIBRARY) $(OBJECT_FILEPATHS)

clean:
	rm -rf --preserve-root $(LIBRARY) $(LIBRARY_PATH) $(OBJECTS) $(OBJECT_PATH)
