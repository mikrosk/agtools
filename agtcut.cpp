// --------------------------------------------------------------------
//	platform stuff - todo: OSX/Linux porting!
// --------------------------------------------------------------------

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
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
#include "3rdparty\stb_image.h"

#define BMP_TYPICAL_ROW_ORDER (1)		// 0 = -ve height, +ve ordering (easier to parse back in)
										// 1 = +ve height, -ve ordering (more common)
#include "rnd.h"

static const int USE_LINEDUPE_OPTIMIZATION = 1;
static const int USE_DELTA_OPTIMIZATION = 1;
static const int USE_LINEORDER_OPTIMIZATION = 1;

static const int DARWIN_DEPTH = 16;

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
static const char* const g_version_string = "v0.26";

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

#include "3rdparty\SimpleOpt.h"
#include "3rdparty\SimpleGlob.h"

// --------------------------------------------------------------------
//	misc. small helper functions
// --------------------------------------------------------------------

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
	OPT_PALETTE,
	OPT_CUTMODE,
	OPT_CCFIELDS,
	OPT_CCREMAP,
	// sprites
//	OPT_SPRITEGUIDE, // may not bother with this - command switches cover it
	OPT_PREVIEW,
	OPT_GENMASK,
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
	{ OPT_MASKLAYOUT,	_T("-ml"),	 			SO_REQ_SEP	},
	{ OPT_MASKLAYOUT,	_T("--mask-layout"),	SO_REQ_SEP	},
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
//	{ OPT_SPRITEGUIDE,	_T("-sg"),	 			SO_REQ_SEP	},
//	{ OPT_SPRITEGUIDE,	_T("--sprguide"), 		SO_REQ_SEP	},
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
	{ OPT_PALETTE,		_T("-p"),	 			SO_REQ_SEP	},
	{ OPT_PALETTE,		_T("--pal"),	 		SO_REQ_SEP	},
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
	std::cout << " -cm, --cutmode       > cutting mode: tiles,sprites,emspr,emxspr,slabs" << std::endl;
	std::cout << " -ccf, --ccfields     > cryptochrome fields: 1 or 2 (for dual-field gfx)" << std::endl;
	std::cout << " -p, --pal            > fixed palette to use: pi1,pal(riff),bmp,png" << std::endl;
	std::cout << " -bp, --bitplanes     > #bitplanes to generate: 1-8" << std::endl;
	std::cout << " -key, --key-colour   > colour index assumed to be transarent: 0-255" << std::endl;
	std::cout << " -keyrgb, --key-rgb   > rgb value assumed to be transarent: <RR:GG:BB> in hex" << std::endl;
	std::cout << " -kg, --key-guard     > guard against reducing colours to key colour" << std::endl;
	std::cout << " -kt, --key-tol       > key guard tolerance: 0.0+ (default: 0.01)" << std::endl;
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
	std::cout << " -ml, --mask-layout   > mask layout strategy: interleaved,planar,preshift" << std::endl;
	std::cout << " -sopt, --soptimize   > optimize away unused pixels: on/off (compensated w/offsets)" << std::endl;
	std::cout << " -ccr                 > cryptochrome colour field remapping: both,high,low" << std::endl;
//	std::cout << " -sl, --slab          > generate 'slab' sprites: on/off" << std::endl;
	std::cout << " -prv, --preview      > show ASCII preview of extracted colours + masking" << std::endl;
//	std::cout << " -sg, --sprguide      > sprite guide file (ASCII)" << std::endl;	// todo: maybe - only if its useful
	std::cout << std::endl;
	std::cout << "map & tile cutting mode:" << std::endl;
	std::cout << " -ts, --tilesize      > tile size (rbplus mode only): 16,32" << std::endl;
	std::cout << " -t, --tol            > tile matching tolerance: 0.0+" << std::endl;
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
	Cutmode_SPRITES,			// arbitrary width sprites
	Cutmode_EMSPR,				// <= 32 wide EM sprites (blitter)
	Cutmode_EMXSPR,				// <= 32 wide EM eXtreme sprites (blitter+codegen)
	Cutmode_SLABS
};

enum Masklayout
{
	Masklayout_INTERLEAVED,
	Masklayout_PLANAR,
	Masklayout_PRESHIFT,
	Masklayout_IMPLIED,
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
		  genmask(false),
		  keyguard(false),
		  tolerance(0.0f),
		  keytol(0.01f),
		  sourcefile(""),
		  outfile(""),
		  palettefile("palette.pi1"),
//		  spriteguidefile("sprites.sgd"),
		  ccfields(1),
		  ccremap(CCRemap_BOTH),
		  masklayout(Masklayout_PRESHIFT),
		  cutmode(Cutmode_TILES),
		  outmode(Outmode_DEGAS),
		  masklayout_lock(false),
		  planes(4),
		  tilesize(16),
		  keycol(0),
		  cut_xpos(0),
		  cut_ypos(0),
		  cut_xsize(16),
		  cut_ysize(16),
		  cut_xinc(0),
		  cut_yinc(0),
		  cut_xcount(0),
		  cut_ycount(0),
		  cut_count(0),
		  //
		  use_solutionpair_reduction_(false)
	{
		keyrgb.red = -1;
		keyrgb.green = -1;
		keyrgb.blue = -1;
	}

	bool verbose;
	bool quiet;
	bool waitkey;
	bool spriteoptimize;
	bool preview;
	bool genmask;
	bool keyguard;

	ubyte_t keycol;
	iRGB keyrgb;
	int cut_xpos;
	int cut_ypos;
	int cut_xsize;
	int cut_ysize;
	int cut_xinc;
	int cut_yinc;
	int cut_xcount;
	int cut_ycount;
	int cut_count;

	float tolerance;
	float keytol;
	std::string sourcefile;
	std::string outfile;
	int ccfields;
//	std::string spriteguidefile;
	Cutmode cutmode;
	Outmode outmode;
	Masklayout masklayout;
	bool masklayout_lock;
	CCRemap ccremap;
	std::string palettefile;
	int planes;
	int tilesize;

	spritemap_t spritemappings;

	// internal: not set via commandline
	bool use_solutionpair_reduction_;
};

Arguments g_allargs;

static void GetArguments(int _argc, char **_argv, Arguments& _args)
{
    CSimpleOpt args(_argc, _argv, g_Options);
    std::string s_outfile;

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
			else if (args.OptionId() == OPT_TOLERANCE)
			{
				std::string s_arg = args.OptionArg();
				float farg = (float)atof(s_arg.c_str());
				if (farg < 0.0f || farg > 1e4f)
				{
					printf("error - invalid arg for tile matching tolerance: %s\n", s_arg.c_str());
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
					printf("error - invalid arg for key guard tolerance: %s\n", s_arg.c_str());
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
				if (s_arg.find("sprites") != std::string::npos)
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
				{
					printf("error - invalid arg for cutmode: %s\n", s_arg.c_str());
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
					printf("error - invalid arg for outmode: %s\n", s_arg.c_str());
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
						printf("error - invalid arg for masklayout: %s\n", s_arg.c_str());
						exit(1);
					}
				}
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
					printf("error - invalid arg for sprite-optimize: %s\n", s_arg.c_str());
					exit(1);
				}
			}
			else if (args.OptionId() == OPT_PALETTE)
			{
				_args.palettefile = args.OptionArg();
			}
			//else if (args.OptionId() == OPT_SPRITEGUIDE)
			//{
			//	_args.spriteguidefile = args.OptionArg();
			//}
			else if (args.OptionId() == OPT_KEYCOL)
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 0 || iarg > 255)
				{
					printf("error - invalid arg for key colour: %s\n", s_arg.c_str());
					exit(1);
				}
				_args.keycol = iarg;	
			}
			else if (args.OptionId() == OPT_KEYRGB)
			{
				std::string s_arg = args.OptionArg();
				
				int r, g, b;
				sscanf(s_arg.c_str(), "%x:%x:%x", &r, &g, &b);

				if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
				{
					printf("error - invalid arg for key rgb: %s\n", s_arg.c_str());
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
					printf("error - invalid arg for key colour: %s\n", s_arg.c_str());
					exit(1);
				}
				_args.keycol = iarg;	
			}
			else if (args.OptionId() == OPT_TILESIZE)
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if ((iarg != 16) && (iarg != 32))
				{
					printf("error - invalid arg for tile size: %s\n", s_arg.c_str());
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
					printf("error - invalid arg for ccfields: %s\n", s_arg.c_str());
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
					printf("error - invalid arg for ccr: %s\n", s_arg.c_str());
					exit(1);
				}
			}
			else if (args.OptionId() == OPT_BITPLANES)
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 0 || iarg > g_maxplanes)
				{
					printf("error - invalid arg for bitplanes/bpp: %s\n", s_arg.c_str());
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
					printf("error - invalid arg for sprite cutting position: %s\n", s_arg.c_str());
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
					printf("error - invalid arg for sprite cutting position: %s\n", s_arg.c_str());
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
					printf("error - invalid arg for sprite cutting size: %s\n", s_arg.c_str());
					exit(1);
				}
				if ((_args.cutmode == Cutmode_SPRITES) && (iarg > 128))
				{
					printf("warning - sprite cutting width %s exceeds max. of 128 for AGT sprite system!\n", s_arg.c_str());
				}
				else
				if ((_args.cutmode == Cutmode_SLABS) && (iarg > 256))
				{
					printf("warning - sprite cutting width %s exceeds max. of 256 for AGT slab system!\n", s_arg.c_str());
				}
				_args.cut_xsize = iarg;	
			}
			else if (args.OptionId() == OPT_SPRYS)
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 1 || iarg > 4096)
				{
					printf("error - invalid arg for sprite cutting size: %s\n", s_arg.c_str());
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
					printf("error - invalid arg for sprite cutting increment/step: %s\n", s_arg.c_str());
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
					printf("error - invalid arg for sprite cutting increment/step: %s\n", s_arg.c_str());
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
					printf("error - invalid arg for sprite cutting x-count: %s\n", s_arg.c_str());
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
					printf("error - invalid arg for sprite cutting y-count: %s\n", s_arg.c_str());
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
					printf("error - invalid arg for sprite cutting count: %s\n", s_arg.c_str());
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
			printf("error - invalid argument: %s\n", args.OptionText());
			exit(1);
		}
    }

	if (_args.sourcefile.length() == 0)
	{
		usage(0);
		exit(1);
	}


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
}


// --------------------------------------------------------------------
//	chunky (8bit) to N-planar conversion
// --------------------------------------------------------------------

