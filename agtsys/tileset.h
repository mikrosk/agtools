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
	s16 numtiles_;
	s16 tilesize_;
	s16 bitplanes_;
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
		  tiles_alloc_(0),
		  numtiles_(0),
		  bitplanes_(0),
		  tilesize_(0)
	{
	}

	~tileset()
	{
		dprintf("destroy tileset\n");
		purge();
	}
	
	// access tiledata
	inline const u16* get() const { return tiles_; }	
	inline const u8* get8() const { return (u8*)tiles_; }	
	// access tiledata for single tile
	//inline const u16* get(s16 _t) const { return &tiles_[_t << (7-1)]; }	// 16x16 tiles
	//inline const u8* get8(s16 _t) const { return (u8*)(&tiles_[_t << (5-1)]); }	// 8x8 tiles
	
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
			
			// determine bitdepth
			bitplanes_ = header.bitplanes;
			if (bitplanes_ == 0)
				bitplanes_ = 4;

			// determine tilesize
			tilesize_ = 1 << (header.flags & 7);
			if (tilesize_ == 1)
				tilesize_ = 16;

			// palette stored with tiles for convenience and debugging
			u16* cols = header.palette;
			for (s16 c = 0; c < 16; ++c)
				palette_[c] = cols[c];
			
			// calculate words per tile
			const s16 tilelinewords = tilesize_>>2;
			s16 tilewords = tilelinewords * tilesize_;

			// tiles with mask consume 2x the space (which may include padding in 16x16 case)
			if (header.flags & 8)
				tilewords <<= 1;

			// calculate words for complete tileset
			numtiles_ = header.tilecount;
			s32 tilesetwords = tilewords * numtiles_;
			
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
			printf("created tileset: %d tiles @ %dx%d in %dp\n", 
				(int)numtiles_, (int)tilesize_, (int)tilesize_, (int)header.bitplanes);

			// read the data
			fread(tiles_, 1, tilesetwords * 2, h);

			fclose(h);
		}
	}
};

// ---------------------------------------------------------------------------------------------------------------------

#endif // tileset_h_
