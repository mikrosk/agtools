//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
//	system-level config, save/restore
//----------------------------------------------------------------------------------------------------------------------

#ifndef system_h_
#define system_h_

// ---------------------------------------------------------------------------------------------------------------------

class machine
{
	bool open_;

public:

	machine()
		: open_(false)
	{
	}

	~machine()
	{
	}

	// publish system-specific metrics (e.g. machine type) which might be needed by other services
	void configure();

	// save full machine state
	void save();
	
	// take ownership over HW vectors & interrupt state
	// each individual service must then configure any interrupts needed
	void claim();

	// restore machine state for clean exit
	void restore();
};

// ---------------------------------------------------------------------------------------------------------------------
// debugging helpers

static inline void halt() { while (1) { } }

// ---------------------------------------------------------------------------------------------------------------------
// A nonzero value signals the display is locked & owned by AGT, preventing any C printfs from reaching the screen. 
// note: NATFEATS debugging continues to work in this mode!

extern "C" short g_displaylocked;

// ---------------------------------------------------------------------------------------------------------------------
// Hatari NATFEATS interface was detected if nonzero

extern "C" short g_hnf_ok;

// ---------------------------------------------------------------------------------------------------------------------
// machine detection

extern "C" u8 bSTF;
extern "C" u8 bSTE;
extern "C" u8 bTT;
extern "C" u8 bMegaSTE;
extern "C" u8 bFalcon030;

// ---------------------------------------------------------------------------------------------------------------------
// Hatari NATFEATS interface for 'remote' console/printf debugging without touching ST's display

#if defined(HATARI_NF_PRINTF)
extern "C" 
{
	#include "3rdparty/natfeats.h"
}
#endif

// ---------------------------------------------------------------------------------------------------------------------
// supervisor/user & USP/SSP management stuff for MiNTlib-specific builds. 'minimal/tiny crt' builds don't require this

#if !defined(USE_TINY_CRT)
#define S_SUPER_SSP(_sz_) \
static const int ssp_size = _sz_>>2; \
u32 s_new_ssp[ssp_size]; \
u32 s_old_ssp; \
u32 s_old_usp;
#else
#define S_SUPER_SSP(_sz_)
#endif

#if !defined(USE_TINY_CRT)
#define S_SUPER \
	__asm__ __volatile__ ("move.l	%%sp,%0;" \
		: "=m"(s_old_usp) \
		: \
		: "cc" \
	); \
	s_old_ssp = Super(&s_new_ssp[ssp_size-64]);
#else
#define S_SUPER
#endif

#if !defined(USE_TINY_CRT)
#define S_USER \
	dprintf("usermode...\n"); \
	Super(s_old_ssp); \
	__asm__ __volatile__ ("move.l	%0,%%sp;" \
		: \
		: "m"(s_old_usp) \
		: "cc" \
	);
#else
#define S_USER
#endif

// ---------------------------------------------------------------------------------------------------------------------
// defines program entrypoint main(argc,argv) and executes AGT_EntryPoint in supervisor mode
// ---------------------------------------------------------------------------------------------------------------------
// notes:
// ---------------------------------------------------------------------------------------------------------------------
// - USE_TINY_CRT employs AGT's own thin CRT which selects supervisor mode early, so the super
//   switch is omitted from this stage when using this mode. it is retained for compatibility
//   with LIBC variants like MiNTlib.
// - Supexec() is preferred because Super() changes the active stack pointer in the current
//   context,  which in turn messes up the compiled C code's stack frame when combined with
//   -fomit-frame-pointer.

extern "C" int AGT_EntryPointPre();
extern "C" int AGT_EntryPointPost();
extern "C" int AGT_EntryPoint();

#if !defined(USE_TINY_CRT)
#define AGT_MAIN								\
void AGT_SuperStart()							\
{												\
	__asm__ __volatile__						\
	(											\
		"										\
		move.l		%0,%%a0;					\
		move.l		%%a0,%%d0;					\
		subq.l		#4,%%d0;					\
		and.w		#-16,%%d0;					\
		move.l		%%d0,%%a0;					\
		move.l		%%sp,-(%%a0);				\
		move.l		%%usp,%%a1;					\
		move.l		%%a1,-(%%a0);				\
		move.l		%%a0,%%sp;					\
		movem.l		%%d1-%%d7/%%a2-%%a6,-(%%sp);	\
		jsr			(%2);							\
		movem.l		(%%sp)+,%%d1-%%d7/%%a2-%%a6;	\
		movem.l		%%d1-%%d7/%%a2-%%a6,-(%%sp);	\
		jsr			(%1);							\
		movem.l		(%%sp)+,%%d1-%%d7/%%a2-%%a6;	\
		movem.l		%%d1-%%d7/%%a2-%%a6,-(%%sp);	\
		jsr			(%3);							\
		movem.l		(%%sp)+,%%d1-%%d7/%%a2-%%a6;	\
		move.l		(%%sp)+,%%a0;				\
		move.l		%%a0,%%usp;					\
		move.l		(%%sp)+,%%sp;				\
		"										\
		:										\
		: "p"(&s_new_ssp[ssp_size-16]), "a"(&AGT_EntryPoint), "a"(&AGT_EntryPointPre), "a"(&AGT_EntryPointPost) \
		: "%%d0", "%%a0", "%%a1", "cc"			\
	);											\
}												\
int main(int argc, char **argv)					\
{												\
	Supexec(&AGT_SuperStart, 0,0,0,0,0);		\
	return 0;									\
}
#else // !defined(USE_TINY_CRT)
#define AGT_MAIN								\
int main(int argc, char **argv)					\
{												\
	AGT_EntryPointPre();						\
	int r = AGT_EntryPoint();					\
	AGT_EntryPointPost();						\
	return r;									\
}
#endif // !defined(USE_TINY_CRT)

// ---------------------------------------------------------------------------------------------------------------------
// access to timing rasters from C code

#ifdef TIMING_RASTERS_MAIN
#define TIMING_MAIN(_rgb_) { reg16(00000064) = _rgb_; reg16(ffff8240) = _rgb_; }
#else
#define TIMING_MAIN(_rgb_)
#endif		

#ifdef TIMING_RASTERS
#define TIMING(_rgb_) { reg16(00000064) = _rgb_; reg16(ffff8240) = _rgb_; }
#else
#define TIMING(_rgb_)
#endif		

// ---------------------------------------------------------------------------------------------------------------------
// estimate of program end address, for memory tracking

extern int end;

// ---------------------------------------------------------------------------------------------------------------------

#endif // system_h_
