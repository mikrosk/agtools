*-------------------------------------------------------*
*	STFDisplayService:DHS7
*-------------------------------------------------------*
*	This is a minor reworking of the DHS reference
*	7-scan syncscroll routine (original info below)
* 	The layout has been changed for a 192-line or
*	240-line display as per other displayservice
*	routines and to incorporate the displaygroup
*	manager.
*-------------------------------------------------------*

; Syncscroll with Timer A setup
; Kills top border along syncscroll Timer A
; Kills lower border with Timer B
; Screen can be at any even address
; Syncscroller uses 7 scanlines + syncline = ~2.24% CPU
; 320*266 usable resolution
;
; 20141105
; ae@dhs.nu

*-------------------------------------------------------*
		text
*-------------------------------------------------------*

displayservice_first_delay	=	193		; delay before syncscroll (standard/192)
displayservice_upper_delay	=	94+4		; delay before syncscroll (overscan/240)
;displayservice_first_delta_tb	=	12
displayservice_lower_delta_tb	=	229		; scheduled from VBI
displayservice_line240_delta_tb	=	17
displayservice_line192_delta_tb	=	198
;timecritical_pad		=	1

*-------------------------------------------------------*
displayservice_stf_dhs7_vbi:
*-------------------------------------------------------*
	move.w		#$2700,sr
*-------------------------------------------------------*
*	schedule upper border / TimerA
*-------------------------------------------------------*
	clr.b		$fffffa19.w
	bset		#5,$fffffa07.w
	bset		#5,$fffffa13.w
	ifne		(use_overscan)
	move.b		#displayservice_upper_delay,$fffffa1f.w
	else
	move.b		#displayservice_first_delay,$fffffa1f.w
	endc
	move.b		#4,$fffffa19.w
*-------------------------------------------------------*
*	schedule lower border / TimerB
*-------------------------------------------------------*
	ifne		(use_overscan)
	clr.b		$fffffa1b.w
	bset		#0,$fffffa07.w
	bset		#0,$fffffa13.w
	move.b		#displayservice_lower_delta_tb,$fffffa21.w
	move.b		#8,$fffffa1b.w
	move.l		#displayservice_stf_dhs7_lower_b,$120.w
	else
	clr.b		$fffffa1b.w
	bset		#0,$fffffa07.w
	bset		#0,$fffffa13.w
	move.b		#displayservice_line192_delta_tb,$fffffa21.w
	move.b		#8,$fffffa1b.w
	move.l		#displayservice_stf_dhs7_final_b,$120.w
	endc
*-------------------------------------------------------*
*	erase palette with current BG colour
*-------------------------------------------------------*
	move.l		d0,-(sp)
	move.l		a0,-(sp)
	lea		$ffff8240.w,a0	
	move.w		(a0),d0
	swap		d0
	move.w		(a0),d0
	rept		8
	move.l		d0,(a0)+
	endr
	move.l		(sp)+,a0
	move.l		(sp)+,d0
	addq.l		#1,$462.w	
	addq.l		#1,$466.w
	rte

*-------------------------------------------------------*
*	open upper border
*-------------------------------------------------------*
displayservice_stf_dhs7_upper_a:	
*-------------------------------------------------------*
	movem.l		d0-a6,-(sp)
*-------------------------------------------------------*
	move.l		stfm_palette,a6
*-------------------------------------------------------*
	move.w		#$2100,sr			;Enable HBL
	stop		#$2100				;Wait for HBL
	move.w		#$2700,sr			;Stop all interrupts
	clr.b		$fffffa19.w			;Stop Timer A
*-------------------------------------------------------*
*	upper border...
*-------------------------------------------------------*
	ifne		(use_overscan)
	dcb.w 		84,$4e71			;Zzzz
	clr.b		$ffff820a.w			;60 Hz
	dcb.w 		9,$4e71				;
	move.b		#2,$ffff820a.w			;50 Hz
	else
;	not.w		$ffff8240.w
	endc
