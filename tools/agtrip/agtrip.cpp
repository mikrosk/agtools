// --------------------------------------------------------------------
//	platform stuff - todo: OSX/Linux porting!
// --------------------------------------------------------------------

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include "stdio.h"
#include <iostream>

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
#include "..\3rdparty\stb_image.h"

#define BMP_TYPICAL_ROW_ORDER (1)		// 0 = -ve height, +ve ordering (easier to parse back in)
										// 1 = +ve height, -ve ordering (more common)

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

typedef struct dRGB
{
    double blue;
    double green;
    double red;
	double n;
} dRGB;

typedef struct pixelsolution_s
{
	iRGB options[8];

} pixelsolution_t;

// --------------------------------------------------------------------
//	constants
// --------------------------------------------------------------------

// tool version
static const char* const g_version_string = "v0.3";

// --------------------------------------------------------------------
//	some ...globals (?!)
// --------------------------------------------------------------------

static int s_progress = 0;

//int frame_xoff = 0;	// 80;
//int frame_yoff = 0;
//int frame_xsize = 480;
//int frame_ysize = 270;	//360;
//int forcestep_x = 0;
//int forcestep_y = 0;

static const int max_stackdepth = (16-1);

//double g_competition_curve = 0.25; //0.25;

// --------------------------------------------------------------------
// SimpleOpt... because life is too short
// --------------------------------------------------------------------

//#if defined(_MSC_VER)
//# include <windows.h>
//# include <tchar.h>
//#else
# define TCHAR		char		// yuck
# define _T(x)		x
# undef _UNICODE
//#endif

