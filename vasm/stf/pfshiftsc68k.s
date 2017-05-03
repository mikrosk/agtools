*-------------------------------------------------------*

	XDEF		_PR_shift_speccolumn_Npix
	XDEF		_P_shift_speccolumn_Npix

*-------------------------------------------------------*

pr_sc_hshift		macro
	move.l		(a1)+,d0
	and.l		d5,d0
	swap		d0
	ror.l		#16-\1,d0
	move.l		(a5)+,d3
	lsr.l		#16-\1,d3
	and.w		d6,d3
	or.l		d3,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
	endm

pr_sc_lshift		macro
	move.l		(a1)+,d0
	and.w		d5,d0
	lsl.l		#\1,d0
	move.l		(a5)+,d3
	swap		d3
	rol.l		#\1,d3
	and.l		d6,d3
	or.l		d3,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
	endm

pr_sc_4bpl		macro
	ifeq		(\1-0)

	; shift=0
	move.l		(a1)+,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
	move.l		(a1)+,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
		
	else
	
	ifeq		(\1-8)

	; shift=8
	movep.l		1(a1),d0
	movep.l		(a5),d3
	movep.l		d0,(a4)				; write P<-[L:R] 
	movep.l		d3,1(a4)
	move.l		(a4)+,(a3)+			; write Q<-[L:R] 
	move.l		(a4)+,(a3)+	
	addq.l		#8,a1
	addq.l		#8,a5
			
	else

	; shift=?
	rept		2
	ifge		(\1-8)
	pr_sc_hshift	\1				; 8-15
	else
	pr_sc_lshift	\1				; 0-7	
	endc
	endr

	endc
	endc

	lea		160-8(a4),a4
	lea		160-8(a3),a3
	
	endm

*-------------------------------------------------------*
_PR_shift_speccolumn_Npix:
*-------------------------------------------------------*

;	move.w		#160,d1
;	mulu.w		d3,d1
;	neg.w		d1
;	addq.w		#8,d1	
	
	moveq		#16-1,d0
	and.w		_g_xpreshift,d0
	lsl.w		#2,d0
	jmp		.tab(pc,d0.w)
.tab:	bra.w		_PR_shift_speccolumn_0pix
	bra.w		_PR_shift_speccolumn_1pix
	bra.w		_PR_shift_speccolumn_2pix
	bra.w		_PR_shift_speccolumn_3pix
	bra.w		_PR_shift_speccolumn_4pix
	bra.w		_PR_shift_speccolumn_5pix
	bra.w		_PR_shift_speccolumn_6pix
	bra.w		_PR_shift_speccolumn_7pix
	bra.w		_PR_shift_speccolumn_8pix
	bra.w		_PR_shift_speccolumn_9pix
	bra.w		_PR_shift_speccolumn_10pix
	bra.w		_PR_shift_speccolumn_11pix
	bra.w		_PR_shift_speccolumn_12pix
	bra.w		_PR_shift_speccolumn_13pix
	bra.w		_PR_shift_speccolumn_14pix
	bra.w		_PR_shift_speccolumn_15pix

*-------------------------------------------------------*
_PR_shift_speccolumn_15pix:
*-------------------------------------------------------*
	move.l		#$00010001,d5
	move.w		#$7FFF,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)
	
	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	pr_sc_4bpl	15
.j15	pr_sc_4bpl	15
.j14	pr_sc_4bpl	15
.j13	pr_sc_4bpl	15
.j12	pr_sc_4bpl	15
.j11	pr_sc_4bpl	15
.j10	pr_sc_4bpl	15
.j9	pr_sc_4bpl	15
.j8	pr_sc_4bpl	15
.j7	pr_sc_4bpl	15
.j6	pr_sc_4bpl	15
.j5	pr_sc_4bpl	15
.j4	pr_sc_4bpl	15
.j3	pr_sc_4bpl	15
.j2	pr_sc_4bpl	15
.j1	pr_sc_4bpl	15
.j0
	add.w		d1,a3
	add.w		d1,a4

.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts

*-------------------------------------------------------*
_PR_shift_speccolumn_14pix:
*-------------------------------------------------------*
	move.l		#$00030003,d5
	move.w		#$3FFF,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)
	
	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	pr_sc_4bpl	14
