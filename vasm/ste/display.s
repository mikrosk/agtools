*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
* 	STE display module
*-------------------------------------------------------*

	COMMENT		HEAD=7
	OPT		P=68000,NOTYPE
	OPT		W+
	OPT		O-

*-------------------------------------------------------*

	XDEF		_STE_ConfigureDisplayService
	XDEF		_STE_InstallDisplayService
	XDEF		_STE_RemoveDisplayService

	XDEF		_g_displayservice_consumed_lines	; informs framebuffer of any difference between displaymemory start and framebuffer start
;	XDEF		_g_tshift
	XDEF		_g_linebytes

	XREF		_gp_active_group
	XREF		_g_vbl
	XREF		_g_fieldmask

	XREF		vbl_sema
	
	XREF		_YMSYS_VBCallback

	XREF		_blackscreen

	XREF		_bFalcon030
	

tacr		=	$fffffa19
tbcr		=	$fffffa1b
cdcr		=	$fffffa1d
tbdr		=	$fffffa21
tcdr		=	$fffffa23
cddr		=	$fffffa25
iera		=	$fffffa07
ierb		=	$fffffa09
imra		=	$fffffa13
imrb		=	$fffffa15
isra		=	$fffffa0f
isrb		=	$fffffa11

VBServiceVec			=	$a0
DMAFlag				=	$a4
	
*-------------------------------------------------------*
highlight_statusbar		=	0

use_auto_end_of_interrupt	=	0		; defaults to manual EOI for compatibility with other user-defined interrupts
use_blitter_compatibility	=	1		; default is to assume blitter is busy on STE, helps prevent flickery rasters
use_overscan			=	1		; enable 240 lines vs usual 200 (can be up to 274 but 240 is a good standard)
use_lower_border		=	1

; internal timera/timerb schedule for overscan mode
displayservice_upper_delay	=	(94+4)		; safety padding of 6...3, but err towards more padding (smaller value) with blitter in use
displayservice_first_delta_tb	=	12
displayservice_lower_delta_tb	=	229-0-displayservice_first_delta_tb
;displayservice_lower_delta_tb	=	229-2-displayservice_first_delta_tb
displayservice_final_delta_tb	=	39-16
timecritical_pad_tb		=	3-1		; TimerB padding (in scanlines)
timecritical_pad_ta		=	6-1		; TimerA padding

*-------------------------------------------------------*

blitoff	macro
	ifne		(use_blitter_compatibility)

	subq.w		#2,sp
	move.b		$ffff8a3c.w,(sp)
	bclr.b		#7,$ffff8a3c.w

	endc
	endm

bliton	macro
	ifne		(use_blitter_compatibility)

	move.b		(sp),$ffff8a3c.w
	addq.l		#2,sp

	endc
	endm

*-------------------------------------------------------*
_STE_ConfigureDisplayService:
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.savea6:		rs.l	1
.save:			rs.l	10
.return:		rs.l	1
*-------------------------------------------------------*
.frame_:		rs.b	0
*-------------------------------------------------------*
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
*-------------------------------------------------------*

	move.w		#0,_g_displayservice_consumed_lines

	ifne		(use_overscan)
	tst.b		_bFalcon030
	bne.s		.skip
	move.w		#displayservice_first_delta_tb,_g_displayservice_consumed_lines
.skip:
	endc
	
*-------------------------------------------------------*
	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts

*-------------------------------------------------------*
_STE_InstallDisplayService:
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.savea6:		rs.l	1
.save:			rs.l	10
.return:		rs.l	1
*-------------------------------------------------------*
.frame_:		rs.b	0
*-------------------------------------------------------*
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
*-------------------------------------------------------*
	or.w		#$0700,sr
*-------------------------------------------------------*
	lea		savestate,a1
	move.l		$68.w,(a1)+
	move.l		$70.w,(a1)+
	move.l		$120.w,(a1)+
	move.l		$134.w,(a1)+
