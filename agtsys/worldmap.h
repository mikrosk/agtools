//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
//	playfield world map data / loader
//----------------------------------------------------------------------------------------------------------------------

#ifndef worldmap_h_
#define worldmap_h_

// ---------------------------------------------------------------------------------------------------------------------

class worldmap
{
public:

	// ---------------------------------------------------------------------------------------------------------------------
	// types

	struct ccm_header
	{
		s32 width;
		s32 height;
	};

	// ---------------------------------------------------------------------------------------------------------------------
	// storage

	ccm_header header_;

	s32* map_;

	// ---------------------------------------------------------------------------------------------------------------------
	// internal stuff

private:

	void purge()
	{
		delete[] map_; map_ = 0;
	}

	// ---------------------------------------------------------------------------------------------------------------------
	// interface

public:

	worldmap()
		: map_(0)
	{
	}

	~worldmap()
	{
		dprintf("destroy worldmap\n");
		purge();
	}
	
	inline s32* get() const { return map_; }	
	inline s32 getwidth() const { return header_.width; }	
	inline s32 getheight() const { return header_.height; }	
			
	void load_ccm(const char* const _filename)
	{
		purge();

		FILE* h = fopen(_filename, "rb");
		if (h != 0)
		{
			// read the header
			fread(&header_, 1, sizeof(ccm_header), h);

			// older format was emitted little-endian, so we just detect it
			// and reformat if needed
			// todo: deprecate the old format, remove the swappage
			bool le = (header_.width & 0xFFFF0000);
			if (le)
			{
				endianswap32((u32*)&header_.width, 1); 
				endianswap32((u32*)&header_.height, 1); 
			}

			pprintf("created worldmap: %dx%d tiles\n", header_.width, header_.height);
									
			// allocate space (+overrun padding)
			u32 mapspace = header_.width * header_.height;
			map_ = new s32[mapspace<<1];

			// read main part of the file
			fread(map_, 1, mapspace * sizeof(s32), h);

			// older format was emitted little-endian, so this is retained
			if (le)
				endianswap32((u32*)map_, mapspace); 

			// we now preshift for 16x16 tiles since other bits are unused (so far - can be masked later)
			for (s32 m = 0; m < mapspace; m++)
			{
				map_[m] <<= 7;	// todo: support different tile sizes, although 16 is optimal for ST/e
			}

			// fill additional rows with copy of map data, to catch map indexing overruns when 
			// VScroll is enabled but map is smaller than virtual page size + vscroll margin
			qmemcpy(&map_[mapspace], &map_[0], mapspace*sizeof(s32));

			// done
			fclose(h);
		}
	}	
};

// ---------------------------------------------------------------------------------------------------------------------

#endif // worldmap_h_