.j15	pr_sc_4bpl	14
.j14	pr_sc_4bpl	14
.j13	pr_sc_4bpl	14
.j12	pr_sc_4bpl	14
.j11	pr_sc_4bpl	14
.j10	pr_sc_4bpl	14
.j9	pr_sc_4bpl	14
.j8	pr_sc_4bpl	14
.j7	pr_sc_4bpl	14
.j6	pr_sc_4bpl	14
.j5	pr_sc_4bpl	14
.j4	pr_sc_4bpl	14
.j3	pr_sc_4bpl	14
.j2	pr_sc_4bpl	14
.j1	pr_sc_4bpl	14
.j0
	add.w		d1,a3
	add.w		d1,a4
	
.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts

*-------------------------------------------------------*
_PR_shift_speccolumn_13pix:
*-------------------------------------------------------*
	move.l		#$00070007,d5
	move.w		#$1FFF,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	pr_sc_4bpl	13
.j15	pr_sc_4bpl	13
.j14	pr_sc_4bpl	13
.j13	pr_sc_4bpl	13
.j12	pr_sc_4bpl	13
.j11	pr_sc_4bpl	13
.j10	pr_sc_4bpl	13
.j9	pr_sc_4bpl	13
.j8	pr_sc_4bpl	13
.j7	pr_sc_4bpl	13
.j6	pr_sc_4bpl	13
.j5	pr_sc_4bpl	13
.j4	pr_sc_4bpl	13
.j3	pr_sc_4bpl	13
.j2	pr_sc_4bpl	13
.j1	pr_sc_4bpl	13
.j0
	add.w		d1,a3
	add.w		d1,a4
	
.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts
	
*-------------------------------------------------------*
_PR_shift_speccolumn_12pix:
*-------------------------------------------------------*
	move.l		#$000F000F,d5
	move.w		#$0FFF,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)
	
	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	pr_sc_4bpl	12
.j15	pr_sc_4bpl	12
.j14	pr_sc_4bpl	12
.j13	pr_sc_4bpl	12
.j12	pr_sc_4bpl	12
.j11	pr_sc_4bpl	12
.j10	pr_sc_4bpl	12
.j9	pr_sc_4bpl	12
.j8	pr_sc_4bpl	12
.j7	pr_sc_4bpl	12
.j6	pr_sc_4bpl	12
.j5	pr_sc_4bpl	12
.j4	pr_sc_4bpl	12
.j3	pr_sc_4bpl	12
.j2	pr_sc_4bpl	12
.j1	pr_sc_4bpl	12
.j0
	add.w		d1,a3
	add.w		d1,a4
	
.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts
	
*-------------------------------------------------------*
_PR_shift_speccolumn_11pix:
*-------------------------------------------------------*
	move.l		#$001F001F,d5
	move.w		#$07FF,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)
	
	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	pr_sc_4bpl	11
.j15	pr_sc_4bpl	11
.j14	pr_sc_4bpl	11
.j13	pr_sc_4bpl	11
.j12	pr_sc_4bpl	11
.j11	pr_sc_4bpl	11
.j10	pr_sc_4bpl	11
.j9	pr_sc_4bpl	11
.j8	pr_sc_4bpl	11
.j7	pr_sc_4bpl	11
.j6	pr_sc_4bpl	11
.j5	pr_sc_4bpl	11
.j4	pr_sc_4bpl	11
.j3	pr_sc_4bpl	11
.j2	pr_sc_4bpl	11
.j1	pr_sc_4bpl	11
.j0
	add.w		d1,a3
	add.w		d1,a4

.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts
	
*-------------------------------------------------------*
_PR_shift_speccolumn_10pix:
*-------------------------------------------------------*
	move.l		#$003F003F,d5
	move.w		#$03FF,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)
	
	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	pr_sc_4bpl	10
.j15	pr_sc_4bpl	10
.j14	pr_sc_4bpl	10
.j13	pr_sc_4bpl	10
.j12	pr_sc_4bpl	10
.j11	pr_sc_4bpl	10
.j10	pr_sc_4bpl	10
.j9	pr_sc_4bpl	10
.j8	pr_sc_4bpl	10
.j7	pr_sc_4bpl	10
.j6	pr_sc_4bpl	10
.j5	pr_sc_4bpl	10
.j4	pr_sc_4bpl	10
.j3	pr_sc_4bpl	10
.j2	pr_sc_4bpl	10
.j1	pr_sc_4bpl	10
.j0
	add.w		d1,a3
	add.w		d1,a4
	
.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts

*-------------------------------------------------------*
_PR_shift_speccolumn_9pix:
*-------------------------------------------------------*
	move.l		#$007F007F,d5
	move.w		#$01FF,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	pr_sc_4bpl	9
.j15	pr_sc_4bpl	9
.j14	pr_sc_4bpl	9
.j13	pr_sc_4bpl	9
.j12	pr_sc_4bpl	9
.j11	pr_sc_4bpl	9
.j10	pr_sc_4bpl	9
.j9	pr_sc_4bpl	9
.j8	pr_sc_4bpl	9
.j7	pr_sc_4bpl	9
.j6	pr_sc_4bpl	9
.j5	pr_sc_4bpl	9
.j4	pr_sc_4bpl	9
.j3	pr_sc_4bpl	9
.j2	pr_sc_4bpl	9
.j1	pr_sc_4bpl	9
.j0
	add.w		d1,a3
	add.w		d1,a4

.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts

*-------------------------------------------------------*
_PR_shift_speccolumn_8pix:
*-------------------------------------------------------*
; 	shiftmask is hardcoded
	
	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	pr_sc_4bpl	8
.j15	pr_sc_4bpl	8
.j14	pr_sc_4bpl	8
.j13	pr_sc_4bpl	8
.j12	pr_sc_4bpl	8
.j11	pr_sc_4bpl	8
.j10	pr_sc_4bpl	8
.j9	pr_sc_4bpl	8
.j8	pr_sc_4bpl	8
.j7	pr_sc_4bpl	8
.j6	pr_sc_4bpl	8
.j5	pr_sc_4bpl	8
.j4	pr_sc_4bpl	8
.j3	pr_sc_4bpl	8
.j2	pr_sc_4bpl	8
.j1	pr_sc_4bpl	8
.j0
	add.w		d1,a3
	add.w		d1,a4

.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts


*-------------------------------------------------------*
_PR_shift_speccolumn_7pix:
*-------------------------------------------------------*
	move.w		#$01FF,d5
	move.l		#$007F007F,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	pr_sc_4bpl	7
.j15	pr_sc_4bpl	7
.j14	pr_sc_4bpl	7
.j13	pr_sc_4bpl	7
.j12	pr_sc_4bpl	7
.j11	pr_sc_4bpl	7
.j10	pr_sc_4bpl	7
.j9	pr_sc_4bpl	7
.j8	pr_sc_4bpl	7
.j7	pr_sc_4bpl	7
.j6	pr_sc_4bpl	7
.j5	pr_sc_4bpl	7
.j4	pr_sc_4bpl	7
.j3	pr_sc_4bpl	7
.j2	pr_sc_4bpl	7
.j1	pr_sc_4bpl	7
.j0
	add.w		d1,a3
	add.w		d1,a4
	
.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts

*-------------------------------------------------------*
_PR_shift_speccolumn_6pix:
*-------------------------------------------------------*
	move.w		#$03FF,d5
	move.l		#$003F003F,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	pr_sc_4bpl	6
.j15	pr_sc_4bpl	6
.j14	pr_sc_4bpl	6
.j13	pr_sc_4bpl	6
.j12	pr_sc_4bpl	6
.j11	pr_sc_4bpl	6
.j10	pr_sc_4bpl	6
.j9	pr_sc_4bpl	6
.j8	pr_sc_4bpl	6
.j7	pr_sc_4bpl	6
.j6	pr_sc_4bpl	6
.j5	pr_sc_4bpl	6
.j4	pr_sc_4bpl	6
.j3	pr_sc_4bpl	6
.j2	pr_sc_4bpl	6
.j1	pr_sc_4bpl	6
.j0
	add.w		d1,a3
	add.w		d1,a4

.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts

*-------------------------------------------------------*
_PR_shift_speccolumn_5pix:
*-------------------------------------------------------*
	move.w		#$07FF,d5
	move.l		#$001F001F,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	pr_sc_4bpl	5
.j15	pr_sc_4bpl	5
.j14	pr_sc_4bpl	5
.j13	pr_sc_4bpl	5
.j12	pr_sc_4bpl	5
.j11	pr_sc_4bpl	5
.j10	pr_sc_4bpl	5
.j9	pr_sc_4bpl	5
.j8	pr_sc_4bpl	5
.j7	pr_sc_4bpl	5
.j6	pr_sc_4bpl	5
.j5	pr_sc_4bpl	5
.j4	pr_sc_4bpl	5
.j3	pr_sc_4bpl	5
.j2	pr_sc_4bpl	5
.j1	pr_sc_4bpl	5
.j0
	add.w		d1,a3
	add.w		d1,a4