*-------------------------------------------------------*
	lea		drte,a0
	move.l		a0,$68.w			; HBL
	move.l		a0,$120.w			; TimerB
	move.l		a0,$134.w			; TimerA

	tst.b		_bFalcon030
	bne.s		.f030
	move.l		#displayservice_ste_vbi,$70.w	; VBL
	bra.s		.done
.f030:	move.l		#displayservice_f030_vbi,$70.w	; VBL
.done:

;	jsr		_blackscreen
*-------------------------------------------------------*
	and.w		#$fbff,sr
*-------------------------------------------------------*
	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts

*-------------------------------------------------------*
_STE_RemoveDisplayService:
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.savea6:		rs.l	1
.save:			rs.l	10
.return:		rs.l	1
*-------------------------------------------------------*
.frame_:		rs.b	0
*-------------------------------------------------------*
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
*-------------------------------------------------------*
	; wait for vbl to elapse
	; note: display timers will already be scheduled
	move.w		$464.w,d0
.wvb0:	cmp.w		$464.w,d0
	beq.s		.wvb0
*-------------------------------------------------------*
	; turn off display timers and restore vectors
	or.w		#$0700,sr
	bclr.b		#5,imra.w
	bclr.b		#0,imra.w
	lea		savestate,a1
	move.l		(a1)+,$68.w
	move.l		(a1)+,$70.w
	move.l		(a1)+,$120.w
	move.l		(a1)+,$134.w
	and.w		#$fbff,sr
*-------------------------------------------------------*
	move.w		$464.w,d0
.wvb2:	cmp.w		$464.w,d0
	beq.s		.wvb2
*-------------------------------------------------------*
	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts

*-------------------------------------------------------*


	ifne		(use_overscan)

*-------------------------------------------------------*
displayservice_ste_vbi_dmasafe:
*-------------------------------------------------------*

	movem.l		d0-d7/a0-a6,-(sp)
	move.l		_gp_active_group,d0
	beq		.skip
	
	move.l		d0,a0
	moveq		#1,d0
	and.w		_g_vbl,d0
	and.w		_g_fieldmask,d0
	lsl.w		#2,d0
	move.l		(a0,d0.w),a0
	
	move.w		_g_linebytes,d1
	
	mulu.w		#29,d1
	
	move.l		(a0),d0
	lsr.l		#8,d0
	add.l		d1,d0
	
;	add.l		#188*16,d0
	move.b		d0,d1
	lsr.w		#8,d0
	move.l		d0,$ffff8200.w
	move.b		d1,$ffff820d.w
	
	move.b		4(a0),$ffff820f.w
	move.b		3(a0),$ffff8265.w

	move.l		d0,$ffff8204.w
	move.b		d1,$ffff8209.w

	move.b		4(a0),$ffff820f.w
	
	; load field palette
	
	lea		6(a0),a0
;	load palette immediately
	movem.l		(a0)+,d0-d7
	movem.l		d0-d7,$ffff8240.w

.skip:
	jsr		_YMSYS_VBCallback
	
	addq.w		#1,vbl_sema
	bgt.s		.nsvc
	
	tst.w		VBServiceVec.w
	beq.s		.nsvc
		
	move.l		VBServiceVec.w,a0
	jsr		(a0)
	
.nsvc:	subq.w		#1,vbl_sema

	movem.l		(sp)+,d0-d7/a0-a6
	subq.w		#1,_g_vbl
	
	addq.l		#1,$462.w	
	addq.l		#1,$466.w	

	bliton
	
 	rte

	endc

		
*-------------------------------------------------------*
displayservice_ste_vbi:
*-------------------------------------------------------*
	blitoff

	move.w		#$2700,sr

	ifne		(use_overscan)
	