*-------------------------------------------------------*
*	hardsync
*-------------------------------------------------------*
	lea		$ffff8209.w,a0
	moveq		#127,d1
.sync:	tst.b		(a0)
	beq.s		.sync
	move.b		(a0),d2
	sub.b		d2,d1
	lsr.l		d1,d1
*-------------------------------------------------------*
*	pre-fetch field palette
*-------------------------------------------------------*
	dcb.w		61-19,$4e71			; -19 nops
	movem.l		(a6),d0-d6/a1			; +19 nops
*-------------------------------------------------------*
*	syncscroll
*-------------------------------------------------------*
	lea		sscrl_jumplist,a3		;3
	move.l		(a3)+,a0			;3
	moveq		#2,d7				;1
	rept		7
	jsr		(a0)				;4*7=28  Seven scanlines of different linewidth
	endr
*-------------------------------------------------------*
*	load field palette
*-------------------------------------------------------*
	movem.l		d0-d6/a1,$ffff8240.w
*-------------------------------------------------------*
	movem.l		(sp)+,d0-a6
	ifeq		(use_auto_end_of_interrupt)
	bclr.b 		#5,$fffffa0f.w
	endc
	rte


	ifne		(use_overscan)
	
*-------------------------------------------------------*
*	open the lower border and schedule final line
*-------------------------------------------------------*
displayservice_stf_dhs7_lower_b:
*-------------------------------------------------------*
	clr.b		$ffff820a.w			; 60 Hz
	rept		5
	nop						; ...
	endr
	move.b		#2,$ffff820a.w			; 50 Hz
*-------------------------------------------------------*
	clr.b		$fffffa1b.w
	bset		#0,$fffffa07.w
	bset		#0,$fffffa13.w
	move.b		#displayservice_line240_delta_tb,$fffffa21.w
	move.b		#8,$fffffa1b.w
	move.l		#displayservice_stf_dhs7_final_b,$120.w
*-------------------------------------------------------*
	ifeq		(use_auto_end_of_interrupt)
	bclr.b 		#0,$fffffa0f.w
	endc
	rte

	endc

*-------------------------------------------------------*
*	we're beyond the final visible line...
*-------------------------------------------------------*
displayservice_stf_dhs7_final_b:
*-------------------------------------------------------*
	move.w		#$2700,sr
	clr.b		$fffffa1b.w
*-------------------------------------------------------*
	movem.l		d0-d7/a0-a6,-(sp)
*-------------------------------------------------------*
*	wait until displaygroup becomes active
*-------------------------------------------------------*
	move.l		_gp_active_group,d0
	beq.s		.skip
*-------------------------------------------------------*
	move.l		d0,a0
	moveq		#1,d0
	and.w		_g_vbl,d0
	and.w		_g_fieldmask,d0
	add.w		d0,d0
	add.w		d0,d0
	move.l		(a0,d0.w),a6
	move.l		(a6),d0
	lsr.l		#8,d0	
	move.l		d0,sscrl_offset
*-------------------------------------------------------*
*	schedule palette for next VBL
*-------------------------------------------------------*
	addq.l		#6,a6
	moveq		#1,d0
	and.w		_g_vbl,d0
	lsl.w		#5,d0
	add.w		d0,a6
	move.l		a6,stfm_palette
*-------------------------------------------------------*
*	schedule syncscroll pattern for next VBL
*-------------------------------------------------------*
	bsr		displayservice_stf_dhs7_commit
*-------------------------------------------------------*
*	set physbase for next VBL
*-------------------------------------------------------*
	move.l		sscrl_offset,d0
	lsr.w		#8,d0
	move.l		d0,$ffff8200.w
.skip:
*-------------------------------------------------------*
*	we're near the end of the last scan now..
*-------------------------------------------------------*
	moveq		#0,d0
	lea		32+$ffff8240.w,a0
	move.l		d0,-(a0)
	move.l		d0,-(a0)
	move.l		d0,-(a0)
	move.l		d0,-(a0)
	move.l		d0,-(a0)
	move.l		d0,-(a0)
	move.l		d0,-(a0)
	move.l		d0,-(a0)