.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts

*-------------------------------------------------------*
_PR_shift_speccolumn_4pix:
*-------------------------------------------------------*
	move.w		#$0FFF,d5
	move.l		#$000F000F,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	pr_sc_4bpl	4
.j15	pr_sc_4bpl	4
.j14	pr_sc_4bpl	4
.j13	pr_sc_4bpl	4
.j12	pr_sc_4bpl	4
.j11	pr_sc_4bpl	4
.j10	pr_sc_4bpl	4
.j9	pr_sc_4bpl	4
.j8	pr_sc_4bpl	4
.j7	pr_sc_4bpl	4
.j6	pr_sc_4bpl	4
.j5	pr_sc_4bpl	4
.j4	pr_sc_4bpl	4
.j3	pr_sc_4bpl	4
.j2	pr_sc_4bpl	4
.j1	pr_sc_4bpl	4
.j0
	add.w		d1,a3
	add.w		d1,a4
	
.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts


*-------------------------------------------------------*
_PR_shift_speccolumn_3pix:
*-------------------------------------------------------*
	move.w		#$1FFF,d5
	move.l		#$00070007,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	pr_sc_4bpl	3
.j15	pr_sc_4bpl	3
.j14	pr_sc_4bpl	3
.j13	pr_sc_4bpl	3
.j12	pr_sc_4bpl	3
.j11	pr_sc_4bpl	3
.j10	pr_sc_4bpl	3
.j9	pr_sc_4bpl	3
.j8	pr_sc_4bpl	3
.j7	pr_sc_4bpl	3
.j6	pr_sc_4bpl	3
.j5	pr_sc_4bpl	3
.j4	pr_sc_4bpl	3
.j3	pr_sc_4bpl	3
.j2	pr_sc_4bpl	3
.j1	pr_sc_4bpl	3
.j0
	add.w		d1,a3
	add.w		d1,a4
	
.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts

*-------------------------------------------------------*
_PR_shift_speccolumn_2pix:
*-------------------------------------------------------*
	move.w		#$3FFF,d5
	move.l		#$00030003,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	pr_sc_4bpl	2
.j15	pr_sc_4bpl	2
.j14	pr_sc_4bpl	2
.j13	pr_sc_4bpl	2
.j12	pr_sc_4bpl	2
.j11	pr_sc_4bpl	2
.j10	pr_sc_4bpl	2
.j9	pr_sc_4bpl	2
.j8	pr_sc_4bpl	2
.j7	pr_sc_4bpl	2
.j6	pr_sc_4bpl	2
.j5	pr_sc_4bpl	2
.j4	pr_sc_4bpl	2
.j3	pr_sc_4bpl	2
.j2	pr_sc_4bpl	2
.j1	pr_sc_4bpl	2
.j0
	add.w		d1,a3
	add.w		d1,a4

.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts

*-------------------------------------------------------*
_PR_shift_speccolumn_1pix:
*-------------------------------------------------------*
	move.w		#$7FFF,d5
	move.l		#$00010001,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	pr_sc_4bpl	1
.j15	pr_sc_4bpl	1
.j14	pr_sc_4bpl	1
.j13	pr_sc_4bpl	1
.j12	pr_sc_4bpl	1
.j11	pr_sc_4bpl	1
.j10	pr_sc_4bpl	1
.j9	pr_sc_4bpl	1
.j8	pr_sc_4bpl	1
.j7	pr_sc_4bpl	1
.j6	pr_sc_4bpl	1
.j5	pr_sc_4bpl	1
.j4	pr_sc_4bpl	1
.j3	pr_sc_4bpl	1
.j2	pr_sc_4bpl	1
.j1	pr_sc_4bpl	1
.j0
	add.w		d1,a3
	add.w		d1,a4
	
.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts

*-------------------------------------------------------*
_PR_shift_speccolumn_0pix:
*-------------------------------------------------------*
;	shiftmask is hardcoded

	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;;	moveq		#0,d0
