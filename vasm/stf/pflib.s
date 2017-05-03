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

maskbuffer_size		=	(1*240)+1

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

;	lea		160-8(a0),a1

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

;	move.l		d1,d7
;	not.l		d7
;;	moveq		#-1,d7

	and.w		#4-1,d2
			
	lea		maskbuffer,a2
	mulu.w		#(8*maskbuffer_size),d2
	add.l		d2,a2
	
	move.w		.height+2(a6),d3

	move.l		a0,(a2)+
	move.w		d3,(a2)+
	
	moveq		#16-1,d0
	and.w		d3,d0
	lsr.w		#4,d3

	neg.w		d0
	add.w		#16,d0

	move.w		d0,d2
	lsl.w		#3,d2
	
;	move.w		d0,d2				; *20
;	lsl.w		#2,d2
;	add.w		d0,d2
;;	lsl.w		#3,d2
;	lsl.w		#2,d2
	jmp		.ylp(pc,d2.w)
.ylp:
	rept		16
	and.l		d1,(a0)+
	and.l		d1,(a0)+
;	move.l		(a0),d0
;	move.l		d0,(a2)+
;	and.l		d1,d0
;	move.l		d0,(a0)+
;	move.l		(a1),d0
;	move.l		d0,(a2)+
;	and.l		d7,d0
;	move.l		d0,(a1)+
;	move.l		(a0),d0
;	move.l		d0,(a2)+
;	and.l		d1,d0
;	move.l		d0,(a0)+
;	move.l		(a1),d0
;	move.l		d0,(a2)+
;	and.l		d7,d0
;	move.l		d0,(a1)+
	lea		160-8(a0),a0
;	lea		160-8(a1),a1
	endr
	
	dbra		d3,.ylp	
	bra		.done
		
*-------------------------------------------------------*
.mask_8:
*-------------------------------------------------------*
	move.w		.height+2(a6),d3

	moveq		#0,d1	

	moveq		#16-1,d0
	and.w		d3,d0
	lsr.w		#4,d3

	neg.w		d0
	add.w		#16,d0

	move.w		d0,d2
	
;	compensate for indexing fixed offset sequence
	mulu.w		#160,d0
	sub.w		d0,a0
	
	lsl.w		#2,d2
	jmp		.ylp8(pc,d2.w)
.ylp8:
	movep.l		d1,0*160(a0)
	movep.l		d1,1*160(a0)
	movep.l		d1,2*160(a0)
	movep.l		d1,3*160(a0)
	movep.l		d1,4*160(a0)
	movep.l		d1,5*160(a0)
	movep.l		d1,6*160(a0)
	movep.l		d1,7*160(a0)
	movep.l		d1,8*160(a0)
	movep.l		d1,9*160(a0)
	movep.l		d1,10*160(a0)
	movep.l		d1,11*160(a0)
	movep.l		d1,12*160(a0)
	movep.l		d1,13*160(a0)
	movep.l		d1,14*160(a0)
	movep.l		d1,15*160(a0)
	
	lea		16*160(a0),a0

	dbra		d3,.ylp8	
	
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
	and.w		#-16,d2
	lsr.w		d2
	add.w		d2,a0

	lea		160-8(a0),a0

	move.w		.xoff+2(a6),d2
	
	moveq		#$c,d3
	and.w		d2,d3
	addq.w		#4,d3
	cmp.w		#16,d3	
	beq		.mask_0
	cmp.w		#8,d3
	beq		.mask_8

.mask_4_12:	
	
	moveq		#-1,d1
	lsr.w		d3,d1
	swap		d1
	lsr.w		d3,d1

	not.l		d1
	
;	move.l		d1,d7
;	not.l		d7
;	moveq		#-1,d7

	and.w		#4-1,d2
			
	lea		maskbuffer,a2
	mulu.w		#(8*maskbuffer_size),d2
	add.l		d2,a2
	
	move.w		.height+2(a6),d3

	move.l		a0,(a2)+
	move.w		d3,(a2)+
	
	moveq		#16-1,d0
	and.w		d3,d0
	lsr.w		#4,d3

	neg.w		d0
	add.w		#16,d0

	move.w		d0,d2
	lsl.w		#3,d2
	
