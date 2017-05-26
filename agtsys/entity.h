//======================================================================================================================
//	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
// 	entity management systems
//----------------------------------------------------------------------------------------------------------------------

#ifndef entity_h_
#define entity_h_

//----------------------------------------------------------------------------------------------------------------------

// required for EntityDrawVisible?? functions
#include "drawcontext.h"

//----------------------------------------------------------------------------------------------------------------------
// external configuration

#if defined(ENTITY_TRACKING) // older, deprecated config switch
#define AGT_CONFIG_ENTITY_TRACKING
#endif

#if defined(AGT_CONFIG_WORLD_XMAJOR)
#define ENABLE_SPATIAL_X
#endif

#if defined(AGT_CONFIG_WORLD_YMAJOR)
#define ENABLE_SPATIAL_Y
#endif

//#define AGT_CONFIG_DYNAMIC_TICKPRIO

// ---------------------------------------------------------------------------------------------------------------------

enum EntityType : int;

// ---------------------------------------------------------------------------------------------------------------------
// different drawing mechanisms which can be used by entities

enum EntityDraw
{
	EntityDraw_NONE,		// invisible/no-draw
	EntityDraw_IMSPR,		// standard [I]nterleaved, [M]asked sprite (software or blitter)
	EntityDraw_SPRITE = EntityDraw_IMSPR, // another name for IMSPR
	EntityDraw_EMSPR,		// blitter [E]nd[M]ask sprite up to 32pixels wide
	EntityDraw_EMXSPR,		// blitter [E]nd[M]ask (extreme optimization) sprite up to 32pixels wide
	EntityDraw_SLAB,		// blitter slab shape for large irregular fill areas
	EntityDraw_CUSTOM,		// user-defined draw function
	EntityDraw_RFILL		// rectangle fill
};

// ---------------------------------------------------------------------------------------------------------------------
// draw priority layers for ordered drawing

enum EntityLayer
{
	EntityLayer_0 = 0,
		EntityLayer_BG = EntityLayer_0,
	EntityLayer_1 = 4,
		EntityLayer_MAIN = EntityLayer_1,
	EntityLayer_2 = 8,
		EntityLayer_FG = EntityLayer_2,
	EntityLayer_3 = 12,
		EntityLayer_HUD = EntityLayer_3
};

// ---------------------------------------------------------------------------------------------------------------------
// entity configuration flags

// flags covering generic properties
enum EntityFlags
{
	EntityFlag_NONE			= 0x0000,		// no classification
	EntityFlag_INERT		= 0x0000,		// inert, no classification

	// classification: breaks entities down roughly into interaction groups

	EntityFlag_VIEWPORT		= 0x0001,		// identifies viewport rectangle, to assist waking up spawnpoints etc.
	//
	EntityFlag_PLAYER		= 0x0002,		// identifies player-aligned entities
	EntityFlag_BULLET		= 0x0004,		// identifies player weapon entities (usually can't harm player or enemy bullets)
	EntityFlag_POWERUP		= 0x0008,		// identifies powerup goodies for collection
	EntityFlag_NME			= 0x0010,		// identifies non-player AIs (typically interact with player-aligned entities)
	EntityFlag_NMEBULLET	= 0x0020,		// identifies enemy bullets separately from player bullets
	EntityFlag_NMEAVOID		= 0x0040,		// identifies enemies as testable against each other for bounce/avoidance


	// CAUTION: do not change the following flags - 68k entity code uses them

	// properties: 
	EntityFlag_SPATIAL		= 0x0100,		// expects to be involved in proximity tests with other objects
	EntityFlag_TICKED		= 0x0200,		// will be linked into the tickchain from the moment of spawning
	EntityFlag_VISTRIG		= 0x0400,		// won't be linked into the tickchain until encountered first in the vischain
											// note: not yet implemented in 68k tick manager - coming soon!

	EntityFlags_KILL		= 0x8000		// entity has been killed but not yet removed from tickchain
};

// ---------------------------------------------------------------------------------------------------------------------

// entity forward-decl
struct entity_t;

// entity generic function pointer
typedef void (*fnptr_t)(entity_t *_pe);
typedef void (*fn2ptr_t)(entity_t *_pe1, entity_t *_pe2);

