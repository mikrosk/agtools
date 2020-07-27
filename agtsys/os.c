//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
//	TOS, CPU & low level systems 
//	todo: still needs rewritten for 68000 (originated from BadMood/030)
//----------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------
//	system headers

#include <mint/sysbind.h>

#include <stdarg.h>
#include <string.h>
#include <memory.h>

#ifdef HATARI_NF_PRINTF
#include "3rdparty/natfeats.h"
#endif

// ---------------------------------------------------------------------------------------------------------------------
//	Our AGT headers

// ---------------------------------------------------------------------------------------------------------------------

#if defined(AGT_CONFIG_QUIET)
#define AGT_CONFIG_ALLOW_PRINT (0)
#else
#define AGT_CONFIG_ALLOW_PRINT (1)
#endif

//#if !defined(AGT_CONFIG_SYS_GUARDX)
//#define AGT_CONFIG_SYS_GUARDX (32)
//#endif

static const int c_con_linewid = 160;//(((20+1)+((AGT_CONFIG_SYS_GUARDX+15)>>4))*8);

// todo: provide common_c.h
#define reg32(_a_) *(volatile u32*)(0x##_a_)
#define agt_used __attribute__((used))

int vsnprintf (char *str, size_t count, const char *fmt, va_list arg);

short g_hnf_ok = 0;
short g_displaylocked = 0;
short g_lock_consolerefresh = 0;
short g_console_dirty = 0;

// must be called only in supervisor mode, before user entrypoint
int AGT_EntryPointPre()
{
#if defined(HATARI_NF_PRINTF)
	nf_init();
#endif
	return 0;
}

int AGT_EntryPointPost()
{
#if defined(HATARI_NF_PRINTF)
	g_hnf_ok = 0;
#endif
	return 0;
}

#if (1)

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;

extern int agtprintf(const char *pFormat, ...);


//#include "common_c.h"

char* console_base = 0;
char* console_minbase = 0;
char* console_maxbase = 0;
char* *min_pconsole = 0;	
char* max_console = 0;	
extern void etext();

/* used by bmengine.a */
//int c_dpf(const char* const string)
//{
//	return printf("%s", string);
//}

static __inline void AGT_ConsoleText(const char* text, void* fb, int c, int r)
{
	// minmal shim to 68k version

/*
_xprn_print_string:
; In: a0.l - pointer to null-terminated string (supports $0d as newline)
;     a1.l - pointer to font file
;     a2.l - screen pointer, adjusted for x and y (i e where to start rendering chars)
;            value must be complete with bitplane offset, must be on an even 16-pixel
;            boundary in X
;     d0.w - column width in pixels (it's your responsibility this value doesn't clash
;            with the screen line width)
;     d1.w - width of screen line in bytes
;     d2.b - extra line height in pixels (0 = the font's own height, 3 = 3 extra scanlines etc)
;     d3.b - number of bitplanes in destination screen
;     d4.b - use blitter? 1 = yes, 0 = no
*/
	s32 offsetaddr = (s32)fb + (r * 12 * /*184*/c_con_linewid);

	__asm__ __volatile__
	(
	"	move.l %0,%%a0;"
	"	move.l %1,%%a2;"
	"	move.w #320,%%d0;"
	"	move.w %2,%%d1;"
	"	moveq #0,%%d2;"
	"	moveq #4,%%d3;"
	"	moveq #1,%%d4;"
	"	move.l %%a6,-(%%sp);"
	"	jsr _xprn_print_string;"
	"	move.l (%%sp)+,%%a6;"
		: 
		: "g"(text), "g"(offsetaddr), "g"(c_con_linewid)
		: "%%d0", "%%d1", "%%d2", "%%d3", "%%d4", "%%d5", "%%d6", "%%d7", 
		  "%%a0", "%%a1", "%%a2", "%%a3", "%%a4", "%%a5",
		  "cc"
	);


/*
	// todo: specify args as explicit registers instead of copying them
	// todo: can just move the whole console update to 68k
	__asm__ __volatile__
	(
	"	move.l %0,%%a0;"
	"	move.l %1,%%a1;"
	"	move.l %2,%%d0;"
	"	move.l %3,%%d1;"
	"	jsr _AGT_ConsoleText_68k;"
		: 
		: "g"(text), "g"(fb), "g"(c), "g"(r)
		: "%%d0", "%%d1", "%%d2", "%%a0", "%%a1", "%%a2", 
		  "cc"
	);
*/
}

