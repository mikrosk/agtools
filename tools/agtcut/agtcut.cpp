//---------------------------------------------------------------------------------------------------------------------
//	[A]tari [G]ame [T]ools / dml 2016
//---------------------------------------------------------------------------------------------------------------------
//	AGTCut - graphics cutting tool
//---------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------
//	platform stuff - todo: OSX/Linux porting!
// --------------------------------------------------------------------

#define PREFER_TRANSFORMS (1) // hacks for testing
#define USE_MOVEQ_INITS (0) // hacks for testing

#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#endif

#include "stdio.h"

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

// --------------------------------------------------------------------
//	STL
// --------------------------------------------------------------------

#include <list>
#include <vector>
#include <map>

// --------------------------------------------------------------------
//	stb_image! nice :)
// --------------------------------------------------------------------

#define STB_IMAGE_IMPLEMENTATION
#include "../3rdparty/stb_image.h"

#define BMP_TYPICAL_ROW_ORDER (1)		// 0 = -ve height, +ve ordering (easier to parse back in)
										// 1 = +ve height, -ve ordering (more common)
#include "../common/rnd.h"
#include "../common/csvreader.h"

#include "../../static/tiny6x6fnt.c"

//static const int USE_LINEDUPE_OPTIMIZATION = 1;
static const int USE_DELTA_OPTIMIZATION = 1;

// single/common optimized line ordering shared by all preshifts of a given frame
static const int USE_COMMON_LINEORDER_OPTIMIZATION = 1;

// USE_EMPTYLINE_OPTIMIZATION can *only* work properly with USE_COMMON_LINEORDER_OPTIMIZATION for sprites which can be clipped,
// since there's no code provision for skipping of source data lines and all lines get emitted for indexed EMS clipping case.
// the USE_COMMON_LINEORDER_OPTIMIZATION phase will move any empty lines to the end of the render to avert the need to skip them.
static const int USE_EMPTYLINE_OPTIMIZATION = 1;

// per-preshift line ordering - implies per-preshift colourdata ordering as well (too much bloat)
//static const int USE_PERSHIFT_LINEORDER_OPTIMIZATION = 0;

//static const int DARWIN_DEPTH = 20;
static const int DEFAULT_EMX_OPTLEVEL = 20;

#define USE_NOP_BINARY_VALIDATION (0)

// --------------------------------------------------------------------
//	local types
// --------------------------------------------------------------------

typedef unsigned int datasize_t;

typedef int long_t;
typedef short word_t;
typedef signed char byte_t;

typedef unsigned int ulong_t;
typedef unsigned short uword_t;
typedef unsigned char ubyte_t;

// alternates, for working with engine types
typedef int s32;
typedef short s16;
typedef signed char s8;

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef unsigned long long u64;

// meh: this arrived with BMP and lingers on...

typedef struct RGB
{
    ubyte_t blue;
    ubyte_t green;
    ubyte_t red;
} RGB;

typedef struct iRGB
{
    int blue;
    int green;
    int red;
} iRGB;

typedef struct spritemapping_s
{
	int to;
	int from;
	std::string flags;

	int x;
	int y;

} spritemapping_t;

typedef std::map<int, spritemapping_t> spritemap_t;

// --------------------------------------------------------------------
//	constants
// --------------------------------------------------------------------

// tool version
static const char* const g_version_string = "v0.4";

// SPS file format version
static const int SPS_VERSION = 0x0100;

static const int SLS_VERSION = 0x1100;

// --------------------------------------------------------------------
//	some ...globals (?!)
// --------------------------------------------------------------------

static int s_progress = 0;
static int g_maxplanes = 4;

static ubyte_t *g_colrbuffer8 = 0;
static uword_t *g_colrbufferbpl = 0;

static ubyte_t *g_maskbuffer8 = 0;
static uword_t *g_maskbufferbpl = 0;

static ubyte_t *g_workbuffer8 = 0;
static uword_t *g_workbufferbpl = 0;

// --------------------------------------------------------------------
// SimpleOpt... because life is too short
// --------------------------------------------------------------------

#if defined(_MSC_VER)
# include <windows.h>
# include <tchar.h>
#else
# define TCHAR		char		// yuck
# define _T(x)		x
#endif

#include "../3rdparty/SimpleOpt.h"
#include "../3rdparty/SimpleGlob.h"

// --------------------------------------------------------------------
//	misc. small helper functions
// --------------------------------------------------------------------

//#undef assert
#define xassert(_x_) \
{ \
	if (!(_x_)) \
	{ \
		__asm { int 3 }; \
	}; \
}

static inline uword_t endianswap16(const uword_t _v)
{
	return ((_v & 0xFF00) >> 8) | ((_v & 0x00FF) << 8);
}

static inline ulong_t endianswap32(const ulong_t _v)
{
	return  ((_v & 0xFF000000) >> 24) |
			((_v & 0x00FF0000) >> 8) |
			((_v & 0x0000FF00) << 8) |
			((_v & 0x000000FF) << 24);
}

static inline int xmin(int _x, int _y)
{
	return (_x < _y) ? _x : _y;
}

static inline int xmax(int _x, int _y)
{
	return (_x > _y) ? _x : _y;
}


ulong_t rofl32(ulong_t _v, size_t _bits)
{
    return (_v >> _bits) | (_v << ((sizeof(_v) << 3) - _bits));
}

ulong_t lofl32(ulong_t _v, size_t _bits)
{
    return (_v << _bits) | (_v << ((sizeof(_v) << 3) - _bits));
}

uword_t rofl16(uword_t _v, size_t _bits)
{
    return (_v >> _bits) | (_v << ((sizeof(_v) << 3) - _bits));
}

uword_t lofl16(uword_t _v, size_t _bits)
{
    return (_v << _bits) | (_v << ((sizeof(_v) << 3) - _bits));
}


static void replace_extension(std::string &_fname, const char* const _ext)
{
	size_t dot = _fname.find_last_of(".");
	if (dot != std::string::npos)
	{
		_fname = _fname.substr(0, dot+1);
		_fname.append(_ext);
	}
	else
	{
		_fname.append(".");
		_fname.append(_ext);
	}
}

static void remove_extension(std::string &_fname)
{
	size_t dot = _fname.find_last_of(".");
	if (dot != std::string::npos)
		_fname = _fname.substr(0, dot);
}

inline char sanitise_csv_integer(char ch)
{
	unsigned char uc = (unsigned char)ch;
	if ((uc >= '0' && uc <= '9') ||
		(uc >= 'a' && uc <= 'f') ||
		(uc >= 'A' && uc <= 'F') ||
		(uc == '-') ||
		(uc == 'x'))
		return ch;

	return ' ';
}

static void plot6x6char(RGB *_buf, int _bufw, int _c, int _x, int _y)
{
	u8 *fnt = &tiny6x6fnt[_c * 6];
	RGB *out = &_buf[(_y * _bufw) + _x];

	for (int l = 0; l < 6; l++)
	{
		u8 b = *fnt++;

		for (int p = 0; p < 6; p++)
		{
			s8 v = (b & 0x80); v >>= 7;
			b <<= 1;
			out[p].red		|= v;
			out[p].green	|= v;
			out[p].blue		|= v;
		}

		out += _bufw;
	}
}

static void plot6x6text(RGB *_buf, int _bufw, char *_str, int _x, int _y)
{
	int c;
	while (0 != (c = *_str++))
	{
		plot6x6char(_buf, _bufw, c, _x, _y);
		_x += 5;
	}
}

static void plot6x6hex3(RGB *_buf, int _bufw, int _v, int _x, int _y)
{
	char numbuf[4] = { 0, 0, 0 };
	int n;

	for (int i = 0; i < 3; i++)
	{
		if (_v == 0)
		{
			n = ' ';
		}
		else
		{
			n = (_v & 0xF); _v >>= 4;

			if (n < 10)
				n += '0';
			else
				n += 'A' - 10;
		}

		numbuf[2-i] = n;
	}

	plot6x6text(_buf, _bufw, numbuf, _x, _y);
}

// --------------------------------------------------------------------
//	interface to engine tileset reader, for access to palettes
// --------------------------------------------------------------------

// todo: move reader/writer out of tool generally!
//#include "tileset.h"

// --------------------------------------------------------------------
//	commandline handling
// --------------------------------------------------------------------

// define the ID values to indentify the option
enum
{ 
	OPT_HELP, 
	OPT_VERBOSE, 
	OPT_QUIET, 
	OPT_WAIT_KEY, 
	// general
	OPT_BITPLANES,
	OPT_KEYCOL,
	OPT_KEYRGB,
	OPT_KEYGUARD,
	OPT_KEYTOL,
	OPT_SOURCE,
	OPT_OUTFILE,
	OPT_TILETAGS,
	OPT_PALETTE,
	OPT_COLRENORM,
	OPT_CUTMODE,
	OPT_CCFIELDS,
	OPT_CCREMAP,
	OPT_NOREMAP,
	OPT_FORCEREMAP,
	// sprites
	OPT_EMXOPT,
	OPT_SPRITEGUIDE,
	OPT_QUICKCUT,
	OPT_VIS,
	OPT_PREVIEW,
	OPT_GENMASK,
	OPT_NOGENMASK,
	OPT_MASKLAYOUT,
	OPT_SPROPT,
	OPT_SPRXP,
	OPT_SPRYP,
	OPT_SPRXS,
	OPT_SPRYS,
	OPT_SPRXI,
	OPT_SPRYI,
	OPT_SPRXC,
	OPT_SPRYC,
	OPT_SPRCOUNT,
	OPT_SPRMAP,
//	OPT_SLAB,
	// tiles
	OPT_MAPRECT,
	OPT_TILESIZE,
	OPT_TOLERANCE, 
	OPT_OUTMODE
	//
};

CSimpleOpt::SOption g_Options[] = 
{
	{ OPT_HELP, 		_T("-?"),     			SO_NONE		},
	{ OPT_HELP, 		_T("-h"),     			SO_NONE		},
	{ OPT_HELP, 		_T("--help"), 			SO_NONE		},
	//
	{ OPT_VERBOSE, 		_T("-v"),     			SO_NONE		},
	{ OPT_VERBOSE,		_T("--verbose"), 		SO_NONE		},
	//
	{ OPT_QUIET, 		_T("-q"),     			SO_NONE		},
	{ OPT_QUIET,		_T("--quiet"), 			SO_NONE		},
	//
	{ OPT_WAIT_KEY,		_T("-wk"),    			SO_NONE		},
	{ OPT_WAIT_KEY,		_T("--wait-key"), 		SO_NONE		},
	//
	{ OPT_PREVIEW,		_T("-prv"),     		SO_NONE		},
	{ OPT_PREVIEW,		_T("--preview"), 		SO_NONE		},
	//
	{ OPT_GENMASK,		_T("-gm"),	 			SO_NONE		},
	{ OPT_GENMASK,		_T("--gen-mask"),		SO_NONE		},
	//
	{ OPT_NOGENMASK,	_T("-nogm"),	 		SO_NONE		},
	{ OPT_NOGENMASK,	_T("--no-gen-mask"),	SO_NONE		},
	//
	{ OPT_MASKLAYOUT,	_T("-ml"),	 			SO_REQ_SEP	},
	{ OPT_MASKLAYOUT,	_T("--mask-layout"),	SO_REQ_SEP	},
	//
	{ OPT_EMXOPT,		_T("-emxopt"),	 		SO_REQ_SEP	},
	{ OPT_EMXOPT,		_T("--emx-opt-level"),	SO_REQ_SEP	},
	//
	{ OPT_KEYCOL,		_T("-key"),	 			SO_REQ_SEP	},
	{ OPT_KEYCOL,		_T("--key-colour"),		SO_REQ_SEP	},
	//
	{ OPT_KEYRGB,		_T("-keyrgb"), 			SO_REQ_SEP	},
	{ OPT_KEYRGB,		_T("--key-rgb"),		SO_REQ_SEP	},
	//
	{ OPT_KEYGUARD,		_T("-kg"),	 			SO_NONE		},
	{ OPT_KEYGUARD,		_T("--key-guard"),		SO_NONE		},
	//
	{ OPT_KEYGUARD,		_T("-kt"),	 			SO_REQ_SEP	},
	{ OPT_KEYGUARD,		_T("--key-tol"),		SO_REQ_SEP	},
	//
	{ OPT_SOURCE,		_T("-s"),	 			SO_REQ_SEP	},
	{ OPT_SOURCE,		_T("--src"), 			SO_REQ_SEP	},
	//
	{ OPT_CUTMODE,		_T("-cm"),	 			SO_REQ_SEP	},
	{ OPT_CUTMODE,		_T("--cutmode"), 		SO_REQ_SEP	},
	//
	{ OPT_CCFIELDS,		_T("-ccf"),	 			SO_REQ_SEP	},
	{ OPT_CCFIELDS,		_T("--ccfields"), 		SO_REQ_SEP	},
	//
	{ OPT_CCREMAP,		_T("-ccr"),	 			SO_REQ_SEP	},
	//
	{ OPT_NOREMAP,		_T("-nr"),	 			SO_NONE		},
	{ OPT_NOREMAP,		_T("--no-remap"),		SO_NONE		},
	//
	{ OPT_FORCEREMAP,	_T("-fr"),	 			SO_NONE		},
	{ OPT_FORCEREMAP,	_T("--force-remap"),	SO_NONE		},
	//
	{ OPT_SPRITEGUIDE,	_T("-sg"),	 			SO_REQ_SEP	},
	{ OPT_SPRITEGUIDE,	_T("--sprguide"), 		SO_REQ_SEP	},
	//
	{ OPT_MAPRECT,		_T("-mr"),	 			SO_REQ_SEP	},
	{ OPT_MAPRECT,		_T("--maprect"), 		SO_REQ_SEP	},
	//
	{ OPT_QUICKCUT,		_T("-qc"),	 			SO_NONE	},
	{ OPT_QUICKCUT,		_T("--quickcut"), 		SO_NONE	},
	//
	{ OPT_VIS,			_T("-vis"),	 			SO_NONE	},
	{ OPT_VIS,			_T("--visual"), 		SO_NONE	},
	//
	{ OPT_SPROPT,		_T("-sopt"), 			SO_REQ_SEP	},
	{ OPT_SPROPT,		_T("--soptimize"),		SO_REQ_SEP	},
	{ OPT_SPROPT,		_T("--soptimise"),		SO_REQ_SEP	},
	//
	{ OPT_SPRXP,		_T("-sxp"),	 			SO_REQ_SEP	},
	{ OPT_SPRXP,		_T("--sprxpos"),		SO_REQ_SEP	},
	//
	{ OPT_SPRYP,		_T("-syp"),	 			SO_REQ_SEP	},
	{ OPT_SPRYP,		_T("--sprypos"),		SO_REQ_SEP	},
	//
	{ OPT_SPRXS,		_T("-sxs"),	 			SO_REQ_SEP	},
	{ OPT_SPRXS,		_T("--sprxsize"),		SO_REQ_SEP	},
	//
	{ OPT_SPRYS,		_T("-sys"),	 			SO_REQ_SEP	},
	{ OPT_SPRYS,		_T("--sprysize"),		SO_REQ_SEP	},
	//
	{ OPT_SPRXI,		_T("-sxi"),	 			SO_REQ_SEP	},
	{ OPT_SPRXI,		_T("--sprxinc"),		SO_REQ_SEP	},
	//
	{ OPT_SPRYI,		_T("-syi"),	 			SO_REQ_SEP	},
	{ OPT_SPRYI,		_T("--spryinc"),		SO_REQ_SEP	},
	//
	{ OPT_SPRXC,		_T("-sxc"),	 			SO_REQ_SEP	},
	{ OPT_SPRXC,		_T("--sprxcount"),		SO_REQ_SEP	},
	//
	{ OPT_SPRYC,		_T("-syc"),	 			SO_REQ_SEP	},
	{ OPT_SPRYC,		_T("--sprycount"),		SO_REQ_SEP	},
	//
	{ OPT_SPRCOUNT,		_T("-sc"),	 			SO_REQ_SEP	},
	{ OPT_SPRCOUNT,		_T("--sprcount"),		SO_REQ_SEP	},
	//
	{ OPT_SPRMAP,		_T("-sm"),	 			SO_REQ_SEP	},
	{ OPT_SPRMAP,		_T("--sprmap"),			SO_REQ_SEP	},
	//
	//{ OPT_SLAB,			_T("-sl"),	 			SO_NONE		},
	//{ OPT_SLAB,			_T("--slab"),			SO_NONE		},
	//
	{ OPT_TILESIZE,		_T("-ts"),	 			SO_REQ_SEP	},
	{ OPT_TILESIZE,		_T("--tilesize"),		SO_REQ_SEP	},
	//
	{ OPT_TOLERANCE,	_T("-t"),	 			SO_REQ_SEP	},
	{ OPT_TOLERANCE,	_T("--tol"), 			SO_REQ_SEP	},
	//
	{ OPT_OUTMODE,		_T("-om"),	 			SO_REQ_SEP	},
	{ OPT_OUTMODE,		_T("--outmode"), 		SO_REQ_SEP	},
	//
	{ OPT_OUTFILE,		_T("-o"),	 			SO_REQ_SEP	},
	{ OPT_OUTFILE,		_T("--outfile"),		SO_REQ_SEP	},
	//
	{ OPT_TILETAGS,		_T("-ttags"), 			SO_REQ_SEP	},
	{ OPT_TILETAGS,		_T("--tiletags"),		SO_REQ_SEP	},
	//
	{ OPT_PALETTE,		_T("-p"),	 			SO_REQ_SEP	},
	{ OPT_PALETTE,		_T("--pal"),	 		SO_REQ_SEP	},
	//
	{ OPT_COLRENORM,	_T("-crn"),	 			SO_REQ_SEP	},
	{ OPT_COLRENORM,	_T("--colrenorm"), 		SO_REQ_SEP	},
	//
	{ OPT_BITPLANES,	_T("-bp"),	 			SO_REQ_SEP	},
	{ OPT_BITPLANES,	_T("--bitplanes"), 		SO_REQ_SEP	},
	//
	SO_END_OF_OPTIONS
};

static void usage(int level)
{
	if (level >= 1) 
	{
		//std::cout << g_version_string << std::endl;
		std::cout << "built on " << __DATE__ << std::endl;
	}
		
	std::cout << "usage:" << std::endl;
	std::cout << " atgcut [opts] -s infile" << std::endl;
	std::cout << std::endl;
	std::cout << " -?, -h, --help       > show help/usage" << std::endl;
	std::cout << " -v, --verbose        > more output" << std::endl;
	std::cout << " -q, --quiet          > less output" << std::endl;
	std::cout << " -wk, --wait-key      > wait key on exit" << std::endl;
	std::cout << std::endl;
	std::cout << "general:" << std::endl;
	std::cout << " -s, --src            > source image containing tiles/sprites/slabs:" << std::endl;
	std::cout << "                      > pre-mapped: png,bmp (i.e. use indexed 4/8bit colour w/no remapping)" << std::endl;
	std::cout << "                      > unmapped:   png,bmp,jpg,psd,tga,gif,hdr,pic,ppm/pgm/pnm" << std::endl;
	std::cout << " -o, --outfile        > output filename" << std::endl;
	std::cout << " -cm, --cutmode       > cutting mode: tiles,imspr,emspr,emxspr,slabs,slabrestore" << std::endl;
	std::cout << " -ccf, --ccfields     > cryptochrome fields: 1 or 2 (for dual-field gfx)" << std::endl;
	std::cout << " -p, --pal            > fixed palette to use: pi1,pal(riff),bmp,png" << std::endl;
	std::cout << " -crn, --colrenorm    > renormalise source image to quantised rgb: 1-255" << std::endl;
	std::cout << " -bp, --bitplanes     > #bitplanes to generate: 1-8" << std::endl;
	std::cout << " -key, --key-colour   > colour index assumed to be transarent: 0-255" << std::endl;
	std::cout << " -keyrgb, --key-rgb   > rgb value assumed to be transarent: <RR:GG:BB> in hex" << std::endl;
	std::cout << " -kg, --key-guard     > guard against reducing colours to key colour" << std::endl;
	std::cout << " -kt, --key-tol       > key guard tolerance: 0.0+ (default: 0.01)" << std::endl;
	std::cout << " -nr, --no-remap      > prevent remapping of 8bit images (to 4bit), use first 16 colours only" << std::endl;
	std::cout << " -fr, --force-remap   > force remapping of 8bit images (to 4bit) so --key-rgb is usable" << std::endl;
	std::cout << std::endl;
	std::cout << "sprite cutting mode:" << std::endl;
	std::cout << " -sxp, --sprxpos      > first frame xpos: 0+" << std::endl;
	std::cout << " -syp, --sprypos      > first frame ypos: 0+" << std::endl;
	std::cout << " -sxs, --sprxsize     > all frames xsize: 1+" << std::endl;
	std::cout << " -sys, --sprysize     > all frames ysize: 1+" << std::endl;
	std::cout << " -sxi, --sprxinc      > x step for next frame in row: 0+" << std::endl;
	std::cout << " -syi, --spryinc      > y step for next row: 0+" << std::endl;
	std::cout << " -sxc, --sprxcount    > frames per row: 1+" << std::endl;
	std::cout << " -syc, --sprycount    > frames per column: 1+" << std::endl;
	std::cout << " -sc, --sprcount      > total frames to cut, if stopping early: 0+, where 0=ignore" << std::endl;
	std::cout << " -sm, --sprmap        > map sprite frame from another: -sm new:existing:flipflags" << std::endl;
	std::cout << " -gm, --gen-mask      > generate 1bpl mask data along with colour data" << std::endl;
	std::cout << " -nogm, --no-gen-mask > don't generate 1bpl mask data - only colour data (STF)" << std::endl;
	std::cout << " -ml, --mask-layout   > mask layout strategy: interleaved,planar,preshift" << std::endl;
	std::cout << " -sopt, --soptimize   > optimize away unused pixels: on/off (compensated w/offsets)" << std::endl;
	std::cout << " -qc, --quickcut      > improve cutting speed (reduce optimization) for quicker test/debug turnaround" << std::endl;
	std::cout << " -vis, --visual       > provide feedback/visualisation of cut assets as .tga file" << std::endl;
	std::cout << " -ccr                 > cryptochrome colour field remapping: both,high,low" << std::endl;
	std::cout << " -prv, --preview      > show ASCII preview of extracted colours + masking" << std::endl;
	std::cout << " -sg, --sprguide      > sprite guide file (ASCII)" << std::endl;
	std::cout << " -emxopt, --emx-opt   > optimization level for EMX mask delta elimination" << std::endl;
	std::cout << std::endl;
	std::cout << "map & tile cutting mode:" << std::endl;
	std::cout << " -ts, --tilesize      > tile size (rbplus mode only): 8,16,32" << std::endl;
	std::cout << " -ttags, --tiletags   > list of tile indices to tag (CSV file - one hex value per row)" << std::endl;
	std::cout << " -t, --tol            > tile matching tolerance: 0.0+" << std::endl;
	std::cout << " -mr, --maprect       > map rectangle to extract (-mr x:y:w:h ..or.. -mr x:y)" << std::endl;
	std::cout << " -om, --outmode       > tile library output mode: direct,degas,rbplus" << std::endl;
	std::cout << std::endl;
	std::cout << "press any key..." << std::endl;
		
	getchar();	
	exit(1);
}

// --------------------------------------------------------------------
//	general argument handling
// --------------------------------------------------------------------


enum CCRemap
{
	CCRemap_BOTH,
	CCRemap_HIGH,
	CCRemap_LOW
};

enum Outmode
{
	Outmode_DIRECT,
	Outmode_DEGAS,
	Outmode_RBPLUS
};