// static members: defined in entity dictionary AND burst-copied to every spawned entity
#define STATIC_MEMBERS			\
								\
	fnptr_t fntick;				\
	fn2ptr_t fncollide;			\
	void *passet;				\
								\
	union { s16 rx; s32 frx; };	\
	union { s16 ry; s32 fry; };	\
	union { s16 vx; s32 fvx; };	\
	union { s16 vy; s32 fvy; };	\
								\
	s16 sx, sy;					\
	s16 ox, oy;					\
								\
	s16 tick_priority;			\
	s16 draw_hitflash;			\
								\
	s16 health;					\
	s16 damage;					\
	s16 frame;					\
	s16 counter;				\
								\
	s16 drawtype;				\
	s16 drawlayer;				\
	s16 f_self;					\
	s16 f_interactswith;		\



// entitydef: represents an entity dictionary entry
struct entitydef_t
{
	STATIC_MEMBERS
};

// entity: represents the minimal state for every entity instance
struct entity_t
{
	// [dynamic]

	// [linkage]
	struct entity_t *pnext_tick;
	struct entity_t *pprev_tick;
#if defined(ENABLE_SPATIAL_X)
	struct entity_t *pnext_x;
	struct entity_t *pprev_x;
#endif
#if defined(ENABLE_SPATIAL_Y)
	struct entity_t *pnext_y;
	struct entity_t *pprev_y;
#endif
	struct entity_t *pparent;
	struct entity_t *ptrack;
	struct entity_t *pdlayer_;

	// [identity]
	s16 id;
	s16 type;
	s16 rev_self;
	s16 rev_parent;
	s16 rev_track;
	s16 rev_user;

	// [coordinates]
	union { s16 wx; s32 fwx; };	\
	union { s16 wy; s32 fwy; };	\
	union { s16 wz; s32 fwz; };	\
	union { s16 vz; s32 fvz; };	\

	// [static]

	// fields copied from dictionary
	STATIC_MEMBERS

};

// ---------------------------------------------------------------------------------------------------------------------
// public interface

// ---------------------------------------------------------------------------------------------------------------------
// entity processes

// initialise/reset the entity chain and set up some optional entity tracking/debugging services
void EntitySetup(int _numtypes);

// execute any scheduled kills before next tick - called by mainloop
void EntityExecuteAllRemoves();

// execute any scheduled spawns before next tick - called by mainloop
void EntityExecuteAllLinks();

// execute entity tick/AI functions
void EntityTickAll();

// draw all entities within viewport rectangle
// note: instead call EntityDrawVisible(...) wrapper from mainloop
extern "C" void BLiT_EntityDrawVisibleXM_68k(drawcontext_t *_pctx);
extern "C" void BLiT_EntityDrawVisibleYM_68k(drawcontext_t *_pctx);
extern "C" void BLiT_EntityDeferVisibleXM_68k(drawcontext_t *_pctx);
extern "C" void BLiT_EntityDeferVisibleYM_68k(drawcontext_t *_pctx);

extern "C" void STF_EntityDrawVisibleXM_68k(drawcontext_t *_pctx);
extern "C" void STF_EntityDrawVisibleYM_68k(drawcontext_t *_pctx);
extern "C" void STF_EntityDeferVisibleXM_68k(drawcontext_t *_pctx);
extern "C" void STF_EntityDeferVisibleYM_68k(drawcontext_t *_pctx);

// interact/collide all entities within viewport rectangle
// note: instead call EntityInteractVisible() wrapper from mainloop
extern "C" void EntityInteractVisibleXM_68k();
extern "C" void EntityInteractVisibleYM_68k();

// ---------------------------------------------------------------------------------------------------------------------
// spawning

// spawn new entity at arbitrary world position
entity_t * EntitySpawn(EntityType _etype, s16 px, s16 py);

// spawn new entity from parent entity (creates child->parent link)
entity_t * EntitySpawnFrom(EntityType _etype, entity_t *_pparent);

// spawn new entity from parent entity (creates child->parent link) with x,y relative offset from parent
entity_t * EntitySpawnRel(EntityType _etype, entity_t *_pparent, s16 _ox, s16 _oy);

// spawn new entity from parent entity (creates child->parent link) with x,y absolute position - but expected to be close to parent
// note: knowledge of locality helps accelerate inserts
entity_t * EntitySpawnNear(EntityType _etype, entity_t *_pparent, s16 _ox, s16 _oy);

// as EntitySpawnRel but instead of scheduling entity to be linked for next frame, entity is linked immediately and can be ticked
// on the same frame if tickpriority dictates processing later than its creator
// note: advanced usage - chance of obscure bugs, so be careful
entity_t * EntitySpawnImmediate(EntityType _etype, entity_t *_pparent, s16 _ox, s16 _oy);

