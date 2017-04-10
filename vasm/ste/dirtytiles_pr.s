
*------------------------------------------------------------------------------*

; a4,d0,d7,d6,a3,a5,a0,a2

tile_pr		macro


	lea	\0*8(\1),\5
	lea	\0*8(\1,\4.l),a6

	move.l	\2,\6
	
	rept	16
	move.l	(\6)+,\2
	move.l	\2,(\5)+
	move.l	\2,(a6)+
	move.l	(\6)+,\2
	move.l	\2,(\5)+
	move.l	\2,(a6)+
	lea	dst_linewid-8(\5),\5
	lea	dst_linewid-8(a6),a6
	endr
	

	ifd		qqqq

	lea		\0*8(\1),\5

	move.l		\2,\6
	
	rept		16	
	move.l		(\6)+,(\5)+
	move.l		(\6)+,(\5)+
	lea		dst_linewid-8(\5),\5
	endr

	lea		\0*8(\1,\4.l),\5

	move.l		\2,\6
	
	rept		16
	move.l		(\6)+,(\5)+
	move.l		(\6)+,(\5)+
	lea		dst_linewid-8(\5),\5
	endr

	endc		;qqqq

	endm

*------------------------------------------------------------------------------*
*	refresh only dirty tiles within rectangle pi,pj->pi2,pj2
*	note: called either 1,2 or 4 times per frame dependent on layout & pos
*------------------------------------------------------------------------------*
_do_dirtyrect_pr:	xdef	_do_dirtyrect_pr
*------------------------------------------------------------------------------*

	movem.l		d0-a6,-(sp)
	
	ifd		TIMING_RASTERS
	move.w		#$753,d0
	move.w		d0,$ffff8240.w
	move.w		d0,$64.w
	endc
	
;	bra		.done

*********************************************************
* todo: set up parameters properly from call context
*********************************************************

	move.l		_dm_dirtymask,a1
	move.l		_dm_dstaddr,a4
	move.l		_dm_ptiles,a2
	move.l		_dm_pmap,d1
	move.w		_dm_visible_linebytes,d7
	move.l		_dm_virtual_pagebytes,d6
	moveq		#0,d2
	move.w		_dm_map_tiles_x,d2
	lsl.l		#2,d2
	
;	bra		*
	
;	move.w		d7,a6
;	add.w		d7,a6


	; check for degenerate rectangle (should never happen, if the C code is right)
;	move.w		_dm_pi2,d5
;	sub.w		_dm_pi,d5
;	ble		.done

	; find rectangle right edge (in 8bit blocks)
	moveq		#8-1,d5
	add.w		_dm_pi2,d5
	and.w		#-8,d5				; rectangle right edge (in 8bit blocks)

	; find dirty mask buffer starting position (right edge, 1 8bit block = 1 word, upper byte)
;	add.w		d5,a1
;	add.w		d5,a1

	; find rectangle left edge (in 8bit blocks)
	moveq		#-8,d3
	and.w		_dm_pi,d3			; rectangle left edge (in 8bit blocks)

	sub.w		d3,d5				; rectangle width (in 8bit blocks)


	; generate mask d5 for EM1:EM2:EM3 based on 8bit block offset and rectangle width

;	10000000	1
;	11000000	2
;	11100000	3
;	11110000	4
;	11111000	5
;	11111100	6
;	11111110	7
;	11111111	0

;	find rectangle width in blocks

	moveq		#8-1,d3
	add.w		_dm_pi2,d3
	moveq		#-8,d4
	and.w		d4,d3
	and.w		_dm_pi,d4
	sub.w		d4,d3

;	generate right mask e.g. FFFFFF80

	move.w		_dm_pi2,d5
	subq.w		#1,d5
	and.w		#8-1,d5
	addq.w		#1,d5
	moveq		#-1,d0
	clr.b		d0
	asr.l		d5,d0

;	generate left mask  e.g. 0007FFFF

	moveq		#8-1,d4
	and.w		_dm_pi,d4
;	move.w		rectblocks,d5

;	move.w		d3,d5
;	subq		#4,d5		; start of register is 4 'blocks' to the left
;	lsl.w		#3,d5
;	sub.w		d5,d4

	sub.w		d3,d4
	add.w		#4*8,d4

	moveq		#-1,d5
	lsr.l		d4,d5

