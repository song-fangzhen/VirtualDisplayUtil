TARGET = .
CC = g++
CFLAGS = -g
CFLAGC = -c

TEST = virtual_display_util_test.cpp
SRC = virtual_display_util.cpp
OBJ = virtual_display_util.o

INCLUDE = -I$(TARGET)
LIBS = -lX11 -lXrandr
EXEC = $(TARGET)/virtual_display_util_test

all: $(EXEC)

$(EXEC): $(OBJ) $(TEST)
	$(CC) $(CFLAGS) $(OBJ) $(TEST) $(INCLUDE) $(LIBS) -o $@
	rm -f $(OBJ)
	@echo "<<<<<< $@ is created successfully! >>>>>>"

$(OBJ): $(SRC)
	$(CC) $(CFLAGC) $(SRC) -o $@

clean:
	rm -f $(EXEC)
