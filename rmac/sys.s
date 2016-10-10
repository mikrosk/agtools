*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
*	System services
*-------------------------------------------------------*

timera		=	$134
timerb		=	$120
timerc		=	$114
timerd		=	$110

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

cookie_jar	=	$5a0

VBServiceVec	=	$a0
DMAFlag		=	$a4

			.if	^^defined AGT_CONFIG_AUDIO_200HZ
configure_50hz		=	0
			.else
configure_50hz		=	1
			.endif

	.globl	_bSTF	
	.globl	_bSTE	
	.globl	_bMegaSTE
	.globl	_bFalcon030
	.globl	_bTT030	

*-------------------------------------------------------*

	.globl	_SYS_Configure
	.globl	_SYS_Save
	.globl	_SYS_Claim
	.globl	_SYS_Restore
	
	.globl	_ST_ResetInput
	.globl	_ST_DisableInput
	
	.globl	_YMSYS_Callback
	.globl	_YMSYS_VBCallback

	.globl		vbl_sema
	
	.globl		_blackscreen

*-------------------------------------------------------*
	.text
*-------------------------------------------------------*

*-------------------------------------------------------*
_SYS_Configure:
*-------------------------------------------------------*
			.abs
*-------------------------------------------------------*
.savea6:		ds.l	1
.save:			ds.l	10
.return:		ds.l	1
*-------------------------------------------------------*
.frame_:
*-------------------------------------------------------*
			.text
*-------------------------------------------------------*
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
*-------------------------------------------------------*
;	todo: get machine type etc.

	move.w		$464.w,d0
.wvb0:	cmp.w		$464.w,d0
	beq.s		.wvb0

;	or.w		#$0700,sr
;	move.b		$ffff8265.w,d0
;	not.b		$ffff8265.w
;	cmp.b		$ffff8265.w,d0
;	sne		bSTE
;	move.b		d0,$ffff8265.w
;	and.w		#$fbff,sr
*-------------------------------------------------------*
*	detect machine type
*-------------------------------------------------------*

	move.l		#'_MCH',d0
	bsr		cookie_search
	tst.l		d0
	sne		_bSTF
	bne		.dunno
	move.l		4(a0),d0
	seq		_bSTF

	move.l		#$ffff0000,d1
	and.l		d0,d1
	cmp.l		#$00010000,d1
	seq		_bSTE
	cmp.l		#$00010010,d0
	seq		_bMegaSTE
	
	cmp.l		#$00020000,d0
	seq		_bTT030
	cmp.l		#$00030000,d0
	seq		_bFalcon030
		
;	tst.b		_bFalcon030
;	bne		*

;	tst.b		bFalcon030
;	beq.s		.not_f030
;	move.b		#%00000000,ffff8007.w
;.not_f030:

;	move.l		8.w,a3
;	move.l		#.cont,8.w
;        move.l		sp,a2
;	clr.b		$ffff8e21.w
;.cont:  move.l		a2,sp
;        move.l		a3,8.w


	tst.b		_bFalcon030
	beq.s		.nf030
	moveq		#0,d0
;	movec		d0,cacr
.nf030:
*-------------------------------------------------------*
*	8mhz switch down MegaSTE
*-------------------------------------------------------*
	tst.b		_bMegaSTE
	beq.s		.not_mste
	clr.b		$ffff8e21.w
.not_mste:

.dunno:
*-------------------------------------------------------*
	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts

*-------------------------------------------------------*
_SYS_Save:
*-------------------------------------------------------*
			.abs
*-------------------------------------------------------*
.savea6:		ds.l	1
.save:			ds.l	10
.return:		ds.l	1
*-------------------------------------------------------*
.frame_:
*-------------------------------------------------------*
			.text
*-------------------------------------------------------*
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
*-------------------------------------------------------*
	move.w		#$2700,sr
*-------------------------------------------------------*
	lea		savestate_store,a5
*-------------------------------------------------------*
*	display basics
*-------------------------------------------------------*
 	move.w		#3,-(sp)			; log
	trap		#14
	move.l		d0,-(a5)
 	move.w		#2,-(sp)			; phys
	trap		#14
	move.l		d0,-(a5)
	addq.l    	#4,sp
	
;	todo: save/restore palette
	
	tst.b		_bSTF
	bne.s		.nste
	
