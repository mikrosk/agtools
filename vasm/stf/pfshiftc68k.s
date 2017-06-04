*-------------------------------------------------------*

	XDEF		_PR_shift_column_Npix
	XDEF		_P_shift_column_Npix


*-------------------------------------------------------*
_PR_shift_column_Npix:
*-------------------------------------------------------*
	moveq		#16-1,d0
	and.w		_g_xpreshift,d0
	add.w		d0,d0
	add.w		d0,d0
	jmp		.tab(pc,d0.w)
.tab:	bra.w		_PR_shift_column_0pix
	bra.w		_PR_shift_column_1pix
	bra.w		_PR_shift_column_2pix
	bra.w		_PR_shift_column_3pix
	bra.w		_PR_shift_column_4pix
	bra.w		_PR_shift_column_5pix
	bra.w		_PR_shift_column_6pix
	bra.w		_PR_shift_column_7pix
	bra.w		_PR_shift_column_8pix
	bra.w		_PR_shift_column_9pix
	bra.w		_PR_shift_column_10pix
	bra.w		_PR_shift_column_11pix
	bra.w		_PR_shift_column_12pix
	bra.w		_PR_shift_column_13pix
	bra.w		_PR_shift_column_14pix
	bra.w		_PR_shift_column_15pix

*-------------------------------------------------------*
_PR_shift_column_15pix:
*-------------------------------------------------------*
	move.l		#$00010001,d5
	move.w		#$7FFF,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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

	add.w		d1,a4
	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts
	
*-------------------------------------------------------*
_PR_shift_column_14pix:
*-------------------------------------------------------*
	move.l		#$00030003,d5
	move.w		#$3FFF,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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

	add.w		d1,a4
	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts
	
*-------------------------------------------------------*
_PR_shift_column_13pix:
*-------------------------------------------------------*
	move.l		#$00070007,d5
	move.w		#$1FFF,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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

	add.w		d1,a4
	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts
		
*-------------------------------------------------------*
_PR_shift_column_12pix:
*-------------------------------------------------------*
	move.l		#$000F000F,d5
	move.w		#$0FFF,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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

	add.w		d1,a4
	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts
	
*-------------------------------------------------------*
_PR_shift_column_11pix:
*-------------------------------------------------------*
	move.l		#$001F001F,d5
	move.w		#$07FF,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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

	add.w		d1,a4
	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts
		
*-------------------------------------------------------*
_PR_shift_column_10pix:
*-------------------------------------------------------*
	move.l		#$003F003F,d5
	move.w		#$03FF,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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

	add.w		d1,a4
	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts
	
*-------------------------------------------------------*
_PR_shift_column_9pix:
*-------------------------------------------------------*
	move.l		#$007F007F,d5
	move.w		#$01FF,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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

	add.w		d1,a4
	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts	
	
*-------------------------------------------------------*
_PR_shift_column_8pix:
*-------------------------------------------------------*
;	move.l		a6,-(sp)
;	lea		160-8.w,a6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

	movep.l		1(a1),d0
	movep.l		(a5),d3
	movep.l		d0,(a4)				; write P<-[L:R] 
	movep.l		d3,1(a4)
	move.l		(a4)+,(a3)+			; write Q<-[L:R] 
	move.l		(a4)+,(a3)+	
	addq.l		#8,a1
	addq.l		#8,a5
	
	add.w		d1,a4
	add.w		d1,a3
;	add.l		a6,a4
;	add.l		a6,a3
	
	endr

.syl:	dbra		d4,.yl
;	move.l		(sp)+,a6
	rts


*-------------------------------------------------------*
_PR_shift_column_7pix:
*-------------------------------------------------------*
	move.w		#$01FF,d5
	move.l		#$007F007F,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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

	add.w		d1,a4
	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts
	
*-------------------------------------------------------*
_PR_shift_column_6pix:
*-------------------------------------------------------*
	move.w		#$03FF,d5
	move.l		#$003F003F,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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

	add.w		d1,a4
	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts


*-------------------------------------------------------*
_PR_shift_column_5pix:
*-------------------------------------------------------*
	move.w		#$07FF,d5
	move.l		#$001F001F,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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

	add.w		d1,a4
	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts
	
