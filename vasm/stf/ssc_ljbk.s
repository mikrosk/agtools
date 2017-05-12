*-----------------------------------------------------------------------------*
*	STFDisplayService:LJBK
*-----------------------------------------------------------------------------*
*	This a minor reworking of LJBK's 'beeshift'
*	experimental syncscroll routine with 4pixel
*	syncshift/hardscroll.
* 	The layout has been changed for a 192-line or
*	240-line display as per other displayservice
*	routines and to incorporate the displaygroup
*	manager.
*-----------------------------------------------------------------------------*

		text


displayservice_upper_delay	=	94+4			; padding of 3-6
timecritical_pad_ta		=	1			; TimerA padding
;syncscroll_bot_scan		=	192
;syncscroll_lower_scan_tb	=	222
displayservice_lower_delta_tb	=	222
displayservice_final_delta_tb	=	34-16
timecritical_pad_tb		=	1			; TimerB padding (in scanlines) just enough for single movem/mul/div

use_topborder			=	1

*-----------------------------------------------------------------------------*
displayservice_ljbk_vbi:
*-----------------------------------------------------------------------------*
	move.w		#$2700,sr				; lockout
*-----------------------------------------------------------------------------*

;	TimerA: schedule top border - force enable every VBL

	clr.b		tacr.w					; TimerA CTRL: halt
	bset		#5,iera.w				; TimerA IE: enable
	bset		#5,imra.w				; TimerA IM: enable
	move.b		#displayservice_upper_delay-timecritical_pad_ta,$fffffa1f.w	; TimerA DATA: delay
	move.b		#4,tacr.w				; TimerA CTRL: start

	move.l		#displayservice_ljbk_upper_a,$134.w

;	syncscroll displaystate commited in low border so sync there instead...
;	subq.w		#1,_g_vbl
		
*-----------------------------------------------------------------------------*
	rte
	
	
*-----------------------------------------------------------------------------*
displayservice_ljbk_upper_a:	
*-----------------------------------------------------------------------------*
	movem.l		d0-a6,-(sp)				; todo: reduce
*-----------------------------------------------------------------------------*

	if		(use_topborder)

	moveq		#1,d1
	lea		$ffff820a.w,a1	

	lea		$fffffa1f.w,a0
	moveq		#displayservice_upper_delay-(timecritical_pad_ta*2),d0
.tasyn:	cmp.b		(a0),d0
	blo.s		.tasyn
	
	move.w		#$2100,sr				; enable HBL
	stop		#$2100					; wait next HBL
	move.w		#$2700,sr				; lockout
	clr.b		tacr.w					; TimerA CTRL: halt

	dcb.w 		84,$4e71				; ...wait...

;	open top border 

	clr.b		$ffff820a.w				; 60 Hz
	dcb.w 		9,$4e71					; ...wait...
	move.b		#2,$ffff820a.w				; 50 Hz

	else

	move.w		#$2700,sr				; lockout
	clr.b		tacr.w					; TimerA CTRL: halt
	
	endc

;	syncscroll event

	bsr		displayservice_ljbk_4pix_impl
;	subq.w		#1,_g_vbl

;	TimerB: schedule final scanline

	clr.b		tbcr.w					; TimerB CTRL: halt
	bset.b		#0,iera.w				; TimerB IE: enable
	bset.b		#0,imra.w				; TimerB IM: enable
;	move.b		#syncscroll_lower_scan_tb,$fffffa21.w	; TimerB DATA: count
	move.b		#displayservice_lower_delta_tb-timecritical_pad_tb,tbdr.w	; TimerB DATA: count
	move.b		#8,tbcr.w				; TimerB CTRL: start))

	move.l		#displayservice_ljbk_lower_b,$120.w

	move.l		stfm_palette,a0

	movem.l		(a0),d0-d7
	movem.l		d0-d7,$ffff8240.w
	ifd		TIMING_RASTERS
	move.w		$64.w,$ffff8240.w
	endc

*-----------------------------------------------------------------------------*
	movem.l		(sp)+,d0-a6

	ifeq		(use_auto_end_of_interrupt)
;	manual EOI
	bclr.b		#5,isra.w
	endc
	
drte:	rte


