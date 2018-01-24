/*
 * unuprough - Decoder for UpRough music disk files.
 * (C) 2015 Pekka Honkanen aka Rogue / Overflow.
 *
 * The usefulness of this tool is somewhat limited by the fact that you
 * can save the modules by pressing "s" in the EPs themselves, but this
 * was fun to do, so here goes.
 *
 * The decoder function was found by intercepting dos.library Read() calls
 * and examining what happens next. The loaded files are immediately
 * decoded, so the task was very easy. The decoder was extracted and a C
 * language emulator written directly based on this. Also, a decryption /
 * unscrambling key referenced by the decoder was extracted, as well as a
 * table. The key has been processed into C header file "key.h" and the
 * table directly inserted into decode() function.
 *
 * Compile this file and run the the .up files:
 * ./unuprough module01.up module02.up
 * (or just ./unuprough *).
 *
 * The tool will resolve the destination file name by assuming this is a
 * Protracker mod, and contatenating string "mod." with whatever is in the
 * beginning of the file. As some of the modules are in a different format
 * (at least AHX was found), these files will get random file names.
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "key.h"

#define MAXMODLEN 1000000

static void decode(uint8_t *buf, size_t len)
{
	uint8_t *a0, *a1, *a2, *a3;
	uint8_t d1, d2, d3, d4, d5, d6;
	uint32_t d0, d7;
	uint16_t mtab[4] = { 1, 3, 7, 15 };

	d0 = len;
	a0 = buf + len;
	a1 = buf + len;
	a2 = key;
	a3 = key + 0x44;
	d1 = 0x27;
	d2 = 0xd;
	d3 = 0xd;
	d7 = d0;
loop1:
	d4 = *(--a1);
	d5 = a2[d1];
	d2 = a3[d2];
	d4 ^= d5;
	d3 = a3[d3];
	*(--a0) = d4;
	d2 ^= d4;
	d3 ^= d4;
	d2 &= 0x3f;
	d3 >>= 2;
	d1 = d2;
	d1 ^= d3;
	d7--;
	if (d7)
		goto loop1;

	d1 = 0;
	d2 = 0xd;
	d3 = 0xd;
	d4 = 0;
	d5 = 0;
	d6 = 0;
	d0--;

loop2:
	d6 = *a0;
	d7 = d6;
	d6 *= mtab[d4];
	d5 += d6;
	d6 = a2[d1];
	d4++; // d4 += 2;
	d7 ^= d6;
	*(a0++) = d7;
	d4 &= 0x03; // d4 &= 0x06;
	d2 = a3[d2];
	d6 = 0;
	d3 = a3[d3];
	d2 ^= d7;
	d3 ^= d7;
	d2 &= 0x3f;
	d3 >>= 2;
	d1 = d2;
	d1 ^= d3;
	d0--;
	if (d0)
		goto loop2;

	d0 = 1;
	if (d5 != *a0)
		d0 = 0;
}

static void unuprough(char *name)
{
	size_t len;
	FILE *fp;
	uint8_t modbuf[MAXMODLEN];
	char outname[30];

	fp = fopen(name, "rb");
	if (NULL == fp) {
		perror(name);
		return;
	}
	len = fread(modbuf, 1, sizeof(modbuf), fp);
	fclose(fp);

	decode(modbuf, len);

	strcpy(outname, "mod.");
	strncat(outname, modbuf, sizeof(outname) - strlen("mod."));
	outname[sizeof(outname) - 1] = 0;
	fp = fopen(outname, "wb");
	if (NULL == fp) {
		perror(outname);
		return;
	}
	fwrite(modbuf, 1, len - 1, fp);
	fclose(fp);
}

int main(int argc, char **argv)
{
	char *name;

	argv++;
	while ((name = *argv++) != NULL)
		unuprough(name);

	return 0;
}
