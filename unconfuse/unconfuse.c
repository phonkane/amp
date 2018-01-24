/*
 * unconfuse - Decoder for Confusion music disk modules.
 * (C) 2014 Pekka Honkanen aka Rogue / Overflow.
 *
 * Using UAE 0.8.29 with following settings:
 *  - kickstart 1.3
 *  - 1MB chip mem, no other mem
 *  - 68000 CPU, ECS/PAL
 * it was discovered that the replay code decodes pattern data on the fly,
 * and recodes it after use again. The module is loaded to address $148000
 * (actually aliases to physical address $48000). Setting a memory write
 * watch to pattern data (starting at $14843c), writes were found within a
 * decoder function found at 0x1209dc. This is just a XOR with 1024-byte long
 * encryption key. The key is at 0x1217e0 and actually is part of the code.
 *
 * So, to decode the patterns, save the encryption key to file named "key"
 * with UAE debugger:
 *
 * >S key 1217e0 400
 *
 * Compile this file and run on the mods:
 * ./unconfuse mod.lost mod.x ...
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <netinet/in.h>

#define KEYSIZE 1024
#define MAXMODSIZE 400000

#define PAT_LIST_OFFSET 0x3b8
#define MK_OFFSET 0x438
#define PAT_DATA_OFFSET 0x43c
#define PAT_SIZE 1024

static void unconfuse(const uint8_t *key, const char *filename)
{
	int fd;
	uint8_t *mod, *p;
	int maxpat = 0;
	int pos, pat;

	fd = open(filename, O_RDWR);
	if (-1 == fd) {
		perror(filename);
		return;
	}
	mod = mmap(0, MAXMODSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (MAP_FAILED == mod) {
		perror(filename);
		close(fd);
		return;
	}
	mod[MK_OFFSET] = 'M';
	mod[MK_OFFSET + 2] = 'K';
	for (pos = PAT_LIST_OFFSET; pos < MK_OFFSET; pos++)
		if (maxpat < mod[pos])
			maxpat = mod[pos];
	for (pat = 0; pat <= maxpat; pat++)
		for (p = mod + PAT_DATA_OFFSET + pat * PAT_SIZE, pos = 0;
		     pos < PAT_SIZE; pos++)
			p[pos] ^= key[pos];

	munmap(mod, MAXMODSIZE);
	close(fd);
}

int main(int argc, char **argv)
{
	int fd;
	uint8_t *key = 0;
	char *filename;

	fd = open("key", O_RDONLY);
	if (-1 == fd) {
		perror("key");
		exit(EXIT_FAILURE);
	}
	key = mmap(0, KEYSIZE, PROT_READ, MAP_PRIVATE, fd, 0);
	if (MAP_FAILED == key) {
		perror("key");
		close(fd);
		exit(EXIT_FAILURE);
	}

	argv++;
	while (NULL != (filename = *argv++))
		unconfuse(key, filename);

	munmap(key, KEYSIZE);
	close(fd);
	return 0;
}