void AGT_ConsoleUpdate_(void* fb1);

//long nf_print(const char *text);

#define AGT_CONSOLE_FONTHEIGHT (12)
#define AGT_CONSOLE_MINIROWS (6/2)
#define AGT_CONSOLE_MAXIROWS (44/2)

#define AGT_CONSOLE_ROWS (64)
#define AGT_CONSOLE_COLUMNS (40)
#define AGT_CONSOLE_COLUMN_STRIDE (AGT_CONSOLE_COLUMNS+1)

s16 AGT_CONSOLE_activerows = AGT_CONSOLE_MAXIROWS;

char AGT_CONSOLE_chars[AGT_CONSOLE_ROWS*AGT_CONSOLE_COLUMN_STRIDE] __attribute__((aligned(2)));
s16 AGT_CONSOLE_head_row = 0;

void program_base() { }

#define EXPRESS32(_v_) \
	__asm__ __volatile__ ("move.l	%0,%%d7; illegal;" \
		: \
		: "g"(_v_) \
		: "%%d7", "cc" \
	);

#define EXPRESS16(_v_) \
	__asm__ __volatile__ ("move.w	%0,%%d7; illegal;" \
		: \
		: "g"(_v_) \
		: "%%d7", "cc" \
	);

#define EXPRESS8(_v_) \
	__asm__ __volatile__ ("move.b	%0,%%d7; illegal;" \
		: \
		: "g"(_v_) \
		: "%%d7", "cc" \
	);

void printf_console_ln(const char *text)
{
	g_console_dirty++;

	char* row = &AGT_CONSOLE_chars[AGT_CONSOLE_head_row * AGT_CONSOLE_COLUMN_STRIDE];

	// limit row fill to column count
	int tl = strlen(text);
	if (tl > AGT_CONSOLE_COLUMNS)
		tl = AGT_CONSOLE_COLUMNS;

	// find remaining columns
	int rl = AGT_CONSOLE_COLUMNS-tl;

	// fill row with text chars
	memcpy(row, text, tl);

	if (rl > 0)
		memset(&row[tl], ' ', rl);

	row[AGT_CONSOLE_COLUMNS] = 0;

	// next row, wrap
	AGT_CONSOLE_head_row++; 
	if (AGT_CONSOLE_head_row >= AGT_CONSOLE_ROWS)
		AGT_CONSOLE_head_row -= AGT_CONSOLE_ROWS;

	// force immediate update to physical fb, since next refresh could be some time away
	// note: can output to console even if not displaying yet - keeps record of early prints during boot
//	if (console_base)
//		AGT_ConsoleUpdate_(console_base + /*184*/c_con_linewid);
}

void AGT_Sys_ConsoleRefresh()
{
	if (0 == g_lock_consolerefresh)
		if (g_console_dirty)
			if (console_base)
				AGT_ConsoleUpdate_(console_base + /*184*/c_con_linewid);
}

#define AGT_CONSOLE_LINESPACE (AGT_CONSOLE_COLUMN_STRIDE)
#define AGT_CONSOLE_LINEMAX (AGT_CONSOLE_LINESPACE-1)
static char s_linebuf[AGT_CONSOLE_LINESPACE];
static char *s_pline = s_linebuf;
static s16 s_linelen = 0;

