//======================================================================================================================
//	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================

//----------------------------------------------------------------------------------------------------------------------
//	tutorial: Sprites
//----------------------------------------------------------------------------------------------------------------------
//	controls: none
//----------------------------------------------------------------------------------------------------------------------
//	notes:
//	+	incremental changes from 'tutor3a'
//
//	additional:

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
#include "agtsys/shifter.h"

// keyboard, joysticks & mouse
//#include "agtsys/input.h"

// background stuff
#include "agtsys/tileset.h"
#include "agtsys/worldmap.h"
#include "agtsys/playfield.h"
#include "agtsys/arena.h"

// sprite stuff
#include "agtsys/spritesheet.h"
#include "agtsys/slabsheet.h"
#include "agtsys/spritelib.h"

// entity/gameobject interface
#include "agtsys/entity.h"

// ---------------------------------------------------------------------------------------------------------------------7
// some things we currently need to define even if we don't use them

entity_t *g_pe_viewport = NULL;

char ascii2font[256];

// ---------------------------------------------------------------------------------------------------------------------
//	hardware interfaces

// interface to hardware claim (for save/restore & clean exit)
machine machinestate;

// interface to STE display services
shifter_ste shift;

// ---------------------------------------------------------------------------------------------------------------------
//	MiNTlib builds need need to define a local superstack. otherwise, AGT does it during CRT startup.
// ---------------------------------------------------------------------------------------------------------------------

S_SUPER_SSP(AGT_CONFIG_STACK);

AGT_MAIN;

// ---------------------------------------------------------------------------------------------------------------------
//	define some space for the display.

#define SCREEN_LINES (240)

// ---------------------------------------------------------------------------------------------------------------------
//	configure our scrolling playfield system

// define a playfield config for vertical-only scrolling 
typedef playfield_template
<
    /*tilesize=*/4,
    /*max_scroll=*/8192,
    /*vscroll=*/VScrollMode_LOOPBACK,
    /*hscroll=*/HScrollMode_NONE,
    /*restore=*/RestoreMode_NONE,
    /*speculative=*/true,
    /*ste_hwscroll=*/true,
    /*stf_hsyncshift=*/false,
    /*blitter=*/false
> playfield_vscroll_ste;

// define an arena (world) composed of 2 such playfields (double-buffered)
typedef arena_template
<
    /*buffers=*/2,
    /*singleframe=*/false,
    /*playfield_type=*/playfield_vscroll_ste
> arena_vscroll_ste;

// ---------------------------------------------------------------------------------------------------------------------
//	playfield assets

// playfield tile assets
tileset mytiles;

// playfield map assets
worldmap mymap;

// ---------------------------------------------------------------------------------------------------------------------
// sprite assets

// test sprite
spritesheet sprite_asset;

// ---------------------------------------------------------------------------------------------------------------------
//	playfield world coordinates

// world map safety margin (in pixels) for speculative scrolling algorithm
static const int c_scroll_safety_margin = 16;

// viewport (scroll) starting position in world map
static s16 g_viewport_worldx = 0; 
static s16 g_viewport_worldy = c_scroll_safety_margin;

// =====================================================================================================================
//	program entrypoint
// =====================================================================================================================

