#pragma once
#ifndef ihex
#define ihex

/* this loads an intel hex file into the memory[] array */
int load_file(char *filename);

/* this is used by load_file to get each line of intex hex */
int parse_hex_line(char *theline, int bytes[], int *addr, int *num, int *code);

int	memory[65536];		/* the memory is global */
int from_addr;
int to_addr;
#endif // !ihex