enum Cutmode
{
	Cutmode_TILES,
	Cutmode_SPRITES,			// standard sprites (software w/o mask, or blitter w/ mask)
	Cutmode_EMSPR,				// <= 32 wide EM blitter sprites (preshifted masks)
	Cutmode_EMXSPR,				// <= 32 wide EM eXtreme blitter sprites (preshifted codegen)
	Cutmode_SLABS,
	Cutmode_SLABRESTORE,
};

enum Masklayout
{
	Masklayout_INTERLEAVED,
	Masklayout_PLANAR,
	Masklayout_PRESHIFT,
	Masklayout_IMPLIED,
};

// sprite guide cutting event, extracted from .sgd text file
class guideevent
{
public:

	guideevent(int _count, std::string _flags, int _xp, int _yp, int _xs, int _ys, int _xi, int _yi)
		: count(_count), flags(_flags), xp(_xp), yp(_yp), xs(_xs), ys(_ys), xi(_xi), yi(_yi)
	{
	}

	int count;
	std::string flags;
	int xp, yp;
	int xs, ys;
	int xi, yi;
};

class Arguments
{
public:

	Arguments()
		: verbose(false),
		  quiet(false),
		  waitkey(false),
		  spriteoptimize(true),
		  preview(false),
		  genmask(true),
		  keyguard(false),
		  quickcut(false),
		  visual(false),
		  keyrgb_specified(false),
		  colrenorm(255),
		  tolerance(0.0f),
		  keytol(0.01f),
		  //sourcefile(""),
		  //outfile(""),
		  //tiletags(""),
		  palettefile("palette.pi1"),
		  //spriteguidefile(""),
		  ccfields(1),
		  ccremap(CCRemap_BOTH),
		  masklayout(Masklayout_INTERLEAVED),
		  cutmode(Cutmode_TILES),
		  outmode(Outmode_DEGAS),
		  noremap(false),
		  forceremap(false),
		  masklayout_lock(false),
		  planes(4),
		  tilesize(16),
		  emxoptlevel(DEFAULT_EMX_OPTLEVEL),
		  keycol(255),
		  cut_xpos(0),
		  cut_ypos(0),
		  cut_xsize(16),
		  cut_ysize(16),
		  cut_xinc(0),
		  cut_yinc(0),
		  cut_xcount(0),
		  cut_ycount(0),
		  cut_count(0),
		  maprect_xpos(0),
		  maprect_ypos(0),
		  maprect_xsize(0x7fffffff),
		  maprect_ysize(0x7fffffff),
		  //
		  use_solutionpair_reduction_(false)
	{
		keyrgb.red = 255;
		keyrgb.green = 0;
		keyrgb.blue = 255;
	}

	bool verbose;
	bool quiet;
	bool waitkey;
	bool spriteoptimize;
	bool preview;
	bool genmask;
	bool keyguard;
	bool quickcut;
	bool visual;
	bool keyrgb_specified;

	ubyte_t keycol;
	iRGB keyrgb;
	int colrenorm;
	int cut_xpos;
	int cut_ypos;
	int cut_xsize;
	int cut_ysize;
	int cut_xinc;
	int cut_yinc;
	int cut_xcount;
	int cut_ycount;
	int cut_count;
	int maprect_xpos;
	int maprect_ypos;
	int maprect_xsize;
	int maprect_ysize;
	int emxoptlevel;

	float tolerance;
	float keytol;
	std::string sourcefile;
	std::string outfile;
	std::string tiletags;
	int ccfields;
	std::string spriteguidefile;
	Cutmode cutmode;
	Outmode outmode;
	Masklayout masklayout;
	bool masklayout_lock;
	CCRemap ccremap;
	std::string palettefile;
	int planes;
	int tilesize;
	std::string invokehash;
	std::string mactmp;
	bool noremap;
	bool forceremap;

	std::list<guideevent> guideeventseq;

	spritemap_t spritemappings;

	// internal: not set via commandline
	bool use_solutionpair_reduction_;
};

Arguments g_allargs;

static ulong_t strqhash(ulong_t _h, char* _str)
{
	ulong_t h = _h;

	int p = 0;
	ubyte_t c = 0;
	while (c = _str[p])
	{
		h -= rofl32(h, 7);
		h *= 0x12967ad1;
		h += rofl32(h, 17);
		h = (h + 0x3869b5a2) * 0x72b1d643;
		p++;
	}

	return h;
}

