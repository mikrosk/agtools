*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
*	Entity drawing system:
*	- Walk viewport entity chain
*	- Interpret entity flags, states, functions
*	- Execute appropriate drawing routines
*-------------------------------------------------------*

	include		"config.inc"

	include		"blitter.inc"
	include		"emsprt.inc"

	include		"entity.inc"
	include		"asset.inc"

	include		"drawctx.inc"

*-------------------------------------------------------*
	.text
*-------------------------------------------------------*

	.if		^^defined AGT_CONFIG_WORLD_XMAJOR
	.globl		_BLiT_EntityDrawVisibleXM_68k
	.globl		_EntityInteractVisibleXM_68k
	.endif
	
	.if		^^defined AGT_CONFIG_WORLD_YMAJOR
	.globl		_BLiT_EntityDrawVisibleYM_68k
	.globl		_EntityInteractVisibleYM_68k
	.endif

;	EM-optimized (but not code-generated) blitter sprite routines for <= 32-wide sprites

	.globl		_AGT_BLiT_EMSprDrawClipped
	.globl		_AGT_BLiT_EMSprDrawUnClipped
	.globl		_AGT_BLiT_EMSprInit
	.globl		_AGT_BLiT_IMSprInit
	.globl		_AGT_SlabDirtyRegion
	.globl		_AGT_BLiT_SlabInit

;	clipping window

	.globl		_g_spr_window

	.globl		_g_linebytes

	.globl		_g_pe_viewport

	.globl		_drawcontext

	.globl		slab_precalcs
	
	.if		^^defined AGT_CONFIG_DEBUGDRAW
	.globl		_g_debugdraw
	.endif

*-------------------------------------------------------*

	.if		^^defined AGT_CONFIG_WORLD_XMAJOR	
	include		"ste\\de_xm.s"
	.endif

	.if		^^defined AGT_CONFIG_WORLD_YMAJOR	
	include		"ste\\de_ym.s"
	.endif
	
*-------------------------------------------------------*
			.text
*-------------------------------------------------------*