#include "..\3rdparty\SimpleOpt.h"
#include "..\3rdparty\SimpleGlob.h"

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
	//
	OPT_SOURCE,
	OPT_OUTFILE,
	OPT_SOURCEMASK,
	OPT_PANWIDTH,
	OPT_PANHEIGHT,
	OPT_FRAMESTART,
	OPT_FRAMEEND,
	OPT_FRAMEXOFF,
	OPT_FRAMEYOFF,
	OPT_FRAMEXSIZE,
	OPT_FRAMEYSIZE,
	OPT_COMPCURVE,
	OPT_ERRCURVE,
	OPT_STARTBOT,
	OPT_STARTRIGHT,
	OPT_MINHS,
	OPT_MAXHS,
	OPT_MINVS,
	OPT_MAXVS
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
	//
	{ OPT_SOURCE,		_T("-s"),	 			SO_REQ_SEP	},
	{ OPT_SOURCE,		_T("--srcfile"),		SO_REQ_SEP	},
	//
	{ OPT_OUTFILE,		_T("-o"),	 			SO_REQ_SEP	},
	{ OPT_OUTFILE,		_T("--outfile"),		SO_REQ_SEP	},
	//
	{ OPT_SOURCEMASK,	_T("-sm"),	 			SO_REQ_SEP	},
	{ OPT_SOURCEMASK,	_T("--srcmask"),		SO_REQ_SEP	},
	//
	{ OPT_PANWIDTH,		_T("-pw"),    			SO_REQ_SEP	},
	{ OPT_PANWIDTH,		_T("--panwidth"), 		SO_REQ_SEP	},
	//
	{ OPT_PANHEIGHT,	_T("-ph"),    			SO_REQ_SEP	},
	{ OPT_PANHEIGHT,	_T("--panheight"), 		SO_REQ_SEP	},
	//
	{ OPT_FRAMEXOFF,	_T("-fxo"),    			SO_REQ_SEP	},
	{ OPT_FRAMEXOFF,	_T("--framexoff"),		SO_REQ_SEP	},
	//
	{ OPT_FRAMEYOFF,	_T("-fyo"),    			SO_REQ_SEP	},
	{ OPT_FRAMEYOFF,	_T("--frameyoff"),		SO_REQ_SEP	},
	//
	{ OPT_FRAMEXSIZE,	_T("-fxs"),    			SO_REQ_SEP	},
	{ OPT_FRAMEXSIZE,	_T("--framexsize"),		SO_REQ_SEP	},
	//
	{ OPT_FRAMEYSIZE,	_T("-fys"),    			SO_REQ_SEP	},
	{ OPT_FRAMEYSIZE,	_T("--frameysize"),		SO_REQ_SEP	},
	//
	{ OPT_FRAMESTART,	_T("-fs"),    			SO_REQ_SEP	},
	{ OPT_FRAMESTART,	_T("--framestart"),		SO_REQ_SEP	},
	//
	{ OPT_FRAMEEND,		_T("-fe"),    			SO_REQ_SEP	},
	{ OPT_FRAMEEND,		_T("--frameend"),		SO_REQ_SEP	},
	//
	{ OPT_MINHS,		_T("-mnhs"),   			SO_REQ_SEP	},
	{ OPT_MINHS,		_T("--minhscroll"),		SO_REQ_SEP	},
	//
	{ OPT_MAXHS,		_T("-mxhs"),   			SO_REQ_SEP	},
	{ OPT_MAXHS,		_T("--maxhscroll"),		SO_REQ_SEP	},
	//
	{ OPT_MINVS,		_T("-mnvs"),   			SO_REQ_SEP	},
	{ OPT_MINVS,		_T("--minvscroll"),		SO_REQ_SEP	},
	//
	{ OPT_MAXVS,		_T("-mxvs"),   			SO_REQ_SEP	},
	{ OPT_MAXVS,		_T("--maxvscroll"),		SO_REQ_SEP	},
	//
	{ OPT_STARTBOT,		_T("-sb"),    			SO_NONE	},
	{ OPT_STARTBOT,		_T("--startbot"),		SO_NONE	},
	//
	{ OPT_STARTBOT,		_T("-sr"),    			SO_NONE	},
	{ OPT_STARTBOT,		_T("--startright"),		SO_NONE	},
	//
	{ OPT_COMPCURVE,	_T("-cc"),    			SO_REQ_SEP	},
	{ OPT_COMPCURVE,	_T("--compcurve"), 		SO_REQ_SEP	},
	//
	{ OPT_ERRCURVE,		_T("-ec"),    			SO_REQ_SEP	},
	{ OPT_ERRCURVE,		_T("--errorcurve"),		SO_REQ_SEP	},
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
	std::cout << " -?, -h, --help        > show help/usage" << std::endl;
	std::cout << " -v, --verbose         > more output" << std::endl;
	std::cout << " -q, --quiet           > less output" << std::endl;
	std::cout << " -wk, --wait-ke y      > wait key on exit" << std::endl;
	std::cout << std::endl;
	std::cout << " -s, --srcseq          > filename stub for source frame sequence (extension assumed .png)" << std::endl;
	std::cout << " -o, --outfile         > output panorama/playfield image (extension forced to .tga)" << std::endl;
	std::cout << " -sm, --srcmask        > magenta mask applied to source frames, for ignoring static (hud) overlays" << std::endl;
	std::cout << " -sb, --startbot       > assume view begins at bottom (not top) of map" << std::endl;
	std::cout << " -sr, --startright     > assume view begins at right (not left) of map" << std::endl;
	std::cout << " -pw, --panwidth       > (over-estimate) width of final panorama/playfield" << std::endl;
	std::cout << " -ph, --panheight      > (over-estimate) height of final panorama/playfield" << std::endl;
	std::cout << " -mnhs, --minhscroll   > set min hscroll step per frame" << std::endl;
	std::cout << " -mxhs, --maxhscroll   > set max hscroll step per frame" << std::endl;
	std::cout << " -mnvs, --minvscroll   > set min vscroll step per frame" << std::endl;
	std::cout << " -mxvs, --maxvscroll   > set max vscroll step per frame" << std::endl;
	std::cout << " -fxo, --framexoff     > xoffset of active capture region in source frames" << std::endl;
	std::cout << " -fyo, --frameyoff     > yoffset of active capture region in source frames" << std::endl;
	std::cout << " -fxs, --framexsize    > xsize of active capture region in source frames" << std::endl;
	std::cout << " -fys, --frameysize    > ysize of active capture region in source frames" << std::endl;
	std::cout << " -fs, --framestart     > first source frame in numbered 000-nnn .png sequence" << std::endl;
	std::cout << " -fe, --frameend       > last source frame in numbered 000-nnn .png sequence" << std::endl;
	std::cout << " -cc, --compcurve      > delta competition curve for synclock & integrate (default=0.25)" << std::endl;
	std::cout << " -ec, --errorcurve     > colour error curve for synclock & integrate (default=20.0)" << std::endl;

	std::cout << std::endl;
		
	getchar();	
	exit(1);
}

// --------------------------------------------------------------------
//	general argument handling
// --------------------------------------------------------------------

class Arguments
{
public:

	Arguments()
		: verbose(false),
		  quiet(false),
		  waitkey(false),
		  sourcefile("default"),
		  pwidth(0),
		  pheight(0),
		  framestart(0),
		  frameend(0),
		  frame_xoff(0),
		  frame_yoff(0),
		  frame_xsize(320),
		  frame_ysize(200),
		  minhs(-32768),
		  maxhs(32767),
		  minvs(-32768),
		  maxvs(32767),
		  competition_curve(0.25),
		  error_curve(20.0),
		  startbot(false),
		  startright(false)
	{
	}