// entity is created but not linked into relevant chains. the link step must be performed manually at some future date.
// note: advanced usage - chance of obscure bugs, so be careful
entity_t * EntityCreateUnlinked(EntityType _etype);

// ---------------------------------------------------------------------------------------------------------------------
// entity linkage

// link/unlink entity into all relevant chains (SPATIAL,TICKED)	
void EntityLink(entity_t *_pent, entity_t *_near);
static inline void EntityUnLink(entity_t *_pent);

// link/unlink entity only to TICKED chain
void EntityLink_Tick(entity_t *_pent, entity_t *_pnear);
static agt_inline void EntityUnLink_Tick(entity_t *_pent);

// link/unlink entity only to SPATIAL chain
void EntityLink2D(entity_t *_pent, entity_t *_pnear);
static agt_inline void EntityUnLink2D(entity_t *_pent);

// schedule entity for linking to world on next EntityExecuteAllLinks()
static agt_inline void EntityScheduleLink(entity_t *_pent, entity_t *_pparent);
// schedule entity for removal from world on next EntityExecuteAllRemoves()
static agt_inline void EntityRemove(entity_t *_pent);

// todo: deprecate, no longer necessary!
// sort individual object after tick
static agt_inline void EntitySort2D(entity_t *pcurr);

// ---------------------------------------------------------------------------------------------------------------------
// space transformations

static agt_inline void EntityMakeRelative(entity_t *_pent, s16 _basisx, s16 _basisy);
static agt_inline void EntityMakeRelative(entity_t *_pent, s32 _basisfx, s32 _basisfy);
static agt_inline void EntityWorldFromRelative(entity_t *_pent, s16 _basisx, s16 _basisy);
static agt_inline void EntityWorldFromRelative(entity_t &ent, s16 _basisx, s16 _basisy);

// ---------------------------------------------------------------------------------------------------------------------
// assets

// attach sprite asset to entity
void EntityDefAsset_Sprite(EntityType _etype, spritesheet *_passet);

// attach slab 'pair' (draw+restore shapes) to entity
void EntityDefAsset_Slab(EntityType _etype, sls_pair *_passet);


// ---------------------------------------------------------------------------------------------------------------------
// entity freelist - new spawns are issued from this pool

extern s16 entities_freelist_count;
extern entity_t *entities_freelist[];

// ---------------------------------------------------------------------------------------------------------------------
// entity dictionary of default fields - must be provided externally by each demo/game
extern entitydef_t entity_dictionary[];

// ---------------------------------------------------------------------------------------------------------------------
// maximum entities allowed in world at any time
// todo: this should be configured via Makefile
static const s16 c_max_entities = 128;

static const s16 c_entity_max_removes = c_max_entities;
static const s16 c_entity_max_links = c_max_entities<<1;

// ---------------------------------------------------------------------------------------------------------------------
// insert/remove schedules

extern s16 s_entity_remove_count;
extern entity_t *s_entity_removes[c_entity_max_removes];

extern s16 s_entity_link_count;
extern entity_t *s_entity_links[c_entity_max_links];

// ---------------------------------------------------------------------------------------------------------------------
// global viewport object
// todo: this won't remain a single global - will become part of drawcontext
extern entity_t *g_pe_viewport;

// ---------------------------------------------------------------------------------------------------------------------
// internal functions

// schedule entity for kill/removal - typically used by AI functions
static agt_inline void EntityRemove(entity_t *_pent)
{
	if (likely((_pent->f_self & EntityFlags_KILL) == 0))
	{	
		_pent->f_self |= EntityFlags_KILL;
		s_entity_removes[s_entity_remove_count++] = _pent;
	}
#if defined(AGT_CONFIG_SAFETY_CHECKS)
	else
	{
		wprintf("warning: EntityRemove repeat calls on same object\n");
		wprintf("ent:%d type:%d self:%04x\n", (unsigned int)_pent->id, (int)_pent->type, (int)_pent->f_self);
	}
#endif
}

// schedule entity to be linked into all relevant chains (SPATIAL,TICKED)	
static agt_inline void EntityScheduleLink(entity_t *_pent, entity_t *_pparent)
{
	s16 c = s_entity_link_count;
	s_entity_links[c++] = _pent;
	s_entity_links[c++] = _pparent;
	s_entity_link_count = c;
}

// ---------------------------------------------------------------------------------------------------------------------
// maintain 2D coherence in SPATIAL chain

