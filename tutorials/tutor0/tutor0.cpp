//======================================================================================================================
//	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================

//----------------------------------------------------------------------------------------------------------------------
//	tutorial: Starting point
//----------------------------------------------------------------------------------------------------------------------
//	controls: none
//----------------------------------------------------------------------------------------------------------------------
//	notes:
//	+	stripped down makefile containing necessary build rules for one program, with few extras
//	+	single sourcefile tutor0.cpp implements the 'program'
//	+	'tutor0' does the following:
//		-	defines program entrypoint: main()
//		-	ensure SUPERVISOR mode, if not already configured (e.g. if using MiNTlib). needed for HW register access.
//		-	print welcome message via BIOS
//		-	configure() AGT sytems for the detected machine type, and report findings
//		-	cycle some colours via palette register 0
//		-	wait for keypress, return to USER mode, exit back to TOS.
//	-	only the minimum required AGT systems are pulled in via #include's - the order of these is important!
//
//	additional:
//	+	not an ideal starting point for a real project - see tutor4 onwards for a more realistic example
//	+	intended to outline a bare minimum for a valid AGT program (if not a useful one!)
//	+	a few dummy objects need to be defined for AGT to compile. some of these will disappear in future cleanups.
//		- a pointer to the active viewport: g_pe_viewport
//		- the entity dictionary: entity_dictionary[]

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

// ---------------------------------------------------------------------------------------------------------------------
//	hardware interfaces

// interface to hardware claim (for save/restore & clean exit)
machine machinestate;

// ---------------------------------------------------------------------------------------------------------------------
//	MiNTlib builds need need to define a local superstack. otherwise, AGT does it during CRT startup.
// ---------------------------------------------------------------------------------------------------------------------

S_SUPER_SSP(AGT_CONFIG_STACK);

AGT_MAIN;

// =====================================================================================================================
//	program entrypoint
// =====================================================================================================================

int AGT_EntryPoint()
{
	// ---------------------------------------------------------------------------------------------------------------------
	//	welcome message

	printf("[A]tari [G]ame [T]ools / dml 2017\n");

	printf("Starting point\n");

	// ---------------------------------------------------------------------------------------------------------------------
	//	detect hardware & configure AGT

	machinestate.configure();

	// ---------------------------------------------------------------------------------------------------------------------
	//	report findings

	if (bFalcon030)
	{	printf("machine: Falcon0x0\n"); }
	else
	if (bMegaSTE)
	{	printf("machine: MegaSTE\n"); }
	else
	if (bSTE)
	{	printf("machine: STE\n"); }
	else
	if (bSTF)
	{	printf("machine: STF\n"); }
	else
	{
		printf("machine: unknown, so assuming STE (?!)...\n");
		bSTE = true;
	}

	// ---------------------------------------------------------------------------------------------------------------------
	//	simple program to flash some colours

	{

		printf("press space to start...\n");
		// wait for key using BIOS (we haven't installed our AGT input services - TOS still in charge)
		Crawcin();

		// save bg colour
		s16 bgcol = reg16(FFFF8240);

		// cycle colour 0 for a while
		for (s32 flash = 0; flash < 200000; flash++)
		{
			reg16(FFFF8240) += 0x237; 
		}

		// restore bg colour
		reg16(FFFF8240) = bgcol;

		printf("press space to exit...\n");

		// wait for key
		Crawcin();

	}

	// ---------------------------------------------------------------------------------------------------------------------
	// back to TOS

	pprintf("terminate...\n");

	return 0;
}


// =====================================================================================================================
// empty entity dictionary (required)
// =====================================================================================================================

entitydef_t entity_dictionary[] =
{
};