static void c2p
(
	ubyte_t* _csrc, uword_t* _pdst, 
	signed short _w, signed short _h,
	signed short _sstride, signed short _dstride,
	signed short _nplanes
)
{	
	// v line loop
	for (signed short yc = 0; yc < _h; ++yc)
	{
		ubyte_t* src = &_csrc[yc * _sstride];
		uword_t* dst = &_pdst[yc * _dstride];
		
		// h word loop
		for (signed short xc = (_w >> 4) - 1; xc >= 0; --xc)
		{
			// h pixel loop				
			for (signed short xbc = 16-1; xbc >= 0; --xbc)
			{
				ubyte_t val = *(src++);
					
				// plane loop
				uword_t* o = dst;
				for (signed short p = _nplanes - 1; p >= 0; --p)
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
	std::cerr << "error: can't open this palette file: " << _fname << std::endl;
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
	std::cerr << "error: can't open this image: " << _fname << std::endl;
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
	std::cerr << "error: can't open complete fileset: " << _fname << std::endl;
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
	std::cerr << "error: can't open this image: " << _fname << std::endl;
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
	std::cerr << "error: can't open this image" << std::endl;
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
		std::cerr << "error: non-indexed colour image or unsupported image type - no palette available" << std::endl;
		return false;
	}

	// not recognized/not found
	std::cerr << "error: can't open this image" << std::endl;
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
		for (int px = 0; px < tilesize; px++)
		{
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
		for (int px = 0; px < tilesize; px++)
		{
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
		mapindex = s_tile8dictionary.size();

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

static void process_image
(
	long_t *&r_ptilemap, int &r_tilemapw, int &r_tilemaph, 
	RGB *_psourcergb, ubyte_t *_psourcecol, ulong_t _sourcew, ulong_t _sourceh
)
{
	s_tile24dictionary.clear();
	s_tile8dictionary.clear();
	s_tileplanardictionary.clear();

	// find source dimensions in terms of tiles
	int tilew = _sourcew / g_allargs.tilesize;
	int tileh = _sourceh / g_allargs.tilesize;

	r_tilemapw = tilew;
	r_tilemaph = tileh;
	r_ptilemap = new long_t[tilew*tileh];

	long_t *ptilemap = r_ptilemap;

	// iterate over source tiles, left-to-right, top-to-bottom
	for (int ty = 0; ty < tileh; ty++)
	{
		for (int tx = 0; tx < tilew; tx++)
		{
			int tilemapindex = process_tile(_psourcergb, _psourcecol, _sourcew, tx, ty);
			*ptilemap++ = tilemapindex;

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

		uword_t *ptileplanar = new uword_t[16*8];

		// convert chunk 8-bit tiles into 8-planar format
		c2p
		(
			/*src=*/ptile8, /*dst*/ptileplanar,
			/*w=*/16, /*h=*/16,
			/*bytes_per_src_line=*/16, /*words_per_dst_line=*/8,
			/*planes=*/8
		);

		s_tileplanardictionary.push_back(ptileplanar);
	}

	std::cout << "...done" << std::endl;
}

// --------------------------------------------------------------------
//	emit .PI1 from 4-plane buffer & palette data
// --------------------------------------------------------------------

static void emit_pi1(std::string outfile, uword_t *buffer4plane, RGB *_ppal24, int pi1_index)
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

	float fr = float(rgb.red);
	float fg = float(rgb.green);
	float fb = float(rgb.blue);

	float best_err = 1e7;
	int best_hit = -1;
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

static void reduce_image24(RGB *_psourcergb, ubyte_t *_psrccol, ubyte_t *_pdstcol0, ubyte_t *_pdstcol1, ulong_t _sourcew, ulong_t _sourceh, RGB *&r_ppal)
{
	unsigned short r_numcolours = 0;
	unsigned short r_numuniquecolours = 0;
	bool success = false;

	// Degas as container for a palette
	if (g_allargs.palettefile.find(".pi1") != std::string::npos)
		success = read_pal_pi1(g_allargs.palettefile.c_str(), r_ppal, r_numcolours, r_numuniquecolours);
	else
	// RIFF format palettes
	if (g_allargs.palettefile.find(".pal") != std::string::npos)
		success = read_pal24(g_allargs.palettefile.c_str(), r_ppal, r_numcolours, r_numuniquecolours);
	else
	// PhotoChrome-generated .cct tile libraries carry the palette as the first part of the header
	if (g_allargs.palettefile.find(".cct") != std::string::npos)
		success = read_pal_cct(g_allargs.palettefile.c_str(), r_ppal, r_numcolours, r_numuniquecolours);
	else
	// a lone .ccp is raw palette data, same as header for .cct, so just reuse that reader for now
	if (g_allargs.palettefile.find(".ccp") != std::string::npos)
		success = read_pal_cct(g_allargs.palettefile.c_str(), r_ppal, r_numcolours, r_numuniquecolours);
	else
	{
		// The .ccs format infers the existence of: .ccs (solutionpairs) + .ccr (RGB->solutionpair reduction map) + .ccp (palette)
		// which all get emitted together from PhotoChrome v6 when using '-ccmode 3' to generate palettes
		// This format also enables a different remapping method in order to apply complementary field dithering.
		if (g_allargs.palettefile.find(".ccs") != std::string::npos)
		{
			success = read_pal_ccs(g_allargs.palettefile.c_str(), r_ppal, r_numcolours, r_numuniquecolours);
			g_allargs.use_solutionpair_reduction_ = true;
		}
		else
		{
			std::cerr << " unrecognized palette format [" << g_allargs.palettefile.c_str() << "] so trying image handlers..." << std::endl;
			success = palette_load(g_allargs.palettefile.c_str(), r_ppal, r_numcolours, r_numuniquecolours);
			if (!success)
				std::cerr << "error: this image format can't be used to supply palettes currently" << std::endl;
		}
	}

	if (success)
	{
		std::cout << " size of palette: " << r_numcolours << std::endl;
		std::cout << " unique 24bit colours available in palette: " << r_numuniquecolours << std::endl;
		if (r_numuniquecolours > (1 << g_allargs.planes))
			std::cout << " warning: target palette contains more colours than " << g_allargs.planes << " bitplanes can encode!" << std::endl;

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

			{
				RGB *psrc = _psourcergb;
				ubyte_t *pdst = _pdstcol0;
				for (int py = 0; py < (int)_sourceh; py++)
				{
					for (int px = 0; px < (int)_sourcew; px++)
					{
						*pdst++ = match_colour(*psrc++, r_ppal, r_numcolours);
					}
				}
			}
		}

		std::cout << " indexed colours used in final image: " << s_num_colours_matched << std::endl;	
	}
	else
	{
		std::cerr << "error: could not read input palette from palette file or image" << std::endl;
		exit(1);
	}
	std::cout << "...done" << std::endl;
}

// --------------------------------------------------------------------
//	emit map data file indicating where tiles go in original image
// --------------------------------------------------------------------

static void emit_tilemap_ccm(std::string outfile, long_t *_ptilemap, int _tilemapw, int _tilemaph)
{
	long_t v;

	replace_extension(outfile, "ccm");

	int mapentries = _tilemapw * _tilemaph;
	for (int i = 0; i < mapentries; i++)
		_ptilemap[i] = endianswap32(_ptilemap[i]);

	FILE *hmap = fopen(outfile.c_str(), "wb");

	v = endianswap32(_tilemapw);
	fwrite(&v, 1, sizeof(v), hmap);
	v = endianswap32(_tilemaph);
	fwrite(&v, 1, sizeof(v), hmap);

	fwrite(_ptilemap, sizeof(long_t), mapentries, hmap);

	fclose(hmap);

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
		uword_t *pout = &buffer4plane[(otx<<2) + ((oty<<4) * (320>>(4-2)))];

		for (int ty = 0; ty < 16; ty++)
		{
			// record 4 planes
			*pout++ = *pin++;
			*pout++ = *pin++;
			*pout++ = *pin++;
			*pout++ = *pin++;
			// next line of PI1
			pout += (320>>(4-2)) - 4;
			// skip 4 planes of 8-plane tile
			pin += 4;
		}

		otx++;
		if (otx >= (320 / 16))
		{
			otx = 0;
			oty++;
			if (oty >= (200 / 16))
			{
				oty = 0;

				// flush image
				emit_pi1(outfile, buffer4plane, _ppalettergb, pi1_index++);
				memset(buffer4plane, 0, 32000);
			}
		}

		s_tileplanardictionary.pop_front();
	}

	// if some tiles are still pending, flush remainder
	if (otx > 0 || oty > 0)
		emit_pi1(outfile, buffer4plane, _ppalettergb, pi1_index++);

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
			byte_t flags = 0;
			fwrite(&flags, 1, sizeof(flags), h);
		}
		// write #bitplanes (1)
		{
			byte_t count = g_allargs.planes;
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
			uword_t tilespace[16*8];

			int tileplanes = g_allargs.planes;
			const int tilewords = tileplanes * 1 * 16;

			for (tileplanardictionary_t::iterator it = s_tileplanardictionary.begin(); it != s_tileplanardictionary.end(); it++)
			{
				uword_t *tile = *it;

				// extract planes
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

bool do_tiles()
{
	ulong_t sourcew = 0;
	ulong_t sourceh = 0;
	ulong_t sourced = 0;
	RGB *m_psourcergb = 0;
	RGB *m_ppalettergb = 0;
	ubyte_t *m_psourcecol = 0;
	ubyte_t *m_psourcecol2 = 0;
	ubyte_t *m_poriginalsrccol = 0;

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
				exit(1);
			}

			if (m_psourcecol)
			{
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
			}

			if (m_psourcecol)
			{
				std::cout << "adopting palette and colourmap provided by source image..." << std::endl;
			}
			else
			{
				std::cout << "reducing 24bit image to external fixed palette..." << std::endl;
				m_psourcecol = new ubyte_t[sourcew * sourceh]; // (ubyte_t*)malloc(sourcew * sourceh);
				if (g_allargs.ccfields == 2)
					m_psourcecol2 = new ubyte_t[sourcew * sourceh]; // (ubyte_t*)malloc(sourcew * sourceh);
				// reduce source image to indexed colour with fixed palette
				reduce_image24(m_psourcergb, 0, m_psourcecol, m_psourcecol2, sourcew, sourceh, m_ppalettergb);
			}

			for (int ccf = 0; ccf < g_allargs.ccfields; ccf++)
			{
				std::cout << "extracting unique tiles..." << std::endl;

				// generate tile dictionary from image
				if (ccf == 0)
					process_image(ptilemap, tilemapw, tilemaph, m_psourcergb, m_psourcecol, sourcew, sourceh);
				else
					process_image(ptilemap, tilemapw, tilemaph, m_psourcergb, m_psourcecol2, sourcew, sourceh);

				std::cout << "...done" << std::endl;
				// calculate compressed footprint
				float footprint = (float)s_tile8dictionary.size() / float(tilemapw*tilemaph);
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

				// convert 8bit indexed tiles into 4-plane graphics
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
						emit_tilemap_ccm(outfile.c_str(), ptilemap, tilemapw, tilemaph);
					}
				}

			} // ccf loop

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
			std::cerr << "error: could not decode [" << g_allargs.sourcefile.c_str() << "] source image" << std::endl;
		}
	}
	else
	{
		std::cerr << "error: could not open [" << g_allargs.sourcefile.c_str() << "] source file" << std::endl;
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

	preshift_t(int _shift, int _planes, int _height, int _wordwidth, uword_t *_data)
		: shift_(_shift),
		  planes_(_planes),
		  height_(_height),
		  wordwidth_(_wordwidth),
		  data_(_data)
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

			//printf("shift: %d\n", s);
			//printf("dst_wordwidth: %d\n", dst_wordwidth);
			//printf("preshiftwords: %d\n", preshiftwords);

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

void add_sprite_frame(ubyte_t* _psource8, int _x, int _y, bool _xflip, bool _yflip, int _hs, int _vs)
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

	// cut chunky sprite to chunky colrbuffer & maskbuffer

	for (int psy = cymin, py = 0; psy < cymax; psy++, py++)
	{
		int psyr = _yflip ? ((g_sprite_height - 1) - psy) : psy;

		for (int psx = cxmin, px = 0; psx < cxmax; psx++, px++)
		{
			int psxr = _xflip ? ((g_sprite_width - 1) - psx) : psx;

			ubyte_t c = _psource8[((psyr + _y) * g_spritesource_w) + (psxr + _x)];

			// colour buffer receives actual colour, or 0 for keycol (so two colours can potentially map to 0)
			g_colrbuffer8[(py * g_sprite_framebpwidth) + px] = (c == g_allargs.keycol) ? 0x00 : c;
			// mask buffer receives 0 for colour, or 1 for keycol
			g_maskbuffer8[(py * g_sprite_framebpwidth) + px] = (c == g_allargs.keycol) ? 0xFF : 0x00;
		}
	}

	// chunky buffer -> planar buffer

	// colour component
	c2p
	(
		/*src=*/g_colrbuffer8, /*dst*/g_colrbufferbpl,
		/*w=*/g_sprite_framebpwidth, /*h=*/g_sprite_frameheight,
		/*bytes_per_src_line=*/g_sprite_framebpwidth, /*words_per_dst_line=*/g_sprite_framebpwidth>>1,
		/*planes=*/8	//g_allargs.planes
	);

	// mask component
	c2p
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

enum RegClass
{
	RegClass_Data,
	RegClass_Address
};

class dataregister
{
public:

	dataregister(const char* _name, int _index, RegClass _regclass, bool _alloc = false, bool _constant = false, ulong_t _init = 0)
		: name(_name),
		  index(_index),
		  regclass(_regclass),
		  allocatedH(_alloc),
		  allocatedL(_alloc),
		  constant(_constant),
		  H(_init >> 16),
		  L(_init & 0xFFFF),
		  stableH(_alloc),
		  stableL(_alloc)
	{
	}

	std::string name;
	int index;
	RegClass regclass;
	bool allocatedH;
	bool allocatedL;
	bool constant;
	bool stableH;
	bool stableL;
	uword_t H;
	uword_t L;
};

class BLiT
{
public:

	BLiT()
		: xcount(0)
	{
	}

	uword_t em[3];
	uword_t xcount;
};

typedef std::map<uword_t, int> relationtable;

class emx_metric_t
{
public:

	emx_metric_t()
		: incidence(0),
		  references(0),
		  name(-1),
		  //incidence_em0(0),
		  //incidence_em1(0),
		  //incidence_em2(0),
		  //
		  fincidence_score(0.0f),
		  fadjacency_score(0.0f)
	{
	}

	int incidence;
	int references;
	int name;
	//int incidence_em0;
	//int incidence_em1;
	//int incidence_em2;
	relationtable line_adjacencies;
	relationtable left_adjacencies;

	float fincidence_score;
	float fadjacency_score;

};

typedef std::map<uword_t, emx_metric_t> metricmap;

static const int max_virtual_registers = 100;

class Operation;

int DataSource_costs[] =
{
	(0<<8),
	(8<<8),
	(12<<8),
	(12<<8),
	(16<<8),
	(20<<8)+1					// small bias against long immediate because it's harder to peephole these vs short immediates
};

enum DataSource
{
	DataSource_None,
	DataSource_RegisterL,		// 8c
	DataSource_RegisterHL,		// 12c
	DataSource_Immediate16,		// 12c
	DataSource_RegisterLL,		// 16c
	DataSource_MixedIR,			// 20c
	DataSource_Immediate32,		// 20+1c
};

class preload
{
public:

	preload(uword_t _l)
		: H(0), L(_l), 
		  validH(false),
		  validL(true)
	{
	}

	uword_t H;
	uword_t L;
	bool validH;
	bool validL;
};

class Context
{
public:

	void reset_registers()
	{
		int r = 0;

		registers.clear();

		registers.push_back(dataregister("d0", r++, RegClass_Data));
		registers.push_back(dataregister("d3", r++, RegClass_Data));
		registers.push_back(dataregister("d4", r++, RegClass_Data));
		registers.push_back(dataregister("d5", r++, RegClass_Data));
		registers.push_back(dataregister("d6", r++, RegClass_Data));
		registers.push_back(dataregister("d7", r++, RegClass_Data));

		for (int p = 0; (p < registers.size()) && (p < preloads.size()); p++)
		{
			if (preloads[p].validL)
			{
				registers[p].stableL = true;
				registers[p].allocatedL = true;
				registers[p].L = preloads[p].L;
			}
			if (preloads[p].validH)
			{
				registers[p].stableH = true;
				registers[p].allocatedH = true;
				registers[p].H = preloads[p].H;
			}
		}

		registers.push_back(dataregister("a3", r++, RegClass_Address,	true, true,		0xffff8a32));
		registers.push_back(dataregister("a5", r++, RegClass_Address,	true, true,		0xffff8a3c));
		registers.push_back(dataregister("a0", r++, RegClass_Address,	true, true,		0xffff8a38));

		registers.push_back(dataregister("a2", r++, RegClass_Address,	true, true,		0xffff8a28));
		registers.push_back(dataregister("a6", r++, RegClass_Address,	true, true,		0xffff8a2a));
		registers.push_back(dataregister("a4", r++, RegClass_Address,	true, true,		0xffff8a2c));

		registers.push_back(dataregister("d1", r++, RegClass_Data,		true, true,		0xffffffc0));
		registers.push_back(dataregister("d2", r++, RegClass_Data,		true, true,		0x00000004));

//		registers.push_back(dataregister("sr", r++, true, true, 0x00002300));
	}

	Context(int _srcww, int _srch)
		: srcww(_srcww), srch(_srch),
		  dstww(3),
		  emaddr(0),
		  memaddr(0),
		  cycles(0),
		  genout(genstr),
		  verbose(false)
	{
		memory = new uword_t[dstww * srch];

//		memset(preloads, 0, sizeof(preloads));

//		blitter.xcount = srcww;

		//BLiTDST_name = "BLiTDST.w";
		//BLiTCTRL_name = "BLiTCTRL.w";
		//BLiTYC_name = "BLiTYC.w";
		BLiTDST_name = "(a3)";
		BLiTCTRL_name = "(a5)";
		BLiTYC_name = "(a0)";

//		BLiTEM_names.push_back("BLiTEM1.w");
//		BLiTEM_names.push_back("BLiTEM2.w");
//		BLiTEM_names.push_back("BLiTEM3.w");
		BLiTEM_names.push_back("(a2)");
		BLiTEM_names.push_back("(a6)");
		BLiTEM_names.push_back("(a4)");
//		BLiTEM1_name = "BLiTEM1.w";
//		BLiTEM2_name = "BLiTEM2.w";
//		BLiTEM3_name = "BLiTEM3.w";
		//BLiTEM1_name = "(a2)";
		//BLiTEM2_name = "(a6)";
		//BLiTEM3_name = "(a4)";

		DSTADDR_regname = "a1";
		BLiTCTRL_go_srcname = "d1";	//#$c0;
		BLiTYC_planes_srcname = "d2";	//#4;

		// free: d0,d3,d4,d5,d6,d7

//		reset_registers();

		//{
		//	char *name = "r00";
		//	for (int v = 0; v < max_virtual_registers; v++)
		//	{
		//		registers.push_back(dataregister(name, r++));

		//		name[1] = (v / 10) + '0';
		//		name[2] = (v % 10) + '0';
		//	}
		//}
	}

	~Context()
	{
		delete[] memory;
	}

	void reset_sim(bool _verbose)
	{
		verbose = _verbose;
		memset(memory, 0xAA, dstww * srch * sizeof(uword_t));
		emaddr = 0;
		memaddr = 0;
		cycles = 0;
		blitter.xcount = srcww;
		reset_registers();
		metrics_sim = metrics;
	}

	void register_rename(std::list<Operation*>::iterator curr, int _rold, int _rnew);

	DataSource sim_resolve_data16(uword_t _data, int &r_reg_index, bool _injectops);
	DataSource sim_resolve_data32(uword_t _dataH, uword_t _dataL, int &r_regH_index, int &r_regL_index, bool _injectops);

	void sim_track_datarefs(uword_t _data)
	{
		emx_metric_t& m = metrics_sim[_data];
		m.references--;
		if (m.references == 0)
		{
//			m.incidence = 0;

			// this data is no longer required - free up whatever register(s) are assigned with it
			// look for existent value in active registers
			for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++)
			{
				if (!(it->constant))
				{
					if ((it->allocatedL) && (it->L == _data))
					{
						//if (verbose)
						//	printf("> released value [$%04x] from register [%s]L\n", _data, it->name.c_str());
						it->allocatedL = false;
					}

					if ((it->allocatedH) && (it->H == _data))
					{
						//if (verbose)
						//	printf("> released value [$%04x] from register [%s]H\n", _data, it->name.c_str());
						it->allocatedH = false;
					}
				}
			}
		}
	}

	void peephole1();
	void peephole2();
	void peephole3() { }

	void register_sweep();

	bool tag_collision(std::list<Operation*>::iterator _start, int _rold, int _rnew);

	void assemble()
	{
		std::string& str = genout.str();  
		std::ofstream cfile;
		cfile.open("tmp68k.s");  
		cfile << str;  
		cfile.close();  

		// execute assembler, turning our temporary .s into binary 68k
		ShellExecute(NULL, "open", "rmac.exe", "-n -o tmp68k.bin tmp68k.s", NULL, SW_HIDE);
	}

	std::string genstr;
	std::stringstream genout;
	bool verbose;

	std::vector<preload> preloads;

	int srcww, dstww;
	int srch;

	metricmap metrics;
	metricmap metrics_sim;

	std::vector<dataregister> registers;

	std::list<Operation*> operations;

	std::string BLiTDST_name;
	std::string BLiTCTRL_name;
//	std::string BLiTEM1_name;
//	std::string BLiTEM2_name;
//	std::string BLiTEM3_name;
	std::string BLiTYC_name;
	std::vector<std::string> BLiTEM_names;

	std::string DSTADDR_regname;
	std::string BLiTCTRL_go_srcname;
	std::string BLiTYC_planes_srcname;


	// blitter
	BLiT blitter;
	int emaddr;

	// framebuffer
	uword_t *memory;
	int memaddr;

//	wordpair_t registers[16];
	int cycles;

	char buf[1024];
};

enum OpKind
{
	OpKind_Other,
	OpKind_LoadI16,
	OpKind_LoadI32,
	OpKind_StoreI16,
	OpKind_StoreI32,
};

enum OpWrite
{
	OpWrite_None,
	OpWrite_Replace,
	OpWrite_Modify,
	OpWrite_ReplaceHCombineL,		// upper word replaced/extended, lower word combined from another source
	OpWrite_Combine,
};

enum OpSource
{
	OpSource_None,
	OpSource_Immediate,
	OpSource_Register,
	OpSource_Memory,
};

enum OpSize
{
	OpSize_8,
	OpSize_16,
	OpSize_32,
	OpSize_None
};

class Operation
{
public:

	Operation
	(
		Context &_ctx, 
		OpKind _kind, 
		OpSize _opsize,
		OpSource _opsrc,
		OpSource _opdst,
		OpWrite _opwr,
		int _cost
	)
		: ctx(_ctx),
		  kind(_kind),
		  opsize(_opsize),
		  opsrc(_opsrc),
		  opdst(_opdst),
		  opwr(_opwr),
		  cost(_cost),
		  sreg(-1),
		  dreg(-1),
		  stagH(-1),
		  stagL(-1),
		  dtagH(-1),
		  dtagL(-1)
	{
	}

	virtual void onExecute() = 0;
	virtual void onEmit() { };

	void execute()
	{
		onExecute();

		// mark parts of registers which become stable for reading/modifying/combining as a result of a replace-write
		if (opdst == OpSource_Register)
		{
			if (opwr == OpWrite_Replace)
				ctx.registers[dreg].stableL = true;

			if (opsize == OpSize_32)
			{
				if (opwr == OpWrite_Replace || opwr == OpWrite_ReplaceHCombineL)
					ctx.registers[dreg].stableH = true;
			}
		}
	}

	void emit()
	{
		onEmit();
		printf("%s", ctx.buf);
		ctx.genout << ctx.buf;
	}

	int alltagsH[16];
	int alltagsL[16];

	Context &ctx;
	OpKind kind;
	OpSize opsize;
	OpSource opsrc;
	OpSource opdst;
	OpWrite opwr;

	int sreg;
	int dreg;

	int stagH;
	int stagL;
	int dtagH;
	int dtagL;

	int cost;
};

class EM_MoveI16 : public Operation
{
public:

	EM_MoveI16(Context &_ctx, int _em, uword_t _data)
		: Operation(_ctx, OpKind_StoreI16, OpSize_16, OpSource_Immediate, OpSource_Memory, OpWrite_Replace, 12),
		  em(_em),
		  data(_data)
	{
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " move.w #$%04x,%s\t;%d [EM:%d]\n", 
			data, 
			ctx.BLiTEM_names[em].c_str(), 
			cost,
			em+1
		);
	}

	virtual void onExecute()
	{
		ctx.blitter.em[em] = data;// ctx.registers[iLHS].L;
		ctx.cycles += cost;
		ctx.sim_track_datarefs(data);
	}

	int em;
	uword_t data;
};

class EM_MoveI32 : public Operation
{
public:

	EM_MoveI32(Context &_ctx, int _em, uword_t _dataH, uword_t _dataL)
		: Operation(_ctx, OpKind_StoreI32, OpSize_32, OpSource_Immediate, OpSource_Memory, OpWrite_Replace, 20),
		  em(_em),
		  dataH(_dataH),
		  dataL(_dataL)
	{
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " move.l #$%04x%04x,%s\t;%d [EM:%d:%d]\n", 
			dataH, 
			dataL, 
			ctx.BLiTEM_names[em].c_str(), 
			cost,
			em+1,em+2
		);
	}

	virtual void onExecute()
	{
		ctx.blitter.em[em + 0] = dataH;// ctx.registers[iLHS].L;
		ctx.blitter.em[em + 1] = dataL;// ctx.registers[iLHS].L;
		ctx.cycles += cost;
		ctx.sim_track_datarefs(dataH);
		ctx.sim_track_datarefs(dataL);
	}

	int em;
	uword_t dataH;
	uword_t dataL;
};

