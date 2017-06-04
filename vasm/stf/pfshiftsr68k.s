*-------------------------------------------------------*

	XDEF		_PR_shift_specrow_Npix
;	XDEF		_P_shift_specrow_Npix


*-------------------------------------------------------*
_PR_shift_specrow_Npix:
*-------------------------------------------------------*
	moveq		#16-1,d0
	and.w		_g_xpreshift,d0
	add.w		d0,d0
	add.w		d0,d0
	jmp		.tab(pc,d0.w)
.tab:	bra.w		_PR_shift_specrow_0pix
	bra.w		_PR_shift_specrow_1pix
	bra.w		_PR_shift_specrow_2pix
	bra.w		_PR_shift_specrow_3pix
	bra.w		_PR_shift_specrow_4pix
	bra.w		_PR_shift_specrow_5pix
	bra.w		_PR_shift_specrow_6pix
	bra.w		_PR_shift_specrow_7pix
	bra.w		_PR_shift_specrow_8pix
	bra.w		_PR_shift_specrow_9pix
	bra.w		_PR_shift_specrow_10pix
	bra.w		_PR_shift_specrow_11pix
	bra.w		_PR_shift_specrow_12pix
	bra.w		_PR_shift_specrow_13pix
	bra.w		_PR_shift_specrow_14pix
	bra.w		_PR_shift_specrow_15pix


*-------------------------------------------------------*
_PR_shift_specrow_0pix:
*-------------------------------------------------------*
;	d3		dy
;	d4		tdx

;	neg.w		d3
;	add.w		#16,d3
;	lsl.w		#3,d3

	move.l		a6,-(sp)

;	lea		8-(160*16)+0(a3),a4
	move.w		#160,d2
	mulu.w		d3,d2
	neg.w		d2
	addq.w		#8,d2
	
	add.w		d3,d3
	add.w		d3,d3
	lea		.jt(pc,d3.w),a6

	move.l		d1,a0				; map xy pos
	bra		.sxl
.xl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0)+,d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

pr_sr_0p		macro
	move.l		(a1)+,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
	move.l		(a1)+,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
	lea		160-8(a4),a4
	lea		160-8(a3),a3
	endm
	
	jmp		(a6)
.jt:	bra.w		.j0	
	bra.w		.j1
	bra.w		.j2
	bra.w		.j3
	bra.w		.j4
	bra.w		.j5
	bra.w		.j6
	bra.w		.j7
	bra.w		.j8
	bra.w		.j9
	bra.w		.j10
	bra.w		.j11
	bra.w		.j12
	bra.w		.j13
	bra.w		.j14
	bra.w		.j15
	bra.w		.j16

.j16	pr_sr_0p
.j15	pr_sr_0p
.j14	pr_sr_0p
.j13	pr_sr_0p
.j12	pr_sr_0p
.j11	pr_sr_0p
.j10	pr_sr_0p
.j9	pr_sr_0p
.j8	pr_sr_0p
.j7	pr_sr_0p
.j6	pr_sr_0p
.j5	pr_sr_0p
.j4	pr_sr_0p
.j3	pr_sr_0p
.j2	pr_sr_0p
.j1	pr_sr_0p
.j0
	add.w		d2,a4
	add.w		d2,a3

.sxl:	dbra		d4,.xl

	move.l		(sp)+,a6

	rts

*-------------------------------------------------------*
_PR_shift_specrow_1pix:
*-------------------------------------------------------*
;	d3		dy
;	d4		tdx

	move.w		#$7FFF,d5
	move.l		#$00010001,d6

	move.l		a6,-(sp)

;	lea		8-(160*16)+0(a3),a4
	move.w		#160,d2
	mulu.w		d3,d2
	neg.w		d2
	addq.w		#8,d2
		
	add.w		d3,d3
	add.w		d3,d3
	lea		.jt(pc,d3.w),a6

	move.l		d1,a0				; map xy pos
	bra		.sxl
.xl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0)+,d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

pr_sr_1p		macro
	rept		2
	move.l		(a1)+,d0
	and.w		d5,d0
	lsl.l		#1,d0
	move.l		(a5)+,d3
	swap		d3
	rol.l		#1,d3
	and.l		d6,d3
	or.l		d3,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
	endr
	lea		160-8(a4),a4
	lea		160-8(a3),a3
	endm
	
	jmp		(a6)
