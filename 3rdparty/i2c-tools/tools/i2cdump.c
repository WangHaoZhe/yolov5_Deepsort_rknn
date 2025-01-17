/*
    i2cdump.c - a user-space program to dump I2C registers
    Copyright (C) 2002-2003  Frodo Looijaard <frodol@dds.nl>, and
                             Mark D. Studebaker <mdsxyz123@yahoo.com>
    Copyright (C) 2004-2022  Jean Delvare <jdelvare@suse.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include "i2cbusses.h"
#include "util.h"
#include "../version.h"

static void help(void)
{
	fprintf(stderr,
		"Usage: i2cdump [-f] [-y] [-r first-last] [-a] I2CBUS ADDRESS [MODE [BANK [BANKREG]]]\n"
		"  I2CBUS is an integer or an I2C bus name\n"
		"  ADDRESS is an integer (0x08 - 0x77, or 0x00 - 0x7f if -a is given)\n"
		"  MODE is one of:\n"
		"    b (byte, default)\n"
		"    w (word)\n"
		"    W (word on even register addresses)\n"
		"    i (I2C block)\n"
		"    c (consecutive byte)\n"
		"    Append p for SMBus PEC\n");
}

static int check_funcs(int file, int size, int pec)
{
	unsigned long funcs;

	/* check adapter functionality */
	if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
		fprintf(stderr, "Error: Could not get the adapter "
			"functionality matrix: %s\n", strerror(errno));
		return -1;
	}

	switch(size) {
	case I2C_SMBUS_BYTE:
		if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus receive byte");
			return -1;
		}
		if (!(funcs & I2C_FUNC_SMBUS_WRITE_BYTE)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus send byte");
			return -1;
		}
		break;

	case I2C_SMBUS_BYTE_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus read byte");
			return -1;
		}
		break;

	case I2C_SMBUS_WORD_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_READ_WORD_DATA)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus read word");
			return -1;
		}
		break;

	case I2C_SMBUS_I2C_BLOCK_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_READ_I2C_BLOCK)) {
			fprintf(stderr, MISSING_FUNC_FMT, "I2C block read");
			return -1;
		}
		break;
	}

	if (pec
	 && !(funcs & (I2C_FUNC_SMBUS_PEC | I2C_FUNC_I2C))) {
		fprintf(stderr, "Warning: Adapter does "
			"not seem to support PEC\n");
	}

	return 0;
}