void printf_console(const char *text)
{
	unsigned char c;
	char *pscanin = text;

	s16 len = s_linelen;
	char *pscanout = s_pline;
	while ((c = *pscanin++) != 0)
	{
		// trigger line flush only on LF
		if (c == 10)
		{
			// terminate & flush
			*pscanout = 0;
			printf_console_ln(s_linebuf);

			// reset line
			pscanout = s_linebuf;
			len = 0;
		}
		else if (len < AGT_CONSOLE_LINEMAX)
		{
			// record characters up to LINEMAX
			*pscanout++ = c;
			len++;
		}
	}
	s_pline = pscanout;
	s_linelen = len;

	// refresh console framebuffer on every print
	AGT_Sys_ConsoleRefresh();
	//if (0 == g_lock_consolerefresh)
	//{
	//	if (console_base)
	//		AGT_ConsoleUpdate_(console_base + /*184*/c_con_linewid);
	//}
}

void AGT_Sys_ConsoleMode(int _mode)
{
#if defined(AGT_CONFIG_DEBUG_CONSOLE)
	switch (_mode)
	{
	default:
	case 0:
		min_pconsole = 0;
		max_console = 0;
		break;
	case 1:
		min_pconsole = &console_minbase;
		max_console = 0;
		break;
	case 2:
		min_pconsole = 0;
		max_console = console_maxbase;
		break;
	};
#endif
}

agt_used void SYS_Assert(char *_msg)
{
	AGT_Sys_ConsoleMode(/*AGTConsole_MINIMISED*/1);
#if (AGT_CONFIG_ALLOW_PRINT)
	agtprintf("%s\n", _msg);
#endif
	while (1) { }
	//__builtin_unreachable;
}
//error: ent %d id %d frame %d > max %d

#if defined(AGT_CONFIG_DEBUG_CONSOLE)
void SYS_ConsoleInit()
{
	s32 size = (/*184*/c_con_linewid*(((AGT_CONSOLE_MAXIROWS+1)*AGT_CONSOLE_FONTHEIGHT)+1))+256;
	console_base = malloc(size);
	memset(console_base, 0, size);

	// todo: decouple for free at end
	console_base = (char*)(((int)console_base + 255) & ~255);

	console_minbase = console_base + 
		c_con_linewid * ((AGT_CONSOLE_FONTHEIGHT*(AGT_CONSOLE_MAXIROWS-AGT_CONSOLE_MINIROWS))+1);
	console_maxbase = console_base;

	s16 r = 0;
	for (; r < AGT_CONSOLE_ROWS; ++r)
	{
		char* row = &AGT_CONSOLE_chars[r * AGT_CONSOLE_COLUMN_STRIDE];
		memset(row, 0, AGT_CONSOLE_COLUMN_STRIDE);
	}
}
#endif

void AGT_ConsoleUpdate_(void* fb1)
{
	g_console_dirty = 0;

#if defined(AGT_CONFIG_DEBUG_CONSOLE)
	s16 r = 0;
	s16 h = (AGT_CONSOLE_head_row + AGT_CONSOLE_ROWS) - AGT_CONSOLE_activerows;
	for (; r < AGT_CONSOLE_activerows; ++r)
	{
		if (h >= AGT_CONSOLE_ROWS)
			h -= AGT_CONSOLE_ROWS;

		char* row = &AGT_CONSOLE_chars[h * AGT_CONSOLE_COLUMN_STRIDE];

		AGT_ConsoleText(row, fb1, 0, r);

		h++;
	}
#endif
}

typedef struct exstackframe_s
{
	u16 sr;
	u32 pc;
	u16 vec;

} exstackframe_t;

//

typedef exstackframe_t exstackframe0000_t;
typedef exstackframe_t exstackframe0001_t;

void AGT_ExceptionHandler_0000(exstackframe0000_t* _pstack)
{
#if (AGT_CONFIG_ALLOW_PRINT)
	agtprintf("FMT0 @ (PC:%08x, VEC:%04x)\n", 
		_pstack->pc,
		_pstack->vec
	);
	agtprintf("REL: (PC:%08x)\n", 
		_pstack->pc-(u32)(&program_base)
	);
#endif
}

