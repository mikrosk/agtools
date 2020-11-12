
split_point			set	136		; absolute max. around line 227 (low border)

displayservice_ste136s_d0_tb	=	0
displayservice_ste136s_ds_tb	=	split_point
displayservice_ste136s_dlb_tb	=	229-displayservice_ste136s_ds_tb
	 
*-------------------------------------------------------*
displayservice_ste136s_vbi:
*-------------------------------------------------------*
	blitoff
	
	move.w		#$2700,sr
	
	tst.b		DMAFlag.w
	bne		displayservice_ste_vbi_dmasafe
	
	.if		(use_overscan_timera)
	
	clr.b		tacr.w					; TimerA CTRL: halt
	bset		#5,iera.w				; TimerA IE: enable
	bset		#5,imra.w				; TimerA IM: enable
	move.b		#displayservice_upper_delay-timecritical_pad_ta,tadr.w	; TimerA DATA: delay
	move.b		#4,tacr.w				; TimerA CTRL: start

	move.l		#displayservice_ste136s_upper_a,$134.w
	
	.else

	and.b		#$f8,cdcr.w				; TimerD CTRL: halt
	bset		#4,ierb.w				; TimerD IE: enable
	bset		#4,imrb.w				; TimerD IM: enable
	move.b		#displayservice_upper_delay-timecritical_pad_td,cddr.w	; TimerD DATA: delay
	or.b		#4,cdcr.w				; TimerD CTRL: start
	
	move.l		#displayservice_ste136s_upper_d,$110.w

	.endif

	;move.w		#$555,$ffff8240.w


	movem.l		d0-d7/a0-a6,-(sp)

	.if		(use_debugconsole)
	move.l		_max_console,d0
	beq.s		.normal
	move.l		d0,d1
	lsr.w		#8,d0
	clr.b		$ffff8265.w
	move.l		d0,$ffff8200.w
	move.b		d1,$ffff820d.w
	move.b		#(dst_linewid-160)>>1,$ffff820f.w
	
	move.l		#$00340fff,$ffff8240.w
	clr.w		$ffff8244.w

	bra		.skip
	.endif
	
.normal:
	move.l		_gp_active_group+0,d0
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

;	schedule palette on first real line, to achieve 256 lines
	move.l		a0,ste_palette
	
	bsr		displayservice_ste136s
	
;	moveq		#0,d0
;	lea		$ffff8240.w,a0
;
;	.if		(^^defined TIMING_RASTERS | use_rasters)
;	move.w		BGRAST.w,(a0)+
;	move.w		d0,(a0)+
;	.else
;	move.l		d0,(a0)+
;	.endif
;
;	move.l		d0,(a0)+
;	move.l		d0,(a0)+
;	move.l		d0,(a0)+
;	move.l		d0,(a0)+
;	move.l		d0,(a0)+
;	move.l		d0,(a0)+
;	move.l		d0,(a0)+
	
	.if		(^^defined TIMING_RASTERS | use_rasters)
	move.w		BGRAST.w,$ffff8240.w
	.endif

.skip:
	.if		^^defined ENABLE_AGT_AUDIO
	jsr		_YMSYS_VBCallback
	.endif
	
	andi.w		#$fbff,sr

	addq.w		#1,vbl_sema
	bgt.s		.nsvc
	
	tst.w		2+VBServiceVec.w
	beq.s		.nsvc
	
	move.l		VBServiceVec.w,a0
	jsr		(a0)
	

.nsvc:	subq.w		#1,vbl_sema

	movem.l		(sp)+,d0-d7/a0-a6
		
	subq.w		#1,_g_vbl

	addq.l		#1,$462.w	
	addq.l		#1,$466.w	

	.if		(use_overscan_timera)
	.else
	.if		^^defined AGT_CONFIG_PROFILER
	.else
	addq.l		#4,$4ba.w			; simulate TimerC, since we have turned it off with TimerD timecritical
	.endif
	.endif

	move.b		#1,_g_seek

	bliton
	
  	rte



	.if		(use_overscan_timera)

*-----------------------------------------------------------------------------*
displayservice_ste136s_upper_a:
*-----------------------------------------------------------------------------*
	blitoff
*-----------------------------------------------------------------------------*
	movem.l		d0/d1/a0/a1,-(sp)
