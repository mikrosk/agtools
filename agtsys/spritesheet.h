//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
//	SPRITE graphics data / loader
//----------------------------------------------------------------------------------------------------------------------

#ifndef spritesheet_h_
#define spritesheet_h_

// ---------------------------------------------------------------------------------------------------------------------

#include "ealloc.h"
#include "compress.h"

// ---------------------------------------------------------------------------------------------------------------------

class spritesheet
{
public:

	// ---------------------------------------------------------------------------------------------------------------------
	// types

	static const int SPS_VERSION = 0x0202;

	// sprite frame header (one per sprite frame)
	struct spf_header
	{
		s16 w;						// frame width in pixels
		s16 h;						// frame height in pixels
		s16 xo;						// frame x offset (from source cutting position, if size was optimized)
		s16 yo;						// frame y offset (from source cutting position, if size was optimized)
		s16 omap;					// offset to optional occlusion maps
		s16 framedata[1];			// mask/colour data follows immediately when no preshifts present
									// OR...
									// pointers[#preshifts=1] to colourdata preshift frames
									// pointers[#preshifts=16] to maskdata preshift frames
	};


	// in-memory spritesheet header
	struct sps_header
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
		spf_header *frameindex[1];	// sprite frame index follows (pointers to individual sprite frame headers)
	};

	// ---------------------------------------------------------------------------------------------------------------------
	// storage

	sps_header* data_;
	u8* alloc_;
	
	// ---------------------------------------------------------------------------------------------------------------------
	// internal stuff

private:

	void purge()
	{
		efree(alloc_); alloc_ = nullptr;
		data_ = nullptr;
	}

	// ---------------------------------------------------------------------------------------------------------------------
	// interface

public:

	spritesheet()
		: alloc_(nullptr),
		  data_(nullptr)
	{
	}

	~spritesheet()
	{
		dprintf("destroy spritesheet\n");
		purge();
	}
	
	inline sps_header* get() const { return data_; }	
	inline s32 getwidth() const { return data_->srcwidth; }	
	inline s32 getheight() const { return data_->srcheight; }	
		
	// todo: read 'preshift' flags from file format instead of relying on user
	bool load(const char* const _filename, bool _preshift = true)
	{
		bool success = false;

		purge();

		// load, or load-and-unpack asset file to memory buffer
		u8* passet = unwrap_p2e_asset(_filename, true, true);

		if (passet)
		{
			// allocation includes size prefix, keep separate
			alloc_ = passet;
			data_ = (sps_header*)(passet+4);

			if (data_->version == SPS_VERSION)
			{

				pprintf("created spritesheet:\n\t%dx%d @ %d bpl, %d frames\n", 
					data_->srcwidth, 
					data_->srcheight, 
					data_->planes, 
					data_->framecount);

				s16 frames = data_->framecount;

				// relocate frame index for convenient access
				// caution: porting hazard, aliasing hazard. but hey its an ST!
				ptrsize_t addr_base = (ptrsize_t)&(data_->frameindex[0]);

				for (s16 frame = 0; frame < frames; frame++)
				{
					ptrsize_t addr_offset = (ptrsize_t)(data_->frameindex[frame]);
					data_->frameindex[frame] = (spf_header*)(addr_base + addr_offset);

					u16 storageflags = data_->flags & 7;

					// detect [EMSPR/EMXSPR] preshifted mask data or code
					if ((storageflags == 5) || (storageflags == 7))
					{
						// get preshift sub-index base address
						ptrsize_t subindex_base = (ptrsize_t)&(data_->frameindex[frame]->framedata[0]);

						// relocate preshifted colour subframes [EMSPR/EMXSPR]
						{
							ptrsize_t addr_base = subindex_base;
							ptrsize_t *psubindex = (ptrsize_t*)addr_base;
							s16 index = 0;
							for (s16 preshift = 0; preshift < 1; preshift++)
							{
								for (s16 field = 0; field < 2; field++)
								{
									ptrsize_t addr_offset = psubindex[index];
								//pprintf("note: colourplanes:%x!\n", addr_offset);
								if (addr_offset > 0)
										psubindex[index] = addr_base + addr_offset;
									subindex_base += 4;
									index++;
								}
							}
						}

						// relocate preshifted colouropt subframes [EMXSPR/CODEGEN ONLY]
						if (storageflags == 7)
						{
							ptrsize_t addr_base = subindex_base;
							ptrsize_t *psubindex = (ptrsize_t*)addr_base;
							s16 index = 0;
							for (s16 preshift = 0; preshift < 1; preshift++)
							{
								for (s16 field = 0; field < 2; field++)
								{
									ptrsize_t addr_offset = psubindex[index];
									if (addr_offset > 0)
										psubindex[index] = addr_base + addr_offset;
								subindex_base += 4;
									index++;
								}
							}
						}

						// relocate preshifted mask subframes [EMSPR/EMXSPR]
						{
							ptrsize_t addr_base = subindex_base;
							ptrsize_t *psubindex = (ptrsize_t*)addr_base;
							for (s16 preshift = 0; preshift < 16; preshift++)
							{
								ptrsize_t addr_offset = psubindex[preshift];
								//pprintf("note: psmask:%x!\n", addr_offset);

								if (addr_offset > 0)
								{
									ptrsize_t addr_abs = addr_base + addr_offset;
									psubindex[preshift] = addr_abs;
								}
								subindex_base += 4;
							}
						}

						// relocate preshifted maskopt subframes [EMXSPR/CODEGEN ONLY]
						if (storageflags == 7)
						{
							ptrsize_t addr_base = subindex_base;
							ptrsize_t *psubindex = (ptrsize_t*)addr_base;
							for (s16 preshift = 0; preshift < 16; preshift++)
							{
								ptrsize_t addr_offset = psubindex[preshift];
								ptrsize_t addr_abs = addr_base + addr_offset;
								psubindex[preshift] = addr_abs;
								subindex_base += 4;
							}
						}
					} // storageflags
				} // frames
				success = true;
			}
			else // SPS_VERSION
			{
				eprintf("error: spritesheet version (%x != %x) is stale - re-generate data with latest AGTCUT!\n", data_->version, SPS_VERSION);

				purge();
			}
		}
		else
		{
			eprintf("error: opening spritesheet: [%s]\n", _filename);
		}

		return success;
	}	
};

// ---------------------------------------------------------------------------------------------------------------------

#endif // spritesheet_h_