.jt:	bra.w		.j0	
	bra.w		.j1
	bra.w		.j2
	bra.w		.j3
	bra.w		.j4
	bra.w		.j5
	bra.w		.j6
	bra.w		.j7
	bra.w		.j8
	bra.w		.j9
	bra.w		.j10
	bra.w		.j11
	bra.w		.j12
	bra.w		.j13
	bra.w		.j14
	bra.w		.j15
	bra.w		.j16

.j16	pr_sr_1p
.j15	pr_sr_1p
.j14	pr_sr_1p
.j13	pr_sr_1p
.j12	pr_sr_1p
.j11	pr_sr_1p
.j10	pr_sr_1p
.j9	pr_sr_1p
.j8	pr_sr_1p
.j7	pr_sr_1p
.j6	pr_sr_1p
.j5	pr_sr_1p
.j4	pr_sr_1p
.j3	pr_sr_1p
.j2	pr_sr_1p
.j1	pr_sr_1p
.j0
	add.w		d2,a4
	add.w		d2,a3

.sxl:	dbra		d4,.xl

	move.l		(sp)+,a6

	rts

*-------------------------------------------------------*
_PR_shift_specrow_2pix:
*-------------------------------------------------------*
;	d3		dy
;	d4		tdx

	move.w		#$3FFF,d5
	move.l		#$00030003,d6

	move.l		a6,-(sp)

;	lea		8-(160*16)+0(a3),a4
	move.w		#160,d2
	mulu.w		d3,d2
	neg.w		d2
	addq.w		#8,d2
		
	add.w		d3,d3
	add.w		d3,d3
	lea		.jt(pc,d3.w),a6

	move.l		d1,a0				; map xy pos
	bra		.sxl
.xl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0)+,d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr
	
pr_sr_2p		macro
	rept		2
	move.l		(a1)+,d0
	and.w		d5,d0
	lsl.l		#2,d0
	move.l		(a5)+,d3
	swap		d3
	rol.l		#2,d3
	and.l		d6,d3
	or.l		d3,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
	endr
	lea		160-8(a4),a4
	lea		160-8(a3),a3
	endm
	
	jmp		(a6)
.jt:	bra.w		.j0	
	bra.w		.j1
	bra.w		.j2
	bra.w		.j3
	bra.w		.j4
	bra.w		.j5
	bra.w		.j6
	bra.w		.j7
	bra.w		.j8
	bra.w		.j9
	bra.w		.j10
	bra.w		.j11
	bra.w		.j12
	bra.w		.j13
	bra.w		.j14
	bra.w		.j15
	bra.w		.j16

.j16	pr_sr_2p
.j15	pr_sr_2p
.j14	pr_sr_2p
.j13	pr_sr_2p
.j12	pr_sr_2p
.j11	pr_sr_2p
.j10	pr_sr_2p
.j9	pr_sr_2p
.j8	pr_sr_2p
.j7	pr_sr_2p
.j6	pr_sr_2p
.j5	pr_sr_2p
.j4	pr_sr_2p
.j3	pr_sr_2p
.j2	pr_sr_2p
.j1	pr_sr_2p
.j0
	add.w		d2,a4
	add.w		d2,a3

.sxl:	dbra		d4,.xl

	move.l		(sp)+,a6

	rts


*-------------------------------------------------------*
_PR_shift_specrow_3pix:
*-------------------------------------------------------*
;	d3		dy
;	d4		tdx

	move.w		#$1FFF,d5
	move.l		#$00070007,d6

	move.l		a6,-(sp)

;	lea		8-(160*16)+0(a3),a4
	move.w		#160,d2
	mulu.w		d3,d2
	neg.w		d2
	addq.w		#8,d2
		
	add.w		d3,d3
	add.w		d3,d3
	lea		.jt(pc,d3.w),a6

	move.l		d1,a0				; map xy pos
	bra		.sxl
.xl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0)+,d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr
		
pr_sr_3p		macro
	rept		2
	move.l		(a1)+,d0
	and.w		d5,d0
	lsl.l		#3,d0
	move.l		(a5)+,d3
	swap		d3
	rol.l		#3,d3
	and.l		d6,d3
	or.l		d3,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
	endr
	lea		160-8(a4),a4
	lea		160-8(a3),a3
	endm
	
	jmp		(a6)
.jt:	bra.w		.j0	
	bra.w		.j1
	bra.w		.j2
	bra.w		.j3
	bra.w		.j4
	bra.w		.j5
	bra.w		.j6
	bra.w		.j7
	bra.w		.j8
	bra.w		.j9
	bra.w		.j10
	bra.w		.j11
	bra.w		.j12
	bra.w		.j13
	bra.w		.j14
	bra.w		.j15
	bra.w		.j16

