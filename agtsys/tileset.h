//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
//	playfield tile library data / loader
//----------------------------------------------------------------------------------------------------------------------

#ifndef tileset_h_
#define tileset_h_

// ---------------------------------------------------------------------------------------------------------------------

#include "ealloc.h"
#include "compress.h"

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

	u8* tiles_alloc_;
	u16* tiles_;
	s16 numtiles_;
	s16 tilesize_;
	s16 bitplanes_;
	u16 palette_[16];

	// ---------------------------------------------------------------------------------------------------------------------
	// internal stuff

private:

	void purge()
	{
		//delete[] tiles_alloc_; tiles_alloc_ = 0;
		efree(tiles_alloc_); tiles_alloc_ = nullptr;
		tiles_ = nullptr;
		numtiles_ = 0;
	}

	// ---------------------------------------------------------------------------------------------------------------------
	// interface

public:

	tileset()
		: tiles_alloc_(nullptr),
		  tiles_(nullptr),
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
		
	s16 interpret_header(const cct_header& header)
	{
		// determine bitdepth
		bitplanes_ = header.bitplanes;
		if (bitplanes_ == 0)
			bitplanes_ = 4;

		// determine tilesize
		tilesize_ = 1 << (header.flags & 7);
		if (tilesize_ == 1)
			tilesize_ = 16;

		// palette stored with tiles for convenience and debugging
		const u16* cols = header.palette;
		for (s16 c = 0; c < 16; ++c)
			palette_[c] = cols[c];
		
		// calculate words per tile
		const s16 tilelinewords = (tilesize_ * bitplanes_) >> (3+1);
		s16 tilewords = tilelinewords * tilesize_;

		// tiles with mask consume 2x the space (which may include padding in 16x16 case)
		if (header.flags & 8)
			tilewords <<= 1;

		// calculate words for complete tileset
		numtiles_ = header.tilecount;

		return tilewords;
	}

	// load the tileset
	void load_cct(const char* const _filename)
	{
		purge();

		// attempt to load packed version of asset
		u8* passet = unwrap_p2e_asset(_filename, false, false);
		if (passet != ((u8*)-1))
		{
			// note: packed assets don't support FASTPAGE_TILES mode (<= 512 tiles) without further reallocation
			tiles_alloc_ = passet;
			tiles_ = (u16*)(passet + sizeof(cct_header));

			// interpret file header
			cct_header &header = *(cct_header*)tiles_alloc_;			
			s16 tilewords = interpret_header(header);

			// calculate words for complete tileset
			numtiles_ = header.tilecount;
			s32 tilesetwords = tilewords * numtiles_;
	
			printf("created tileset: %d tiles @ %dx%d in %dp\n", 
				(int)numtiles_, (int)tilesize_, (int)tilesize_, (int)header.bitplanes);
		}
		else
		if (passet) 
		{
			// otherwise load raw asset using explicit code
			// this supports FASTPAGE_TILES mode (<= 512 tiles)

			FILE* h = fopen(_filename, "rb");
			if (h != 0)
			{
				// read file header
				cct_header header;
				fread(&header, 1, sizeof(cct_header), h);
				s16 tilewords = interpret_header(header);
				s32 tilesetwords = tilewords * numtiles_;
				
	#ifdef FASTPAGE_TILES
				// 'FASTPAGE_TILES' allows 16bit addressing and requires the
				// raw tiledata is aligned on a 64k boundary, with no more than
				// 512 tiles in total
				tiles_alloc_ = (u8*)ealloc((tilesetwords<<1) + 65536);//new u16[tilesetwords + 32768];	// +64k, in words!
				tiles_ = (u16*)(((u32)tiles_alloc_ + 0x0000FFFF) & 0xFFFF0000);
	#else
				// normal, 24/32bit addressing
				tiles_alloc_ = (u8*)ealloc(tilesetwords<<1);//new u16[tilesetwords];
				tiles_ = (u16*)tiles_alloc_;
	#endif
				printf("created tileset: %d tiles @ %dx%d in %dp\n", 
					(int)numtiles_, (int)tilesize_, (int)tilesize_, (int)header.bitplanes);

				// read the data
				fread(tiles_, 1, tilesetwords * 2, h);

				fclose(h);
			}
			else
			{
				eprintf("error: opening raw tileset: [%s]\n", _filename);
			}
		}
		else
		{
			eprintf("error: opening packed tileset: [%s]\n", _filename);
		}
	}
};

// ---------------------------------------------------------------------------------------------------------------------

#endif // tileset_h_