;	combine masks       e.g. 0007FF80
;	note: only LS 24bits used

	and.l		d0,d5

	; find rectangle height

	move.w		_dm_pj2,d4
	sub.w		_dm_pj,d4
	subq.w		#1,d4

;	d0 : *
;	d1 : map source
;	d2 : map x stride
;	d3 : rectangle width in ceil(rx2)-floor(rx1) blocks
;	d4 : ycount-1
;	d5 : rectangle mask, 8bit-aligned
;	d6 : dstpgh
;	d7 : dstpgw

;	a0 : *
;	a1 : dirty mask source, word-aligned bytes
;	a2 : tilesrc
;	a3 : *
;	a4 : dst
;	a5 : *
;	a6 : dst base

	; select 1,2,3-wide fetch from rectangle width in 8bit blocks

	lsr.w		#3,d3
	add.w		d3,d3
;	bra		*
	
;	move.w		rectblocks,d0
	move.w		.jtab-2(pc,d3.w),d0
	; dirty mask buffer stride, in bytes (8bit blocks*2)
;	move.w		_dm_maskstride,d3
	jmp		.jtab(pc,d0.w)
.jtab:	dc.w		.start1-.jtab
	dc.w		.start2-.jtab
	dc.w		.start3-.jtab

.done:
	ifd		TIMING_RASTERS
	move.w		#$777,d0
	move.w		d0,$ffff8240.w
	move.w		d0,$64.w
	endc

	movem.l		(sp)+,d0-a6
	rts

*------------------------------------------------------------------------------*

	; extract 8-24 relevant bits for rectangle right edge (right-justified)
	; note: visible region normally 20+1 tiles wide max so 21 bits max
	; todo: replace bsr's with dedicated code per case

;	d0 : *
;	d1 : map source
;	d2 : map x stride
;	d3 : dirty mask source stride in bytes / * dirty mask data
;	d4 : ycount-1
;	d5 : rectangle mask, 8bit-aligned
;	d6 : dstpgh
;	d7 : dstpgw

;	a0 : *
;	a1 : dirty mask source, word-aligned bytes
;	a2 : tilesrc
;	a3 : *
;	a4 : dst
;	a5 : *
;	a6 : dst base


*------------------------------------------------------------------------------*

.start3:
	move.w		_dm_lineskip,.smc3d+2
	move.w		_dm_maskstride,.smc3+2

.ylp3:
	; advance Y
	
	move.l		d1,a0
	add.l		d2,d1

	; check row

	movep.l		-2(a1),d3		; xx:AA:BB:CC <-[AA:BB:CC:xx]
	move.l		d3,d0
	and.l		d5,d3
	beq.s		.next3
	eor.l		d3,d0
	movep.l		d0,-2(a1)

	; dirty row

	ifd		TIMING_RASTERS
	move.w		#$707,d0
	move.w		d0,$ffff8240.w
	move.w		d0,$64.w
	endc

	lsl.l		#3,d3
	bsr		.do_block
	lsr.l		#8,d3			; 00:xx:AA:BB
	bsr		.do_block
	lsr.l		#8,d3			; 00:00:xx:AA
	bsr		.do_block

	lea		8*8*3(a4),a4

	ifd		TIMING_RASTERS
	move.w		#$753,d0
	move.w		d0,$ffff8240.w
	move.w		d0,$64.w
	endc

.next3:

.smc3d:	lea		1234(a4),a4

	; next Y

.smc3:	lea		1234(a1),a1
	dbra		d4,.ylp3
	bra		.done

*------------------------------------------------------------------------------*

.start2:
	move.w		_dm_lineskip,.smc2d+2
	move.w		_dm_maskstride,.smc2+2
.ylp2:
	; advance Y
	
	move.l		d1,a0
	add.l		d2,d1

	; check row

	movep.w		0(a1),d3		; xx:xx:BB:CC
	move.w		d3,d0
	and.w		d5,d3
	beq.s		.next2
	eor.w		d3,d0
	movep.w		d0,0(a1)

	; dirty row

	ifd		TIMING_RASTERS
	move.w		#$707,d0
	move.w		d0,$ffff8240.w
	move.w		d0,$64.w
	endc

	lsl.l		#3,d3
	bsr		.do_block
	lsr.l		#8,d3			; xx:xx:00:BB
	bsr		.do_block

	lea		8*8*2(a4),a4

	ifd		TIMING_RASTERS
	move.w		#$753,d0
	move.w		d0,$ffff8240.w
	move.w		d0,$64.w
	endc

