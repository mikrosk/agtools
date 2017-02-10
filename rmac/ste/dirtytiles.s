
dst_linewid	set	184

*------------------------------------------------------------------------------*
*	tile macros for writing different vpage layouts:
*
*	P|Q
*	---
*	R|S
*------------------------------------------------------------------------------*

	include		"ste/dirtytiles_pqrs.s"
	include		"ste/dirtytiles_pq.s"
	include		"ste/dirtytiles_pr.s"
	include		"ste/dirtytiles_p.s"
	
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

_dm_dirtymask:		ds.l	1
	.globl	_dm_dirtymask
_dm_dstaddr:		ds.l	1
	.globl	_dm_dstaddr
_dm_ptiles:		ds.l	1
	.globl	_dm_ptiles
_dm_pmap:		ds.l	1
	.globl	_dm_pmap
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