static agt_inline void EntitySort2D(entity_t *pcurr)
{
	entity_t &curr = *pcurr;

#if defined(ENABLE_SPATIAL_X)
	if (curr.rx < curr.pprev_x->rx)
	{
		entity_t *pnext = curr.pnext_x;
		entity_t *pprev = curr.pprev_x;

		// unlink
		pnext->pprev_x = pprev;
		pprev->pnext_x = pnext;

		// find the re-insert position
		entity_t *psort = curr.pprev_x->pprev_x;
		s16 pos = curr.rx;
		while (psort->rx > pos)
			psort = psort->pprev_x;

		// re-insert
		entity_t *psortnext = psort->pnext_x;
		curr.pnext_x = psortnext;
		curr.pprev_x = psort;
		psortnext->pprev_x = pcurr;
		psort->pnext_x = pcurr;
	}
#endif

#if defined(ENABLE_SPATIAL_Y)
	if (curr.ry < curr.pprev_y->ry)
	{
		entity_t *pnext = curr.pnext_y;
		entity_t *pprev = curr.pprev_y;

		// unlink
		pnext->pprev_y = pprev;
		pprev->pnext_y = pnext;

		// find the re-insert position
		entity_t *psort = curr.pprev_y->pprev_y;
		s16 pos = curr.ry;
		while (psort->ry > pos)
			psort = psort->pprev_y;

		// re-insert
		entity_t *psortnext = psort->pnext_y;
		curr.pnext_y = psortnext;
		curr.pprev_y = psort;
		psortnext->pprev_y = pcurr;
		psort->pnext_y = pcurr;
	}
#endif
}

static agt_inline void EntityUnLink_Tick(entity_t *_pent)
{
	entity_t *pcurr = _pent;

#if defined(AGT_CONFIG_SAFETY_CHECKS)
	if (!pcurr->pprev_tick || !pcurr->pnext_tick)
	{
		eprintf("error: EntityUnLink_Tick called on unlinked object\n");
		eprintf("ent:%d type:%d\n", (unsigned int)pcurr->id, (int)pcurr->type);
	}
#endif

	entity_t *pnext = pcurr->pnext_tick;
	entity_t *pprev = pcurr->pprev_tick;

	pnext->pprev_tick = pprev;
	pprev->pnext_tick = pnext;

#if defined(AGT_CONFIG_SAFETY_CHECKS)
	pcurr->pprev_tick = 0;
	pcurr->pnext_tick = 0;
#endif

}

static agt_inline void EntityUnLink2D(entity_t *_pent)
{
	entity_t *pcurr = _pent;

#if defined(ENABLE_SPATIAL_X)

#if defined(AGT_CONFIG_SAFETY_CHECKS)
	if (!pcurr->pprev_x || !pcurr->pnext_x)
	{
		eprintf("error: EntityUnLink2D called on unlinked object\n");
		eprintf("ent:%d type:%d\n", (unsigned int)pcurr->id, (int)pcurr->type);
	}
#endif

	entity_t *pnextx = pcurr->pnext_x;
	entity_t *pprevx = pcurr->pprev_x;

	pnextx->pprev_x = pprevx;
	pprevx->pnext_x = pnextx;

#if defined(AGT_CONFIG_SAFETY_CHECKS)
	pcurr->pprev_x = 0;
	pcurr->pnext_x = 0;
#endif

#endif

#if defined(ENABLE_SPATIAL_Y)

#if defined(AGT_CONFIG_SAFETY_CHECKS)
	if (!pcurr->pprev_y || !pcurr->pnext_y)
	{
		eprintf("error: EntityUnLink2D called on unlinked object\n");
		eprintf("ent:%d type:%d\n", (unsigned int)pcurr->id, (int)pcurr->type);
	}
#endif

	entity_t *pnexty = pcurr->pnext_y;
	entity_t *pprevy = pcurr->pprev_y;

	pnexty->pprev_y = pprevy;
	pprevy->pnext_y = pnexty;
#endif

#if defined(AGT_CONFIG_SAFETY_CHECKS)
	pcurr->pprev_y = 0;
	pcurr->pnext_y = 0;
#endif
}

static inline void EntityUnLink(entity_t *_pent)
{
	if (likely(_pent->f_self & EntityFlag_TICKED))
		EntityUnLink_Tick(_pent);

	if (likely(_pent->f_self & EntityFlag_SPATIAL))
		EntityUnLink2D(_pent);
}

static agt_inline void EntityMakeRelative(entity_t *_pent, s16 _basisx, s16 _basisy)
{
	_pent->wx = _pent->rx - _basisx;
	_pent->wy = _pent->ry - _basisy;
}

static agt_inline void EntityMakeRelative(entity_t *_pent, s32 _basisfx, s32 _basisfy)
{
	_pent->fwx = _pent->frx - _basisfx;
	_pent->fwy = _pent->fry - _basisfy;
}

