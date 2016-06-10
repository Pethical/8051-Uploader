#include "stdafx.h"
#include <stdio.h>
#include <string.h>

#define _CRT_SECURE_NO_WARNINGS 1
#define _CRT_OBSOLETE_NO_WARNINGS 1
#define _CRT_NONSTDC_NO_WARNINGS 1

extern int	memory[65536];	

extern int minaddr = 65536;
extern int maxaddr = 0;

extern int from_addr;
extern int to_addr;

int parse_hex_line(char * theline, int bytes[], int * addr, int *num, int *code)
{
	int sum, len, cksum;
	char *ptr;

	*num = 0;
	if (theline[0] != ':') return 0;
	if (strlen(theline) < 11) return 0;
	ptr = theline + 1;
	if (!sscanf_s(ptr, "%02x", &len)) return 0;
	ptr += 2;
	if (strlen(theline) < (11 + (len * 2))) return 0;
	if (!sscanf_s(ptr, "%04x", addr)) return 0;
	ptr += 4;
	/* printf("Line: length=%d Addr=%d\n", len, *addr); */
	if (!sscanf_s(ptr, "%02x", code)) return 0;
	ptr += 2;
	sum = (len & 255) + ((*addr >> 8) & 255) + (*addr & 255) + (*code & 255);
	while (*num != len) {
		if (!sscanf_s(ptr, "%02x", &bytes[*num])) return 0;
		ptr += 2;
		sum += bytes[*num] & 255;
		(*num)++;
		if (*num >= 256) return 0;
	}
	if (!sscanf_s(ptr, "%02x", &cksum)) return 0;
	if (((sum & 255) + (cksum & 255)) & 255) return 0; /* checksum error */
	return 1;
}

/* loads an intel hex file into the global memory[] array */
/* filename is a string of the file to be opened */

int load_file(char *filename)
{
	char line[1000];
	FILE *fin;
	int addr, n, status, bytes[256];
	int i, total = 0, lineno = 1;
	

	fopen_s(&fin, filename, "r");
	if (fin == NULL) {
		printf("   Can't open file '%s' for reading.\n", filename);
		return 0;
	}
	while (!feof(fin) && !ferror(fin)) {
		line[0] = '\0';
		fgets(line, 1000, fin);
		if (line[strlen(line) - 1] == '\n') line[strlen(line) - 1] = '\0';
		if (line[strlen(line) - 1] == '\r') line[strlen(line) - 1] = '\0';
		if (parse_hex_line(line, bytes, &addr, &n, &status)) {
			if (status == 0) {  /* data */
				for (i = 0; i <= (n - 1); i++) {
					memory[addr] = bytes[i] & 255;
					total++;
					if (addr < minaddr) minaddr = addr;
					if (addr > maxaddr) maxaddr = addr;
					addr++;
				}
			}
			if (status == 1) {  /* end of file */
				fclose(fin);
				from_addr = minaddr;
				to_addr = maxaddr;
				printf("   Loaded %d bytes between:", total);
				printf(" %04X to %04X\n", minaddr, maxaddr);
				return maxaddr;
			}
			if (status == 2);  /* begin of file */
		}
		else {
			printf("   Error: '%s', line: %d\n", filename, lineno);
		}
		lineno++;
	}
	return 0;

}