;;	move.w		(a0),d0				; R tile idx
;;	addq.l		#4,a0
;;	lsl.l		#7,d0
;	move.l		4(a0),d0
;	add.l		d7,d0
;	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	pr_sc_4bpl	0
.j15	pr_sc_4bpl	0
.j14	pr_sc_4bpl	0
.j13	pr_sc_4bpl	0
.j12	pr_sc_4bpl	0
.j11	pr_sc_4bpl	0
.j10	pr_sc_4bpl	0
.j9	pr_sc_4bpl	0
.j8	pr_sc_4bpl	0
.j7	pr_sc_4bpl	0
.j6	pr_sc_4bpl	0
.j5	pr_sc_4bpl	0
.j4	pr_sc_4bpl	0
.j3	pr_sc_4bpl	0
.j2	pr_sc_4bpl	0
.j1	pr_sc_4bpl	0
.j0
	add.w		d1,a3
	add.w		d1,a4
	
.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts




*-------------------------------------------------------*

p_sc_hshift		macro
	move.l		(a1)+,d0
	and.l		d5,d0
	swap		d0
	ror.l		#16-\1,d0
	move.l		(a5)+,d3
	lsr.l		#16-\1,d3
	and.w		d6,d3
	or.l		d3,d0
	move.l		d0,(a4)+
	endm

p_sc_lshift		macro
	move.l		(a1)+,d0
	and.w		d5,d0
	lsl.l		#\1,d0
	move.l		(a5)+,d3
	swap		d3
	rol.l		#\1,d3
	and.l		d6,d3
	or.l		d3,d0
	move.l		d0,(a4)+
	endm

p_sc_4bpl		macro
	ifeq		(\1-0)

	; shift=0
	move.l		(a1)+,(a4)+
	move.l		(a1)+,(a4)+
		
	else
	
	ifeq		(\1-8)

	; shift=8
	movep.l		1(a1),d0
	movep.l		(a5),d3
	movep.l		d0,(a4)				; write P<-[L:R] 
	movep.l		d3,1(a4)
	addq.l		#8,a1
	addq.l		#8,a5
			
	else

	; shift=?
	rept		2
	ifge		(\1-8)
	p_sc_hshift	\1				; 8-15
	else
	p_sc_lshift	\1				; 0-7	
	endc
	endr

	endc
	endc

	lea		160-8(a4),a4
	
	endm

*-------------------------------------------------------*
_P_shift_speccolumn_Npix:
*-------------------------------------------------------*

;	move.w		#160,d1
;	mulu.w		d3,d1
;	neg.w		d1
;	addq.w		#8,d1	
	
	moveq		#16-1,d0
	and.w		_g_xpreshift,d0
	lsl.w		#2,d0
	jmp		.tab(pc,d0.w)
.tab:	bra.w		_P_shift_speccolumn_0pix
	bra.w		_P_shift_speccolumn_1pix
	bra.w		_P_shift_speccolumn_2pix
	bra.w		_P_shift_speccolumn_3pix
	bra.w		_P_shift_speccolumn_4pix
	bra.w		_P_shift_speccolumn_5pix
	bra.w		_P_shift_speccolumn_6pix
	bra.w		_P_shift_speccolumn_7pix
	bra.w		_P_shift_speccolumn_8pix
	bra.w		_P_shift_speccolumn_9pix
	bra.w		_P_shift_speccolumn_10pix
	bra.w		_P_shift_speccolumn_11pix
	bra.w		_P_shift_speccolumn_12pix
	bra.w		_P_shift_speccolumn_13pix
	bra.w		_P_shift_speccolumn_14pix
	bra.w		_P_shift_speccolumn_15pix

*-------------------------------------------------------*
_P_shift_speccolumn_15pix:
*-------------------------------------------------------*
	move.l		#$00010001,d5
	move.w		#$7FFF,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)
	
	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	p_sc_4bpl	15
.j15	p_sc_4bpl	15
.j14	p_sc_4bpl	15
.j13	p_sc_4bpl	15
.j12	p_sc_4bpl	15
.j11	p_sc_4bpl	15
.j10	p_sc_4bpl	15
.j9	p_sc_4bpl	15
.j8	p_sc_4bpl	15
.j7	p_sc_4bpl	15
.j6	p_sc_4bpl	15
.j5	p_sc_4bpl	15
.j4	p_sc_4bpl	15
.j3	p_sc_4bpl	15
.j2	p_sc_4bpl	15
.j1	p_sc_4bpl	15
.j0
	add.w		d1,a4

.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts

*-------------------------------------------------------*
_P_shift_speccolumn_14pix:
*-------------------------------------------------------*
	move.l		#$00030003,d5
	move.w		#$3FFF,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)
	
	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	p_sc_4bpl	14