class EM_MoveR16 : public Operation
{
public:

	EM_MoveR16(Context &_ctx, int _em, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Memory, OpWrite_Replace, 8),
		  em(_em)
	{
		sreg = _reg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " move.w %s,%s\t\t;%d [EM:%d] ($%04x)\n", 
			ctx.registers[sreg].name.c_str(), 
			ctx.BLiTEM_names[em].c_str(), 
			cost,
			em+1,
			ctx.registers[sreg].L);
	}

	virtual void onExecute()
	{
		ctx.blitter.em[em] = ctx.registers[sreg].L;
		ctx.cycles += cost;
		ctx.sim_track_datarefs(ctx.registers[sreg].L);
	}

	int em;
	uword_t data;
};

class EM_MoveR32 : public Operation
{
public:

	EM_MoveR32(Context &_ctx, int _em, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_32, OpSource_Register, OpSource_Memory, OpWrite_Replace, 12),
		  em(_em)
	{
		sreg = _reg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " move.l %s,%s\t\t;%d [EM:%d:%d] ($%04x%04x)\n", 
			ctx.registers[sreg].name.c_str(), 
			ctx.BLiTEM_names[em].c_str(), 
			cost,
			em+1,em+2,
			ctx.registers[sreg].H, ctx.registers[sreg].L
		);
	}

	virtual void onExecute()
	{
		ctx.blitter.em[em + 0] = ctx.registers[sreg].H;
		ctx.blitter.em[em + 1] = ctx.registers[sreg].L;
		ctx.cycles += cost;
		ctx.sim_track_datarefs(ctx.registers[sreg].H);
		ctx.sim_track_datarefs(ctx.registers[sreg].L);
	}

	int em;
};


class DR_LoadI16 : public Operation
{
public:

	DR_LoadI16(Context &_ctx, int _reg, uword_t _data)
		: Operation(_ctx, OpKind_LoadI16, OpSize_16, OpSource_Immediate, OpSource_Register, OpWrite_Replace, 8),
		  data(_data)
	{
		dreg = _reg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " move.w #$%04x,%s\t;%d\n", 
			data, 
			ctx.registers[dreg].name.c_str(), 
			cost
		);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = data;// ctx.registers[iLHS].L;
		ctx.registers[dreg].allocatedL = true;
		ctx.cycles += cost;
	}

	uword_t data;
};

class DR_Clr16 : public Operation
{
public:

	DR_Clr16(Context &_ctx, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_None, OpSource_Register, OpWrite_Replace, 4)
	{
		dreg = _reg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " clr.w %s\t;%d\n", 
			ctx.registers[dreg].name.c_str(), 
			cost
		);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = 0;
		ctx.registers[dreg].allocatedL = true;
		ctx.cycles += cost;
	}
};

class DR_Not16 : public Operation
{
public:

	DR_Not16(Context &_ctx, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_None, OpSource_Register, OpWrite_Modify, 4)
	{
		sreg = dreg = _reg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " not.w %s\t;%d\n", 
			ctx.registers[dreg].name.c_str(), 
			cost
		);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = ~ctx.registers[dreg].L;
		ctx.cycles += cost;
	}
};

class DR_Neg16 : public Operation
{
public:

	DR_Neg16(Context &_ctx, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_None, OpSource_Register, OpWrite_Modify, 4)
	{
		sreg = dreg = _reg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " neg.w %s\t;%d\n", 
			ctx.registers[dreg].name.c_str(), 
			cost
		);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = uword_t(-(word_t(ctx.registers[dreg].L)));
		ctx.cycles += cost;
	}
};

class DR_AddSubQ16 : public Operation
{
public:

	DR_AddSubQ16(Context &_ctx, int _imm, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Immediate, OpSource_Register, OpWrite_Modify, 4),
		  imm(_imm)
	{
		sreg = dreg = _reg;
	}

