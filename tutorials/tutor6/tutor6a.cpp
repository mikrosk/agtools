//======================================================================================================================
//	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================

//----------------------------------------------------------------------------------------------------------------------
//	tutorial: Entity Interaction - Collisions
//----------------------------------------------------------------------------------------------------------------------
//	controls: none
//----------------------------------------------------------------------------------------------------------------------
//	notes:
//	+	
//	+	
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

#include "agtsys/rnd.h"
#include "tables/fdirs_32angles.h"

mersenne_twister random;

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

	// some objects
	EntityType_RED,
	EntityType_GREEN,
	EntityType_BLUE,
	EntityType_YELLOW,
	
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
    /*restore=*/RestoreMode_PAGERESTORE

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

// ---------------------------------------------------------------------------------------------------------------------
//	playfield world coordinates

// clipping margin outside of the scrolling viewport, to accommodate the largest typical sprite before deletion etc.
static const s16 c_viewport_xmargin = playfield_t::c_guardx;
static const s16 c_viewport_ymargin = playfield_t::c_guardy;

// world map safety margin (in pixels) for speculative scrolling algorithm
static const int c_scroll_safety_margin = 16;

// viewport (scroll) starting position in world map
static s16 g_viewport_worldx = 0; 
static s16 g_viewport_worldy = c_scroll_safety_margin;

static s16 g_viewport_changeinx = 0;
static s16 g_viewport_changeiny = 0;

//static const s16 c_xenon_yoffset = -50;

// =====================================================================================================================
//	program entrypoint
// =====================================================================================================================