*-------------------------------------------------------*
_PR_shift_column_4pix:
*-------------------------------------------------------*
	move.w		#$0FFF,d5
	move.l		#$000F000F,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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

	add.w		d1,a4
	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts
	

*-------------------------------------------------------*
_PR_shift_column_3pix:
*-------------------------------------------------------*
	move.w		#$1FFF,d5
	move.l		#$00070007,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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

	add.w		d1,a4
	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts
	
*-------------------------------------------------------*
_PR_shift_column_2pix:
*-------------------------------------------------------*
	move.w		#$3FFF,d5
	move.l		#$00030003,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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

	add.w		d1,a4
	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts	


*-------------------------------------------------------*
_PR_shift_column_1pix:
*-------------------------------------------------------*
	move.w		#$7FFF,d5
	move.l		#$00010001,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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

	add.w		d1,a4
	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts
	
*-------------------------------------------------------*
_PR_shift_column_0pix:
*-------------------------------------------------------*
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

	move.l		(a1)+,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
	move.l		(a1)+,d0
	move.l		d0,(a4)+
	move.l		d0,(a3)+
	
	add.w		d1,a4
	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl
	rts


*-------------------------------------------------------*
_P_shift_column_Npix:
*-------------------------------------------------------*
	moveq		#16-1,d0
	and.w		_g_xpreshift,d0
	add.w		d0,d0
	add.w		d0,d0
	jmp		.tab(pc,d0.w)
.tab:	bra.w		_P_shift_column_0pix
	bra.w		_P_shift_column_1pix
	bra.w		_P_shift_column_2pix
	bra.w		_P_shift_column_3pix
	bra.w		_P_shift_column_4pix
	bra.w		_P_shift_column_5pix
	bra.w		_P_shift_column_6pix
	bra.w		_P_shift_column_7pix
	bra.w		_P_shift_column_8pix
	bra.w		_P_shift_column_9pix
	bra.w		_P_shift_column_10pix
	bra.w		_P_shift_column_11pix
	bra.w		_P_shift_column_12pix
	bra.w		_P_shift_column_13pix
	bra.w		_P_shift_column_14pix
	bra.w		_P_shift_column_15pix
	
*-------------------------------------------------------*
_P_shift_column_15pix:
*-------------------------------------------------------*
	move.l		#$00010001,d5
	move.w		#$7FFF,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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
;	move.l		d0,(a3)+
	
	endr

	add.w		d1,a4
;	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts

*-------------------------------------------------------*
_P_shift_column_14pix:
*-------------------------------------------------------*
	move.l		#$00030003,d5
	move.w		#$3FFF,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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
;	move.l		d0,(a3)+
	
	endr

	add.w		d1,a4
;	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts
	
*-------------------------------------------------------*
_P_shift_column_13pix:
*-------------------------------------------------------*
	move.l		#$00070007,d5
	move.w		#$1FFF,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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
;	move.l		d0,(a3)+
	
	endr

	add.w		d1,a4
;	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts

*-------------------------------------------------------*
_P_shift_column_12pix:
*-------------------------------------------------------*
	move.l		#$000F000F,d5
	move.w		#$0FFF,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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
	
	endr

	add.w		d1,a4
;	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts
	
*-------------------------------------------------------*
_P_shift_column_11pix:
*-------------------------------------------------------*
	move.l		#$001F001F,d5
	move.w		#$07FF,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

	rept		2
	
	move.l		(a1)+,d0
;		and.w		d5,d0
;		lsl.l		#8,d0
;		lsl.l		#2,d0
	and.l		d5,d0
	swap		d0
	ror.l		#16-11,d0
	move.l		(a5)+,d3
	lsr.l		#16-11,d3
	and.w		d6,d3
	or.l		d3,d0
	move.l		d0,(a4)+
	
	endr

	add.w		d1,a4
;	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts

*-------------------------------------------------------*
_P_shift_column_10pix:
*-------------------------------------------------------*
	move.l		#$003F003F,d5
	move.w		#$03FF,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

	rept		2
	
	move.l		(a1)+,d0