	int pwidth;
	int pheight;
	int framestart;
	int frameend;
	int frame_xsize;
	int frame_ysize;
	int frame_xoff;
	int frame_yoff;
	int minhs;
	int maxhs;
	int minvs;
	int maxvs;
	std::string sourcefile;
	std::string sourcemask;
	std::string outfile;
	float competition_curve;
	float error_curve;
	bool startbot;
	bool startright;

	bool verbose;
	bool quiet;
	bool waitkey;
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
			else if (args.OptionId() == OPT_STARTBOT) 
			{
				_args.startbot = true;
			}	
			else if (args.OptionId() == OPT_STARTRIGHT) 
			{
				_args.startright = true;
			}		
			else if (args.OptionId() == OPT_SOURCE)
			{
				_args.sourcefile = args.OptionArg();
			}
			else if (args.OptionId() == OPT_OUTFILE)
			{
				_args.outfile = args.OptionArg();
			}
			else if (args.OptionId() == OPT_SOURCEMASK)
			{
				_args.sourcemask = args.OptionArg();
			}
			else if (args.OptionId() == OPT_PANWIDTH) 
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 320 || iarg > 32768)
				{
					printf("error - invalid arg panorama width: %s\n", s_arg.c_str());
					exit(1);
				}
				_args.pwidth = iarg;	
			}
			else if (args.OptionId() == OPT_PANHEIGHT) 
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 120 || iarg > 32768)
				{
					printf("error - invalid arg panorama height: %s\n", s_arg.c_str());
					exit(1);
				}
				_args.pheight = iarg;	
			}
			else if (args.OptionId() == OPT_FRAMESTART) 
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 0 || iarg > 32768)
				{
					printf("error - invalid arg frame start: %s\n", s_arg.c_str());
					exit(1);
				}
				_args.framestart = iarg;	
			}
			else if (args.OptionId() == OPT_FRAMEEND) 
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg <= 0 || iarg > 32768)
				{
					printf("error - invalid arg frame end: %s\n", s_arg.c_str());
					exit(1);
				}
				_args.frameend = iarg;	
			}
			else if (args.OptionId() == OPT_MINHS) 
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < -512 || iarg > 512)
				{
					printf("error - invalid arg min hscroll: %s\n", s_arg.c_str());
					exit(1);
				}
				_args.minhs = iarg;	
			}
			else if (args.OptionId() == OPT_MAXHS) 
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < -512 || iarg > 512)
				{
					printf("error - invalid arg max hscroll: %s\n", s_arg.c_str());
					exit(1);
				}
				_args.maxhs = iarg;	
			}
			else if (args.OptionId() == OPT_MINVS) 
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < -512 || iarg > 512)
				{
					printf("error - invalid arg min vscroll: %s\n", s_arg.c_str());
					exit(1);
				}
				_args.minvs = iarg;	
			}
			else if (args.OptionId() == OPT_MAXVS) 
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < -512 || iarg > 512)
				{
					printf("error - invalid arg max vscroll: %s\n", s_arg.c_str());
					exit(1);
				}
				_args.maxvs = iarg;	
			}
			else if (args.OptionId() == OPT_FRAMEXOFF) 
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 0 || iarg > 1024)
				{
					printf("error - invalid arg frame xoff: %s\n", s_arg.c_str());
					exit(1);
				}
				_args.frame_xoff = iarg;	
			}
			else if (args.OptionId() == OPT_FRAMEYOFF) 
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg < 0 || iarg > 1024)
				{
					printf("error - invalid arg frame yoff: %s\n", s_arg.c_str());
					exit(1);
				}
				_args.frame_yoff = iarg;	
			}
			else if (args.OptionId() == OPT_FRAMEXSIZE) 
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg <= 0 || iarg > 1024)
				{
					printf("error - invalid arg frame xsize: %s\n", s_arg.c_str());
					exit(1);
				}
				_args.frame_xsize = iarg;	
			}
			else if (args.OptionId() == OPT_FRAMEYSIZE) 
			{
				std::string s_arg = args.OptionArg();
				int iarg = atoi(s_arg.c_str());
				if (iarg <= 0 || iarg > 1024)
				{
					printf("error - invalid arg frame ysize: %s\n", s_arg.c_str());
					exit(1);
				}
				_args.frame_ysize = iarg;	
			}
			else if (args.OptionId() == OPT_COMPCURVE) 
			{
				std::string s_arg = args.OptionArg();
				float farg = (float)atof(s_arg.c_str());
				if (farg <= 0.0f || farg > 256.0f)
				{
					printf("error - invalid arg for competition curve: %s\n", s_arg.c_str());
					exit(1);
				}
				_args.competition_curve = farg;	
			}
			else if (args.OptionId() == OPT_ERRCURVE) 
			{
				std::string s_arg = args.OptionArg();
				float farg = (float)atof(s_arg.c_str());
				if (farg <= 0.0f || farg > 256.0f)
				{
					printf("error - invalid arg for error curve: %s\n", s_arg.c_str());
					exit(1);
				}
				_args.error_curve = farg;	
			}
		}
		else 
		{
			printf("error - invalid argument: %s\n", args.OptionText());
			usage(0);
			exit(1);
		}
    }

	if (
		(_args.sourcefile.length() == 0) ||
		(_args.pwidth == 0) ||
		(_args.pheight == 0)
	)
	{
		usage(0);
		exit(1);
	}

	if (_args.outfile.length() == 0)
		_args.outfile = _args.sourcefile;
}

