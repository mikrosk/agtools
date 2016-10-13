//======================================================================================================================
//	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================

//----------------------------------------------------------------------------------------------------------------------
//	example: minimal program to test GCC compiler - prints message and exits
//----------------------------------------------------------------------------------------------------------------------
//	controls: none
//----------------------------------------------------------------------------------------------------------------------
//	notes:
//----------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------
//	system headers

#include <mint/sysbind.h>
#include <stdio.h>

// ---------------------------------------------------------------------------------------------------------------------
//	Our AGT headers

#include "agtsys/common_cpp.h"

// basic memory interface
#include "agtsys/ealloc.h"
//#include "agtsys/compress.h"

// machine stuff
#include "agtsys/system.h"

// display stuff
//#include "agtsys/shifter.h"

// keyboard, joysticks & mouse
//#include "agtsys/input.h"

// background stuff
//#include "agtsys/tileset.h"
//#include "agtsys/worldmap.h"
//#include "agtsys/playfield.h"
//#include "agtsys/arena.h"

// sprite stuff
#include "agtsys/spritesheet.h"
#include "agtsys/slabsheet.h"
//#include "agtsys/spritelib.h"

// entity/gameobject interface
#include "agtsys/entity.h"

// ---------------------------------------------------------------------------------------------------------------------7
// some things we currently need to define even if we don't use them

entity_t *g_pe_viewport = NULL;

char ascii2font[256];

// ---------------------------------------------------------------------------------------------------------------------
//	machine state interface (setup/shutdown)

machine machinestate;

// ---------------------------------------------------------------------------------------------------------------------
//	MiNTlib builds need need to define a local superstack. otherwise, AGT does it during CRT startup.
// ---------------------------------------------------------------------------------------------------------------------

S_SUPER_SSP(AGT_CONFIG_STACK);

// =====================================================================================================================
//	program start
// =====================================================================================================================

int main(int argc, char **argv)
{
	// ---------------------------------------------------------------------------------------------------------------------
	//	switch to supervisor if still in user mode
	// ---------------------------------------------------------------------------------------------------------------------

	S_SUPER;

	// ---------------------------------------------------------------------------------------------------------------------
	//	we can detect Hatari via natfeats API for debug-printf routing, if Hatari started with '--natfeats yes'
	// ---------------------------------------------------------------------------------------------------------------------
#if defined(HATARI_NF_PRINTF)
	nf_init();

	if (g_hnf_ok)
	{
		printf("Hatari NATFEATS detected!\n");
	}
	else
	{
		printf("Hatari NATFEATS NOT detected... :(\n");
	}
#endif

	// ---------------------------------------------------------------------------------------------------------------------
	//	locate the end of the program to help track memory usage, when USE_MEMTRACE is enabled

	g_tracked_allocation_ += (int)(&end);
#if defined(USE_MEMTRACE)
	printf("ealloc: [%d] initial committed\n", g_tracked_allocation_);
#endif

	// ---------------------------------------------------------------------------------------------------------------------
	//	welcome message

	printf("[A]tari [G]ame [T]ools\n");

	printf("Hi World! Compiler tools are good!\n");

	// ---------------------------------------------------------------------------------------------------------------------
	//	do something...

	for (s16 n = 0; n < 20; n++)
	{
		u16 r = (n ^ 13219) * (n + 31773);
		printf("pseudorandom: %d\n", r);
	}

	printf("press space to exit...\n");

	// wait for key using BIOS (we haven't installed our AGT input services - TOS still in charge)
	Crawcin();

	// ---------------------------------------------------------------------------------------------------------------------
	//	back to user mode
	// ---------------------------------------------------------------------------------------------------------------------

	S_USER;

	// ---------------------------------------------------------------------------------------------------------------------

	pprintf("terminate...\n");
}


// =====================================================================================================================
// empty entity dictionary (required)
// =====================================================================================================================

entitydef_t entity_dictionary[] =
{
};
