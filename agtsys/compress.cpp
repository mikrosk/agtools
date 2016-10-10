//======================================================================================================================
//	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
// 	compression interface
//----------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------
//	system headers

// ---------------------------------------------------------------------------------------------------------------------
//	Our AGT headers

#include "common_cpp.h"

// basic memory interface
#include "ealloc.h"
#include "compress.h"

// ---------------------------------------------------------------------------------------------------------------------

bool place_p2e(const char *const _name, u8 *_place)
{
	reg8(a4) = 1;

	FILE *fh = fopen(_name, "rb");
	if (fh == 0)
	{
		reg8(a4) = 0;

		printf("error: failed to read P2E: %s\n", _name);
	}
	else
	{
		// read filesize
		u32 datasize = 0;
		fread(&datasize, 1, 4, fh);
		endianswap32(&datasize, 1);

		u8 *temp_ptr = (u8*)ealloc(datasize);
		fread(temp_ptr, 1, datasize, fh);
		fclose(fh);

		reg8(a4) = 0;

		// check for packed indicator
		if (*((u32*)temp_ptr) == 'pk2e')
		{
			// skip 'pk2e' ident
			u8* data_ptr = temp_ptr; data_ptr += 4;

			// fetch size of unpacked data
			datasize = *((u32*)data_ptr); data_ptr += 4;
			endianswap32(&datasize, 1);

			// unpack
			qmemclr(_place, datasize);
			DePack2e_68k(data_ptr, _place);

			// release file buffer
			efree(temp_ptr);

			return true;
		}
		else
		{
			printf("error: p2e incorrect header: %s\n", _name);
		}
	}

	return false;
}

// ---------------------------------------------------------------------------------------------------------------------