*-----------------------------------------------------------------------------*
displayservice_ljbk_lower_b:
*-----------------------------------------------------------------------------*
;	clr.b		$ffff820a.w			; 60 Hz
;	move.w		#$2700,sr
;	dcb.w		4,$4e71				; ...wait...
;	move.b		#2,$ffff820a.w			; 50 Hz

	move.w		#$2700,sr
*-----------------------------------------------------------------------------*
	movem.l		d0/a0,-(sp)

;	insert 'padding' scans to absorb short blitter jobs - 

	moveq		#displayservice_lower_delta_tb-(timecritical_pad_tb*2),d0
	lea		tbdr.w,a0
.wl:	cmp.b		(a0),d0
	blo.s		.wl

	nop		; pad until lower border event
	nop
	nop
	nop
	nop
	nop
	nop
	nop

	nop		; HW: almost stable
	nop		; HW: stable
;	nop		; HW: almost stable

	clr.b		$ffff820a.w			; 60 Hz
	dcb.w		5,$4e71				; ...wait...
	move.b		#2,$ffff820a.w			; 50 Hz


	clr.b		tbcr.w					; TimerB CTRL: halt
;	bset.b		#0,iera.w				; TimerB IE: enable
;	bset.b		#0,imra.w				; TimerB IM: enable
	move.b		#displayservice_final_delta_tb,tbdr.w	; TimerB DATA: count
	move.b		#8,tbcr.w				; TimerB CTRL: start))

	move.l		#displayservice_ljbk_final_b,$120.w


	movem.l		(sp)+,d0/a0

	addq.l		#1,$462.w	
	addq.l		#1,$466.w
;	subq.w		#1,_g_vbl

	bclr.b		#0,isra.w

	rte
	

dummy:	rte


*-----------------------------------------------------------------------------*
displayservice_ljbk_final_b:
*-----------------------------------------------------------------------------*
	move.l		d0,-(sp)
	move.l		a0,-(sp)
*-----------------------------------------------------------------------------*

;	wipe out all colours - black lower border

	moveq		#0,d0
	lea		$ffff8240.w,a0

.wt:	cmp.b		#displayservice_final_delta_tb,tbdr.w
	beq.s		.wt
	
	ifd		TIMING_RASTERS
	move.w		$64.w,(a0)+
	move.w		d0,(a0)+
	else
	move.l		d0,(a0)+
	endc

	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+

;	no more scanline scheduling this frame - stop TimerB

	move.b		d0,tbcr.w

*-----------------------------------------------------------------------------*

;	not.w		$ffff8240.w
	movem.l		d0-a6,-(sp)

;	move.l		stfm_palette,a0
;	movem.l		(a0),d0-d7
;	movem.l		d0-d7,$ffff8240.w

	move.l		_gp_active_group,d0
	beq.s		.skip
	move.l		d0,a0
	moveq		#1,d0
	and.w		_g_vbl,d0
	and.w		_g_fieldmask,d0
	add.w		d0,d0
	add.w		d0,d0
	move.l		(a0,d0.w),a6
	move.l		(a6),d0
	
	moveq		#$c,d1
	and.b		d0,d1
	lsr.w		d1
	move.w		d1,syncshift
	
	lsr.l		#8,d0	
	move.l		d0,sscrl_offset

;	moveq		#$c,d0
;	and.b		3(a6),d0
;	lsr.w		d0
;	move.w		d0,syncshift

;	addq.l		#6,a6
	
	moveq		#1,d0
	and.w		_g_vbl,d0
	lsl.w		#5,d0
;	add.w		d0,a6
	lea		6(a6,d0.w),a6
	move.l		a6,stfm_palette

;	move.l		sscrl_offset,d0
;	lsr.w		#8,d0
;	move.l		d0,$ffff8200.w			;Set screen

;	movem.l		(a6),d0-d7
;	movem.l		d0-d7,$ffff8240.w

	bsr		displayservice_ljbk_4pix_setup

.skip:

;	movem.l		6(a6),d0-d7
;	movem.l		d0-d7,$ffff8240.w



	movem.l		(sp)+,d0-a6
	
	ifeq		(use_auto_end_of_interrupt)
;	manual EOI
	bclr.b		#0,isra.w
	endc	

	subq.w		#1,_g_vbl
	
	move.l		(sp)+,a0
	move.l		(sp)+,d0
	
	rte