.j16	pr_sr_3p
.j15	pr_sr_3p
.j14	pr_sr_3p
.j13	pr_sr_3p
.j12	pr_sr_3p
.j11	pr_sr_3p
.j10	pr_sr_3p
.j9	pr_sr_3p
.j8	pr_sr_3p
.j7	pr_sr_3p
.j6	pr_sr_3p
.j5	pr_sr_3p
.j4	pr_sr_3p
.j3	pr_sr_3p
.j2	pr_sr_3p
.j1	pr_sr_3p
.j0
	add.w		d2,a4
	add.w		d2,a3

.sxl:	dbra		d4,.xl

	move.l		(sp)+,a6

	rts

*-------------------------------------------------------*
_PR_shift_specrow_4pix:
*-------------------------------------------------------*
;	d3		dy
;	d4		tdx

	move.w		#$0FFF,d5
	move.l		#$000F000F,d6

	move.l		a6,-(sp)

;	lea		8-(160*16)+0(a3),a4
	move.w		#160,d2
	mulu.w		d3,d2
	neg.w		d2
	addq.w		#8,d2
		
	add.w		d3,d3
	add.w		d3,d3
	lea		.jt(pc,d3.w),a6

	move.l		d1,a0				; map xy pos
	bra		.sxl
.xl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0)+,d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

pr_sr_4p		macro
	rept		2
	move.l		(a1)+,d0
	and.w		d5,d0
	lsl.l		#4,d0
	move.l		(a5)+,d3
	swap		d3
	rol.l		#4,d3
	and.l		d6,d3
	or.l		d3,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
	endr
	lea		160-8(a4),a4
	lea		160-8(a3),a3
	endm
	
	jmp		(a6)
.jt:	bra.w		.j0	
	bra.w		.j1
	bra.w		.j2
	bra.w		.j3
	bra.w		.j4
	bra.w		.j5
	bra.w		.j6
	bra.w		.j7
	bra.w		.j8
	bra.w		.j9
	bra.w		.j10
	bra.w		.j11
	bra.w		.j12
	bra.w		.j13
	bra.w		.j14
	bra.w		.j15
	bra.w		.j16

.j16	pr_sr_4p
.j15	pr_sr_4p
.j14	pr_sr_4p
.j13	pr_sr_4p
.j12	pr_sr_4p
.j11	pr_sr_4p
.j10	pr_sr_4p
.j9	pr_sr_4p
.j8	pr_sr_4p
.j7	pr_sr_4p
.j6	pr_sr_4p
.j5	pr_sr_4p
.j4	pr_sr_4p
.j3	pr_sr_4p
.j2	pr_sr_4p
.j1	pr_sr_4p
.j0
	add.w		d2,a4
	add.w		d2,a3

.sxl:	dbra		d4,.xl

	move.l		(sp)+,a6

	rts

*-------------------------------------------------------*
_PR_shift_specrow_5pix:
*-------------------------------------------------------*
;	d3		dy
;	d4		tdx

	move.w		#$07FF,d5
	move.l		#$001F001F,d6

	move.l		a6,-(sp)

;	lea		8-(160*16)+0(a3),a4
	move.w		#160,d2
	mulu.w		d3,d2
	neg.w		d2
	addq.w		#8,d2
		
	add.w		d3,d3
	add.w		d3,d3
	lea		.jt(pc,d3.w),a6

	move.l		d1,a0				; map xy pos
	bra		.sxl
.xl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0)+,d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

pr_sr_5p		macro
	rept		2
	move.l		(a1)+,d0
	and.w		d5,d0
	lsl.l		#5,d0
	move.l		(a5)+,d3
	swap		d3
	rol.l		#5,d3
	and.l		d6,d3
	or.l		d3,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
	endr
	lea		160-8(a4),a4
	lea		160-8(a3),a3
	endm
	
	jmp		(a6)
.jt:	bra.w		.j0	
	bra.w		.j1
	bra.w		.j2
	bra.w		.j3
	bra.w		.j4
	bra.w		.j5
	bra.w		.j6
	bra.w		.j7
	bra.w		.j8
	bra.w		.j9
	bra.w		.j10
	bra.w		.j11
	bra.w		.j12
	bra.w		.j13
	bra.w		.j14
	bra.w		.j15
	bra.w		.j16