typedef struct image_s
{
	RGB *psourcergb;
	unsigned char *psourcecol;
	RGB *ppalettergb;
	double *pmask;
	ulong_t w;
	ulong_t h;
	ulong_t d;

	//dRGB *pcompute;
	//int xclip;
	//int yclip;

} image_t;

image_t maskobj;

typedef struct pixelstack_s
{
	RGB stack[max_stackdepth+1];
	double error[max_stackdepth+1];
	int count;

} pixelstack_t;

typedef struct canvas_s
{
	dRGB *pcompute;
	pixelstack_t *pstack;

	ulong_t w;
	ulong_t h;
	int xclip;
	int yclip;

} canvas_t;

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
	std::cerr << "error: can't open this image: " << filename << std::endl;
	return false;
}

void integrate(const image_t &inew, canvas_t &ipan, int xr, int yr)
{
	int sw = xmin(inew.w, g_allargs.frame_xsize);
	int sh = xmin(inew.h, g_allargs.frame_ysize);

	for (int y = 0; (y < sh)/* && ((y+yr) < ipan.h)*/; y++)
	{
		if ((y + yr) >= ipan.h) continue;
		if ((y + yr) < 0) continue;

		int yy = y + g_allargs.frame_yoff;

		for (int x = 0; (x < sw)/* && ((x+xr) < ipan.w)*/; x++)
		{
			if ((x + xr) >= ipan.w) continue;
			if ((x + xr) < 0) continue;

			int xx = x + g_allargs.frame_xoff;

			const RGB &rgbnew = inew.psourcergb[(yy * inew.w) + xx];
			dRGB &rgbpan = ipan.pcompute[((y+yr) * ipan.w) + (x+xr)];
			pixelstack_t &rgbstack = ipan.pstack[((y+yr) * ipan.w) + (x+xr)];

			// dodge areas which have been statically (manually) masked for all frames
			// i.e. painted magenta in mask.png
			if (maskobj.pmask)
			if ((yy >= 0) && (yy < maskobj.h))
				if ((xx >= 0) && (xx < maskobj.w))
						if (maskobj.pmask[(yy * maskobj.w) + xx] < -0.5)
						continue;

			// running average
			rgbpan.red +=	rgbnew.red;
			rgbpan.green += rgbnew.green;
			rgbpan.blue +=	rgbnew.blue;
			rgbpan.n +=		1.0;

			// discrete competitors
			if (rgbstack.count < max_stackdepth)
			{
				// always add competitor if there is space
				RGB& col = rgbstack.stack[rgbstack.count];
				col.red =	rgbnew.red;
				col.green = rgbnew.green;
				col.blue =	rgbnew.blue;
				rgbstack.count++;
			}
			else
			{
				// otherwise compute error and replace

				// add to temporary slot
				RGB& col = rgbstack.stack[rgbstack.count];
				col.red = rgbnew.red;
				col.green = rgbnew.green;
				col.blue = rgbnew.blue;

				// compute error for all slots (including temporary)
				for (int o = 0; o < rgbstack.count+1; o++)
				{
					const RGB &oc = rgbstack.stack[o];

					double osum = 0;
					for (int i = 0; i < rgbstack.count+1; i++)
					{
						if (o != i)
						{
							const RGB &ic = rgbstack.stack[i];

							double dr = (oc.red - ic.red);
							double dg = (oc.green - ic.green);
							double db = (oc.blue - ic.blue);

							// scale
							dr /= 255.0;
							dg /= 255.0;
							db /= 255.0;

							// error
							double e = (dr * dr) + (dg * dg) + (db * db);

							osum += e;
						}
					}

					// record error for this entry vs all others (including temporary)
					rgbstack.error[o] = osum;
				}

				// find highest, lowest error (including temporary)
				//double elo = 1e30;
				//int ilo = -1;
				double ehi = 0;
				int ihi = -1;
				for (int i = 0; i < rgbstack.count + 1; i++)
				{
					double e = rgbstack.error[i];

					if (e > ehi)
					{
						ehi = e;
						ihi = i;
					}
				}

				if (ihi != max_stackdepth)
				{
					// temporary candidate is not highest error, so we should
					// replace the highest error candidate

					rgbstack.error[ihi] = rgbstack.error[max_stackdepth];
					rgbstack.stack[ihi] = rgbstack.stack[max_stackdepth];
				}
			}
		}
	}

	if (ipan.xclip < (sw + xr))
		ipan.xclip = (sw + xr);

	if (ipan.yclip < (sh + yr))
		ipan.yclip = (sh + yr);

	printf("integrated %dx%d @ %d,%d -> %dx%d\n", sw, sh, xr, yr, ipan.xclip, ipan.yclip);
}