*-------------------------------------------------------*
	movem.l		(sp)+,d0-d7/a0-a6
*-------------------------------------------------------*
*	mainloop synchronizes here
*-------------------------------------------------------*
	subq.w		#1,_g_vbl
*-------------------------------------------------------*	
	ifeq		(use_auto_end_of_interrupt)
	bclr.b		#0,$fffffa0f.w
	endc
	rte
		
*-------------------------------------------------------*

drte:
dummy:	rte

						
*-------------------------------------------------------*
*	syncscroll routines & data follow...
*-------------------------------------------------------*

*-------------------------------------------------------*
displayservice_stf_dhs7_init:
*-------------------------------------------------------*
	lea		sscrl_jumplist,a0
	lea		sscrl_line0,a1
	moveq		#7-1,d0
.in:	move.l		a1,(a0)+
	dbra		d0,.in
	rts
		
*-------------------------------------------------------*
displayservice_stf_dhs7_commit:					;Setup the jumptable each VBL
*-------------------------------------------------------*
	move.l		scraddr,d2
	add.l		sscrl_offset,d2
	and.w		#$00fe,d2

	lea		sscrl_linewidths,a0		; overscan routines for different line widths
	lea		sscrl_combo_table,a1		; syncscroll pattern table (128 patterns)
	lea		sscrl_jumplist,a2		; pattern for this frame
	lsl.w		#2,d2
	add.w		d2,a1

	rept		7
	moveq		#0,d0
	move.b		(a1)+,d0			;Get routine to run (0-6)
	lsl.w		#2,d0				;Align to address list (4 byte boundary)
	move.l		(a0,d0.w),(a2)+			;Write address to list
	endr

	rts

;-------------- Overscan routines

	; 160 byte line
sscrl_line0:
	dcb.w		117,$4e71
	move.l		(a3)+,a0			;3 fetch next address
	rts					;4

	; 158 byte line
sscrl_line1:
	dcb.w		94,$4e71
	move.w		d7,$ffff820a.w			;3
	move.b		d7,$ffff820a.w			;3
	dcb.w		17,$4e71
	move.l		(a3)+,a0			;3
	rts					;4

	; 184 byte line
sscrl_line2:
	dcb.w		2,$4e71
	move.b		d7,$ffff8260.w			;3
	move.w		d7,$ffff8260.w			;3
	dcb.w		86,$4e71
	move.w		d7,$ffff820a.w			;3
	move.b		d7,$ffff820a.w			;3
	dcb.w		10,$4e71
	move.l		(a3)+,a0			;3
	move.b		d7,$ffff8260.w			;3
	move.w		d7,$ffff8260.w			;3
	nop
	rts					;4

	; 204 byte line
sscrl_line3:	
	dcb.w		96,$4e71
	move.w		d7,$ffff820a.w			;3
	move.b		d7,$ffff820a.w			;3
	dcb.w		15,$4e71
	move.l		(a3)+,a0			;3
	rts					;4

	; 230 byte line
sscrl_line4:
	dcb.w		2,$4e71
	move.b		d7,$ffff8260.w			;3
	move.w		d7,$ffff8260.w			;3
	dcb.w		88,$4e71
	move.w		d7,$ffff820a.w			;3
	move.b		d7,$ffff820a.w			;3
	dcb.w		8,$4e71
	move.l		(a3)+,a0			;3
	move.b		d7,$ffff8260.w			;3
	move.w		d7,$ffff8260.w			;3
	nop
	rts						;4

	; 186 byte line
sscrl_line5:
	dcb.w		2,$4e71
	move.b		d7,$ffff8260.w			;3
	move.w		d7,$ffff8260.w			;3
	dcb.w		102,$4e71
	move.l		(a3)+,a0			;3
	move.b		d7,$ffff8260.w			;3
	move.w		d7,$ffff8260.w			;3
	nop
	rts						;4

	; 54 byte line
