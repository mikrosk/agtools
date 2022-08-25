//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
//	PhotoChrome splash screen interface
//----------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------
//	system headers

#include <mint/sysbind.h>

// ---------------------------------------------------------------------------------------------------------------------
//	Our AGT headers

#include "common_cpp.h"

// basic memory interface
#include "ealloc.h"
#include "compress.h"

// machine stuff
#include "system.h"

#include "pcs.h"

// ---------------------------------------------------------------------------------------------------------------------

extern "C" void STE_PCSV6_begin_();
extern "C" void STE_PCSV6_end_();
extern "C" int STE_PCSV6_Setup416x272(int num_fields, int num_lines, u16 **pimg_fieldseq, u16 **pbpal_fieldseq, u16 **pcpal_fieldseq, int scrolldelay);
extern "C" int STE_PCSV6_Show416x272();

// ---------------------------------------------------------------------------------------------------------------------

u16 *pcs416_image_fields[4];
u16 *pcs416_bpal_fields[4];
u16 *pcs416_cpal_fields[4];

// ---------------------------------------------------------------------------------------------------------------------
//	convert [8 nibble-pairs] to [4 plane-words]

static void rotate_bfield(u16 *data, s32 words)
{
	__asm__
	(
		"move.l		%0,%%a0;"
		"move.l		%0,%%a1;"
		"move.l		%1,%%d0;"
		"subq.w		#1,%%d0"
		";"
		"llp%=:;"
		"moveq		#4-1,%%d6;"
		";"
		"wlp%=:;"
		"move.w		(%%a0)+,%%d7;"
		"moveq		#4-1,%%d5;"
		";"
		"nlp%=:;"
		"add.w		%%d7,%%d7;"
		"addx.w		%%d1,%%d1;"
		"add.w		%%d7,%%d7;"
		"addx.w		%%d2,%%d2;"
		"add.w		%%d7,%%d7;"
		"addx.w		%%d3,%%d3;"
		"add.w		%%d7,%%d7;"
		"addx.w		%%d4,%%d4;"
		"dbf		%%d5,nlp%=;"
		"dbf		%%d6,wlp%=;"
		"move.w		%%d4,(%%a1)+;"
		"move.w		%%d3,(%%a1)+;"
		"move.w		%%d2,(%%a1)+;"
		"move.w		%%d1,(%%a1)+;"
		"dbf		%%d0,llp%=;"
		";"
		:
		: "g"(data), "g"(words>>2)
		: "%%d0", "%%d1", "%%d2", "%%d3", "%%d4", "%%d5", "%%d6", "%%d7", "%%a0", "%%a1" 
	);

}

// ---------------------------------------------------------------------------------------------------------------------

void STE_ShowAGI416(char *name, int scrolldelay)
{
	// pull in the asset and unpack it if necessary
	u8 * asset = load_asset
	(
		name,
		AssetFlags
		(
			afi_load_unwrapped |
			afi_asset_scratch 
			|
			(
				// .agi format a size prefix even if not wrapped
				afi_size_prefix |
				afi_size_exclusive
			)
		)
	);
	
	if (asset)
	{
		agiheader_t *pheader = (agiheader_t*)asset;

		if (pheader->code == 'p6t1')
		{
			s16 nlines = pheader->h;
			s16 palette_fields = pheader->pf;
			s16 bitmap_fields = pheader->bf;

			printf("loaded PCS: w=%d, h=%d, p-fields=%d, b-fields=%d\n", pheader->w, pheader->h, pheader->pf, pheader->bf);

			++pheader;
			u16 *imagedata = (u16*)pheader;
			--pheader;

			u16 *ptr = imagedata;

			s16 b = 0;
			for (; b < bitmap_fields; b++)
			{
				static const int bfield_words = (160+(224*nlines))/2;
				pcs416_image_fields[b] = ptr;	ptr += bfield_words;

				rotate_bfield(pcs416_image_fields[b], bfield_words);
			}
			for (; b < 4; b++)
				pcs416_image_fields[b] = pcs416_image_fields[0];


			s16 p = 0;
			for (; p < palette_fields; p++)
			{
				pcs416_bpal_fields[p] = ptr;	ptr += ((14*3)*nlines);
				pcs416_cpal_fields[p] = ptr;	ptr += (10*(1+nlines));
			}

			// pass the information to the PCSv6 viewer
			STE_PCSV6_Setup416x272
			(
				/*fields=*/palette_fields, 
				/*lines=*/nlines, 
				pcs416_image_fields,  
				pcs416_bpal_fields, 
				pcs416_cpal_fields,
				scrolldelay
			);

			// invoke the viewer, which hogs the machine until SPACEBAR is hit
			STE_PCSV6_Show416x272();
		}

		// release asset space
		efree(asset);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
