*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
*	Playfield support lib
*-------------------------------------------------------*

	COMMENT		HEAD=7
	OPT		P=68000,NOTYPE
	OPT		W+
	OPT		O+

*-------------------------------------------------------*

	XDEF	_STF_MaskEdgeL
	XDEF	_STF_MaskEdgeR
	XDEF	_STF_RestoreEdges

	XDEF	_SYS_MapModInit
	
	XDEF	_g_xpreshift

*-------------------------------------------------------*
	text
*-------------------------------------------------------*

	include		vasm/stf/pfshiftc68k.s
	include		vasm/stf/pfshiftsr68k.s
	include		vasm/stf/pfshiftsc68k.s

*-------------------------------------------------------*

_SYS_MapModInit:
	rts

;maskbuffer_size		=	(1*240)+1

; todo: does not work in ljbk 8pix mode
*-------------------------------------------------------*
_STF_MaskEdgeL:
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.savea6:		rs.l	1
.save:			rs.l	10
.return:		rs.l	1
*-------------------------------------------------------*
.pfb:			rs.l	1
.xoff:			rs.l	1
.yoff:			rs.l	1
.height:		rs.l	1
*-------------------------------------------------------*
.frame_:		rs.b	0
*-------------------------------------------------------*
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
*-------------------------------------------------------*
	move.l		.pfb(a6),a0
	
	move.w		.yoff+2(a6),d1
	mulu.w		#160,d1
	add.l		d1,a0

	move.w		.xoff+2(a6),d2
	and.w		#-16,d2
	lsr.w		d2
	add.w		d2,a0

	move.w		.xoff+2(a6),d2

;	determine mask: 0,4,8,12 pixels	
	moveq		#$c,d3
	and.w		d2,d3
;	shorcut for 0-pixel case
	beq		.mask_0
;	optimized version of 8-pixel case
	cmp.w		#8,d3
	beq		.mask_8

*-------------------------------------------------------*
.mask_4_12	
*-------------------------------------------------------*
	moveq		#-1,d1
	lsr.w		d3,d1
	swap		d1
	lsr.w		d3,d1
	
	move.w		.height+2(a6),d3
	
	moveq		#32-1,d0
	and.w		d3,d0
	lsr.w		#5,d3
	neg.w		d0
	add.w		#32,d0

	lsl.w		#3,d0
	jmp		.y412(pc,d0.w)
.y412:
	rept		32
	and.l		d1,(a0)+
	and.l		d1,(a0)+
	lea		160-8(a0),a0
	endr
	
	dbra		d3,.y412	
	bra		.done
		
*-------------------------------------------------------*
.mask_8:
*-------------------------------------------------------*
	moveq		#0,d1	

	move.w		.height+2(a6),d3

	moveq		#32-1,d0
	and.w		d3,d0
	lsr.w		#5,d3
	neg.w		d0
	add.w		#32,d0
	
;	compensate for indexing fixed offset sequence
	move.w		d0,d2
	mulu.w		#160,d2
	sub.w		d2,a0
	
	add.w		d0,d0
	add.w		d0,d0
	jmp		.y8(pc,d0.w)
.y8:

ln	set		0	
	rept		32
	movep.l		d1,ln*160(a0)
ln	set		ln+1	
	endr
		
	lea		32*160(a0),a0

	dbra		d3,.y8	
	
*-------------------------------------------------------*
.mask_0:
*-------------------------------------------------------*
.done:	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts


; todo: does not work in ljbk 8pix mode
*-------------------------------------------------------*
_STF_MaskEdgeR:
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.savea6:		rs.l	1
.save:			rs.l	10
.return:		rs.l	1
*-------------------------------------------------------*
.pfb:			rs.l	1
.xoff:			rs.l	1
.yoff:			rs.l	1
.height:		rs.l	1
*-------------------------------------------------------*
.frame_:		rs.b	0
*-------------------------------------------------------*
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
*-------------------------------------------------------*
	move.l		.pfb(a6),a0
	
	move.w		.yoff+2(a6),d1
	mulu.w		#160,d1
	add.l		d1,a0

	move.w		.xoff+2(a6),d2
	add.w		#320-16,d2
	and.w		#-16,d2
	lsr.w		d2
	add.w		d2,a0

;	lea		160-8(a0),a0

	move.w		.xoff+2(a6),d2
	
	moveq		#$c,d3
	and.w		d2,d3
;	offset R
	addq.w		#4,d3
	cmp.w		#16,d3	
	beq		.mask_0
	cmp.w		#8,d3
	beq		.mask_8

*-------------------------------------------------------*
.mask_4_12:	
*-------------------------------------------------------*
	
	moveq		#-1,d1
	lsr.w		d3,d1
	swap		d1
	lsr.w		d3,d1
	not.l		d1
					
	move.w		.height+2(a6),d3
	
	moveq		#32-1,d0
	and.w		d3,d0
	lsr.w		#5,d3
	neg.w		d0
	add.w		#32,d0
	
	lsl.w		#3,d0
	jmp		.ylp(pc,d0.w)
.ylp:
	rept		32
	and.l		d1,(a0)+
	and.l		d1,(a0)+
	lea		160-8(a0),a0
	endr
	
	dbra		d3,.ylp	
	bra		.done
	
*-------------------------------------------------------*
.mask_8:
*-------------------------------------------------------*
	moveq		#0,d1	

	move.w		.height+2(a6),d3

	moveq		#32-1,d0
	and.w		d3,d0
	lsr.w		#5,d3
	neg.w		d0
	add.w		#32,d0
	
;	compensate for indexing fixed offset sequence
	move.w		d0,d2
	mulu.w		#160,d2
	sub.w		d2,a0
	
	add.w		d0,d0
	add.w		d0,d0
	jmp		.ylp8(pc,d0.w)
.ylp8:

ln	set		0	
	rept		32
	movep.l		d1,(ln*160)+1(a0)
ln	set		ln+1	
	endr
	
	lea		32*160(a0),a0

	dbra		d3,.ylp8	

*-------------------------------------------------------*
.mask_0:	
*-------------------------------------------------------*
.done:	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts
	
*-------------------------------------------------------*
	bss
*-------------------------------------------------------*

;maskbuffer:		ds.b	(8*maskbuffer_size)*4

*-------------------------------------------------------*
	text
*-------------------------------------------------------*

*-------------------------------------------------------*
	data
*-------------------------------------------------------*
	
_g_xpreshift:		dc.w	0
			dc.w	0
		
*-------------------------------------------------------*
	text
*-------------------------------------------------------*
	