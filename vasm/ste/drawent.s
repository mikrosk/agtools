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

within_entitydraw	set	1

*-------------------------------------------------------*
	text
*-------------------------------------------------------*

	ifd		AGT_CONFIG_WORLD_XMAJOR
	XDEF		_BLiT_EntityDrawVisibleXM_68k
	XDEF		_EntityInteractVisibleXM_68k
	endc
	
	ifd		AGT_CONFIG_WORLD_YMAJOR
	XDEF		_BLiT_EntityDrawVisibleYM_68k
	XDEF		_EntityInteractVisibleYM_68k
	endc

;	EM-optimized (but not code-generated) blitter sprite routines for <= 32-wide sprites

	XREF		_AGT_BLiT_EMSprDrawClipped
	XREF		_AGT_BLiT_EMSprDrawUnClipped
	XREF		_AGT_BLiT_EMSprInit
	XREF		_AGT_BLiT_IMSprInit
	XREF		_AGT_SlabDirtyRegion
	XREF		_AGT_BLiT_SlabInit

;	safety checks

	ifd		AGT_CONFIG_SAFETY_CHECKS
	XREF		_AGT_Exception_IMSFrame
	XREF		_AGT_Exception_EMSFrame
	XREF		_AGT_Exception_EMXFrame
	XREF		_AGT_Exception_SLSFrame
	XREF		_AGT_Exception_SLRFrame
	endc
	
;	clipping window

	XREF		_g_spr_window
	
	XREF		_g_linebytes
	
	XREF		_g_pe_viewport

	XREF		_drawcontext
	
	XREF		slab_precalcs

	ifd		AGT_CONFIG_DEBUGDRAW
	XREF		_g_debugdraw
	endc
	
*-------------------------------------------------------*

	ifd		AGT_CONFIG_WORLD_XMAJOR	
	include		"ste\\de_xm.s"
	endc

	ifd		AGT_CONFIG_WORLD_YMAJOR	
	include		"ste\\de_ym.s"
	endc
	
*-------------------------------------------------------*
			text
*-------------------------------------------------------*

	ifd		AGT_CONFIG_SAFETY_CHECKS
_AGT_Exception_IMSFrame_68k:
	move.l		a0,-(sp)
	move.l		d0,-(sp)
	jsr		_AGT_Exception_IMSFrame
	; noreturn
_AGT_Exception_EMSFrame_68k:
	move.l		a0,-(sp)
	move.l		d0,-(sp)
	jsr		_AGT_Exception_EMSFrame
	; noreturn
_AGT_Exception_EMXFrame_68k:
	move.l		a0,-(sp)
	move.l		d0,-(sp)
	jsr		_AGT_Exception_EMXFrame
	; noreturn
_AGT_Exception_SLSFrame_68k:
	move.l		a0,-(sp)
	move.l		d0,-(sp)
	jsr		_AGT_Exception_SLSFrame
	; noreturn
_AGT_Exception_SLRFrame_68k:
	move.l		a0,-(sp)
	move.l		d0,-(sp)
	jsr		_AGT_Exception_SLRFrame
	; noreturn
	endc

*-------------------------------------------------------*