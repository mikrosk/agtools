//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
//	playfield tile library data / loader
//----------------------------------------------------------------------------------------------------------------------

#ifndef tileset_h_
#define tileset_h_

// ---------------------------------------------------------------------------------------------------------------------

class tileset
{
public:

	// ---------------------------------------------------------------------------------------------------------------------
	// types

	struct cct_header
	{
		u16 palette[16];
		u8 flags;
		u8 bitplanes;
		u16 tilecount;
	};

	// ---------------------------------------------------------------------------------------------------------------------
	// storage

	u16* tiles_;
	u16* tiles_alloc_;
	u16 numtiles_;
	u16 palette_[16];

	// ---------------------------------------------------------------------------------------------------------------------
	// internal stuff

private:

	void purge()
	{
		delete[] tiles_alloc_; tiles_alloc_ = 0;
	}

	// ---------------------------------------------------------------------------------------------------------------------
	// interface

public:

	tileset()
		: tiles_(0),
		  tiles_alloc_(0)
	{
	}

	~tileset()
	{
		dprintf("destroy tileset\n");
		purge();
	}
	
	// access tiledata
	inline const u16* get() const { return tiles_; }	
	// access tiledata for single tile
	inline const u16* get(s16 _t) const { return &tiles_[_t << 6]; }
	
	// access individual palette entries
	inline u16 getcol(s16 _c) { return palette_[_c]; }
		
	// load the tileset
	void load_cct(const char* const _filename)
	{
		purge();

		FILE* h = fopen(_filename, "rb");
		if (h != 0)
		{
			// read file header
			cct_header header;
			fread(&header, 1, sizeof(cct_header), h);
			
			// hackety hack - cope with earlier format
			if (header.bitplanes == 0)
				header.bitplanes = 4;

			// palette stored with tiles for convenience and debugging
			u16* cols = header.palette;
			for (s16 c = 0; c < 16; ++c)
				palette_[c] = cols[c];
			
			numtiles_ = header.tilecount;
			
			const s16 tilelinewords = 4;
			const s16 tilewords = tilelinewords * 16;

			u32 tilesetwords = tilewords * numtiles_;
			
#ifdef FASTPAGE_TILES
			// 'FASTPAGE_TILES' allows 16bit addressing and requires the
			// raw tiledata is aligned on a 64k boundary, with no more than
			// 512 tiles in total
			tiles_alloc_ = new u16[tilesetwords + 32768];	// +64k, in words!
			tiles_ = (u16*)(((u32)tiles_alloc_ + 0x0000FFFF) & 0xFFFF0000);
#else
			// normal, 24/32bit addressing
			tiles_alloc_ = new u16[tilesetwords];
			tiles_ = tiles_alloc_;
#endif
			pprintf("created tileset: %d tiles @ %d bpl\n", (int)numtiles_, (int)header.bitplanes);

			// read the data
			fread(tiles_, 1, tilesetwords * 2, h);

			fclose(h);
		}
	}
};

// ---------------------------------------------------------------------------------------------------------------------

#endif // tileset_h_