.next2:

.smc2d:	lea		1234(a4),a4

	; next Y

.smc2:	lea		1234(a1),a1
	dbra		d4,.ylp2
	bra		.done

*------------------------------------------------------------------------------*

.start1:
	move.w		_dm_lineskip,.smc1d+2
	move.w		_dm_maskstride,.smc1+2
.ylp1:
	; advance Y

	move.l		d1,a0
	add.l		d2,d1

	; check row

	move.b		(a1),d3			; xx:xx:xx:CC
	move.b		d3,d0
	and.b		d5,d3
	beq.s		.next1
	eor.b		d3,d0
	move.b		d0,(a1)

	; dirty row

	ifd		TIMING_RASTERS
	move.w		#$707,d0
	move.w		d0,$ffff8240.w
	move.w		d0,$64.w
	endc

	lsl.w		#3,d3
	bsr		.do_block

	lea		8*8*1(a4),a4

	ifd		TIMING_RASTERS
	move.w		#$753,d0
	move.w		d0,$ffff8240.w
	move.w		d0,$64.w
	endc
	
.next1:

.smc1d:	lea		1234(a4),a4

	; next Y

.smc1:	lea		1234(a1),a1
	dbra		d4,.ylp1
	bra		.done



*------------------------------------------------------------------------------*
*	emit single flag-conditional tile
*------------------------------------------------------------------------------*

; d3.l,	a4.l,	d0.l,	d7.w,	d6.l,	a3,	a5,	a0,	a2
; flags	dst	src	dstpgw	dstpgh	tmp	tmp	mapsrc	tilesrc

;ctile_pr		macro
;
;	; test dirty flag
;
;	add.b		\1,\1
;	bcc		.fclr\@
;
;	; address tile source
;
;	move.l		\9,\3			; tile library
;	add.l		\0*4(\8),\3		; map index
;
;	; plot tile to relevant vpage(s)
;
;	tile_pr.\0	\2,\3,\4,\5,\6,\7
;.fclr\@:
;	endm
		
*------------------------------------------------------------------------------*
*	emit up to 8 tiles, left-to-right, one per set bit in d0
*------------------------------------------------------------------------------*
*	note: at least 1 bit set in d0 if reached this point
*	note: blocks themselves applied right-to-left, starting at right edge
*	note: d7 (dstpgw) becomes free for P,PR page layouts (PQ,PQRS not really used anyway)
*------------------------------------------------------------------------------*
*	todo: break 8 tests into 2x nibble patterns for quicker sub-rejection
*	todo: turn each nibble into a jmptab with dedicated code per pattern
*------------------------------------------------------------------------------*
;.do_block_:
*------------------------------------------------------------------------------*
;	ctile_pr.-8	d3,a4,d0,d7,d6,a3,a5,a0,a2
;	ctile_pr.-7	d3,a4,d0,d7,d6,a3,a5,a0,a2
;	ctile_pr.-6	d3,a4,d0,d7,d6,a3,a5,a0,a2
;	ctile_pr.-5	d3,a4,d0,d7,d6,a3,a5,a0,a2
;	ctile_pr.-4	d3,a4,d0,d7,d6,a3,a5,a0,a2
;	ctile_pr.-3	d3,a4,d0,d7,d6,a3,a5,a0,a2
;	ctile_pr.-2	d3,a4,d0,d7,d6,a3,a5,a0,a2
;	ctile_pr.-1	d3,a4,d0,d7,d6,a3,a5,a0,a2
;
;;	now move 8 tiles to the left for the next block
;
;	lea		-8*4(a0),a0
;	lea		-8*8(a4),a4
;
;	rts



*------------------------------------------------------------------------------*

jtile_pr		macro

	; address tile source

	move.l		\8,\2			; tile library
	add.l		\0*4(\7),\2		; map index

	; plot tile to relevant vpage(s)

	tile_pr.\0	\1,\2,\3,\4,\5,\6

	endm
	