;	tst.b		_bFalcon030
;	bne		displayservice_f030_vbi
	tst.b		DMAFlag.w
	bne		displayservice_ste_vbi_dmasafe
	
	clr.b		tacr.w				; TimerA CTRL: halt
	bset		#5,iera.w				; TimerA IE: enable
	bset		#5,imra.w				; TimerA IM: enable
	move.b		#displayservice_upper_delay-timecritical_pad_ta,$fffffa1f.w	; TimerA DATA: delay
	move.b		#4,tacr.w				; TimerA CTRL: start

	move.l		#displayservice_ste_upper_a,$134.w
	
	endc
	

	movem.l		d0-d7/a0-a6,-(sp)
	move.l		_gp_active_group,d0
	beq		.skip
	
	move.l		d0,a0
	moveq		#1,d0
	and.w		_g_vbl,d0
	and.w		_g_fieldmask,d0
	lsl.w		#2,d0
	move.l		(a0,d0.w),a0
	move.b		0(a0),$ffff8201.w
	move.b		1(a0),$ffff8203.w
	move.b		2(a0),$ffff820d.w
	move.b		4(a0),$ffff820f.w
	move.b		3(a0),$ffff8265.w
	move.b		0(a0),$ffff8205.w
	move.b		1(a0),$ffff8207.w
	move.b		2(a0),$ffff8209.w
	move.b		4(a0),$ffff820f.w
	
	; load field palette
	
	lea		6(a0),a0
	ifne		(use_overscan)	
;	if overscan is enabled, schedule palette on first real line, to achieve 240 lines
	move.l		a0,ste_palette
	
	
	moveq		#0,d0
	lea		$ffff8240.w,a0
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	
	else
;	otherwise load palette immediately
	movem.l		(a0)+,d0-d7
	movem.l		d0-d7,$ffff8240.w
	endc

.skip:
	jsr		_YMSYS_VBCallback
	
	addq.w		#1,vbl_sema
	bgt.s		.nsvc
	
	tst.w		VBServiceVec.w
	beq.s		.nsvc
		
	move.l		VBServiceVec.w,a0
	jsr		(a0)
	

.nsvc:	subq.w		#1,vbl_sema

	movem.l		(sp)+,d0-d7/a0-a6
		
	subq.w		#1,_g_vbl

	addq.l		#1,$462.w	
	addq.l		#1,$466.w	

	bliton
	
drte:  	rte


*-------------------------------------------------------*
displayservice_f030_vbi:
*-------------------------------------------------------*
	blitoff

	move.w		#$2700,sr

	movem.l		d0-d7/a0-a6,-(sp)
	move.l		_gp_active_group,d0
	beq		.skip
	
	move.l		d0,a0
	moveq		#1,d0
	and.w		_g_vbl,d0
	and.w		_g_fieldmask,d0
	lsl.w		#2,d0
	move.l		(a0,d0.w),a0
	move.b		0(a0),$ffff8201.w
	move.b		1(a0),$ffff8203.w
	move.b		2(a0),$ffff820d.w
	move.b		4(a0),$ffff820f.w
	move.b		3(a0),$ffff8265.w
	move.b		0(a0),$ffff8205.w
	move.b		1(a0),$ffff8207.w
	move.b		2(a0),$ffff8209.w
	move.b		4(a0),$ffff820f.w
	
	; load field palette
	
	lea		6(a0),a0

;	otherwise load palette immediately
	movem.l		(a0)+,d0-d7
	movem.l		d0-d7,$ffff8240.w


.skip:
	jsr		_YMSYS_VBCallback
	
	addq.w		#1,vbl_sema
	bgt.s		.nsvc
	
	tst.w		VBServiceVec.w
	beq.s		.nsvc
		
	move.l		VBServiceVec.w,a0
	jsr		(a0)

.nsvc:	subq.w		#1,vbl_sema

	movem.l		(sp)+,d0-d7/a0-a6
	subq.w		#1,_g_vbl

	addq.l		#1,$462.w	
	addq.l		#1,$466.w	

	bliton
	
  	rte


	ifne		(use_overscan)

*-----------------------------------------------------------------------------*
displayservice_ste_upper_a:
*-----------------------------------------------------------------------------*
	blitoff
