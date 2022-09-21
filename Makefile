OBJECTS=memory.o parse.o print.o eval.o
#CFLAGS=-Wall -Wunused -Os

all: unpair

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

unpair: $(OBJECTS) main.o
	gcc $(CFLAGS) $(OBJECTS) main.o -o unpair

clean:
	rm -rf unpair *.o