void resolve(canvas_t &ipan)
{
	for (int y = 0; y < ipan.yclip; y++)
	{
		for (int x = 0; x < ipan.xclip; x++)
		{
			dRGB &rgbpan = ipan.pcompute[(y * ipan.w) + x];
			pixelstack_t &rgbstack = ipan.pstack[(y * ipan.w) + x];

			//if (x > 480 && y == 8)
			//{
			//	printf("");
			//}

			{
				// compute error for all slots
				for (int o = 0; o < rgbstack.count; o++)
				{
					const RGB &oc = rgbstack.stack[o];

					double osum = 0;
					for (int i = 0; i < rgbstack.count; i++)
					{
						if (o != i)
						{
							const RGB &ic = rgbstack.stack[i];

							double dr = ((double)oc.red - (double)ic.red);
							double dg = ((double)oc.green - (double)ic.green);
							double db = ((double)oc.blue - (double)ic.blue);

							// scale
							dr /= 255.0;
							dg /= 255.0;
							db /= 255.0;

							// error
							double e = (dr * dr) + (dg * dg) + (db * db);

							osum += e;
						}
					}

					// record error for this entry vs all others (including temporary)
					rgbstack.error[o] = pow(osum, g_allargs.error_curve);
				}

				// normalize error for all slots
				double rc = 1.0 / double(rgbstack.count);
				double emin = 1e30;
				double emax = 0;
				for (int o = 0; o < rgbstack.count; o++)
				{
					// normalize
					rgbstack.error[o] *= rc;

					// bound
					double e = rgbstack.error[o];
					if (e < emin) emin = e;
					if (e > emax) emax = e;
				}

				// careful with exact matches
				emin += 1e-20;

				// convert to reciprocal
				for (int o = 0; o < rgbstack.count; o++)
				{
					double re = emin / (emin + rgbstack.error[o]);
					if (re < 0.1)
						re = 0;

					rgbstack.error[o] = re;
				}

				// solve colour
				{
					double r = 0;
					double g = 0;
					double b = 0;
					double ws = 0;

					for (int o = 0; o < rgbstack.count; o++)
					{
						double w = rgbstack.error[o];
						r += rgbstack.stack[o].red *	w;
						g += rgbstack.stack[o].green *	w;
						b += rgbstack.stack[o].blue *	w;
						ws += w;
					}

					r /= ws;
					g /= ws;
					b /= ws;

					rgbpan.red =	r;
					rgbpan.green =	g;
					rgbpan.blue =	b;
					rgbpan.n = 1.0;
				}
			}
		}
	}

	printf("resolved!\n");
}

double compare(const image_t &inew, const canvas_t &ipan, int xr, int yr)
{
	int sw = xmin(inew.w, g_allargs.frame_xsize);
	int sh = xmin(inew.h, g_allargs.frame_ysize);

	double esum = 0.0;
	double count = 0;

	for (int y = 0; (y < sh) /*&& ((y+yr) < ipan.yclip)*/; y++)
	{
		if ((y + yr) < 0) continue;
		if ((y + yr) >= ipan.yclip) continue;

		int yy = y + g_allargs.frame_yoff;

		for (int x = 0; (x < sw)/* && ((x+xr) < ipan.xclip)*/; x++)
		{
			if ((x + xr) < 0) continue;
			if ((x + xr) >= ipan.xclip) continue;

			int xx = x + g_allargs.frame_xoff;

			const dRGB &rgbpan = ipan.pcompute[((y+yr) * ipan.w) + (x+xr)];
			const RGB &rgbnew = inew.psourcergb[((y+g_allargs.frame_yoff) * inew.w) + (x+g_allargs.frame_xoff)];

			double n = rgbpan.n;
			if (n < 1.0)
				n = 1.0;

			double pr = rgbpan.red;
			double pg = rgbpan.green;
			double pb = rgbpan.blue;

			double pn = 1.0 / n;
			pr *= pn;
			pg *= pn;
			pb *= pn;

			double dr = (pr - (double)rgbnew.red);
			double dg = (pg - (double)rgbnew.green);
			double db = (pb - (double)rgbnew.blue);

			// scale
			dr /= 255.0;
			dg /= 255.0;
			db /= 255.0;

			// error
			double e = (dr * dr) + (dg * dg) + (db * db);

			e = pow(e, g_allargs.competition_curve);

			// demote areas which have been statically (manually) masked for all frames
			// i.e. painted green in mask.png
			if (maskobj.pmask)
				if ((yy >= 0) && (yy < maskobj.h))
					if ((xx >= 0) && (xx < maskobj.w))
						if (maskobj.pmask[(yy * maskobj.w) + xx] < 0.5)
							e = 0;

			// error sum
			esum += e; count += 1.0;
		}
	}

	if (count < 1e-20)
		return 1e30;

	// normalize for clipping
	return esum / count;
}