int main(int argc, char *argv[])
{
	char *end;
	int i, j, res, i2cbus, address, size, file;
	int bank = 0, bankreg = 0x4E, old_bank = 0;
	char filename[20];
	int block[256];
	int pec = 0, even = 0;
	int opt;
	int force = 0, yes = 0, version = 0, all_addrs = 0;
	const char *range = NULL;
	int first = 0x00, last = 0xff;

	/* handle (optional) flags first */
	while ((opt = getopt(argc, argv, "Vafhr:y")) != -1) {
		switch (opt) {
		case 'V': version = 1; break;
		case 'f': force = 1; break;
		case 'r': range = optarg; break;
		case 'y': yes = 1; break;
		case 'a': all_addrs = 1; break;
		case 'h':
		case '?':
			help();
			exit(opt == '?');
		}
	}

	if (version) {
		fprintf(stderr, "i2cdump version %s\n", VERSION);
		exit(0);
	}

	if (argc < optind + 1) {
		fprintf(stderr, "Error: No i2c-bus specified!\n");
		help();
		exit(1);
	}
	i2cbus = lookup_i2c_bus(argv[optind]);
	if (i2cbus < 0) {
		help();
		exit(1);
	}

	if (argc < optind + 2) {
		fprintf(stderr, "Error: No address specified!\n");
		help();
		exit(1);
	}
	address = parse_i2c_address(argv[optind+1], all_addrs);
	if (address < 0) {
		help();
		exit(1);
	}

	if (argc < optind + 3) {
		fprintf(stderr, "No size specified (using byte-data access)\n");
		size = I2C_SMBUS_BYTE_DATA;
	} else if (!strncmp(argv[optind+2], "b", 1)) {
		size = I2C_SMBUS_BYTE_DATA;
		pec = argv[optind+2][1] == 'p';
	} else if (!strncmp(argv[optind+2], "w", 1)) {
		size = I2C_SMBUS_WORD_DATA;
		pec = argv[optind+2][1] == 'p';
	} else if (!strncmp(argv[optind+2], "W", 1)) {
		size = I2C_SMBUS_WORD_DATA;
		even = 1;
	} else if (!strncmp(argv[optind+2], "s", 1)) {
		fprintf(stderr,
			"SMBus block mode is no longer supported, please use i2cget instead\n");
		exit(1);
	} else if (!strncmp(argv[optind+2], "c", 1)) {
		size = I2C_SMBUS_BYTE;
		pec = argv[optind+2][1] == 'p';
	} else if (!strcmp(argv[optind+2], "i"))
		size = I2C_SMBUS_I2C_BLOCK_DATA;
	else {
		fprintf(stderr, "Error: Invalid mode!\n");
		help();
		exit(1);
	}

	if (argc > optind + 3) {
		bank = strtol(argv[optind+3], &end, 0);
		if (*end || size == I2C_SMBUS_I2C_BLOCK_DATA) {
			fprintf(stderr, "Error: Invalid bank number!\n");
			help();
			exit(1);
		}
		if ((size == I2C_SMBUS_BYTE_DATA || size == I2C_SMBUS_WORD_DATA)
		 && (bank < 0 || bank > 15)) {
			fprintf(stderr, "Error: bank out of range!\n");
			help();
			exit(1);
		}

		if (argc > optind + 4) {
			bankreg = strtol(argv[optind+4], &end, 0);
			if (*end) {
				fprintf(stderr, "Error: Invalid bank register "
					"number!\n");
				help();
				exit(1);
			}
			if (bankreg < 0 || bankreg > 0xff) {
				fprintf(stderr, "Error: bank out of range "
					"(0-0xff)!\n");
				help();
				exit(1);
			}
		}
	}

	/* Parse optional range string */
	if (range) {
		char *dash;

		first = strtol(range, &dash, 0);
		if (dash == range || *dash != '-'
		 || first < 0 || first > 0xff) {
			fprintf(stderr, "Error: Invalid range parameter!\n");
			exit(1);
		}
		last = strtol(++dash, &end, 0);
		if (end == dash || *end != '\0'
		 || last < first || last > 0xff) {
			fprintf(stderr, "Error: Invalid range parameter!\n");
			exit(1);
		}

		/* Check mode constraints */
		if (size == I2C_SMBUS_WORD_DATA && even && (first%2 || !(last%2))) {
			fprintf(stderr,
				"Error: Range parameter not compatible with selected mode!\n");
			exit(1);
		}
	}

	file = open_i2c_dev(i2cbus, filename, sizeof(filename), 0);
	if (file < 0
	 || check_funcs(file, size, pec)
	 || set_slave_addr(file, address, force))
		exit(1);

	if (pec) {
		if (ioctl(file, I2C_PEC, 1) < 0) {
			fprintf(stderr, "Error: Could not set PEC: %s\n",
				strerror(errno));
			exit(1);
		}
	}

	if (!yes) {
		fprintf(stderr, "WARNING! This program can confuse your I2C "
			"bus, cause data loss and worse!\n");

		fprintf(stderr, "I will probe file %s, address 0x%x, mode "
			"%s\n", filename, address,
			size == I2C_SMBUS_I2C_BLOCK_DATA ? "i2c block" :
			size == I2C_SMBUS_BYTE ? "byte consecutive read" :
			size == I2C_SMBUS_BYTE_DATA ? "byte" : "word");
		if (pec)
			fprintf(stderr, "PEC checking enabled.\n");
		if (even)
			fprintf(stderr, "Only probing even register "
				"addresses.\n");
		if (bank) {
			fprintf(stderr, "Probing bank %d using bank "
				"register 0x%02x.\n", bank, bankreg);
		}
		if (range) {
			fprintf(stderr,
				"Probe range limited to 0x%02x-0x%02x.\n",
				first, last);
		}

		fprintf(stderr, "Continue? [Y/n] ");
		fflush(stderr);
		if (!user_ack(1)) {
			fprintf(stderr, "Aborting on user request.\n");
			exit(0);
		}
	}

	/* See Winbond w83781d data sheet for bank details */
	if (bank) {
		res = i2c_smbus_read_byte_data(file, bankreg);
		if (res >= 0) {
			old_bank = res;
			res = i2c_smbus_write_byte_data(file, bankreg,
				bank | (old_bank & 0xf0));
		}
		if (res < 0) {
			fprintf(stderr, "Error: Bank switching failed\n");
			exit(1);
		}
	}

	/* handle all but word data */
	if (size != I2C_SMBUS_WORD_DATA || even) {
		/* do the block transaction */
		if (size == I2C_SMBUS_I2C_BLOCK_DATA) {
			unsigned char cblock[288];

			for (res = first; res <= last; res += i) {
				i = i2c_smbus_read_i2c_block_data(file,
					res, 32, cblock + res);
				if (i <= 0) {
					res = i;
					break;
				}
			}
			if (res <= 0) {
				fprintf(stderr, "Error: Block read failed, "
					"return code %d\n", res);
				exit(1);
			}
			for (i = first; i <= last; i++)
				block[i] = cblock[i];
		}

		if (size == I2C_SMBUS_BYTE) {
			res = i2c_smbus_write_byte(file, first);
			if(res != 0) {
				fprintf(stderr, "Error: Write start address "
					"failed, return code %d\n", res);
				exit(1);
			}
		}

		printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f"
		       "    0123456789abcdef\n");
		for (i = 0; i < 256; i+=16) {
			if (i/16 < first/16)
				continue;
			if (i/16 > last/16)
				break;

			printf("%02x: ", i);
			for (j = 0; j < 16; j++) {
				fflush(stdout);
				/* Skip unwanted registers */
				if (i+j < first || i+j > last) {
					printf("   ");
					if (size == I2C_SMBUS_WORD_DATA) {
						printf("   ");
						j++;
					}
					continue;
				}

				if (size == I2C_SMBUS_BYTE_DATA) {
					block[i+j] = res =
					  i2c_smbus_read_byte_data(file, i+j);
				} else if (size == I2C_SMBUS_WORD_DATA) {
					res = i2c_smbus_read_word_data(file,
								       i+j);
					if (res < 0) {
						block[i+j] = res;
						block[i+j+1] = res;
					} else {
						block[i+j] = res & 0xff;
						block[i+j+1] = res >> 8;
					}
				} else if (size == I2C_SMBUS_BYTE) {
					block[i+j] = res =
					  i2c_smbus_read_byte(file);
				} else
					res = block[i+j];

				if (res < 0) {
					printf("XX ");
					if (size == I2C_SMBUS_WORD_DATA)
						printf("XX ");
				} else {
					printf("%02x ", block[i+j]);
					if (size == I2C_SMBUS_WORD_DATA)
						printf("%02x ", block[i+j+1]);
				}
				if (size == I2C_SMBUS_WORD_DATA)
					j++;
			}
			printf("   ");

			for (j = 0; j < 16; j++) {
				/* Skip unwanted registers */
				if (i+j < first || i+j > last) {
					printf(" ");
					continue;
				}

				res = block[i+j];
				if (res < 0)
					printf("X");
				else
				if ((res & 0xff) == 0x00
				 || (res & 0xff) == 0xff)
					printf(".");
				else
				if ((res & 0xff) < 32
				 || (res & 0xff) >= 127)
					printf("?");
				else
					printf("%c", res & 0xff);
			}
			printf("\n");
		}
	} else {
		printf("     0,8  1,9  2,a  3,b  4,c  5,d  6,e  7,f\n");
		for (i = 0; i < 256; i+=8) {
			if (i/8 < first/8)
				continue;
			if (i/8 > last/8)
				break;

			printf("%02x: ", i);
			for (j = 0; j < 8; j++) {
				/* Skip unwanted registers */
				if (i+j < first || i+j > last) {
					printf("     ");
					continue;
				}

				res = i2c_smbus_read_word_data(file, i+j);
				if (res < 0)
					printf("XXXX ");
				else
					printf("%04x ", res & 0xffff);
			}
			printf("\n");
		}
	}
	if (bank) {
		i2c_smbus_write_byte_data(file, bankreg, old_bank);
	}
	exit(0);
}
