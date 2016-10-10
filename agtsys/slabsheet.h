//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
//	SLAB graphics data / loader
//----------------------------------------------------------------------------------------------------------------------

#ifndef slabsheet_h_
#define slabsheet_h_

// ---------------------------------------------------------------------------------------------------------------------

class slabsheet
{
public:

	// ---------------------------------------------------------------------------------------------------------------------
	// types

	// spanblock clipper header
	struct slc_header
	{
		s16 offset;					// poffset from indexbase to first spanblock record for this yc
		s16	remaining;				// remaining spanblocks to draw for this yc
		s16 spdoff;					// offset from indexbase to spanblock pixel data
	};

	// spanblock header
	struct ssb_header
	{
		s16 dy;						// skipped lines since last spanblock (for gaps)
		s16	xo;						// pixel xoffset from frame left edge
		s16	xs;						// pixel xsize of span
		s16 dxw;					// word xsize of span ((xs+15)&-16)
		s16 y;						// first y of spanblock
		s16 ys;						// ysize-1 of spanblock (0 for single span)
		s32 spandata;				// pointer to actual span pixels, left-justified & contiguous
	};

	// slab frame header (one per slab frame)
	struct slf_header
	{
		s16 w;						// frame width in pixels
		s16 h;						// frame height in pixels
		s16 xo;						// frame x offset (from source cutting position, if size was optimized)
		s16 yo;						// frame y offset (from source cutting position, if size was optimized)
		s16 spanblocks;				// number of spanblocks
		s8 indexbase[8];			// variable sized index & data follows 
									// ([h]eight * 8 bytes) clipping index to spanblock ssb_headers
									// followed by (spanblocks * 2 bytes) condensed index to unique spanblock ssb_headers
									// note: each index holds offsets from indexbase[]
	};

	// in-memory slabsheet header
	struct sls_header
	{
		// note: initial [filebytes] field is not part of in-memory header!
		s16 version;				// file format version info
		s16 framecount;				// number of frames in spritesheet
		s16 planes;					// number of bitplanes in sprite
		u16 flags;					// misc. flags (mask format etc.)
		u8 preshift_range;			// preshift range = 2^preshift_range pixels (0..4 for 0->16 pixels)
		u8 preshift_step;			// preshift step = 2^preshift_step pixels (0..3 for 1->8 pixels)
		s16 srcwidth;				// source cutting width (before frame size optimization)
		s16 srcheight;				// source cutting height (before frame size optimization)
		slf_header *frameindex[1];	// sprite frame index follows (pointers to individual sprite frame headers)
	};

	// ---------------------------------------------------------------------------------------------------------------------
	// storage

	sls_header* data_;
	
	// ---------------------------------------------------------------------------------------------------------------------
	// internal stuff

private:

	void purge()
	{
		delete[] data_; data_ = 0;
	}

	// ---------------------------------------------------------------------------------------------------------------------
	// interface

public:

	slabsheet()
		: data_(0)
	{
	}

	~slabsheet()
	{
		dprintf("destroy slabsheet\n");
		purge();
	}
	
	inline sls_header* get() const { return data_; }	
	inline s32 getwidth() const { return data_->srcwidth; }	
	inline s32 getheight() const { return data_->srcheight; }	
			
	bool load_sls(const char* const _filename)
	{
		bool success = false;

		purge();

		FILE* h = fopen(_filename, "rb");
		if (h != 0)
		{
			s32 size = 0;
			fread(&size, 1, sizeof(s32), h);

			// sanity check for corrupt files etc.
			if ((size > 0) && (size < (1024*1024)))
			{
				// allocate space
				data_ = (sls_header*)(new u8[size]);
				// read main part of file
				fread((char*)data_, 1, size, h);
									
				pprintf("created slabsheet:\n\t%dx%d @ %d bpl, %d frames\n", 
					data_->srcwidth, 
					data_->srcheight, 
					data_->planes, 
					data_->framecount);

				s16 frames = data_->framecount;

				// relocate frame index for convenient access
				// caution: porting hazard, aliasing hazard. but hey its an ST!
				ptrsize_t addr_base = (ptrsize_t)&(data_->frameindex[0]);

				for (s16 f = 0; f < frames; f++)
				{
					// relocate frame index
					ptrsize_t addr_offset = (ptrsize_t)(data_->frameindex[f]);
					slf_header *pbh = (slf_header*)(addr_base + addr_offset);
					data_->frameindex[f] = pbh;

					ptrsize_t index_base = (ptrsize_t)&(pbh->indexbase[0]);

					// clipper and spanblock indices remain as offsets to save space
					// but pixeldata references are pointers so they need relocation

					// clipper table continues from indexbase
					//slc_header *pclipper = (slc_header*)(&(pbh->indexbase[0]));

					//for (s16 yc = 0; yc < pbh->h; yc++)
					//{
					//	//printf("clipper[%d]: %08x\n", (int)yc, pclipper[yc].spandata);

					//	// relocate pixel data vs index base, so each spanblock record points at its own pixels
					//	pclipper[yc].spandata += index_base;	
					//}

					// spanblock index follows clipper table, which is (8 bytes * height) in size
					s16 *pspanblocks = (s16*)&(pbh->indexbase[8 * pbh->h]);

					for (s16 sb = 0; sb < pbh->spanblocks; sb++)
					{
						// note: pixeldata offsets are in terms of bytes, not words - meant for 68k lookups.
						ssb_header *pssb = (ssb_header*)(&pbh->indexbase[pspanblocks[sb]]);

						//printf("spanrecord[%d]: %08x\n", (int)sb, pssb->spandata);

						// relocate pixel data vs index base, so each spanblock record points at its own pixels
						// note: this is used mainly by the x-clipper to find the source offset in the spanblock
						pssb->spandata += index_base;		
					}
				}
			}
			fclose(h);

			success = true;
		}
		else
		{
			eprintf("error: opening slabsheet: [%s]\n", _filename);
		}

		return success;
	}	
};

// ---------------------------------------------------------------------------------------------------------------------
// slab pair structure allows detailed draw shape and simplified restore shape to be assigned as single asset

class sls_pair
{
public:

	slabsheet::sls_header *psls_draw_;
	slabsheet::sls_header *psls_restore_;

	sls_pair()
		: psls_draw_(0),
		  psls_restore_(0)
	{
	}

	void assign(slabsheet::sls_header *_draw, slabsheet::sls_header *_restore)
	{
		psls_draw_ = _draw;
		psls_restore_ = _restore;
	}
};

// ---------------------------------------------------------------------------------------------------------------------

#endif // slabsheet_h_