bool synchronize(const image_t &inew, const canvas_t &ipan, int lastx, int lasty, int& newx, int& newy)
{
	int sw = xmin(inew.w, g_allargs.frame_xsize);
	int sh = xmin(inew.h, g_allargs.frame_ysize);

	// sliding window

	double best_oe = 1e30;
	int best_xo = -1;
	int best_yo = -1;

	/*
//	for (int yo = lasty + g_allargs.forcestep_y; (yo < ipan.yclip) && (yo < lasty+1); yo++)
	for (int yo = lasty + g_allargs.forcestep_y; (yo < (ipan.yclip - (sh>>1))); yo += 16)
	{
		for (int xo = lastx + g_allargs.forcestep_x; (xo < ipan.yclip) && (xo < lastx+1); xo++)
//		for (int xo = lastx + g_allargs.forcestep_x; (xo < (ipan.xclip - (sw>>1))); xo += 16)
		{
			double try_oe = compare(inew, ipan, xo, yo);

			if (try_oe < best_oe)
			{
				best_oe = try_oe;
				best_xo = xo;
				best_yo = yo;
			}
		}
	}
*/
#if (1)
	// pass #0

//	for (int yo = lasty + g_allargs.forcestep_y; (yo < (ipan.yclip - (sh>>1))); yo += 16)
//	for (int yo = lasty - (sh>>1); yo < lasty + (sh>>1); yo += 16)
	for (int yo = lasty + g_allargs.minvs; yo < lasty + g_allargs.maxvs; yo += 16)
	{
//		for (int xo = lastx + g_allargs.forcestep_x; (xo < (ipan.xclip - (sw>>1))); xo += 16)
//		for (int xo = lastx - (sw>>1); xo < lastx + (sw>>1); xo += 16)
		for (int xo = lastx + g_allargs.minhs; xo < lastx + g_allargs.maxhs; xo += 16)
		{
			double try_oe = compare(inew, ipan, xo, yo);

			if (try_oe < best_oe)
			{
				best_oe = try_oe;
				best_xo = xo;
				best_yo = yo;
			}
		}
	}

	// pass #1

	for (int yo = best_yo - 16; (yo <= (best_yo + 16)) /*&& (yo < (ipan.yclip - (sh>>1)))*/; yo += 4)
	{
		if ((yo < lasty + g_allargs.minvs) || (yo >= lasty + g_allargs.maxvs)) continue;

		for (int xo = best_xo - 16; (xo <= (best_xo + 16)) /*&& (xo < (ipan.xclip - (sw>>1)))*/; xo += 4)
		{
			if ((xo < lastx + g_allargs.minhs) || (xo >= lastx + g_allargs.maxhs)) continue;

			double try_oe = compare(inew, ipan, xo, yo);

			if (try_oe < best_oe)
			{
				best_oe = try_oe;
				best_xo = xo;
				best_yo = yo;
			}
		}
	}

	// pass #2

	for (int yo = best_yo - 4; (yo <= (best_yo + 4)) /*&& (yo < (ipan.yclip - (sh>>1)))*/; yo += 1)
	{
		if ((yo < lasty + g_allargs.minvs) || (yo >= lasty + g_allargs.maxvs)) continue;

		for (int xo = best_xo - 4; (xo <= (best_xo + 4)) /*&& (xo < (ipan.xclip - (sw>>1)))*/; xo += 1)
		{
			if ((xo < lastx + g_allargs.minhs) || (xo >= lastx + g_allargs.maxhs)) continue;

			double try_oe = compare(inew, ipan, xo, yo);

			if (try_oe < best_oe)
			{
				best_oe = try_oe;
				best_xo = xo;
				best_yo = yo;
			}
		}
	}

	/*
	// pass #3

	for (int yo = best_yo - 4; (yo <= (best_yo + 4)) && (yo < (ipan.yclip - (sh>>1))); yo += 2)
	{
		if (yo < 0) continue;

		for (int xo = best_xo - 4; (xo <= (best_xo + 4)) && (xo < (ipan.xclip - (sw>>1))); xo += 2)
		{
			if (xo < 0) continue;

			double try_oe = compare(inew, ipan, xo, yo);

			if (try_oe < best_oe)
			{
				best_oe = try_oe;
				best_xo = xo;
				best_yo = yo;
			}
		}
	}


	// pass #4

	for (int yo = best_yo - 2; (yo <= (best_yo + 2)) && (yo < (ipan.yclip - (sh>>1))); yo += 1)
	{
		if (yo < 0) continue;

		for (int xo = best_xo - 2; (xo <= (best_xo + 2)) && (xo < (ipan.xclip - (sw>>1))); xo += 1)
		{
			if (xo < 0) continue;

			double try_oe = compare(inew, ipan, xo, yo);

			if (try_oe < best_oe)
			{
				best_oe = try_oe;
				best_xo = xo;
				best_yo = yo;
			}
		}
	}
	*/
#endif

	newx = best_xo;
	newy = best_yo;

	return true;
}