int AGT_EntryPoint()
{
	// ---------------------------------------------------------------------------------------------------------------------
	//	welcome message

	printf("[A]tari [G]ame [T]ools\n");

	printf("Sprites: Basic Animation\n");


	// ---------------------------------------------------------------------------------------------------------------------
	//	detect hardware & configure AGT

	machinestate.configure();


	// ---------------------------------------------------------------------------------------------------------------------
	//	compatibility check for this example

	if (!bSTE && !bMegaSTE && !bFalcon030)
	{
		// ---------------------------------------------------------------------------------------------------------------------
		// if not one of these 3 machines, complain and bail out...

		printf("unsupported machine!\n");
		printf("press space to quit...\n");
		Crawcin();
	}
	else
	{
		// ---------------------------------------------------------------------------------------------------------------------
		//	...otherwise continue with program

		printf("press space to start...\n");
		// wait for key using BIOS (we haven't installed our AGT input services - TOS still in charge)
		Crawcin();


		// ---------------------------------------------------------------------------------------------------------------------
		//	save machine state for a clean exit

		printf("save machine state...\n");
		machinestate.save();
		


		// ---------------------------------------------------------------------------------------------------------------------
		//	shifter (display) initialisation
		//	pull any machine-specific metrics out of the display service & save state before any drawing systems are used

		printf("configure shifter...\n");
		shift.configure();
		shift.save();

		// ---------------------------------------------------------------------------------------------------------------------
		//	load map tiles

		printf("loading tiles...\n");
		mytiles.load_cct("tutor3.cct");

		// ---------------------------------------------------------------------------------------------------------------------
		//	load map
		
		printf("loading map...\n");
		mymap.load_ccm("tutor3.ccm");

		// ---------------------------------------------------------------------------------------------------------------------
		//	get map dimensions (x16 -> in pixels) to help keep scroll inside the map limits

		s16 map_xtiles = mymap.getwidth();
		s16 map_ytiles = mymap.getheight();

		s16 map_xpixels = map_xtiles << 4;
		s16 map_ypixels = map_ytiles << 4;

		// max scroll is map size minus screen dimensions (plus safety margin for the scrolling axes)
		s16 map_xlimit = (map_xpixels-320);
		s16 map_ylimit = (map_ypixels-SCREEN_LINES-c_scroll_safety_margin);

		// ---------------------------------------------------------------------------------------------------------------------
		//	create an instance of the double-buffered arena (our game world), connected to the shifter interface

		printf("creating world...\n");
		arena_vscroll_ste myworld(shift, 320, SCREEN_LINES, /*dualfield=*/false);

		// ---------------------------------------------------------------------------------------------------------------------
		//	associate map & tiles with the arena playfield(s)
	
		printf("assigning world map...\n");
		myworld.setmap(&mymap, &mymap);

		printf("assigning world tiles to each playfield buffer...\n");
		for (int b = myworld.c_num_buffers_-1; b >= 0; b--)
		{
			myworld.select_buffer(b, 0);
			myworld.settiles(&mytiles, &mytiles);
		}

		// ---------------------------------------------------------------------------------------------------------------------
		//	load sprite gfx assets
		// ---------------------------------------------------------------------------------------------------------------------
		//	note: using EMSPR sprite format - fast for smaller sprites up to 32 wide

		// load spinny disc sprite
		sprite_asset.load("spinner.ems");

		// ---------------------------------------------------------------------------------------------------------------------
		//	init world with initial viewport coordinates

		g_viewport_worldx = 0;
		g_viewport_worldy = c_scroll_safety_margin;

		printf("init world viewport...\n");
		myworld.init(g_viewport_worldx, g_viewport_worldy);

		// ---------------------------------------------------------------------------------------------------------------------
		//	now claim control over all machine resources - display, timers, input etc.

		printf("claim hardware...\n");
		machinestate.claim();


		// ---------------------------------------------------------------------------------------------------------------------
		//	initialize display, configure single- or dual-field graphics mode, load palettes for phys & log shiftfields
		// ---------------------------------------------------------------------------------------------------------------------
		// note: there are 2 shiftfields by default, even if there are more than 2 playfield buffers. The shiftfield contains
		// all HW state associated with a single frame, including the framebuffer address, palette, scroll/linewid and other
		// relevant state. They must all be committed at exactly the same time by the displayservice.
		// The logical shifstate is always the one to be written by the game loop because it will not reach the HW registers 
		// until the framebuffers are exchanged.

		printf("init shifter...\n");
		shift.set_fieldmode(/*dualfield=*/false);
		shift.init();

		// ---------------------------------------------------------------------------------------------------------------------
		//	load palette for both front and backbuffer 'shiftstates'. we use the tile library palette.

		printf("set palette...\n");	

		for (s16 c = 0; c < 16; ++c)
		{
			// logic (next)
			shift.setcolour(0, 0, c, mytiles.getcol(c));

			// physic (current)
			shift.setcolour(1, 0, c, mytiles.getcol(c));
		}

		// =====================================================================================================================
		//	mainloop setup
		// =====================================================================================================================

		// ---------------------------------------------------------------------------------------------------------------------
		//	initialise playfield-restore state before any sprites are drawn

		AGT_PlayfieldRestoreInit();

		// =====================================================================================================================
		//	mainloop
		// =====================================================================================================================

		// ---------------------------------------------------------------------------------------------------------------------
		//	some persistent variables we're going to need in the mainloop

		s16 scroll_speed = 0;			// disable scrolling
		s16 backbuffer_num = 0;			// index of current backbuffer (0 or 1)
		s16 curr_vbl = g_vbl;			// initial vblank number
		s16 animation_time = 0;

		// ---------------------------------------------------------------------------------------------------------------------
		//	start the mainloop - run for a limited time

		while (animation_time < 400)
		{
			// ---------------------------------------------------------------------------------------------------------------------
			//	wait for the current VBL period to elapse

			curr_vbl = g_vbl;
			while (g_vbl == curr_vbl) { /* spin until counter changes */ }

			TIMING_MAIN(0x700);	

			// ---------------------------------------------------------------------------------------------------------------------
			//	update the background & scroll

			// select the next workbuffer (..of two - we're using double buffering here)
			myworld.select_buffer(backbuffer_num, 0);

			// update the playfield coordinates for this draw pass
			myworld.setpos(g_viewport_worldx, g_viewport_worldy);

			// fill any background tiles required by this change in coordinates
			myworld.hybrid_fill(false);

			TIMING_MAIN(0x070);	

			// ---------------------------------------------------------------------------------------------------------------------
			// retrieve playfield [drawcontext] structure for sharing with low-level drawing functions
			// e.g.
			// - current framebuffer
			// - framebuffer scanline index (because screen size varies)
			// - dimensions
			// - vpage snapping offsets for loopback scrolling
			// - other devious details used to synchronize drawing with playfields
			// todo: this has not been properly adopted yet by some of the draw functions - changes are ongoing

			myworld.get_context(drawcontext, 0, 320, SCREEN_LINES);

			// ---------------------------------------------------------------------------------------------------------------------
			//	reset playfield-restore after sprites are drawn (it needs flushed)

			AGT_PlayfieldRestoreReset();

			// ---------------------------------------------------------------------------------------------------------------------
			//	prepare to draw EMSPR-format sprites with blitter

			AGT_BLiT_EMSprInit();

			// ---------------------------------------------------------------------------------------------------------------------
			//	draw a sprite directly to the workbuffer (backbuffer) playfield

			// draw 32 sprites in a 8 x 4 grid
			for (s16 num = 32-1; num >= 0; num--)
			{
				// get the x,y offset for each, based on the current loopcount
				s32 xo = ((num &  7) << 5) - (((8 - 1)<<5)/2);
				s32 yo = ((num >> 3) << 5) - (((4 - 1)<<5)/2);

				// draw each sprite, centred on 160,120
				AGT_BLiT_EMSprDrawClipped
				(
					/*context=*/&drawcontext,
					/*asset=*/sprite_asset.get(), 
					/*frame=*/((animation_time >> 2) + num) & 7, // careful to keep inside range 0-7
					/*worldpos=*/160 + xo, 120 + yo
				);
			}

			// animate the sprite
			animation_time++;

			TIMING_MAIN(0x007);	

			// ---------------------------------------------------------------------------------------------------------------------
			//	present the current workbuffer as the new frontbuffer

			myworld.activate_buffer();

			// ---------------------------------------------------------------------------------------------------------------------
			//	toggle buffers so next time we write to the new backbuffer

			backbuffer_num = backbuffer_num ^ 1;

			// ---------------------------------------------------------------------------------------------------------------------
			//	update the scroll coordinates

			g_viewport_worldy += scroll_speed;

			// if we hit the end of the map, turn around
			if ((g_viewport_worldy >= map_ylimit) ||
				(g_viewport_worldy < c_scroll_safety_margin))
			{
				// change scroll direction
				g_viewport_worldy -= scroll_speed;
				scroll_speed = -scroll_speed;
				g_viewport_worldy += scroll_speed;
			}

			// ---------------------------------------------------------------------------------------------------------------------
			//	repeat forever...

			TIMING_MAIN(0x000);	

		}

		// =====================================================================================================================
		//	shutdown: release hardware claim back to TOS
		// =====================================================================================================================

		shift.restore();

		machinestate.restore();

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