sscrl_line6:
	dcb.w		42,$4e71
	move.b		d7,$ffff8260.w			;3
	move.w		d7,$ffff8260.w			;3
	dcb.w		69,$4e71
	move.l		(a3)+,a0			;3
	rts						;4
		
*-------------------------------------------------------*
		data
*-------------------------------------------------------*

sscrl_scraddr:	dc.l	0
sscrl_offset:	dc.l	0

sscrl_linewidths:
		dc.l	sscrl_line0			;+000 bytes
		dc.l	sscrl_line1			;-002 bytes
		dc.l	sscrl_line2			;+024 bytes
		dc.l	sscrl_line3			;+044 bytes
		dc.l	sscrl_line4			;+070 bytes
		dc.l	sscrl_line5			;+026 bytes
		dc.l	sscrl_line6			;-106 bytes

sscrl_combo_table:
		dc.b 	0,0,0,0,0,0,0,0  		;0
 		dc.b 	6,4,3,1,1,1,0,0  		;2
 		dc.b 	6,4,3,1,1,0,0,0  		;4
 		dc.b 	6,4,3,1,0,0,0,0  		;6
 		dc.b 	6,4,3,0,0,0,0,0  		;8
 		dc.b 	6,4,2,2,1,0,0,0  		;10
 		dc.b 	6,4,2,2,0,0,0,0  		;12
 		dc.b 	6,5,4,2,0,0,0,0  		;14
 		dc.b 	6,5,5,4,0,0,0,0  		;16
 		dc.b 	2,1,1,1,0,0,0,0  		;18
 		dc.b 	2,1,1,0,0,0,0,0  		;20
 		dc.b 	2,1,0,0,0,0,0,0  		;22
 		dc.b 	2,0,0,0,0,0,0,0  		;24
 		dc.b 	5,0,0,0,0,0,0,0  		;26
 		dc.b 	6,4,4,1,1,1,0,0  		;28
 		dc.b 	6,4,4,1,1,0,0,0  		;30
 		dc.b 	6,4,4,1,0,0,0,0  		;32
 		dc.b 	6,4,4,0,0,0,0,0  		;34
 		dc.b 	3,1,1,1,1,0,0,0  		;36
 		dc.b 	3,1,1,1,0,0,0,0  		;38
 		dc.b 	3,1,1,0,0,0,0,0  		;40
 		dc.b 	3,1,0,0,0,0,0,0  		;42
 		dc.b 	3,0,0,0,0,0,0,0  		;44
 		dc.b 	2,2,1,0,0,0,0,0  		;46
 		dc.b 	2,2,0,0,0,0,0,0  		;48
 		dc.b 	5,2,0,0,0,0,0,0  		;50
 		dc.b 	5,5,0,0,0,0,0,0  		;52
 		dc.b 	6,4,4,2,1,1,0,0  		;54
 		dc.b 	6,4,4,2,1,0,0,0  		;56
 		dc.b 	6,4,4,2,0,0,0,0  		;58
 		dc.b 	6,5,4,4,0,0,0,0  		;60
 		dc.b 	4,1,1,1,1,0,0,0  		;62
 		dc.b 	4,1,1,1,0,0,0,0  		;64
 		dc.b 	4,1,1,0,0,0,0,0  		;66
 		dc.b 	4,1,0,0,0,0,0,0  		;68
 		dc.b 	4,0,0,0,0,0,0,0  		;70
 		dc.b 	2,2,2,0,0,0,0,0  		;72
 		dc.b 	5,2,2,0,0,0,0,0  		;74
 		dc.b 	5,5,2,0,0,0,0,0  		;76
 		dc.b 	5,5,5,0,0,0,0,0 		;78
 		dc.b 	3,3,1,1,1,1,0,0  		;80
 		dc.b 	3,3,1,1,1,0,0,0  		;82
 		dc.b 	3,3,1,1,0,0,0,0  		;84
 		dc.b 	3,3,1,0,0,0,0,0  		;86
 		dc.b 	3,3,0,0,0,0,0,0  		;88
 		dc.b 	4,2,1,1,0,0,0,0  		;90
 		dc.b 	4,2,1,0,0,0,0,0  		;92
 		dc.b 	4,2,0,0,0,0,0,0  		;94
 		dc.b 	5,4,0,0,0,0,0,0  		;96
 		dc.b 	5,2,2,2,0,0,0,0  		;98
 		dc.b 	5,5,2,2,0,0,0,0  		;100
 		dc.b 	5,5,5,2,0,0,0,0  		;102
 		dc.b 	6,4,4,4,0,0,0,0  		;104
 		dc.b 	4,3,1,1,1,1,0,0  		;106
 		dc.b 	4,3,1,1,1,0,0,0  		;108
 		dc.b 	4,3,1,1,0,0,0,0  		;110
 		dc.b 	4,3,1,0,0,0,0,0  		;112
 		dc.b 	4,3,0,0,0,0,0,0  		;114
 		dc.b 	4,2,2,1,0,0,0,0  		;116
 		dc.b 	4,2,2,0,0,0,0,0  		;118
 		dc.b 	5,4,2,0,0,0,0,0  		;120
 		dc.b 	5,5,4,0,0,0,0,0  		;122
 		dc.b 	5,5,2,2,2,0,0,0  		;124
 		dc.b 	5,5,5,2,2,0,0,0  		;126
 		dc.b 	3,3,3,1,1,0,0,0  		;128
 		dc.b 	3,3,3,1,0,0,0,0  		;130
 		dc.b 	3,3,3,0,0,0,0,0  		;132
 		dc.b 	4,4,1,1,1,0,0,0  		;134
 		dc.b 	4,4,1,1,0,0,0,0  		;136
 		dc.b 	4,4,1,0,0,0,0,0  		;138
 		dc.b 	4,4,0,0,0,0,0,0  		;140
 		dc.b 	4,2,2,2,0,0,0,0  		;142
 		dc.b 	5,4,2,2,0,0,0,0  		;144
 		dc.b 	5,5,4,2,0,0,0,0  		;146
 		dc.b 	5,5,5,4,0,0,0,0  		;148
 		dc.b 	5,5,5,2,2,2,0,0  		;150
 		dc.b 	4,3,3,1,1,1,0,0  		;152
 		dc.b 	4,3,3,1,1,0,0,0  		;154
 		dc.b 	4,3,3,1,0,0,0,0  		;156
 		dc.b 	4,3,3,0,0,0,0,0  		;158
 		dc.b 	4,4,2,1,1,0,0,0  		;160
 		dc.b 	4,4,2,1,0,0,0,0  		;162
 		dc.b 	4,4,2,0,0,0,0,0  		;164
 		dc.b 	5,4,4,0,0,0,0,0  		;166
 		dc.b 	5,4,2,2,2,0,0,0  		;168
 		dc.b 	5,5,4,2,2,0,0,0  		;170
 		dc.b 	5,5,5,4,2,0,0,0  		;172
 		dc.b 	3,3,3,3,1,0,0,0  		;174
 		dc.b 	3,3,3,3,0,0,0,0  		;176
 		dc.b 	4,4,3,1,1,1,0,0 		;178
 		dc.b 	4,4,3,1,1,0,0,0 		;180
 		dc.b 	4,4,3,1,0,0,0,0 		;182
 		dc.b 	4,4,3,0,0,0,0,0 		;184
 		dc.b 	4,4,2,2,1,0,0,0  		;186
 		dc.b 	4,4,2,2,0,0,0,0  		;188
 		dc.b 	5,4,4,2,0,0,0,0  		;190
 		dc.b 	5,5,4,4,0,0,0,0  		;192
 		dc.b 	5,5,4,2,2,2,0,0  		;194
 		dc.b 	5,5,5,4,2,2,0,0  		;196
 		dc.b 	4,3,3,3,1,1,0,0  		;198
 		dc.b 	4,3,3,3,1,0,0,0  		;200
 		dc.b 	4,3,3,3,0,0,0,0  		;202
 		dc.b 	4,4,4,1,1,1,0,0  		;204
 		dc.b 	4,4,4,1,1,0,0,0  		;206
 		dc.b 	4,4,4,1,0,0,0,0  		;208
 		dc.b 	4,4,4,0,0,0,0,0  		;210
 		dc.b 	4,4,2,2,2,0,0,0  		;212
 		dc.b 	5,4,4,2,2,0,0,0  		;214
 		dc.b 	5,5,4,4,2,0,0,0  		;216
 		dc.b 	5,5,5,4,4,0,0,0  		;218
 		dc.b 	3,3,3,3,3,0,0,0  		;220
 		dc.b 	4,4,3,3,1,1,1,0  		;222
 		dc.b 	4,4,3,3,1,1,0,0  		;224
 		dc.b 	4,4,3,3,1,0,0,0  		;226
 		dc.b 	4,4,3,3,0,0,0,0  		;228
 		dc.b 	4,4,4,2,1,1,0,0  		;230
 		dc.b 	4,4,4,2,1,0,0,0  		;232
 		dc.b 	4,4,4,2,0,0,0,0  		;234
 		dc.b 	5,4,4,4,0,0,0,0  		;236
 		dc.b 	5,4,4,2,2,2,0,0  		;238
 		dc.b 	5,5,4,4,2,2,0,0  		;240
 		dc.b 	5,5,5,4,4,2,0,0  		;242
 		dc.b 	4,3,3,3,3,1,0,0  		;244
 		dc.b 	4,3,3,3,3,0,0,0  		;246
 		dc.b 	4,4,4,3,1,1,1,0  		;248
 		dc.b 	4,4,4,3,1,1,0,0  		;250
 		dc.b 	4,4,4,3,1,0,0,0  		;252
 		dc.b 	4,4,4,3,0,0,0,0  		;254

		even
		