.j16	pr_sr_5p
.j15	pr_sr_5p
.j14	pr_sr_5p
.j13	pr_sr_5p
.j12	pr_sr_5p
.j11	pr_sr_5p
.j10	pr_sr_5p
.j9	pr_sr_5p
.j8	pr_sr_5p
.j7	pr_sr_5p
.j6	pr_sr_5p
.j5	pr_sr_5p
.j4	pr_sr_5p
.j3	pr_sr_5p
.j2	pr_sr_5p
.j1	pr_sr_5p
.j0
	add.w		d2,a4
	add.w		d2,a3

.sxl:	dbra		d4,.xl

	move.l		(sp)+,a6

	rts

*-------------------------------------------------------*
_PR_shift_specrow_6pix:
*-------------------------------------------------------*
;	d3		dy
;	d4		tdx

	move.w		#$03FF,d5
	move.l		#$003F003F,d6

	move.l		a6,-(sp)

;	lea		8-(160*16)+0(a3),a4
	move.w		#160,d2
	mulu.w		d3,d2
	neg.w		d2
	addq.w		#8,d2
		
	add.w		d3,d3
	add.w		d3,d3
	lea		.jt(pc,d3.w),a6

	move.l		d1,a0				; map xy pos
	bra		.sxl
.xl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0)+,d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

pr_sr_6p		macro
	rept		2
	move.l		(a1)+,d0
	and.w		d5,d0
	lsl.l		#6,d0
	move.l		(a5)+,d3
	swap		d3
	rol.l		#6,d3
	and.l		d6,d3
	or.l		d3,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
	endr
	lea		160-8(a4),a4
	lea		160-8(a3),a3
	endm
	
	jmp		(a6)
.jt:	bra.w		.j0	
	bra.w		.j1
	bra.w		.j2
	bra.w		.j3
	bra.w		.j4
	bra.w		.j5
	bra.w		.j6
	bra.w		.j7
	bra.w		.j8
	bra.w		.j9
	bra.w		.j10
	bra.w		.j11
	bra.w		.j12
	bra.w		.j13
	bra.w		.j14
	bra.w		.j15
	bra.w		.j16

.j16	pr_sr_6p
.j15	pr_sr_6p
.j14	pr_sr_6p
.j13	pr_sr_6p
.j12	pr_sr_6p
.j11	pr_sr_6p
.j10	pr_sr_6p
.j9	pr_sr_6p
.j8	pr_sr_6p
.j7	pr_sr_6p
.j6	pr_sr_6p
.j5	pr_sr_6p
.j4	pr_sr_6p
.j3	pr_sr_6p
.j2	pr_sr_6p
.j1	pr_sr_6p
.j0
	add.w		d2,a4
	add.w		d2,a3

.sxl:	dbra		d4,.xl

	move.l		(sp)+,a6

	rts

*-------------------------------------------------------*
_PR_shift_specrow_7pix:
*-------------------------------------------------------*
;	d3		dy
;	d4		tdx

	move.w		#$01FF,d5
	move.l		#$007F007F,d6

	move.l		a6,-(sp)

;	lea		8-(160*16)+0(a3),a4
	move.w		#160,d2
	mulu.w		d3,d2
	neg.w		d2
	addq.w		#8,d2
		
	add.w		d3,d3
	add.w		d3,d3
	lea		.jt(pc,d3.w),a6

	move.l		d1,a0				; map xy pos
	bra		.sxl
.xl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0)+,d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

pr_sr_7p		macro
	rept		2
	move.l		(a1)+,d0
	and.w		d5,d0
	lsl.l		#7,d0
	move.l		(a5)+,d3
	swap		d3
	rol.l		#7,d3
	and.l		d6,d3
	or.l		d3,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
	endr
	lea		160-8(a4),a4
	lea		160-8(a3),a3
	endm
	
	jmp		(a6)
.jt:	bra.w		.j0	
	bra.w		.j1
	bra.w		.j2
	bra.w		.j3
	bra.w		.j4
	bra.w		.j5
	bra.w		.j6
	bra.w		.j7
	bra.w		.j8
	bra.w		.j9
	bra.w		.j10
	bra.w		.j11
	bra.w		.j12
	bra.w		.j13
	bra.w		.j14
	bra.w		.j15
	bra.w		.j16