static void GetArguments(int _argc, char **_argv, Arguments& _args)
{
    CSimpleOpt args(_argc, _argv, g_Options);
	std::string s_outfile;

	// create a hash of all arguments e.g. for tempfile specialization in parallel builds
	// this is more stable/repeatable for debugging vs using time or PID or some other flaky source
	{
		char buf[16];
		ulong_t h = 0x194bca85;
		for (int a = 0; a < _argc; a++)
		{
			if (_argv[a])
				h = strqhash(h, _argv[a]);
		}
		sprintf(buf, "%08x", h);
		_args.invokehash = buf;
	}

	_args.mactmp = "tmp68k_";
	_args.mactmp += _args.invokehash;

    // while there are arguments left to process
    while (args.Next()) 
    {
		if (args.LastError() == SO_SUCCESS) 
		{
			if (args.OptionId() == OPT_HELP) 
			{
				usage(1);
			}
			else if (args.OptionId() == OPT_VERBOSE) 
			{
				_args.verbose = true;
			}
			else if (args.OptionId() == OPT_QUIET) 
			{
				_args.quiet = true;
			}
			else if (args.OptionId() == OPT_WAIT_KEY) 
			{
				_args.waitkey = true;
			}			
			else if (args.OptionId() == OPT_PREVIEW) 
			{
				_args.preview = true;
			}
			else if (args.OptionId() == OPT_GENMASK) 
			{
				_args.genmask = true;
			}
			else if (args.OptionId() == OPT_NOGENMASK) 
			{
				_args.genmask = false;
			}	
			else if (args.OptionId() == OPT_KEYGUARD) 
			{
				_args.keyguard = true;
			}	
			//
			else if (args.OptionId() == OPT_SOURCE)
			{
				_args.sourcefile = args.OptionArg();
			}
			else if (args.OptionId() == OPT_OUTFILE)
			{
				_args.outfile = args.OptionArg();
			}
			else if (args.OptionId() == OPT_TILETAGS)
			{
				_args.tiletags = args.OptionArg();
			}
			else if (args.OptionId() == OPT_TOLERANCE)
			{
				std::string s_arg = args.OptionArg();
				float farg = (float)atof(s_arg.c_str());
				if (farg < 0.0f || farg > 1e4f)
				{
					printf("error: invalid arg for tile matching tolerance: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
				_args.tolerance = farg;	
			}
			else if (args.OptionId() == OPT_KEYTOL)
			{
				std::string s_arg = args.OptionArg();
				float farg = (float)atof(s_arg.c_str());
				if (farg < 0.0f || farg > 1e4f)
				{
					printf("error: invalid arg for key guard tolerance: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
				_args.keytol = farg;	
			}
			else if (args.OptionId() == OPT_CUTMODE)
			{
				std::string s_arg = args.OptionArg();

				if (s_arg.find("tiles") != std::string::npos)
				{
					_args.cutmode = Cutmode_TILES;
				}
				else
				if ((s_arg.find("imspr") != std::string::npos) || 
					(s_arg.find("sprites") != std::string::npos))
				{
					_args.cutmode = Cutmode_SPRITES;
				}
				else
				if (s_arg.find("emspr") != std::string::npos)
				{
					_args.cutmode = Cutmode_EMSPR;
					_args.masklayout = Masklayout_PRESHIFT;
					_args.masklayout_lock = true;
					_args.genmask = true;
				}
				else
				if (s_arg.find("emxspr") != std::string::npos)
				{
					_args.cutmode = Cutmode_EMXSPR;
					_args.masklayout = Masklayout_PRESHIFT;
					_args.masklayout_lock = true;
					_args.genmask = true;
				}
				else
				if (s_arg.find("slabs") != std::string::npos)
				{
					_args.cutmode = Cutmode_SLABS;
					_args.masklayout = Masklayout_IMPLIED;
					_args.masklayout_lock = true;
				}
				else
				if (s_arg.find("slabrestore") != std::string::npos)
				{
					_args.cutmode = Cutmode_SLABRESTORE;
					_args.masklayout = Masklayout_IMPLIED;
					_args.masklayout_lock = true;
				}
				else
				{
					printf("error: invalid arg for cutmode: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
			}
			else if (args.OptionId() == OPT_OUTMODE)
			{
				std::string s_arg = args.OptionArg();

				if (s_arg.find("degas") != std::string::npos)
				{
					_args.outmode = Outmode_DEGAS;
					g_maxplanes = 4;
				}
				else
				if (s_arg.find("direct") != std::string::npos)
				{
					_args.outmode = Outmode_DIRECT;
					g_maxplanes = 8;
				}
				else
				if (s_arg.find("rbplus") != std::string::npos)
				{
					_args.outmode = Outmode_RBPLUS;
					g_maxplanes = 8;
				}
				else
				{
					printf("error: invalid arg for outmode: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
			}
			else if (args.OptionId() == OPT_MASKLAYOUT)
			{
				std::string s_arg = args.OptionArg();

				// cutmode can dictate mask layout for some modes
				if (!_args.masklayout_lock)
				{
					if (s_arg.find("planar") != std::string::npos)
					{
						_args.masklayout = Masklayout_PLANAR;
					}
					else
					if (s_arg.find("interleaved") != std::string::npos)
					{
						_args.masklayout = Masklayout_INTERLEAVED;
					}
					else
					if (s_arg.find("preshift") != std::string::npos)
					{
						_args.masklayout = Masklayout_PRESHIFT;
					}
					else
					{
						printf("error: invalid arg for masklayout: %s\n", s_arg.c_str());
						printf("press any key...\n");
						getchar();
						exit(1);
					}
				}
			}
			else if (args.OptionId() == OPT_QUICKCUT)
			{
				_args.quickcut = true;
			}
			else if (args.OptionId() == OPT_VIS)
			{
				_args.visual = true;
			}
			else if (args.OptionId() == OPT_SPROPT)
			{
				std::string s_arg = args.OptionArg();

				if (s_arg.find("on") != std::string::npos)
				{
					_args.spriteoptimize = true;
				}
				else
				if (s_arg.find("off") != std::string::npos)
				{
					_args.spriteoptimize = false;
				}
				else
				{
					printf("error: invalid arg for sprite-optimize: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
			}
			else if (args.OptionId() == OPT_PALETTE)
			{
				_args.palettefile = args.OptionArg();
			}
			else if (args.OptionId() == OPT_SPRITEGUIDE)
			{
				_args.spriteguidefile = args.OptionArg();
			}
			else if (args.OptionId() == OPT_COLRENORM)
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 1 || iarg > 255)
				{
					printf("error: invalid arg for colrenorm: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
				_args.colrenorm = iarg;	
			}
			else if (args.OptionId() == OPT_EMXOPT)
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 0 || iarg > 31)
				{
					printf("error: invalid arg for emxopt: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
				_args.emxoptlevel = iarg;	
			}
			else if (args.OptionId() == OPT_KEYCOL)
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 0 || iarg > 255)
				{
					printf("error: invalid arg for key colour: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
				_args.keycol = iarg;	
			}
			else if (args.OptionId() == OPT_KEYRGB)
			{
				_args.keyrgb_specified = true;

				std::string s_arg = args.OptionArg();
				
				int r, g, b;
				sscanf(s_arg.c_str(), "%x:%x:%x", &r, &g, &b);

				if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
				{
					printf("error: invalid arg for key rgb: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}

				printf("key colour from rgb: r(%x):g(%x):b(%x):\n", r, g, b);

				_args.keyrgb.red = r;
				_args.keyrgb.green = g;
				_args.keyrgb.blue = b;
			}
			else if (args.OptionId() == OPT_KEYCOL)
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 0 || iarg > 255)
				{
					printf("error: invalid arg for key colour: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
				_args.keycol = iarg;	
			}
			else if (args.OptionId() == OPT_TILESIZE)
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if ((iarg != 8) && (iarg != 16) && (iarg != 32))
				{
					printf("error: invalid arg for tile size: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
				_args.tilesize = iarg;	
			}
			else if (args.OptionId() == OPT_CCFIELDS)
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if ((iarg != 1) && (iarg != 2))
				{
					printf("error: invalid arg for ccfields: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
				_args.ccfields = iarg;	
			}
			else if (args.OptionId() == OPT_CCREMAP)
			{
				std::string s_arg = args.OptionArg();

				if (s_arg.find("both") != std::string::npos)
				{
					_args.ccremap = CCRemap_BOTH;
				}
				else
				if (s_arg.find("high") != std::string::npos)
				{
					_args.ccremap = CCRemap_HIGH;
				}
				else
				if (s_arg.find("low") != std::string::npos)
				{
					_args.ccremap = CCRemap_LOW;
				}
				else
				{
					printf("error: invalid arg for ccr: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
			}
			else if (args.OptionId() == OPT_NOREMAP)
			{
				_args.noremap = true;
			}
			else if (args.OptionId() == OPT_FORCEREMAP)
			{
				_args.forceremap = true;
			}
			else if (args.OptionId() == OPT_MAPRECT)
			{
				std::string s_arg = args.OptionArg();
				
				// try extracting all 4 fields of cutting rectangle
				int mx = 0, my = 0, mw = 0x7fffffff, mh = 0x7fffffff;
				int count = sscanf(s_arg.c_str(), "%d:%d:%d:%d", &mx, &my, &mw, &mh);
				if (count < 4)
				{
					// otherwise try just x,y
					mw = 0x7fffffff;
					mh = 0x7fffffff;
					count = sscanf(s_arg.c_str(), "%d:%d", &mx, &my);
				}

				if ((count < 2) ||
					(mx < 0) || 
					(my < 0) ||
					(mw < 0) ||
					(mh < 0))
				{
					printf("error: invalid map cutting rectangle: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}

				if (count == 2)
					printf("cutting map sub-rectangle from tilemap (dimensions in tiles): x(%d):y(%d)\n",
						mx, my);
				else
					printf("cutting map sub-rectangle from tilemap (dimensions in tiles): x(%d):y(%d):w(%d):h(%d)\n",
						mx, my, mw, mh);
			

				_args.maprect_xpos = mx;
				_args.maprect_ypos = my;
				_args.maprect_xsize = mw;
				_args.maprect_ysize = mh;
			}
			else if (args.OptionId() == OPT_BITPLANES)
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 0 || iarg > g_maxplanes)
				{
					printf("error: invalid arg for bitplanes/bpp: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
				_args.planes = iarg;	
			}
			else if (args.OptionId() == OPT_SPRXP)
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 0 || iarg > 4096)
				{
					printf("error: invalid arg for sprite cutting position: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
				_args.cut_xpos = iarg;	
			}
			else if (args.OptionId() == OPT_SPRYP)
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 0 || iarg > 4096)
				{
					printf("error: invalid arg for sprite cutting position: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
				_args.cut_ypos = iarg;	
			}
			else if (args.OptionId() == OPT_SPRXS)
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 1 || iarg > 4096)
				{
					printf("error: invalid arg for sprite cutting size: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
				if ((_args.cutmode == Cutmode_SPRITES) && (iarg > 128))
				{
					printf("warning: sprite cutting width %s exceeds max. of 128 for AGT sprite system!\n", s_arg.c_str());
				}
				else
				if ((_args.cutmode == Cutmode_SLABS || _args.cutmode == Cutmode_SLABRESTORE) && (iarg > 256))
				{
					printf("warning: sprite cutting width %s exceeds max. of 256 for AGT slab system!\n", s_arg.c_str());
				}
				_args.cut_xsize = iarg;	
			}
			else if (args.OptionId() == OPT_SPRYS)
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 1 || iarg > 4096)
				{
					printf("error: invalid arg for sprite cutting size: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
				_args.cut_ysize = iarg;	
			}
			else if (args.OptionId() == OPT_SPRXI)
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 0 || iarg > 4096)
				{
					printf("error: invalid arg for sprite cutting increment/step: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
				_args.cut_xinc = iarg;	
			}
			else if (args.OptionId() == OPT_SPRYI)
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 0 || iarg > 4096)
				{
					printf("error: invalid arg for sprite cutting increment/step: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
				_args.cut_yinc = iarg;	
			}
			else if (args.OptionId() == OPT_SPRXC)
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 1 || iarg > 1024)
				{
					printf("error: invalid arg for sprite cutting x-count: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
				_args.cut_xcount = iarg;	
			}
			else if (args.OptionId() == OPT_SPRYC)
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 1 || iarg > 1024)
				{
					printf("error: invalid arg for sprite cutting y-count: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
				_args.cut_ycount = iarg;	
			}
			else if (args.OptionId() == OPT_SPRCOUNT)
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 1 || iarg > 1024)
				{
					printf("error: invalid arg for sprite cutting count: %s\n", s_arg.c_str());
					printf("press any key...\n");
					getchar();
					exit(1);
				}
				_args.cut_count = iarg;	
			}
			else if (args.OptionId() == OPT_SPRMAP)
			{
				std::string s_arg = args.OptionArg();

				char flipflags[256];
				int to, from;
				sscanf(s_arg.c_str(), "%d:%d:%s", &to, &from, flipflags);

				// record each individual mapping for processing later
				{
					spritemapping_t sm;
					sm.to = to;
					sm.from = from;
					sm.flags = flipflags;
					sm.x = sm.y = 0;
					g_allargs.spritemappings[to] = sm;
				}
			}
		}
		else 
		{
			printf("error: invalid argument: %s\n", args.OptionText());
			printf("press any key...\n");
			getchar();
			exit(1);
		}
    }

	if (_args.sourcefile.length() == 0)
	{
		usage(0);
		exit(1);
	}
	
	// trounce verbosity in quickcut mode
	if (g_allargs.quickcut)
		g_allargs.verbose = false;

	// adjust counts to make sense for common scenarios with terse commandlines

	if (g_allargs.cut_count > 0)
	{
		// if count cap has been set, decide if this is a grid, row or column cutting sequence
		if (g_allargs.cut_xcount && g_allargs.cut_ycount)
		{
			// grid. leave things alone.
		}
		else
		{
			// either row, or column, or single, or error
			if (g_allargs.cut_xinc == 0 && g_allargs.cut_yinc == 0)
			{
				// single cut. leave things alone
			}
			if (g_allargs.cut_yinc == 0)
			{
				// row
				g_allargs.cut_xcount = 1024;
			}
			else
			if (g_allargs.cut_xinc == 0)
			{
				// column
				g_allargs.cut_ycount = 1024;
			}
		}
	}
	else
	{
		// otherwise make sure each grid axis has count of at least 1
		// this works for grids, single rows and single columns
		if (g_allargs.cut_xcount == 0)
			g_allargs.cut_xcount = 1;
		if (g_allargs.cut_ycount == 0)
			g_allargs.cut_ycount = 1;
	}

	// if a spriteguide file was specified, read it
	if (g_allargs.spriteguidefile.length() > 0)
	{
		std::cout << "processing sprite guide file: " << g_allargs.spriteguidefile.c_str() << std::endl;
		std::ifstream sprguide(g_allargs.spriteguidefile.c_str());
		if (!(sprguide.is_open() && sprguide.good()))
		{
			std::cerr << "error: unable to locate or read specified spriteguide file!" << std::endl;
			printf("press any key...\n");
			getchar();
			exit(1);
		}

		// [size]
		int cutxs = 16;
		int cutys = 16;
		// [step]
		int cutxi = 0;
		int cutyi = 0;
		// [flags]
		std::string cutflags;
		// [cut]
		int cutcount = 1;
		int cutxp = 0;
		int cutyp = 0;
		int cutsofar = 0;
		int cmdmode = -1;

		// parse CSV
		for (CSVIterator loop(sprguide); loop != CSVIterator(); ++loop)
		{
			const CSVRow& r = (*loop);
			size_t s = r.size();
			if (s > 0)
			{
				std::string inspect = r[0];

				if ((inspect.length() == 0) ||
					(inspect[0] == ';'))
				{
					// skip comments & empty lines
				}
				else
				if (inspect[0] == '[')
				{
					// command
					if (inspect.find("[size]") == 0)
					{
						cmdmode = 0;
					}
					else
					if (inspect.find("[step]") == 0)
					{
						cmdmode = 1;
					}
					else
					if (inspect.find("[flags]") == 0)
					{
						cmdmode = 2;
					}
					else
					if (inspect.find("[cut]") == 0)
					{
						cmdmode = 3;
					}
					else
					{
						std::cerr << "error: unknown spriteguide command: " << inspect.c_str() << std::endl;
						printf("press any key...\n");
						getchar();
						exit(1);
					}
				}
				else
				{
					int column_value = 0;
					int column_index = 0;
					std::string colval;
					char *pdummy = 0;

					switch (cmdmode)
					{
					case 0:
						{
							// size
							colval = r[column_index++];
							std::transform(colval.begin(), colval.end(), colval.begin(), sanitise_csv_integer);
							column_value = strtol(colval.c_str(), &pdummy, 10);
							cutxs = column_value;
							//
							colval = r[column_index++];
							std::transform(colval.begin(), colval.end(), colval.begin(), sanitise_csv_integer);
							column_value = strtol(colval.c_str(), &pdummy, 10);
							cutys = column_value;

							cmdmode = -1;
						}
						break;
					case 1:
						{
							// step
							colval = r[column_index++];
							std::transform(colval.begin(), colval.end(), colval.begin(), sanitise_csv_integer);
							column_value = strtol(colval.c_str(), &pdummy, 10);
							cutxi = column_value;
							//
							colval = r[column_index++];
							std::transform(colval.begin(), colval.end(), colval.begin(), sanitise_csv_integer);
							column_value = strtol(colval.c_str(), &pdummy, 10);
							cutyi = column_value;

							cmdmode = -1;
						}
						break;
					case 2:
						{
							// flags
							colval = r[column_index++];
							cutflags = colval;

							cmdmode = -1;
						}
						break;
					case 3:
						{
							// count
							colval = r[column_index++];
							std::transform(colval.begin(), colval.end(), colval.begin(), sanitise_csv_integer);
							column_value = strtol(colval.c_str(), &pdummy, 10);
							cutcount = column_value;
							// xpos
							colval = r[column_index++];
							std::transform(colval.begin(), colval.end(), colval.begin(), sanitise_csv_integer);
							column_value = strtol(colval.c_str(), &pdummy, 10);
							cutxp = column_value;
							// ypos
							colval = r[column_index++];
							std::transform(colval.begin(), colval.end(), colval.begin(), sanitise_csv_integer);
							column_value = strtol(colval.c_str(), &pdummy, 10);
							cutyp = column_value;

							// create extra mappings for frames with flags
							if (cutflags.length() > 0)
							{
								int mapend = cutsofar + cutcount;
								int mappos = cutsofar;
								while (mappos < mapend)
								{
									spritemapping_t &mapping = g_allargs.spritemappings[mappos];
									mapping.flags = cutflags;
									// no remapping of frames, just applying flags
									// also, no need to modify x/y position when not remapping frames
									mapping.from = mappos;
									mapping.to = mappos;
									mappos++;
								}
							}

							// record cutting event
							g_allargs.guideeventseq.push_back(guideevent(cutcount, cutflags, cutxp, cutyp, cutxs, cutys, cutxi, cutyi));

							std::cout << "recorded cutting event:" <<
								" count=" << cutcount << 
								" xp=" << cutxp <<
								" yp=" << cutyp <<
								" xs=" << cutxs <<
								" ys=" << cutys <<
								" xi=" << cutxi <<
								" yi=" << cutyi <<
								" fl=" << cutflags.c_str() <<
								std::endl;

							// reset flags
							cutflags = "";

							cutsofar += cutcount;

							cmdmode = -1;
						}
					default:
						break;
					};
				}
			}
		}
	}
}


// --------------------------------------------------------------------
//	chunky (8bit) to N-planar conversion
// --------------------------------------------------------------------

static void c2pw
(
	ubyte_t* _csrc, uword_t* _pdst, 
	int _w, int _h,
	int _sstride, int _dstride,
	int _nplanes
)
{	
	// v line loop
	for (int yc = 0; yc < _h; ++yc)
	{
		ubyte_t* src = &_csrc[yc * _sstride];
		uword_t* dst = &_pdst[yc * _dstride];
		
		// h word loop
		for (int xc = (_w >> 4) - 1; xc >= 0; --xc)
		{
			// h pixel loop				
			for (int xbc = 16-1; xbc >= 0; --xbc)
			{
				ubyte_t val = *(src++);
					
				// plane loop
				uword_t* o = dst;
				for (int p = _nplanes - 1; p >= 0; --p)
				{
					*(o) = (*(o) << 1) | (val & 1); val >>= 1; o++;
				}
			}
				
			// advance per word
			dst += _nplanes;
		}
	}
}	

static void c2pb
(
	ubyte_t* _csrc, ubyte_t* _pdst, 
	int _w, int _h,
	int _sstride, int _dstride,
	int _nplanes
)
{	
	// v line loop
	for (int yc = 0; yc < _h; ++yc)
	{
		ubyte_t* src = &_csrc[yc * _sstride];
		ubyte_t* dst = &_pdst[yc * _dstride];
		
		// h word loop
		for (int xc = (_w >> 3) - 1; xc >= 0; --xc)
		{
			// h pixel loop				
			for (int xbc = 8-1; xbc >= 0; --xbc)
			{
				ubyte_t val = *(src++);
					
				// plane loop
				ubyte_t* o = dst;
				for (int p = _nplanes - 1; p >= 0; --p)
				{
					*(o) = (*(o) << 1) | (val & 1); val >>= 1; o++;
				}
			}
				
			// advance per word
			dst += _nplanes;
		}
	}
}	

// --------------------------------------------------------------------
//	palette reading: 24bit .PAL
// --------------------------------------------------------------------

static bool read_pal24(const char* _fname, RGB *&r_ppal, unsigned short& r_numcolours, unsigned short& r_numuniquecolors)
{
	// we only need this here, so why pollute?
	typedef struct bgra_s
	{
		ubyte_t red;
		ubyte_t green;
		ubyte_t blue;
		ubyte_t alpha;
	} bgra_t;

	FILE *hpal = fopen(_fname, "rb");
	if (hpal)
	{
		std::cout << "reading fixed palette source [" << _fname << "]" << std::endl;

		// yeah, i know...
		fseek(hpal, 4 + 4 + 4 + 4 + 4 + 2, 0);

		short numcolors = 0;
		fread(&numcolors, 2, 1, hpal);

		r_numcolours = numcolors;
		if (r_numcolours == 0)
		{
			fclose(hpal);
			return false;
		}

		{
			// read in the colors
			bgra_t *temp = new bgra_t[numcolors];
			fread(temp, 1, 4 * numcolors, hpal);
			fclose(hpal);

			// convert whatever colours we were able to read into local format
			r_ppal = new RGB[numcolors];
			for (int i = 0; i < numcolors; i++)
			{
				r_ppal[i].blue = temp[i].blue;
				r_ppal[i].green = temp[i].green;
				r_ppal[i].red = temp[i].red;
			}

			delete[] temp;
		}

		// count distinct colours (in case of duplicates)
		int u = r_numcolours;
		for (int o = 0; o < r_numcolours; o++)
		{
			for (int i = o + 1; i < r_numcolours; i++)
			{
				if ((r_ppal[o].red == r_ppal[i].red) &&
					(r_ppal[o].green == r_ppal[i].green) &&
					(r_ppal[o].blue == r_ppal[i].blue))
					{
						u--; break;
					}
			}
		}
		r_numuniquecolors = u;

		return true;
	}

	// not recognized/not found
	std::cerr << "warning: can't open this palette file: " << _fname << std::endl;
	return false;
}

// --------------------------------------------------------------------
//	palette reading: 12bit .cct tileset files
// --------------------------------------------------------------------

static bool read_pal_cct(const char* _fname, RGB *&r_ppal, unsigned short& r_numcolours, unsigned short& r_numuniquecolours)
{
	FILE *hpal = fopen(_fname, "rb");
	if (hpal)
	{
		std::cout << "reading fixed palette source [" << _fname << "]" << std::endl;

		r_numcolours = 16;

		// read in the colors
		uword_t palette[16];
		//fread(palette, 1, 2, hpal);
		fread(palette, 1, 32, hpal);
		fclose(hpal);

		// convert whatever colours we were able to read into local format
		r_ppal = new RGB[16];

		for (int i = 0; i < 16; i++)
		{
			// reformat from bigendian for motorola->intel
			uword_t ste_col = endianswap16(palette[i]);

			word_t ste_r = (ste_col >> 8) & 0xF;
			word_t ste_g = (ste_col >> 4) & 0xF;
			word_t ste_b = (ste_col >> 0) & 0xF;

			// reformat STE swizzled 4:4:4 format into 24bit 8:8:8
			word_t r = ((ste_r & 0x7) << 1) | ((ste_r >> 3) & 1);
			word_t g = ((ste_g & 0x7) << 1) | ((ste_g >> 3) & 1);
			word_t b = ((ste_b & 0x7) << 1) | ((ste_b >> 3) & 1);

			r_ppal[i].red   = r << 4;
			r_ppal[i].green = g << 4;
			r_ppal[i].blue  = b << 4;
		}

		// count distinct colours (in case of duplicates)
		int u = r_numcolours;
		for (int o = 0; o < r_numcolours; o++)
		{
			for (int i = o + 1; i < r_numcolours; i++)
			{
				if ((r_ppal[o].red == r_ppal[i].red) &&
					(r_ppal[o].green == r_ppal[i].green) &&
					(r_ppal[o].blue == r_ppal[i].blue))
					{
						u--; break;
					}
			}
		}
		r_numuniquecolours = u;

		return true;
	}

	// not recognized/not found
	std::cerr << "warning: can't open this image: " << _fname << std::endl;
	return false;
}


// --------------------------------------------------------------------
//	palette reading: 18bit->4:4 solutionpair from PhotoChrome
// --------------------------------------------------------------------

u8 *g_rgb_solutionpair_map = NULL;
u8 g_solutionpairs[256];

static bool read_pal_ccs(const char* _fname, RGB *&r_ppal, unsigned short& r_numcolours, unsigned short& r_numuniquecolours)
{
	std::string file_ccs(_fname);		// name of solutionpair file
	std::string file_ccr(_fname);		// name of RGB->solutionpair reductionmap file
	std::string file_ccp(_fname);		// name of palette file

	replace_extension(file_ccr, "ccr");
	replace_extension(file_ccp, "ccp");

	FILE *hpal = NULL;
	FILE *hsol = NULL;
	FILE *hred = NULL;

	hpal = fopen(file_ccp.c_str(), "rb");
	if (hpal)
	{
		hsol = fopen(file_ccs.c_str(), "rb");
		if (hsol)
		{
			hred = fopen(file_ccr.c_str(), "rb");
			if (hred)
			{
				std::cout << "reading fixed palette fileset [" << file_ccs.c_str() << "] [" << file_ccr.c_str() << "] [" << file_ccp.c_str() << "]" << std::endl;

				r_numcolours = 16;

				// read in the colors
				uword_t palette[16];
				//fread(palette, 1, 2, hpal);
				fread(palette, 1, sizeof(palette), hpal);
				fclose(hpal); hpal = NULL;

				// convert whatever colours we were able to read into local format
				r_ppal = new RGB[16];

				for (int i = 0; i < 16; i++)
				{
					// reformat from bigendian for motorola->intel
					uword_t ste_col = endianswap16(palette[i]);

					word_t ste_r = (ste_col >> 8) & 0xF;
					word_t ste_g = (ste_col >> 4) & 0xF;
					word_t ste_b = (ste_col >> 0) & 0xF;

					// reformat STE swizzled 4:4:4 format into 24bit 8:8:8
					word_t r = ((ste_r & 0x7) << 1) | ((ste_r >> 3) & 1);
					word_t g = ((ste_g & 0x7) << 1) | ((ste_g >> 3) & 1);
					word_t b = ((ste_b & 0x7) << 1) | ((ste_b >> 3) & 1);

					r_ppal[i].red = r << 4;
					r_ppal[i].green = g << 4;
					r_ppal[i].blue = b << 4;
				}

				// count distinct colours (in case of duplicates)
				int u = r_numcolours;
				for (int o = 0; o < r_numcolours; o++)
				{
					for (int i = o + 1; i < r_numcolours; i++)
					{
						if ((r_ppal[o].red == r_ppal[i].red) &&
							(r_ppal[o].green == r_ppal[i].green) &&
							(r_ppal[o].blue == r_ppal[i].blue))
						{
							u--; break;
						}
					}
				}
				r_numuniquecolours = u;

				// read solutionpairs (256 8bit records)
				fread(g_solutionpairs, 1, 256, hsol);
				fclose(hsol); hsol = NULL;

				// read RGB->solutionpair reductionmap (64x64x64 = 256kb table)
				delete[] g_rgb_solutionpair_map;
				g_rgb_solutionpair_map = new u8[64*64*64];
				fread(g_rgb_solutionpair_map, 1, (64*64*64), hred);
				fclose(hred); hred = NULL;

				return true;
			}
		}
	}

	if (hsol)
		fclose(hsol);
	if (hpal)
		fclose(hpal);

	// not recognized/not found
	std::cerr << "warning: can't open complete fileset: " << _fname << std::endl;
	return false;
}

// --------------------------------------------------------------------
//	palette reading: 12bit .PI1
// --------------------------------------------------------------------

static bool read_pal_pi1(const char* _fname, RGB *&r_ppal, unsigned short& r_numcolours, unsigned short& r_numuniquecolours)
{
	FILE *hpal = fopen(_fname, "rb");
	if (hpal)
	{
		std::cout << "reading fixed palette source [" << _fname << "]" << std::endl;

		r_numcolours = 16;

		// read in the colors
		uword_t palette[16];
		fread(palette, 1, 2, hpal);
		fread(palette, 1, 32, hpal);
		fclose(hpal);

		// convert whatever colours we were able to read into local format
		r_ppal = new RGB[16];

		for (int i = 0; i < 16; i++)
		{
			// reformat from bigendian for motorola->intel
			uword_t ste_col = endianswap16(palette[i]);

			word_t ste_r = (ste_col >> 8) & 0xF;
			word_t ste_g = (ste_col >> 4) & 0xF;
			word_t ste_b = (ste_col >> 0) & 0xF;

			// reformat STE swizzled 4:4:4 format into 24bit 8:8:8
			word_t r = ((ste_r & 0x7) << 1) | ((ste_r >> 3) & 1);
			word_t g = ((ste_g & 0x7) << 1) | ((ste_g >> 3) & 1);
			word_t b = ((ste_b & 0x7) << 1) | ((ste_b >> 3) & 1);

			r_ppal[i].red   = r << 4;
			r_ppal[i].green = g << 4;
			r_ppal[i].blue  = b << 4;
		}

		// count distinct colours (in case of duplicates)
		int u = r_numcolours;
		for (int o = 0; o < r_numcolours; o++)
		{
			for (int i = o + 1; i < r_numcolours; i++)
			{
				if ((r_ppal[o].red == r_ppal[i].red) &&
					(r_ppal[o].green == r_ppal[i].green) &&
					(r_ppal[o].blue == r_ppal[i].blue))
					{
						u--; break;
					}
			}
		}
		r_numuniquecolours = u;

		return true;
	}

	// not recognized/not found
	std::cerr << "warning: can't open this image: " << _fname << std::endl;
	return false;
}

// --------------------------------------------------------------------
//	wrappers for stb_image loader
// --------------------------------------------------------------------


// --------------------------------------------------------------------
//	load image, recovering indexed colour & palette if available
// --------------------------------------------------------------------

bool image_load(const char* filename, RGB *&r_psourcergb, unsigned char *&r_psourcecol, RGB *&r_ppalettergb, ulong_t &r_w, ulong_t &r_h, ulong_t &r_d)
{
	int stbx,stby,stbn;
	unsigned char *stbdata = stbi_load(filename, &stbx, &stby, &stbn, 3);

	if (stbdata)
	{
		int imagesize = stbx*stby;
		r_psourcergb = new RGB[imagesize];
		unsigned char* data = stbdata;
		for (int p = 0; p < imagesize; p++)
		{
			r_psourcergb[p].red = *data++;
			r_psourcergb[p].green = *data++;
			r_psourcergb[p].blue = *data++;
		}
		STBI_FREE(stbdata);

		if (g_stbi_colours)
		{
			r_ppalettergb = new RGB[256];
			for (int p = 0; p < g_stbi_colours; p++)
			{
				r_ppalettergb[p].red = g_stbi_palette[p][0];
				r_ppalettergb[p].green = g_stbi_palette[p][1];
				r_ppalettergb[p].blue = g_stbi_palette[p][2];
			}
		}

		r_psourcecol = g_stbi_colourmap;
		r_w = stbx;
		r_h = stby;
		r_d = g_stbi_bits;

		return true;
	}

	// not recognized/not found
	std::cerr << "warning: can't open this image" << std::endl;
	return false;
}

// --------------------------------------------------------------------
//	load image, recovering ONLY the palette if available
// --------------------------------------------------------------------

bool palette_load(const char* filename, RGB *&r_ppal, unsigned short &r_numcolours, unsigned short &r_numuniquecolours)
{
	int stbx,stby,stbn;
	unsigned char *stbdata = stbi_load(filename, &stbx, &stby, &stbn, 3);

	if (stbdata)
	{
		// don't need pixel data
		STBI_FREE(stbdata);

		if (g_stbi_colours)
		{
			r_ppal = new RGB[256];
			for (int p = 0; p < g_stbi_colours; p++)
			{
				r_ppal[p].red = g_stbi_palette[p][0];
				r_ppal[p].green = g_stbi_palette[p][1];
				r_ppal[p].blue = g_stbi_palette[p][2];
			}
			
			r_numcolours = g_stbi_colours;

			// count distinct colours (in case of duplicates)
			int u = r_numcolours;
			for (int o = 0; o < r_numcolours; o++)
			{
				for (int i = o + 1; i < r_numcolours; i++)
				{
					if ((r_ppal[o].red == r_ppal[i].red) &&
						(r_ppal[o].green == r_ppal[i].green) &&
						(r_ppal[o].blue == r_ppal[i].blue))
					{
						u--; break;
					}
				}
			}
			r_numuniquecolours = u;

			return true;
		}

		// not indexed colour - no palette
		std::cerr << "warning: non-indexed colour image or unsupported image type - no palette available" << std::endl;
		return false;
	}

	// not recognized/not found
	std::cerr << "warning: can't open this image" << std::endl;
	return false;
}

// --------------------------------------------------------------------
//	tile dictionary stuff
// --------------------------------------------------------------------

typedef std::list<RGB*> tile24dictionary_t;
typedef std::list<ubyte_t*> tile8dictionary_t;
typedef std::list<uword_t*> tileplanardictionary_t;

tile24dictionary_t s_tile24dictionary;
tile8dictionary_t s_tile8dictionary;
tileplanardictionary_t s_tileplanardictionary;

// extract pixels from 8bit indexed tile data
static inline void extract_tile8(ubyte_t *_pdsttile, ubyte_t *_psrctile, ulong_t _sourcew)
{
	int tilesize = g_allargs.tilesize;
	for (int py = 0; py < tilesize; py++)
	{
//		printf("\n");
		for (int px = 0; px < tilesize; px++)
		{
//			printf("%02x ", _psrctile[(py * _sourcew) + px]);
			_pdsttile[(py * tilesize) + px] = _psrctile[(py * _sourcew) + px];
		}
	}
}

// extract pixels from 24bit TC tile data (for error comparison)
static inline void extract_tile24(RGB *_pdsttile, RGB *_psrctile, ulong_t _sourcew)
{
	int tilesize = g_allargs.tilesize;
	for (int py = 0; py < tilesize; py++)
	{
//		printf("\n");
		for (int px = 0; px < tilesize; px++)
		{
//			printf("%02x ", _psrctile[(py * _sourcew) + px].green);
			_pdsttile[(py * tilesize) + px] = _psrctile[(py * _sourcew) + px];
		}
	}
}

// compare two 24bit tiles for similarity
static inline float compare_tile24(RGB *_pdsttile, RGB *_psrctile, ulong_t _sourcew)
{
	int tilesize = g_allargs.tilesize;

	static const float c_n = 1.0f / 255.0f;

	float errsum = 0.0f;

	for (int py = 0; py < tilesize; py++)
	{
		for (int px = 0; px < tilesize; px++)
		{
			const RGB &spix = _psrctile[(py * _sourcew) + px];
			const RGB &dpix = _pdsttile[(py * tilesize) + px];

			float dr = float(spix.red   - dpix.red) * c_n;
			float dg = float(spix.green - dpix.green) * c_n;
			float db = float(spix.blue  - dpix.blue) * c_n;

			float err = (dr * dr) + (dg * dg) + (db * db);
//			float err = pow(fabs(dr), 0.5) + pow(fabs(dg), 0.5) + pow(fabs(db), 0.5);
//			float err = fabs(dr) + fabs(dg) + fabs(db);

			errsum += err;
		}
	}

	return errsum;
}

// --------------------------------------------------------------------
//	add tile to dictionary if it appears to be unique
// --------------------------------------------------------------------

static int process_tile(RGB *_psourcergb, ubyte_t *_psourcecol, ulong_t _sourcew, int _tx, int _ty)
{
	int mapindex = -1;

	// compare this tile with all tiles already in dictionary

	// address source tile in terms of tile coords
	RGB *psrctile24 = &_psourcergb[(_sourcew * (_ty * g_allargs.tilesize)) + (_tx * g_allargs.tilesize)];

	// configure nearest match to worst case before search
	tile24dictionary_t::iterator best_it24 = s_tile24dictionary.end();
	tile8dictionary_t::iterator best_it8 = s_tile8dictionary.end();
	float best_error = g_allargs.tolerance+(1e-7f);
	int best_index = -1;
	bool match = false;

	// run comparison against all tiles
	tile24dictionary_t::iterator it24 = s_tile24dictionary.begin();
	tile8dictionary_t::iterator it8 = s_tile8dictionary.begin();
	int index = 0;
	for (; it24 != s_tile24dictionary.end(); it24++, it8++, index++)
	{
		RGB *pdict24 = *it24;
		float match_error = compare_tile24(pdict24, psrctile24, _sourcew);

		// if this dictionary tile yields a closer match, track it
		if (match_error < best_error)
		{
			match = true;
			best_error = match_error;
			best_it24 = it24;
			best_it8 = it8;
			best_index = index;
		}
	}

	if (match)
	{
		// if match was found, consider it reused
		//		std::cout << "reused tile with match error @ " << best_error << std::endl;
		mapindex = best_index;
	}
	else
	{
		mapindex = (int)s_tile8dictionary.size();

		// if no match was found, store tile as unique
		RGB *pnewtile24 = new RGB[64 * 64];
		ubyte_t *pnewtile8 = new ubyte_t[64 * 64];
		ubyte_t *psrctile8 = &_psourcecol[(_sourcew * (_ty * g_allargs.tilesize)) + (_tx * g_allargs.tilesize)];

		extract_tile24(pnewtile24, psrctile24, _sourcew);
		extract_tile8(pnewtile8, psrctile8, _sourcew);

		//		std::cout << "recorded unique tile " << s_tile8dictionary.size() << " @ " << _tx << "x" << _ty << " with match error > " << best_error << std::endl;
		s_tile24dictionary.push_back(pnewtile24);
		s_tile8dictionary.push_back(pnewtile8);
	}

	return mapindex;
}

// --------------------------------------------------------------------
//	process all map tiles from source image
// --------------------------------------------------------------------

static void process_tilemap
(
	long_t *&r_ptilesrcmap, int &r_tilesrcw, int &r_tilesrch,
	long_t *&r_ptilemap, int &r_tilemapw, int &r_tilemaph, 
	RGB *_psourcergb, ubyte_t *_psourcecol, ulong_t _sourcew, ulong_t _sourceh
)
{
	s_tile24dictionary.clear();
	s_tile8dictionary.clear();
	s_tileplanardictionary.clear();

	// find source dimensions in terms of tiles
	int tilesrcw = (_sourcew / g_allargs.tilesize);
	int tilesrch = (_sourceh / g_allargs.tilesize);
	r_tilesrcw = tilesrcw;
	r_tilesrch = tilesrch;
	r_ptilesrcmap = new long_t[tilesrcw*tilesrch];

	long_t *psrcmap = r_ptilesrcmap;

	// find map dimensions in terms of tiles
	int mapw = xmin(tilesrcw - g_allargs.maprect_xpos, g_allargs.maprect_xsize);
	int maph = xmin(tilesrch - g_allargs.maprect_ypos, g_allargs.maprect_ysize);
	r_tilemapw = mapw;
	r_tilemaph = maph;
	r_ptilemap = new long_t[mapw*maph];

	long_t *ptilemap = r_ptilemap;

	// iterate over source tiles, left-to-right, top-to-bottom
	for (int ty = 0; ty < tilesrch; ty++)
	{
		for (int tx = 0; tx < tilesrcw; tx++)
		{
			int tilemapindex = process_tile(_psourcergb, _psourcecol, _sourcew, tx, ty);

			// update map only within cutting rectangle
			int mx = tx - g_allargs.maprect_xpos;
			int my = ty - g_allargs.maprect_ypos;
			if ((mx >= 0 && my >= 0) && (mx < g_allargs.maprect_xsize && my < g_allargs.maprect_ysize))
			{
				ptilemap[(my * mapw) + mx] = tilemapindex;
			}
			// update src map always (for --visual)
			psrcmap[(ty*tilesrcw)+tx] = tilemapindex;

			if ((s_progress & 0x1F) == 0)
			{
				switch ((s_progress >> 5) & 3)
				{
					case 0: printf("|\r"); break;
					case 1: printf("/\r"); break;
					case 2: printf("-\r"); break;
					case 3: printf("\\\r"); break;
				}
			}
			s_progress++;
		}
	}

	int uniquetiles = s_tile8dictionary.size();
}

// --------------------------------------------------------------------
//	convert 8bit tiles to 8-plane for export (later chop to N planes)
// --------------------------------------------------------------------

static void remap_dictionary_tiles()
{
	std::cout << "converting " << s_tile8dictionary.size() << " 8-bit tiles into planar format..." << std::endl;

	tile8dictionary_t::iterator it8 = s_tile8dictionary.begin();
	for (; it8 != s_tile8dictionary.end(); it8++)
	{
		ubyte_t *ptile8 = *it8;
/*
		for (int y = 0; y < 16; y++)
		{
			printf("\n");
			for (int x = 0; x < 16; x++)
			{
				printf("%02x ", ptile8[(y*16)+x]);
			}
		}
*/
		// operate on 8 planes for intermediate steps, then save only required planes later
		int planes = 8;
		uword_t *ptileplanar = 0;

		// handle 8x8 a bit differently so we can use word-based c2p on it
		if (g_allargs.tilesize == 8)
		{
			// emit 8x8 to 16x8 buffer for c2p
			int bytes_per_dst_line = planes;

			int bytes_per_tile = g_allargs.tilesize*bytes_per_dst_line;
			ptileplanar = new uword_t[bytes_per_tile>>1];
			memset(ptileplanar, 0, bytes_per_tile);

			// convert chunk 8-bit tiles into 8-planar format...
			c2pb
			(
				/*src=*/ptile8, /*dst*/(ubyte_t*)ptileplanar,
				/*w=*/g_allargs.tilesize, /*h=*/g_allargs.tilesize,
				/*bytes_per_src_line=*/g_allargs.tilesize, /*bytes_per_dst_line=*/bytes_per_dst_line,
				/*planes=*/planes
			);
		}
		else
		{
			// all other sizes go to normal buffer
			int words_per_dst_line = (planes * (g_allargs.tilesize >> 4));

			int words_per_tile = g_allargs.tilesize*words_per_dst_line;
			ptileplanar = new uword_t[words_per_tile];
			memset(ptileplanar, 0, words_per_tile<<1);

			// convert chunk 8-bit tiles into 8-planar format
			c2pw
			(
				/*src=*/ptile8, /*dst*/ptileplanar,
				/*w=*/g_allargs.tilesize, /*h=*/g_allargs.tilesize,
				/*bytes_per_src_line=*/g_allargs.tilesize, /*words_per_dst_line=*/words_per_dst_line,
				/*planes=*/planes
			);
		}

		s_tileplanardictionary.push_back(ptileplanar);
	}

	std::cout << "...done" << std::endl;
}

// --------------------------------------------------------------------
//	emit .PI1 from 4-plane buffer & palette data
// --------------------------------------------------------------------

static void emit_pi1(std::string outfile, uword_t *buffer4plane, RGB *_ppal24, bool _fromwords, int pi1_index)
{
	// t<N><outfile>.pi1
	std::string fname("t");
	fname += ('0' + pi1_index);
	fname.append(outfile.c_str());
	replace_extension(fname, "pi1");

	FILE *hout = fopen(fname.c_str(), "wb");
	if (hout)
	{
		uword_t palette4bit[16] = { 0x000, 0xfff, 0x111, 0x222, 0x333, 0x444, 0x555, 0x666, 0x777, 0x888, 0x999, 0xaaa, 0xbbb, 0xccc, 0xddd, 0xeee };

		for (int i = 0; i < 16; i++)
		{
			word_t r = word_t(_ppal24[i].red);
			word_t g = word_t(_ppal24[i].green);
			word_t b = word_t(_ppal24[i].blue);

			// reformat 24bit 8:8:8 into STE swizzled 4:4:4 format
			word_t ste_r = (r >> 5) | (((r >> 4) & 1) << 3);
			word_t ste_g = (g >> 5) | (((g >> 4) & 1) << 3);
			word_t ste_b = (b >> 5) | (((b >> 4) & 1) << 3);

			uword_t ste_col = uword_t((ste_r << 8) | (ste_g << 4) | ste_b);

			// reformat to bigendian for intel->motorola
			palette4bit[i] = endianswap16(ste_col);
		}

		// reformat image to bigendian for intel->motorola
		if (_fromwords)
			for (int i = 0; i < 16000; i++)
			{
				buffer4plane[i] = endianswap16(buffer4plane[i]);
			}

		// write header
		unsigned short v = 0;
		fwrite(&v, 1, 2, hout);

		// write palette
		fwrite(palette4bit, 1, 32, hout);

		// write buffer
		fwrite(buffer4plane, 1, 32000, hout);
		fclose(hout);

		std::cout << "emitted tilegroup [" << fname.c_str() << "]" << std::endl;
	}
}

// --------------------------------------------------------------------
//	colour reduction (fixed palette)
// --------------------------------------------------------------------

static ubyte_t s_colours_matched[256];
int s_num_colours_matched = 0;

static bool s_reducer_keycol_exact = true;

static inline ubyte_t match_colour(const RGB &rgb, RGB *_ppal, int _ncols)
{
	static const float c_n = 1.0f / 255.0f;

	float fr = (float(rgb.red) * g_allargs.colrenorm) / 255.0f;
	float fg = (float(rgb.green) * g_allargs.colrenorm) / 255.0f;
	float fb = (float(rgb.blue) * g_allargs.colrenorm) / 255.0f;

	//if (fr > 250.0f && fg > 250.0f && fb > 250.0f)
	//{
	//	__asm int 3;
	//}

	// default to keycol
	int best_hit = g_allargs.keycol;

	u8 kr = g_allargs.keyrgb.red;
	u8 kg = g_allargs.keyrgb.green;
	u8 kb = g_allargs.keyrgb.blue;

	if ((rgb.red == kr) &&
		(rgb.green == kg) &&
		(rgb.blue == kb))
	{
		// map keycol, even if it's outside indexed colour range (it won't be emitted as graphics anyway)
	}
	else
	{
		float best_err = 1e7;
		for (int i = 0; i < _ncols; i++)
		{
			const RGB &pal = _ppal[i];

			float pr = float(pal.red /*+ 8*/);
			float pg = float(pal.green /*+ 8*/);
			float pb = float(pal.blue /*+ 8*/);

			// YUV error - meh
			//float dr = (fr - pr) * c_n * 0.299;
			//float dg = (fg - pg) * c_n * 0.587;
			//float db = (fb - pb) * c_n * 0.114;

			float dr = (fr - pr) * c_n;
			float dg = (fg - pg) * c_n;
			float db = (fb - pb) * c_n;

			float err = (dr * dr) + (dg * dg) + (db * db);

			// in exact mode, don't remap anything to keycol index unless its an exact colour match
			if (!g_allargs.keyguard ||
				(i != g_allargs.keycol) ||
				(err < g_allargs.keytol))
			{
				if (err < best_err)
				{
					best_err = err;
					best_hit = i;
				}
			}
		}
	}

	if (s_colours_matched[best_hit] == 0)
	{
		s_colours_matched[best_hit] = 1;
		s_num_colours_matched++;
	}

	return (ubyte_t)best_hit;
}

// --------------------------------------------------------------------
//	reduce TC image to indexed colour using fixed palette
// --------------------------------------------------------------------

void review_palettes(RGB *pspal, RGB *pdpal, int _sourced)
{
	memset(s_colours_matched, 0, 256);
	if (_sourced == 0)
		_sourced = 24;

	int ncols = 1 << _sourced;

	for (int c = 0; c < (1 << _sourced); c++)
	{
		if (pspal && pdpal && (c < 16))
		{
			int m = match_colour(pspal[c], pdpal, ncols);

			// index-index remapping case
			printf("src: [%03d] $%02x:$%02x:$%02x  mapto: [%02d] $%02x:$%02x:$%02x  target: [%02d] $%02x:$%02x:$%02x\n",
				c,
				(int)pspal[c].red, (int)pspal[c].green, (int)pspal[c].blue,
				m,
				(int)pdpal[m].red, (int)pdpal[m].green, (int)pdpal[m].blue,
				c,
				(int)pdpal[c].red, (int)pdpal[c].green, (int)pdpal[c].blue
				);
		}
		else
		if (pdpal && (c < 16))
		{
			// 24bit reduction case
			printf("src: target: [%02d] $%02x:$%02x:$%02x\n",
				c,
				(int)pdpal[c].red, (int)pdpal[c].green, (int)pdpal[c].blue
				);
		}
		else
		if (pspal)
		{
			// direct index translation case
			printf("src: [%03d] $%02x:$%02x:$%02x\n",
				c,
				(int)pspal[c].red, (int)pspal[c].green, (int)pspal[c].blue
				);
		}
	}
}

static void reduce_image24(RGB *_psourcergb, ubyte_t *_psrccol, ubyte_t *_pdstcol0, ubyte_t *_pdstcol1, ulong_t _sourcew, ulong_t _sourceh, ulong_t _sourced, RGB *&r_ppal)
{
	unsigned short r_numcolours = 0;
	unsigned short r_numuniquecolours = 0;
	bool success = false;

	RGB* ppal = 0;

	// Degas as container for a palette
	if (g_allargs.palettefile.find(".pi1") != std::string::npos)
		success = read_pal_pi1(g_allargs.palettefile.c_str(), ppal, r_numcolours, r_numuniquecolours);
	else
	// RIFF format palettes
	if (g_allargs.palettefile.find(".pal") != std::string::npos)
		success = read_pal24(g_allargs.palettefile.c_str(), ppal, r_numcolours, r_numuniquecolours);
	else
	// PhotoChrome-generated .cct tile libraries carry the palette as the first part of the header
	if (g_allargs.palettefile.find(".cct") != std::string::npos)
		success = read_pal_cct(g_allargs.palettefile.c_str(), ppal, r_numcolours, r_numuniquecolours);
	else
	// a lone .ccp is raw palette data, same as header for .cct, so just reuse that reader for now
	if (g_allargs.palettefile.find(".ccp") != std::string::npos)
		success = read_pal_cct(g_allargs.palettefile.c_str(), ppal, r_numcolours, r_numuniquecolours);
	else
	{
		// The .ccs format infers the existence of: .ccs (solutionpairs) + .ccr (RGB->solutionpair reduction map) + .ccp (palette)
		// which all get emitted together from PhotoChrome v6 when using '-ccmode 3' to generate palettes
		// This format also enables a different remapping method in order to apply complementary field dithering.
		if (g_allargs.palettefile.find(".ccs") != std::string::npos)
		{
			success = read_pal_ccs(g_allargs.palettefile.c_str(), ppal, r_numcolours, r_numuniquecolours);
			g_allargs.use_solutionpair_reduction_ = true;
		}
		else
		{
			std::cout << " unrecognized palette format [" << g_allargs.palettefile.c_str() << "] so trying image handlers..." << std::endl;
			success = palette_load(g_allargs.palettefile.c_str(), ppal, r_numcolours, r_numuniquecolours);
			if (!success)
				std::cerr << "warning: this image format can't be used to supply palettes currently" << std::endl;
		}
	}

	if (success)
	{
		// review palettes if source image is indexed
		if ((_sourced > 0) && (_sourced < 16) && 
			!g_allargs.quickcut && !g_allargs.quiet)
		{
			std::cout << "summary of source->desintation palettes for indexed source image being remapped" << std::endl;
			review_palettes(r_ppal, ppal, _sourced);
		}

		r_ppal = ppal;

		std::cout << " size of palette: " << r_numcolours << std::endl;
		std::cout << " unique 24bit colours available in palette: " << r_numuniquecolours << std::endl;
		if (r_numuniquecolours > (1 << g_allargs.planes))
			std::cout << " warning: target palette contains more colours than " << g_allargs.planes << " bitplanes can encode!" << std::endl;

		int num_unique = 16;

		if (g_allargs.use_solutionpair_reduction_)
		{
			// use PhotoChrome-generated complementary solution pairs for extra win!
			// todo: assume single-field for now, add dual-field later
//			int field = 0;

			{
				RGB *psrc = _psourcergb;
				ubyte_t *pdst0 = _pdstcol0;
				ubyte_t *pdst1 = _pdstcol1;
				for (int py = 0; py < (int)_sourceh; py++)
				{
					for (int px = 0; px < (int)_sourcew; px++)
					{
						RGB &rgb = *psrc++;

						u8 r8 = rgb.red;
						u8 g8 = rgb.green;
						u8 b8 = rgb.blue;

						u8 solution0 = g_allargs.keycol;
						u8 solution1 = g_allargs.keycol;

						if ((r8 == g_allargs.keyrgb.red) &&
							(g8 == g_allargs.keyrgb.green) &&
							(b8 == g_allargs.keyrgb.blue))
						{
							// default to keycol
						}
						else
						{
							// otherwise use the solution pair (usually a dither)
							int rgb666 = ((b8 >> 2) << 12) | ((g8 >> 2) << 6) | (r8 >> 2);

							u8 solution = g_rgb_solutionpair_map[rgb666];
							u8 solutionpair = g_solutionpairs[solution];

							solution0 = (solutionpair >> 4) & 0xF;
							solution1 = (solutionpair >> 0) & 0xF;
						}

						if (g_allargs.ccremap == CCRemap_LOW)
						{
							// primary field gets low intensity component, secondary field gets high intensity
							*pdst0++ = solution0;
							if (pdst1)
								*pdst1++ = solution1;
						}
						else
						if (g_allargs.ccremap == CCRemap_HIGH)
						{
							// primary field gets high intensity component, secondary field gets low intensity
							*pdst0++ = solution1;
							if (pdst1)
								*pdst1++ = solution0;
						}
						else
						{
							// both fields get alternating, complementary pattern of high/low
							*pdst0++ = (((px ^ py) ^ 0) & 1) ? solution0 : solution1;
							if (pdst1)
								*pdst1++ = (((px ^ py) ^ 1) & 1) ? solution0 : solution1;
						}
					}
				}
			}
		}
		else
		{
			// use average-joe palette, with mundane nearest-match thingy. zzzzz...

			memset(s_colours_matched, 0, 256);
			num_unique = 0;
			{
				char unique[256];
				memset(unique, 0, 256);
				RGB *psrc = _psourcergb;
				ubyte_t *pdst = _pdstcol0;
				for (int py = 0; py < (int)_sourceh; py++)
				{
					for (int px = 0; px < (int)_sourcew; px++)
					{
						ubyte_t m = match_colour(*psrc++, r_ppal, r_numcolours);
						*pdst++ = m;
						if (unique[m] == 0)
						{
							unique[m] = 1;
							num_unique++;
						}
					}
				}
			}
		}

		std::cout << " indexed colours used in final image: " << num_unique << std::endl;

		if ((_sourced == 0) || (_sourced >= 16) && 
			!g_allargs.quickcut && !g_allargs.quiet)
		{
			std::cout << "summary of destination palette for " << ((_sourced > 0) ? _sourced : 24) << "-bit source image being remapped" << std::endl;
			review_palettes(NULL, r_ppal, _sourced);
		}

	}
	else
	{
		std::cerr << "error: could not read input palette from palette file or image" << std::endl;
		printf("press any key...\n");
		getchar();	
		exit(1);
	}
	std::cout << "...done" << std::endl;
}

// --------------------------------------------------------------------
//	emit map data file indicating where tiles go in original image
// --------------------------------------------------------------------

static void emit_tilemap_ccm(std::string outfile, long_t *_ptilemap, int _tilemapw, int _tilemaph, int _tilestep)
{
	long_t v;
	word_t vw;
	byte_t vb;

	replace_extension(outfile, "ccm");

	int mapentries = _tilemapw * _tilemaph;
	long_t *be_tilemap = new long_t[mapentries];
//	memcpy(be_tilemap, _ptilemap, mapentries * sizeof(long_t));

	for (int i = 0; i < mapentries; i++)
		be_tilemap[i] = endianswap32(_ptilemap[i]);

	FILE *hmap = fopen(outfile.c_str(), "wb");

	// flags
	vb = 0;
	fwrite(&vb, 1, sizeof(vb), hmap);
	// tilestep
	vb = (byte_t)_tilestep;
	fwrite(&vb, 1, sizeof(vb), hmap);
	// width
	vw = endianswap16((s16)_tilemapw);
	fwrite(&vw, 1, sizeof(vw), hmap);
	// unused
	vw = 0;
	fwrite(&vw, 1, sizeof(vw), hmap);
	// height
	vw = endianswap16((s16)_tilemaph);
	fwrite(&vw, 1, sizeof(vw), hmap);

	fwrite(be_tilemap, sizeof(long_t), mapentries, hmap);

	fclose(hmap);

	delete[] be_tilemap;

	std::cout << "emitted tilemap [" << outfile.c_str() << "]" << std::endl;
}

// --------------------------------------------------------------------
//	emit tile library as sequence of Degas .PI1 images
// --------------------------------------------------------------------

// emit tiles to a series of 320x200 .PI1 images
static void emit_tiles_degas(std::string outfile, RGB *_ppalettergb)
{
	uword_t *buffer4plane = new uword_t[16000];
	memset(buffer4plane, 0, 32000);

	int pi1_index = 0;
	int otx = 0;
	int oty = 0;
	// emit PI1 images representing dictionary
	while (!s_tileplanardictionary.empty())
	{
		// copy 4 least significant planes to PI1 buffer
		uword_t *pin = s_tileplanardictionary.front();
		// find output tile address in 4-plane output buffer
		uword_t *pout = &buffer4plane[((otx & -16)>>(4-2)) + (oty * (320>>(4-2)))];

		// special handling of 8x8 tiles. note this produces motorola/bigendian ordering. other sizes produce intel words.
		if (g_allargs.tilesize == 8)
		{
			ubyte_t *pinb = (ubyte_t*)pin;
			for (int ty = 0; ty < g_allargs.tilesize; ty++)
			{
				ubyte_t *poutb = (ubyte_t *)pout;
				{
					if (otx & 8)
					{
						// record 4 planes
						poutb[0+1] = *pinb++;
						poutb[2+1] = *pinb++;
						poutb[4+1] = *pinb++;
						poutb[6+1] = *pinb++;
					}
					else
					{
						// record 4 planes
						poutb[0+0] = *pinb++;
						poutb[2+0] = *pinb++;
						poutb[4+0] = *pinb++;
						poutb[6+0] = *pinb++;
					}
					// skip 4 planes of 8-plane tile
					pinb += 4;
				}
				// next line of PI1
				pout += (320 >> (4-2));
			}
		}
		else
		{
			for (int ty = 0; ty < g_allargs.tilesize; ty++)
			{
				uword_t *poutw = pout;
				for (int tx = 0; tx < (g_allargs.tilesize >> 4); tx++)
				{
					// record 4 planes
					*poutw++ = *pin++;
					*poutw++ = *pin++;
					*poutw++ = *pin++;
					*poutw++ = *pin++;
					// skip 4 planes of 8-plane tile
					pin += 4;
				}
				// next line of PI1
				pout += (320 >> (4-2));
			}
		}

		otx += g_allargs.tilesize;
		if ((otx + g_allargs.tilesize) > 320)
		{
			otx = 0;
			oty += g_allargs.tilesize;
			if ((oty + g_allargs.tilesize) > 200)
			{
				oty = 0;

				// flush image
				emit_pi1(outfile, buffer4plane, _ppalettergb, /*fromwords=*/(g_allargs.tilesize!=8), pi1_index++);
				memset(buffer4plane, 0, 32000);
			}
		}

		s_tileplanardictionary.pop_front();
	}

	// if some tiles are still pending, flush remainder
	if (otx > 0 || oty > 0)
		emit_pi1(outfile, buffer4plane, _ppalettergb, /*fromwords=*/(g_allargs.tilesize!=8), pi1_index++);

	delete[] buffer4plane;
}

// --------------------------------------------------------------------
//	emit tile library data file
// --------------------------------------------------------------------

static void emit_tiles_cct(std::string outfile, RGB *_ppal24)
{
//	std::string filename("tiles.cct");

	// construct output filename: [infile.ccm]

	//std::string outfile;
	//if (g_allargs.outfile.length() > 0)
	//{
	//	outfile = g_allargs.outfile;
	//}
	//else
	//{
	//	outfile = g_allargs.sourcefile;
	//}

	replace_extension(outfile, "cct");

	FILE* h = fopen(outfile.c_str(), "wb");
	if (h)
	{
		// write palette (16*2)
		{
			uword_t palette4bit[16];

			for (int i = 0; i < 16; i++)
			{
				word_t r = word_t(_ppal24[i].red);
				word_t g = word_t(_ppal24[i].green);
				word_t b = word_t(_ppal24[i].blue);

				// reformat 24bit 8:8:8 into STE swizzled 4:4:4 format
				word_t ste_r = (r >> 5) | (((r >> 4) & 1) << 3);
				word_t ste_g = (g >> 5) | (((g >> 4) & 1) << 3);
				word_t ste_b = (b >> 5) | (((b >> 4) & 1) << 3);

				uword_t ste_col = uword_t((ste_r << 8) | (ste_g << 4) | ste_b);

				// reformat to bigendian for intel->motorola
				palette4bit[i] = endianswap16(ste_col);
			}

			fwrite(palette4bit, 1, sizeof(palette4bit), h);
		}

		// write flags (1)
		{
			// interpretation:
			// 0=16x16 (legacy)
			// 3=8x8
			// 4=16x16
			// 5=32x32

			ubyte_t flags = 0;
			flags = log2<ubyte_t>((ubyte_t)g_allargs.tilesize);
			fwrite(&flags, 1, sizeof(flags), h);
		}

		// write #bitplanes (1)
		{
			ubyte_t count = g_allargs.planes;
			fwrite(&count, 1, sizeof(count), h);
		}

		// write tile count (2)
		{
			word_t count = s_tileplanardictionary.size();
			count = endianswap16(count);
			fwrite(&count, 1, sizeof(count), h);
		}

		// write tiles (n)
		{
			uword_t tilespace[32*(8*(32>>4))]; // worst: 32x32x8

			int tileplanes = g_allargs.planes;


			for (tileplanardictionary_t::iterator it = s_tileplanardictionary.begin(); it != s_tileplanardictionary.end(); it++)
			{

				// extract planes
				if (g_allargs.tilesize == 8)
				{
					// 8x8 mode encoded differently for movep
					const int tilebytes = tileplanes * g_allargs.tilesize * (g_allargs.tilesize>>3);
					uword_t *tilew = *it;
					ubyte_t *tileb = (ubyte_t *)tilew;
					ubyte_t *ptilespaceb = (ubyte_t *)tilespace;
					for (int y = 0; y < 8; y++)
					{
						for (int p = 0; p < tileplanes; p++)
							*ptilespaceb++ = *tileb++;
						tileb += (8 - tileplanes);
					}

					fwrite(tilespace, 1, tilebytes, h);
				}
				else
				{
					// any other size
					const int tilewords = tileplanes * g_allargs.tilesize * (g_allargs.tilesize>>4);
					uword_t *tile = *it;
					uword_t *ptilespace = tilespace;
					for (int y = 0; y < 16; y++)
					{
						for (int p = 0; p < tileplanes; p++)
							*ptilespace++ = *tile++;
						tile += (8 - tileplanes);
					}

					// reformat image to bigendian for intel->motorola
					for (int i = 0; i < tilewords; i++)
						tilespace[i] = endianswap16(tilespace[i]);

					fwrite(tilespace, 1, (tilewords * 2), h);
				}
			}
		}

		std::cout << "emitted tiles [" << outfile.c_str() << "]" << std::endl;

		fclose(h);
	}
}

// --------------------------------------------------------------------
//	emit map data file indicating where tiles go in original image
// --------------------------------------------------------------------

static void emit_tilemap_16bitraw(std::string outfile, long_t *_ptilemap, int _tilemapw, int _tilemaph)
{
	long_t v;

	replace_extension(outfile, "bin");

	FILE *hmap = fopen(outfile.c_str(), "wb");
	if (hmap)
	{
		int mapentries = _tilemapw * _tilemaph;
		for (int i = 0; i < mapentries; i++)
		{
			uword_t tile_index_mot = endianswap16(uword_t(_ptilemap[i]));
			fwrite(&tile_index_mot, 1, sizeof(tile_index_mot), hmap);
		}

		fclose(hmap);
	}

	std::cout << "emitted tilemap [" << outfile.c_str() << "]" << std::endl;
}

// --------------------------------------------------------------------
//	emit tile library data file for RBasic+ (Jaguar)
// --------------------------------------------------------------------

#pragma pack(push,2) // it's a file format :-/
struct bmp_header
{
	// file
	uword_t	file_sig;
	ulong_t	file_size;
	uword_t	file_rsv1;
	uword_t	file_rsv2;
	ulong_t file_offset_pixeldata;
	// dib
	ulong_t dib_size_header;
	ulong_t dib_w;
	ulong_t dib_signed_h;
	uword_t dib_planes;
	uword_t dib_bpp;
	ulong_t dib_format;
	ulong_t dib_rawsize;
	ulong_t dib_print_xres;
	ulong_t dib_print_yres;
	ulong_t dib_ncolours;
	ulong_t dib_icolours;
	// pixeldata
};
#pragma pack(pop)

static void emit_tiles_rbplus(std::string outfile, RGB *_ppal24)
{
	// construct output filename: [infile.ccm]
	//std::string outfile;
	//if (g_allargs.outfile.length() > 0)
	//{
	//	outfile = g_allargs.outfile;
	//}
	//else
	//{
	//	outfile = g_allargs.sourcefile;
	//}

	replace_extension(outfile, "bmp");

	FILE* h = fopen(outfile.c_str(), "wb");
	if (h)
	{
		int tracked_filesize = 0;
		ulong_t zero = 0;

		int tilesize = g_allargs.tilesize;
		// create header (16 x (tiles*count) x nBPP)

		int pixeldatasize = (tilesize * tilesize) * s_tile8dictionary.size();
		if (g_allargs.planes <= 4)
			pixeldatasize >>= 1;

		bmp_header bmph;
		bmph.file_sig = 'MB';
		bmph.file_size = 0; // patch later
		bmph.file_rsv1 = 0;
		bmph.file_rsv2 = 0;
		bmph.file_offset_pixeldata = 0; // patch later
		bmph.dib_size_header = 40;
		bmph.dib_w = tilesize;
#if (BMP_TYPICAL_ROW_ORDER)
		bmph.dib_signed_h = (tilesize * s_tile8dictionary.size());
#else
		bmph.dib_signed_h = -(tilesize * s_tile8dictionary.size());
#endif
		bmph.dib_planes = 1;
		bmph.dib_bpp = (g_allargs.planes <= 4) ? 4 : 8;
		bmph.dib_format = 0;
		bmph.dib_rawsize = pixeldatasize;
		bmph.dib_print_xres = tilesize;
		bmph.dib_print_yres = (tilesize * s_tile8dictionary.size());
		bmph.dib_ncolours = (g_allargs.planes <= 4) ? 16 : 256;
		bmph.dib_icolours = bmph.dib_ncolours;
		// palette follows - no padding required
		// image pixels follow - with padding

		// write header
		fwrite(&bmph, 1, sizeof(bmph), h); tracked_filesize += sizeof(bmph);

		// write palette (16*BGRA)
		{
			for (int i = 0; i < bmph.dib_ncolours; i++)
			{
				ubyte_t r = ubyte_t(_ppal24[i].red);
				ubyte_t g = ubyte_t(_ppal24[i].green);
				ubyte_t b = ubyte_t(_ppal24[i].blue);

				fwrite(&b, 1, 1, h);
				fwrite(&g, 1, 1, h);
				fwrite(&r, 1, 1, h);
				fwrite(&zero, 1, 1, h);

				tracked_filesize += 4;
			}
		}

		// align the pixeldata
		int aligned_filepos = (tracked_filesize + 3) & ~3;
		int padding = aligned_filepos - tracked_filesize;
		tracked_filesize += padding;
		while (padding--)
		{
			fwrite(&zero, 1, 1, h);
		}

		int offset_pixeldata = tracked_filesize;

		// write tiles (n)
		{
			int tileplanes = g_allargs.planes;
			int tilepixels = tilesize * tilesize;
			ubyte_t tilespace[64];	// expected worst case is 32

#if (BMP_TYPICAL_ROW_ORDER)
			// so we need to emit the tiles in reverse. what a crock...
			for (tile8dictionary_t::reverse_iterator it = s_tile8dictionary.rbegin(); it != s_tile8dictionary.rend(); it++)
#else
			for (tile8dictionary_t::iterator it = s_tile8dictionary.begin(); it != s_tile8dictionary.end(); it++)
#endif
			{
				ubyte_t *tile = *it;

#if (BMP_TYPICAL_ROW_ORDER)
				for (int r = tilesize-1; r >= 0; r--)
#else
				for (int r = 0; r < tilesize; r++)
#endif
				{
					// access row
					ubyte_t *tilerow = &tile[r * tilesize];

					// emit a row at a time, so we can BMP-flip the row order :(

					// extract pixels
					if (tileplanes <= 4)
					{
						// pack as nibbles
						for (int a = 0, p = 0; a < (tilesize >> 1); ++a, p += 2)
						{
							ubyte_t pixel = (tilerow[p + 0] << 4) | (tilerow[p + 1] << 0);
							tilespace[a] = pixel;
						}

						fwrite(tilespace, 1, tilesize >> 1, h); tracked_filesize += tilesize >> 1;
					}
					else
					{
						// already in correct 8bit chunky format
						fwrite(tilerow, 1, tilesize, h); tracked_filesize += tilesize;
					}
				}
			}
		}

		// patch filesize
		fseek(h, 2, 0);
		fwrite(&tracked_filesize, 1, 4, h);

		// patch pixeldata offset
		fseek(h, 10, 0);
		fwrite(&offset_pixeldata, 1, 4, h);

		std::cout << "emitted tiles [" << outfile.c_str() << "]" << std::endl;

		fclose(h);
	}
}

// --------------------------------------------------------------------
//	map & tile cutting mode
// --------------------------------------------------------------------

void emit_tga(const char* const _name, RGB *data, int _w, int _h)
{
	// emit trivial targa format, out of sheer lazyness
	FILE *fptr = fopen(_name, "wb");
	if (fptr)
	{
		putc(0,fptr);
		putc(0,fptr);
		putc(2,fptr);                         /* uncompressed RGB */
		putc(0,fptr); putc(0,fptr);
		putc(0,fptr); putc(0,fptr);
		putc(0,fptr);
		putc(0,fptr); putc(0,fptr);           /* X origin */
		putc(0,fptr); putc(0,fptr);           /* y origin */
		putc((_w & 0x00FF),fptr);
		putc((_w & 0xFF00) / 256,fptr);
		putc((_h & 0x00FF),fptr);
		putc((_h & 0xFF00) / 256,fptr);
		putc(24,fptr);                        /* 24 bit bitmap */
		putc(0,fptr);

		for (int y = _h-1; y >= 0; y--)
		{
			for (int x = 0; x < _w; x++)
			{
				const RGB &rgbpan = data[(y * _w) + x];

				putc(rgbpan.blue,		fptr);
				putc(rgbpan.green,		fptr);
				putc(rgbpan.red,		fptr);
			}
		}

		fclose(fptr);
	}
	else
	{
		std::cerr << "warning: could not create review TGA file: " << _name << std::endl;
	}
}

bool do_tilemap()
{
	ulong_t sourcew = 0;
	ulong_t sourceh = 0;
	ulong_t sourced = 0;
	RGB *m_psourcergb = 0;
	RGB *m_ppalettergb = 0;
	ubyte_t *m_psourcecol = 0;
	ubyte_t *m_psourcecol2 = 0;
	ubyte_t *m_poriginalsrccol = 0;

	long_t *psrcmap = 0;
	int tilesrcw = 0;
	int tilesrch = 0;
	long_t *ptilemap = 0;
	int tilemapw = 0;
	int tilemaph = 0;

	std::cout << "map / tile cutting mode..." << std::endl;

	if (image_load(g_allargs.sourcefile.c_str(), m_psourcergb, m_psourcecol, m_ppalettergb, sourcew, sourceh, sourced))
	{
		if (1)
		{
			std::cout << "read [" << g_allargs.sourcefile.c_str() << "] with colour depth " << sourced << " and dimensions " << sourcew << " x " << sourceh << std::endl;

			if ((sourcew & (g_allargs.tilesize - 1)) || (sourceh & (g_allargs.tilesize - 1)))
			{
				std::cerr << "error: source image is not a multiple of " << g_allargs.tilesize << " pixels on both axes - check the map!" << std::endl;
				printf("press any key...\n");
				getchar();	
				exit(1);
			}

			if (m_psourcecol)
			{
				bool doremap = true;

				if (sourced > g_allargs.planes)
				{
					if (g_allargs.noremap)
					{
						std::cout << "warning: source image is indexed and > " << g_allargs.planes << " index bits - will use first 16 colours only (--no-remap)" << std::endl;
						doremap = false;
					}
					else
					{
						std::cout << "warning: source image is indexed and > " << g_allargs.planes << " index bits - will be remapped to external palette" << std::endl;
					}
				}
				else
				{
					if (g_allargs.forceremap)
					{
						std::cout << "warning: source image is indexed and <= " << g_allargs.planes << " index bits - but will remap anyway (--force-remap)" << std::endl;
					}
					else
					{
						std::cout << "warning: source image is indexed and <= " << g_allargs.planes << " index bits - will pass through without remapping" << std::endl;
						doremap = false;
					}
				}

				if (doremap)
				{
					m_poriginalsrccol = m_psourcecol;
					m_psourcecol = 0;
				}
				else
				{
					// check that indexed colours are now in range for bitplanes specified
					int maxcolidx = 1 << g_allargs.planes;
					int highest = 0;
					for (size_t p = 0; p < sourcew*sourceh; p++)
					{
						if ((m_psourcecol[p] >= maxcolidx) && (m_psourcecol[p] != g_allargs.keycol))
						{
							highest = xmax(highest, m_psourcecol[p]);
							m_psourcecol[p] = g_allargs.keycol;
//							break;
						}
					}

					if (highest)
					{
						std::cout << "warning: --no-remap was specified, but source colour indices exceed target bitplane count!" << std::endl;
						std::cout << "highest usable index = " << maxcolidx - 1 << ", highest index encountered = " << highest << std::endl;
						std::cout << "all illegal colours have been converted to --key-colour..." << std::endl;
					}
				}

/*
				int maxcolidx = 1 << g_allargs.planes;
				// check that indexed colours are in range for bitplanes specified
				for (size_t p = 0; p < sourcew*sourceh; p++)
				{
					if (m_psourcecol[p] >= maxcolidx)
					{
						std::cout << "warning: source image is indexed, but values exceed target bitplane count: remapping..." << std::endl;
						// ignore colourmap
						m_poriginalsrccol = m_psourcecol;
						m_psourcecol = 0;
						break;
					}
				}

				std::cout << "adopting palette and colourmap provided by source image..." << std::endl;
*/
			}

			if (m_psourcecol)
			{
				std::cout << "using palette and colourmap provided by source image..." << std::endl;

				if (!g_allargs.quickcut && !g_allargs.quiet)
				{
					// review palette
					std::cout << "summary of source palette for indexed source image being remapped" << std::endl;
					review_palettes(m_ppalettergb, NULL, sourced);
				}
			}
			else
			{
				std::cout << "reducing " << ((sourced > 0) ? sourced : 24) << "-bit image to external fixed palette..." << std::endl;
				m_psourcecol = new ubyte_t[sourcew * sourceh]; // (ubyte_t*)malloc(sourcew * sourceh);
				if (g_allargs.ccfields == 2)
					m_psourcecol2 = new ubyte_t[sourcew * sourceh]; // (ubyte_t*)malloc(sourcew * sourceh);
				// reduce source image to indexed colour with fixed palette
				reduce_image24(m_psourcergb, 0, m_psourcecol, m_psourcecol2, sourcew, sourceh, sourced, m_ppalettergb);
			}

			for (int ccf = 0; ccf < g_allargs.ccfields; ccf++)
			{
				std::cout << "extracting unique tiles..." << std::endl;

				// generate tile dictionary from image
				if (ccf == 0)
					process_tilemap(psrcmap, tilesrcw, tilesrch, ptilemap, tilemapw, tilemaph, m_psourcergb, m_psourcecol, sourcew, sourceh);
				else
					process_tilemap(psrcmap, tilesrcw, tilesrch, ptilemap, tilemapw, tilemaph, m_psourcergb, m_psourcecol2, sourcew, sourceh);

				std::cout << "...done" << std::endl;
				// calculate compressed footprint
				float footprint = (float)s_tile8dictionary.size() / float(tilesrcw*tilesrch);
				std::cout << "final dictionary contains " << s_tile8dictionary.size() << " unique tiles with footprint @ " << footprint << " of original image." << std::endl;

				if (ccf == 0)
				{
					if (footprint > 0.5f)
						std::cout << " tile reuse is terrible - source image was rendered in crayon by a womble" << std::endl;
					if (footprint > 0.3f)
						std::cout << " tile reuse is poor - check tile alignment at top left and lower right of source image" << std::endl;
					else
					if (footprint > 0.2f)
						std::cout << " tile reuse is fair - but carefully recheck tile alignment to be sure?" << std::endl;
					else
					if (footprint > 0.1f)
						std::cout << " tile reuse seems ok but keep an eye on things..." << std::endl;
					else
						std::cout << " tile reuse is good! > l33t mapping 5ki11z <" << std::endl;
				}

				// convert 8bit indexed tiles into 8-plane graphics
				remap_dictionary_tiles();

				// construct output filename: [infile]
				std::string outfile;
				if (g_allargs.outfile.length() > 0)
				{
					outfile = g_allargs.outfile;
				}
				else
				{
					outfile = g_allargs.sourcefile;
				}

				remove_extension(outfile);
				if (g_allargs.ccfields == 2)
				{
					if (ccf == 0)
						outfile.append("0");
					else
						outfile.append("1");
				}

				if (g_allargs.outmode == Outmode_DEGAS)
				{
					// emit tiles to a series of 320x200 .PI1 images
					emit_tiles_degas(outfile, m_ppalettergb);
				}
				else
				if (g_allargs.outmode == Outmode_DIRECT)
				{
					emit_tiles_cct(outfile, m_ppalettergb);
				}
				else
				if (g_allargs.outmode == Outmode_RBPLUS)
				{
					emit_tiles_rbplus(outfile, m_ppalettergb);
				}

				if (ptilemap)
				{
					// construct output filename: [infile.map]

					if (g_allargs.outmode == Outmode_RBPLUS)
					{
						emit_tilemap_16bitraw(outfile.c_str(), ptilemap, tilemapw, tilemaph);
					}
					else
					{
						int tilestep = log2<int>(g_allargs.tilesize);
						emit_tilemap_ccm(outfile.c_str(), ptilemap, tilemapw, tilemaph, tilestep);
					}
				}

			} // ccf loop

			// emit tilemap review files
			{
				int tilesize = g_allargs.tilesize;

				// only emit mapvis review file if requested
				if (g_allargs.visual)
				{
					// rebuild source from tiles
					for (int yy = 0; yy < tilesrch; yy++)
					{
						for (int xx = 0; xx < tilesrcw; xx++)
						{
							RGB *pdst = &m_psourcergb[((yy * tilesize) * sourcew) + (xx * tilesize)];

							//int index = -1;
							//
							//int mx = xx - g_allargs.maprect_xpos;
							//int my = yy - g_allargs.maprect_ypos;
							//if ((mx >= 0 && my >= 0) && (mx < g_allargs.maprect_xsize && my < g_allargs.maprect_ysize))
							//{
							//	index = ptilemap[(my * tilemapw) + mx];
							//}

							int index = psrcmap[(yy*tilesrcw)+xx];

							bool inside = false;
							int mx = xx - g_allargs.maprect_xpos;
							int my = yy - g_allargs.maprect_ypos;
							if ((mx >= 0 && my >= 0) && 
								(mx < g_allargs.maprect_xsize && my < g_allargs.maprect_ysize))
							{
								inside = true;
							}

							// unfortunately we're storing the tiles in a list - better change this
							tile8dictionary_t::iterator it8 = s_tile8dictionary.begin();
							for (int i = 0; i < index; i++, it8++);

							if (inside)
							{
								// valid index, part of the map - fill with tile
								u8 *psrc = *it8;
								for (int py = 0; py < tilesize; py++)
								{
									for (int px = 0; px < tilesize; px++)
									{
										u8 c = psrc[(py * tilesize) + px];
										pdst[(py * sourcew) + px] = m_ppalettergb[c];
									}
								}
							}
							else
							{
								// not part of the map - darken pixels
								u8 *psrc = *it8;
								for (int py = 0; py < tilesize; py++)
								{
									for (int px = 0; px < tilesize; px++)
									{
										u8 c = psrc[(py * tilesize) + px];
										RGB v = m_ppalettergb[c];
										v.red >>= 1;
										v.green >>= 1;
										v.blue >>= 1;
										pdst[(py * sourcew) + px] = v;
									}
								}
							}
						}
					}

					std::string reviewfile(g_allargs.outfile);
					remove_extension(reviewfile);
					reviewfile.append("_mapvis.tga");

					emit_tga(reviewfile.c_str(), m_psourcergb, sourcew, sourceh);
					std::cout << "emitted map visualisation [" << reviewfile.c_str() << "]" << std::endl;
				}

				// only emit map index review file if requested
				if (g_allargs.visual)
				{
					// rebuild source from tiles
					for (int yy = 0; yy < tilesrch; yy++)
					{
						for (int xx = 0; xx < tilesrcw; xx++)
						{
							RGB *pdst = &m_psourcergb[((yy * tilesize) * sourcew) + (xx * tilesize)];

							//int index = -1;

							//int mx = xx - g_allargs.maprect_xpos;
							//int my = yy - g_allargs.maprect_ypos;
							//if ((mx >= 0 && my >= 0) && (mx < g_allargs.maprect_xsize && my < g_allargs.maprect_ysize))
							//{
							//	index = ptilemap[(my * tilemapw) + mx];
							//}

							int index = psrcmap[(yy*tilesrcw)+xx];

							bool inside = false;
							int mx = xx - g_allargs.maprect_xpos;
							int my = yy - g_allargs.maprect_ypos;
							if ((mx >= 0 && my >= 0) && 
								(mx < g_allargs.maprect_xsize && my < g_allargs.maprect_ysize))
							{
								inside = true;
							}

							// unfortunately we're storing the tiles in a list - better change this
							tile24dictionary_t::iterator it24 = s_tile24dictionary.begin();
							for (int i = 0; i < index; i++, it24++);

							if (inside)
							{
								// valid index, part of the map - fill with tile
								RGB *psrc = *it24;
								for (int py = 0; py < tilesize; py++)
								{
									for (int px = 0; px < tilesize; px++)
									{
										pdst[(py * sourcew) + px] = psrc[(py * tilesize) + px];

										plot6x6hex3(pdst, sourcew, index, 1, 1);
									}
								}
							}
							else
							{
								// not part of the map - darken pixels
								RGB *psrc = *it24;
								for (int py = 0; py < tilesize; py++)
								{
									for (int px = 0; px < tilesize; px++)
									{
										RGB v = psrc[(py * tilesize) + px];
										v.red >>= 1;
										v.green >>= 1;
										v.blue >>= 1;
										pdst[(py * sourcew) + px] = v;

										plot6x6hex3(pdst, sourcew, index, 1, 1);
									}
								}
							}
						}
					}

					std::string reviewfile(g_allargs.outfile);
					remove_extension(reviewfile);
					reviewfile.append("_index.tga");

					emit_tga(reviewfile.c_str(), m_psourcergb, sourcew, sourceh);
					std::cout << "emitted map index visualisation [" << reviewfile.c_str() << "]" << std::endl;
				}

				// only emit tags review if requested
				if (g_allargs.tiletags.length() > 0)
				{
					std::ifstream tiletags(g_allargs.tiletags.c_str());
					if (!(tiletags.is_open() && tiletags.good()))
					{
						std::cerr << "error: unable to locate or read specified tiletags CSV file!" << std::endl;
						printf("press any key...\n");
						getchar();	
						exit(1);
					}

					// create maskindex from marked indices
					const int numtiles = s_tile24dictionary.size();
					u8 *pmaskindex = new u8[numtiles];
					memset(pmaskindex, 0, numtiles);

					// parse CSV
					for (CSVIterator loop(tiletags); loop != CSVIterator(); ++loop)
					{
						const CSVRow& r = (*loop);
						size_t s = r.size();
						if (s > 0)
						{
							std::string inspect = r[0];
							if (inspect[0] == ';')
							{
								// skip comment
							}
							else
							{
								int column_value = 0;
								int column_index = 0;

								std::cout << "reading tiletags: ";
		
								std::string colval = r[column_index++];
								std::transform(colval.begin(), colval.end(), colval.begin(), sanitise_csv_integer);
								char *pend = 0;
								column_value = strtol(colval.c_str(), &pend, 16);
								printf("index: %s\n", colval.c_str());
								if ((column_value >= 0) && (column_value <= numtiles))
								{	
									// tag this tile
									pmaskindex[column_value] = 1;
									std::cout << "tagged tile index [" << column_value << "]" << std::endl;
								}
								else
								{
									fprintf(stderr, "error: invalid tile index field (%d) in tiletags CSV file!\n", column_value);
									printf("press any key...\n");
									getchar();	
									exit(1);				
								}
							}
						}
					}

					// rebuild source from tiles
					for (int yy = 0; yy < tilesrch; yy++)
					{
						for (int xx = 0; xx < tilesrcw; xx++)
						{
							RGB *pdst = &m_psourcergb[((yy * tilesize) * sourcew) + (xx * tilesize)];

							//int index = -1;

							//int mx = xx - g_allargs.maprect_xpos;
							//int my = yy - g_allargs.maprect_ypos;
							//if ((mx >= 0 && my >= 0) && (mx < g_allargs.maprect_xsize && my < g_allargs.maprect_ysize))
							//{
							//	index = ptilemap[(my * tilemapw) + mx];
							//}

							int index = psrcmap[(yy*tilesrcw)+xx];

							bool inside = false;
							int mx = xx - g_allargs.maprect_xpos;
							int my = yy - g_allargs.maprect_ypos;
							if ((mx >= 0 && my >= 0) && 
								(mx < g_allargs.maprect_xsize && my < g_allargs.maprect_ysize))
							{
								inside = true;
							}

							if (inside)
							{
								if (pmaskindex[index])
								{
									for (int py = 0; py < tilesize; py++)
									{
										for (int px = 0; px < tilesize; px++)
										{
											pdst[(py * sourcew) + px].red = ~(pdst[(py * sourcew) + px].red >> 1);
											pdst[(py * sourcew) + px].green = ~(pdst[(py * sourcew) + px].green >> 1);
											pdst[(py * sourcew) + px].blue = ~(pdst[(py * sourcew) + px].blue >> 1);
										}
									}
								}
							}
						}
					}

					delete[] pmaskindex;

					{
						std::string reviewfile(g_allargs.outfile);
						remove_extension(reviewfile);
						reviewfile.append("_tags.tga");

						emit_tga(reviewfile.c_str(), m_psourcergb, sourcew, sourceh);
						std::cout << "emitted map tags visualisation [" << reviewfile.c_str() << "]" << std::endl;
					}

				}
			}

			if (m_psourcergb)
				delete[] m_psourcergb;
			if (m_ppalettergb)
				delete[] m_ppalettergb;

			if (m_psourcecol)
				delete[] m_psourcecol;
			if (m_psourcecol2)
				delete[] m_psourcecol2;
		}
		else
		{
			std::cerr << "warning: could not decode [" << g_allargs.sourcefile.c_str() << "] source image" << std::endl;
		}
	}
	else
	{
		std::cerr << "warning: could not open [" << g_allargs.sourcefile.c_str() << "] source file" << std::endl;
	}

	return true;
}

// --------------------------------------------------------------------
//	sprite cutting storage
// --------------------------------------------------------------------

// todo: tidy this up!

static int g_sprite_bpwidth = 0;
static int g_sprite_width = 0;
static int g_sprite_height = 0;
static int g_spritesource_w = 0;
static int g_spritesource_h = 0;

class preshift_t
{
public:

	word_t shift_;
	word_t planes_;
	word_t height_;
	word_t wordwidth_;
	word_t planewords_;
	uword_t *data_;
	ubyte_t *p68kbin;
	size_t size68kbin;

	preshift_t(int _shift, int _planes, int _height, int _wordwidth, uword_t *_data)
		: shift_(_shift),
		  planes_(_planes),
		  height_(_height),
		  wordwidth_(_wordwidth),
		  data_(_data),
		  p68kbin(0),
		  size68kbin(0)
	{
		planewords_ = _height * _wordwidth;
	}

	~preshift_t()
	{
		// do not delete here - no refcounting on allocations happening within STL container
	}

	void release()
	{
		delete[] data_; data_ = 0;
	}
};

typedef std::list<preshift_t> preshiftlist_t;

class spriteframe_t
{
public:

	word_t w_, h_;
	word_t xo_, yo_;
	uword_t *mask_;
	uword_t *colr_;

	preshiftlist_t pscolr_frames_;
	preshiftlist_t psmask_frames_;

	std::vector<int> lineorder;

	spriteframe_t(int _w, int _h, int _xo, int _yo, uword_t *_mask, uword_t *_colr)
		: w_(_w),
		  h_(_h),
		  xo_(_xo),
		  yo_(_yo),
		  mask_(_mask),
		  colr_(_colr)
	{
	}

	~spriteframe_t()
	{
		// do not delete here - no refcounting on allocations happening within STL container
	}

	void generate_mask_preshifts(int _range, int _step)
	{
		int src_wordwidth = ((w_ + 15) & -16) >> 4;

		for (int s = 0; s < _range; s += _step)
		{
//			printf("\n");

			int dst_wordwidth = (((w_ + s) + 15) & -16) >> 4;
			int preshiftwords = h_ * dst_wordwidth;

//			printf("shift: %d\n", s);
//			printf("dst_wordwidth: %d\n", dst_wordwidth);
//			printf("preshiftwords: %d\n", preshiftwords);

			uword_t *preshift = new uword_t[preshiftwords];
			uword_t *pout = preshift;

			for (int y = 0; y < h_; y++)
			{
//				printf("\n");

				// preload shift register
				uword_t *pin = &mask_[y * src_wordwidth];
				u32 shiftreg = 0;

				for (int dw = 0; dw < dst_wordwidth; dw++)
				{
//					printf(" sr:%08x", (u32)shiftreg);

					shiftreg <<= 16;

					// fetch source while data remains
					if (dw < src_wordwidth)
						shiftreg |= u32(u16(~(*pin++)));

					uword_t m = ((uword_t)(shiftreg >> s));

//					printf(" m:%04x", (u32)m);

					*pout++ = m;
				}
			}

			psmask_frames_.push_back(preshift_t(s, 1, h_, dst_wordwidth, preshift));
		}
	}

	void release()
	{
		delete[] mask_; mask_ = 0;
		delete[] colr_; colr_ = 0;
	}
};

typedef std::list<spriteframe_t> spritelist_t;

// todo: tidy this up!

spritelist_t g_spriteframes;

int g_sprite_framewidth;
int g_sprite_frameheight;
int g_sprite_framebpwidth;

// --------------------------------------------------------------------
//	prepare sprite cutting sequence
// --------------------------------------------------------------------

void init_sprite_sequence(int _xs, int _ys, int _sw, int _sh)
{
	for (spritelist_t::iterator it = g_spriteframes.begin(); it != g_spriteframes.end(); it++)
		it->release();

	g_spriteframes.clear();

	g_spritesource_w = _sw;
	g_spritesource_h = _sh;

	// sprite width in bitplane words
	g_sprite_bpwidth = (_xs + (16-1)) & -16;
	g_sprite_width = _xs;
	g_sprite_height = _ys;

	// allocate sprite buffer for chunky processing (default to keycol)
	delete[] g_colrbuffer8;
	g_colrbuffer8 = new ubyte_t[g_sprite_bpwidth * g_sprite_height];
	memset(g_colrbuffer8, g_allargs.keycol, g_sprite_bpwidth * g_sprite_height);

	// allocate sprite buffer for bitplane processing
	delete[] g_colrbufferbpl;
	g_colrbufferbpl = new uword_t[(g_sprite_bpwidth * g_sprite_height) >> 1];
	memset(g_colrbufferbpl, 0, g_sprite_bpwidth * g_sprite_height);


	// allocate mask buffer for chunky processing (default to transparency)
	delete[] g_maskbuffer8;
	g_maskbuffer8 = new ubyte_t[g_sprite_bpwidth * g_sprite_height];
	memset(g_maskbuffer8, 0xFF, g_sprite_bpwidth * g_sprite_height);

	// allocate mask buffer for bitplane processing
	delete[] g_maskbufferbpl;
	g_maskbufferbpl = new uword_t[(g_sprite_bpwidth * g_sprite_height) >> 1];
	memset(g_maskbufferbpl, 0, g_sprite_bpwidth * g_sprite_height);
}

// --------------------------------------------------------------------
//	cut a single frame from source image
// --------------------------------------------------------------------

void add_sprite_frame(ubyte_t* _psource8, int _x, int _y, bool _xflip, bool _yflip, int _hs, int _vs, RGB* _ppal, RGB* _pvisualrgb)
{
	memset(g_colrbuffer8, g_allargs.keycol, g_sprite_bpwidth * g_sprite_height);
	memset(g_maskbuffer8, 0xFF, g_sprite_bpwidth * g_sprite_height);

	int cxmin = 0;
	int cymin = 0;
	int cxmax = g_sprite_width;
	int cymax = g_sprite_height;

	// cutting may read source either x-flipped, y-flipped, both or neither so we configure loops here
	int pxs = _xflip ? (g_sprite_width-1) : 0;
	int pys = _yflip ? (g_sprite_height-1) : 0;
	int pxi = _xflip ? -1 : 1;
	int pyi = _yflip ? -1 : 1;

	if (g_allargs.visual && _pvisualrgb)
	{
		for (int lpy = 0, py = pys; lpy < g_sprite_height; lpy++, py += pyi)
		{
			for (int lpx = 0, px = pxs; lpx < g_sprite_width; lpx++, px += pxi)
			{
				ubyte_t c = _psource8[((py + _y) * g_spritesource_w) + (px + _x)];

				int frame = g_spriteframes.size();

				RGB &pix = _pvisualrgb[(frame * (g_sprite_width*g_sprite_height)) + (lpy*g_sprite_width) + lpx];

				if (c == g_allargs.keycol)
				{	
					pix.red = g_allargs.keyrgb.red;
					pix.green = g_allargs.keyrgb.green;
					pix.blue = g_allargs.keyrgb.blue;
				}
				else
				{
					pix = _ppal[c];
				}
			}
		}
	}

	if (g_allargs.preview)
	{
		printf("\n");
		for (int lpy = 0, py = pys; lpy < g_sprite_height; lpy++, py += pyi)
		{
			printf("\t|");
			for (int lpx = 0, px = pxs; lpx < g_sprite_width; lpx++, px += pxi)
			{
				ubyte_t c = _psource8[((py + _y) * g_spritesource_w) + (px + _x)];

				if (c == g_allargs.keycol)
					printf("  ");
				else
					printf("%02x", (int)c);
				if (lpx < g_sprite_width-1)
					printf(" ");
			}
			printf("|\n");
		}
		printf("\n");
	}

	if (g_allargs.spriteoptimize)
	{
		cxmin = g_sprite_width;
		cymin = g_sprite_height;
		cxmax = 0;
		cymax = 0;

		// optimize cutting area

		for (int lpy = 0, py = pys; lpy < g_sprite_height; lpy++, py += pyi)
		{
			for (int lpx = 0, px = pxs; lpx < g_sprite_width; lpx++, px += pxi)
			{
				ubyte_t c = _psource8[((py + _y) * g_spritesource_w) + (px + _x)];

				if (c != g_allargs.keycol)
				{
					// opaque pixel - adjust bounds
					cxmin = xmin(cxmin, px);
					cxmax = xmax(cxmax, px + 1);
					cymin = xmin(cymin, py);
					cymax = xmax(cymax, py + 1);
				}
			}
		}
	}

	// capture at least one pixel per frame
	if ((cxmax < cxmin) || (cymax < cymin))
	{
		cxmin = 0;
		cxmax = 1;
		cymin = 0;
		cymax = 1;
	}

	g_sprite_framewidth = cxmax - cxmin;
	g_sprite_frameheight = cymax - cymin;
	g_sprite_framebpwidth = (g_sprite_framewidth + (16-1)) & -16;

	// guard against active sprite source widths > 32 pixels for EMS/EMX formats
	if (
		(g_allargs.cutmode == Cutmode_EMSPR) ||
		(g_allargs.cutmode == Cutmode_EMXSPR)
	)
	{
		if ((g_sprite_framebpwidth >> 4) > 2)
		{
			std::cerr << "error: max active source width for EMS/EMX sprite is 32 (2x words). this frame has a max source width of " << g_sprite_bpwidth
				<< std::endl;
			printf("press any key...\n");
			getchar();
			exit(1);
		}
	}

	// cut chunky sprite to chunky colrbuffer & maskbuffer

	for (int psy = cymin, py = 0; psy < cymax; psy++, py++)
	{
		//int psyr = _yflip ? ((g_sprite_height - 1) - psy) : psy;
		int psyr = _yflip ? ((cymax - 1) - py) : (cymin + py);

		for (int psx = cxmin, px = 0; psx < cxmax; psx++, px++)
		{
			//int psxr = _xflip ? ((g_sprite_width - 1) - psx) : psx;
			int psxr = _xflip ? ((cxmax - 1) - px) : (cxmin + px);

			ubyte_t c = _psource8[((psyr + _y) * g_spritesource_w) + (psxr + _x)];

			// colour buffer receives actual colour, or 0 for keycol (so two colours can potentially map to 0)
			g_colrbuffer8[(py * g_sprite_framebpwidth) + px] = (c == g_allargs.keycol) ? 0x00 : c;
			// mask buffer receives 0 for colour, or 1 for keycol
			g_maskbuffer8[(py * g_sprite_framebpwidth) + px] = (c == g_allargs.keycol) ? 0xFF : 0x00;
		}
	}

	// chunky buffer -> planar buffer

	// colour component
	c2pw
	(
		/*src=*/g_colrbuffer8, /*dst*/g_colrbufferbpl,
		/*w=*/g_sprite_framebpwidth, /*h=*/g_sprite_frameheight,
		/*bytes_per_src_line=*/g_sprite_framebpwidth, /*words_per_dst_line=*/g_sprite_framebpwidth>>1,
		/*planes=*/8	//g_allargs.planes
	);

	// mask component
	c2pw
	(
		/*src=*/g_maskbuffer8, /*dst*/g_maskbufferbpl,
		/*w=*/g_sprite_framebpwidth, /*h=*/g_sprite_frameheight,
		/*bytes_per_src_line=*/g_sprite_framebpwidth, /*words_per_dst_line=*/g_sprite_framebpwidth>>1,
		/*planes=*/8	//g_allargs.planes
	);

	// generate the sprite data

	size_t spriteplane_wordsize = (g_sprite_framebpwidth >> 4) * g_sprite_frameheight;

	uword_t *maskdata = new uword_t[spriteplane_wordsize * 1];
	uword_t *colrdata = new uword_t[spriteplane_wordsize * g_allargs.planes];

	// copy mask data - just need one plane of the 8 we converted!
	for (size_t v = 0; v < spriteplane_wordsize; v++)
	{
		maskdata[v] = g_maskbufferbpl[v << 3];
	}

	// copy plane data - need N planes of the 8 we converted
	{
		int s = 0;
		int d = 0;
		for (size_t v = 0; v < spriteplane_wordsize; v++, s += 8, d += g_allargs.planes)
		{
			for (int p = 0; p < g_allargs.planes; p++)
				colrdata[d + p] = g_colrbufferbpl[s + p];
		}
	}

	std::cout << "created sprite frame " << g_spriteframes.size() << " from offset [" << cxmin << ", " << cymin << "] size [" << g_sprite_framewidth << " x " << g_sprite_frameheight << "]" << std::endl;
	g_spriteframes.push_back(spriteframe_t(g_sprite_framewidth, g_sprite_frameheight, cxmin, cymin, maskdata, colrdata));

	// optionally generate preshifts for mask or colour data
	if (g_allargs.genmask && (g_allargs.masklayout == Masklayout_PRESHIFT))
	{
		std::cout << "preshifting mask..." << std::endl;
		g_spriteframes.back().generate_mask_preshifts(16, 1);
	}
}

// --------------------------------------------------------------------

#include "emxgen.inl"

// --------------------------------------------------------------------
//	emit .sps spritesheet sequence file
// --------------------------------------------------------------------

bool emit_sprite_sequence()
{
	std::string fname;
	if (g_allargs.outfile.length() > 0)
	{
		fname = g_allargs.outfile;
	}
	else
	{
		fname = g_allargs.sourcefile;
		replace_extension(fname, "sps");
	}

	// generate the right extension for sprite types - because there are several sprite formats confusion is never far away
	if (g_allargs.cutmode == Cutmode_EMSPR)
		replace_extension(fname, "ems");
	else
	if (g_allargs.cutmode == Cutmode_EMXSPR)
		replace_extension(fname, "emx");


	FILE *hout = fopen(fname.c_str(), "wb");
	if (hout)
	{
		word_t wb;
		word_t wv;
		long_t lv;
		int tracked_filesize = 0;

		std::vector<int> sprite_index;

		// 4: size of file remaining (patched before closing)
		lv = 0;
		fwrite(&lv, 1, 4, hout);

		// 2: version info
		wv = endianswap16(SPS_VERSION);
		fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

		// 2: number of sprite frames in sequence
		wv = endianswap16((uword_t)g_spriteframes.size());
		fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

		// 2: number of bitplanes in sprite data
		wv = endianswap16(g_allargs.planes);
		fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

		// 2: flags
		// 0 0 0 0 0 0 0 0 0 0 0 0 0 L L M
		// M	mask present (1bpl mask data before colour data)
		// LL	mask layout mode (00=interleaved, 01=planar, 10=preshift, 11=codegen)
		wv = 0;
		wv |= (g_allargs.genmask) ? 1 : 0;

		if (g_allargs.cutmode == Cutmode_EMXSPR)
			wv |= 3 << 1;
		else
		if (g_allargs.cutmode == Cutmode_EMSPR)
			wv |= 2 << 1;
		else
		if (g_allargs.masklayout == Masklayout_PLANAR)
			wv |= 1 << 1;

//		wv |= (g_allargs.genmask && (g_allargs.masklayout == Masklayout_PLANAR)) ? 2 : 0;

		wv = endianswap16((uword_t)wv);
		fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

		// 1: preshift range
		wb = 0;
		fwrite(&wb, 1, 1, hout); tracked_filesize += 1;
		// 1: preshift step
		wb = 0;
		fwrite(&wb, 1, 1, hout); tracked_filesize += 1;

		// 2: source width
		wv = endianswap16((uword_t)g_sprite_width);
		fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
		// 2: source height
		wv = endianswap16((uword_t)g_sprite_height);
		fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

		// create sprite index (patched before closing)
		int sprite_index_fpos = tracked_filesize;
		lv = 0xBBBBBBBB;
		for (spritelist_t::iterator it = g_spriteframes.begin(); it != g_spriteframes.end(); it++)
		{
			fwrite(&lv, 1, 4, hout); tracked_filesize += 4;
		}

		// individual sprites follow
		for (spritelist_t::iterator it = g_spriteframes.begin(); it != g_spriteframes.end(); it++)
		{
			// record file-offset to sprite in sprite index, for patching later
			sprite_index.push_back(tracked_filesize);

			// 2: sprite frame pixel width
			wv = endianswap16(it->w_);
			fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
			// 2: sprite frame pixel height
			wv = endianswap16(it->h_);
			fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

			// 2: sprite frame xoff
			wv = endianswap16(it->xo_);
			fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
			// 2: sprite frame yoff
			wv = endianswap16(it->yo_);
			fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

			// width of plane in words
			int wordwidth = (it->w_ + (16 - 1)) >> 4;
			// size of plane in words
			int planewords = wordwidth * it->h_;

			// if preshifting is specified, additional indirection table used for preshifted sub-frames
			if (g_allargs.masklayout == Masklayout_PRESHIFT)
			{
				// preshifting is present so we need additional indirection for preshifted sub-frames
				std::vector<int> pscolr_index;
				std::vector<int> psmask_index;
				std::vector<int> pscolropt_index;
				std::vector<int> psmaskopt_index;

				lv = 0xBBBBBBBB;

				// write single offset for plain colourdata (no preshifts yet)
				int colr_preshift_index_fpos = tracked_filesize;
				fwrite(&lv, 1, 4, hout); tracked_filesize += 4;

				int colropt_preshift_index_fpos = tracked_filesize;
				if (g_allargs.cutmode == Cutmode_EMXSPR)
				{
					// write single offset for optimized colourdata (no preshifts yet)
					fwrite(&lv, 1, 4, hout); tracked_filesize += 4;
				}

				// write N offsets for plain mask preshifts
				int mask_preshift_index_fpos = tracked_filesize;
				for (preshiftlist_t::iterator itm = it->psmask_frames_.begin(); itm != it->psmask_frames_.end(); itm++)
				{
					fwrite(&lv, 1, 4, hout); tracked_filesize += 4;
				}

				int maskopt_preshift_index_fpos = tracked_filesize;
				if (g_allargs.cutmode == Cutmode_EMXSPR)
				{
					// write N offsets for optimixed mask preshifts
					for (preshiftlist_t::iterator itm = it->psmask_frames_.begin(); itm != it->psmask_frames_.end(); itm++)
					{
						fwrite(&lv, 1, 4, hout); tracked_filesize += 4;
					}
				}

				// plain colour 
				{
					// track colour preshifts
					// todo: assumes only one for now
					pscolr_index.push_back(tracked_filesize);

					// write word-width for this preshift (will be 1, 2 or 3 in this mode for STE/F030 blitter endmasks)
					wv = endianswap16(wordwidth);
					fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

					// write colour words for N lines, lines(planes(words))) order
					for (int y = 0; y < it->h_; y++)
					{
						// reorder colour data if necessary
						int realy = y;
//						if (g_allargs.cutmode == Cutmode_EMXSPR)
//							realy = it->lineorder[y];

						for (int p = 0; p < g_allargs.planes; p++)
						{
							for (int x = 0; x < wordwidth; x++)
							{
								wv = endianswap16(it->colr_[(((realy*wordwidth) + x)*g_allargs.planes) + p]);
								fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
							}
						}
					}
				}

				// optimized colour
				if (g_allargs.cutmode == Cutmode_EMXSPR)
				{
					// track colour preshifts
					// todo: assumes only one for now
					pscolropt_index.push_back(tracked_filesize);

					// write word-width for this preshift (will be 1, 2 or 3 in this mode for STE/F030 blitter endmasks)
//					wv = endianswap16(wordwidth);
//					fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

					// write colour words for N lines, lines(planes(words))) order
					for (int y = 0; y < it->h_; y++)
					{
						// reorder colour data if necessary
						int realy = y;
						//if (g_allargs.cutmode == Cutmode_EMXSPR)
							realy = it->lineorder[y];

						for (int p = 0; p < g_allargs.planes; p++)
						{
							for (int x = 0; x < wordwidth; x++)
							{
								wv = endianswap16(it->colr_[(((realy*wordwidth) + x)*g_allargs.planes) + p]);
								fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
							}
						}
					}
				}

				//for (int o = 0; o < planewords; o++)
				//{
				//	// write colour words for N planes, interleaved
				//	for (int p = 0; p < g_allargs.planes; p++)
				//	{
				//		wv = endianswap16(it->colr_[(o*g_allargs.planes) + p]);
				//		fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
				//	}
				//}

				// track mask preshifts
				for (preshiftlist_t::iterator itm = it->psmask_frames_.begin(); itm != it->psmask_frames_.end(); itm++)
				{
					psmask_index.push_back(tracked_filesize);

					// write word-width for this preshift (will be 1, 2 or 3 in this mode for STE/F030 blitter endmasks)
					wv = endianswap16(itm->wordwidth_);
					fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

					//if (g_allargs.cutmode == Cutmode_EMXSPR)
					//{
					//	fwrite(fname.c_str(), 1, 32, hout); tracked_filesize += 32;

					//	// write 68k binary code for this preshift
					//	fwrite((itm->p68kbin + 32), 1, (itm->size68kbin - 32), hout);
					//	tracked_filesize += (itm->size68kbin - 32);
					//}
					//else
					{
						// write whole mask preshift
						for (int o = 0; o < itm->planewords_; o++)
						{
							// write colour words for single mask plane
							wv = endianswap16(itm->data_[o]);
							fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
						}
					}
				}

				// track mask preshifts
				if (g_allargs.cutmode == Cutmode_EMXSPR)
				{
					for (preshiftlist_t::iterator itm = it->psmask_frames_.begin(); itm != it->psmask_frames_.end(); itm++)
					{
						psmaskopt_index.push_back(tracked_filesize);

						// write word-width for this preshift (will be 1, 2 or 3 in this mode for STE/F030 blitter endmasks)
						//					wv = endianswap16(itm->wordwidth_);
						//					fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

						//					if (g_allargs.cutmode == Cutmode_EMXSPR)
						{
							//fwrite(fname.c_str(), 1, 32, hout); tracked_filesize += 32;

							// write 68k binary code for this preshift
							fwrite((itm->p68kbin + 32), 1, (itm->size68kbin - 32), hout);
							tracked_filesize += (itm->size68kbin - 32);
						}
						//else
						//{
						//	// write whole mask preshift
						//	for (int o = 0; o < itm->planewords_; o++)
						//	{
						//		// write colour words for single mask plane
						//		wv = endianswap16(itm->data_[o]);
						//		fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
						//	}
						//}
					}
				}

				// patch colour preshift index
				{
					lv = 0;
					size_t save = ftell(hout);
					fseek(hout, colr_preshift_index_fpos + 4, 0);
					for (std::vector<int>::iterator it = pscolr_index.begin(); it != pscolr_index.end(); it++)
					{
						lv = (*it) - colr_preshift_index_fpos;
						lv = endianswap32(lv);
						fwrite(&lv, 1, 4, hout);
					}
					fseek(hout, save, 0);
				}

				// patch colouropt preshift index
				if (g_allargs.cutmode == Cutmode_EMXSPR)
				{
					lv = 0;
					size_t save = ftell(hout);
					fseek(hout, colropt_preshift_index_fpos + 4, 0);
					for (std::vector<int>::iterator it = pscolropt_index.begin(); it != pscolropt_index.end(); it++)
					{
						lv = (*it) - colropt_preshift_index_fpos;
						lv = endianswap32(lv);
						fwrite(&lv, 1, 4, hout);
					}
					fseek(hout, save, 0);
				}

				// patch mask preshift index
				{
					lv = 0;
					size_t save = ftell(hout);
					fseek(hout, mask_preshift_index_fpos + 4, 0);
					for (std::vector<int>::iterator it = psmask_index.begin(); it != psmask_index.end(); it++)
					{
						lv = (*it) - mask_preshift_index_fpos;
						lv = endianswap32(lv);
						fwrite(&lv, 1, 4, hout);
					}
					fseek(hout, save, 0);
				}

				// patch maskopt preshift index
				if (g_allargs.cutmode == Cutmode_EMXSPR)
				{
					lv = 0;
					size_t save = ftell(hout);
					fseek(hout, maskopt_preshift_index_fpos + 4, 0);
					for (std::vector<int>::iterator it = psmaskopt_index.begin(); it != psmaskopt_index.end(); it++)
					{
						lv = (*it) - maskopt_preshift_index_fpos;
						lv = endianswap32(lv);
						fwrite(&lv, 1, 4, hout);
					}
					fseek(hout, save, 0);
				}

			}
			else
			{
				// immediate data follows - no preshifting present

				// write mask,colr[n] words

				// in planar-mask mode, we emit the entire mask before the colour data planes
				if (g_allargs.genmask && (g_allargs.masklayout == Masklayout_PLANAR))
				{
					for (int o = 0; o < planewords; o++)
					{
						// write mask plane data
						wv = endianswap16(it->mask_[o]);
						fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
					}
				}

				for (int o = 0; o < planewords; o++)
				{
					if (g_allargs.genmask && (g_allargs.masklayout == Masklayout_INTERLEAVED))
					{
						// write interleaved mask word BEFORE each group of colour plane words
						wv = endianswap16(it->mask_[o]);
						fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
					}

					// write colour words for N planes
					for (int p = 0; p < g_allargs.planes; p++)
					{
						wv = endianswap16(it->colr_[(o*g_allargs.planes) + p]);
						fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
					}
				}
			}
		}

		// patch sprite index
		lv = 0;
		fseek(hout, sprite_index_fpos+4, 0);
		for (std::vector<int>::iterator it = sprite_index.begin(); it != sprite_index.end(); it++)
		{
			lv = (*it) - sprite_index_fpos;
			lv = endianswap32(lv);
			fwrite(&lv, 1, 4, hout);
		}

		// patch filesize
		fseek(hout, 0, 0);
		lv = endianswap32(tracked_filesize);
		fwrite(&lv, 1, 4, hout);
	}

	std::cout << "emitted sprite sequence [" << fname.c_str() << "]" << std::endl;

	return true;
}

// --------------------------------------------------------------------
//	slab mode
// --------------------------------------------------------------------

class slabspan_t
{
public:

	word_t xo_;
	word_t xs_;
	word_t dxw_;
	word_t y_;
	word_t dy_;
	word_t ys_;
	uword_t *pspan_;

	slabspan_t(int _xo, int _xs, int _dxw, int _y, int _dy, int _ys, uword_t *_pspan)
		: xo_(_xo),
		  xs_(_xs),
		  dxw_(_dxw),
		  y_(_y),
		  dy_(_dy),
		  ys_(_ys),
		  pspan_(_pspan)
	{
	}
};

class slabframe_t
{
public:

	word_t w_, h_;
	word_t xo_, yo_;
	uword_t *colr_;

	slabframe_t(int _w, int _h, int _xo, int _yo, uword_t *_colr)
		: w_(_w),
		  h_(_h),
		  xo_(_xo),
		  yo_(_yo),
		  colr_(_colr)
	{
	}

	~slabframe_t()
	{
		// do not delete here - no refcounting on allocations happening within STL container
	}

	void release()
	{
		delete[] colr_; colr_ = 0;
	}

	std::list<slabspan_t> spans_;
};

typedef std::list<slabframe_t> slablist_t;

// todo: tidy this up!

slablist_t g_slabframes;

// --------------------------------------------------------------------
//	slab mode
// --------------------------------------------------------------------

void init_slab_sequence(int _xs, int _ys, int _sw, int _sh)
{
	for (slablist_t::iterator it = g_slabframes.begin(); it != g_slabframes.end(); it++)
		it->release();

	g_slabframes.clear();

	g_spritesource_w = _sw;
	g_spritesource_h = _sh;

	// worst-case width in bitplane words
	g_sprite_bpwidth = (_xs + (16-1)) & -16;
	g_sprite_width = _xs;
	g_sprite_height = _ys;


	// allocate sprite buffer for chunky processing (default to keycol)
	delete[] g_colrbuffer8;
	g_colrbuffer8 = new ubyte_t[g_sprite_bpwidth * g_sprite_height];
	memset(g_colrbuffer8, g_allargs.keycol, g_sprite_bpwidth * g_sprite_height);

	// allocate sprite buffer for bitplane processing
	delete[] g_colrbufferbpl;
	g_colrbufferbpl = new uword_t[(g_sprite_bpwidth * g_sprite_height) >> 1];
	memset(g_colrbufferbpl, 0, g_sprite_bpwidth * g_sprite_height);

	// allocate sprite buffer for chunky processing (default to keycol)
	delete[] g_workbuffer8;
	g_workbuffer8 = new ubyte_t[g_sprite_bpwidth * g_sprite_height];
	memset(g_workbuffer8, g_allargs.keycol, g_sprite_bpwidth * g_sprite_height);

	// allocate sprite buffer for bitplane processing
	delete[] g_workbufferbpl;
	g_workbufferbpl = new uword_t[(g_sprite_bpwidth * g_sprite_height) >> 1];
	memset(g_workbufferbpl, 0, g_sprite_bpwidth * g_sprite_height);
}

void extract_spanblock(slabframe_t &slab, int _spanblock_firstx, int _spanblock_lastx, int _spanblock_firsty, int _spanblock_lasty, int _spanblock_gapy, int cxmax)
{
	int spanblock_lines = _spanblock_lasty - _spanblock_firsty;

	assert(spanblock_lines > 0);

	// left-justify span to remove leading keycolour pixels
	for (int ey = _spanblock_firsty; ey < _spanblock_lasty; ey++)
	{
		int dstx = 0;
		for (int px = _spanblock_firstx; px <= _spanblock_lastx; px++, dstx++)
		{
			// extract pixel
			ubyte_t c = g_colrbuffer8[(ey * g_sprite_framebpwidth) + px];
			// erase pixel with keycol
			g_colrbuffer8[(ey * g_sprite_framebpwidth) + px] = g_allargs.keycol;
			// transfer to workbuffer, left-justified
			g_workbuffer8[(ey * g_sprite_framebpwidth) + dstx] = c;
		}

		// fill remainder of workbuffer spanblock with 0
		for (; dstx < cxmax; dstx++)
		{
			g_workbuffer8[(ey * g_sprite_framebpwidth) + dstx] = 0;
		}
	}

	// chunky -> planar
	c2pw
	(
		/*src=*/&g_workbuffer8[_spanblock_firsty * g_sprite_framebpwidth], /*dst*/&g_workbufferbpl[(_spanblock_firsty * g_sprite_framebpwidth) >> 1],
		/*w=*/g_sprite_framebpwidth, /*h=*/spanblock_lines,
		/*bytes_per_src_line=*/g_sprite_framebpwidth, /*words_per_dst_line=*/g_sprite_framebpwidth >> 1,
		/*planes=*/8
	);


	int spanlength = (_spanblock_lastx - _spanblock_firstx) + 1;
	int spanlength16 = (spanlength + (16 - 1)) & -16;
	int spanlengthwords = spanlength16 >> 4;

	// space for spanblock lines @ 8bpl
	uword_t *spanwords = new uword_t[(spanlengthwords * 8) * spanblock_lines];

	// copy 8-plane span lines into slab span container
	for (int ey = _spanblock_firsty, dsty = 0; ey < _spanblock_lasty; ey++, dsty++)
	{
		memcpy(&spanwords[(spanlengthwords * 8) * dsty], &g_workbufferbpl[(ey * g_sprite_framebpwidth) >> 1], spanlength16);
	}

	// store slab span
	slab.spans_.push_back
	(
		slabspan_t
		(
			_spanblock_firstx, spanlength, spanlengthwords, 
			_spanblock_firsty, _spanblock_gapy, spanblock_lines, 
			spanwords
		)
	);
}


void add_slab_frame(ubyte_t* _psource8, int _x, int _y)
{
	memset(g_colrbuffer8, g_allargs.keycol, g_sprite_bpwidth * g_sprite_height);

	int cxmin = 0;
	int cymin = 0;
	int cxmax = g_sprite_width;
	int cymax = g_sprite_height;

	if (g_allargs.preview)
	{
		printf("\n");
		for (int py = 0; py < g_sprite_height; py++)
		{
			printf("\t");
			for (int px = 0; px < g_sprite_width; px++)
			{
				ubyte_t c = _psource8[((py + _y) * g_spritesource_w) + (px + _x)];

				if (c == g_allargs.keycol)
					printf("   ");
				else
					printf("%02x ", (int)c);
			}
			printf("\n");
		}
		printf("\n");
	}

	//if (g_allargs.spriteoptimize)
	// always optimize the slab region 
	{
		cxmin = g_sprite_width;
		cymin = g_sprite_height;
		cxmax = 0;
		cymax = 0;

		// optimize cutting area

		for (int py = 0; py < g_sprite_height; py++)
		{
			for (int px = 0; px < g_sprite_width; px++)
			{
				ubyte_t c = _psource8[((py + _y) * g_spritesource_w) + (px + _x)];

				if (c != g_allargs.keycol)
				{
					// opaque pixel - adjust bounds
					cxmin = xmin(cxmin, px);
					cxmax = xmax(cxmax, px + 1);
					cymin = xmin(cymin, py);
					cymax = xmax(cymax, py + 1);
				}
			}
		}
	}

	// capture at least one pixel per frame
	if ((cxmax < cxmin) || (cymax < cymin))
	{
		cxmin = 0;
		cxmax = 1;
		cymin = 0;
		cymax = 1;
	}

	g_sprite_framewidth = cxmax - cxmin;
	g_sprite_frameheight = cymax - cymin;

	// this gets calculated again for each individual span
	g_sprite_framebpwidth = (g_sprite_framewidth + (16-1)) & -16;

	// cut chunky sprite to chunky colrbuffer & maskbuffer

	for (int psy = cymin, py = 0; psy < cymax; psy++, py++)
	{
		for (int psx = cxmin, px = 0; psx < cxmax; psx++, px++)
		{
			ubyte_t c = _psource8[((psy + _y) * g_spritesource_w) + (psx + _x)];

			// colour buffer receives actual colour, or 0 for keycol (so two colours can potentially map to 0)
			g_colrbuffer8[(py * g_sprite_framebpwidth) + px] = c;// (c == g_allargs.keycol) ? 0x00 : c;
		}
	}

	slabframe_t slab(g_sprite_framewidth, g_sprite_frameheight, cxmin, cymin, NULL);

	int spanblock_firstx = -1;
	int spanblock_lastx = -2;
	int spanblock_firsty = 0;
	int spanblock_lasty = 0;
	int prevlasty = 0;

	// process individual spans
	for (int psy = cymin, py = 0; psy < cymax; psy++, py++)
	{
		// find begin/end of solid part of span
		int firstx = 0;
		for (int psx = cxmin; psx < cxmax; psx++, firstx++)
		{
			ubyte_t c = g_colrbuffer8[(py * g_sprite_framebpwidth) + firstx];
			if (c != g_allargs.keycol) break;
		}

		int lastx = cxmax - 1;
		for (int psx = cxmin; psx < cxmax; psx++, lastx--)
		{
			ubyte_t c = g_colrbuffer8[(py * g_sprite_framebpwidth) + lastx];
			if (c != g_allargs.keycol) break;
		}

		// terminate spanblock if ends change
		if ((spanblock_firstx != firstx) || (spanblock_lastx != lastx))
		{
			if (spanblock_lastx >= spanblock_firstx)
			{
				// completion of non-empty spanblock

				// extract spanblock, left-justified to workbuffer and erase original (for multiple extractions)
				extract_spanblock(slab, spanblock_firstx, spanblock_lastx, spanblock_firsty, spanblock_lasty, (spanblock_firsty-prevlasty), cxmax);
				prevlasty = spanblock_lasty;
			}
			else
			{
				// completion of empty block
			}

			// reset run
			spanblock_firstx = firstx;
			spanblock_lastx = lastx;
			spanblock_firsty = py;
		}
		else
		{
			// continue run
		}

		spanblock_lasty++;
	}

	if (spanblock_lastx >= spanblock_firstx)
	{
		// final spanblock (if any)

		// extract spanblock, left-justified to workbuffer and erase original (for multiple extractions)
		extract_spanblock(slab, spanblock_firstx, spanblock_lastx, spanblock_firsty, spanblock_lasty, (spanblock_firsty-prevlasty), cxmax);
	}

	std::cout << "created slab frame " << g_slabframes.size() << " from offset [" << cxmin << ", " << cymin << "] size [" << g_sprite_framewidth << " x " << g_sprite_frameheight << "]" << std::endl;
	g_slabframes.push_back(slab);
}

void add_slabrestore_frame(ubyte_t* _psource8, int _x, int _y)
{
	// preprocess slab to produce fewer, less precise spanblock regions for accelerated clearing

	// 1) quantise spans to 16 pixel (planeword) boundaries
	{
		for (int py = 0; py < g_sprite_height; py++)
		{
			// find first solid pixel
			int px = 0;
			for (; px < g_sprite_width; px++)
			{
				ubyte_t c = _psource8[((py + _y) * g_spritesource_w) + (px + _x)];

				if (c != g_allargs.keycol)
					break;
			}

			// round towards left
			int left = px & -16;

			// find last solid pixel
			for (px = g_sprite_width-1; px >= 0; px--)
			{
				ubyte_t c = _psource8[((py + _y) * g_spritesource_w) + (px + _x)];

				if (c != g_allargs.keycol)
					break;
			}

			// last pixel exclusive
			px++;

			// round towards right
			int right = (px + 15) & -16;

			// clamp
			if (right > g_spritesource_w)
				right = g_spritesource_w;

			// choose random colours for debugging the regions
//			int cc = py & 15;
//			if (cc == g_allargs.keycol)
//				cc = 15 - cc;

			ubyte_t cc = g_allargs.keycol + 1;

			// replace colour for quantised region
			px = left;
			for (; px < right; px++)
			{
				// final colour doesn't matter, so long as its not keycol
				_psource8[((py + _y) * g_spritesource_w) + (px + _x)] = cc;
			}
		}
	}

	// 2) reduce fragmentation of quantised regions
	{
		ubyte_t k = g_allargs.keycol;
		ubyte_t s = k + 1;
		ubyte_t b = s + 1;
		// process columns, not rows
		for (int px = 0; px < g_sprite_width; px += 16)
		{
			// single column
			for (int py = 0; py < g_sprite_height; py++)
			{
				// for each word, look at the above/below flanking word pattern and blur

				// this whole sequence is crap, but it'll do

				int pym2 = xmax(py - 2, 0);
				int pym1 = xmax(py - 1, 0);
				int pyp1 = xmin(py + 1, g_sprite_height-1);
				int pyp2 = xmin(py + 2, g_sprite_height-1);

				ubyte_t cm2 = _psource8[((pym2 + _y) * g_spritesource_w) + (px + _x)];
				ubyte_t cm1 = _psource8[((pym1 + _y) * g_spritesource_w) + (px + _x)];
				ubyte_t c00 = _psource8[((py   + _y) * g_spritesource_w) + (px + _x)];
				ubyte_t cp1 = _psource8[((pyp1 + _y) * g_spritesource_w) + (px + _x)];
				ubyte_t cp2 = _psource8[((pyp2 + _y) * g_spritesource_w) + (px + _x)];

				if (c00 == k)
				{
					if ((cm1 == s && cp2 == s) ||
						(cm2 == s && cp1 == s) ||
						(cm1 == s && cp1 == s))
					{
						for (int x = px; x < px + 16; x++)
						{
							_psource8[((py + _y) * g_spritesource_w) + (x + _x)] = b;
						}
					}
				}
			}
		}
	}

	std::vector<std::pair<int, int> > fragments;
	fragments.resize(g_sprite_height);
	std::vector<int> blocksize;
	blocksize.resize(g_sprite_height);


	// 4) merge single-span blocks with neighbour blocks
	//    most of the overhead is coming from these alone
	bool changed = true;
//	for (int p = 0; p < 3; p++)
	while (changed)
	{
		// 4a) recolour/encode by spanblock fragmentation
		{
			ubyte_t k = g_allargs.keycol;
			ubyte_t cc = k + 1;

			int lastleft = -1;
			int lastright = -1;
			int spancount = 1;
			int last_block = 0;

			for (int py = 0; py < g_sprite_height; py++)
			{
				// find first solid pixel
				int px = 0;
				for (; px < g_sprite_width; px++)
				{
					ubyte_t c = _psource8[((py + _y) * g_spritesource_w) + (px + _x)];

					if (c != g_allargs.keycol)
						break;
				}

				// round towards left
				int left = px & -16;

				// find last solid pixel
				for (px = g_sprite_width-1; px >= 0; px--)
				{
					ubyte_t c = _psource8[((py + _y) * g_spritesource_w) + (px + _x)];

					if (c != g_allargs.keycol)
						break;
				}

				// last pixel exclusive
				px++;

				// round towards right
				int right = (px + 15) & -16;

				// clamp
				if (right > g_spritesource_w)
					right = g_spritesource_w;

				// record fragment left/right coords
				fragments[py].first = left;
				fragments[py].second = right;

				if ((left != lastleft) ||
					(right != lastright))
				{
					// change colour
					cc++;

					// but dodge key colour
					if (cc == k)
						cc++;

					lastleft = left;
					lastright = right;

					for (int s = last_block; s < py; s++)
					{
						blocksize[s] = spancount;
						printf("spancount[%d]: %d\n", s, spancount);
					}


					spancount = 1;
					last_block = py;
				}
				else
				{
					spancount++;
				}

				// replace colour for quantised region
				px = left;
				for (; px < right; px++)
				{
					// final colour doesn't matter, so long as its not keycol
					_psource8[((py + _y) * g_spritesource_w) + (px + _x)] = cc;
				}
			}
		}

		ubyte_t k = g_allargs.keycol;
		ubyte_t cc = k + 1;

		changed = false;

		for (int py = 0; py < g_sprite_height; py++)
		{


//			continue;

			// 4b) merge spans with neighbours 
			
			int pym2 = xmax(py - 2, 0);
			int pym1 = xmax(py - 1, 0);
			int pyp1 = xmin(py + 1, g_sprite_height - 1);
			int pyp2 = xmin(py + 2, g_sprite_height - 1);

			if (
				(blocksize[py] < 3)
/*
				(
					//difference with m1
					((pym1 == py) ||
					 (fragments[pym1].first  != fragments[py].first) ||
					 (fragments[pym1].second != fragments[py].second))
					&&
					//difference with p1
					((pyp1 == py) ||
					 (fragments[pyp1].first  != fragments[py].first) ||
					 (fragments[pyp1].second != fragments[py].second))
				)
				||
				(
					//difference with m2
					((pym2 == py) ||
					 (fragments[pym2].first  != fragments[py].first) ||
					 (fragments[pym2].second != fragments[py].second))
					&&
					//difference with p1
					((pyp1 == py) ||
					 (fragments[pyp1].first  != fragments[py].first) ||
					 (fragments[pyp1].second != fragments[py].second))
				)
				||
				(
					//difference with m1
					((pym1 == py) ||
					 (fragments[pym1].first  != fragments[py].first) ||
					 (fragments[pym1].second != fragments[py].second))
					&&
					//difference with p2
					((pyp2 == py) ||
					 (fragments[pyp2].first  != fragments[py].first) ||
					 (fragments[pyp2].second != fragments[py].second))
				)
*/
			)
			{
				// this is a single-span or 2-span block

				// check possibility of merging with upper neighbour
				bool mu = 
					(py != pym1) &&
					(fragments[py].first  >= fragments[pym1].first) &&
					(fragments[py].second <= fragments[pym1].second);

				// check possibility of merging with lower neighbour
				bool ml = 
					(py != pyp1) &&
					(fragments[py].first  >= fragments[pyp1].first) &&
					(fragments[py].second <= fragments[pyp1].second);

				// find merge error with each neighbour
				int erru = mu ? ((fragments[py].first - fragments[pym1].first) + (fragments[pym1].second - fragments[py].second)) : 0x7fffffff;
				int errl = ml ? ((fragments[py].first - fragments[pyp1].first) + (fragments[pyp1].second - fragments[py].second)) : 0x7fffffff;

				// ignore pre-merged cases
				if (erru == 0) erru = 0x7fffffff;
				if (errl == 0) errl = 0x7fffffff;

				// find merge error with lower neighbour
				if (erru < errl)
				{
					if (erru < 0x7fffffff)
					{
						ubyte_t col = _psource8[((py + _y) * g_spritesource_w) + (fragments[py].first + _x)];

						int px = fragments[pym1].first;
						for (; px < fragments[pym1].second; px++)
						{
							_psource8[((py + _y) * g_spritesource_w) + (px + _x)] = col;
						}
						changed = true;
						break;
					}
				}
				else
				{
					if (errl < 0x7fffffff)
					{
						ubyte_t col = _psource8[((py + _y) * g_spritesource_w) + (fragments[py].first + _x)];

						int px = fragments[pyp1].first;
						for (; px < fragments[pyp1].second; px++)
						{
							_psource8[((py + _y) * g_spritesource_w) + (px + _x)] = col;
						}
						changed = true;
						break;
					}
				}
			}

		} // py loop
	}

	// now process the quantised frame normally
	add_slab_frame(_psource8, _x, _y);
}

bool emit_slab_sequence()
{
	int s = 0;
	for (slablist_t::iterator fit = g_slabframes.begin(); fit != g_slabframes.end(); fit++, s++)
	{
		std::cout << "slab " << s << std::endl;
		for (std::list<slabspan_t>::iterator sit = fit->spans_.begin(); sit != fit->spans_.end(); sit++)
		{
			std::cout << "spanblock[" << sit->ys_ << "]: " << sit->xo_ << ":" << sit->xs_ << std::endl;
		}
	}

	std::string outfile;
	if (g_allargs.outfile.length() > 0)
	{
		outfile = g_allargs.outfile;
	}
	else
	{
		outfile = g_allargs.sourcefile;
	}

	if (g_allargs.cutmode == Cutmode_SLABS)
		replace_extension(outfile, "sls");
	else
	if (g_allargs.cutmode == Cutmode_SLABRESTORE)
		replace_extension(outfile, "slr");

	FILE *hout = fopen(outfile.c_str(), "wb");
	if (hout)
	{
		word_t wb;
		word_t wv;
		long_t lv;
		int tracked_filesize = 0;

		std::vector<int> slab_index;

		// 4: size of file remaining (patched before closing)
		lv = 0;
		fwrite(&lv, 1, sizeof(lv), hout);

		// 2: version info
		wv = endianswap16(SLS_VERSION);
		fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

		// 2: number of slab frames in sequence
		wv = endianswap16((uword_t)g_slabframes.size());
		fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

		// 2: number of bitplanes in slab data
		wv = endianswap16(g_allargs.planes);
		fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

		// 2: flags
		wv = 0;
		wv = endianswap16((uword_t)wv);
		fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

		// 1: preshift range
		wb = 0;
		fwrite(&wb, 1, 1, hout); tracked_filesize += 1;
		// 1: preshift step
		wb = 0;
		fwrite(&wb, 1, 1, hout); tracked_filesize += 1;

		// 2: source width
		wv = endianswap16((uword_t)g_sprite_width);
		fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
		// 2: source height
		wv = endianswap16((uword_t)g_sprite_height);
		fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

		// create slab index (patched before closing)
		int slab_index_fpos = tracked_filesize;
		lv = 0xBBBBBBBB;
		for (slablist_t::iterator it = g_slabframes.begin(); it != g_slabframes.end(); it++)
		{
			fwrite(&lv, 1, sizeof(lv), hout); tracked_filesize += sizeof(lv);
		}

		// individual slabs follow
		for (slablist_t::iterator it = g_slabframes.begin(); it != g_slabframes.end(); it++)
		{
			std::vector<int> spanblock_index;
			std::vector<int> spanblock_clipper;
			std::vector<int> clipspandata_ptrs;
			std::vector<int> spandata_ptrs;
			std::vector<int> spandata_index;

			// record file-offset to slab in slab index, for patching later
			slab_index.push_back(tracked_filesize);

			// 2: slab frame pixel width
			wv = endianswap16(it->w_);
			fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
			// 2: slab frame pixel height
			wv = endianswap16(it->h_);
			fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

			// 2: slab frame xoff
			wv = endianswap16(it->xo_);
			fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
			// 2: slab frame yoff
			wv = endianswap16(it->yo_);
			fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

			// 2: slab spanblocks
			wv = endianswap16(s16(it->spans_.size()));
			fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

			//// pointer to spanblock index (patched after writing)
			//int spanblock_indexptr_fpos = tracked_filesize;
			//lv = 0xEEEEEEEE;
			//fwrite(&lv, 1, sizeof(lv), hout); tracked_filesize += sizeof(lv);

			int indextables_fpos = tracked_filesize;

			// create clipper index (patched after writing)
			int spanblock_clipper_fpos = tracked_filesize;
			wv = 0xAAAA;
//			lv = 0xDDDDDDDD;
			for (int i = 0; i < it->h_; i++)
			{
				// 4x 16bit fields per line of slab frame

				// offset to spanblock reference for this line
				fwrite(&wv, 1, sizeof(wv), hout); tracked_filesize += sizeof(wv);

				// remaining spanblocks for this top yc -1
				fwrite(&wv, 1, sizeof(wv), hout); tracked_filesize += sizeof(wv);

				// pointer to spandata for top yc
				clipspandata_ptrs.push_back(tracked_filesize);
//				fwrite(&lv, 1, sizeof(lv), hout); tracked_filesize += sizeof(lv);
				fwrite(&wv, 1, sizeof(wv), hout); tracked_filesize += sizeof(wv);

				// remaining spanblocks for this bottom yc -1
				fwrite(&wv, 1, sizeof(wv), hout); tracked_filesize += sizeof(wv);
			}

			// create spanblock index (patched after writing)
			int spanblock_index_fpos = tracked_filesize;
			wv = 0xBBBB;
			for (int i = 0; i < it->spans_.size(); i++)
			{
				// one 16bit offset per line of frame
				fwrite(&wv, 1, sizeof(wv), hout); tracked_filesize += sizeof(wv);

				//	cy		sbi		sb
				//	0		A		A
				//	1		A		-
				//	2		A		-
				//	3		B		B
				//	4		B		-
				//	5		C		C
				//	6		D		D
				//	7		D		-
			}

			// write out spanblock records
//			int mapped_line = 0;
			int last_spanblock = -1;
			int last_spanblock_y = 0;
			for (std::list<slabspan_t>::iterator spanit = it->spans_.begin(); spanit != it->spans_.end(); spanit++)
			{
				spanblock_index.push_back(tracked_filesize);

				// fill clipper for all lines up to start of new spanblock (if any)
				// note: take care over initial y skip, which would otherwise fill -1 for last spanblock
				int last_valid_spanblock = (last_spanblock >= 0) ? last_spanblock : 0;
				for (; last_spanblock_y < (spanit->y_ - spanit->dy_); last_spanblock_y++)
				{
					spanblock_clipper.push_back(last_valid_spanblock);
				}

				last_spanblock++;

				// 2: spanblock dy (skipped lines)
				wv = endianswap16(spanit->dy_);
				fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
				// 2: spanblock xo
				wv = endianswap16(spanit->xo_);
				fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
				// 2: spanblock xs (<<4 to preshift for blitter lookups)
				wv = endianswap16(u16(spanit->xs_)<<4);
				fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
				// 2: spanblock dxw
				wv = endianswap16(spanit->dxw_);
				fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
				// 2: spanblock y (for skipped then nonskipped lines of spanblock)
				wv = endianswap16(spanit->y_);
				fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
				// 2: spanblock nonskipped lines (ys-1)
				wv = endianswap16(spanit->ys_-1);
				fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

				// 4: spandata ptr
				spandata_ptrs.push_back(tracked_filesize);
				lv = 0xAAAAAAAA;
				fwrite(&lv, 1, sizeof(lv), hout); tracked_filesize += sizeof(lv);
			}

			// now write pixel data separately, and contiguously
			for (std::list<slabspan_t>::iterator spanit = it->spans_.begin(); spanit != it->spans_.end(); spanit++)
			{
				spandata_index.push_back(tracked_filesize);

				// output spanblock lines
				for (int l = 0; l < spanit->ys_; l++)
				{

					//if (0) // interleaved planewords (CPU)
					//{
					//	// output span plane words for a line
					//	for (int o = 0; o < spanit->dxw_; o++)
					//	{
					//		// write colour words for N planes
					//		for (int p = 0; p < g_allargs.planes; p++)
					//		{
					//			wv = endianswap16(spanit->pspan_[(((l*spanit->dxw_) + o) * 8) + p]);
					//			fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
					//		}
					//	}
					//}
					//else // plane-at-a-time scans (blitter)
					{
						// write colour words for N planes
						for (int p = 0; p < g_allargs.planes; p++)
						{
							// output span plane words for a line
							for (int o = 0; o < spanit->dxw_; o++)
							{
								wv = endianswap16(spanit->pspan_[(((l*spanit->dxw_) + o) * 8) + p]);
								fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
							}
						}
					}
				}
			}

			// fill clipper for all lines up to end of last spanblock
			// note: take care over initial y skip, which would otherwise fill -1 for last spanblock
			int last_valid_spanblock = (last_spanblock >= 0) ? last_spanblock : 0;
			for (; last_spanblock_y < it->h_; last_spanblock_y++)
			{
				spanblock_clipper.push_back(last_valid_spanblock);
			}

//			assert(mapped_line == it->h_);

/*			while (mapped_line < it->h_)
			{
				spanblock_index.push_back(-1);
				mapped_line++;
			}
*/

			// save seek			
			int tmp_pos = ftell(hout);

			// patch spanblock record data pointers
			int yi = 0;
			for (std::vector<int>::iterator cspit = clipspandata_ptrs.begin(); cspit != clipspandata_ptrs.end(); cspit++, yi++)
			{
				int ptr_off = (*cspit);
				fseek(hout, ptr_off+4, 0);
				// find spanblock used by this y
				int pi = spanblock_clipper[yi];
				// antireloc spandata ptr

				wv = (word_t)(spandata_index[pi] - indextables_fpos);
//				printf("clipper[%d]:[%08x] %08x\n", yi, ptr_off, lv);
				wv = endianswap16(wv);
				fwrite(&wv, 1, sizeof(wv), hout);

//				lv = spandata_index[pi] - indextables_fpos;
////				printf("clipper[%d]:[%08x] %08x\n", yi, ptr_off, lv);
//				lv = endianswap32(lv);
//				fwrite(&lv, 1, sizeof(lv), hout);
			}

			// patch spanblock record data pointers
			int pi = 0;
			for (std::vector<int>::iterator srpit = spandata_ptrs.begin(); srpit != spandata_ptrs.end(); srpit++, pi++)
			{
				int ptr_off = (*srpit);
				fseek(hout, ptr_off+4, 0);
				// antireloc next spandata ptr
				lv = spandata_index[pi] - indextables_fpos;
//				printf("record[%d]: %08x\n", pi, lv);
				lv = endianswap32(lv);
				fwrite(&lv, 1, sizeof(lv), hout);
			}

			// patch clipper index
			fseek(hout, spanblock_clipper_fpos+4, 0);
			int yc = 0;
			for (std::vector<int>::iterator cit = spanblock_clipper.begin(); cit != spanblock_clipper.end(); cit++, yc++)
			{
				// spanblock clipper addresses spanblocks, but based on scanline indices
				// the clipper array contains spanblock indices counting from 0 and we translate 
				// these into offset from the spanblock index so the drawing routine starts on 
				// the correct spanblock index entry for any initial yclip

				int clipfirst = (*cit);

//				printf("clipper[%d]: %d\n", ci, clipfirst);

				wv = (spanblock_index_fpos + (clipfirst * sizeof(word_t))) - indextables_fpos;
				wv = endianswap16(wv);
				fwrite(&wv, 1, sizeof(wv), hout);

				// remaining spanblocks for top yc, assuming this was the first
				wv = spanblock_index.size() - clipfirst;
				wv = endianswap16(wv);
				fwrite(&wv, 1, sizeof(wv), hout);

				// skip offset, already patched
				fseek(hout, 2, SEEK_CUR);

				// sum spanblocks until (y2 >= yc)
				{
					// ybot recedes as yc advances
					int ybot = it->h_ - yc;

					int sbcount = 0;
					for (std::list<slabspan_t>::iterator blkit = it->spans_.begin(); blkit != it->spans_.end(); blkit++)
					{
						int spanblock_y2 = (blkit->y_ + blkit->dy_ + blkit->ys_);
						if (spanblock_y2 >= ybot) 
							break;
						sbcount++;
					}

					// remaining spanblocks for bottom yc -1
					wv = sbcount;
					wv = endianswap16(wv);
					fwrite(&wv, 1, sizeof(wv), hout);
				}
			}

			// patch spanblock index
			fseek(hout, spanblock_index_fpos+4, 0);
			for (std::vector<int>::iterator spanit = spanblock_index.begin(); spanit != spanblock_index.end(); spanit++)
			{
				wv = (*spanit) - indextables_fpos;// spanblock_index_fpos;
				wv = endianswap16(wv);
				fwrite(&wv, 1, sizeof(wv), hout);
			}

			// restore seek
			fseek(hout, tmp_pos, 0);
		}

		// patch slab index
		lv = 0;
		fseek(hout, slab_index_fpos+4, 0);
		for (std::vector<int>::iterator it = slab_index.begin(); it != slab_index.end(); it++)
		{
			lv = (*it) - slab_index_fpos;
			lv = endianswap32(lv);
			fwrite(&lv, 1, sizeof(lv), hout);
		}

		// patch filesize
		fseek(hout, 0, 0);
		lv = endianswap32(tracked_filesize);
		fwrite(&lv, 1, sizeof(lv), hout);
	}

	std::cout << "emitted slab sequence [" << outfile.c_str() << "]" << std::endl;

	return true;
}

// --------------------------------------------------------------------
//	sheet mode
// --------------------------------------------------------------------

bool do_sheet()
{
	ulong_t sourcew = 0;
	ulong_t sourceh = 0;
	ulong_t sourced = 0;
	RGB *m_psourcergb = 0;
	RGB *m_ppalettergb = 0;
	ubyte_t *m_psourcecol = 0;
	ubyte_t *m_poriginalsrccol = 0;
	RGB *pvisualrgb = 0;
	int visualw, visualh;

	if (g_allargs.cutmode == Cutmode_SPRITES)
		std::cout << "spritesheet cutting mode..." << std::endl;
	else
	if (g_allargs.cutmode == Cutmode_SLABS)
		std::cout << "slab-sheet cutting mode..." << std::endl;
	else
	if (g_allargs.cutmode == Cutmode_SLABRESTORE)
		std::cout << "slab-sheet region restore mode..." << std::endl;

	{
		if (image_load(g_allargs.sourcefile.c_str(), m_psourcergb, m_psourcecol, m_ppalettergb, sourcew, sourceh, sourced))
		{
			std::cout << "read [" << g_allargs.sourcefile.c_str() << "] with colour depth " << sourced << " and dimensions " << sourcew << " x " << sourceh << std::endl;

			bool doremap = true;

			if (m_psourcecol)
			{
				if (sourced > g_allargs.planes)
				{
					if (g_allargs.noremap)
					{
						std::cout << "warning: source image is indexed and > " << g_allargs.planes << " index bits - will use first 16 colours only (--no-remap)" << std::endl;
						doremap = false;
					}
					else
					{
						std::cout << "warning: source image is indexed and > " << g_allargs.planes << " index bits - will be remapped to external palette" << std::endl;
					}
				}
				else
				{
					if (g_allargs.forceremap)
					{
						std::cout << "warning: source image is indexed and <= " << g_allargs.planes << " index bits - but will remap anyway (--force-remap)" << std::endl;
					}
					else
					{
						std::cout << "warning: source image is indexed and <= " << g_allargs.planes << " index bits - will pass through without remapping" << std::endl;
						doremap = false;
					}
				}

				if (doremap)
				{
					m_poriginalsrccol = m_psourcecol;
					m_psourcecol = 0;
				}
				else
				{
					// check that indexed colours are now in range for bitplanes specified
					int maxcolidx = 1 << g_allargs.planes;
					int highest = 0;
					for (size_t p = 0; p < sourcew*sourceh; p++)
					{
						if ((m_psourcecol[p] >= maxcolidx) && (m_psourcecol[p] != g_allargs.keycol))
						{
							highest = xmax(highest, m_psourcecol[p]);
							m_psourcecol[p] = g_allargs.keycol;
//							break;
						}
					}

					if (highest)
					{
						std::cout << "warning: --no-remap was specified, but source colour indices exceed target bitplane count!" << std::endl;
						std::cout << "highest usable index = " << maxcolidx - 1 << ", highest index encountered = " << highest << std::endl;
						std::cout << "all illegal colours have been converted to --key-colour..." << std::endl;
					}
				}

/*
				int maxcolidx = 1 << g_allargs.planes;

				// check that indexed colours are in range for bitplanes specified
				for (size_t p = 0; p < sourcew*sourceh; p++)
				{
					if (m_psourcecol[p] >= maxcolidx)
					{
						std::cout << "warning: source image is indexed, but values exceed target bitplane count: remapping..." << std::endl;
						// ignore colourmap
						m_poriginalsrccol = m_psourcecol;
						m_psourcecol = 0;
						break;
					}
				}
*/
			}

			if (m_psourcecol)
			{
				if (g_allargs.keyrgb_specified)
				{
					std::cerr << "error: can't use -keyrgb <R:G:B> on non-remapped spritesheet images - use -key <col> instead." << std::endl;
					printf("press any key...\n");
					getchar();	
					exit(1);
				}

				std::cout << "using palette and colourmap provided by source image..." << std::endl;
			}
			else
			{
				std::cout << "reducing " << ((sourced > 0) ? sourced : 24) << "-bit image to external fixed palette..." << std::endl;
				m_psourcecol = new ubyte_t[sourcew * sourceh]; //(ubyte_t*)malloc(sourcew * sourceh);
				// reduce source image to indexed colour with fixed palette
				reduce_image24(m_psourcergb, m_poriginalsrccol, m_psourcecol, 0, sourcew, sourceh, sourced, m_ppalettergb);
			}

			// sprite cutting loop
			{
				// either cut a rectangular grid of frames based on ycount,xcount
				// or, if specified, stop cutting when a total count is reached instead
				int cuts;
				if (g_allargs.cut_count)
					cuts = g_allargs.cut_count;
				else
					cuts = g_allargs.cut_xcount * g_allargs.cut_ycount;

				// configure expected cut count for allocating visual feedback image
				int total_expected_cuts = cuts;
				// if using a spriteguide file, sum up frames from the cutting events
				if (!g_allargs.guideeventseq.empty())
				{
					total_expected_cuts = 0;
					for (std::list<guideevent>::iterator git = g_allargs.guideeventseq.begin();
						git != g_allargs.guideeventseq.end();
						git++)
					{
						total_expected_cuts += git->count;
					}
				}

				// configure default cut from command args

				int cutting_source_xs = g_allargs.cut_xsize;
				int cutting_source_ys = g_allargs.cut_ysize;

				int cutting_source_x = g_allargs.cut_xpos;
				int cutting_source_y = g_allargs.cut_ypos;

				int total_cuts = 0;

				do
				{
					// if cutting from a guide file, override command settings for cutting cursor
					// and repeat the cut while guide events remain unprocessed
					if (!g_allargs.guideeventseq.empty())
					{
						guideevent &ge = g_allargs.guideeventseq.front();

						cutting_source_x = ge.xp;
						cutting_source_y = ge.yp;
						g_allargs.cut_xpos = ge.xp;
						g_allargs.cut_ypos = ge.yp;

						cutting_source_xs = ge.xs;
						cutting_source_ys = ge.ys;
						g_allargs.cut_xsize = ge.xs;
						g_allargs.cut_ysize = ge.ys;

						g_allargs.cut_xinc = ge.xi;
						g_allargs.cut_yinc = ge.yi;

						// cutting N frames from static position
						if ((ge.xi == 0) && (ge.yi == 0))
						{
							g_allargs.cut_xcount = ge.count;
							g_allargs.cut_ycount = 1;
						}
						else
						// row cut
						if ((ge.xi != 0) && (ge.yi == 0))
						{
							g_allargs.cut_xcount = ge.count;
							g_allargs.cut_ycount = 1;
						}
						else
						// column cut
						if ((ge.xi == 0) && (ge.yi != 0))
						{
							g_allargs.cut_xcount = 1;
							g_allargs.cut_ycount = ge.count;
						}
						else
						// everything else is an error
						{
							std::cerr << "error: cutting sequences in spriteguide files must be row or column, not diagonal!" << std::endl;
							printf("press any key...\n");
							getchar();	
							exit(1);
						}
	
						// configure cut count based on new args
						cuts = g_allargs.cut_xcount * g_allargs.cut_ycount;

						// this event has been dealt with - remove it
						g_allargs.guideeventseq.pop_front();
					}

					// can only have one sprite (outer) frame size per file, so we initialise
					// the sequence on the first cut and ignore any subsequent size changes
					static bool bfirstcut = true;
					if (bfirstcut)
					{
						bfirstcut = false;

						std::cout << "sprite dimensions [" << cutting_source_xs << ", " << cutting_source_ys << "]" << std::endl;

						if (g_allargs.cutmode == Cutmode_SPRITES ||
							g_allargs.cutmode == Cutmode_EMSPR ||
							g_allargs.cutmode == Cutmode_EMXSPR)
						{
							init_sprite_sequence(cutting_source_xs, cutting_source_ys, sourcew, sourceh);

							// opportunity to create visual feedback for cutting jobs
							// note: sprites only
							if (g_allargs.visual && !pvisualrgb)
							{
								visualw = cutting_source_xs;
								visualh = cutting_source_ys * total_expected_cuts;
								pvisualrgb = new RGB[visualw * visualh];
								memset(pvisualrgb, 0, visualw * visualh * sizeof(RGB));
							}
						}
						else
							if (g_allargs.cutmode == Cutmode_SLABS || g_allargs.cutmode == Cutmode_SLABRESTORE)
							{
								init_slab_sequence(cutting_source_xs, cutting_source_ys, sourcew, sourceh);
							}
					}


					for (int event_cuts = 0, xc = 0; event_cuts < cuts; event_cuts++, total_cuts++)
					{
						int cx = cutting_source_x;
						int cy = cutting_source_y;
						bool xflip = false;
						bool yflip = false;
						int hs = 0;
						int vs = 0;

						spritemap_t::iterator found = g_allargs.spritemappings.find(total_cuts);
						if (found != g_allargs.spritemappings.end())
						{
							// this frame is mapped from another
							int from = found->second.from;

							// locate cut record for 'from' frame
							const spritemapping_t &other = g_allargs.spritemappings[from];

							if (from != total_cuts)
							{
								// move cutting position to that of 'from' frame, if they are different frames
								cx = other.x;
								cy = other.y;
							}

							// check for X/Y flip-flags
							if (found->second.flags.find("x") != std::string::npos)
								xflip = true;

							if (found->second.flags.find("y") != std::string::npos)
								yflip = true;

							// ...and scroll adjust L/R/U/D
							if (found->second.flags.find("l") != std::string::npos)
								hs = -1;
							else
								if (found->second.flags.find("r") != std::string::npos)
									hs = 1;

							if (found->second.flags.find("u") != std::string::npos)
								vs = -1;
							else
								if (found->second.flags.find("d") != std::string::npos)
									vs = 1;
						}

						if (g_allargs.cutmode == Cutmode_SPRITES ||
							g_allargs.cutmode == Cutmode_EMSPR ||
							g_allargs.cutmode == Cutmode_EMXSPR)
						{
							std::cout << "cutting sprite frame " << total_cuts << " from position [" << cutting_source_x << ", " << cutting_source_y << "]" << std::endl;
							add_sprite_frame(m_psourcecol, cx, cy, xflip, yflip, hs, vs, 
								m_ppalettergb, pvisualrgb);
						}
						else
							if (g_allargs.cutmode == Cutmode_SLABS)
							{
								std::cout << "cutting slab frame " << total_cuts << " from position [" << cutting_source_x << ", " << cutting_source_y << "]" << std::endl;
								memset(g_colrbuffer8, g_allargs.keycol, g_sprite_bpwidth * g_sprite_height);
								add_slab_frame(m_psourcecol, cx, cy);
							}
							else
								if (g_allargs.cutmode == Cutmode_SLABRESTORE)
								{
									std::cout << "cutting slabrestore frame " << total_cuts << " from position [" << cutting_source_x << ", " << cutting_source_y << "]" << std::endl;
									memset(g_colrbuffer8, g_allargs.keycol, g_sprite_bpwidth * g_sprite_height);
									add_slabrestore_frame(m_psourcecol, cx, cy);
								}

						// store mappings for each frame, so future frames can refer to these
						// mappings in order to re-cut earlier frames with optional flipping
						g_allargs.spritemappings[total_cuts].x = cx;
						g_allargs.spritemappings[total_cuts].y = cy;

						cutting_source_x += g_allargs.cut_xinc;

						xc++;

						if (xc >= g_allargs.cut_xcount)
						{
							// reset row when cut_xcount expires
							xc = 0;
							cutting_source_x = g_allargs.cut_xpos;
							cutting_source_y += g_allargs.cut_yinc;
						}
					}

					// continue while guide events remain (if there were any guide events)
				}
				while (!g_allargs.guideeventseq.empty());

				std::cout << "...done" << std::endl;
			}

			// create sprite datafile

			switch (g_allargs.cutmode)
			{
			//
			case Cutmode_EMXSPR:
				// code-generated preshifted blitter EM sprites
				generate_emx_sequence();
			case Cutmode_SPRITES:
			case Cutmode_EMSPR:
				// non-code-generated sprite formats
				emit_sprite_sequence();
				break;
			//
			case Cutmode_SLABS:
				emit_slab_sequence();
				break;
			case Cutmode_SLABRESTORE:
				emit_slab_sequence();
				break;
			}
		}

		if (g_allargs.visual && pvisualrgb)
		{
			std::string reviewfile(g_allargs.outfile);
			remove_extension(reviewfile);
			reviewfile.append("_vis.tga");

			emit_tga(reviewfile.c_str(), pvisualrgb, visualw, visualh);
			std::cout << "emitted spritesheet visualisation [" << reviewfile.c_str() << "]" << std::endl;
			delete[] pvisualrgb; pvisualrgb = 0;
		}

	}

	return true;
}

// --------------------------------------------------------------------
//	main program entrypoint
// --------------------------------------------------------------------

int main(int argc, char* argv[])
{

	std::cout << "[A]tari [G]ame [T]ools: playfield/sprite/slab cutter " << g_version_string << " / dml" << std::endl;

	GetArguments(argc, argv, g_allargs);

	// degas mode supports 8x8, 16x16, 32x32...
	// rb+ mode supports 16x16, 32x32...
	// direct mode supports 8x8, 16x16 only

	if (g_allargs.cutmode == Cutmode_TILES)
	{
		switch (g_allargs.outmode)
		{
		case Outmode_DEGAS:
			// degas mode supports all sizes
			break;
		case Outmode_RBPLUS:
			if (g_allargs.tilesize < 16)
			{
				std::cerr << "error: rbplus mode only supports tilesize 16x16 or larger" << std::endl;
				printf("press any key...\n");
				getchar();	
				exit(1);
			}
			break;
		case Outmode_DIRECT:
			if (g_allargs.tilesize > 16)
			{
				std::cerr << "error: direct mode only supports tilesizes 8x8 or 16x16." << std::endl;
				printf("press any key...\n");
				getchar();	
				exit(1);
			}
			break;
		}
	}

	if (g_allargs.cutmode == Cutmode_TILES)
		do_tilemap();
	else
	if (g_allargs.cutmode == Cutmode_SPRITES ||
		g_allargs.cutmode == Cutmode_EMSPR ||
		g_allargs.cutmode == Cutmode_EMXSPR ||
		g_allargs.cutmode == Cutmode_SLABS ||
		g_allargs.cutmode == Cutmode_SLABRESTORE)
	{
		do_sheet();
	}
	else
	{
		// error
		std::cerr << "error: unsupported cutmode!" << std::endl;
		printf("press any key...\n");
		getchar();	
		exit(1);
	}
//	xassert(false);
}