void AGT_ExceptionHandler_0001(exstackframe0001_t* _pstack)
{
#if (AGT_CONFIG_ALLOW_PRINT)
	agtprintf("FMT1 @ (PC:%08x, VEC:%04x)\n", 
		_pstack->pc,
		_pstack->vec
	);
	agtprintf("REL: (PC:%08x)\n", 
		_pstack->pc-(u32)(&program_base)
	);
#endif
}

//

typedef struct exstackframe0010_s
{
	u16 sr;
	u32 pc;
	u16 vec;
	//
	u32 ins;

} exstackframe0010_t;

void AGT_ExceptionHandler_0010(exstackframe0010_t* _pstack)
{
#if (AGT_CONFIG_ALLOW_PRINT)
	agtprintf("FMT2 @ %08x (PC:%08x, VEC:%04x)\n", 
		_pstack->ins,
		_pstack->pc,
		_pstack->vec
	);
	agtprintf("REL: %08x (PC:%08x)\n", 
		_pstack->ins-(u32)(&program_base),
		_pstack->pc-(u32)(&program_base)
	);
#endif
}

//

typedef struct exstackframe1001_s
{
	u16 sr;
	u32 pc;
	u16 vec;
	//
	u32 ins;
	u16 ir[4];

} exstackframe1001_t;


void AGT_ExceptionHandler_1001(exstackframe1001_t* _pstack)
{
#if (AGT_CONFIG_ALLOW_PRINT)
	agtprintf("FMT9 @ %08x (PC:%08x, VEC:%04x)\n", 
		_pstack->ins,
		_pstack->pc,
		_pstack->vec
	);
	agtprintf("REL: %08x (PC:%08x)\n", 
		_pstack->ins-(u32)(&program_base),
		_pstack->pc-(u32)(&program_base)
	);
#endif
}

//

typedef struct exstackframe1010_s
{
	u16 sr;
	u32 pc;
	u16 vec;
	//
	u16 ir0;
	u16 ssw;
	u16 ipsc;
	u16 ipsb;
	u32 dcfa;
	u16 ir1;
	u16 ir2;
	u32 dob;
	u16 ir3;
	u16 ir4;

} exstackframe1010_t;

void AGT_ExceptionHandler_1010(exstackframe1010_t* _pstack)
{
	// addr/bus error on instruction boundary
#if (AGT_CONFIG_ALLOW_PRINT)
	agtprintf("SBERR @ %08x (PC:%08x,SSW:%04x,VEC:%04x)\n", 
		_pstack->dcfa, _pstack->pc, _pstack->ssw, _pstack->vec);

	agtprintf("REL: %08x (PC:%08x)\n", 
		_pstack->dcfa-(u32)(&program_base), 
		_pstack->pc-(u32)(&program_base)
	);
#endif
}

typedef struct exstackframe1011_s
{
	u16 sr;
	u32 pc;
	u16 vec;
	//
	u16 ir0;
	u16 ssw;
	u16 ipsc;
	u16 ipsb;
	u32 dcfa;
	u16 ir1;
	u16 ir2;
	u32 dob;
	u16 ir3;
	u16 ir4;
	u16 ir5;
	u16 ir6;
	u32 sba;
	u16 ir7;
	u16 ir8;
	u32 dib;
	u16 ir9;
	u16 ir10;
	u16 ir11;
	u16 version;
	u16 irpad[18];

} exstackframe1011_t;

void AGT_ExceptionHandler_1011(exstackframe1011_t* _pstack)
{
	// addr/bus error while instruction executes
	int i;

#if (AGT_CONFIG_ALLOW_PRINT)
	agtprintf("LBERR @ %08x (PC:%08x,SSW:%04x,VEC:%04x)\n", 
		_pstack->dcfa, _pstack->pc, _pstack->ssw, _pstack->vec);
	agtprintf("REL: %08x (PC:%08x)\n", 
		_pstack->dcfa-(u32)(&program_base), 
		_pstack->pc-(u32)(&program_base));
#endif

	return;

	u16* pcallstack = (u16*)(((u8*)_pstack)+sizeof(exstackframe1011_t));

	for (i = 0; i < 4; ++i)
	{
		int val = *(u32*)pcallstack;

		if (val >= (u32)(&program_base) &&
			val < (u32)(&etext))
		{
#if (AGT_CONFIG_ALLOW_PRINT)
			agtprintf("%08x", val - (u32)(&program_base));
#endif
			pcallstack += 4;
		}
		else
		{
			pcallstack += 2;
		}
	}
}