*-----------------------------------------------------------------------------*
displayservice_ljbk_4pix_setup:
*-----------------------------------------------------------------------------*
	move.l		sscrl_offset,d0
	
;	add.l		#160*7,d0
	
	moveq		#0,d1
	subq.b		#1,d1
	and.l		d0,d1
	clr.b		d0

;	move.l		pframebuffer(pc),d0
;	move.l		scroffset(pc),d1

	moveq		#0,d2
	move.w		syncshift,d2
	add.l		d2,d1

	move.w		d1,d2
	andi.w		#$fe,d2
	add.w		d2,d2
	add.w		d2,d2
	add.w		d2,d2
	movea.l		sync1_data,a1
	adda.w		d2,a1
	lea		$ffff8201.w,a0
	lsr.l		#8,d0
	add.w		(a1)+,d0
	lsr.l		#8,d1
	add.w		d1,d0
	movep.w		d0,0(a0)
	lea		sync2_data,a2
	move.l		a1,(a2)
	rts


*-----------------------------------------------------------------------------*
displayservice_ljbk_4pix_impl:
*-----------------------------------------------------------------------------*
	lea		$ffff820a.w,a4
	lea		$ffff8260.w,a5
	moveq		#2,d0
	lea		$ffff8209.w,a0
	moveq		#$20,d2
	movea.l		sync2_data,a1
	move.w		(a1)+,d1
.wvid:	tst.b		(a0)
	beq.s		.wvid
	sub.b		(a0),d2
	lsr.w		d2,d2
	tst.b		d1
	beq.s		.sk1
	or.l		d0,d0
	or.l		d0,d0
	bra.s		.nosk1
.sk1:	move.b		d0,(a5)
	moveq		#6,d1
	nop
	move.w		d0,(a5)
.nosk1:	lea		$ffff8240.w,a0
	move.w		d1,d3
	moveq		#1,d1
	moveq		#2,d2
patch_us_target1:
	move.b		d2,(a5)
	or.l		d0,d0
	move.w		d2,(a5)
	nop
	move.w		d3,d1
	nop
	moveq		#4,d2
	move.w		(a0),(a0)
	move.l		(a0),(a0)
	move.l		(a0),(a0)
	move.l		(a0),(a0)
	move.l		(a0),(a0)
	move.l		(a0),(a0)
	subq.b		#2,d1
	bne.s		.sk2
	move.w		d0,(a4)
	move.b		d0,(a4)
	moveq		#2,d1
	bra.s		.nosk2
.sk2:	subq.w		#2,d1
	move.b		d1,(a4)
	move.b		d0,(a4)
	or.l		d0,d0
.nosk2:	or.l		d0,d0
	nop

ljbk_lp:
	nop
	move.w		#0,(a0)
	nop
	move.w		(a0),(a0)
	move.l		(a0),(a0)
	move.l		(a0),(a0)
	or.l		d0,d0
	move.b		(a1)+,d1
	bne.s		.sk3
	move.b		d0,(a5)
	nop
	nop
	move.w		d0,(a5)
	nop
	nop
	or.l		d0,d0
	or.l		d0,d0
	nop
	nop
	nop
	nop
	nop
	nop
	bra.s		g_cont0
.sk3:	nop
	subq.b		#2,d1
	beq.s		g_sk0
	subq.b		#2,d1
	nop
	nop
	or.l		d0,d0
	move.b		d1,(a4)
	nop
patch_ws_1_3_A:
	or.l		d0,d0
	move.b		d0,(a4)
	nop
	bra.s		g_cont0
g_sk0:	nop
	nop
	or.l		d0,d0
	nop
patch_ste_A:
	or.l		d0,d0
	nop
patch_ws_1_3_C:
	exg		d0,d0
	move.b		d1,(a4)
	move.b		d0,(a4)
patch_ws_1_3_E:
	nop
g_cont0:
	move.w		#0,(a0)
	or.l		d0,d0
	move.l		(a0),(a0)
	move.l		(a0),(a0)
	move.b		(a1)+,d1
	beq.s		.sk4
	or.l		d0,d0
	or.l		d0,d0
	bra.s		.nosk4