*-----------------------------------------------------------------------------*
*	critical: re-synchronize with free running TimerA data before
*	starting to count hblanks as TimerA may be delayed by BLITs and
*	we may not have landed on the correct line...
*-----------------------------------------------------------------------------*

	;move.w		#$fff,$ffff8240.w
	
	moveq		#1,d1
	lea		$ffff820a.w,a1	

	lea		tadr.w,a0
	moveq		#displayservice_upper_delay-(timecritical_pad_ta*2),d0
.tasyn:	cmp.b		(a0),d0
	blo.s		.tasyn
*-----------------------------------------------------------------------------*
	
	moveq		#0,d0
	
	move.w		#$2100,sr				; enable HBL
	stop		#$2100					; wait next HBL
	move.w		#$2700,sr				; lockout
		
	.if		(1)

	dcb.w		94,$4e71
	
;	divs.w		d1,d0
;	divs.w		d1,d0	
;;	eor.l		d0,d0
;;	eor.l		d0,d0
;;	nop
;;	lsl.l		#6,d0
;	muls.w		d0,d0
;;	muls.w		d0,d0
;	lsl.l		#8,d0
;;	eor.l		d0,d0
;;	nop
;	lsl.l		#3,d0
		
	.endif

;	open top border

	move.b		d0,(a1)	;$ffff820a.w
	
	lsl.l		#8,d0
	
	move.w		a1,(a1)	;$ffff820a.w

;	move.b		d0,tbcr.w				; TimerB CTRL: halt
;	bset.b		#0,iera.w				; TimerB IE: enable
;	bset.b		#0,imra.w				; TimerB IM: enable
;	move.b		#displayservice_ste136s_ds_tb-timecritical_pad_tb,tbdr.w	; TimerB DATA: count
;	move.b		#8,tbcr.w				; TimerB CTRL: start))
;
;	move.l		#displayservice_ste136s_split_b,$120.w

	move.b		d0,tacr.w				; TimerA CTRL: halt

	movem.l		(sp)+,d0/d1/a0/a1

	;move.w		#$0,$ffff8240.w

	bliton
	
	.if		(0=use_auto_end_of_interrupt)
;	manual EOI
	bclr.b		#5,isra.w
	.endif
		
	rte

	.else

*-----------------------------------------------------------------------------*
displayservice_ste136s_upper_d:
*-----------------------------------------------------------------------------*
	blitoff
	move.w		#$2700,sr
*-----------------------------------------------------------------------------*
	movem.l		d0/d1/a0/a1,-(sp)
*-----------------------------------------------------------------------------*
*	critical: re-synchronize with free running TimerA data before
*	starting to count hblanks as TimerA may be delayed by BLITs and
*	we may not have landed on the correct line...
*-----------------------------------------------------------------------------*
	;move.w		#$fff,$ffff8240.w

	moveq		#1,d1
	lea		$ffff820a.w,a1	

	lea		cddr.w,a0
	moveq		#displayservice_upper_delay-(timecritical_pad_td*2),d0
.tasyn:	cmp.b		(a0),d0
	blo.s		.tasyn
*-----------------------------------------------------------------------------*
	
	moveq		#0,d0
	
	move.w		#$2100,sr				; enable HBL
	stop		#$2100					; wait next HBL
	move.w		#$2700,sr				; lockout
		
	.if		(1)

	dcb.w		94,$4e71
	
;	divs.w		d1,d0
;	divs.w		d1,d0	
;;	eor.l		d0,d0
;;	eor.l		d0,d0
;;	nop
;;	lsl.l		#6,d0
;	muls.w		d0,d0
;;	muls.w		d0,d0
;	lsl.l		#8,d0
;;	eor.l		d0,d0
;;	nop
;	lsl.l		#3,d0
		
	.endif

;	open top border

	move.b		d0,(a1)	;$ffff820a.w
	
	lsl.l		#8,d0
	
	move.w		a1,(a1)	;$ffff820a.w