.j16	pr_sr_7p
.j15	pr_sr_7p
.j14	pr_sr_7p
.j13	pr_sr_7p
.j12	pr_sr_7p
.j11	pr_sr_7p
.j10	pr_sr_7p
.j9	pr_sr_7p
.j8	pr_sr_7p
.j7	pr_sr_7p
.j6	pr_sr_7p
.j5	pr_sr_7p
.j4	pr_sr_7p
.j3	pr_sr_7p
.j2	pr_sr_7p
.j1	pr_sr_7p
.j0
	add.w		d2,a4
	add.w		d2,a3

.sxl:	dbra		d4,.xl

	move.l		(sp)+,a6

	rts

*-------------------------------------------------------*
_PR_shift_specrow_8pix:
*-------------------------------------------------------*
;	d3		dy
;	d4		tdx

	move.l		a6,-(sp)

;	lea		8-(160*16)+0(a3),a4
	move.w		#160,d2
	mulu.w		d3,d2
	neg.w		d2
	addq.w		#8,d2
		
	add.w		d3,d3
	add.w		d3,d3
	lea		.jt(pc,d3.w),a6

	move.l		d1,a0				; map xy pos
	bra		.sxl
.xl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0)+,d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

pr_sr_8p		macro
	movep.l		1(a1),d0
	movep.l		(a5),d3
	movep.l		d0,(a4)				; write P<-[L:R] 
	movep.l		d3,1(a4)
	move.l		(a4)+,(a3)+			; write Q<-[L:R] 
	move.l		(a4)+,(a3)+	
	addq.l		#8,a1
	addq.l		#8,a5
	lea		160-8(a4),a4
	lea		160-8(a3),a3
	endm
	
	jmp		(a6)
.jt:	bra.w		.j0	
	bra.w		.j1
	bra.w		.j2
	bra.w		.j3
	bra.w		.j4
	bra.w		.j5
	bra.w		.j6
	bra.w		.j7
	bra.w		.j8
	bra.w		.j9
	bra.w		.j10
	bra.w		.j11
	bra.w		.j12
	bra.w		.j13
	bra.w		.j14
	bra.w		.j15
	bra.w		.j16

.j16	pr_sr_8p
.j15	pr_sr_8p
.j14	pr_sr_8p
.j13	pr_sr_8p
.j12	pr_sr_8p
.j11	pr_sr_8p
.j10	pr_sr_8p
.j9	pr_sr_8p
.j8	pr_sr_8p
.j7	pr_sr_8p
.j6	pr_sr_8p
.j5	pr_sr_8p
.j4	pr_sr_8p
.j3	pr_sr_8p
.j2	pr_sr_8p
.j1	pr_sr_8p
.j0
	add.w		d2,a4
	add.w		d2,a3

.sxl:	dbra		d4,.xl

	move.l		(sp)+,a6

	rts

*-------------------------------------------------------*
_PR_shift_specrow_9pix:
*-------------------------------------------------------*
;	d3		dy
;	d4		tdx

	move.l		#$007F007F,d5
	move.w		#$01FF,d6
	
	move.l		a6,-(sp)

;	lea		8-(160*16)+0(a3),a4
	move.w		#160,d2
	mulu.w		d3,d2
	neg.w		d2
	addq.w		#8,d2
		
	add.w		d3,d3
	add.w		d3,d3
	lea		.jt(pc,d3.w),a6

	move.l		d1,a0				; map xy pos
	bra		.sxl
.xl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0)+,d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

pr_sr_9p		macro
	rept		2
	move.l		(a1)+,d0
	and.l		d5,d0
	swap		d0
	ror.l		#16-9,d0
	move.l		(a5)+,d3
	lsr.l		#16-9,d3
	and.w		d6,d3
	or.l		d3,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
	endr
	lea		160-8(a4),a4
	lea		160-8(a3),a3
	endm
	
	jmp		(a6)
.jt:	bra.w		.j0	
	bra.w		.j1
	bra.w		.j2
	bra.w		.j3
	bra.w		.j4
	bra.w		.j5
	bra.w		.j6
	bra.w		.j7
	bra.w		.j8
	bra.w		.j9
	bra.w		.j10
	bra.w		.j11
	bra.w		.j12
	bra.w		.j13
	bra.w		.j14
	bra.w		.j15
	bra.w		.j16

