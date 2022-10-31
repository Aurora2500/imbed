#include <stdio.h>

void extract_and_write(FILE *png);

void embed_png(FILE *png, char *data_filename, FILE *data, FILE *out);

void probe_png_file(FILE *png);