.j15	p_sc_4bpl	14
.j14	p_sc_4bpl	14
.j13	p_sc_4bpl	14
.j12	p_sc_4bpl	14
.j11	p_sc_4bpl	14
.j10	p_sc_4bpl	14
.j9	p_sc_4bpl	14
.j8	p_sc_4bpl	14
.j7	p_sc_4bpl	14
.j6	p_sc_4bpl	14
.j5	p_sc_4bpl	14
.j4	p_sc_4bpl	14
.j3	p_sc_4bpl	14
.j2	p_sc_4bpl	14
.j1	p_sc_4bpl	14
.j0
	add.w		d1,a4
	
.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts

*-------------------------------------------------------*
_P_shift_speccolumn_13pix:
*-------------------------------------------------------*
	move.l		#$00070007,d5
	move.w		#$1FFF,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	p_sc_4bpl	13
.j15	p_sc_4bpl	13
.j14	p_sc_4bpl	13
.j13	p_sc_4bpl	13
.j12	p_sc_4bpl	13
.j11	p_sc_4bpl	13
.j10	p_sc_4bpl	13
.j9	p_sc_4bpl	13
.j8	p_sc_4bpl	13
.j7	p_sc_4bpl	13
.j6	p_sc_4bpl	13
.j5	p_sc_4bpl	13
.j4	p_sc_4bpl	13
.j3	p_sc_4bpl	13
.j2	p_sc_4bpl	13
.j1	p_sc_4bpl	13
.j0
	add.w		d1,a4
	
.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts
	
*-------------------------------------------------------*
_P_shift_speccolumn_12pix:
*-------------------------------------------------------*
	move.l		#$000F000F,d5
	move.w		#$0FFF,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)
	
	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	p_sc_4bpl	12
.j15	p_sc_4bpl	12
.j14	p_sc_4bpl	12
.j13	p_sc_4bpl	12
.j12	p_sc_4bpl	12
.j11	p_sc_4bpl	12
.j10	p_sc_4bpl	12
.j9	p_sc_4bpl	12
.j8	p_sc_4bpl	12
.j7	p_sc_4bpl	12
.j6	p_sc_4bpl	12
.j5	p_sc_4bpl	12
.j4	p_sc_4bpl	12
.j3	p_sc_4bpl	12
.j2	p_sc_4bpl	12
.j1	p_sc_4bpl	12
.j0
	add.w		d1,a4
	
.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts
	
*-------------------------------------------------------*
_P_shift_speccolumn_11pix:
*-------------------------------------------------------*
	move.l		#$001F001F,d5
	move.w		#$07FF,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)
	
	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	p_sc_4bpl	11
.j15	p_sc_4bpl	11
.j14	p_sc_4bpl	11
.j13	p_sc_4bpl	11
.j12	p_sc_4bpl	11
.j11	p_sc_4bpl	11
.j10	p_sc_4bpl	11
.j9	p_sc_4bpl	11
.j8	p_sc_4bpl	11
.j7	p_sc_4bpl	11
.j6	p_sc_4bpl	11
.j5	p_sc_4bpl	11
.j4	p_sc_4bpl	11
.j3	p_sc_4bpl	11
.j2	p_sc_4bpl	11
.j1	p_sc_4bpl	11
.j0
	add.w		d1,a4

.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts
	
*-------------------------------------------------------*
_P_shift_speccolumn_10pix:
*-------------------------------------------------------*
	move.l		#$003F003F,d5
	move.w		#$03FF,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)
	
	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	p_sc_4bpl	10
.j15	p_sc_4bpl	10
.j14	p_sc_4bpl	10
.j13	p_sc_4bpl	10
.j12	p_sc_4bpl	10
.j11	p_sc_4bpl	10
.j10	p_sc_4bpl	10
.j9	p_sc_4bpl	10
.j8	p_sc_4bpl	10
.j7	p_sc_4bpl	10
.j6	p_sc_4bpl	10
.j5	p_sc_4bpl	10
.j4	p_sc_4bpl	10
.j3	p_sc_4bpl	10
.j2	p_sc_4bpl	10
.j1	p_sc_4bpl	10
.j0
	add.w		d1,a4
	
.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts

*-------------------------------------------------------*
_P_shift_speccolumn_9pix:
*-------------------------------------------------------*
	move.l		#$007F007F,d5
	move.w		#$01FF,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	p_sc_4bpl	9