*------------------------------------------------------------------------------*
.do_block:
*------------------------------------------------------------------------------*
*	schedule 2 patterns of 4 bits & execute
*------------------------------------------------------------------------------*
	move.w		#$07f8,d0
	and.w		d3,d0
	lea		.jt(pc,d0.w),a5
	move.l		(a5)+,-(sp)
	move.l		(a5),a3
	jmp		(a3)

.jt:
	rept		16
	dc.l		.do_block_0000,.do_block_0000
	dc.l		.do_block_0000,.do_block_0001
	dc.l		.do_block_0000,.do_block_0010
	dc.l		.do_block_0000,.do_block_0011
	dc.l		.do_block_0000,.do_block_0100
	dc.l		.do_block_0000,.do_block_0101
	dc.l		.do_block_0000,.do_block_0110
	dc.l		.do_block_0000,.do_block_0111
	dc.l		.do_block_0000,.do_block_1000
	dc.l		.do_block_0000,.do_block_1001
	dc.l		.do_block_0000,.do_block_1010
	dc.l		.do_block_0000,.do_block_1011
	dc.l		.do_block_0000,.do_block_1100
	dc.l		.do_block_0000,.do_block_1101
	dc.l		.do_block_0000,.do_block_1110
	dc.l		.do_block_0000,.do_block_1111
	endr

*------------------------------------------------------------------------------*
*	cascade tiles into patterns of 4 bits
*------------------------------------------------------------------------------*

.do_block_1111:
	jtile_pr.-4	a4,d0,d7,d6,a3,a5,a0,a2
.do_block_0111:
	jtile_pr.-3	a4,d0,d7,d6,a3,a5,a0,a2
.do_block_0011:
	jtile_pr.-2	a4,d0,d7,d6,a3,a5,a0,a2
.do_block_0001:
	jtile_pr.-1	a4,d0,d7,d6,a3,a5,a0,a2
	lea		-4*4(a0),a0
	lea		-4*8(a4),a4	
	rts

.do_block_1110:
	jtile_pr.-2	a4,d0,d7,d6,a3,a5,a0,a2
.do_block_1100:
	jtile_pr.-3	a4,d0,d7,d6,a3,a5,a0,a2
.do_block_1000:
	jtile_pr.-4	a4,d0,d7,d6,a3,a5,a0,a2
.do_block_0000:
	lea		-4*4(a0),a0
	lea		-4*8(a4),a4	
	rts

.do_block_0110:
	jtile_pr.-2	a4,d0,d7,d6,a3,a5,a0,a2
.do_block_0100:
	jtile_pr.-3	a4,d0,d7,d6,a3,a5,a0,a2
	lea		-4*4(a0),a0
	lea		-4*8(a4),a4	
	rts
	
.do_block_1011:
	jtile_pr.-2	a4,d0,d7,d6,a3,a5,a0,a2
.do_block_1001:
	jtile_pr.-1	a4,d0,d7,d6,a3,a5,a0,a2
	jtile_pr.-4	a4,d0,d7,d6,a3,a5,a0,a2
	lea		-4*4(a0),a0
	lea		-4*8(a4),a4	
	rts
		
.do_block_1101:
	jtile_pr.-4	a4,d0,d7,d6,a3,a5,a0,a2
.do_block_0101:
	jtile_pr.-3	a4,d0,d7,d6,a3,a5,a0,a2
	jtile_pr.-1	a4,d0,d7,d6,a3,a5,a0,a2
	lea		-4*4(a0),a0
	lea		-4*8(a4),a4	
	rts
	
.do_block_1010:
	jtile_pr.-4	a4,d0,d7,d6,a3,a5,a0,a2
.do_block_0010:
	jtile_pr.-2	a4,d0,d7,d6,a3,a5,a0,a2
	lea		-4*4(a0),a0
	lea		-4*8(a4),a4	
	rts
	
*------------------------------------------------------------------------------*

pairtable_pr		=	.jt

*------------------------------------------------------------------------------*
dirtytile_setup_pr:
*------------------------------------------------------------------------------*
	lea		pairtable_pr,a0
	bra		dirtytile_setup
	
*------------------------------------------------------------------------------*