.j16	pr_sr_9p
.j15	pr_sr_9p
.j14	pr_sr_9p
.j13	pr_sr_9p
.j12	pr_sr_9p
.j11	pr_sr_9p
.j10	pr_sr_9p
.j9	pr_sr_9p
.j8	pr_sr_9p
.j7	pr_sr_9p
.j6	pr_sr_9p
.j5	pr_sr_9p
.j4	pr_sr_9p
.j3	pr_sr_9p
.j2	pr_sr_9p
.j1	pr_sr_9p
.j0
	add.w		d2,a4
	add.w		d2,a3

.sxl:	dbra		d4,.xl

	move.l		(sp)+,a6

	rts

*-------------------------------------------------------*
_PR_shift_specrow_10pix:
*-------------------------------------------------------*
;	d3		dy
;	d4		tdx

	move.l		#$003F003F,d5
	move.w		#$03FF,d6
	
	move.l		a6,-(sp)

;	lea		8-(160*16)+0(a3),a4
	move.w		#160,d2
	mulu.w		d3,d2
	neg.w		d2
	addq.w		#8,d2
		
	add.w		d3,d3
	add.w		d3,d3
	lea		.jt(pc,d3.w),a6

	move.l		d1,a0				; map xy pos
	bra		.sxl
.xl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0)+,d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

pr_sr_10p		macro
	rept		2
	move.l		(a1)+,d0
	and.l		d5,d0
	swap		d0
	ror.l		#16-10,d0
	move.l		(a5)+,d3
	lsr.l		#16-10,d3
	and.w		d6,d3
	or.l		d3,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
	endr
	lea		160-8(a4),a4
	lea		160-8(a3),a3
	endm
	
	jmp		(a6)
.jt:	bra.w		.j0	
	bra.w		.j1
	bra.w		.j2
	bra.w		.j3
	bra.w		.j4
	bra.w		.j5
	bra.w		.j6
	bra.w		.j7
	bra.w		.j8
	bra.w		.j9
	bra.w		.j10
	bra.w		.j11
	bra.w		.j12
	bra.w		.j13
	bra.w		.j14
	bra.w		.j15
	bra.w		.j16

.j16	pr_sr_10p
.j15	pr_sr_10p
.j14	pr_sr_10p
.j13	pr_sr_10p
.j12	pr_sr_10p
.j11	pr_sr_10p
.j10	pr_sr_10p
.j9	pr_sr_10p
.j8	pr_sr_10p
.j7	pr_sr_10p
.j6	pr_sr_10p
.j5	pr_sr_10p
.j4	pr_sr_10p
.j3	pr_sr_10p
.j2	pr_sr_10p
.j1	pr_sr_10p
.j0
	add.w		d2,a4
	add.w		d2,a3

.sxl:	dbra		d4,.xl

	move.l		(sp)+,a6

	rts

*-------------------------------------------------------*
_PR_shift_specrow_11pix:
*-------------------------------------------------------*
;	d3		dy
;	d4		tdx

	move.l		#$001F001F,d5
	move.w		#$07FF,d6
	
	move.l		a6,-(sp)

;	lea		8-(160*16)+0(a3),a4
	move.w		#160,d2
	mulu.w		d3,d2
	neg.w		d2
	addq.w		#8,d2
		
	add.w		d3,d3
	add.w		d3,d3
	lea		.jt(pc,d3.w),a6

	move.l		d1,a0				; map xy pos
	bra		.sxl
.xl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0)+,d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

pr_sr_11p		macro
	rept		2
	move.l		(a1)+,d0
	and.l		d5,d0
	swap		d0
	ror.l		#16-11,d0
	move.l		(a5)+,d3
	lsr.l		#16-11,d3
	and.w		d6,d3
	or.l		d3,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
	endr
	lea		160-8(a4),a4
	lea		160-8(a3),a3
	endm
	
	jmp		(a6)
.jt:	bra.w		.j0	
	bra.w		.j1
	bra.w		.j2
	bra.w		.j3
	bra.w		.j4
	bra.w		.j5
	bra.w		.j6
	bra.w		.j7
	bra.w		.j8
	bra.w		.j9
	bra.w		.j10
	bra.w		.j11
	bra.w		.j12
	bra.w		.j13
	bra.w		.j14
	bra.w		.j15
	bra.w		.j16

.j16	pr_sr_11p
.j15	pr_sr_11p
.j14	pr_sr_11p
.j13	pr_sr_11p
.j12	pr_sr_11p
.j11	pr_sr_11p
.j10	pr_sr_11p
.j9	pr_sr_11p
.j8	pr_sr_11p
.j7	pr_sr_11p
.j6	pr_sr_11p
.j5	pr_sr_11p
.j4	pr_sr_11p
.j3	pr_sr_11p
.j2	pr_sr_11p
.j1	pr_sr_11p
.j0
	add.w		d2,a4
	add.w		d2,a3

