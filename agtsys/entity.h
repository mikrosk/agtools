//======================================================================================================================
//	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
// 	entity management systems
//----------------------------------------------------------------------------------------------------------------------

#ifndef entity_h_
#define entity_h_

//----------------------------------------------------------------------------------------------------------------------
// external configuration

#if defined(AGT_CONFIG_WORLD_XMAJOR)
#define ENABLE_SPATIAL_X
#endif

#if defined(AGT_CONFIG_WORLD_YMAJOR)
#define ENABLE_SPATIAL_Y
#endif

//#define USE_DYNAMIC_TICKPRIORITY

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
	struct entity_t *pnext_x;
	struct entity_t *pprev_x;
	struct entity_t *pnext_y;
	struct entity_t *pprev_y;
	struct entity_t *pparent;

	// [identity]
	s16 id;
	s16 type;

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
// internal functions

// link entity into all relevant chains (SPATIAL,TICKED)	
static void EntityLink(entity_t *_pent, entity_t *_near);

// schedule entity to be linked into all relevant chains (SPATIAL,TICKED)	
static inline void EntityScheduleLink(entity_t *_pent, entity_t *_pparent);

// unlink entity into all relevant chains (SPATIAL,TICKED)	
static void EntityUnLink(entity_t *_pent, entity_t *_near);

// maintain 2D coherence in SPATIAL chain
static inline void EntitySort2D(entity_t *pcurr);

// ---------------------------------------------------------------------------------------------------------------------
// public interface

// initialise/reset the entity chain and set up some optional entity tracking/debugging services
void EntitySetup(int _numtypes);

// schedule entity for kill/removal - typically used by AI functions
static inline void EntityRemove(entity_t *_pent);

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

// interact/collide all entities within viewport rectangle
// note: instead call EntityInteractVisible() wrapper from mainloop
extern "C" void EntityInteractVisibleXM_68k();
extern "C" void EntityInteractVisibleYM_68k();

// attach sprite asset to entity
void EntityDefAsset_Sprite(EntityType _etype, spritesheet *_passet);

// attach slab 'pair' (draw+restore shapes) to entity
void EntityDefAsset_Slab(EntityType _etype, sls_pair *_passet);

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
// entity freelist - new spawns are issued from this pool

extern s16 entities_freelist_count;
extern entity_t *entities_freelist[];

// ---------------------------------------------------------------------------------------------------------------------
// entity dictionary of default fields - must be provided externally by each demo/game
extern entitydef_t entity_dictionary[];

// ---------------------------------------------------------------------------------------------------------------------
// maximum entities allowed in world at any time
// todo: this should be configured via Makefile
static const s16 c_max_entities = 64;

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

static inline void EntityRemove(entity_t *_pent)
{
	if ((_pent->f_self & EntityFlags_KILL) == 0)
	{	
		_pent->f_self |= EntityFlags_KILL;
		s_entity_removes[s_entity_remove_count++] = _pent;
	}
}

static inline void EntityScheduleLink(entity_t *_pent, entity_t *_pparent)
{
	s16 c = s_entity_link_count;
	s_entity_links[c++] = _pent;
	s_entity_links[c++] = _pparent;
	s_entity_link_count = c;
}

// ---------------------------------------------------------------------------------------------------------------------

static inline void EntitySort2D(entity_t *pcurr)
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

static inline void EntityUnLink2D(entity_t *_pent)
{
	entity_t *pcurr = _pent;

#if defined(ENABLE_SPATIAL_X)
	entity_t *pnextx = pcurr->pnext_x;
	entity_t *pprevx = pcurr->pprev_x;

	pnextx->pprev_x = pprevx;
	pprevx->pnext_x = pnextx;
#endif

#if defined(ENABLE_SPATIAL_Y)
	entity_t *pnexty = pcurr->pnext_y;
	entity_t *pprevy = pcurr->pprev_y;

	pnexty->pprev_y = pprevy;
	pprevy->pnext_y = pnexty;
#endif
}

// ---------------------------------------------------------------------------------------------------------------------
// EntityDrawVisible(), EntityInteractVisible() are configured according to world major axis

#if defined(AGT_CONFIG_WORLD_YMAJOR)
#define EntityDrawVisible BLiT_EntityDrawVisibleYM_68k
#define EntityInteractVisible EntityInteractVisibleYM_68k
#else
#define EntityDrawVisible BLiT_EntityDrawVisibleXM_68k
#define EntityInteractVisible EntityInteractVisibleXM_68k
#endif

// ---------------------------------------------------------------------------------------------------------------------

#endif // entity_h_
