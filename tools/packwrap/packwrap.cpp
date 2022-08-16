
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <ctype.h>

//#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__linux__)
#include <stdint.h>		// for uintxx_t
#include <arpa/inet.h>	// for htonl/ntohl
#endif

// grrr!!
#if defined(__MINGW32__)
__stdcall uint32_t htonl(uint32_t hostlong)
{ 
	uint32_t v = hostlong;

	uint32_t vs = 
		((v & 0x000000FF) << 24) | 
		((v & 0x0000FF00) << 8) | 
		((v & 0x00FF0000) >> 8) | 
		((v & 0xFF000000) >> 24)
	;

	return vs;
}
__stdcall uint32_t ntohl(uint32_t netlong)
{ 
	uint32_t v = netlong;

	uint32_t vs = 
		((v & 0x000000FF) << 24) | 
		((v & 0x0000FF00) << 8) | 
		((v & 0x00FF0000) >> 8) | 
		((v & 0xFF000000) >> 24)
	;

	return vs;
}
__stdcall uint16_t htons(uint16_t hostword)
{ 
	uint16_t v = hostword;

	uint16_t vs = 
		((v & 0x00FF) << 8) | 
		((v & 0xFF00) >> 8)
	;

	return vs;
}
__stdcall uint16_t ntohs(uint16_t netword)
{ 
	uint16_t v = netword;

	uint16_t vs = 
		((v & 0x00FF) << 8) | 
		((v & 0xFF00) >> 8)
	;

	return vs;
}
#endif


typedef struct wrapheader_s
{
	// full header excluded from data_size

	uint32_t prefix;			// 'wrap'
	uint32_t wrapcode;			// 4-char code for wrapping packer type
	uint32_t data_size;			// size of compressed data (on disk size minus this header)
	uint32_t unpacked_size;		// size of unpacked data
	uint32_t pcrc;				// crc of packed data (check for device read errors)
	uint32_t ucrc;				// crc of unpacked data (check for packer/decompressor faults)

} wrapheader_t;

static uint32_t gen_crc_lite(uint16_t *_data, uint32_t _words)
{
	// perform hash
	uint32_t CRC = 0;

	do
	{
		uint16_t data16 = ntohs(*_data++);		// BE data representation on target

		// add
		CRC += (uint32_t)data16;

		// ror 7
		CRC = (CRC >> 7) | (CRC << ((sizeof(CRC)<<3)-7));

	} while ((--_words) != 0);

	return CRC;
}

static const uint32_t c_FNVOBase = 2166136261UL;

// generate 32bit CRC (FNV hash) based on 16 bit words
static uint32_t gen_crc(uint16_t *_data, uint32_t _words, uint32_t hash = c_FNVOBase)
{
	// perform hash
	// note: 32bit CRC position (and anything before it) will not be included
	uint32_t CRC = 0;
	{
		static const uint32_t c_FNVPrime = 16777619;

		do
		{
			uint16_t data16 = ntohs(*_data++);		// BE data representation on target

			// xor
			hash ^= data16;

			// mul c
			hash *= c_FNVPrime;

			// ror 7
			hash = (hash >> 7) | (hash << ((sizeof(hash)<<3)-7));

		} while ((--_words) != 0);

		CRC = hash;
	}

	return CRC;
}

int main(int argc, char * argv[]) 
{
	const char *name_orig;
	const char *name_packed;
	const char *name_wrapped;
	const char *wrapping_code;

	struct stat filestatus;

	uint32_t orig_filesize, be_orig_filesize;
	uint32_t in_filesize, be_in_filesize;


	printf("PackWrap: compressed asset wrapper for AGT : dml/2022\n");

	if (argc < 5)
	{
		printf("usage: packwrap <origfile> <packedfile> <wrappedfile> <wrapcode=lz77/nrve/arj4/arj7/none>\n");  
		return 0;
	}

	name_orig = argv[1];
	name_packed = argv[2];
	name_wrapped = argv[3];
	wrapping_code = argv[4];

	if ((0 == name_orig)||
		(0 == name_packed)||
	    (0 == name_wrapped)||
		(0 == wrapping_code)) 
	{
		printf("Bad command inputs\n");  
		return 0;
	}

	if (strlen(wrapping_code) != 4)
	{
		printf("Bad wrapping code, must be 4-char sequence\n");  
		return 0;
	}

	// size of original file
	filestatus.st_size = 0;
	stat(name_orig, &filestatus);
	if (!filestatus.st_size)
	{
		printf("Could not stat original file: %s\n", name_orig);  
		return 0;
	}
	orig_filesize = filestatus.st_size;

	// perform quick CRC on the original data before packing (packer safety check)
	uint32_t orig_crc = 0;
	{
		FILE *origfile = fopen(name_orig, "rb");
		uint8_t *orig_buffer = new uint8_t[orig_filesize];
		fread(orig_buffer, 1, orig_filesize, origfile);
		fclose(origfile);

		orig_crc = gen_crc_lite((uint16_t*)orig_buffer, orig_filesize>>1);

		delete[] orig_buffer;
	}


	// size of packed file
	filestatus.st_size = 0;
	stat(name_packed, &filestatus);
	if (!filestatus.st_size)
	{
		printf("Could not stat packed file: %s\n", name_packed);  
		return 0;
	}
	in_filesize = filestatus.st_size;

	printf("original size: %d\n", orig_filesize);
	printf("packed size: %d\n", in_filesize);
	printf("wrapping code: %s\n", wrapping_code);

	// open files



	FILE *infile = fopen(name_packed, "rb");
	if (infile == NULL)
	{
		printf("Failed to open packed file: %s\n", name_packed);  
		return 0;
	}

	uint8_t *buffer = new uint8_t[in_filesize + sizeof(wrapheader_t)];

	if (!buffer)
	{
		printf("Failed to allocate memory\n");  
		return 0;
	}

	uint8_t *bufferpos = buffer;

	uint32_t wrapcode = 
		((uint32_t)wrapping_code[0] << 24) |
		((uint32_t)wrapping_code[1] << 16) |
		((uint32_t)wrapping_code[2] <<  8) |
		((uint32_t)wrapping_code[3] <<  0);

	wrapheader_t *header = (wrapheader_t*)bufferpos;

	// create wrapper with BE encoding for 68k
	header->prefix =		htonl('wrap');
	header->wrapcode =		htonl(wrapcode);
	header->data_size =		htonl(in_filesize);
	header->unpacked_size =	htonl(orig_filesize);

	bufferpos += sizeof(wrapheader_t);

	fread(bufferpos, 1, in_filesize, infile);
	fclose(infile);

	// calculate simple CRC
	uint32_t packed_crc = gen_crc((uint16_t*)bufferpos, in_filesize >> 1);

	header->pcrc = htonl(packed_crc);
	header->ucrc = htonl(orig_crc);

	FILE *outfile = fopen(name_wrapped, "wb");
	if (outfile == NULL)
	{
		printf("Failed to write wrapped file: %s\n", name_wrapped);  
		return 0;
	}

	// write out wrapped file
	// [wrap][code][size_packed][size_unpacked][rawdata]
	fwrite(buffer, 1, (in_filesize + sizeof(wrapheader_t)), outfile);
	fclose(outfile);

	delete[] buffer;

	return 0;
}
