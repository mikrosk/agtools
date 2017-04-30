
dst_linewid	set	184

*	turn off 0(An) outer displacements for sized code indexing

	.opt		"~o3"

	.globl		_SYS_MapModInit

*------------------------------------------------------------------------------*
*	tile macros for writing different vpage layouts:
*
*	P|Q
*	---
*	R|S
*------------------------------------------------------------------------------*

	.if		^^defined AGT_CONFIG_PFLAYOUT_PQRS
	include		"ste/mapmod_pqrs16.s"
	.endif
	.if		^^defined AGT_CONFIG_PFLAYOUT_PQ
	include		"ste/mapmod_pq16.s"
	.endif
	include		"ste/mapmod_pr16.s"
	include		"ste/mapmod_pr16m.s"
	include		"ste/mapmod_pr8.s"
	include		"ste/mapmod_pr8m.s"
	.if		^^defined AGT_CONFIG_PFLAYOUT_P
	include		"ste/mapmod_p16.s"
	.endif
	
*------------------------------------------------------------------------------*
_SYS_MapModInit:
*------------------------------------------------------------------------------*
	.if		^^defined AGT_CONFIG_PFLAYOUT_PQRS
	jsr		mapmod_setup_pqrs
	.endif
	.if		^^defined AGT_CONFIG_PFLAYOUT_PQ
	jsr		mapmod_setup_pq
	.endif
	jsr		mapmod_setup_pr16
	jsr		mapmod_setup_pr16m
	jsr		mapmod_setup_pr8
	jsr		mapmod_setup_pr8m
	.if		^^defined AGT_CONFIG_PFLAYOUT_P
	jsr		mapmod_setup_p
	.endif
	rts
	
*------------------------------------------------------------------------------*
mapmod_setup:	
*------------------------------------------------------------------------------*
	move.l		d2,-(sp)
	move.l		a0,a1
	moveq		#16-1,d2
.ol:	moveq		#16-1,d1
	move.l		4(a0),d0
	addq.l		#8,a0
.il:	move.l		d0,(a1)
	addq.l		#8,a1
	dbra		d1,.il
	dbra		d2,.ol
	move.l		(sp)+,d2
	rts

*------------------------------------------------------------------------------*
	.data
*------------------------------------------------------------------------------*

	if	(0)
diagnostic_tile:
	rept	8
	dc.l	$AAAAAAAA,-1
	dc.l	$55555555,-1
	endr
	endif

*------------------------------------------------------------------------------*
	.bss
*------------------------------------------------------------------------------*

_dm_dstaddr:		ds.l	1
	.globl	_dm_dstaddr
_dm_dirtymask:		ds.l	1
	.globl	_dm_dirtymask
_dm_ptiles:		ds.l	1
	.globl	_dm_ptiles
_dm_pltiles:		ds.l	1
	.globl	_dm_pltiles
_dm_pmap:		ds.l	1
	.globl	_dm_pmap
_dm_plmap:		ds.l	1
	.globl	_dm_plmap
_dm_virtual_pagebytes:	ds.l	1
	.globl	_dm_virtual_pagebytes
_dm_visible_linebytes:	ds.w	1
	.globl	_dm_visible_linebytes
_dm_map_tiles_x:	ds.w	1
	.globl	_dm_map_tiles_x
_dm_pi:			ds.w	1
	.globl	_dm_pi
_dm_pi2:		ds.w	1
	.globl	_dm_pi2
_dm_pj:			ds.w	1
	.globl	_dm_pj
_dm_pj2:		ds.w	1
	.globl	_dm_pj2
_dm_maskstride:		ds.w	1
	.globl	_dm_maskstride
_dm_lineskip:		ds.w	1
	.globl	_dm_lineskip
	
*------------------------------------------------------------------------------*
	.text
*------------------------------------------------------------------------------*