.j15	p_sc_4bpl	9
.j14	p_sc_4bpl	9
.j13	p_sc_4bpl	9
.j12	p_sc_4bpl	9
.j11	p_sc_4bpl	9
.j10	p_sc_4bpl	9
.j9	p_sc_4bpl	9
.j8	p_sc_4bpl	9
.j7	p_sc_4bpl	9
.j6	p_sc_4bpl	9
.j5	p_sc_4bpl	9
.j4	p_sc_4bpl	9
.j3	p_sc_4bpl	9
.j2	p_sc_4bpl	9
.j1	p_sc_4bpl	9
.j0
	add.w		d1,a4

.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts

*-------------------------------------------------------*
_P_shift_speccolumn_8pix:
*-------------------------------------------------------*
; 	shiftmask is hardcoded
	
	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	p_sc_4bpl	8
.j15	p_sc_4bpl	8
.j14	p_sc_4bpl	8
.j13	p_sc_4bpl	8
.j12	p_sc_4bpl	8
.j11	p_sc_4bpl	8
.j10	p_sc_4bpl	8
.j9	p_sc_4bpl	8
.j8	p_sc_4bpl	8
.j7	p_sc_4bpl	8
.j6	p_sc_4bpl	8
.j5	p_sc_4bpl	8
.j4	p_sc_4bpl	8
.j3	p_sc_4bpl	8
.j2	p_sc_4bpl	8
.j1	p_sc_4bpl	8
.j0
	add.w		d1,a4

.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts


*-------------------------------------------------------*
_P_shift_speccolumn_7pix:
*-------------------------------------------------------*
	move.w		#$01FF,d5
	move.l		#$007F007F,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	p_sc_4bpl	7
.j15	p_sc_4bpl	7
.j14	p_sc_4bpl	7
.j13	p_sc_4bpl	7
.j12	p_sc_4bpl	7
.j11	p_sc_4bpl	7
.j10	p_sc_4bpl	7
.j9	p_sc_4bpl	7
.j8	p_sc_4bpl	7
.j7	p_sc_4bpl	7
.j6	p_sc_4bpl	7
.j5	p_sc_4bpl	7
.j4	p_sc_4bpl	7
.j3	p_sc_4bpl	7
.j2	p_sc_4bpl	7
.j1	p_sc_4bpl	7
.j0
	add.w		d1,a4
	
.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts

*-------------------------------------------------------*
_P_shift_speccolumn_6pix:
*-------------------------------------------------------*
	move.w		#$03FF,d5
	move.l		#$003F003F,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	p_sc_4bpl	6
.j15	p_sc_4bpl	6
.j14	p_sc_4bpl	6
.j13	p_sc_4bpl	6
.j12	p_sc_4bpl	6
.j11	p_sc_4bpl	6
.j10	p_sc_4bpl	6
.j9	p_sc_4bpl	6
.j8	p_sc_4bpl	6
.j7	p_sc_4bpl	6
.j6	p_sc_4bpl	6
.j5	p_sc_4bpl	6
.j4	p_sc_4bpl	6
.j3	p_sc_4bpl	6
.j2	p_sc_4bpl	6
.j1	p_sc_4bpl	6
.j0
	add.w		d1,a4

.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts

*-------------------------------------------------------*
_P_shift_speccolumn_5pix:
*-------------------------------------------------------*
	move.w		#$07FF,d5
	move.l		#$001F001F,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	p_sc_4bpl	5
.j15	p_sc_4bpl	5
.j14	p_sc_4bpl	5
.j13	p_sc_4bpl	5
.j12	p_sc_4bpl	5
.j11	p_sc_4bpl	5
.j10	p_sc_4bpl	5
.j9	p_sc_4bpl	5
.j8	p_sc_4bpl	5
.j7	p_sc_4bpl	5
.j6	p_sc_4bpl	5
.j5	p_sc_4bpl	5
.j4	p_sc_4bpl	5
.j3	p_sc_4bpl	5
.j2	p_sc_4bpl	5
.j1	p_sc_4bpl	5
.j0
	add.w		d1,a4

.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts

*-------------------------------------------------------*
_P_shift_speccolumn_4pix:
*-------------------------------------------------------*
	move.w		#$0FFF,d5
	move.l		#$000F000F,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	p_sc_4bpl	4