	virtual void onEmit()
	{
		if (imm > 0)
		{
			sprintf(ctx.buf, " addq.w #%d,%s\t;%d [$%04x]\n",
				imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
		else
		{
			sprintf(ctx.buf, " subq.w #%d,%s\t;%d [$%04x]\n",
				-imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = uword_t(word_t(ctx.registers[dreg].L) + word_t(imm));
		ctx.cycles += cost;
	}

	int imm;
};

class DR_LSI16 : public Operation
{
public:

	DR_LSI16(Context &_ctx, int _imm, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Immediate, OpSource_Register, OpWrite_Modify, 4),
		  imm(_imm)
	{
		sreg = dreg = _reg;
	}

	virtual void onEmit()
	{
		if (imm > 0)
		{
			sprintf(ctx.buf, " lsr.w #%d,%s\t;%d [$%04x]\n",
				imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
		else
		{
			sprintf(ctx.buf, " lsl.w #%d,%s\t;%d [$%04x]\n",
				-imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
	}

	virtual void onExecute()
	{
		if (imm < 0)
			ctx.registers[dreg].L = ctx.registers[dreg].L << (-imm);
		else
			ctx.registers[dreg].L = ctx.registers[dreg].L >> imm;

		ctx.cycles += cost;
	}

	int imm;
};

class DR_ASI16 : public Operation
{
public:

	DR_ASI16(Context &_ctx, int _imm, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Immediate, OpSource_Register, OpWrite_Modify, 4),
		  imm(_imm)
	{
		sreg = dreg = _reg;
	}

	virtual void onEmit()
	{
		if (imm > 0)
		{
			sprintf(ctx.buf, " asr.w #%d,%s\t;%d [$%04x]\n",
				imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
		else
		{
			sprintf(ctx.buf, " asl.w #%d,%s\t;%d [$%04x]\n",
				-imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
	}

	virtual void onExecute()
	{
		if (imm < 0)
			ctx.registers[dreg].L = uword_t(word_t(ctx.registers[dreg].L) << (-imm));
		else
			ctx.registers[dreg].L = uword_t(word_t(ctx.registers[dreg].L) >> imm);

		ctx.cycles += cost;
	}

	int imm;
};

class DR_ROI16 : public Operation
{
public:

	DR_ROI16(Context &_ctx, int _imm, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Immediate, OpSource_Register, OpWrite_Modify, 4),
		  imm(_imm)
	{
		sreg = dreg = _reg;
	}

	virtual void onEmit()
	{
		if (imm > 0)
		{
			sprintf(ctx.buf, " ror.w #%d,%s\t;%d [$%04x]\n",
				imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
		else
		{
			sprintf(ctx.buf, " rol.w #%d,%s\t;%d [$%04x]\n",
				-imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
	}

	virtual void onExecute()
	{
		if (imm < 0)
			ctx.registers[dreg].L = _rotl16(ctx.registers[dreg].L, (-imm));
		else
			ctx.registers[dreg].L = _rotr16(ctx.registers[dreg].L, imm);

		ctx.cycles += cost;
	}

	int imm;
};

class DR_AddR16 : public Operation
{
public:

	DR_AddR16(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_Combine, 4)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " add.w %s,%s\t;%d [$%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = uword_t(word_t(ctx.registers[dreg].L) + word_t(ctx.registers[sreg].L));
		ctx.cycles += cost;
	}
};

class DR_BchgR32 : public Operation
{
public:

	DR_BchgR32(Context &_ctx, OpSize _opsize, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, _opsize, OpSource_Register, OpSource_Register, OpWrite_Combine, 4)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " bchg %s,%s\t;%d [$%04x%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].H, 
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].H = ctx.registers[dreg].H ^ ((1 << (ctx.registers[sreg].L & 31)) >> 16);
		ctx.registers[dreg].L = ctx.registers[dreg].L ^ ((1 << (ctx.registers[sreg].L & 31)) & 0xFFFF);
		ctx.cycles += cost;
	}
};

class DR_MuluR16 : public Operation
{
public:

	DR_MuluR16(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_ReplaceHCombineL, 70)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " mulu.w %s,%s\t;%d [$%04x%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].H,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ulong_t result = ctx.registers[dreg].L * ctx.registers[sreg].L;
		ctx.registers[dreg].H = result >> 16;
		ctx.registers[dreg].L = result & 0xFFFF;
		ctx.cycles += cost;
	}
};

class DR_MulsR16 : public Operation
{
public:

	DR_MulsR16(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_ReplaceHCombineL, 70)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " muls.w %s,%s\t;%d [$%04x%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].H,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		long_t result = word_t(ctx.registers[dreg].L) * word_t(ctx.registers[sreg].L);
		ctx.registers[dreg].H = uword_t(ulong_t(result) >> 16);
		ctx.registers[dreg].L = uword_t(ulong_t(result) & 0x0000FFFF);
		ctx.cycles += cost;
	}
};

class DR_SubR16 : public Operation
{
public:

	DR_SubR16(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_Combine, 4)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " sub.w %s,%s\t;%d [$%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = uword_t(word_t(ctx.registers[dreg].L) - word_t(ctx.registers[sreg].L));
		ctx.cycles += cost;
	}
};

class DR_AndR16 : public Operation
{
public:

	DR_AndR16(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_Combine, 4)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " and.w %s,%s\t;%d [$%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = ctx.registers[dreg].L & ctx.registers[sreg].L;
		ctx.cycles += cost;
	}
};

class DR_OrR16 : public Operation
{
public:

	DR_OrR16(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_Combine, 4)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " or.w %s,%s\t;%d [$%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = ctx.registers[dreg].L | ctx.registers[sreg].L;
		ctx.cycles += cost;
	}
};

class DR_XorR16 : public Operation
{
public:

	DR_XorR16(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_Combine, 4)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " eor.w %s,%s\t;%d [$%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = ctx.registers[dreg].L ^ ctx.registers[sreg].L;
		ctx.cycles += cost;
	}
};

class DR_SccR16 : public Operation
{
public:

	DR_SccR16(Context &_ctx, bool _cond, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_None, OpSource_Register, OpWrite_Modify, 4),
		  cond(_cond)
	{
		sreg = dreg = _dreg;
	}

	virtual void onEmit()
	{
		if (cond)
		{
			sprintf(ctx.buf, " st %s\t;%d [$%04x]\n",
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
		else
		{
			sprintf(ctx.buf, " sf %s\t;%d [$%04x]\n",
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
	}

	virtual void onExecute()
	{
		if (cond)
			ctx.registers[dreg].L = ctx.registers[dreg].L | 0x00FF;
		else
			ctx.registers[dreg].L = ctx.registers[dreg].L & 0xFF00;

		ctx.cycles += cost;
	}

	bool cond;
};

class DR_TasR16 : public Operation
{
public:

	DR_TasR16(Context &_ctx, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_None, OpSource_Register, OpWrite_Modify, 4)
	{
		sreg = dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " tas.b %s\t;%d [$%04x]\n",
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = ctx.registers[dreg].L | 0x0080;
		ctx.cycles += cost;
	}
};

class DR_SwapR32 : public Operation
{
public:

	DR_SwapR32(Context &_ctx, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_32, OpSource_None, OpSource_Register, OpWrite_Replace, 4)
		// note: swap acts like a replace since for each word, the new value doesn't depend on the previous value
		// however it is not injected if the word to be used (usually upper) is not stable in the first place
	{
		sreg = dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " swap %s\t;%d [$%04x]\n",
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		uword_t tmp = ctx.registers[dreg].L;
		ctx.registers[dreg].L = ctx.registers[dreg].H;
		ctx.registers[dreg].H = tmp;
		ctx.cycles += cost;
	}
};

class DR_LoadI32 : public Operation
{
public:

	DR_LoadI32(Context &_ctx, int _reg, uword_t _dataH, uword_t _dataL)
		: Operation(_ctx, OpKind_LoadI32, OpSize_32, OpSource_Immediate, OpSource_Register, OpWrite_Replace, 12),
		  dataH(_dataH),
		  dataL(_dataL)
	{
		dreg = _reg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " move.l #$%04x%04x,%s\t;%d\n", dataH, dataL, ctx.registers[dreg].name.c_str(), cost);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].H = dataH;
		ctx.registers[dreg].L = dataL;
		ctx.registers[dreg].allocatedH = true;
		ctx.registers[dreg].allocatedL = true;
		ctx.cycles += cost;
	}

	uword_t dataH;
	uword_t dataL;
};

class BLiT_Go : public Operation
{
public:

	BLiT_Go(Context &_ctx)
		: Operation(_ctx, OpKind_Other, OpSize_None, OpSource_None, OpSource_None, OpWrite_None, 8)
	{
	}

	virtual void onEmit()
	{
		char buf[1024];
		sprintf(ctx.buf,	" move.w %s,%s\n", ctx.BLiTYC_planes_srcname.c_str(), ctx.BLiTYC_name.c_str());
		sprintf(buf,		" move.l %s,%s\n", ctx.DSTADDR_regname.c_str(), ctx.BLiTDST_name.c_str());
		strcat(ctx.buf, buf);
		sprintf(buf,		" move.b %s,%s\n", ctx.BLiTCTRL_go_srcname.c_str(), ctx.BLiTCTRL_name.c_str());
		strcat(ctx.buf, buf);
	}

	virtual void onExecute()
	{
		ctx.memory[ctx.memaddr + 0] = ctx.blitter.em[0];

		if (ctx.blitter.xcount == 2)
		{
			ctx.memory[ctx.memaddr + 1] = ctx.blitter.em[2];
		}
		else
		if (ctx.blitter.xcount == 3)
		{
			ctx.memory[ctx.memaddr + 1] = ctx.blitter.em[1];
			ctx.memory[ctx.memaddr + 2] = ctx.blitter.em[2];
		}
	}
};

class DST_Lea : public Operation
{
public:

	DST_Lea(Context &_ctx, int _delta)
		: Operation(_ctx, OpKind_Other, OpSize_None, OpSource_None, OpSource_None, OpWrite_None, 8),
		  delta(_delta)
	{
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " lea %d(%s),%s\n", delta * 188, ctx.DSTADDR_regname.c_str(), ctx.DSTADDR_regname.c_str());
	}

	virtual void onExecute()
	{
		ctx.memaddr += delta * ctx.dstww;
	}

	int delta;
};

class Terminate : public Operation
{
public:

	Terminate(Context &_ctx)
		: Operation(_ctx, OpKind_Other, OpSize_None, OpSource_None, OpSource_None, OpWrite_None, 0)
	{
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " rts\n");
	}

	virtual void onExecute()
	{
	}
};

// metrics: 
//	total incidence
//	incidence at EM0,EM1,EM2
//	line adjacency vs all others
//	left-adjacency vs all others

typedef std::map<u64, int> linemap;


void print_dataword(uword_t _data)
{
	uword_t v = _data;
	for (int i = 0; i < 16; i++)
	{
		if (v & 0x8000)
			putchar('1');
		else
			putchar('0');

		v <<= 1;
	}
}

void print_datalong(ulong_t _data)
{
	ulong_t v = _data;
	for (int i = 0; i < 32; i++)
	{
		if (v & 0x80000000)
			putchar('1');
		else
			putchar('0');

		v <<= 1;
	}
}

void print_data(u64 _data, int _count)
{
	u64 v = _data;
	u64 mask = (u64)1 << (_count - 1);
	for (int i = 0; i < _count; i++)
	{
		if (v & mask)
			putchar('1');
		else
			putchar('0');

		v <<= 1;
	}
}

//static void trylineseq(const std::vector<int> &_stack_free, const std::vector<int> &_stack, std::vector<int> &_stack_best)
//{
//	int freesize = _stack_free.size();
//	if (freesize == 0)
//		return;
//
//	std::vector<int> stack;
//	std::vector<int> stack_free;
//
//	for (int n = 0; n < freesize; n++)
//	{
//		// try all positions as next place
//		stack = _stack;
//		stack.push_back(_stack_free[n]);
//
//		stack_free = _stack_free;
//		stack_free[n] = _stack_free[freesize-1];
//		stack_free.pop_back();
//
//		trylineseq(stack_free, stack, _stack_best);
//	}
//}

DataSource Context::sim_resolve_data16(uword_t _data, int &r_reg_index, bool _injectops)
{
	// 1) look for existent value in active registers (allocated or otherwise)
	for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++)
	{
		if (it->stableL && 
			(it->L == _data))
		{
			if (_injectops)
			{
				printf("> referenced value [$%04x] in register [%s]L\n", _data, it->name.c_str());
			}
			r_reg_index = it->index;
			return DataSource_RegisterL;
		}
	}

	// 2) assign value to unallocated register if sufficient reuse expected
	if (metrics_sim[_data].references >= 2)
	{
		int best_refcount = 1<<30;
		int best_reg = -1;
		int i = 0;
		for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++, i++)
		{
			if (!(it->constant))
			{
				if (!(it->allocatedL))
				{
					if (_injectops)
					{
						printf("> assigned value [$%04x] to register [%s]L\n", _data, it->name.c_str());
						operations.push_back(new DR_LoadI16(*this, it->index, _data));

						it->allocatedL = true;
						it->L = _data;

						operations.back()->execute();
					}

					r_reg_index = it->index;
					return DataSource_RegisterL;
				}
				else
				{
					// meanwhile, look for the best still-active register to steal
					emx_metric_t &m = metrics_sim[it->L];
					if (m.references < best_refcount)
					{
						best_refcount = m.references;
						best_reg = i;
					}
				}
			}
		}

		// 3) steal allocated register, if estimated saving makes it worthwhile
		// factor +1 cost for reload
		if ((best_reg >= 0) && 
			(metrics_sim[_data].references > (best_refcount+1)))
		{
			dataregister &r = registers[best_reg];
			if (_injectops)
			{
				printf("> replaced value [$%04x] with [$%04x] at register [%s]L\n", r.L, _data, r.name.c_str());
				r.L = _data;
				// allows future choices to acknowledge the reuse incidence looks different from now on
//				metrics[_data].incidence = metrics[_data].references;
			}
			r_reg_index = r.index;
			return DataSource_RegisterL;			
		}
	}

	// single-use, or last resort - just load immediate data
	if (_injectops)
		printf("> resolved value [$%04x] as immediate\n", _data);

	return DataSource_Immediate16;
}

DataSource Context::sim_resolve_data32(uword_t _dataH, uword_t _dataL, int &r_regH_index, int &r_regL_index, bool _injectops)
{
	// 1) look for existent value in active registers (allocated or otherwise)
	for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++)
	{
		if (it->stableH &&
			(it->H == _dataH) && 
			(it->L == _dataL)
		)
		{
			if (_injectops)
			{
				printf("> referenced value [$%04x%04x] in register [%s]HL\n", _dataH, _dataL, it->name.c_str());
			}
			r_regH_index = it->index;
			return DataSource_RegisterHL;
		}
	}

	// 2) assign value to unallocated register if sufficient reuse expected
	if ((metrics_sim[_dataH].references >= 1) && 
		(metrics_sim[_dataL].references >= 2))
	{
		int best_refcount = 1<<30;
		int best_reg = -1;
		int i = 0;
		for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++, i++)
		{
			// don't touch constant registers
			if (!(it->constant))
			{
				// don't touch allocated fields
				if (!(it->allocatedH) && 
					!(it->allocatedL))
				{
					if (_injectops)
					{
						printf("> assigned value [$%04x%04x] to register [%s]HL\n", _dataH, _dataL, it->name.c_str());

						operations.push_back(new DR_LoadI32(*this, it->index, _dataH, _dataL));

						it->allocatedH = true;
						it->allocatedL = true;
						it->H = _dataH;
						it->L = _dataL;

						operations.back()->execute();
					}

					r_regH_index = it->index;
					return DataSource_RegisterHL;
				}
				else
				{
					// meanwhile, look for the best still-active register to steal
					//emx_metric_t &m = metrics[it->L];
					//if (m.references < best_refcount)
					//{
					//	best_refcount = m.references;
					//	best_reg = i;
					//}
				}
			}
		}

		// 3) steal allocated register, if estimated saving makes it worthwhile
		// factor +1 cost for reload
		//if (best_reg >= 0 && metrics[_data].references > (best_refcount+1))
		//{
		//	dataregister &r = registers[best_reg];
		//	printf("> replaced value [$%04x] with [$%04x] at register [%s]L\n", r.L, _data, r.name.c_str());
		//	r.L = _data;
		//	// allows future choices to acknowledge the reuse incidence looks different from now on
		//	metrics[_data].incidence = metrics[_data].references;
		//	r_reg_index = r.index;
		//	return DataSource_RegisterL;			
		//}
	}

	// look for mixed combination (high immediate + low reg) for cases with nontrivial high word
	// since immediate words are easier to peephole than arbitrary immediate longs
	for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++)
	{
		if ((it->H != 0x0000) && 
			(it->H != 0xFFFF))
		{
			if (it->stableL && 
				(it->L == _dataL))
			{
				if (_injectops)
				{
					printf("> referenced value [$%04x] in register [%s]L\n", _dataL, it->name.c_str());
				}
				r_regL_index = it->index;
				return DataSource_MixedIR;
			}
		}
	}

	// single-use, or last resort - just load immediate data
	if (_injectops)
		printf("> resolved value [$%04x%04x] as immediate\n", _dataH, _dataL);

	return DataSource_Immediate32;
}

void Context::register_rename(std::list<Operation*>::iterator _start, int _regold, int _regnew)
{
	if (_regold == _regnew)
		return;

	Operation* poper = *_start;

	// both 16 and 32bit users of a register will tag the low word so we use this to remap
	int tagold = -1;
	if (_regold == poper->sreg)
		tagold = poper->stagL;
	else
	if (_regold == poper->dreg)
		tagold = poper->dtagL;

	if (tagold < 0)
		return;

	for (std::list<Operation*>::iterator it = operations.begin(); it != operations.end(); it++)
	{
		Operation *poper = *it;

		if (poper->stagL == tagold)
		{
			printf("replacing S:%s with %s\n", registers[poper->sreg].name.c_str(), registers[_regnew].name.c_str());
			poper->sreg = _regnew;
		}

		if (poper->dtagL == tagold)
		{
			printf("replacing D:%s with %s\n", registers[poper->dreg].name.c_str(), registers[_regnew].name.c_str());
			poper->dreg = _regnew;
		}
	}
}

void Context::register_sweep()
{
	std::vector<std::pair<int,int> > tags;
	tags.resize(registers.size(), std::pair<int,int>(-1,-1));
	int newtag = 0;

	for (std::list<Operation*>::reverse_iterator rit = operations.rbegin(); rit != operations.rend(); rit++)
	{
		Operation *poper = *rit;

		int replacedL = -1;
		int replacedH = -1;

		// normal source
		if (poper->sreg >= 0)
		{
			// if not yet tagged - generate new tag
			std::pair<int,int> &optag = tags[poper->sreg];

			if (optag.first < 0)
			{
				optag.first = newtag++;
				if (poper->opsize == OpSize_32)
					optag.second = optag.first;
			}

			poper->stagH = optag.second;
			poper->stagL = optag.first;

			//printf("referenced: %s (%d:%d)\n", 
			//	registers[poper->sreg].name.c_str(), 
			//	poper->stagH, 
			//	poper->stagL
			//	);
		}

		// destination as source
		if ((poper->dreg >= 0) && 
			((poper->opwr == OpWrite_Modify) || 
			(poper->opwr == OpWrite_Combine))
			)
		{
			// if not yet tagged - generate new tag
			std::pair<int,int> &optag = tags[poper->dreg];

			if (optag.first < 0)
			{
				optag.first = newtag++;
				if (poper->opsize == OpSize_32)
					optag.second = optag.first;
			}

			poper->dtagH = optag.second;
			poper->dtagL = optag.first;

			//printf("modified: %s (%d:%d)\n", 
			//	registers[poper->dreg].name.c_str(), 
			//	poper->dtagH, 
			//	poper->dtagL
			//	);
		}

		// destination replaced (rename)
		if ((poper->dreg >= 0) && 
			((poper->opwr == OpWrite_Replace) || 
			(poper->opwr == OpWrite_ReplaceHCombineL))
			)
		{
			// if not yet tagged - generate new tag
			std::pair<int,int> &optag = tags[poper->dreg];

			if (optag.first < 0)
			{
				optag.first = newtag++;
				if (poper->opsize == OpSize_32)
					optag.second = optag.first;
			}

			poper->dtagH = optag.second;
			poper->dtagL = optag.first;

			//printf("replaced: %s (%d:%d)\n", 
			//	registers[poper->dreg].name.c_str(), 
			//	poper->dtagH, 
			//	poper->dtagL
			//	);

			// lower word only replaced if a simple replace op (e.g. not a sign-extending combine, where only upper word is replaced)
			if (poper->opwr != OpWrite_ReplaceHCombineL)
				replacedL = poper->dreg;

			if (poper->opsize == OpSize_32)
				replacedH = poper->dreg;
		}

		int r = 0;
		for (std::vector<std::pair<int,int> >::iterator it = tags.begin(); it != tags.end(); it++, r++)
		{
			std::pair<int,int> &tag = *it;

			// save full tag pattern for this state in operation to help with renaming
			poper->alltagsH[r] = tag.second;
			poper->alltagsL[r] = tag.first;

			//if (tag.first >= 0)
			//	printf("%d ", tag.first);
			//else
			//	printf(". ", tag.first);
		}

		if (replacedL >= 0)
			tags[replacedL].first = -1;
		if (replacedH >= 0)
			tags[replacedH].second = -1;

		//printf("\n");
	}
}

bool Context::tag_collision(std::list<Operation*>::iterator _start, int _rold, int _rnew)
{
	if (_rnew == _rold)
		return false;

	// detect lifespan tag conflicts between two registers
	for (std::list<Operation*>::iterator it = _start; it != operations.end(); it++)
	{
		Operation *poper = *it;

		if (poper->alltagsL[_rold] >= 0)
		{
			if (poper->alltagsL[_rnew] >= 0)
				return true;
		}
	}

	return false;
}

#define USE_XFORM_ADDSUBQ (1)

void Context::peephole1()
{
	reset_sim(true);

	register_sweep();

	printf("peephole pass #1: replace word #immediate register loads with derived register values\n");

	for (std::list<Operation*>::iterator it = operations.begin(); it != operations.end();)
	{
		std::list<Operation*>::iterator curr = it++;
		Operation *pop = *curr;

		if (pop->kind == OpKind_LoadI16)
		{
			DR_LoadI16 *pactual = reinterpret_cast<DR_LoadI16*>(pop);
			int rold = pactual->dreg;

			// OPPORTUNITY: (CLR.W Rn == EXPECTED)
			if ((pactual->data == 0) && 
				(registers[rold].regclass == RegClass_Data))
			{
				printf("transform: move.w #$0000,%s -> clr.w %s [$%04x]\n", 
					registers[rold].name.c_str(),
					registers[rold].name.c_str(),
					registers[rold].L
				);
				// replace operation with equivalent CLR.W
				curr = operations.erase(curr);
				curr = operations.insert(curr, new DR_Clr16(*this, rold));
				goto terminate;
			}

			// now look for transforms of free register values
//			if (!(registers[pactual->dreg].allocatedH))
			{
				for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++)
				{
					int rnew = it->index;

					// can't reassign to a register with a pre-existing lifespan which overlaps with the lifespan of the current job
					// note: register can't collide with itself
					if (tag_collision(curr, rold, rnew))
						continue;

					// avoid transforming constant registers
					if (!(it->constant))
					{
						// OPPORTUNITY: (NOT.W Rn == EXPECTED)
						if ((pactual->data == ~(it->L)) && 
							(registers[rnew].regclass == RegClass_Data))
						{
							printf("transform: move.w #$%04x,%s -> not.w %s [~$%04x]\n", 
								pactual->data, 
								registers[rold].name.c_str(),
								registers[rnew].name.c_str(),
								registers[rnew].L
							);
							register_rename(curr, rold, rnew);
							curr = operations.erase(curr);
							curr = operations.insert(curr, new DR_Not16(*this, rnew));
							register_sweep();
							goto terminate;
						}

						// OPPORTUNITY: (NEG.W Rn == EXPECTED)
						if ((pactual->data == uword_t(-(word_t(it->L)))) && 
							(registers[rnew].regclass == RegClass_Data))
						{
							printf("transform: move.w #$%04x,%s -> neg.w %s [-$%04x]\n", 
								pactual->data, 
								registers[rold].name.c_str(),
								registers[rnew].name.c_str(),
								registers[rnew].L
							);
							register_rename(curr, rold, rnew);
							curr = operations.erase(curr);
							curr = operations.insert(curr, new DR_Neg16(*this, rnew));
							register_sweep();
							goto terminate;
						}

						// OPPORTUNITY: (ST Rn == EXPECTED)
						if ((pactual->data == (it->L | 0x00FF)) && 
							(registers[rnew].regclass == RegClass_Data))
						{
							printf("transform: move.w #$%04x,%s -> st %s [$%04x|$00FF]\n", 
								pactual->data, 
								registers[rold].name.c_str(),
								registers[rnew].name.c_str(),
								registers[rnew].L
							);
							register_rename(curr, rold, rnew);
							curr = operations.erase(curr);
							curr = operations.insert(curr, new DR_SccR16(*this, true, rnew));
							register_sweep();
							goto terminate;
						}
	
						// OPPORTUNITY: (SF Rn == EXPECTED)
						if ((pactual->data == (it->L & 0xFF00)) && 
							(registers[rnew].regclass == RegClass_Data))
						{
							printf("transform: move.w #$%04x,%s -> sf %s [$%04x&$FF00]\n", 
								pactual->data, 
								registers[rold].name.c_str(),
								registers[rnew].name.c_str(),
								registers[rnew].L
							);
							register_rename(curr, rold, rnew);
							curr = operations.erase(curr);
							curr = operations.insert(curr, new DR_SccR16(*this, false, rnew));
							register_sweep();
							goto terminate;
						}

						// OPPORTUNITY: (TAS.B Rn == EXPECTED)
						if ((pactual->data == (it->L | 0x0080)) && 
							(registers[rnew].regclass == RegClass_Data))
						{
							printf("transform: move.w #$%04x,%s -> tas.b %s [$%04x|$0080]\n", 
								pactual->data, 
								registers[rold].name.c_str(),
								registers[rnew].name.c_str(),
								registers[rnew].L
							);
							register_rename(curr, rold, rnew);
							curr = operations.erase(curr);
							curr = operations.insert(curr, new DR_TasR16(*this, rnew));
							register_sweep();
							goto terminate;
						}

						// OPPORTUNITY: (SWAP Rx == EXPECTED)
						// note: looking to replace a 16bit load, but SWAP is a 32bit operation
						{
							if ((pactual->data == registers[rnew].H) && 
								(registers[rnew].regclass == RegClass_Data))
							{
								printf("transform: move.w #$%04x,%s -> swap %s [$%04x<:>$%04x]\n",
									pactual->data,
									registers[rold].name.c_str(),
									registers[rnew].name.c_str(),
									registers[rnew].H,
									registers[rnew].L
									);

								register_rename(curr, rold, rnew);
								curr = operations.erase(curr);
								curr = operations.insert(curr, new DR_SwapR32(*this, rnew));
								register_sweep();
								goto terminate;
							}
						}

						// now check sets and ranges (more expensive to find)
						{
#if (USE_XFORM_ADDSUBQ)
							// OPPORTUNITY: (ADDQ/SUBQ.W #IMM,Rn == EXPECTED)
							for (word_t i = -8; i <= 8; i++)
							{
								// note: may have side effects on upper word of address register destinations
								if (pactual->data == uword_t(i + word_t(it->L)))
								{
									if (i > 0)
									{
										printf("transform: move.w #$%04x,%s -> addq.w #%d,%s [$%04x+%d]\n",
											pactual->data, 
											registers[rold].name.c_str(),
											i, 
											registers[rnew].name.c_str(),
											registers[rnew].L, 
											i);
									}
									else
									{
										printf("transform: move.w #$%04x,%s -> subq.w #%d,%s [$%04x-%d]\n",
											pactual->data, 
											registers[rold].name.c_str(),
											-i, 
											registers[rnew].name.c_str(),
											registers[rnew].L, 
											-i);
									}
									register_rename(curr, rold, rnew);
									curr = operations.erase(curr);
									curr = operations.insert(curr, new DR_AddSubQ16(*this, i, rnew));
									register_sweep();
									goto terminate;
								}
							}
#endif //USE_XFORM_ADDSUBQ
							// now check all register-register transformations
							for (int rs = 0; rs < registers.size(); rs++)
							{
//								if (rs == rnew) 
//									continue;

								// OPPORTUNITY: (ADD.W Rx,Ry == EXPECTED)
								// note: may have side effects on upper word of address register destinations
								if (word_t(pactual->data) == word_t(word_t(registers[rnew].L) + word_t(registers[rs].L)))
								{
									printf("transform: move.w #$%04x,%s -> add.w %s,%s [$%04x+$%04x]\n",
										pactual->data,
										registers[rold].name.c_str(),
										registers[rs].name.c_str(),
										registers[rnew].name.c_str(),
										registers[rnew].L,
										registers[rs].L
										);

									register_rename(curr, rold, rnew);
									curr = operations.erase(curr);
									curr = operations.insert(curr, new DR_AddR16(*this, rs, rnew));
									register_sweep();
									goto terminate;
								}

								// OPPORTUNITY: (SUB.W Rx,Ry == EXPECTED)
								// note: avoid carry from upper word of address register destinations by filtering them out
								if ((word_t(pactual->data) == word_t(word_t(registers[rnew].L) - word_t(registers[rs].L))) &&
									(registers[rnew].regclass == RegClass_Data))
								{
									printf("transform: move.w #$%04x,%s -> sub.w %s,%s [$%04x-$%04x]\n",
										pactual->data,
										registers[rold].name.c_str(),
										registers[rs].name.c_str(),
										registers[rnew].name.c_str(),
										registers[rnew].L,
										registers[rs].L
										);

									register_rename(curr, rold, rnew);
									curr = operations.erase(curr);
									curr = operations.insert(curr, new DR_SubR16(*this, rs, rnew));
									register_sweep();
									goto terminate;
								}

								// OPPORTUNITY: (AND.W Rx,Ry == EXPECTED)
								if ((pactual->data == (registers[rnew].L & registers[rs].L)) &&
									(registers[rnew].regclass == RegClass_Data) &&
									(registers[rs].regclass == RegClass_Data)
									)
								{
									printf("transform: move.w #$%04x,%s -> and.w %s,%s [$%04x&$%04x]\n",
										pactual->data,
										registers[rold].name.c_str(),
										registers[rs].name.c_str(),
										registers[rnew].name.c_str(),
										registers[rnew].L,
										registers[rs].L
										);

									register_rename(curr, rold, rnew);
									curr = operations.erase(curr);
									curr = operations.insert(curr, new DR_AndR16(*this, rs, rnew));
									register_sweep();
									goto terminate;
								}

								// OPPORTUNITY: (OR.W Rx,Ry == EXPECTED)
								if ((pactual->data == (registers[rnew].L | registers[rs].L)) &&
									(registers[rnew].regclass == RegClass_Data) &&
									(registers[rs].regclass == RegClass_Data)
									)
								{
									printf("transform: move.w #$%04x,%s -> or.w %s,%s [$%04x|$%04x]\n",
										pactual->data,
										registers[rold].name.c_str(),
										registers[rs].name.c_str(),
										registers[rnew].name.c_str(),
										registers[rnew].L,
										registers[rs].L
										);

									register_rename(curr, rold, rnew);
									curr = operations.erase(curr);
									curr = operations.insert(curr, new DR_OrR16(*this, rs, rnew));
									register_sweep();
									goto terminate;
								}

								// OPPORTUNITY: (EOR.W Rx,Ry == EXPECTED)
								if ((pactual->data == (registers[rnew].L ^ registers[rs].L)) &&
									(registers[rnew].regclass == RegClass_Data) &&
									(registers[rs].regclass == RegClass_Data)
									)
								{
									printf("transform: move.w #$%04x,%s -> eor.w %s,%s [$%04x^$%04x]\n",
										pactual->data,
										registers[rold].name.c_str(),
										registers[rs].name.c_str(),
										registers[rnew].name.c_str(),
										registers[rnew].L,
										registers[rs].L
										);

									register_rename(curr, rold, rnew);
									curr = operations.erase(curr);
									curr = operations.insert(curr, new DR_XorR16(*this, rs, rnew));
									register_sweep();
									goto terminate;
								}

								// OPPORTUNITY: (BCHG Rx,Ry == EXPECTED)
								// note: looking to replace a 16bit load, but BCHG is a 32bit operation so we filter a bit
								if (
									((registers[rs].L & 31) < 16) && // filter out side effects on H word
									(pactual->data == (registers[rnew].L ^ (1 << (registers[rs].L & 31)))) &&
									(registers[rnew].regclass == RegClass_Data) &&
									(registers[rs].regclass == RegClass_Data)
									)
								{
									printf("transform: move.w #$%04x,%s -> bchg %s,%s [$%04x^$%04x]\n",
										pactual->data,
										registers[rold].name.c_str(),
										registers[rs].name.c_str(),
										registers[rnew].name.c_str(),
										registers[rnew].L,
										(1 << (registers[rs].L & 31))
										);

									register_rename(curr, rold, rnew);
									curr = operations.erase(curr);
									// note: we know this bit is in the lower word, so we can specify word opsize
									curr = operations.insert(curr, new DR_BchgR32(*this, OpSize_16, rs, rnew));
									register_sweep();
									goto terminate;
								}

								// OPPORTUNITY: (MULU.W Rx,Ry == EXPECTED)
								// note: looking to replace a 16bit load, but MULU.W is a 32bit operation with unavoidable destruction of upper word
								{
									ulong_t result = registers[rnew].L * registers[rs].L;
									if (
										(pactual->data == (result & 0x0000FFFF)) &&
										(registers[rnew].regclass == RegClass_Data) &&
										(registers[rs].regclass == RegClass_Data)
										)
									{
										printf("transform: move.w #$%04x,%s -> mulu.w %s,%s [$%04x*$%04x]\n",
											pactual->data,
											registers[rold].name.c_str(),
											registers[rs].name.c_str(),
											registers[rnew].name.c_str(),
											registers[rnew].L,
											registers[rs].L
											);

										register_rename(curr, rold, rnew);
										curr = operations.erase(curr);
										curr = operations.insert(curr, new DR_MuluR16(*this, rs, rnew));
										register_sweep();
										goto terminate;
									}
								}

								// OPPORTUNITY: (MULS.W Rx,Ry == EXPECTED)
								// note: looking to replace a 16bit load, but MULS.W is a 32bit operation with unavoidable destruction of upper word
								{
									long_t result = word_t(registers[rnew].L) * word_t(registers[rs].L);
									if (
										(pactual->data == uword_t(result & 0x0000FFFF)) &&
										(registers[rnew].regclass == RegClass_Data) &&
										(registers[rs].regclass == RegClass_Data)
										)
									{
										printf("transform: move.w #$%04x,%s -> muls.w %s,%s [$%04x*$%04x]\n",
											pactual->data,
											registers[rold].name.c_str(),
											registers[rs].name.c_str(),
											registers[rnew].name.c_str(),
											registers[rnew].L,
											registers[rs].L
											);

										register_rename(curr, rold, rnew);
										curr = operations.erase(curr);
										curr = operations.insert(curr, new DR_MulsR16(*this, rs, rnew));
										register_sweep();
										goto terminate;
									}
								}
							}
						}

						// OPPORTUNITY: (LSL/LSR/ASL/ASR.W #N,Rx == EXPECTED)
						if (registers[rnew].regclass == RegClass_Data)
						{
							for (word_t i = 1; i <= 8; i++)
							{
								if (pactual->data == (it->L << i))
								{
									printf("transform: move.w #$%04x,%s -> lsl.w #%d,%s [$%04x<<%d]\n",
										pactual->data,
										registers[rold].name.c_str(),
										i,
										registers[rnew].name.c_str(),
										registers[rnew].L,
										i);

									register_rename(curr, rold, rnew);
									curr = operations.erase(curr);
									curr = operations.insert(curr, new DR_LSI16(*this, -i, rnew));
									register_sweep();
									goto terminate;
								}

								if (pactual->data == (it->L >> i))
								{
									printf("transform: move.w #$%04x,%s -> lsr.w #%d,%s [$%04x>>%d]\n",
										pactual->data,
										registers[rold].name.c_str(),
										i,
										registers[rnew].name.c_str(),
										registers[rnew].L,
										i);

									register_rename(curr, rold, rnew);
									curr = operations.erase(curr);
									curr = operations.insert(curr, new DR_LSI16(*this, i, rnew));
									register_sweep();
									goto terminate;
								}

								if (word_t(pactual->data) == word_t(word_t(it->L) << i))
								{
									printf("transform: move.w #$%04x,%s -> asl.w #%d,%s [$%04x<<%d]\n",
										pactual->data,
										registers[rold].name.c_str(),
										i,
										registers[rnew].name.c_str(),
										registers[rnew].L,
										i);

									register_rename(curr, rold, rnew);
									curr = operations.erase(curr);
									curr = operations.insert(curr, new DR_ASI16(*this, -i, rnew));
									register_sweep();
									goto terminate;
								}

								if (word_t(pactual->data) == word_t(word_t(it->L) >> i))
								{
									printf("transform: move.w #$%04x,%s -> asr.w #%d,%s [$%04x>>%d]\n",
										pactual->data,
										registers[rold].name.c_str(),
										i,
										registers[rnew].name.c_str(),
										registers[rnew].L,
										i);

									register_rename(curr, rold, rnew);
									curr = operations.erase(curr);
									curr = operations.insert(curr, new DR_ASI16(*this, i, rnew));
									register_sweep();
									goto terminate;
								}

								if (pactual->data == _rotl16(it->L, i))
								{
									printf("transform: move.w #$%04x,%s -> rol.w #%d,%s [$%04x<-<%d]\n",
										pactual->data,
										registers[rold].name.c_str(),
										i,
										registers[rnew].name.c_str(),
										registers[rnew].L,
										i);

									register_rename(curr, rold, rnew);
									curr = operations.erase(curr);
									curr = operations.insert(curr, new DR_ROI16(*this, -i, rnew));
									register_sweep();
									goto terminate;
								}

								if (pactual->data == _rotr16(it->L, i))
								{
									printf("transform: move.w #$%04x,%s -> ror.w #%d,%s [$%04x>->%d]\n",
										pactual->data,
										registers[rold].name.c_str(),
										i,
										registers[rnew].name.c_str(),
										registers[rnew].L,
										i);

									register_rename(curr, rold, rnew);
									curr = operations.erase(curr);
									curr = operations.insert(curr, new DR_ROI16(*this, i, rnew));
									register_sweep();
									goto terminate;
								}
							}
						}
					}
				}
			}
		}

//		goto no_changes;

	terminate:
		// register usage may have changed - changed
//		register_sweep();

//no_changes:
		(*curr)->execute();
	}
}


void Context::peephole2()
{
	reset_sim(true);

	register_sweep();

	printf("peephole cleanup #1: replace word #immediate EM stores with modified register values\n");

	for (std::list<Operation*>::iterator it = operations.begin(); it != operations.end();)
	{
		std::list<Operation*>::iterator curr = it++;
		Operation *pop = *curr;

		if (pop->kind == OpKind_StoreI16)
		{
			EM_MoveI16 *pactual = reinterpret_cast<EM_MoveI16*>(pop);
			int emold = pactual->em;

			// now look for transforms of free register values
//			if (!(registers[pactual->dreg].allocatedH))
			{
				for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++)
				{
					int rnew = it->index;

					if (registers[rnew].stableL &&
						(pactual->data == (it->L)))
					{
						printf("transform: move.w #$%04x,%s -> move.w %s,%s [$%04x]\n", 
							pactual->data, 
							BLiTEM_names[emold].c_str(),
							registers[rnew].name.c_str(),
							BLiTEM_names[emold].c_str(),
							registers[rnew].L
						);
						curr = operations.erase(curr);
						curr = operations.insert(curr, new EM_MoveR16(*this, emold, rnew));
						register_sweep();
						goto terminate;
					}
				}
			}
		}

//		goto no_changes;

	terminate:
		// register usage may have changed - changed
//		register_sweep();

//no_changes:
		(*curr)->execute();
	}
}