;	todo: save/restore blitter state
;	
;	reg16(ffff8a02) = *psave++;
;	reg16(ffff8a04) = *psave++;
;	reg16(ffff8a06) = *psave++;
;	reg16(ffff8a08) = *psave++;
;	reg16(ffff8a0a) = *psave++;
;	reg16(ffff8a0c) = *psave++;
;	reg16(ffff8a0e) = *psave++;
;	reg16(ffff8a10) = *psave++;
;	reg16(ffff8a12) = *psave++;
;	reg16(ffff8a14) = *psave++;
;	reg16(ffff8a16) = *psave++;
;	reg16(ffff8a18) = *psave++;
;	reg16(ffff8a1a) = *psave++;
;	reg16(ffff8a1c) = *psave++;
;	reg16(ffff8a1e) = *psave++;
;
;	reg16(ffff8a20) = *psave++;
;	reg16(ffff8a22) = *psave++;
;	reg16(ffff8a24) = *psave++;
;	reg16(ffff8a26) = *psave++;
;	reg16(ffff8a28) = *psave++;
;	reg16(ffff8a2a) = *psave++;
;	reg16(ffff8a2c) = *psave++;
;	reg16(ffff8a2e) = *psave++;
;	reg16(ffff8a30) = *psave++;
;	reg16(ffff8a32) = *psave++;
;	reg16(ffff8a34) = *psave++;
;	reg16(ffff8a36) = *psave++;
;	reg16(ffff8a38) = *psave++;
;	reg16(ffff8a3a) = *psave++;	
	
	move.b		$ffff8265.w,-(a5)
	move.b		$ffff820e.w,-(a5)
.nste:	move.b		$ffff820a.w,-(a5)
	move.b		$ffff8260.w,-(a5)
*-------------------------------------------------------*
*	vectors
*-------------------------------------------------------*
	lea		$140.w,a0
	moveq		#(($140-$8)>>2)-1,d0
.vecs:	move.l		-(a0),-(a5)
	dbra		d0,.vecs
*-------------------------------------------------------*
*	MFP state
*-------------------------------------------------------*
	move.b		$fffffa07.w,-(a5)
	move.b		$fffffa13.w,-(a5)
	move.b		$fffffa09.w,-(a5)
	move.b		$fffffa15.w,-(a5)
*-------------------------------------------------------*
	move.w		#$2300,sr
*-------------------------------------------------------*
	cmp.l		#savestate_restore,a5
.lock:	bne.s		.lock
*-------------------------------------------------------*	
	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts

*-------------------------------------------------------*
_SYS_Claim:
*-------------------------------------------------------*
			.abs
*-------------------------------------------------------*
.savea6:		ds.l	1
.save:			ds.l	10
.return:		ds.l	1
*-------------------------------------------------------*
.frame_:
*-------------------------------------------------------*
			.text
*-------------------------------------------------------*
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
*-------------------------------------------------------*
	move.w		#$2700,sr
*-------------------------------------------------------*
*	Halt all input reporting via ACIA
*-------------------------------------------------------*
;	move.b		#$12,$fffffc02.w		; stop mouse reporting
	jsr		_ST_DisableInput

	clr.l		VBServiceVec.w
	clr.b		DMAFlag.w

*-------------------------------------------------------*
*	Mega STE
*-------------------------------------------------------*

; from Paranoid/Paradox:
;
;  ! The Mega STE has a register to control speed and cache
;    status:
;     $FFFF8E21  - - - -  - - X X
;     
;     Bit 0 controls the clockspeed (0 = 8 MHz, 1 = 16 MHz),
;     the upper bit controls the cache (0 = Cache off, 1 = cache on).
;     Some docs say that all upper 15 bits of $FFFF8E20 need to
;     be set to "1" to turn the cache on.
;     Writing to this register in anything but a Mega STE will
;     most probably lead to a crash, so be sure to check the
;     Cookie Jar for _MCH to estimate wether this is a Mega STE
;     or not (Upper word is 1 for STE, lower word is 0x0010 for
;     Mega STE, 0x0000 for anything else).	


*-------------------------------------------------------*
*	display basics
*-------------------------------------------------------*

	clr.b		$ffff8260.w
	move.b		#2,$ffff820a.w
	
;	move.w		#-1,-(sp)
;	move.l		#256,-(sp)			; phys
;	move.l		#256,-(sp)			; log
;	move.w		#5,-(sp)
;	trap		#14
;	lea		12(sp),sp

	jsr		_blackscreen

*-------------------------------------------------------*
*	vectors
*-------------------------------------------------------*
	lea		drte,a0
	move.l		a0,$68.w			; HBL
	move.l		#dummy_vbl,$70.w		; VBL
;	move.l		#dvbl,$70.w
	if		(configure_50hz)
	else
	move.l		#ym_td,$110.w			; TimerD
	endif
;	move.l		#dummy_tc,$114.w		; TimerC
	move.l		a0,$118.w			; IKBD/MIDI ACIA
	move.l		a0,$120.w			; TimerB
	move.l		a0,$134.w			; TimerA
	move.l		a0,$13c.w			; GPI7

;	move.l		$11c.w,loader_reroute+2
;	move.l		#loader_rerouter,$11c.w
		
*-------------------------------------------------------*
*	MFP state
*-------------------------------------------------------*
	clr.b		$fffffa07.w			; IEA (Timer-A & B)
	clr.b		$fffffa13.w			; IMA (Timer-A & B)
	clr.b		$fffffa09.w			; IEB (Timer-C & D)
	clr.b		$fffffa15.w			; IMB (Timer-C & D)