.j15	p_sc_4bpl	4
.j14	p_sc_4bpl	4
.j13	p_sc_4bpl	4
.j12	p_sc_4bpl	4
.j11	p_sc_4bpl	4
.j10	p_sc_4bpl	4
.j9	p_sc_4bpl	4
.j8	p_sc_4bpl	4
.j7	p_sc_4bpl	4
.j6	p_sc_4bpl	4
.j5	p_sc_4bpl	4
.j4	p_sc_4bpl	4
.j3	p_sc_4bpl	4
.j2	p_sc_4bpl	4
.j1	p_sc_4bpl	4
.j0
	add.w		d1,a4
	
.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts


*-------------------------------------------------------*
_P_shift_speccolumn_3pix:
*-------------------------------------------------------*
	move.w		#$1FFF,d5
	move.l		#$00070007,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	p_sc_4bpl	3
.j15	p_sc_4bpl	3
.j14	p_sc_4bpl	3
.j13	p_sc_4bpl	3
.j12	p_sc_4bpl	3
.j11	p_sc_4bpl	3
.j10	p_sc_4bpl	3
.j9	p_sc_4bpl	3
.j8	p_sc_4bpl	3
.j7	p_sc_4bpl	3
.j6	p_sc_4bpl	3
.j5	p_sc_4bpl	3
.j4	p_sc_4bpl	3
.j3	p_sc_4bpl	3
.j2	p_sc_4bpl	3
.j1	p_sc_4bpl	3
.j0
	add.w		d1,a4
	
.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts

*-------------------------------------------------------*
_P_shift_speccolumn_2pix:
*-------------------------------------------------------*
	move.w		#$3FFF,d5
	move.l		#$00030003,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	p_sc_4bpl	2
.j15	p_sc_4bpl	2
.j14	p_sc_4bpl	2
.j13	p_sc_4bpl	2
.j12	p_sc_4bpl	2
.j11	p_sc_4bpl	2
.j10	p_sc_4bpl	2
.j9	p_sc_4bpl	2
.j8	p_sc_4bpl	2
.j7	p_sc_4bpl	2
.j6	p_sc_4bpl	2
.j5	p_sc_4bpl	2
.j4	p_sc_4bpl	2
.j3	p_sc_4bpl	2
.j2	p_sc_4bpl	2
.j1	p_sc_4bpl	2
.j0
	add.w		d1,a4

.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts

*-------------------------------------------------------*
_P_shift_speccolumn_1pix:
*-------------------------------------------------------*
	move.w		#$7FFF,d5
	move.l		#$00010001,d6

	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		(a0),d0				; R tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	p_sc_4bpl	1
.j15	p_sc_4bpl	1
.j14	p_sc_4bpl	1
.j13	p_sc_4bpl	1
.j12	p_sc_4bpl	1
.j11	p_sc_4bpl	1
.j10	p_sc_4bpl	1
.j9	p_sc_4bpl	1
.j8	p_sc_4bpl	1
.j7	p_sc_4bpl	1
.j6	p_sc_4bpl	1
.j5	p_sc_4bpl	1
.j4	p_sc_4bpl	1
.j3	p_sc_4bpl	1
.j2	p_sc_4bpl	1
.j1	p_sc_4bpl	1
.j0
	add.w		d1,a4
	
.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts

*-------------------------------------------------------*
_P_shift_speccolumn_0pix:
*-------------------------------------------------------*
;	shiftmask is hardcoded

	move.l		a5,-(sp)
	move.l		a6,-(sp)

	lsl.w		#2,d3
	lea		.jt(pc,d3.w),a6

	bra		.syl
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	addq.l		#4,a0
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;;	moveq		#0,d0
;;	move.w		(a0),d0				; R tile idx
;;	addq.l		#4,a0
;;	lsl.l		#7,d0
;	move.l		4(a0),d0
;	add.l		d7,d0
;	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos

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
	
.j16	p_sc_4bpl	0
.j15	p_sc_4bpl	0
.j14	p_sc_4bpl	0
.j13	p_sc_4bpl	0
.j12	p_sc_4bpl	0
.j11	p_sc_4bpl	0
.j10	p_sc_4bpl	0
.j9	p_sc_4bpl	0
.j8	p_sc_4bpl	0
.j7	p_sc_4bpl	0
.j6	p_sc_4bpl	0
.j5	p_sc_4bpl	0
.j4	p_sc_4bpl	0
.j3	p_sc_4bpl	0
.j2	p_sc_4bpl	0
.j1	p_sc_4bpl	0
.j0
	add.w		d1,a4
	
.syl:	dbra		d4,.yl

	move.l		(sp)+,a6
	move.l		(sp)+,a5
	rts
