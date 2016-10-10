//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
//	PhotoChrome splash screen interface
//----------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------
//	system headers

// ---------------------------------------------------------------------------------------------------------------------
//	Our AGT headers

#include "common_cpp.h"

// basic memory interface
#include "ealloc.h"
#include "compress.h"

// machine stuff
#include "system.h"

// ---------------------------------------------------------------------------------------------------------------------

extern "C" void STE_PCSV6_begin_();
extern "C" void STE_PCSV6_end_();
extern "C" int STE_PCSV6_Setup416x272(int num_fields, int num_lines, void **pimg_fieldseq, void **pbpal_fieldseq, void **pcpal_fieldseq, int scrolldelay);
extern "C" int STE_PCSV6_Show416x272();

// ---------------------------------------------------------------------------------------------------------------------

void *pcs416_image_fields[3];
void *pcs416_bpal_fields[3];
void *pcs416_cpal_fields[3];

// ---------------------------------------------------------------------------------------------------------------------

bool STE_ConfigurePCS416()
{
	// load the correct driver on first use (MSTE blitter timing is different)
	static bool s_configured = false;

	if (!s_configured)
	{
		bool r = false;
		if (bMegaSTE)
			r = place_p2e("b52m.p2e", (u8*)&STE_PCSV6_begin_ /*, ((int)&STE_PCSV6_end_-(int)&STE_PCSV6_begin_)*/);
		else
			r = place_p2e("b52.p2e", (u8*)&STE_PCSV6_begin_ /*, ((int)&STE_PCSV6_end_-(int)&STE_PCSV6_begin_)*/);

		s_configured |= r;
	}

	return s_configured;
}

void STE_ShowPCS416(char *name, int nfields, int nlines, int scrolldelay)
{
	FILE *h = fopen(name, "rb");
	if (h)
	{
		// allocate and load the image
		int filesize = (((224+(52*2)) * nlines) + 160) * nfields;
		char *imagedata = (char*)ealloc(filesize);
		fread(imagedata, 1, filesize, h);
		fclose(h);

		char *ptr = imagedata;

		for (s16 f = 0; f < nfields; f++)
		{
			pcs416_image_fields[f] = ptr;	ptr += 160+(224*nlines);
			pcs416_bpal_fields[f] = ptr;	ptr += (42*2*nlines);
			pcs416_cpal_fields[f] = ptr;	ptr += (10*2*nlines);
		}

		// pass the information to the PCSv6 viewer
		STE_PCSV6_Setup416x272
		(
			/*fields=*/nfields, 
			/*lines=*/nlines, 
			pcs416_image_fields,  
			pcs416_bpal_fields, 
			pcs416_cpal_fields,
			scrolldelay
		);

		// we can just free this since nothing will be written or lost during viewing
		efree(imagedata);

		// invoke the viewer, which hogs the machine until SPACEBAR is hit
		STE_PCSV6_Show416x272();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