*-------------------------------------------------------*
	move.w		#$2300,sr
*-------------------------------------------------------*	
	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts

drte:	rte

;loader_rerouter:
;	ori.w		#$0700,sr
;	andi.w		#$fbff,sr
;loader_reroute:
;	jmp		$12345678

	
dvbl:	movem.l		d0-a6,-(sp)
;	move.w		$ffff8240.w,-(sp)
		

;	bra		*

	addq.w		#1,vbl_sema
	bgt.s		.nsvc
	tst.w		VBServiceVec.w
	beq.s		.nsvc

	
;	move.w		#$770,$ffff8240.w
	

	
	move.l		VBServiceVec.w,a0
	jsr		(a0)

.nsvc:	subq.w		#1,vbl_sema

;	move.w		(sp)+,$ffff8240.w
	movem.l		(sp)+,d0-a6

dummy_vbl:
	addq.l		#1,$462.w
	addq.l		#1,$466.w
	rte
	
	
dummy_tc:
	addq.l		#1,$4ba.w
	bclr		#5,isrb.w
	rte
	
ym_td:
;	move.w		$ffff8240.w,-(sp)
;	move.w		#-1,$ffff8240.w
	
	bclr		#4,isrb.w
;	ori.w		#$0700,sr
	andi.w		#$fbff,sr
		
	movem.l		d0-a6,-(sp)
	jsr		_YMSYS_Callback
	movem.l		(sp)+,d0-a6

	;and.b		#$f0,cdcr.w
	
;	move.w		(sp)+,$ffff8240.w
	rte


*-------------------------------------------------------*
_SYS_Restore:
*-------------------------------------------------------*
			.abs
*-------------------------------------------------------*
.savea6:		ds.l	1
.save:			ds.l	10
.return:		ds.l	1
*-------------------------------------------------------*
.frame_:
*-------------------------------------------------------*
			.text
*-------------------------------------------------------*
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
*-------------------------------------------------------*
	move.w		#$2700,sr
	lea		savestate_restore,a5
*-------------------------------------------------------*
*	MFP state
*-------------------------------------------------------*
	move.b		(a5)+,$fffffa15.w
	move.b		(a5)+,$fffffa09.w
	move.b		(a5)+,$fffffa13.w
	move.b		(a5)+,$fffffa07.w
*-------------------------------------------------------*
*	vectors
*-------------------------------------------------------*
	lea		$8.w,a0
	moveq		#(($140-$8)>>2)-1,d0
.vecs:	move.l		(a5)+,(a0)+
	dbra		d0,.vecs
*-------------------------------------------------------*
*	display basics
*-------------------------------------------------------*
	move.b		(a5)+,$ffff8260.w
	move.b		(a5)+,$ffff820a.w
	tst.b		_bSTF
	bne.s		.nste
	move.b		(a5)+,$ffff820e.w
	move.b		(a5)+,$ffff8265.w
.nste:	move.w		#-1,-(sp)
	move.l		(a5)+,-(sp)			; phys
	move.l		(a5)+,-(sp)			; log
	move.w		#5,-(sp)
	trap		#14
	lea		12(sp),sp		
*-------------------------------------------------------*
*	Resume input reporting via ACIA
*-------------------------------------------------------*
	jsr		_ST_ResetInput
*-------------------------------------------------------*
	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts

*-------------------------------------------------------*
_blackscreen:
*-------------------------------------------------------*
	moveq		#0,d0
	moveq		#0,d1
	moveq		#0,d2
	moveq		#0,d3
	moveq		#0,d4
	moveq		#0,d5
	moveq		#0,d6
	moveq		#0,d7
	movem.l		d0-d7,$ffff8240.w
	rts
		

*---------------------------------------------------------------*
cookie_search:
*---------------------------------------------------------------*
	move.l		cookie_jar.w,a0
.loop:	move.l		(a0),d1
	beq.s		.fail
	cmp.l		d0,d1
	beq.s		.find
	addq.l		#8,a0
	bra.s		.loop
.find:	moveq		#0,d0
	bra.s		.end
.fail:	moveq		#-1,d0
.end:	rts

*-------------------------------------------------------*
	.data
*-------------------------------------------------------*

vbl_sema:		dc.w	-1
	
*-------------------------------------------------------*
	.bss
*-------------------------------------------------------*

savestate_restore:
			ds.b	4			; MFP
			ds.l	(($140-$8)>>2)		; vectors
			ds.l	1+1+1			; display
savestate_store:
			;ds.b	256
_bSTF:			ds.b	1	
_bSTE:			ds.b	1	
_bMegaSTE:		ds.b	1		
_bFalcon030:		ds.b	1		
_bTT030:		ds.b	1	
			even
						
*-------------------------------------------------------*
	.text
*-------------------------------------------------------*