;	move.b		d0,tbcr.w				; TimerB CTRL: halt
;	bset.b		#0,iera.w				; TimerB IE: enable
;	bset.b		#0,imra.w				; TimerB IM: enable
;	move.b		#displayservice_ste136s_ds_tb-timecritical_pad_tb,tbdr.w	; TimerB DATA: count
;	move.b		#8,tbcr.w				; TimerB CTRL: start))
;
;	move.l		#displayservice_ste136s_split_b,$120.w

	and.b		#$f8,cdcr.w				; TimerD CTRL: halt


	movem.l		(sp)+,d0/d1/a0/a1

	;move.w		#$0,$ffff8240.w

	bliton
	
	.if		(0=use_auto_end_of_interrupt)
;	manual EOI
	bclr.b		#4,isrb.w
	.endif
		
	rte

	.endif


*-----------------------------------------------------------------------------*
displayservice_ste136s_split_b:
*-----------------------------------------------------------------------------*
	.if		(use_blitter_compatibility)
	blitoff
	.endif
	move.w		#$2700,sr
*-----------------------------------------------------------------------------*

	movem.l		d0/a0/a1,-(sp)

	move.l		_gp_active_group+4,d0
	beq.s		.skip
	
	move.l		d0,a0
	moveq		#1,d0
	and.w		_g_vbl,d0
	and.w		_g_fieldmask,d0
	lsl.w		#2,d0
	move.l		(a0,d0.w),a0

;	insert 'padding' scans to absorb short blitter jobs - 

	moveq		#displayservice_ste136s_ds_tb-(timecritical_pad_tb*2),d0
	lea		tbdr.w,a1
.wl:	cmp.b		(a1),d0
	blo.s		.wl

	move.b		4(a0),$ffff820f.w
	move.b		3(a0),$ffff8265.w
	move.b		0(a0),$ffff8205.w
	move.b		1(a0),$ffff8207.w
	move.b		2(a0),$ffff8209.w
	move.b		4(a0),$ffff820f.w
	
;	not.w		$ffff8240.w
	
	clr.b		tbcr.w					; TimerB CTRL: halt
;	bset.b		#0,iera.w				; TimerB IE: enable
;	bset.b		#0,imra.w				; TimerB IM: enable
	move.b		#displayservice_ste136s_dlb_tb-timecritical_pad_tb,tbdr.w	; TimerB DATA: count
	move.b		#8,tbcr.w				; TimerB CTRL: start))

	move.l		#displayservice_ste136s_lower_b,$120.w

.skip:
	movem.l		(sp)+,d0/a0/a1

*-----------------------------------------------------------------------------*

	.if		(use_blitter_compatibility)
	bliton
	.endif
	
	.if		(0=use_auto_end_of_interrupt)
;	manual EOI
	bclr.b		#0,isra.w
	.endif
	
	rte
	
*-----------------------------------------------------------------------------*
displayservice_ste136s:	;_first_b:
*-----------------------------------------------------------------------------*
	
	clr.b		tbcr.w				; TimerB CTRL: halt

	.if		(use_lower_border)	

	bset.b		#0,iera.w				; TimerB IE: enable
	bset.b		#0,imra.w				; TimerB IM: enable
	move.b		#displayservice_ste136s_ds_tb-timecritical_pad_tb,tbdr.w	; TimerB DATA: count
	move.b		#8,tbcr.w				; TimerB CTRL: start))

;	move.b		#displayservice_ste136s_dlb_tb-timecritical_pad_tb,tbdr.w	; TimerB DATA: count
;	move.b		#8,tbcr.w				; TimerB CTRL: start))

;	.if		(use_debugconsole)
;	tst.w		_max_console
;	bne		.normal
;	tst.w		_min_pconsole
;	beq.s		.normal
;	move.l		#displayservice_ste136s_lowerconsole_b,$120.w
;	bra.s		.cont
;	.endif		;(use_debugconsole)
;	
.normal:
	move.l		#displayservice_ste136s_split_b,$120.w
.cont:
	.endif		;(use_lower_border)
	
	move.l		ste_palette,a0
	.if		(use_debugconsole)
	tst.w		_max_console
	beq.s		.ncon
	lea		con_palette,a0	
.ncon:	
	.endif
	
		
	lea		$ffff8240.w,a1	
	.if		(^^defined TIMING_RASTERS | use_rasters)
	move.w		BGRAST.w,(a1)+
	addq		#2,a0
	move.w		(a0)+,(a1)+
	.else
	move.l		(a0)+,(a1)+
	.endif
	move.l		(a0)+,(a1)+
	move.l		(a0)+,(a1)+
	move.l		(a0)+,(a1)+
	move.l		(a0)+,(a1)+
	move.l		(a0)+,(a1)+
	move.l		(a0)+,(a1)+
	move.l		(a0)+,(a1)+

			
	rts

