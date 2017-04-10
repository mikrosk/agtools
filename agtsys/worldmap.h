//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
//	playfield world map data / loader
//----------------------------------------------------------------------------------------------------------------------

#ifndef worldmap_h_
#define worldmap_h_

#include "ealloc.h"
#include "../3rdparty/tinycsv.h"
#include "fileio.h"

// ---------------------------------------------------------------------------------------------------------------------

enum MapFormat
{
	MapFormat_PLAYFIELD,
	MapFormat_32,
	MapFormat_16,
	MapFormat_8
};

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
		header_.width = 0;
		header_.height = 0;
	}

	s32 allocate(MapFormat _format)
	{
		// allocate space (+overrun padding)
		s32 mapspace = header_.width * header_.height;
		s32 allocspace = mapspace;
		switch (_format)
		{
		case MapFormat_PLAYFIELD:
			allocspace = allocspace << 1;
			break;
		case MapFormat_32:
			break;
		case MapFormat_16:
			allocspace = (allocspace+1) >> 1;
			break;
		case MapFormat_8:
			allocspace = (allocspace+3) >> 2;
			break;
		};

		map_ = new s32[allocspace];
		return mapspace;
	}

	// specific step to prepare map for playfield tile engine. not required for collision maps etc.
	void prepare_playfield(s32 _mapspace)
	{
		// we now preshift for 16x16 tiles since other bits are unused (so far - can be masked later)
		for (s32 m = 0; m < _mapspace; m++)
		{
			map_[m] <<= 7;	// todo: support different tile sizes, although 16 is optimal for ST/e
		}

		// fill additional rows with copy of map data, to catch map indexing overruns when 
		// VScroll is enabled but map is smaller than virtual page size + vscroll margin
		qmemcpy(&map_[_mapspace], &map_[0], _mapspace*sizeof(s32));
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
	inline s32* get32() const { return map_; }	
	inline s16* get16() const { return (s16*)map_; }	
	inline s8* get8() const { return (s8*)map_; }	
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

			s32 mapspace = allocate(MapFormat_PLAYFIELD);
									
			pprintf("created ccm worldmap: %dx%d tiles\n", (int)header_.width, (int)header_.height);

			// read main part of the file
			fread(map_, 1, mapspace * sizeof(s32), h);
			// done
			fclose(h);

			// older format was emitted little-endian, so this is retained
			if (le)
				endianswap32((u32*)map_, mapspace); 

			prepare_playfield(mapspace);
		}
			}

	void load_csv(const char* const _filename, s16 _w, s16 _h, MapFormat _format)
	{
		purge();

		size_t rawsize = fsize((char*)_filename);
		if (rawsize)
		{
			FILE* h = fopen(_filename, "rb");
			if (h != 0)
			{
				header_.width = _w;
				header_.height = _h;

				s32 mapspace = allocate(_format);

				pprintf("created csv worldmap: %dx%d tiles\n", (int)header_.width, (int)header_.height);

				char *rawdata = (char*)ealloc(rawsize);
				fread(rawdata, 1, rawsize, h);
			// done
			fclose(h);
				{
					int i, j;

					// alloc row scratch area
					char *scratch = (char*)ealloc(32768);

					// parse in rows
					char **rows = NULL;
					csv_getcols(rawdata, "\n", &rows);

					s32 *pmap32 = map_;
					s16 *pmap16 = (s16*)map_;
					s8 *pmap8 = (s8*)map_;

					for (i = 0; rows[i]; i++) 
					{
						// parse in columns from each row
						char **columnX = (char**)scratch;//NULL;
						csv_getcols(rows[i], ",", &columnX);

						switch (_format)
						{
						default:
						case MapFormat_PLAYFIELD:
						case MapFormat_32:
							for (j = 0; columnX[j]; j++) *pmap32++ = atoi(columnX[j]);
							break;
						case MapFormat_16:
							for (j = 0; columnX[j]; j++) *pmap16++ = atoi(columnX[j]);
							break;
						case MapFormat_8:
							for (j = 0; columnX[j]; j++) *pmap8++ = atoi(columnX[j]);
							break;
						};
					}

					// free rows
					efree(rows);

					// free row scratch area
					efree(scratch);
				}

				efree(rawdata);

				if (_format == MapFormat_PLAYFIELD)
					prepare_playfield(mapspace);
			}
			else
			{
				eprintf("error: could not open csv map: %s @ %dx%d tiles\n", _filename, _w, _h);
			}
		}
		else
		{
			eprintf("error: could not size csv map: %s @ %dx%d tiles\n", _filename, _w, _h);
		}
	}	
};

// ---------------------------------------------------------------------------------------------------------------------

#endif // worldmap_h_