combotable2:
		dc.w	768,  2, 5, 5, 6, 1, 0, 0
		dc.w	768,  5, 5, 1, 6, 5, 0, 0
		dc.w	1024,  3, 3, 3, 5, 4, 0, 0
		dc.w	1024,  1, 1, 5, 1, 5, 2, 0
		dc.w	768,  4, 1, 3, 2, 0, 0, 0
		dc.w	768,  1, 5, 3, 4, 0, 0, 0
		dc.w	512,  6, 5, 4, 6, 0, 0, 0
		dc.w	256,  6, 6, 6, 6, 6, 0, 0
		dc.w	768,  2, 1, 4, 1, 6, 0, 0
		dc.w	768,  2, 6, 5, 3, 1, 0, 0
		dc.w	768,  5, 3, 1, 6, 5, 0, 0
		dc.w	512,  5, 6, 6, 5, 6, 0, 0
		dc.w	768,  2, 5, 2, 6, 2, 0, 0
		dc.w	768,  6, 2, 2, 5, 5, 0, 0
		dc.w	768,  5, 5, 6, 5, 2, 0, 0
		dc.w	1024,  4, 3, 5, 3, 4, 0, 0
		dc.w	768,  6, 6, 3, 4, 3, 6, 0
		dc.w	768,  4, 1, 2, 4, 0, 0, 0
		dc.w	512,  1, 5, 3, 0, 0, 0, 0
		dc.w	512,  6, 2, 6, 6, 3, 0, 0
		dc.w	512,  5, 3, 6, 6, 6, 0, 0
		dc.w	512,  2, 6, 1, 1, 0, 0, 0
		dc.w	512,  5, 1, 1, 6, 0, 0, 0
		dc.w	768,  5, 4, 6, 5, 1, 0, 0
		dc.w	768,  5, 5, 3, 5, 6, 0, 0
		dc.w	768,  1, 1, 1, 5, 1, 0, 0
		dc.w	768,  6, 1, 2, 2, 6, 5, 0
		dc.w	1024,  3, 4, 2, 4, 4, 0, 0
		dc.w	768,  3, 5, 4, 3, 0, 0, 0
		dc.w	512,  6, 3, 3, 6, 6, 0, 0
		dc.w	768,  2, 4, 4, 2, 0, 0, 0
		dc.w	512,  5, 3, 2, 0, 0, 0, 0
		dc.w	768,  1, 4, 5, 6, 3, 0, 0
		dc.w	768,  3, 5, 6, 5, 3, 0, 0
		dc.w	512,  1, 2, 6, 2, 0, 0, 0
		dc.w	512,  6, 5, 1, 2, 0, 0, 0
		dc.w	512,  5, 1, 6, 5, 0, 0, 0
		dc.w	768,  5, 5, 5, 6, 4, 0, 0
		dc.w	768,  1, 5, 1, 2, 1, 0, 0
		dc.w	768,  1, 1, 5, 5, 1, 0, 0
		dc.w	512,  3, 3, 2, 0, 0, 0, 0
		dc.w	512,  3, 3, 5, 0, 0, 0, 0
		dc.w	512,  6, 4, 6, 3, 6, 0, 0
		dc.w	512,  2, 2, 4, 0, 0, 0, 0
		dc.w	512,  2, 5, 4, 0, 0, 0, 0
		dc.w	256,  6, 6, 6, 2, 0, 0, 0
		dc.w	768,  6, 5, 5, 3, 4, 0, 0
		dc.w	768,  1, 3, 1, 1, 2, 0, 0
		dc.w	512,  6, 5, 2, 2, 0, 0, 0
		dc.w	512,  1, 6, 1, 6, 5, 0, 0
		dc.w	768,  2, 2, 1, 2, 1, 0, 0
		dc.w	768,  1, 2, 1, 2, 5, 0, 0
		dc.w	768,  1, 5, 5, 2, 1, 0, 0
		dc.w	512,  2, 4, 3, 0, 0, 0, 0
		dc.w	768,  4, 3, 3, 2, 6, 0, 0
		dc.w	768,  4, 6, 5, 3, 3, 0, 0
		dc.w	256,  2, 2, 0, 0, 0, 0, 0
		dc.w	256,  5, 2, 0, 0, 0, 0, 0
		dc.w	512,  4, 5, 1, 6, 0, 0, 0
		dc.w	512,  6, 5, 3, 5, 0, 0, 0
		dc.w	768,  1, 2, 2, 3, 1, 0, 0
		dc.w	512,  2, 2, 6, 6, 1, 0, 0
		dc.w	768,  1, 1, 5, 5, 3, 0, 0
		dc.w	512,  6, 5, 1, 5, 6, 0, 0
		dc.w	768,  5, 2, 1, 2, 2, 0, 0
		dc.w	768,  1, 2, 2, 5, 5, 0, 0
		dc.w	768,  5, 2, 5, 1, 5, 0, 0
		dc.w	768,  5, 5, 1, 5, 5, 0, 0
		dc.w	256,  4, 6, 6, 6, 0, 0, 0
		dc.w	512,  4, 6, 6, 3, 6, 6, 0
		dc.w	768,  2, 3, 1, 1, 3, 0, 0
		dc.w	512,  4, 5, 6, 2, 0, 0, 0
		dc.w	512,  5, 5, 6, 4, 0, 0, 0
		dc.w	512,  1, 1, 2, 1, 0, 0, 0
		dc.w	768,  1, 2, 5, 3, 2, 0, 0
		dc.w	768,  1, 3, 5, 2, 5, 0, 0
		dc.w	512,  2, 6, 5, 5, 6, 0, 0
		dc.w	768,  4, 3, 3, 6, 4, 0, 0
		dc.w	768,  5, 5, 2, 2, 2, 0, 0
		dc.w	768,  5, 2, 5, 2, 5, 0, 0
		dc.w	256,  6, 3, 1, 0, 0, 0, 0
		dc.w	512,  3, 4, 6, 5, 0, 0, 0
		dc.w	256,  6, 3, 6, 6, 6, 0, 0
		dc.w	512,  1, 1, 1, 3, 0, 0, 0
		dc.w	512,  6, 3, 2, 2, 6, 0, 0
		dc.w	512,  6, 2, 6, 5, 3, 0, 0
		dc.w	512,  6, 5, 5, 6, 3, 0, 0
		dc.w	768,  2, 1, 2, 5, 4, 0, 0
		dc.w	768,  3, 5, 5, 2, 2, 0, 0
		dc.w	768,  5, 5, 1, 4, 5, 0, 0
		dc.w	512,  4, 3, 6, 3, 0, 0, 0
		dc.w	768,  1, 2, 2, 6, 2, 5, 0
		dc.w	0,  2, 0, 0, 0, 0, 0, 0
		dc.w	0,  5, 0, 0, 0, 0, 0, 0
		dc.w	512,  3, 6, 1, 4, 6, 0, 0
		dc.w	512,  6, 6, 3, 5, 3, 0, 0
		dc.w	768,  4, 4, 1, 1, 2, 0, 0
		dc.w	512,  5, 1, 1, 3, 0, 0, 0
		dc.w	768,  5, 1, 5, 4, 3, 0, 0
		dc.w	512,  6, 4, 6, 5, 5, 0, 0
		dc.w	768,  2, 2, 5, 2, 4, 0, 0
		dc.w	512,  2, 5, 1, 5, 0, 0, 0
		dc.w	0,  3, 0, 0, 0, 0, 0, 0
		dc.w	768,  4, 4, 4, 6, 4, 0, 0
		dc.w	512,  3, 6, 3, 6, 3, 0, 0
		dc.w	768,  5, 5, 6, 2, 2, 2, 0
		dc.w	768,  3, 1, 4, 2, 3, 0, 0
		dc.w	512,  2, 6, 4, 6, 3, 0, 0
		dc.w	512,  3, 6, 6, 5, 4, 0, 0
		dc.w	256,  1, 1, 1, 0, 0, 0, 0
		dc.w	256,  2, 6, 6, 2, 0, 0, 0
		dc.w	768,  5, 4, 2, 5, 3, 0, 0
		dc.w	256,  5, 6, 5, 6, 0, 0, 0
		dc.w	512,  5, 2, 2, 2, 0, 0, 0
		dc.w	512,  6, 1, 2, 5, 1, 0, 0
		dc.w	512,  6, 1, 1, 5, 5, 0, 0
		dc.w	256,  3, 6, 4, 0, 0, 0, 0
		dc.w	512,  6, 3, 4, 3, 6, 0, 0
		dc.w	768,  5, 1, 5, 1, 1, 1, 0
		dc.w	512,  1, 3, 2, 3, 0, 0, 0
		dc.w	256,  6, 4, 1, 6, 0, 0, 0
		dc.w	256,  6, 3, 6, 5, 0, 0, 0
		dc.w	768,  2, 2, 4, 2, 4, 0, 0
		dc.w	512,  2, 6, 1, 3, 1, 0, 0
		dc.w	512,  5, 2, 5, 3, 0, 0, 0
		dc.w	256,  6, 5, 1, 6, 6, 0, 0
		dc.w	512,  2, 2, 6, 1, 2, 0, 0
		dc.w	512,  1, 6, 2, 2, 5, 0, 0

sscrl_jumplist:	dc.l	sscrl_line0	
		dc.l	sscrl_line0
		dc.l	sscrl_line0
		dc.l	sscrl_line0
		dc.l	sscrl_line0
		dc.l	sscrl_line0
		dc.l	sscrl_line0
		
*-------------------------------------------------------*
		data
*-------------------------------------------------------*

palette:	dc.w	$0000,$0224,$0422,$0424,$0444,$0557,$0755,$0757
		dc.w	$0000,$0000,$0000,$0000,$0000,$0000,$0000,$0000

stfm_palette:	dc.l	palette

*-------------------------------------------------------*
		bss
*-------------------------------------------------------*

scraddr:	ds.l	1

*-------------------------------------------------------*
		text
*-------------------------------------------------------*
