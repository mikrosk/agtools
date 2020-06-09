//======================================================================================================================
//	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================

//----------------------------------------------------------------------------------------------------------------------
//	tutorial: The Thinking Entity
//----------------------------------------------------------------------------------------------------------------------
//	controls: none
//----------------------------------------------------------------------------------------------------------------------
//	notes:
//	+	incremental changes from 'tutor4a'
//	+	display 'Xenon' text using entities instead of sprites (as before)
//	+	implement entity 'think' or 'tick' function (fntick) which automatically moves each entity to track the viewport 
//		movement. this implements a simplistic 'screenspace' or viewport-relative coordinate system
//	+	all entities share the same tick function
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
//	Game Entity IDs
// ---------------------------------------------------------------------------------------------------------------------7
//	Notes:
//		- add new entities to this index
//		- keep entries matched 1:1 with the entity dictionary table! (entity_dictionary[])
// ---------------------------------------------------------------------------------------------------------------------7

enum EntityType : int
{
	// special viewport entity, tracks visible world
	EntityType_VIEWPORT,

	// letter objects
	EntityType_LETTER_X,
	EntityType_LETTER_e,
	EntityType_LETTER_n,
	EntityType_LETTER_o,
	
	// end!
	EntityType_MAX_
};

// ---------------------------------------------------------------------------------------------------------------------7
// some things we currently need to define even if we don't use them

entity_t *g_pe_viewport = NULL;

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
    /*restore=*/RestoreMode_PAGERESTORE,
    /*updatemode=*/UpdateMode_NORMAL,
    /*ste_hw=*/true,
    /*stf_hsyncshift=*/false,
    /*dynamic=*/false
> playfield_t;

// define an arena (world) composed of 2 such playfields (double-buffered)
typedef arena_template
<
    /*buffers=*/2,
    /*singleframe=*/false,
    /*dualfield=*/false,
    /*playfield_type=*/playfield_t
> arena_t;

// ---------------------------------------------------------------------------------------------------------------------
//	playfield assets

// playfield tile assets
tileset mytiles;

// playfield map assets
worldmap mymap;

// ---------------------------------------------------------------------------------------------------------------------
// sprite assets

// letter sprites
spritesheet X_asset;
spritesheet e_asset;
spritesheet n_asset;
spritesheet o_asset;

// ---------------------------------------------------------------------------------------------------------------------
//	playfield world coordinates

// clipping margin outside of the scrolling viewport, to accommodate the largest typical sprite before deletion etc.
static const s16 c_viewport_margin = 32;

// world map safety margin (in pixels) for speculative scrolling algorithm
static const int c_scroll_safety_margin = 16;

// viewport (scroll) starting position in world map
static s16 g_viewport_worldx = 0; 
static s16 g_viewport_worldy = c_scroll_safety_margin;

static s16 g_viewport_changeinx = 0;
static s16 g_viewport_changeiny = 0;

// =====================================================================================================================
//	program entrypoint
// =====================================================================================================================

