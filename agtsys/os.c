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

// ---------------------------------------------------------------------------------------------------------------------
//	Our AGT headers

// ---------------------------------------------------------------------------------------------------------------------

short g_displaylocked = 0;

#if (1)

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;

extern int bmprintf(const char *pFormat, ...);


//#include "common_c.h"

//short bytewidth = 184;//320*2;
char* console_base = 0;	
char* show_console = 0;	
extern void etext();

/* used by bmengine.a */
//int c_dpf(const char* const string)
//{
//	return printf("%s", string);
//}

static __inline void AGT_ConsoleText(const char* text, void* fb, int c, int r)
{
	// minmal shim to 68k version
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
}

//extern void AGT_ConsoleUpdate(void* fb1, void* fb2);

void nf_print(const char *text);

#define AGT_CONSOLE_ACTIVEROWS (6)
#define AGT_CONSOLE_ROWS (32)
#define AGT_CONSOLE_COLUMNS (40)
#define AGT_CONSOLE_COLUMN_STRIDE (AGT_CONSOLE_COLUMNS+1)

s16 AGT_CONSOLE_activerows = AGT_CONSOLE_ACTIVEROWS;

char AGT_CONSOLE_chars[AGT_CONSOLE_ROWS*AGT_CONSOLE_COLUMN_STRIDE] __attribute__((aligned(2)));
s16 AGT_CONSOLE_head_row = 0;

void program_base() { }

void printf_console(const char *text)
{
	char* row = &AGT_CONSOLE_chars[AGT_CONSOLE_head_row * AGT_CONSOLE_COLUMN_STRIDE];

	// limit row fill to column count
	int tl = strlen(text);
	if (tl > AGT_CONSOLE_COLUMNS)
		tl = AGT_CONSOLE_COLUMNS;

	// find remaining columns
	int rl = AGT_CONSOLE_COLUMNS-tl;

	// fill row with text chars
	memcpy(row, text, tl);
	row += tl;

	// fill row ends to remove stale chars from framebuffer
	if (rl > 0)
		memset(row, ' ', rl);
	row += rl;

	// terminate row
	*row = 0;

	// next row, wrap
	AGT_CONSOLE_head_row++; 
	if (AGT_CONSOLE_head_row >= AGT_CONSOLE_ROWS)
		AGT_CONSOLE_head_row -= AGT_CONSOLE_ROWS;

	// force immediate update to physical fb, since next refresh could be some time away
	// note: can output to console even if not displaying yet - keeps record of early prints during boot
	if (/*g_displaylocked && */console_base)
		AGT_ConsoleUpdate(console_base + 184);
}

void SYS_Assert(char *_msg)
{
	show_console = console_base;
	bmprintf("%s\n", _msg);
	while (1) { }
	__builtin_unreachable;
}
//error: ent %d id %d frame %d > max %d

#if defined(AGT_CONFIG_DEBUG_CONSOLE)
void SYS_ConsoleInit()
{
	console_base = malloc((184*(((AGT_CONSOLE_ACTIVEROWS+1)*6)+1))+256);
	console_base = (char*)(((int)console_base + 255) & ~255);

	s16 r = 0;
	for (; r < AGT_CONSOLE_ROWS; ++r)
	{
		char* row = &AGT_CONSOLE_chars[r * AGT_CONSOLE_COLUMN_STRIDE];
		memset(row, 0, AGT_CONSOLE_COLUMN_STRIDE);
	}
}
#endif

void AGT_ConsoleUpdate(void* fb1)
{
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
	bmprintf("FMT0 @ (PC:%08x, VEC:%04x)\n", 
		_pstack->pc,
		_pstack->vec
	);
	bmprintf("REL: (PC:%08x)\n", 
		_pstack->pc-(u32)(&program_base)
	);
}