.sk4:	move.b		d0,(a5)
	moveq		#6,d1
	nop
	move.w		d0,(a5)
.nosk4:	move.w		#0,(a0)
	move.l		(a0),(a0)
	move.l		(a0),(a0)
	move.l		(a0),(a0)
	move.l		(a0),(a0)
	move.l		(a0),(a0)
	move.l		(a0),(a0)
	move.l		(a0),(a0)
	move.l		(a0),(a0)
	subq.b		#2,d1
	bne.s		.sk5
	move.w		d0,(a4)
	move.b		d0,(a4)
	moveq		#2,d1
	bra.s		.nosk5
.sk5:	subq.w		#2,d1
	move.b		d1,(a4)
	move.b		d0,(a4)
	or.l		d0,d0
.nosk5:	dbf		d2,ljbk_lp

	move.w		#0,(a0)
	nop
	move.w		(a0),(a0)
	move.l		(a0),(a0)
	move.l		(a0),(a0)
	or.l		d0,d0
	move.b		(a1)+,d1
	bne.s		.sk6
	move.b		d0,(a5)
	nop
	nop
	move.w		d0,(a5)
	nop
	nop
	or.l		d0,d0
	or.l		d0,d0
	nop
	nop
	nop
	nop
	nop
	nop
	bra.s		g_cont7
.sk6:	nop
	subq.b		#2,d1
	beq.s		g_sk7
	subq.b		#2,d1
	nop
	nop
	or.l		d0,d0
	move.b		d1,(a4)
	nop
patch_ws_1_3_B:
	or.l		d0,d0
	move.b		d0,(a4)
	nop
	bra.s		g_cont7
g_sk7:	nop
	nop
	or.l		d0,d0
	nop
patch_ste_B:
	or.l		d0,d0
	nop
patch_ws_1_3_D:
	exg		d0,d0
	move.b		d1,(a4)
	move.b		d0,(a4)
patch_ws_1_3_F:	
	nop
g_cont7:
	move.w		#0,(a0)
	or.l		d0,d0
	move.l		(a0),(a0)
	move.l		(a0),(a0)
	move.b		(a1)+,d1
	beq.s		.sk8
	or.l		d0,d0
	or.l		d0,d0
	bra.s		.nosk8
.sk8:	move.b		d0,(a5)
	moveq		#6,d1
	nop
	move.w		d0,(a5)
.nosk8:	move.w		syncshift(pc),d2
	lea		syncshift_tab(pc),a2
	move.w		0(a2,d2.w),d3
	add.w		d2,d2
	add.w		d2,d2
	lea		patch_unstabilizer_src(pc),a2
	adda.w		d2,a2
	lea		patch_us_target0(pc),a3
	move.l		(a2)+,(a3)+
	move.l		(a2)+,(a3)+
	lea		patch_unstabilizer_src(pc),a2
	adda.w		d3,a2
	lea		patch_us_target1(pc),a3
	move.l		(a2)+,(a3)+
	move.l		(a2)+,(a3)+
	subq.b		#2,d1
	bne.s		.sk9
	move.w		d0,(a4)
	move.b		d0,(a4)
	moveq		#0,d1
	bra.s		.nosk9
.sk9:	subq.w		#2,d1
	move.b		d1,(a4)
	move.b		d0,(a4)
	nop
	eor.b		d0,d1
.nosk9:	
	lea 		palette(pc),a3
	lea		$ffff8242.w,a2
	move.w		(a0),(a0)
	moveq		#1,d1
	move.w		(a0),(a0)
	move.l		(a0),(a0)
	move.l		(a3)+,(a2)+
	move.l		(a3)+,(a2)+
	move.l		(a3)+,(a2)+
	move.w		(a3)+,(a2)+
	moveq		#2,d2
	move.w		d0,(a4)
	move.b		d0,(a4)
	move.l		(a3)+,(a2)+
	move.l		(a3)+,(a2)+
patch_us_target0:
	move.b		d2,(a5)
	or.l		d0,d0
	move.w		d2,(a5)
	nop
	move.l		(a3)+,(a2)+
	move.l		(a3)+,(a2)+

	rts

stfm_palette:	
	dc.l		palette

