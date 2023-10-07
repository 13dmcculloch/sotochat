IDIR=inc
ODIR=obj
LDIR=src

TARGET=main

CC=gcc
CFLAGS=-I$(IDIR) -Wall -Wpedantic
LIBS=-lm

_DEPS = network_libs.h server.h client.h Messages.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o network_libs.o server.o client.o Messages.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: $(LDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(IDIR)/*~
