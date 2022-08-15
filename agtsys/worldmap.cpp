//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
//	playfield tile library data / loader
//----------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------
//	system headers

// ---------------------------------------------------------------------------------------------------------------------
//	Our AGT headers

#include "common_cpp.h"

#include "compress.h"

#include "worldmap.h"

// ---------------------------------------------------------------------------------------------------------------------

worldmap::~worldmap()
{
	dprintf("destroy map\n");
	purge();
}

bool worldmap::create(s16 _w, s16 _h, int _tilestep, bool _overlay, int _fill_index)
{
	purge();

	bool success = false;

	header_.flags = _overlay ? 8 : 0;
	header_.tilestep = _tilestep;
	header_.width = _w;
	header_.height = _h;

	s32 mapspace = allocate(MapFormat_PLAYFIELD);

	if (mapspace)
	{
		pprintf("created map: %dx%d tiles\n", (int)header_.width, (int)header_.height);

		for (int i = 0; i < mapspace; i++)
			map_[i] = _fill_index;

		prepare_playfield(mapspace);

		success = true;
	}

	return success;
}

bool worldmap::load_ccm(const char* const _filename)
{
	purge();

	bool success = false;

	// load-and-unpack asset file to memory buffer
	u8* passet = load_asset
	(
		_filename,
		AssetFlags(af_none) // don't proceed to load the file if the header is unwrapped
	);

	s32 mapspace = 0;

	if (passet)
	{
		// loaded as wrapped asset - transfer data to map

		const ccm_header *psrcheader = (ccm_header*)passet;
		qmemcpy(&header_, psrcheader, sizeof(ccm_header));

		mapspace = allocate(MapFormat_PLAYFIELD);

		++psrcheader;

//		s32 *psrcmap = (s32*)(((u8*)passet) + sizeof(ccm_header));
		s32 *psrcmap = (s32*)psrcheader;

		qmemcpy(map_, psrcmap, mapspace * sizeof(s32));

		efree(passet);
	}
	else
	{
		// not wrapped - read directly, in steps

		FILE* h = fopen(_filename, "rb");
		if (h != 0)
		{
			// read the header
			fread(&header_, 1, sizeof(ccm_header), h);

			// older format was emitted little-endian, so we just detect it
			// and reformat if needed
			// todo: deprecate the old format, remove the swappage
			//bool le = (header_.width & 0xFFFF0000);
			//if (le)
			//{
			//	endianswap32((u32*)&header_.width, 1); 
			//	endianswap32((u32*)&header_.height, 1); 
			//}

			mapspace = allocate(MapFormat_PLAYFIELD);
									
			// read main part of the file
			fread(map_, 1, mapspace * sizeof(s32), h);
			// done
			fclose(h);

			// older format was emitted little-endian, so this is retained
			//if (le)
			//	endianswap32((u32*)map_, mapspace); 
		}
	}

	if (mapspace)
	{
		pprintf("created ccm map: %dx%d tiles\n", (int)header_.width, (int)header_.height);
		prepare_playfield(mapspace);
		success = true;
	}
	else
	{
		eprintf("error: opening ccm map: [%s]\n", _filename);
	}

	return success;
}

bool worldmap::load_csv(const char* const _filename, s16 _w, s16 _h, MapFormat _format)
{
	purge();

	bool success = false;

	//size_t rawsize = fsize((char*)_filename);
	//if (rawsize)
	{
		reg8(a4) = 1;

		FILE* fh = fopen(_filename, "rb");
		if (fh != 0)
		{
			size_t rawsize = 0;
			fseek(fh, 0, SEEK_END);
			rawsize = ftell(fh);
			fseek(fh, 0, SEEK_SET);

			header_.width = _w;
			header_.height = _h;

			s32 mapspace = allocate(_format);
	
			char *rawdata = nullptr;
			if (mapspace)
			{
				pprintf("created csv map: %dx%d tiles\n", (int)header_.width, (int)header_.height);

				rawdata = (char*)talloc(rawsize);
				fread(rawdata, 1, rawsize, fh);
			}

			// done
			fclose(fh);

			reg8(a4) = 0;

			if (mapspace)
			{
				int i, j;

				// alloc row scratch area
				char *scratch = (char*)talloc(32768);

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

			if (mapspace)
			{
				if (_format == MapFormat_PLAYFIELD)
					prepare_playfield(mapspace);

				success = true;
			}
		}
		else
		{
			reg8(a4) = 0;
			eprintf("error: opening csv map: [%s] @ %dx%d tiles\n", _filename, _w, _h);
		}
	}
	//else
	//{
	//	eprintf("error: sizing csv map: [%s] @ %dx%d tiles\n", _filename, _w, _h);
	//}

	return success;
}	

s32 worldmap::allocate(MapFormat _format)
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

	map_ = mapalloc_ = new s32[allocspace];

	if (!mapalloc_)
		mapspace = 0;

	return mapspace;
}

void worldmap::prepare_playfield(s32 _mapspace)
{
	int tilestep = header_.tilestep;
	if (tilestep == 0)
		tilestep = 4;

	int overlay = 0;
	if (header_.flags & 8)
		overlay = 1;

	int preshift = (tilestep + tilestep + 2) - 3 + overlay;

	// we now preshift for 16x16 or 8x8 tiles since other bits are unused (so far - can be masked later)
	for (s32 m = 0; m < _mapspace; m++)
	{
		map_[m] <<= preshift;
	}

	// fill additional rows with copy of map data, to catch map indexing overruns when 
	// VScroll is enabled but map is smaller than virtual page size + vscroll margin
	qmemcpy(&map_[_mapspace], &map_[0], _mapspace*sizeof(s32));
}

// ---------------------------------------------------------------------------------------------------------------------