*-----------------------------------------------------------------------------*
wakestate_detect:
*-----------------------------------------------------------------------------*
	bsr		qwaitVBL
	move.b		$ffff8260.w,d0
	clr.b		$ffff8260.w
	move.w		d0,-(a7)
	move		sr,-(a7)
	move		#$2700,sr
	bsr		qscr0
	lea		qscr0_result,a0
	move.b		d0,(a0)
	bsr		qscr1
	lea		qscr1_result,a0
	move.b		d0,(a0)
	bsr		qscr2
	lea		qscr2_result,a0
	move.b		d0,(a0)
	move		(a7)+,sr
	move.l		qscr_results,d0
	cmp.l		#$cca0cc,d0
	bne.s		.case0
	moveq		#4,d1
	bra.s		.go
.case0:	cmp.l		#$cca0a0,d0
	bne.s		.case1
	moveq		#3,d1
	bra.s		.go
.case1:	cmp.l		#$cccccc,d0
	bne.s		.case2
	moveq		#2,d1
	bra.s		.go
.case2:	cmp.l		#$a0a0a0,d0
	bne.s		.case3
	moveq		#1,d1
	bra.s		.go
.case3:	moveq		#0,d1
.go:	lea		_eWAKEUP,a0
	move.w		d1,(a0)
	bsr		qwaitVBL
	move.w		(a7)+,d0
	move.b		d0,$ffff8260.w
	move.w		_eWAKEUP,d0
	cmp.w		#1,d0
	beq.s		.patc
	cmp.w		#3,d0
	bne.s		.nopa
.patc:	bsr		patch_wakestates_1_3
.nopa:	rts

patch_wakestates_1_3:
	lea		patch_ws_1_3_A,a0
	lea		patch_ws_1_3_B,a1
	move.w		#$c140,(a0)
	move.w		#$c140,(a1)
	lea		patch_ws_1_3_C,a0
	lea		patch_ws_1_3_D,a1
	move.w		#$4e71,(a0)
	move.w		#$4e71,(a1)
	lea		patch_ws_1_3_E,a0
	lea		patch_ws_1_3_F,a1
	move.w		#$8080,(a0)
	move.w		#$8080,(a1)
	rts

patch_ste:
	lea		patch_ste_A,a0
	move.w		#$1881,(a0)+
	move.w		#$4e71,(a0)+
	move.w		#$1880,(a0)+
	move.w		#$8080,(a0)+
	move.w		#$8080,(a0)+
	lea		patch_ste_B,a0
	move.w		#$1881,(a0)+
	move.w		#$4e71,(a0)+
	move.w		#$1880,(a0)+
	move.w		#$8080,(a0)+
	move.w		#$8080,(a0)+
	rts


qscr0:	bsr		qwaitVBL
	lea		$ffff8209.w,a0
	lea		$ffff8240.w,a1
	lea		$ffff820a.w,a4
	lea		$ffff8260.w,a5
	moveq		#$20,d0
.wt:	tst.b		(a0)
	beq.s		.wt
	sub.b		(a0),d0
	lsl.w		d0,d0
	moveq		#2,d0
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	nop
	exg		d0,d0
	move.b		d0,(a5)
	move.w		d0,(a5)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	or.l		d0,d0
	move.b		d0,(a5)
	nop
	move.w		d0,(a5)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.b		(a0),d0
	rts

qscr1:	bsr		qwaitVBL
	lea		$ffff8209.w,a0
	lea		$ffff8240.w,a1
	lea		$ffff820a.w,a4
	lea		$ffff8260.w,a5
	moveq		#$20,d0
.wt:	tst.b		(a0)
	beq.s		.wt
	sub.b		(a0),d0
	lsl.w		d0,d0
	moveq		#2,d0
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	nop
	or.l		d0,d0
	move.b		d0,(a5)
	move.w		d0,(a5)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	or.l		d0,d0
	move.b		d0,(a5)
	nop
	move.w		d0,(a5)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.b		(a0),d0
	rts

qscr2:	bsr		qwaitVBL
	lea		$ffff8209.w,a0
	lea		$ffff8240.w,a1
	lea		$ffff820a.w,a4
	lea		$ffff8260.w,a5
	moveq		#$20,d0