void generate_emx_frame(spriteframe_t &spr)
{
	printf("frame:\n");

	// generate metrics for each data word
	int pi = 0;
	preshiftlist_t::iterator pit = spr.psmask_frames_.begin();

//	pit++; pi++;

//	preshiftlist_t::iterator pit = spr.psmask_frames_.end();
//	pit--;

	for (; pit != spr.psmask_frames_.end(); pit++, pi++)
	{
		preshift_t &shift = *pit;

		Context context(shift.wordwidth_, shift.height_);

		printf("\n\npreshift %d preview:\n", pi);

		// output the source image pattern
		for (int v = 0; v < shift.height_; v++)
		{
			for (int h = 0; h < shift.wordwidth_; h++)
			{
				print_dataword(shift.data_[(v * shift.wordwidth_) + h]);
			}
			printf("\n");
		}
		printf("\n");

		byte_t *changes = new byte_t[shift.planewords_ * shift.height_];
		memset(changes, 0, shift.planewords_ * shift.height_);

		// make copy of data in terms of 3*y EM words
		uword_t *buffer = new uword_t[3 * shift.height_];
		memset(buffer, 0, 2 * 3 * shift.height_);
		{
			for (int v = 0; v < shift.height_; v++)
			{
				buffer[(v * 3) + 0] = shift.data_[(v * shift.wordwidth_) + 0];

				if (shift.wordwidth_ == 2)
				{
					// note: em1,em3 used in 2-wide blits. em2 is skipped
					buffer[(v * 3) + 2] = shift.data_[(v * shift.wordwidth_) + 1];
				}
				else
				if (shift.wordwidth_ == 3)
				{
					buffer[(v * 3) + 1] = shift.data_[(v * shift.wordwidth_) + 1];
					buffer[(v * 3) + 2] = shift.data_[(v * shift.wordwidth_) + 2];
				}

				//for (int h = 0; h < shift.wordwidth_ && h < 3; h++)
				//{
				//	buffer[(v * 3) + h] = shift.data_[(v * shift.wordwidth_) + h];
				//}
			}
		}

		std::vector<int> best_pattern;
		for (int f = 0; f < shift.height_; f++)
		{
			best_pattern.push_back(f);
		}

		if (USE_LINEORDER_OPTIMIZATION)
		{
			printf("optimizing line order...\n");

			// optimize the line order to minimize changes to EM registers
			{
				mersenne_twister twist;

				// begin with default line pattern (top-to-bottom)
				int best_cost = 1 << 20;
				int best_loadcost = best_cost;
				int last_improvement = 0;

				// optimize line pattern by reordering
				for (int trial = 0; trial < (1 << DARWIN_DEPTH); trial++)
				{
					std::vector<int> trial_pattern(best_pattern);

					// select mutation rate
					int mutrate = ((twist.genS32() >> 4) % 6) + 1;

					// mutate
					for (int mut = 0; mut < mutrate; mut++)
					{
						// select pair to exchange
						int choice1 = (twist.genS32() >> 4) % trial_pattern.size();
						int choice2 = (twist.genS32() >> 4) % trial_pattern.size();
						while (choice1 == choice2)
							choice2 = (twist.genS32() >> 4) % trial_pattern.size();

						// reorder
						int tmp = trial_pattern[choice1];
						trial_pattern[choice1] = trial_pattern[choice2];
						trial_pattern[choice2] = tmp;
					}

					// estimate efficiency of new pattern in terms of deltas per line
					{
						int totalcost = 0;
						int loadcost = 0;

						for (int v = 0; v < (shift.height_ - 1); v++)
						{
							int v0 = trial_pattern[v + 0];
							int v1 = trial_pattern[v + 1];
							int yp = v0 * 3;
							int yn = v1 * 3;
							int lc1 = (buffer[yp + 0] != buffer[yn + 0]) ? 1 : 0;
							int lc2 = (buffer[yp + 1] != buffer[yn + 1]) ? 1 : 0;
							int lc3 = (buffer[yp + 2] != buffer[yn + 2]) ? 1 : 0;

							totalcost += (lc1 + lc2 + lc3) << 8;
							loadcost += lc1 + lc2 + lc3;

							// attempt to keep lines roughly in top-bottom order by including small cost for 'pointless' reordering
							// this provides more opportunity to use a fixed increment between lines (save space with register add vs lea)
							// notes: 
							// - some chance this could prevent a difficult-to-reach optimization via small incremental mutations
							// - perhaps a bit more likely to result in extra LEA at start, to reach first block of contiguous lines 

							// cost is same for forward or reverse ordering - delta value will be the same even if instruction isn't
							int dycost = (v1 - v0);
							if (dycost < 0)
								dycost = -dycost;
							if (dycost > 255)
								dycost = 255;

							totalcost += dycost;
						}

						if (totalcost < best_cost)
						{
							best_cost = totalcost;
							best_loadcost = loadcost;
							best_pattern = trial_pattern;
							last_improvement = trial;
						}
					}
				}

				printf("\nconverged or exhausted at %d trials @ loadcost=%d (totalcost=%d)\n", last_improvement, best_cost, best_loadcost);

				// output the best pattern
				for (int v = 0; v < shift.height_; v++)
				{
					int y = best_pattern[v];
					print_dataword(buffer[(y * 3) + 0]);
					print_dataword(buffer[(y * 3) + 1]);
					print_dataword(buffer[(y * 3) + 2]);
					printf("\n");
				}
			}
		}


		// generate map for only changing EM registers 
		// (excluding first output line - this is handled as dirty registers to minimize initial setup if the first line is empty)

		// mark first line as dirty
		//int y = best_pattern[0] * 3;
		//changes[y + 0] = 1;
		//if (shift.wordwidth_ >= 2)
		//	changes[y + 2] = 1;
		//if (shift.wordwidth_ == 3)
		//	changes[y + 1] = 1;

		if (USE_DELTA_OPTIMIZATION)
		{
			for (int v = 0; v < (shift.height_ - 1); v++)
			{
				int yp = best_pattern[v + 0] * 3;
				int yn = best_pattern[v + 1] * 3;
				changes[yn + 0] = (buffer[yp + 0] != buffer[yn + 0]) ? 1 : 0;
				if (shift.wordwidth_ == 2)
				{
					changes[yn + 2] = (buffer[yp + 2] != buffer[yn + 2]) ? 1 : 0;
				}
				else
				if (shift.wordwidth_ == 3)
				{
					changes[yn + 1] = (buffer[yp + 1] != buffer[yn + 1]) ? 1 : 0;
					changes[yn + 2] = (buffer[yp + 2] != buffer[yn + 2]) ? 1 : 0;
				}
			}
		}
		else
		{
			for (int v = 1; v < shift.height_; v++)
			{
				int y = best_pattern[v] * 3;

				changes[y + 0] = true;
				changes[y + 1] = (shift.wordwidth_ > 2);
				changes[y + 2] = (shift.wordwidth_ > 1);
			}
		}

		bool em_dirty[3];

		printf("deltas...\n");

		// reset dirty EM registers for this blit width
		em_dirty[0] = true;
		em_dirty[1] = (shift.wordwidth_ > 2);
		em_dirty[2] = (shift.wordwidth_ > 1);

		for (int v = 0; v < shift.height_; v++)
		{
			int y = best_pattern[v] * 3;

			if (changes[y + 0] || em_dirty[0])
			{
				em_dirty[0] = false;
				print_dataword(buffer[y + 0]);
			}
			else
				printf("................");

			if (changes[y + 1] || em_dirty[1])
			{
				em_dirty[1] = false;
				print_dataword(buffer[y + 1]);
			}
			else
				printf("................");

			if (changes[y + 2] || em_dirty[2])
			{
				em_dirty[2] = false;
				print_dataword(buffer[y + 2]);
			}
			else
				printf("................");

			printf("\n");
		}

		class lineout
		{
		public:

			lineout(int s, int d)
				: sline(s), dline(d)
			{
			}

			int sline;
			int dline;
		};

		std::vector<lineout> active_lines;

		printf("surviving datawords:\n");

		// reset dirty EM registers for this blit width
		em_dirty[0] = true;
		em_dirty[1] = (shift.wordwidth_ > 2);
		em_dirty[2] = (shift.wordwidth_ > 1);

//		int unique_datawords = 0; // each unique dataword has a uid so it can be reused
		{
			linemap lines;
			for (int y = 0; y < shift.height_; y++)
			{
				uword_t *srcdata = &buffer[best_pattern[y] * 3];
				byte_t *linechanges = &changes[best_pattern[y] * 3];

				// identify duplicate lines in terms of complete EM state
				u64 line = 0;
				for (int x = 0; x < 3; x++)
				{
					line <<= 16;
					line |= srcdata[x];
				}

				// generate metrics for nonempty, unique lines only - lines where 1 or more EM registers must change
				if ((em_dirty[0] || em_dirty[1] || em_dirty[2]) || 
					(line && (lines.find(line) == lines.end()))
				)
				{

					// mark this line configuration as encountered
					if (USE_LINEDUPE_OPTIMIZATION)
						lines[line] = 1;

					// gather metrics for datawords on this line
					bool changed = false;
					for (int x = 0; x < 3; x++)
					{
						if (!linechanges[x] && !em_dirty[x])
						{
							printf("----------------");
							continue;
						}

						changed = true;

						uword_t dataword = srcdata[x];
						emx_metric_t &m = context.metrics[dataword];

						print_dataword(dataword);

						//if (m.name < 0)
						//	m.name = unique_datawords++;

						// incidence of this word
						m.incidence++;
						m.references++;

						//// incidence at each EM position
						//if (x == 0)
						//	m.incidence_em0++;
						//else
						//	if (x == 1)
						//		m.incidence_em1++;
						//	else
						//		if (x == 2)
						//			m.incidence_em2++;

						// incidence of this word relative to other words
						for (int lna = 0; lna < 3; lna++)
						{
							if ((x != lna) && (linechanges[lna]||em_dirty[lna]))
							{
								uword_t lnaword = srcdata[lna];

								// relative to all other words referenced on same line
								m.line_adjacencies[lnaword]++;

								// relative to other words used immediately to the right
								if (x == lna - 1)
									m.left_adjacencies[lnaword]++;
							}
						}
					}

					// track this as a live line which changes
					if (changed)
						active_lines.push_back(lineout(y, best_pattern[y]));

					// all EM words on this line have been dealt with one way or another
					em_dirty[0] = em_dirty[1] = em_dirty[2] = false;

					printf("\n");
				}
			}
		}

		printf("\nscoring:\n");

		// globally score each data word according to metrics
		int assigned_registers = 0;
		for (metricmap::iterator mit = context.metrics.begin(); mit != context.metrics.end(); mit++)
		{
			const uword_t &dataword = mit->first;

			print_dataword(dataword);

			emx_metric_t &m = mit->second;

			if ((m.name < 0) && (m.incidence > 1))
			{
				m.name = assigned_registers++;
			}

			float fscore = log(1.0 + (float)m.incidence);

			float sharing = 0.0f;
			for (relationtable::iterator rit = m.line_adjacencies.begin(); rit != m.line_adjacencies.end(); rit++)
				sharing += (float)rit->second;

			// score never modulates to 0 - can only increase with sharing
			fscore *= (1.0f + log(1.0 + sharing));

			m.fincidence_score = fscore;

			printf(" incidence_score: %.2f\n", fscore);
		}

		// globally score each data word according to its left-adjacency (upper word opportuntiy)
		for (metricmap::iterator mit = context.metrics.begin(); mit != context.metrics.end(); mit++)
		{
			const uword_t &dataword = mit->first;

			print_dataword(dataword);

			emx_metric_t &m = mit->second;

			float fscore = m.fincidence_score;

			float fadj = 0.0f;
			for (relationtable::iterator rit = m.left_adjacencies.begin(); rit != m.left_adjacencies.end(); rit++)
				fadj += (float)rit->second;

			// score can modulate to 0 if this dataword never appears on the left of any other
			fscore *= (0.0f + log(1.0 + fadj));

			m.fadjacency_score = fscore;

			printf(" adjacency_score: %.2f\n", fscore);
		}

		// find the most useful values

		typedef std::map<float, std::list<metricmap::iterator> > usemap;

		usemap usemap_lw;
		usemap usemap_hw;

		for (metricmap::iterator mit = context.metrics.begin(); mit != context.metrics.end(); mit++)
		{
			usemap_lw[mit->second.fincidence_score].push_back(mit);
			usemap_hw[mit->second.fadjacency_score].push_back(mit);
		}

		for (usemap::reverse_iterator uit = usemap_lw.rbegin(); uit != usemap_lw.rend(); uit++)
		{
			for (std::list<metricmap::iterator>::iterator lit = uit->second.begin(); lit != uit->second.end(); lit++)
			{
				const uword_t &dataword = (*lit)->first;

				emx_metric_t &m = context.metrics[dataword];

				if (m.incidence >= 2)
				{
					printf("L preload: [$%04x] iscore: %.2f\n", dataword, uit->first);
					context.preloads.push_back(preload(dataword));
				}
			}
		}

		for (usemap::reverse_iterator uit = usemap_hw.rbegin(); uit != usemap_hw.rend(); uit++)
		{
			float h_score = uit->first;

			for (std::list<metricmap::iterator>::iterator lit = uit->second.begin(); lit != uit->second.end(); lit++)
			{
				const uword_t &dataword = (*lit)->first;

				// metrics for this word relative to others
				emx_metric_t &m = context.metrics[dataword];

//				if (m.incidence > 1)
				{
					printf("H preload: [$%04x] ascore: %.2f\n", dataword, h_score);


					// sort adjacencies with indirection

					typedef std::map<int, std::list<relationtable::iterator> > hitmap;
					hitmap hits;

					for (relationtable::iterator relit = m.left_adjacencies.begin(); relit != m.left_adjacencies.end(); relit++)
					{
						int h = relit->second;
						hits[h].push_back(relit);
					}

					// scan through the most promising adjacents
					for (hitmap::reverse_iterator hit = hits.rbegin(); hit != hits.rend(); hit++)
					{
						std::list<relationtable::iterator> &competing = hit->second;

						while (!competing.empty())
						{
							relationtable::iterator relit = competing.back();
							competing.pop_back();

							uword_t adjacent_low = relit->first;

							printf("candidate adjacent low: [$%04x] hits: %d\n", adjacent_low, relit->second);

							// try to locate it in the existing pool of L preloads
							// if we find a match, bind it and break

							for (std::vector<preload>::iterator pit = context.preloads.begin(); pit != context.preloads.end(); pit++)
							{
								if (pit->validL && (pit->L == adjacent_low))
								{
									pit->H = dataword;
									pit->validH = true;

									printf("bound preload [$%04x]:[$%04x]\n", dataword, adjacent_low);

									goto break_bind;
								}
							}
						}
					}
				break_bind:;
				}
			}
		}


		// preload most useful values

		//context

		// data dependencies
		// process all *changing words* on *changing lines* in *optimized line order*

		class storedependency
		{
		public:
			storedependency(int _name, int _pos, int _job, int _type, uword_t _H, uword_t _L)
				: name(_name), pos(_pos), job(_job), type(_type), H(_H), L(_L)
			{
			}

			int name;
			int pos;
			int job;
			int type;
			uword_t H;
			uword_t L;
		};

		std::vector<storedependency> deps;
		std::vector<u64> lifespans;

		// todo: determine max regs available for final code
		int max_regs = 4;

		// 1)
		// put each piece of data in a new register in order of need, assuming an infinute number of free 
		// registers (to begin with).
		// note that in worst case, several changes may involve the same data at spaced intervals desite
		// minimization of deltas - because multiple words may change at once in any line. 
		// while such data probably can survive in a register throughout its needed lifespan, it may need 
		// to be reallocated if registers become exhausted - so we do a full register rename and reduce.

		// reset dirty EM registers for this blit width
		em_dirty[0] = true;
		em_dirty[1] = (shift.wordwidth_ > 2);
		em_dirty[2] = (shift.wordwidth_ > 1);

		std::vector<int> blits;

		int stage = 0;
//		int regcount = 0;
		u64 lifebits = 0;
		int job = 0;
		for (std::vector<lineout>::iterator lit = active_lines.begin(); lit != active_lines.end(); lit++, job++)
		{
			lineout &lo = *lit;
			int y = lo.dline;

			uword_t *srcdata = &buffer[y * 3];
			byte_t *linechanges = &changes[y * 3];

			blits.push_back(deps.size());

			for (int x = 0; x < 3; x++)
			{
				if (!(linechanges[x]||em_dirty[x]))
					continue;

				uword_t dataword = srcdata[x];

				emx_metric_t &m = context.metrics[dataword];

				// resource type: 0 = reused (register), 1 = single-use (#immediate/transform)
//				int regname = -1;
				int type = 1;
				if (m.incidence > 1)
				{
					type = 0;
//					regname = regcount++;
				}

				// todo: load H word with best candidate for each L data word (if any)
				deps.push_back(storedependency(m.name, stage, job, type, 0, dataword));
				if (m.name >= 0)
					lifebits |= (u64(1) << m.name);
				lifespans.push_back(lifebits);
				stage++;

				em_dirty[x] = false;
			}
		}

		// 2)
		// trim lifespans for each piece of data
		u64 cancelbits = 0;
		std::vector<u64>::reverse_iterator lit = lifespans.rbegin();
		for (std::vector<storedependency>::reverse_iterator rit = deps.rbegin(); rit != deps.rend(); rit++, lit++)
		{
			storedependency &dr = *rit;
			u64 &lifebits = *lit;

			if (dr.name >= 0)
			{
				// register name for this data
				u64 activebit = u64(1) << dr.name;

				// prevent cancelling of this data's lifespan from here backwards
				cancelbits |= activebit;
			}

			// cancel lifespans for data not yet encountered
			lifebits &= cancelbits;
		}

		printf("\n\ninitial data lifespans:\n");
		//for (std::vector<storedependency>::iterator rit = deps.begin(); rit != deps.end(); rit++)
		//{
		//	storedependency &dr = *rit;
		//	if (dr.type == 0)
		//		printf("R");
		//	else
		//	if (dr.type == 1)
		//		printf("I");
		//}
		//printf("\n");

		// report lifespan status for each state for each execution stage
		{
			std::vector<storedependency>::iterator dit = deps.begin();
			for (std::vector<u64>::iterator lit = lifespans.begin(); lit != lifespans.end(); lit++, dit++)
			{
				assert(dit != deps.end());
				print_data(*lit, /*unique_datawords*/assigned_registers); printf(" %s\n", (dit->type == 0) ? "R" : "U");
			}
		}


		// look for ways to derive unique values without loads, by transforming values with expired lifespan
			
		//for (std::vector<storedependency>::iterator dit = deps.begin(); dit != deps.end(); dit++)
		//{
		//	storedependency &dep = *dit;

		//	if (dep.type == 1)
		//	{
		//		// this dependency is isolated and expensive to load, so look for different ways to derive it from spent values
		//	}
		//}

		//for (std::vector<storedependency>::iterator dit = deps.begin(); dit != deps.end(); dit++)
		//{
		//	storedependency &dep = *dit;

		//	if (dep.type == 0)
		//	{
		//		// this value gets reused so place it in registers


		//	}
		//}


		// 3)
		// optimize assignments to fit available resources
		// - finish with no more than max_registers unique names
		// - avoid interrupting long lifespans (or lifespans of high incidence data)
		// - don't mess with immediate types
		// - minimize number of load events



		//std::vector<int>::reverse_iterator lit = lifespans.rbegin();
		//for (std::vector<dataregister>::reverse_iterator rit = regs.rbegin(); rit != regs.rend(); rit++, lit++)
		//{
		//	dataregister &dr = *rit;
		//	int &lifebits = *lit;

		//	// register name for this data
		//	int activebit = 1 << dr.name;

		//	// prevent cancelling of this data's lifespan from here backwards
		//	cancelbits |= activebit;

		//	// cancel lifespans for data not yet encountered
		//	lifebits &= cancelbits;
		//}

		context.reset_sim(true);

		// reset dirty EM registers for this blit width
		em_dirty[0] = true;
		em_dirty[1] = (shift.wordwidth_ > 2);
		em_dirty[2] = (shift.wordwidth_ > 1);

		// generate operations
		int current_line = 0;
		int reg_index = 0;
		int reg_index2 = 0;
		int reg_index3 = 0;
		{
			linemap lines;
			for (int y = 0; y < shift.height_; y++)
			{
				int dsty = best_pattern[y];

				uword_t *srcdata = &buffer[dsty * 3];
				byte_t *linechanges = &changes[dsty * 3];

				// bypass empty blits (all EMs == 0)
				u64 line = 0;
				for (int x = 0; x < 3; x++)
				{
					line <<= 16;
					line |= srcdata[x];
				}

				// is the blit going to output something?
				if (line)
				{
					// line hop +/-
					int linejump = dsty - current_line;

					// all non-empty blits require a dst address update
					// todo: linewidth should be an argument
					if (linejump)
					{
						context.operations.push_back(new DST_Lea(context, linejump));
						context.operations.back()->execute();
					}

					// check for all 3 changing
					if ((linechanges[0] || em_dirty[0]) &&
						(linechanges[1] || em_dirty[1]) &&
						(linechanges[2] || em_dirty[2]))
					{
						int sumA = 0, sumB = 0;

						// test-resolve EM1:EM2 and EM2:EM3, looking for the cheapest solution
						DataSource ds12 = context.sim_resolve_data32(srcdata[0], srcdata[1], reg_index, reg_index2, false);
						DataSource ds3 = context.sim_resolve_data16(srcdata[2], reg_index3, false);

						sumA += DataSource_costs[ds12];
						sumA += DataSource_costs[ds3];

						DataSource ds23 = context.sim_resolve_data32(srcdata[1], srcdata[2], reg_index, reg_index2, false);
						DataSource ds1 = context.sim_resolve_data16(srcdata[0], reg_index3, false);

						sumB += DataSource_costs[ds23];
						sumB += DataSource_costs[ds1];

						if (sumA < sumB)
						{
							DataSource ds12 = context.sim_resolve_data32(srcdata[0], srcdata[1], reg_index, reg_index2, true);
							if (ds12 == DataSource_RegisterHL)
							{
								context.operations.push_back(new EM_MoveR32(context, 0, reg_index));
								context.operations.back()->execute();
							}
							else
							if (ds12 == DataSource_RegisterLL)
							{
								context.operations.push_back(new EM_MoveR16(context, 0, reg_index));
								context.operations.back()->execute();
								context.operations.push_back(new EM_MoveR16(context, 1, reg_index2));
								context.operations.back()->execute();
							}
							else
							if (ds12 == DataSource_MixedIR)
							{
								context.operations.push_back(new EM_MoveI16(context, 0, srcdata[0]));
								context.operations.back()->execute();
								context.operations.push_back(new EM_MoveR16(context, 1, reg_index2));
								context.operations.back()->execute();
							}
							else
							{
								context.operations.push_back(new EM_MoveI32(context, 0, srcdata[0], srcdata[1]));
								context.operations.back()->execute();
							}

							DataSource ds3 = context.sim_resolve_data16(srcdata[2], reg_index3, true);
							if (ds3 == DataSource_RegisterL)
							{
								context.operations.push_back(new EM_MoveR16(context, 2, reg_index3));
								context.operations.back()->execute();
							}
							else
							{
								context.operations.push_back(new EM_MoveI16(context, 2, srcdata[2]));
								context.operations.back()->execute();
							}
						}
						else
						{
							DataSource ds23 = context.sim_resolve_data32(srcdata[1], srcdata[2], reg_index, reg_index2, true);
							if (ds23 == DataSource_RegisterHL)
							{
								context.operations.push_back(new EM_MoveR32(context, 1, reg_index));
								context.operations.back()->execute();
							}
							else
							if (ds23 == DataSource_RegisterLL)
							{
								context.operations.push_back(new EM_MoveR16(context, 1, reg_index));
								context.operations.back()->execute();
								context.operations.push_back(new EM_MoveR16(context, 2, reg_index2));
								context.operations.back()->execute();
							}
							else
							if (ds23 == DataSource_MixedIR)
							{
								context.operations.push_back(new EM_MoveI16(context, 1, srcdata[1]));
								context.operations.back()->execute();
								context.operations.push_back(new EM_MoveR16(context, 2, reg_index2));
								context.operations.back()->execute();
							}
							else
								context.operations.push_back(new EM_MoveI32(context, 1, srcdata[1], srcdata[2]));

							DataSource ds1 = context.sim_resolve_data16(srcdata[0], reg_index3, true);
							if (ds1 == DataSource_RegisterL)
							{
								context.operations.push_back(new EM_MoveR16(context, 0, reg_index3));
								context.operations.back()->execute();
							}
							else
							{
								context.operations.push_back(new EM_MoveI16(context, 0, srcdata[0]));
								context.operations.back()->execute();
							}
						}

						//context.sim_track_datarefs(srcdata[0]);
						//context.sim_track_datarefs(srcdata[1]);
						//context.sim_track_datarefs(srcdata[2]);

						em_dirty[0] = em_dirty[1] = em_dirty[2] = false;
					}
					else
					if ((linechanges[0] || em_dirty[0]) && 
						(linechanges[1] || em_dirty[1]))
					{
						// longword to EM1, possibly also EM3
//						context.operations.push_back(new EM_MoveI32(context, 0, srcdata[0], srcdata[1]));
						DataSource ds = context.sim_resolve_data32(srcdata[0], srcdata[1], reg_index, reg_index2, true);
						if (ds == DataSource_RegisterHL)
						{
							context.operations.push_back(new EM_MoveR32(context, 0, reg_index));
							context.operations.back()->execute();
						}
						else
						if (ds == DataSource_RegisterLL)
						{
							context.operations.push_back(new EM_MoveR16(context, 0, reg_index));
							context.operations.back()->execute();
							context.operations.push_back(new EM_MoveR16(context, 1, reg_index2));
							context.operations.back()->execute();
						}
						else
						{
							context.operations.push_back(new EM_MoveI32(context, 0, srcdata[0], srcdata[1]));
							context.operations.back()->execute();
						}

						//context.sim_track_datarefs(srcdata[0]);
						//context.sim_track_datarefs(srcdata[1]);

						em_dirty[0] = em_dirty[1] = false;

//						if (linechanges[2] || em_dirty[2])
//						{
////							context.operations.push_back(new EM_MoveI16(context, 2, srcdata[2]));
//							if (context.resolve_data16(srcdata[2], reg_index, true) == DataSource_RegisterL)
//								context.operations.push_back(new EM_MoveR16(context, 2, reg_index));
//							else
//								context.operations.push_back(new EM_MoveI16(context, 2, srcdata[2]));
//
//							context.track_datarefs(srcdata[2]);
//
//							em_dirty[2] = false;
//						}
					}
					else
					if ((linechanges[1] || em_dirty[1]) && 
						(linechanges[2] || em_dirty[2]))
					{
						// longword to EM2
						context.operations.push_back(new EM_MoveI32(context, 1, srcdata[1], srcdata[2]));
						context.operations.back()->execute();

						//context.sim_track_datarefs(srcdata[1]);
						//context.sim_track_datarefs(srcdata[2]);

						em_dirty[1] = em_dirty[2] = false;
					}
					else
					{
						// words only

						// this line involves at least one write to EM1/EM2/EM3
						for (int x = 0; x < 3; x++)
						{
							if (linechanges[x] || em_dirty[x])
							{
								if (context.sim_resolve_data16(srcdata[x], reg_index, true) == DataSource_RegisterL)
								{
									context.operations.push_back(new EM_MoveR16(context, x, reg_index));
									context.operations.back()->execute();
								}
								else
								{
									context.operations.push_back(new EM_MoveI16(context, x, srcdata[x]));
									context.operations.back()->execute();
								}

								//context.sim_track_datarefs(srcdata[x]);

								em_dirty[x] = false;
							}
						}
					}

					// execute the blit for this line
					context.operations.push_back(new BLiT_Go(context));
					context.operations.back()->execute();

					// next linejump+blit will be relative to this one (and not an empty one)
					current_line = dsty;
				}
			}
		}

		//int job = 0;
		//for (std::vector<dataregister>::iterator rit = regs.begin(); rit != regs.end(); rit++)
		//{
		//	dataregister &dr = *rit;
		//	if (job != dr.job)
		//	{
		//		// previous job complete - emit blit operations
		//		job = dr.job;

		//		operations.push_back(new BLiT_Go(context));
		//	}
		//}
		// emit final blit operations
		//operations.push_back(new BLiT_Go(context));
		// emit terminal
		context.operations.push_back(new Terminate(context));
		context.operations.back()->execute();

		// peephole passes

		// 1) replace 16bit immediate loads to registers with register-derived values
		// clr.w,not.w,neg.w,and/or/eor.w,add/sub.w,addq/subq.w,scc.b,tas.b

		context.peephole1();

		// 2) replace 16bit immediate loads to EMs with register-derived values

		context.peephole2();

		// 3) replace immediate loads with ops causing side-effects
		// moveq,swap,addq/subq.l,move-usp,ext.l

//		context.peephole3();

		//

		context.reset_sim(false);
		for (std::list<Operation*>::iterator it = context.operations.begin(); it != context.operations.end(); it++)
		{
			(*it)->execute();
			(*it)->emit();
		}

		// debug the blit
		printf("debug simulation...\n");
		for (int v = 0; v < context.srch; v++)
		{
			for (int h = 0; h < context.dstww; h++)
			{
				uword_t data = context.memory[(v * context.dstww) + h];
				print_dataword(data);
			}
			printf("\n");
		}

		context.assemble();

		delete[] changes; changes = 0;
		delete[] buffer; buffer = 0;
	}

	// generate the generator



}