void AGT_ExceptionHandler_0001(exstackframe0001_t* _pstack)
{
	bmprintf("FMT1 @ (PC:%08x, VEC:%04x)\n", 
		_pstack->pc,
		_pstack->vec
	);
	bmprintf("REL: (PC:%08x)\n", 
		_pstack->pc-(u32)(&program_base)
	);
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
	bmprintf("FMT2 @ %08x (PC:%08x, VEC:%04x)\n", 
		_pstack->ins,
		_pstack->pc,
		_pstack->vec
	);
	bmprintf("REL: %08x (PC:%08x)\n", 
		_pstack->ins-(u32)(&program_base),
		_pstack->pc-(u32)(&program_base)
	);
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
	bmprintf("FMT9 @ %08x (PC:%08x, VEC:%04x)\n", 
		_pstack->ins,
		_pstack->pc,
		_pstack->vec
	);
	bmprintf("REL: %08x (PC:%08x)\n", 
		_pstack->ins-(u32)(&program_base),
		_pstack->pc-(u32)(&program_base)
	);
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

	bmprintf("SBERR @ %08x (PC:%08x,SSW:%04x,VEC:%04x)\n", 
		_pstack->dcfa, _pstack->pc, _pstack->ssw, _pstack->vec);

	bmprintf("REL: %08x (PC:%08x)\n", 
		_pstack->dcfa-(u32)(&program_base), 
		_pstack->pc-(u32)(&program_base)
	);
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

	bmprintf("LBERR @ %08x (PC:%08x,SSW:%04x,VEC:%04x)\n", 
		_pstack->dcfa, _pstack->pc, _pstack->ssw, _pstack->vec);
	bmprintf("REL: %08x (PC:%08x)\n", 
		_pstack->dcfa-(u32)(&program_base), 
		_pstack->pc-(u32)(&program_base));

	return;

	u16* pcallstack = ((u8*)_pstack)+sizeof(exstackframe1011_t);

	for (i = 0; i < 4; ++i)
	{
		int val = *(u32*)pcallstack;

		if (val >= (u32)(&program_base) &&
			val < (u32)(&etext))
		{
			bmprintf("%08x", val - (u32)(&program_base));
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

	bmprintf("d0:%08x a0:%08x\n", *psavedn++, *psavean++);	
	bmprintf("d1:%08x a1:%08x\n", *psavedn++, *psavean++);	
	bmprintf("d2:%08x a2:%08x\n", *psavedn++, *psavean++);	
	bmprintf("d3:%08x a3:%08x\n", *psavedn++, *psavean++);	
	bmprintf("d4:%08x a4:%08x\n", *psavedn++, *psavean++);	
	bmprintf("d5:%08x a5:%08x\n", *psavedn++, *psavean++);	
	bmprintf("d6:%08x a6:%08x\n", *psavedn++, *psavean++);	
	bmprintf("d7:%08x a7:%08x\n", *psavedn++, *psavean++);	
	bmprintf("usp: %08x\n", *(u32*)(0x3c8UL));	
//	bmprintf("cacr:%08x\n", *(u32*)(0x3c4UL));	
//	bmprintf("caar:%08x\n", *(u32*)(0x3ccUL));	

//	while (1) { }
}

#endif

int bmprintf(const char *pFormat, ...)
{
	int len = 0;

#if (1)

	char buf[4096];
	va_list pArg;
	va_start(pArg, pFormat);
	len = vsnprintf(buf, 1000, pFormat, pArg);
	va_end(pArg);
#ifdef HATARI_NF_PRINTF
	// always route to Hatari NF console if enabled and detected
	if (g_hnf)
		nf_print(buf);
#endif

	// route to TOS console
#ifndef HATARI_NF_PRINTF
	if (!g_displaylocked)
	{
		char *pscan = buf;
		while (*pscan)
		{
			unsigned short c = *pscan++;
			if (c == 10) 
				Bconout(2, 13);
			Bconout(2, c);
		}
	}
#endif

	// always route to BM console
	printf_console(buf);

#endif

	return len;
}