;		and.w		d5,d0
;		lsl.l		#8,d0
;		lsl.l		#2,d0
	and.l		d5,d0
	swap		d0
	ror.l		#16-10,d0
	move.l		(a5)+,d3
	lsr.l		#16-10,d3
	and.w		d6,d3
	or.l		d3,d0
	move.l		d0,(a4)+
	
	endr

	add.w		d1,a4
;	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts
	
*-------------------------------------------------------*
_P_shift_column_9pix:
*-------------------------------------------------------*
	move.l		#$007F007F,d5
	move.w		#$01FF,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

	rept		2
	
	move.l		(a1)+,d0
;		and.w		d5,d0
;		lsl.l		#8,d0
;		lsl.l		#2,d0
	and.l		d5,d0
	swap		d0
	ror.l		#16-9,d0
	move.l		(a5)+,d3
	lsr.l		#16-9,d3
	and.w		d6,d3
	or.l		d3,d0
	move.l		d0,(a4)+
	
	endr

	add.w		d1,a4
;	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts

*-------------------------------------------------------*
_P_shift_column_8pix:
*-------------------------------------------------------*
;	move.l		a6,-(sp)
;	lea		160-8.w,a6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

	movep.l		1(a1),d0
	movep.l		(a5),d3
	movep.l		d0,(a4)				; write P<-[L:R] 
	movep.l		d3,1(a4)
	addq.l		#8,a1
	addq.l		#8,a5
	
	lea		160(a4),a4
;	add.l		a6,a4
;	add.l		a6,a3
	
	endr

.syl:	dbra		d4,.yl
;	move.l		(sp)+,a6
	rts

*-------------------------------------------------------*
_P_shift_column_7pix:
*-------------------------------------------------------*
	move.w		#$01FF,d5
	move.l		#$007F007F,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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
	
	endr

	add.w		d1,a4
;	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts

*-------------------------------------------------------*
_P_shift_column_6pix:
*-------------------------------------------------------*
	move.w		#$03FF,d5
	move.l		#$003F003F,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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
	
	endr

	add.w		d1,a4
;	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts
	
*-------------------------------------------------------*
_P_shift_column_5pix:
*-------------------------------------------------------*
	move.w		#$07FF,d5
	move.l		#$001F001F,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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
	
	endr

	add.w		d1,a4
;	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts	

*-------------------------------------------------------*
_P_shift_column_4pix:
*-------------------------------------------------------*
	move.w		#$0FFF,d5
	move.l		#$000F000F,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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
	
	endr

	add.w		d1,a4
;	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts
	

*-------------------------------------------------------*
_P_shift_column_3pix:
*-------------------------------------------------------*
	move.w		#$1FFF,d5
	move.l		#$00070007,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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
	
	endr

	add.w		d1,a4
;	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts
	
*-------------------------------------------------------*
_P_shift_column_2pix:
*-------------------------------------------------------*
	move.w		#$3FFF,d5
	move.l		#$00030003,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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
	
	endr

	add.w		d1,a4
;	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts	

*-------------------------------------------------------*
_P_shift_column_1pix:
*-------------------------------------------------------*
	move.w		#$7FFF,d5
	move.l		#$00010001,d6
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

;	moveq		#0,d0
;	move.w		4(a0),d0			; R tile idx
;	lsl.l		#7,d0
	move.l		4(a0),d0
	add.l		d7,d0
	move.l		d0,a5				; R tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

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
	
	endr

	add.w		d1,a4
;	add.w		d1,a3

	endr

.syl:	dbra		d4,.yl

	rts	
	
*-------------------------------------------------------*
_P_shift_column_0pix:
*-------------------------------------------------------*
	subq.w		#1,d4
.yl:
;	moveq		#0,d0
;	move.w		(a0),d0				; L tile idx
;	lsl.l		#7,d0
	move.l		(a0),d0
	add.l		d7,d0
	move.l		d0,a1				; L tile addr

	add.l		d2,a0				; advance map y pos
	
	rept		16

	move.l		(a1)+,(a4)+
	move.l		(a1)+,(a4)+	
	add.w		d1,a4
;	add.w		d1,a3
	
	endr

.syl:	dbra		d4,.yl
	rts