.sxl:	dbra		d4,.xl

	move.l		(sp)+,a6

	rts

*-------------------------------------------------------*
_PR_shift_specrow_12pix:
*-------------------------------------------------------*
;	d3		dy
;	d4		tdx

	move.l		#$000F000F,d5
	move.w		#$0FFF,d6
	
	move.l		a6,-(sp)

;	lea		8-(160*16)+0(a3),a4
	move.w		#160,d2
	mulu.w		d3,d2
	neg.w		d2
	addq.w		#8,d2
		
	add.w		d3,d3
	add.w		d3,d3
	lea		.jt(pc,d3.w),a6

	move.l		d1,a0				; map xy pos
	bra		.sxl
.xl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0)+,d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

pr_sr_12p		macro
	rept		2
	move.l		(a1)+,d0
	and.l		d5,d0
	swap		d0
	ror.l		#16-12,d0
	move.l		(a5)+,d3
	lsr.l		#16-12,d3
	and.w		d6,d3
	or.l		d3,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
	endr
	lea		160-8(a4),a4
	lea		160-8(a3),a3
	endm
	
	jmp		(a6)
.jt:	bra.w		.j0	
	bra.w		.j1
	bra.w		.j2
	bra.w		.j3
	bra.w		.j4
	bra.w		.j5
	bra.w		.j6
	bra.w		.j7
	bra.w		.j8
	bra.w		.j9
	bra.w		.j10
	bra.w		.j11
	bra.w		.j12
	bra.w		.j13
	bra.w		.j14
	bra.w		.j15
	bra.w		.j16

.j16	pr_sr_12p
.j15	pr_sr_12p
.j14	pr_sr_12p
.j13	pr_sr_12p
.j12	pr_sr_12p
.j11	pr_sr_12p
.j10	pr_sr_12p
.j9	pr_sr_12p
.j8	pr_sr_12p
.j7	pr_sr_12p
.j6	pr_sr_12p
.j5	pr_sr_12p
.j4	pr_sr_12p
.j3	pr_sr_12p
.j2	pr_sr_12p
.j1	pr_sr_12p
.j0
	add.w		d2,a4
	add.w		d2,a3

.sxl:	dbra		d4,.xl

	move.l		(sp)+,a6

	rts


*-------------------------------------------------------*
_PR_shift_specrow_13pix:
*-------------------------------------------------------*
;	d3		dy
;	d4		tdx

	move.l		#$00070007,d5
	move.w		#$1FFF,d6
	
	move.l		a6,-(sp)

;	lea		8-(160*16)+0(a3),a4
	move.w		#160,d2
	mulu.w		d3,d2
	neg.w		d2
	addq.w		#8,d2
		
	add.w		d3,d3
	add.w		d3,d3
	lea		.jt(pc,d3.w),a6

	move.l		d1,a0				; map xy pos
	bra		.sxl
.xl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0)+,d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

pr_sr_13p		macro
	rept		2
	move.l		(a1)+,d0
	and.l		d5,d0
	swap		d0
	ror.l		#16-13,d0
	move.l		(a5)+,d3
	lsr.l		#16-13,d3
	and.w		d6,d3
	or.l		d3,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
	endr
	lea		160-8(a4),a4
	lea		160-8(a3),a3
	endm
	
	jmp		(a6)
.jt:	bra.w		.j0	
	bra.w		.j1
	bra.w		.j2
	bra.w		.j3
	bra.w		.j4
	bra.w		.j5
	bra.w		.j6
	bra.w		.j7
	bra.w		.j8
	bra.w		.j9
	bra.w		.j10
	bra.w		.j11
	bra.w		.j12
	bra.w		.j13
	bra.w		.j14
	bra.w		.j15
	bra.w		.j16

.j16	pr_sr_13p
.j15	pr_sr_13p
.j14	pr_sr_13p
.j13	pr_sr_13p
.j12	pr_sr_13p
.j11	pr_sr_13p
.j10	pr_sr_13p
.j9	pr_sr_13p
.j8	pr_sr_13p
.j7	pr_sr_13p
.j6	pr_sr_13p
.j5	pr_sr_13p
.j4	pr_sr_13p
.j3	pr_sr_13p
.j2	pr_sr_13p
.j1	pr_sr_13p
.j0
	add.w		d2,a4
	add.w		d2,a3

