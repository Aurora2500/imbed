#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "png.h"

void print_usage(char *program)
{
	fprintf(stderr, "Usage:\n"
									"\t%s embed <png file> <data file> <output file>\n"
									"\t%s extract <png file>\n"
									"\t%s probe <png file>\n",
					program, program, program);
}

int main(int argc, char **argv)
{
	(void)argc;
	assert(*argv != NULL);
	char *program = *argv++;

	if (*argv == NULL)
	{
		fprintf(stderr, "ERROR: command is not provided\n");
		print_usage(program);
		exit(1);
	}

	char *command = *argv++;

	if (*argv == NULL)
	{
		fprintf(stderr, "ERROR: file is not provided\n");
		print_usage(program);
		exit(1);
	}

	char *filename = *argv++;
	FILE *file = fopen(filename, "rb");

	if (file == NULL)
	{
		fprintf(stderr, "ERROR: cannot open file %s: %s\n", filename, strerror(errno));
		exit(1);
	}

	if (strcmp(command, "probe") == 0)
	{
		probe_png_file(file);
	}
	else if (strcmp(command, "extract") == 0)
	{
		extract_and_write(file);
	}
	else if (strcmp(command, "embed") == 0)
	{
		char *data_filename = *argv++;
		FILE *data = fopen(data_filename, "rb");
		if (data == NULL)
		{
			fprintf(stderr, "ERROR: cannot open file %s: %s\n", data_filename, strerror(errno));
			exit(1);
		}
		FILE *out = fopen(*argv++, "wb");
		if (out == NULL)
		{
			fprintf(stderr, "ERROR: cannot open file %s: %s\n", filename, strerror(errno));
			exit(1);
		}

		embed_png(file, data_filename, data, out);
	}
	else
	{
		fprintf(stderr, "ERROR: unknown command %s\n", command);
	}

	fclose(file);

	return 0;
}
