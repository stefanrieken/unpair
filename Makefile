OBJECTS=node.o memory.o parse.o print.o primitive.o transform.o eval.o gc.o
CFLAGS=-Wall -Wunused -Os

all: unpair

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

unpair: $(OBJECTS) main.o
	gcc $(CFLAGS) $(OBJECTS) main.o -o unpair

clean:
	rm -rf unpair *.o

