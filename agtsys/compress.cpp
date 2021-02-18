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

u8* unwrap_p2e_asset
(
	const char *const _name,	// filename
	bool _load_raw,				// load raw version anyway, if not p2e wrapped (otherwise return -1 so caller can attempt load)
	bool _raw_size_prefix,		// raw version has size prefix, avoids need to derive filesize by other means
	u32 *_psize
)
{
	reg8(a4) = 1;

	FILE *fh = fopen(_name, "rb");
	if (fh == 0)
	{
		reg8(a4) = 0;

		printf("error: p2e unwrap fail: %s\n", _name);
		
		return nullptr;
	}
	else
	{
		// read p2e header (8 bytes)
		u32 p2eh[2];
		fread(p2eh, 1, 8, fh);
		u32 rawsize = p2eh[0];

		endianswap32(&p2eh[0], 1);

		// check for p2e wrapping signature
		if ((p2eh[1] == 'pk2e') && (p2eh[0] < (1<<21)))
		{
			// p2e-wrapped asset

			// reset
			fseek(fh, 4, 0);

			u32 packedsize = p2eh[0];

			u8 *temp_ptr = (u8*)ealloc(packedsize);
			fread(temp_ptr, 1, packedsize, fh);
			fclose(fh);

			reg8(a4) = 0;

			// skip 'pk2e' ident
			u8* data_ptr = temp_ptr; data_ptr += 4;

			// fetch size of unpacked data
			u32 datasize = *((u32*)data_ptr); data_ptr += 4;
			endianswap32(&datasize, 1);

			u8* place = (u8*)ealloc(datasize);
			if (place)
			{
				if (_psize)
				{
					*_psize = datasize;
				}

				// unpack
				qmemclr(place, datasize);
				DePack2e_68k(data_ptr, place);

				// release file buffer
				efree(temp_ptr);

				return place;
			}
			else
			{
				printf("error: p2e unwrap failed allocate space: %s\n", _name);

				// release file buffer
				efree(temp_ptr);

				return nullptr;
			}
		}
		else
		{
			if (_load_raw)
			{
				// plain asset, no p2e wrapping
				// can be loaded as contiguous blob

				if (_raw_size_prefix)
				{
					// we know the size from the prefix/header
					rawsize += 4;
				}
				else
				{
					// if raw asset has no size prefix we need to find the size directly
					fseek(fh, 0, SEEK_END);
					rawsize = ftell(fh);
				}

				if (_psize)
				{
					*_psize = rawsize;
				}

				// reset
				fseek(fh, 0, 0);

				// allocate space
				u8* passet = (u8*)ealloc(rawsize);

				// read main part of file
				fread(passet, 1, rawsize, fh);

				fclose(fh);

				reg8(a4) = 0;

				return passet;
			}
			else
			{
				reg8(a4) = 0;

				// signal caller that file is raw but can't be loaded as contiguous block
				return (u8*)-1;
			}
		}
	}
				
	reg8(a4) = 0;

	return nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