int AGT_EntryPoint()
{
	// ---------------------------------------------------------------------------------------------------------------------
	//	welcome message

	printf("[A]tari [G]ame [T]ools\n");

	printf("Entity Interaction: Collisions\n");


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
		mytiles.load_cct("tutor6.cct");

		// ---------------------------------------------------------------------------------------------------------------------
		//	load map
		
		printf("loading map...\n");
		mymap.load_ccm("tutor6.ccm");

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
		//	init world with initial viewport coordinates

		g_viewport_worldx = 0;
		g_viewport_worldy = 0;

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

		shift.setcolour(0, 0, 6, 0x007);
		shift.setcolour(1, 0, 6, 0x007);

		// =====================================================================================================================
		//	mainloop setup
		// =====================================================================================================================

		// ---------------------------------------------------------------------------------------------------------------------
		//	initialize entity manager - removes any lingering entities, sets things up for a new session

		EntitySetup(EntityType_MAX_);

		// ---------------------------------------------------------------------------------------------------------------------
		//	create entities needed for this session - Viewport

		// create a viewport entity so we can track its relationship with everything else in the world
		g_pe_viewport = EntitySpawn(EntityType_VIEWPORT, g_viewport_worldx-c_viewport_xmargin, g_viewport_worldy-c_viewport_ymargin);

		// ---------------------------------------------------------------------------------------------------------------------
		//	create entities needed for this session - 'X e n o n'

		// spawn individual letter entities at the correct relative positions, in world coordinates
		// note that we create 2 separate 'instances' of the entity type 'n'

		// this time we're interested in changing each entity as soon as its created so we want
		// to have a temporary pointer to each one returned, for editing
		entity_t *penew;
		s32 x,y;

		for (int n = 0; n < 8; n++)
		{
			x = random.genS32();
			y = random.genS32();
			penew = EntitySpawn(EntityType_RED,		x&255, y&127);
			penew->fvx = x >> 16;
			penew->fvy = y >> 16;
			penew->counter = x & 31;

			x = random.genS32();
			y = random.genS32();
			penew = EntitySpawn(EntityType_GREEN,		x&255, y&127);
			penew->fvx = x >> 16;
			penew->fvy = y >> 16;
			penew->counter = x & 31;

			x = random.genS32();
			y = random.genS32();
			penew = EntitySpawn(EntityType_BLUE,		x&255, y&127);
			penew->fvx = x >> 16;
			penew->fvy = y >> 16;
			penew->counter = x & 31;

			x = random.genS32();
			y = random.genS32();
			penew = EntitySpawn(EntityType_YELLOW,		x&255, y&127);
			penew->fvx = x >> 16;
			penew->fvy = y >> 16;
			penew->counter = x & 31;
		}

		// =====================================================================================================================
		//	must link any pending entities before starting mainloop (viewport is an entity and viewport scans rely on it)

		EntityExecuteAllLinks();

		// =====================================================================================================================
		//	mainloop
		// =====================================================================================================================

		// ---------------------------------------------------------------------------------------------------------------------
		//	some persistent variables we're going to need in the mainloop

		s16 scroll_speed = 1;			// disable scrolling
		s16 backbuffer_num = 0;			// index of current backbuffer (0 or 1)
		s16 curr_vbl = g_vbl;			// initial vblank number
		s16 animation_timer = 0;

		// ---------------------------------------------------------------------------------------------------------------------
		//	start the mainloop

		while (1)//(animation_timer++ < 600)
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
			g_pe_viewport->rx = g_viewport_worldx - c_viewport_xmargin;
			g_pe_viewport->ry = g_viewport_worldy - c_viewport_ymargin;

			// ---------------------------------------------------------------------------------------------------------------------
			//	tick all currently linked objects
			// ---------------------------------------------------------------------------------------------------------------------
			//	this causes the user defined entity::fntick() function of each entity to be called
			//	note: tick order is not guaranteed unless entity::tick_priority is used to specify an order. this incurs a small cost.

			EntityTickAll();

			TIMING_MAIN(0x777);	

			EntityInteractVisible(320, c_viewport_xmargin, 240, c_viewport_ymargin);

			TIMING_MAIN(0x707);	

			EntityExecuteAllLinks();

			EntityExecuteAllRemoves();

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

			TIMING_MAIN(0x770);	

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

			TIMING_MAIN(0x000);	

			// ---------------------------------------------------------------------------------------------------------------------
			//	repeat until mainloop expires...

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
// 'tick' behaviour functios
// =====================================================================================================================



// =====================================================================================================================
// tick function for different object types

// shared behaviour used by all object types in this demo:
// move around the screen, bounce off the borders

agt_inline void ai_common(entity_t &_self)
{
	_self.frx += _self.fvx;
	_self.fry += _self.fvy;

	// change direction if we hit the screen border
	if (_self.rx > (320-16) || _self.rx < 0)
	{
		_self.fvx = -_self.fvx;
		_self.frx += _self.fvx;
	}

	if (_self.ry > (240-16) || _self.ry < 0)
	{
		_self.fvy = -_self.fvy;
		_self.fry += _self.fvy;
	}

	// when drawing in filled rectangle mode, 'frame' sets the drawing colour
	// in our sample, 'health' is used to reset the object's base colour since it may change when colliding
	_self.frame = _self.health;
}

// every 50 ticks, turn right 90'
void red_fntick(entity_t *_pself)
{
	entity_t &self = *_pself;
	ai_common(self);

	if (--self.counter < 0)
	{
		self.counter = 50;

		// turn 90' right
		s32 tmp = self.fvx;
		self.fvx = -self.fvy;
		self.fvy = tmp;
	}
}

// every 50 ticks, turn left 90'
void green_fntick(entity_t *_pself)
{
	entity_t &self = *_pself;
	ai_common(self);

	if (--self.counter < 0)
	{
		self.counter = 50;

		// turn 90' left
		s32 tmp = self.fvx;
		self.fvx = self.fvy;
		self.fvy = -tmp;
	}
}

// every 64 ticks, pick a random direction
void blue_fntick(entity_t *_pself)
{
	entity_t &self = *_pself;
	ai_common(self);

	if ((self.counter++ & 63) == 0)
	{
		// next quasirandom angle
		s16 ang = ((self.rx ^ self.ry) + self.id) & 31;
		// get dir vector from angle
		const s32 *vec = &s_fdirs_32ang[ang<<1];
		self.fvx = vec[0];
		self.fvy = vec[1];
	}
}

// just bounce off the borders, nothing else
void yellow_fntick(entity_t *_pself)
{
	entity_t &self = *_pself;
	ai_common(self);
}

// shared collision response: change fill colour = 1
void generic_fncol(entity_t *_pself, entity_t *_pother)
{
	_pself->frame = 1;
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
	{ &red_fntick,&generic_fncol,0,0,			{0},{0},{0},{0},16,16,8,8,		0,0,			12,0,0,0,			EntityDraw_RFILL,0,		EntityFlag_PLAYER|EntityFlag_TICKED|EntityFlag_SPATIAL,		EntityFlag_NME|EntityFlag_POWERUP },
	{ &green_fntick,&generic_fncol,0,0,			{0},{0},{0},{0},16,16,8,8,		0,0,			3,0,0,0,			EntityDraw_RFILL,0,		EntityFlag_NME|EntityFlag_TICKED|EntityFlag_SPATIAL,		EntityFlag_PLAYER },
	{ &blue_fntick,&generic_fncol,0,0,			{0},{0},{0},{0},16,16,8,8,		0,0,			6,0,0,0,			EntityDraw_RFILL,0,		EntityFlag_POWERUP|EntityFlag_TICKED|EntityFlag_SPATIAL,	0 },
	{ &yellow_fntick,&generic_fncol,0,0,		{0},{0},{0},{0},16,16,8,8,		0,0,			14,0,0,0,			EntityDraw_RFILL,0,		EntityFlag_INERT|EntityFlag_TICKED|EntityFlag_SPATIAL,		0 },

};