.sxl:	dbra		d4,.xl

	move.l		(sp)+,a6

	rts

*-------------------------------------------------------*
_PR_shift_specrow_14pix:
*-------------------------------------------------------*
;	d3		dy
;	d4		tdx

	move.l		#$00030003,d5
	move.w		#$3FFF,d6
	
	move.l		a6,-(sp)

;	lea		8-(160*16)+0(a3),a4
	move.w		#160,d2
	mulu.w		d3,d2
	neg.w		d2
	addq.w		#8,d2
		
	add.w		d3,d3
	add.w		d3,d3
	lea		.jt(pc,d3.w),a6

	move.l		d1,a0				; map xy pos
	bra		.sxl
.xl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0)+,d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

pr_sr_14p		macro
	rept		2
	move.l		(a1)+,d0
	and.l		d5,d0
	swap		d0
	ror.l		#16-14,d0
	move.l		(a5)+,d3
	lsr.l		#16-14,d3
	and.w		d6,d3
	or.l		d3,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
	endr
	lea		160-8(a4),a4
	lea		160-8(a3),a3
	endm
	
	jmp		(a6)
.jt:	bra.w		.j0	
	bra.w		.j1
	bra.w		.j2
	bra.w		.j3
	bra.w		.j4
	bra.w		.j5
	bra.w		.j6
	bra.w		.j7
	bra.w		.j8
	bra.w		.j9
	bra.w		.j10
	bra.w		.j11
	bra.w		.j12
	bra.w		.j13
	bra.w		.j14
	bra.w		.j15
	bra.w		.j16

.j16	pr_sr_14p
.j15	pr_sr_14p
.j14	pr_sr_14p
.j13	pr_sr_14p
.j12	pr_sr_14p
.j11	pr_sr_14p
.j10	pr_sr_14p
.j9	pr_sr_14p
.j8	pr_sr_14p
.j7	pr_sr_14p
.j6	pr_sr_14p
.j5	pr_sr_14p
.j4	pr_sr_14p
.j3	pr_sr_14p
.j2	pr_sr_14p
.j1	pr_sr_14p
.j0
	add.w		d2,a4
	add.w		d2,a3

.sxl:	dbra		d4,.xl

	move.l		(sp)+,a6

	rts


*-------------------------------------------------------*
_PR_shift_specrow_15pix:
*-------------------------------------------------------*
;	d3		dy
;	d4		tdx

	move.l		#$00010001,d5
	move.w		#$7FFF,d6
	
	move.l		a6,-(sp)

;	lea		8-(160*16)+0(a3),a4
	move.w		#160,d2
	mulu.w		d3,d2
	neg.w		d2
	addq.w		#8,d2
		
	add.w		d3,d3
	add.w		d3,d3
	lea		.jt(pc,d3.w),a6

	move.l		d1,a0				; map xy pos
	bra		.sxl
.xl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0)+,d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

pr_sr_15p		macro
	rept		2
	move.l		(a1)+,d0
	and.l		d5,d0
	swap		d0
	ror.l		#16-15,d0
	move.l		(a5)+,d3
	lsr.l		#16-15,d3
	and.w		d6,d3
	or.l		d3,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
	endr
	lea		160-8(a4),a4
	lea		160-8(a3),a3
	endm
	
	jmp		(a6)
.jt:	bra.w		.j0	
	bra.w		.j1
	bra.w		.j2
	bra.w		.j3
	bra.w		.j4
	bra.w		.j5
	bra.w		.j6
	bra.w		.j7
	bra.w		.j8
	bra.w		.j9
	bra.w		.j10
	bra.w		.j11
	bra.w		.j12
	bra.w		.j13
	bra.w		.j14
	bra.w		.j15
	bra.w		.j16

.j16	pr_sr_15p
.j15	pr_sr_15p
.j14	pr_sr_15p
.j13	pr_sr_15p
.j12	pr_sr_15p
.j11	pr_sr_15p
.j10	pr_sr_15p
.j9	pr_sr_15p
.j8	pr_sr_15p
.j7	pr_sr_15p
.j6	pr_sr_15p
.j5	pr_sr_15p
.j4	pr_sr_15p
.j3	pr_sr_15p
.j2	pr_sr_15p
.j1	pr_sr_15p
.j0
	add.w		d2,a4
	add.w		d2,a3

.sxl:	dbra		d4,.xl

	move.l		(sp)+,a6

	rts