*-----------------------------------------------------------------------------*
	movem.l		d0/d1/a0/a1,-(sp)
*-----------------------------------------------------------------------------*
*	critical: re-synchronize with free running TimerA data before
*	starting to count hblanks as TimerA may be delayed by BLITs and
*	we may not have landed on the correct line...
*-----------------------------------------------------------------------------*

	moveq		#1,d1
	lea		$ffff820a.w,a1	

	lea		$fffffa1f.w,a0
	moveq		#displayservice_upper_delay-(timecritical_pad_ta*2),d0
.tasyn:	cmp.b		(a0),d0
	blo.s		.tasyn
*-----------------------------------------------------------------------------*
	
	moveq		#0,d0
	
	move.w		#$2100,sr				; enable HBL
	stop		#$2100					; wait next HBL
	move.w		#$2700,sr				; lockout

	
	ifne		(1)
	
	divs.w		d1,d0
	divs.w		d1,d0
;	eor.l		d0,d0
;	eor.l		d0,d0
;	nop
;	lsl.l		#6,d0
	muls.w		d0,d0
;	muls.w		d0,d0
	lsl.l		#8,d0
;	eor.l		d0,d0
;	nop
	lsl.l		#3,d0
		
	endc
	
;	open top border

	move.b		d0,(a1)	;$ffff820a.w
	
	lsl.l		#8,d0

	move.w		a1,(a1)	;$ffff820a.w

	move.b		d0,tacr.w

	clr.b		tbcr.w				; TimerB CTRL: halt
	bset.b		#0,iera.w				; TimerB IE: enable
	bset.b		#0,imra.w				; TimerB IM: enable
	move.b		#displayservice_first_delta_tb-timecritical_pad_tb,tbdr.w	; TimerB DATA: count
	move.b		#8,tbcr.w				; TimerB CTRL: start))

	move.l		#displayservice_ste_first_b,$120.w

	movem.l		(sp)+,d0/d1/a0/a1

	bliton
	
	ifeq		(use_auto_end_of_interrupt)
;	manual EOI
	bclr.b		#5,isra.w
	endc
		
	rte
	
	endc


	ifne		(use_overscan)

*-----------------------------------------------------------------------------*
displayservice_ste_first_b:
*-----------------------------------------------------------------------------*
	blitoff
	
	move.w		#$2700,sr

	move.l		a0,-(sp)
	move.l		a1,-(sp)

	move.l		ste_palette,a0
	
	lea		tbdr.w,a1
.wl:	cmp.b		#displayservice_first_delta_tb-(timecritical_pad_tb*2),(a1)
	bhi.s		.wl
	
	lea		$ffff8240.w,a1	
	move.l		(a0)+,(a1)+
	move.l		(a0)+,(a1)+
	move.l		(a0)+,(a1)+
	move.l		(a0)+,(a1)+
	move.l		(a0)+,(a1)+
	move.l		(a0)+,(a1)+
	move.l		(a0)+,(a1)+
	move.l		(a0)+,(a1)+

	clr.b		tbcr.w				; TimerB CTRL: halt
	
	move.b		#displayservice_lower_delta_tb-timecritical_pad_tb,tbdr.w	; TimerB DATA: count
	move.b		#8,tbcr.w				; TimerB CTRL: start))

	move.l		#displayservice_ste_lower_b,$120.w
		
	move.l		(sp)+,a1
	move.l		(sp)+,a0
	
	bliton
	
	ifeq		(use_auto_end_of_interrupt)
;	manual EOI
	bclr.b		#0,isra.w
	endc
	
	rte

	endc


	ifne		(use_overscan)

*-----------------------------------------------------------------------------*
displayservice_ste_lower_b:
*-----------------------------------------------------------------------------*
	ifne		(use_blitter_compatibility)	

	blitoff
		
	move.w		#$2700,sr
	movem.l		d0/d1/a0/a1,-(sp)

;	tst.b		DMAFlag.w
;	bne		.safe_path
	
	lea		$ffff820a.w,a1
	moveq		#0,d1
	