;	move.w		d0,d2				; *20
;	lsl.w		#2,d2
;	add.w		d0,d2
;;	lsl.w		#3,d2
;	lsl.w		#2,d2
	jmp		.ylp(pc,d2.w)
.ylp:
	rept		16
	and.l		d1,(a0)+
	and.l		d1,(a0)+
;	move.l		(a0),d0
;	move.l		d0,(a2)+
;	and.l		d1,d0
;	move.l		d0,(a0)+
;	move.l		(a1),d0
;	move.l		d0,(a2)+
;	and.l		d7,d0
;	move.l		d0,(a1)+
;	move.l		(a0),d0
;	move.l		d0,(a2)+
;	and.l		d1,d0
;	move.l		d0,(a0)+
;	move.l		(a1),d0
;	move.l		d0,(a2)+
;	and.l		d7,d0
;	move.l		d0,(a1)+
	lea		160-8(a0),a0
;	lea		160-8(a1),a1
	endr
	
	dbra		d3,.ylp	
	bra		.done
	
*-------------------------------------------------------*
.mask_8:
*-------------------------------------------------------*
	move.w		.height+2(a6),d3

	moveq		#0,d1	

	moveq		#16-1,d0
	and.w		d3,d0
	lsr.w		#4,d3

	neg.w		d0
	add.w		#16,d0

	move.w		d0,d2
	
;	compensate for indexing fixed offset sequence
	mulu.w		#160,d0
	sub.w		d0,a0

	addq.l		#1,a0
	
	lsl.w		#2,d2
	jmp		.ylp8(pc,d2.w)
.ylp8:
	movep.l		d1,0*160(a0)
	movep.l		d1,1*160(a0)
	movep.l		d1,2*160(a0)
	movep.l		d1,3*160(a0)
	movep.l		d1,4*160(a0)
	movep.l		d1,5*160(a0)
	movep.l		d1,6*160(a0)
	movep.l		d1,7*160(a0)
	movep.l		d1,8*160(a0)
	movep.l		d1,9*160(a0)
	movep.l		d1,10*160(a0)
	movep.l		d1,11*160(a0)
	movep.l		d1,12*160(a0)
	movep.l		d1,13*160(a0)
	movep.l		d1,14*160(a0)
	movep.l		d1,15*160(a0)
	
	lea		16*160(a0),a0

	dbra		d3,.ylp8	

*-------------------------------------------------------*
.mask_0:	
*-------------------------------------------------------*
.done:	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts


*-------------------------------------------------------*
_STF_RestoreEdges:
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.savea6:		rs.l	1
.save:			rs.l	10
.return:		rs.l	1
*-------------------------------------------------------*
.xoff:			rs.l	1
*-------------------------------------------------------*
.frame_:		rs.b	0
*-------------------------------------------------------*
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
*-------------------------------------------------------*
	move.w		.xoff+2(a6),d2
	
	and.w		#4-1,d2
			
	lea		maskbuffer,a2
	mulu.w		#(8*maskbuffer_size),d2
	add.l		d2,a2
			
	move.l		(a2)+,d0
	beq		.quit
	move.l		d0,a0
	lea		160-8(a0),a1
	move.w		(a2)+,d3

	move.w		d3,d0
	lsr.w		#4,d3
		
	and.w		#16-1,d0
	neg.w		d0
	add.w		#16,d0
;	lsl.w		#4,d0
	lsl.w		#3,d0
	jmp		.ylp(pc,d0.w)
.ylp:
	rept		16
	move.l		(a2)+,(a0)+
;	move.l		(a2)+,(a1)+
	move.l		(a2)+,(a0)+
;	move.l		(a2)+,(a1)+
	lea		160-8(a0),a0
;	lea		160-8(a1),a1
	endr
	
	dbra		d3,.ylp	

*-------------------------------------------------------*
.quit:	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts
	
*-------------------------------------------------------*
	bss
*-------------------------------------------------------*

maskbuffer:		ds.b	(8*maskbuffer_size)*4

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
	