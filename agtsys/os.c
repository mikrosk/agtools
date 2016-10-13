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

#if (0)

#include "common_c.h"

char console_enabled = 1;
short bytewidth = 320*2;
extern char* bm_physbase;	
extern void etext();

/* used by bmengine.a */
//int c_dpf(const char* const string)
//{
//	return printf("%s", string);
//}

extern int AGT_ConsoleText(const char* text, void* fb, int c, int r);
//extern void AGT_ConsoleUpdate(void* fb1, void* fb2);

void nf_print(const char *text);

#define AGT_CONSOLE_ACTIVEROWS (12)
#define AGT_CONSOLE_ROWS (32)
#define AGT_CONSOLE_COLUMNS (53)
#define AGT_CONSOLE_COLUMN_STRIDE (AGT_CONSOLE_COLUMNS+1)

s16 AGT_CONSOLE_activerows = AGT_CONSOLE_ACTIVEROWS;

char AGT_CONSOLE_chars[AGT_CONSOLE_ROWS*AGT_CONSOLE_COLUMN_STRIDE];
s16 AGT_CONSOLE_head_row = 0;

void program_base() { }

void bmprintf_console(const char *text)
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
	if (g_displaylocked)
		AGT_ConsoleUpdate(bm_physbase, 0);
}

void BM_E_ConsoleInit()
{
	s16 r = 0;
	for (; r < AGT_CONSOLE_ROWS; ++r)
	{
		char* row = &AGT_CONSOLE_chars[r * AGT_CONSOLE_COLUMN_STRIDE];
		memset(row, 0, AGT_CONSOLE_COLUMN_STRIDE);
	}
}

void AGT_ConsoleUpdate(void* fb1, void* fb2)
{
	s16 r = 0;
	s16 h = (AGT_CONSOLE_head_row + AGT_CONSOLE_ROWS) - AGT_CONSOLE_activerows;
	for (; r < AGT_CONSOLE_activerows; ++r)
	{
		if (h >= AGT_CONSOLE_ROWS)
			h -= AGT_CONSOLE_ROWS;

		char* row = &AGT_CONSOLE_chars[h * AGT_CONSOLE_COLUMN_STRIDE];

		if (fb1)
			AGT_ConsoleText(row, fb1, 0, r);
//		if (fb2)
//			AGT_ConsoleText(row, fb2, 0, r);

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

	printf("d0:%08x a0:%08x\n", *psavedn++, *psavean++);	
	printf("d1:%08x a1:%08x\n", *psavedn++, *psavean++);	
	printf("d2:%08x a2:%08x\n", *psavedn++, *psavean++);	
	printf("d3:%08x a3:%08x\n", *psavedn++, *psavean++);	
	printf("d4:%08x a4:%08x\n", *psavedn++, *psavean++);	
	printf("d5:%08x a5:%08x\n", *psavedn++, *psavean++);	
	printf("d6:%08x a6:%08x\n", *psavedn++, *psavean++);	
	printf("d7:%08x a7:%08x\n", *psavedn++, *psavean++);	
	printf("usp: %08x\n", *(u32*)(0x3c8UL));	
	printf("cacr:%08x\n", *(u32*)(0x3c4UL));	
	printf("caar:%08x\n", *(u32*)(0x3ccUL));	

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
//	bmprintf_console(buf);

#endif

	return len;
}

