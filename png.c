#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include "crc.h"

#define PNG_SIG_LEN 8
const uint8_t png_sig[PNG_SIG_LEN] = {137, 80, 78, 71, 13, 10, 26, 10};

bool is_png(FILE *png)
{
	uint8_t sig[PNG_SIG_LEN];
	fread(sig, 1, PNG_SIG_LEN, png);
	return memcmp(sig, png_sig, PNG_SIG_LEN) == 0;
}

#define IEND 0x49454E44
#define IDAT 0x49444154
#define XDAT 0x78646174

struct chunk
{
	uint32_t length;
	uint32_t type;
	uint8_t *data;
	uint32_t crc;
};

struct chunk *parse_chunk(FILE *png)
{
	struct chunk *chunk = malloc(sizeof(struct chunk));
	if (chunk == NULL)
	{
		fprintf(stderr, "ERROR: cannot allocate memory for chunk: %s\n", strerror(errno));
		exit(1);
	}

	if (fread(&chunk->length, sizeof(uint32_t), 1, png) != 1)
	{
		fprintf(stderr, "ERROR: cannot read chunk length: %s\n", strerror(errno));
		exit(1);
	}

	chunk->length = ntohl(chunk->length);

	if (fread(&chunk->type, sizeof(uint32_t), 1, png) != 1)
	{
		fprintf(stderr, "ERROR: cannot read chunk type: %s\n", strerror(errno));
		exit(1);
	}

	chunk->type = ntohl(chunk->type);

	chunk->data = malloc(chunk->length);
	if (chunk->data == NULL)
	{
		fprintf(stderr, "ERROR: cannot allocate memory for chunk data: %s\n", strerror(errno));
		exit(1);
	}

	if (fread(chunk->data, sizeof(uint8_t), chunk->length, png) != chunk->length)
	{
		fprintf(stderr, "ERROR: cannot read chunk data: %s\n", strerror(errno));
		exit(1);
	}

	if (fread(&chunk->crc, sizeof(uint32_t), 1, png) != 1)
	{
		fprintf(stderr, "ERROR: cannot read chunk crc: %s\n", strerror(errno));
		exit(1);
	}

	chunk->crc = ntohl(chunk->crc);

	return chunk;
}

void write_chunk(struct chunk *chunk, FILE *out)
{
	uint32_t length = htonl(chunk->length);
	fwrite(&length, sizeof(uint32_t), 1, out);

	uint32_t type = htonl(chunk->type);
	fwrite(&type, sizeof(uint32_t), 1, out);

	fwrite(chunk->data, sizeof(uint8_t), chunk->length, out);

	uint32_t crc = htonl(chunk->crc);
	fwrite(&crc, sizeof(uint32_t), 1, out);
}

void free_chunk(struct chunk *chunk)
{
	free(chunk->data);
	free(chunk);
}

void read_buf(uint8_t *buf, FILE *f, size_t len)
{
	size_t n = fread(buf, 1, len, f);
	if (n != len)
	{
		fprintf(stderr, "Error reading %zu bytes: %s", len, strerror(errno));
		exit(1);
	}
}

void probe_png_file(FILE *file)
{
	if (!is_png(file))
	{
		fprintf(stderr, "ERROR: file is not a PNG file\n");
		exit(1);
	}

	// loop trough all the chunks
	while (1)
	{
		unsigned char chunk_length[4];
		read_buf(chunk_length, file, 4);
		uint32_t length = ntohl(*(uint32_t *)chunk_length);
		unsigned char chunk_name[4];
		read_buf(chunk_name, file, 4);
		printf("chunk name: %.4s\nchunk length: %u\n", chunk_name, length);
		uint8_t *chunk_data = malloc(length + 4);
		memcpy(chunk_data, chunk_name, 4);
		read_buf(chunk_data + 4, file, length);
		// fseek(file, length, SEEK_CUR);
		unsigned char chunk_crc[4];
		read_buf(chunk_crc, file, 4);
		uint32_t crc_val = ntohl(*(uint32_t *)chunk_crc);
		printf("chunk crc: %u\n", crc_val);
		uint32_t crc_calc = crc_check(chunk_data, length + 4);
		printf("bytes crc: %u\n\n", crc_calc);
		free(chunk_data);
		if (memcmp(chunk_name, "IEND", 4) == 0)
		{
			break;
		}
	}
}

void extract_and_write(FILE *png)
{
	if (!is_png(png))
	{
		fprintf(stderr, "ERROR: file is not a PNG file\n");
		exit(1);
	}

	// loop trough all the chunks and find the xdat chunk
	while (1)
	{
		struct chunk *chunk = parse_chunk(png);
		if (chunk->type == IEND)
		{
			fprintf(stderr, "ERROR: no xdat chunk found\n");
			exit(1);
		}
		if (chunk->type == XDAT)
		{
			uint32_t total_len = chunk->length;
			uint32_t file_len = ntohl(*(uint32_t *)chunk->data);
			char *filename = malloc(file_len + 1);
			memcpy(filename, chunk->data + 4, file_len);
			filename[file_len] = '\0';
			FILE *out = fopen(filename, "w");
			if (out == NULL)
			{
				fprintf(stderr, "ERROR: cannot open file %s: %s\n", filename, strerror(errno));
				exit(1);
			}
			fwrite(chunk->data + 4 + file_len, 1, total_len - file_len - 4, out);
			fclose(out);
			free(filename);
			free_chunk(chunk);
			break;
		}
		free_chunk(chunk);
	}
}

void embed_png(FILE *png, char *data_filename, FILE *data, FILE *out)
{
	if (!is_png(png))
	{
		fprintf(stderr, "ERROR: file is not a PNG file\n");
		exit(1);
	}

	// write png signature to output file
	fwrite(png_sig, sizeof(uint8_t), 8, out);

	// loop trough all the chunks and write an xdat chunk after the first IDAT chunk
	while (1)
	{
		struct chunk *chunk = parse_chunk(png);
		write_chunk(chunk, out);
		if (chunk->type == IDAT)
		{
			// write xdat chunk
			struct chunk *xdat = malloc(sizeof(struct chunk));
			xdat->type = XDAT;
			uint32_t filename_len = strlen(data_filename);
			fseek(data, 0, SEEK_END);
			uint32_t data_len = ftell(data);
			fseek(data, 0, SEEK_SET);
			xdat->length = filename_len + data_len + 4;
			xdat->data = malloc(xdat->length);
			uint32_t be_filename_len = htonl(filename_len);
			memcpy(xdat->data, &be_filename_len, 4);
			memcpy(xdat->data + 4, data_filename, filename_len);
			fread(xdat->data + 4 + filename_len, 1, data_len, data);
			uint8_t *xdat_data = malloc(xdat->length + 4);
			uint32_t epyt = htonl(xdat->type);
			memcpy(xdat_data, &epyt, 4);
			memcpy(xdat_data + 4, xdat->data, xdat->length);
			uint32_t crc = crc_check(xdat_data, xdat->length + 4);
			xdat->crc = crc;
			write_chunk(xdat, out);
			free_chunk(xdat);
		}
		if (chunk->type == IEND)
		{
			break;
		}
		free_chunk(chunk);
	}
}