;	not.w	$ffff8240.w

;	insert 'padding' scans to absorb short blitter jobs - 

	moveq		#displayservice_lower_delta_tb-(timecritical_pad_tb*2),d0
	lea		tbdr.w,a0
.wl:	cmp.b		(a0),d0
	blo.s		.wl



	ifne		(use_lower_border)
	
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
		
	endc		; (use_lower_border)
	

;	not.w	$ffff8240.w
	
	else		; (use_blitter_compatibility)	

	ifne		(use_lower_border)

	clr.b		$ffff820a.w			; 60 Hz
	move.w		#$2700,sr
	dcb.w		4,$4e71				; ...wait...
	move.b		#2,$ffff820a.w			; 50 Hz
	
	endc

	endc		; (use_blitter_compatibility)	

	clr.b		tbcr.w				; TimerB CTRL: halt

	ifne		(use_lower_border)

	move.b		#displayservice_final_delta_tb-1,tbdr.w	; TimerB DATA: count
	move.b		#8,tbcr.w				; TimerB CTRL: start))

	move.l		#displayservice_ste_final_b,$120.w
	
	else

	moveq		#0,d0
	lea		$ffff8240.w,a0
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+	
	
	endc

	ifne		(highlight_statusbar)
	lea		$ffff8240.w,a0
	move.l		#$06660666,d0
;	move.l		#$09990999,d0
	and.l		d0,(a0)+
	and.l		d0,(a0)+
	and.l		d0,(a0)+
	and.l		d0,(a0)+
	and.l		d0,(a0)+
	and.l		d0,(a0)+
	and.l		d0,(a0)+
	and.l		d0,(a0)+
	endc
	
	ifne		(use_blitter_compatibility)
	movem.l		(sp)+,d0/d1/a0/a1

	bliton
	endc
	
	ifeq		(use_auto_end_of_interrupt)
;	manual EOI
	bclr.b		#0,isra.w
	endc
	
	rte

	endc
	

*-----------------------------------------------------------------------------*
*	We land here if we're trying to open the bottom border while loading
*-----------------------------------------------------------------------------*
.safe_path:
*-----------------------------------------------------------------------------*

	moveq		#0,d0
	lea		$ffff8240.w,a0
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	
	ifne		(use_blitter_compatibility)
	movem.l		(sp)+,d0/d1/a0/a1

	bliton
	endc
	
	ifeq		(use_auto_end_of_interrupt)
;	manual EOI
	bclr.b		#0,isra.w
	endc
	
	rte
	
	
	ifne		(use_overscan)
	
*-----------------------------------------------------------------------------*
displayservice_ste_final_b:
*-----------------------------------------------------------------------------*
	blitoff
	
	move.l		d0,-(sp)
	move.l		a0,-(sp)

;	wipe out all colours - black lower border

	moveq		#0,d0
	lea		$ffff8242.w,a0

.wt:	cmp.b		#displayservice_final_delta_tb-1,tbdr.w
	beq.s		.wt		

	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.l		d0,(a0)+
	move.w		d0,(a0)+
	move.w		d0,$ffff8240.w

;	no more scanline scheduling this frame - stop TimerB

	move.b		d0,tbcr.w

	ifeq		(use_auto_end_of_interrupt)
;	manual EOI
	bclr.b		#0,isra.w
	endc
	
	move.l		(sp)+,a0
	move.l		(sp)+,d0
	
	bliton

	rte
	
	endc


*-------------------------------------------------------*
	data
*-------------------------------------------------------*

ste_palette:		dc.l	empty
empty:			dcb.w	16,0
	
_g_displayservice_consumed_lines:	
			dc.w	0
;_g_tshift:		dc.w	0
_g_linebytes:		dc.w	0
	
*-------------------------------------------------------*
	bss
*-------------------------------------------------------*

savestate:		ds.l	4

*-------------------------------------------------------*
	text
*-------------------------------------------------------*
