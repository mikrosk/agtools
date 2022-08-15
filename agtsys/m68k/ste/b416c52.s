*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
* 	PCS v6 blitter palette smasher
*-------------------------------------------------------*

	nop

*-----------------------------------------------------------------------------*
*	wait for first real scanline
*-----------------------------------------------------------------------------*
	lea	$ffff8240.w,a1		; ready palette registers
	move.l	a6,a1
	moveq	#1,d2
*-----------------------------------------------------------------------------*

;	.rept	7
;	mulu.w	d2,d2			; 10*7
;	.endr
;	nop

	cycles	(72-2)-0			; idle time...			; idle time...
	
*-----------------------------------------------------------------------------*
	.rept	display_lines_primary
*-----------------------------------------------------------------------------*

		move.b	d1,(a3)		; + left border
		move.b	d0,(a3)		; - left border

	move.w	d6,(a4)			; BCH: y-count
	.if	(configure_mste=0)
		nop
	.endif
	move.b	d7,(a5)			; BCH: go
		move.b	d0,(a2)		; + right border

	move.l	(a0)+,(a7)+		; CCH: 2

		move.b	d1,(a2)		; - right border
		
	move.l	(a0)+,(a7)+		; CCH: 2+8=10
	move.l	(a0)+,(a7)+
	move.l	(a0)+,(a7)+
	move.l	(a0)+,(a7)+
	move.l	a6,a7

	nop				; pad...
	nop				; pad...
	nop
;	nop
;	lsl.l	#2,d2

*-----------------------------------------------------------------------------*
	.endr	; display_lines_primary
*-----------------------------------------------------------------------------*


*-----------------------------------------------------------------------------*
*	special line #1
*-----------------------------------------------------------------------------*

		move.b	d1,(a3)		; + left border
		move.b	d0,(a3)		; - left border

	move.w	d6,(a4)			; BCH: y-count
	.if	(configure_mste=0)
		nop
	.endif
	move.b	d7,(a5)			; BCH: go
		move.b	d0,(a2)		; + right border

	move.l	(a0)+,(a1)+		; CCH: 2
	
		move.b	d1,(a2)		; - right border
		
	move.l	(a0)+,(a1)+		; CCH: 2+8=10
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+

		move.b	d0,(a2)		; + toggle
		
	move.w	d6,(a4)			; BCH: y-count


*-----------------------------------------------------------------------------*
*	 special line #2
*-----------------------------------------------------------------------------*

		move.b	d1,(a3)		; + left border
		move.b	d0,(a3)		; - left border
		move.b	d1,(a2)		; - toggle
		
	.if	(configure_mste=0)
		nop
	.endif
	move.b	d7,(a5)			; BCH: go
		move.b	d0,(a2)		; + right border

	move.l	(a0)+,(a7)+		; CCH: 2

		move.b	d1,(a2)		; - right border
		
	move.l	(a0)+,(a7)+		; CCH: 2+8=10
	move.l	(a0)+,(a7)+
	move.l	(a0)+,(a7)+
	move.l	(a0)+,(a7)+
	move.l	a6,a7

	nop				; pad...
	nop
	nop	
;	lsl.l	#2,d2

*-----------------------------------------------------------------------------*
	.rept	display_lines_lowborder
*-----------------------------------------------------------------------------*

		move.b	d1,(a3)		; + left border
		move.b	d0,(a3)		; - left border

	move.w	d6,(a4)			; BCH: y-count
	.if	(configure_mste=0)
		nop
	.endif
	move.b	d7,(a5)			; BCH: go
		move.b	d0,(a2)		; + right border

	move.l	(a0)+,(a7)+		; CCH: 2
	
		move.b	d1,(a2)		; - right border
		
	move.l	(a0)+,(a7)+		; CCH: 2+8=10
	move.l	(a0)+,(a7)+
	move.l	(a0)+,(a7)+
	move.l	(a0)+,(a7)+
	move.l	a6,a7

	nop				; pad...
	nop
	nop
;	lsl.l	#2,d2

*-----------------------------------------------------------------------------*
	.endr	; display_lines_lowborder
*-----------------------------------------------------------------------------*

	move.l	d2,(a6)+
	move.l	d2,(a6)+
	move.l	d2,(a6)+
	move.l	d2,(a6)+
	move.l	d2,(a6)+
	move.l	d2,(a6)+
	move.l	d2,(a6)+
	move.l	d2,(a6)+

*-----------------------------------------------------------------------------*