// --------------------------------------------------------------------
//	main program entrypoint
// --------------------------------------------------------------------

int main(int argc, char* argv[])
{

	std::cout << "[A]tari [G]ame [T]ools: screenshot->playfield ripper " << g_version_string << " / dml" << std::endl;

	GetArguments(argc, argv, g_allargs);

	// generate image name sequence
	std::vector<std::string> namestream;
//	for (int i = 545; i <= 656; i++)
//	for (int i = 2; i <= 116; i++)
////	for (int i = 13; i <= 33; i++)
	for (int i = g_allargs.framestart; i <= g_allargs.frameend; i++)
	{
		int d = i;
//		std::string name("s:\\grad_bgs\\Image");
//		std::string name("s:\\grad_bgs\\g3_final\\img_");

		std::string name = g_allargs.sourcefile;
		remove_extension(name);
		if (name.back() == '.')
			name.pop_back();
		name.push_back('_');


		int c0 = d % 10; d /= 10;
		int c1 = d % 10; d /= 10;
		int c2 = d % 10; d /= 10;

		name.push_back('0'+c2);
		name.push_back('0'+c1);
		name.push_back('0'+c0);

		name.append(".png");
		namestream.push_back(name);
	}


	// load images
	std::vector<image_t> imagestream;
	for (std::vector<std::string>::iterator nit = namestream.begin(); nit != namestream.end(); nit++)
	{
		std::string& name = *nit;

		image_t imageobj;

		if (image_load(name.c_str(), imageobj.psourcergb, imageobj.psourcecol, imageobj.ppalettergb, imageobj.w, imageobj.h, imageobj.d))
		{
			imagestream.push_back(imageobj);

			std::cout << "loaded image frame: " << name.c_str() << std::endl;
		}
		else
		{
			std::cerr << "error: could not read source image frame: " << name.c_str() << std::endl;
		}
	}

	// load mask

	maskobj.pmask = 0;
	if (g_allargs.sourcemask.length() > 0)
	{
		if (image_load(g_allargs.sourcemask.c_str(), maskobj.psourcergb, maskobj.psourcecol, maskobj.ppalettergb, maskobj.w, maskobj.h, maskobj.d))
		{
			maskobj.pmask = new double[maskobj.w * maskobj.h];

			for (int y = 0; y < maskobj.h; y++)
			{
				for (int x = 0; x < maskobj.w; x++)
				{
					RGB &rgb = maskobj.psourcergb[(y*maskobj.w) + x];

					if ((rgb.red > 250) && (rgb.green < 8) && (rgb.blue > 250))
						// magenta = -1
						// no synclock, no integrate (e.g. completely ignore HUD text)
						maskobj.pmask[(y*maskobj.w) + x] = -1.0;
					else
					if ((rgb.red < 8) && (rgb.green) > 250 && (rgb.blue < 8))
						// green = 0
						// no synclock, but still integrate (e.g. insensitive to parallax but keeping pixels)
						maskobj.pmask[(y*maskobj.w) + x] = 0.0;
					else
						// remainder = 1
						// synclock + integrate
						maskobj.pmask[(y*maskobj.w) + x] = 1.0;
				}
			}
		}
		else
		{
			std::cerr << "error: could not read source mask:" << g_allargs.sourcemask.c_str() << std::endl;
		}
	}

	if (imagestream.empty())
	{
		std::cerr << "error: no source frames to process!" << std::endl;
		exit(1);
	}
	// for each image, compare with both neighbours & detect/mark unchanging pixels
#if (0)
	if (0)
	{
		std::vector<image_t>::iterator cit = imagestream.begin();
		std::vector<image_t>::iterator pit = cit++;
		for (; cit != imagestream.end(); cit++)
		{
			image_t& pimg = *pit;
			const image_t& cimg = *cit;

			int sw = xmin(pimg.w, cimg.w);
			int sh = xmin(pimg.h, cimg.h);

			for (int y = 0; (y < sh); y++)
			{
				for (int x = 0; (x < sw); x++)
				{
					const RGB &prgb = pimg.psourcergb[(y * pimg.w) + x];
					const RGB &crgb = cimg.psourcergb[(y * cimg.w) + x];

					double dr = ((double)crgb.red -		(double)prgb.red);
					double dg = ((double)crgb.green -	(double)prgb.green);
					double db = ((double)crgb.blue -	(double)prgb.blue);

					// scale
					dr /= 255.0;
					dg /= 255.0;
					db /= 255.0;

					double fi = ((double)crgb.red + (double)crgb.green + (double)crgb.blue) / 255.0;

					// error
					double e = (dr * dr) + (dg * dg) + (db * db);

//					e = pow(e, g_competition_curve);

					// a neigbour can disable a pixel, but can't re-enable it
					if ((e < 1e-5) && (fi > 0.01))
					{
						pimg.pmask[(y * pimg.w) + x] = 0.0;
						cimg.pmask[(y * pimg.w) + x] = 0.0;
					}
				}
			}


			pit = cit;
		}
	}
#endif

	// for each image, compare with subsequent images (until some error threshold exceeded),
	// using varying offsets until lowest error found

	// set scroll step search limits on both axes
	int limhs = imagestream.front().w >> 2;
	int limvs = imagestream.front().h >> 2;

	if (g_allargs.minhs < -16384)
		g_allargs.minhs = -limhs;
	if (g_allargs.maxhs > 16384)
		g_allargs.maxhs = limhs;

	if (g_allargs.minvs < -16384)
		g_allargs.minvs = -limvs;
	if (g_allargs.maxvs > 16384)
		g_allargs.maxvs = limvs;

	canvas_t panorama;
//	panorama.w = 12000;
//	panorama.h = 512;
	panorama.w = g_allargs.pwidth;
	panorama.h = g_allargs.pheight;
	panorama.xclip = 0;
	panorama.yclip = 0;

	panorama.pcompute = new dRGB[panorama.w * panorama.h];
	memset(panorama.pcompute, 0, panorama.w*panorama.h*sizeof(dRGB));

	panorama.pstack = new pixelstack_t[panorama.w * panorama.h];
	memset(panorama.pstack, 0, panorama.w*panorama.h*sizeof(pixelstack_t));

	std::vector<image_t>::iterator oit = imagestream.begin();
	// load first image

	// default origin (top/left)
	int lastx = 0; 
	int lasty = 0;

	// adjust origin for initial synclock on different types of scroll
	if (g_allargs.startright)
		lastx = panorama.w - g_allargs.frame_xsize;
	if (g_allargs.startbot)
		lasty = panorama.h - g_allargs.frame_ysize;

	// integrate the first image at the origin
	integrate(*oit, panorama, lastx, lasty);

	// integrate subsequent images based on synclock deltas
	oit++;
	for (; oit != imagestream.end(); oit++)
	{
		const image_t& oimg = *oit;

		int newx = 0;
		int newy = 0;
		if (synchronize(oimg, panorama, lastx, lasty, newx, newy))
		{
			printf("synclock: %d,%d / delta: %d,%d\n", newx, newy, newx-lastx, newy-lasty);

			integrate(oimg, panorama, newx, newy);
			lastx = newx;
			lasty = newy;
		}
	}

	// solve competing candidate pixels within panorama canvas
	resolve(panorama);
	
	replace_extension(g_allargs.outfile, "tga");

	// emit trivial targa format, out of sheer lazyness
	FILE *fptr = fopen(g_allargs.outfile.c_str(), "wb");
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
		putc((panorama.xclip & 0x00FF),fptr);
		putc((panorama.xclip & 0xFF00) / 256,fptr);
		putc((panorama.yclip & 0x00FF),fptr);
		putc((panorama.yclip & 0xFF00) / 256,fptr);
		putc(24,fptr);                        /* 24 bit bitmap */
		putc(0,fptr);

		for (int y = panorama.yclip-1; y >= 0; y--)
		{
			for (int x = 0; x < panorama.xclip; x++)
			{
				const dRGB &rgbpan = panorama.pcompute[(y * panorama.w) + x];

				putc((int)(rgbpan.blue	/ rgbpan.n),		fptr);
				putc((int)(rgbpan.green	/ rgbpan.n),		fptr);
				putc((int)(rgbpan.red	/ rgbpan.n),		fptr);
			}
		}

		fclose(fptr);
	}
	else
	{
		std::cerr << "error: could not create output TGA file: " << g_allargs.outfile.c_str() << std::endl;
	}

}