.wt:	tst.b		(a0)
	beq.s		.wt
	sub.b		(a0),d0
	lsl.w		d0,d0
	moveq		#2,d0
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	nop
	exg		d0,d0
	move.w		d0,(a4)
	move.b		d0,(a4)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	or.l		d0,d0
	move.b		d0,(a5)
	nop
	move.w		d0,(a5)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.l		(a1),(a1)
	move.b		(a0),d0
	rts

*-----------------------------------------------------------------------------*
toggle_stabilizer:
*-----------------------------------------------------------------------------*
	lea		patch_unstabilizer_mode,a0
	not.w		(a0)
	lea		patch_unstabilizer_src,a1
	move.w		(a0),d0
	bne.s		.o1
	lea		patch_unstabilizer_opt0,a0
	bra.s		.o0
.o1:	lea		patch_unstabilizer_opt1,a0
.o0:	move.l		(a0)+,(a1)+
	move.l		(a0)+,(a1)+
	move.l		(a0)+,(a1)+
	move.l		(a0)+,(a1)+
	move.l		(a0)+,(a1)+
	move.l		(a0)+,(a1)+
	move.l		(a0)+,(a1)+
	move.l		(a0)+,(a1)+
	rts


patch_unstabilizer_src:
	move.b		d1,(a5)
	or.l		d0,d0
	move.w		d1,(a5)
	nop
	move.b		d1,(a5)
	or.l		d0,d0
	nop
	move.w		d1,(a5)
	move.b		d1,(a5)
	move.w		d1,(a5)
	or.l		d0,d0
	nop
	move.b		d1,(a5)
	nop
	move.w		d1,(a5)
	or.l		d0,d0

patch_unstabilizer_opt0:
	move.b		d2,(a5)
	or.l		d0,d0
	move.w		d2,(a5)
	nop
	move.b		d2,(a5)
	nop
	move.w		d2,(a5)
	or.l		d0,d0
	move.b		d2,(a5)
	move.w		d2,(a5)
	or.l		d0,d0
	nop
	move.b		d2,(a5)
	or.l		d0,d0
	nop
	move.w		d2,(a5)

patch_unstabilizer_opt1:
	move.b		d1,(a5)
	or.l		d0,d0
	move.w		d1,(a5)
	nop
	move.b		d1,(a5)
	or.l		d0,d0
	nop
	move.w		d1,(a5)
	move.b		d1,(a5)
	move.w		d1,(a5)
	or.l		d0,d0
	nop
	move.b		d1,(a5)
	nop
	move.w		d1,(a5)
	or.l		d0,d0
	or.l		d0,d0
	nop
	move.b		d1,(a5)
	lsr.b		#1,d1
	move.b		d1,(a5)
	nop

*-----------------------------------------------------------------------------*
qwaitVBL:
*-----------------------------------------------------------------------------*
	lea		$ffff8201.w,a0
	movep.w		0(a0),d0
	lea		$ffff8205.w,a0
.w0:	movep.w		0(a0),d1
	cmp.w		d0,d1
	beq.s		.w0
.w1:	movep.w		0(a0),d1
	cmp.w		d0,d1
	bne.s		.w1
	rts

wrte:	rte

*-----------------------------------------------------------------------------*

syncshift:		dc.l	0

syncshift_tab:		dc.w	$0000,$0018,$0010,$0008
		

*-----------------------------------------------------------------------------*

palette:		dc.w	$0000,$0224,$0422,$0424,$0444,$0557,$0755,$0757
			dc.w	$0000,$0000,$0000,$0000,$0000,$0000,$0000,$0000

*-----------------------------------------------------------------------------*
			data
*-----------------------------------------------------------------------------*
			
pframebuffer:		dc.l	0
_unused_rezcode:	dc.b	0
bSTE:			dc.b	0

_eWAKEUP:		dc.w	0
patch_unstabilizer_mode	dc.w	-1

qscr_results:		dc.b	0
qscr0_result:		dc.b	0
qscr1_result:		dc.b	0
qscr2_result:		dc.b	0

sync1_data:		dc.l	sync_data
sync2_data:		dc.l	sync_data+2

sscrl_scraddr:		dc.l	0
sscrl_offset:		dc.l	0

sync_data:		incbin	static\stf\hwscrlns.blk
			even

*-----------------------------------------------------------------------------*
			text
*-----------------------------------------------------------------------------*