static agt_inline void EntityWorldFromRelative(entity_t *_pent, s16 _basisx, s16 _basisy)
{
	_pent->rx = _pent->wx + _basisx;
	_pent->ry = _pent->wy + _basisy;
}

static agt_inline void EntityWorldFromRelative(entity_t &ent, s16 _basisx, s16 _basisy)
{
	ent.rx = ent.wx + _basisx;
	ent.ry = ent.wy + _basisy;
}

static inline void EntitySwitchAsset_Sprite(entity_t *_pent, spritesheet *_passet)
{
	spritesheet::sps_header *pspr = _passet->get();

	_pent->passet = pspr;
	_pent->sx = pspr->srcwidth;
	_pent->sy = pspr->srcheight;
	// todo: origin should also be stored/retrieved via spritesheet
	_pent->ox = pspr->srcwidth>>1;
	_pent->oy = pspr->srcheight>>1;
}

// ---------------------------------------------------------------------------------------------------------------------
// select default entity draw functions based on AGT_CONFIG_STF/AGT_CONFIG_STE (fallback is AGT_CONFIG_STE)

#if defined(AGT_CONFIG_STF)
#define EntityDrawVisibleXM STF_EntityDrawVisibleXM_68k
#define EntityDrawVisibleYM STF_EntityDrawVisibleYM_68k
#define EntityDeferVisibleXM STF_EntityDeferVisibleXM_68k
#define EntityDeferVisibleYM STF_EntityDeferVisibleYM_68k
#else
#define EntityDrawVisibleXM BLiT_EntityDrawVisibleXM_68k
#define EntityDrawVisibleYM BLiT_EntityDrawVisibleYM_68k
#define EntityDeferVisibleXM BLiT_EntityDeferVisibleYM_68k
#define EntityDeferVisibleYM BLiT_EntityDeferVisibleYM_68k
#endif

// ---------------------------------------------------------------------------------------------------------------------
// select multi-layered vs single-layer drawing based on AGT_CONFIG_ENTITYLAYERS
// note: single-layer drawing is (very) slightly more efficient than multi-layered with 1 layer for quantities of objects

#if defined(AGT_CONFIG_ENTITYLAYERS) && (AGT_CONFIG_ENTITYLAYERS > 1)
// makefile-controlled drawing method (configured by AGT_CONFIG_STF/AGT_CONFIG_ST)
#undef EntityDrawVisibleXM
#define EntityDrawVisibleXM EntityDeferVisibleXM
#undef EntityDrawVisibleYM
#define EntityDrawVisibleYM EntityDeferVisibleYM
// explicit multi-layer drawing for STF/STE
#define STF_EntityDrawVisibleXM STF_EntityDeferVisibleXM_68k
#define STF_EntityDrawVisibleYM STF_EntityDeferVisibleYM_68k
#define BLiT_EntityDrawVisibleXM BLiT_EntityDeferVisibleXM_68k
#define BLiT_EntityDrawVisibleYM BLiT_EntityDeferVisibleYM_68k
#else
// explicit single-layer drawing for STF/STE
#define STF_EntityDrawVisibleXM STF_EntityDrawVisibleXM_68k
#define STF_EntityDrawVisibleYM STF_EntityDrawVisibleYM_68k
#define BLiT_EntityDrawVisibleXM BLiT_EntityDrawVisibleXM_68k
#define BLiT_EntityDrawVisibleYM BLiT_EntityDrawVisibleYM_68k
#endif

// ---------------------------------------------------------------------------------------------------------------------
// EntityDrawVisible(), EntityInteractVisible() are configured according to world major axis

#if defined(AGT_CONFIG_WORLD_YMAJOR)
#define EntityDrawVisible EntityDrawVisibleYM
#define STF_EntityDrawVisible STF_EntityDrawVisibleYM
#define BLiT_EntityDrawVisible BLiT_EntityDrawVisibleYM
#define EntityInteractVisible EntityInteractVisibleYM_68k
#else
#define EntityDrawVisible EntityDrawVisibleXM
#define STF_EntityDrawVisible STF_EntityDrawVisibleXM
#define BLiT_EntityDrawVisible BLiT_EntityDrawVisibleXM
#define EntityInteractVisible EntityInteractVisibleXM_68k
#endif

// ---------------------------------------------------------------------------------------------------------------------

extern "C" s16 g_debugdraw;

// ---------------------------------------------------------------------------------------------------------------------

#endif // entity_h_