bool generate_emx_sequence()
{
	// individual sprites follow
	for (spritelist_t::iterator it = g_spriteframes.begin(); it != g_spriteframes.end(); it++)
	{
		generate_emx_frame(*it);
	}

	return true;
}


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
		// LL	mask layout mode (00=interleaved, 01=planar, 10=preshift)
		wv = 0;
		wv |= (g_allargs.genmask) ? 1 : 0;
		wv |= (g_allargs.genmask && (g_allargs.masklayout == Masklayout_PLANAR)) ? 2 : 0;
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

				lv = 0xBBBBBBBB;

				// write single offset for colourdata (no preshifts yet)
				int colr_preshift_index_fpos = tracked_filesize;
					fwrite(&lv, 1, 4, hout); tracked_filesize += 4;

				// write N offsets for maskdata preshifts
				int mask_preshift_index_fpos = tracked_filesize;
				for (preshiftlist_t::iterator itm = it->psmask_frames_.begin(); itm != it->psmask_frames_.end(); itm++)
				{
					fwrite(&lv, 1, 4, hout); tracked_filesize += 4;
				}

				// track colour preshifts
				// todo: assumes only one for now
				pscolr_index.push_back(tracked_filesize);

				// write word-width for this preshift (will be 1, 2 or 3 in this mode for STE/F030 blitter endmasks)
				wv = endianswap16(wordwidth);
				fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

				// write colour words for N lines, lines(planes(words))) order
				for (int y = 0; y < it->h_; y++)
				{
					for (int p = 0; p < g_allargs.planes; p++)
					{
						for (int x = 0; x < wordwidth; x++)
						{
							wv = endianswap16(it->colr_[(((y*wordwidth)+x)*g_allargs.planes) + p]);
							fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
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

					// write whole mask preshift
					for (int o = 0; o < itm->planewords_; o++)
					{
						// write colour words for single mask plane
						wv = endianswap16(itm->data_[o]);
						fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
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
	c2p
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

	replace_extension(outfile, "sls");

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
		fwrite(&lv, 1, 4, hout);

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
			fwrite(&lv, 1, 4, hout); tracked_filesize += 4;
		}

		// individual slabs follow
		for (slablist_t::iterator it = g_slabframes.begin(); it != g_slabframes.end(); it++)
		{
			std::vector<int> spanblock_index;

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

			// create clipper (patched after writing)
			wv = 0xAAAA;
			for (int i = 0; i < it->h_; i++)
			{
				// one 16bit index per line of frame
				fwrite(&wv, 1, sizeof(wv), hout); tracked_filesize += sizeof(wv);
			}

			// create spanblock index (patched after writing)
			int spanblock_index_fpos = tracked_filesize;
			wv = 0xBBBB;
			for (int i = 0; i < it->spans_.size(); i++)
			{
				// one 16bit offset per line of frame
				fwrite(&wv, 1, sizeof(wv), hout); tracked_filesize += sizeof(wv);
			}

			// write out spanblocks
//			int mapped_line = 0;
			for (std::list<slabspan_t>::iterator spanit = it->spans_.begin(); spanit != it->spans_.end(); spanit++)
			{
				spanblock_index.push_back(tracked_filesize);

				// 2: spanblock dy
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
				// 2: spanblock y
				wv = endianswap16(spanit->y_);
				fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
				// 2: spanblock ys-1
				wv = endianswap16(spanit->ys_-1);
				fwrite(&wv, 1, 2, hout); tracked_filesize += 2;

				// output spanblock lines
				for (int l = 0; l < spanit->ys_; l++)
				{

					if (0) // interleaved planewords (CPU)
					{
						// output span plane words for a line
						for (int o = 0; o < spanit->dxw_; o++)
						{
							// write colour words for N planes
							for (int p = 0; p < g_allargs.planes; p++)
							{
								wv = endianswap16(spanit->pspan_[(((l*spanit->dxw_) + o) * 8) + p]);
								fwrite(&wv, 1, 2, hout); tracked_filesize += 2;
							}
						}
					}
					else // plane-at-a-time scans (blitter)
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

//			assert(mapped_line == it->h_);

/*			while (mapped_line < it->h_)
			{
				spanblock_index.push_back(-1);
				mapped_line++;
			}
*/

			// patch spanblock index
			int tmp_pos = ftell(hout);
			fseek(hout, spanblock_index_fpos+4, 0);
			for (std::vector<int>::iterator spanit = spanblock_index.begin(); spanit != spanblock_index.end(); spanit++)
			{
				wv = (*spanit) - spanblock_index_fpos;
				wv = endianswap16(wv);
				fwrite(&wv, 1, 2, hout);
			}
			fseek(hout, tmp_pos, 0);

		}

		// patch slab index
		lv = 0;
		fseek(hout, slab_index_fpos+4, 0);
		for (std::vector<int>::iterator it = slab_index.begin(); it != slab_index.end(); it++)
		{
			lv = (*it) - slab_index_fpos;
			lv = endianswap32(lv);
			fwrite(&lv, 1, 4, hout);
		}

		// patch filesize
		fseek(hout, 0, 0);
		lv = endianswap32(tracked_filesize);
		fwrite(&lv, 1, 4, hout);
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

	if (g_allargs.cutmode == Cutmode_SPRITES)
		std::cout << "spritesheet cutting mode..." << std::endl;
	else
	if (g_allargs.cutmode == Cutmode_SLABS)
		std::cout << "slab-sheet cutting mode..." << std::endl;

	{
		if (image_load(g_allargs.sourcefile.c_str(), m_psourcergb, m_psourcecol, m_ppalettergb, sourcew, sourceh, sourced))
		{
			std::cout << "read [" << g_allargs.sourcefile.c_str() << "] with colour depth " << sourced << " and dimensions " << sourcew << " x " << sourceh << std::endl;

			if (m_psourcecol)
			{
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
			}

			if (!m_psourcecol)
			{
				std::cout << "reducing 24bit image to external fixed palette..." << std::endl;
				m_psourcecol = new ubyte_t[sourcew * sourceh]; //(ubyte_t*)malloc(sourcew * sourceh);
				// reduce source image to indexed colour with fixed palette
				reduce_image24(m_psourcergb, m_poriginalsrccol, m_psourcecol, 0, sourcew, sourceh, m_ppalettergb);
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

				int cutting_source_xs = g_allargs.cut_xsize;
				int cutting_source_ys = g_allargs.cut_ysize;

				std::cout << "sprite dimensions [" << cutting_source_xs << ", " << cutting_source_ys << "]" << std::endl;

				if (g_allargs.cutmode == Cutmode_SPRITES ||
					g_allargs.cutmode == Cutmode_EMSPR ||
					g_allargs.cutmode == Cutmode_EMXSPR)
					init_sprite_sequence(cutting_source_xs, cutting_source_ys, sourcew, sourceh);
				else
				if (g_allargs.cutmode == Cutmode_SLABS)
					init_slab_sequence(cutting_source_xs, cutting_source_ys, sourcew, sourceh);

				int cutting_source_x = g_allargs.cut_xpos;
				int cutting_source_y = g_allargs.cut_ypos;

				for (int s = 0, xc = 0; s < cuts; s++)
				{
					int cx = cutting_source_x;
					int cy = cutting_source_y;
					bool xflip = false;
					bool yflip = false;
					int hs = 0;
					int vs = 0;

					spritemap_t::iterator found = g_allargs.spritemappings.find(s);
					if (found != g_allargs.spritemappings.end())
					{
						// this frame is mapped from another
						int from = found->second.from;

						// locate cut record for 'from' frame
						const spritemapping_t &other = g_allargs.spritemappings[from];

						// move cutting position to that of 'from' frame
						cx = other.x;
						cy = other.y;

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
						std::cout << "cutting sprite frame " << s << " from position [" << cutting_source_x << ", " << cutting_source_y << "]" << std::endl;
						add_sprite_frame(m_psourcecol, cx, cy, xflip, yflip, hs, vs);
					}
					else
					if (g_allargs.cutmode == Cutmode_SLABS)
					{
						std::cout << "cutting slab frame " << s << " from position [" << cutting_source_x << ", " << cutting_source_y << "]" << std::endl;
						add_slab_frame(m_psourcecol, cx, cy);
					}

					// store mappings for each frame, so future frames can refer to these
					// mappings in order to re-cut earlier frames with optional flipping
					g_allargs.spritemappings[s].x = cx;
					g_allargs.spritemappings[s].y = cy;

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

				std::cout << "...done" << std::endl;
			}

			// create sprite datafile

			switch (g_allargs.cutmode)
			{
			//
			case Cutmode_EMXSPR:
				generate_emx_sequence();
			case Cutmode_SPRITES:
			case Cutmode_EMSPR:
				emit_sprite_sequence();
				break;
			//
			case Cutmode_SLABS:
				emit_slab_sequence();
				break;
			}
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

	if (g_allargs.cutmode == Cutmode_TILES &&
		g_allargs.outmode != Outmode_RBPLUS &&
		g_allargs.tilesize != 16)
	{
		std::cerr << "error: only 16x16 tile size is supported in degas/direct mode. other sizes are for rbplus only" << std::endl;
		exit(1);
	}

	if (g_allargs.cutmode == Cutmode_TILES)
		do_tiles();
	else
	if (g_allargs.cutmode == Cutmode_SPRITES || 
		g_allargs.cutmode == Cutmode_EMSPR ||
		g_allargs.cutmode == Cutmode_EMXSPR ||
		g_allargs.cutmode == Cutmode_SLABS)
		do_sheet();
	else
	{
		// error
		std::cerr << "error: unsupported cutmode!" << std::endl;
	}
}

