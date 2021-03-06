# movietool makefile

STD=-std=c99
WFLAGS=-Wall
OPT=-O2
IDIR=-I. -Iinclude
LIBS=utopia pdftool
CC=gcc
NAME=movietool
SRC=*.c

CFLAGS=$(STD) $(WFLAGS) $(OPT) $(IDIR)
OS=$(shell uname -s)

LDIR=lib
LSTATIC=$(patsubst %,lib%.a,$(LIBS))
LPATHS=$(patsubst %,$(LDIR)/%,$(LSTATIC))
LFLAGS=$(patsubst %,-L%,$(LDIR))
LFLAGS += $(patsubst %,-l%,$(LIBS))
LFLAGS += -lz -lxlsxwriter

ifeq ($(OS),Darwin)
	OSFLAGS=-mmacos-version-min=10.9
endif

$(NAME): $(LPATHS) $(SRC)
	$(CC) -o $@ $(SRC) $(CFLAGS) $(LFLAGS) $(OSFLAGS)

$(LPATHS): $(LDIR) $(LSTATIC)
	mv *.a $(LDIR)/

$(LDIR): 
	mkdir $@

$(LDIR)%.a: %
	cd $^ && make && mv $@ ../

clean:
	rm -r $(LDIR) && rm $(NAME)
	
install: $(NAME)
	sudo cp $^ /usr/local/bin/