*-----------------------------------------------------------------------------*
displayservice_ste136s_lower_b:
*-----------------------------------------------------------------------------*
	.if		(use_blitter_compatibility)	

	blitoff
		
	move.w		#$2700,sr
	movem.l		d0/a0,-(sp)

;	insert 'padding' scans to absorb short blitter jobs - 

	moveq		#displayservice_ste136s_dlb_tb-(timecritical_pad_tb*2),d0
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
		
	
	.else		; (use_blitter_compatibility)	

	clr.b		$ffff820a.w			; 60 Hz
	move.w		#$2700,sr
	dcb.w		4,$4e71				; ...wait...
	move.b		#2,$ffff820a.w			; 50 Hz
		
	movem.l		d0/a0,-(sp)

	.endif		; (use_blitter_compatibility)	

	clr.b		tbcr.w				; TimerB CTRL: halt
	
	.if		(highlight_statusbar)
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
	.endif
	
	movem.l		(sp)+,d0/a0
	
	clr.b		_g_seek
	
	.if		(use_blitter_compatibility)
	bliton
	.endif
	
	.if		(0=use_auto_end_of_interrupt)
;	manual EOI
	bclr.b		#0,isra.w
	.endif
	
	rte

	.if		(use_debugconsole)

*-----------------------------------------------------------------------------*
displayservice_ste136s_lowerconsole_b:
*-----------------------------------------------------------------------------*
;	.if		(use_blitter_compatibility)	

	blitoff
		
	move.w		#$2700,sr
	movem.l		d0/d1/a0,-(sp)

;	move.l		_min_pconsole,d1
	move.l		_min_pconsole,a0
	move.l		(a0),d1
	
;	insert 'padding' scans to absorb short blitter jobs - 

	moveq		#displayservice_ste136s_dlb_tb-(timecritical_pad_tb*2),d0
	lea		tbdr.w,a0
.wl:	cmp.b		(a0),d0
	blo.s		.wl

	; 10 nops worth
	move.l		#$00340fff,$ffff8240.w
	clr.w		$ffff8244.w

;	nop		; pad until lower border event
;	nop
;	nop
;	nop
;	nop
;	nop
;	nop
;	nop
;	nop		; HW: almost stable
;	nop		; HW: stable
;;	nop		; HW: almost stable

	clr.b		$ffff820a.w			; 60 Hz
	dcb.w		5,$4e71				; ...wait...
	move.b		#2,$ffff820a.w			; 50 Hz
	
;	.else		; (use_blitter_compatibility)	
;
;	clr.b		$ffff820a.w			; 60 Hz
;	move.w		#$2700,sr
;	dcb.w		4,$4e71				; ...wait...
;	move.b		#2,$ffff820a.w			; 50 Hz
;	
;	movem.l		d0/d1/a0,-(sp)
;	
;	.endif		; (use_blitter_compatibility)	

;	if console is enabled by build flags, 
;	show it in lower border if the console buffer is valid
	
	move.b		d1,$ffff8265.w
	move.b		d1,$ffff8209.w
	lsr.w		#8,d1
	move.l		d1,$ffff8204.w
	move.b		#(dst_linewid-160)>>1,$ffff820f.w

;	lea		$ffff8240.w,a0
;	moveq		#0,d0
;	subq.w		#1,d0
;	move.l		d0,(a0)+
;	clr.w		d0
;	move.l		d0,(a0)+
;	move.l		d0,(a0)+
;	move.l		d0,(a0)+
;	move.l		d0,(a0)+
;	move.l		d0,(a0)+
;	move.l		d0,(a0)+
;	move.l		d0,(a0)+

	move.b		d0,tbcr.w				; TimerB CTRL: halt
	
	movem.l		(sp)+,d0/d1/a0

	clr.b		_g_seek

;	.if		(use_blitter_compatibility)
	bliton
;	.endif
	
	.if		(0=use_auto_end_of_interrupt)
;	manual EOI
	bclr.b		#0,isra.w
	.endif
	
	rte

	.endif		;(use_debugconsole)
	
		
