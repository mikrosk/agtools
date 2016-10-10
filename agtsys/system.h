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

extern short g_displaylocked;

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
static const int ssp_size = _sz_; \
u32 new_ssp[ssp_size]; \
u32 old_ssp;
#else
#define S_SUPER_SSP(_sz_)
#endif

#if !defined(USE_TINY_CRT)
#define S_SUPER \
	int old_usp; \
	__asm__ __volatile__ ("move.l	sp,%0;" \
		: "=g"(old_usp) \
		: \
		: "cc" \
	); \
	old_ssp = Super(&new_ssp[ssp_size-256]);
#else
#define S_SUPER
#endif

#if !defined(USE_TINY_CRT)
#define S_USER \
	dprintf("usermode...\n"); \
	Super(old_ssp); \
	__asm__ __volatile__ ("move.l	%0,sp;" \
		: \
		: "g"(old_usp) \
		: "cc" \
	);
#else
#define S_USER
#endif

// ---------------------------------------------------------------------------------------------------------------------
// access to timing rasters from C code

#ifdef TIMING_RASTERS_MAIN
#define TIMING_MAIN(_rgb_) { reg16(ffff8240) = _rgb_; }
#else
#define TIMING_MAIN(_rgb_)
#endif		

#ifdef TIMING_RASTERS
#define TIMING(_rgb_) { reg16(ffff8240) = _rgb_; }
#else
#define TIMING(_rgb_)
#endif		

// ---------------------------------------------------------------------------------------------------------------------
// estimate of program end address, for memory tracking

extern int end;

// ---------------------------------------------------------------------------------------------------------------------

#endif // system_h_
