CC=clang
CFLAGS=-Wall -Werror -Wextra -Wpedantic -ggdb


imbed: imbed.o png.o crc.o
	$(CC) -o $@ $^ $(CFLAGS)

imbed.o: imbed.c
	$(CC) -c $< $(CFLAGS)

crc.o: crc.c
	$(CC) -c $< $(CFLAGS)

png.o: png.c
	$(CC) -c $< $(CFLAGS)

clean:
	rm -f imbed imbed.o png.o crc.o