void AGT_ExceptionHandler(exstackframe_t* _pstack)
{
	s16 type;
	u32 *psavedn, *psavean; 

	AGT_CONSOLE_activerows = 20;
	
	type = _pstack->vec >> (16-4);

	switch (type)
	{
	case 0:  AGT_ExceptionHandler_0000((exstackframe0000_t*)_pstack); break;
	case 1:  AGT_ExceptionHandler_0001((exstackframe0001_t*)_pstack); break;
	case 2:  AGT_ExceptionHandler_0010((exstackframe0010_t*)_pstack); break;
	case 9:  AGT_ExceptionHandler_1001((exstackframe1001_t*)_pstack); break;
	case 10: AGT_ExceptionHandler_1010((exstackframe1010_t*)_pstack); break;
	case 11: AGT_ExceptionHandler_1011((exstackframe1011_t*)_pstack); break;
	}

	psavedn = (u32*)0x384UL;
	psavean = (u32*)0x3a4UL;

#if (AGT_CONFIG_ALLOW_PRINT)
	agtprintf("d0:%08x a0:%08x\n", *psavedn++, *psavean++);	
	agtprintf("d1:%08x a1:%08x\n", *psavedn++, *psavean++);	
	agtprintf("d2:%08x a2:%08x\n", *psavedn++, *psavean++);	
	agtprintf("d3:%08x a3:%08x\n", *psavedn++, *psavean++);	
	agtprintf("d4:%08x a4:%08x\n", *psavedn++, *psavean++);	
	agtprintf("d5:%08x a5:%08x\n", *psavedn++, *psavean++);	
	agtprintf("d6:%08x a6:%08x\n", *psavedn++, *psavean++);	
	agtprintf("d7:%08x a7:%08x\n", *psavedn++, *psavean++);	
	agtprintf("usp: %08x\n", *(u32*)(0x3c8UL));	
//	agtprintf("cacr:%08x\n", *(u32*)(0x3c4UL));	
//	agtprintf("caar:%08x\n", *(u32*)(0x3ccUL));	
#endif

//	while (1) { }
}

#endif

#if (AGT_CONFIG_ALLOW_PRINT)
int agtprintf(const char *pFormat, ...)
{
	int len = 0;

#if (1)

	char buf[4096];
	va_list pArg;
	va_start(pArg, pFormat);
	len = vsnprintf(buf, 1000, pFormat, pArg);
	va_end(pArg);
#if defined(HATARI_NF_PRINTF)
	// always route to Hatari NF console if enabled and detected
	if (g_hnf_ok)
		nf_print(buf);
#endif

#ifdef AGT_CONFIG_STEEM_DEBUG
	reg32(FFC1F0) = buf;
#endif

	// route to TOS console if display is not locked for graphics
	if (!g_displaylocked)
	{
#if defined(HATARI_NF_PRINTF)
		// if using NATFEATS debugging, route to BCONOUT when NATFEATS
		// is not initialised. this catches early/late prints during
		// static initialisation etc.
		if (!g_hnf_ok)
		{
#endif
		char *pscan = buf;
		while (*pscan)
		{
			unsigned short c = *pscan++;
			if (c == 10) 
				Bconout(2, 13);
			Bconout(2, c);
		}
#if defined(HATARI_NF_PRINTF)
	}
#endif
	}

	// always route to BM console
	printf_console(buf);

#endif

	return len;
}
#else
int agtprintf(const char *pFormat, ...)
{
	return 0;
}
#endif