int AGT_EntryPoint()
{
	// ---------------------------------------------------------------------------------------------------------------------
	//	welcome message

	printf("[A]tari [G]ame [T]ools / dml 2017\n");

	printf("Entities: The Thinking Entity\n");


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
		mytiles.load_cct("tutor4.cct");

		// ---------------------------------------------------------------------------------------------------------------------
		//	load map
		
		printf("loading map...\n");
		mymap.load_ccm("tutor4.ccm");

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
		arena_t myworld(shift, 320, SCREEN_LINES, /*dualfield=*/false);

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

		// load letter sprites
		X_asset.load("X.ims");
		e_asset.load("e.emx");
		n_asset.load("n.emx");
		o_asset.load("o.emx");

		// attach assets to entity types (note that 'n' is used twice, but reuse one entity 'type' for both 'instances')
		EntityDefAsset_Sprite(EntityType_LETTER_X, &X_asset);
		EntityDefAsset_Sprite(EntityType_LETTER_e, &e_asset);
		EntityDefAsset_Sprite(EntityType_LETTER_n, &n_asset);
		EntityDefAsset_Sprite(EntityType_LETTER_o, &o_asset);

		// ---------------------------------------------------------------------------------------------------------------------
		//	init world with initial viewport coordinates

		g_viewport_worldx = 0;
		g_viewport_worldy = 640;

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
		//	initialize entity manager - removes any lingering entities, sets things up for a new session

		EntitySetup(EntityType_MAX_);

		// ---------------------------------------------------------------------------------------------------------------------
		//	create entities needed for this session - Viewport

		// create a viewport entity so we can track its relationship with everything else in the world
		g_pe_viewport = EntitySpawn(EntityType_VIEWPORT, g_viewport_worldx, g_viewport_worldy-c_viewport_margin);

		// ---------------------------------------------------------------------------------------------------------------------
		//	create entities needed for this session - 'X e n o n'

		// world position for the whole word 'Xenon'
		s16 xenonx = 80;
		s16 xenony = 90;

		// spawn individual letter entities at the correct relative positions, in world coordinates
		// note that we create 2 separate 'instances' of the entity type 'n'
		EntitySpawn(EntityType_LETTER_X,	g_viewport_worldx+xenonx+0,				g_viewport_worldy+xenony+0);
		EntitySpawn(EntityType_LETTER_e,	g_viewport_worldx+xenonx+56,			g_viewport_worldy+xenony+7);
		EntitySpawn(EntityType_LETTER_n,	g_viewport_worldx+xenonx+56+24,			g_viewport_worldy+xenony+7);
		EntitySpawn(EntityType_LETTER_o,	g_viewport_worldx+xenonx+56+24+28,		g_viewport_worldy+xenony+7);
		EntitySpawn(EntityType_LETTER_n,	g_viewport_worldx+xenonx+56+24+28+28,	g_viewport_worldy+xenony+7);

		// =====================================================================================================================
		//	must link any pending entities before starting mainloop (viewport is an entity and viewport scans rely on it)

		EntityExecuteAllLinks();

		// =====================================================================================================================
		//	mainloop
		// =====================================================================================================================

		// ---------------------------------------------------------------------------------------------------------------------
		//	some persistent variables we're going to need in the mainloop

		s16 scroll_speed = 0;			// disable scrolling
		s16 backbuffer_num = 0;			// index of current backbuffer (0 or 1)
		s16 curr_vbl = g_vbl;			// initial vblank number
		s16 animation_timer = 0;

		// ---------------------------------------------------------------------------------------------------------------------
		//	start the mainloop

		while (animation_timer++ < 800)
		{

			// ---------------------------------------------------------------------------------------------------------------------
			//	wait for the current VBL period to elapse

			curr_vbl = g_vbl;
			while (g_vbl == curr_vbl) { /* spin until counter changes */ }

			TIMING_MAIN(0x077);	

			// ---------------------------------------------------------------------------------------------------------------------
			//	tick all entities linked into the 'tick' chain
			//	note: not all entities need to be in the tick chain. it depends on EntityFlag_TICKED. however non-ticked entities
			//	should be manually sorted using EntitySort2D if they are still being drawn, in order to maintain 2D coherence. 
			//	'ticked' entities get sorted automatically by the tick system.

			// manually update the viewport first. we don't need a full tick for this but we do need to keep the viewport sorted
			g_pe_viewport->rx = g_viewport_worldx;
			g_pe_viewport->ry = g_viewport_worldy - c_viewport_margin;

			// ---------------------------------------------------------------------------------------------------------------------
			//	tick all currently linked objects
			// ---------------------------------------------------------------------------------------------------------------------
			//	this causes the user defined entity::fntick() function of each entity to be called
			//	note: tick order is not guaranteed unless entity::tick_priority is used to specify an order. this incurs a small cost.

			EntityTickAll();

			TIMING_MAIN(0x700);	

			// ---------------------------------------------------------------------------------------------------------------------
			//	update the background & scroll

			// select the next workbuffer (..of two - we're using double buffering here)
			myworld.select_buffer(backbuffer_num, 0);

			// update the playfield coordinates for this draw pass
			myworld.setpos(g_viewport_worldx, g_viewport_worldy);

			// restore playfield from any previous drawing
			myworld.bgrestore();

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
			//	draw 'all the things'

			EntityDrawVisible(&drawcontext);

			TIMING_MAIN(0x007);	

			// ---------------------------------------------------------------------------------------------------------------------
			//	present the current workbuffer as the new frontbuffer

			myworld.activate_buffer();

			// ---------------------------------------------------------------------------------------------------------------------
			//	toggle buffers so next time we write to the new backbuffer

			backbuffer_num = backbuffer_num ^ 1;

			// ---------------------------------------------------------------------------------------------------------------------
			//	update the scroll coordinates

			// start scrolling after 50 ticks
			if (animation_timer > 50)
				scroll_speed = 1;

			// keep copy of viewport's last position
			g_viewport_changeinx = g_viewport_worldx;
			g_viewport_changeiny = g_viewport_worldy;

			// scroll viewport
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

			// get change in viewport coords since last update
			g_viewport_changeinx -= g_viewport_worldx;
			g_viewport_changeiny -= g_viewport_worldy;

			// ---------------------------------------------------------------------------------------------------------------------
			//	repeat until mainloop expires...

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
// 'tick' behaviour function
// =====================================================================================================================

void letter_fntick(entity_t *_pself)
{
	// compensate entity position for change in viewport position - track the viewport
	_pself->rx -= g_viewport_changeinx;
	_pself->ry -= g_viewport_changeiny;
}

// =====================================================================================================================
// empty entity dictionary (required)
// =====================================================================================================================

entitydef_t entity_dictionary[EntityType_MAX_] =
{
	// [functions]								[coords]						[control]		[tick state]		[rendering]				[classification properties]
	// tick,collide,asset,hasset				px/y,vx/y,sx/y,ox/y				tickpri,drflag	h,d,frame,counter	drawtype,drawlayer		f_self,f_interactswith	

	// engine objects...

	// viewport
	{ 0,0,0,0,									{0},{0},{0},{0},0,0,0,0,		0,0,			0,0,0,0,			EntityDraw_NONE,0,		EntityFlag_VIEWPORT|EntityFlag_SPATIAL,	0 },

	// other objects...

	// letters
	{ &letter_fntick,0,0,0,						{0},{0},{0},{0},0,0,0,0,		0,0,			0,0,0,0,			EntityDraw_IMSPR,0,		EntityFlag_INERT|EntityFlag_TICKED|EntityFlag_SPATIAL,	0 },
	{ &letter_fntick,0,0,0,						{0},{0},{0},{0},0,0,0,0,		0,0,			0,0,0,0,			EntityDraw_EMXSPR,0,	EntityFlag_INERT|EntityFlag_TICKED|EntityFlag_SPATIAL,	0 },
	{ &letter_fntick,0,0,0,						{0},{0},{0},{0},0,0,0,0,		0,0,			0,0,0,0,			EntityDraw_EMXSPR,0,	EntityFlag_INERT|EntityFlag_TICKED|EntityFlag_SPATIAL,	0 },
	{ &letter_fntick,0,0,0,						{0},{0},{0},{0},0,0,0,0,		0,0,			0,0,0,0,			EntityDraw_EMXSPR,0,	EntityFlag_INERT|EntityFlag_TICKED|EntityFlag_SPATIAL,	0 },

};
