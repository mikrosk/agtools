;	wwww:ssss
	
sfmc_terminate		macro	
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	move.l		(sp)+,a6
	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
	endm
	
shiftgroup		macro
	dc.w		\1_00_\3-\2,\1_01_\3-\2,\1_03_\3-\2,\1_03_\3-\2,\1_04_\3-\2,\1_05_\3-\2,\1_06_\3-\2,\1_07_\3-\2,\1_08_\3-\2,\1_09_\3-\2,\1_10_\3-\2,\1_11_\3-\2,\1_12_\3-\2,\1_13_\3-\2,\1_14_\3-\2,\1_15_\3-\2
	endm

*-------------------------------------------------------*
*	multiword, no shift
*-------------------------------------------------------*
shiftF0maskC		macro
*-------------------------------------------------------*

	; fetch
	move.l		(\1)+,\3			; ab:CD
	move.l		(\1)+,\4			; ef:GH
	
	; generate mask		
	move.l		\3,\8				;  0a:0C
	or.l		\4,\8				; +0e:0G = 0f:0f
	move.l		\8,\9
	swap		\8
	or.l		\9,\8
	not.l		\8				; f0:f0

	; write
	if		(use_sprite_saverestore)
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\8,\9				; mask dst
	or.l		\3,\9				; emit
	move.l		\9,(\2)+
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\9,\8				; mask dst
	or.l		\4,\8
	move.l		\8,(\2)+			; emit	
	else	
	and.l		\8,(\2)				; mask dst
	or.l		\3,(\2)+			; emit
	and.l		(\2),\8				; mask dst
	or.l		\4,\8
	move.l		\8,(\2)+			; emit
	endc
	
*-------------------------------------------------------*
	endm
*-------------------------------------------------------*

*-------------------------------------------------------*
*	multiword, fixed low shift (0-7)
*-------------------------------------------------------*
shiftFLmaskC		macro
*-------------------------------------------------------*

	; fetch
	move.l		(\1)+,\3			; ab:CD
	move.l		(\1)+,\4			; ef:GH
	
	; shift
	ror.l		#.shift,\3			; Da:bC
	ror.l		#.shift,\4			; He:fG
	
	; rotate & combine state
	swap		\5
	swap		\6
	eor.l		\3,\5				; ^Da:bC
	and.l		\7,\3				; ^D0:b0
	eor.l		\3,\5				; =Wa:XC
	eor.l		\4,\6				; ^He:fG
	and.l		\7,\4				; ^H0:f0
	eor.l		\4,\6				; =Ye:ZG
		
	; generate mask		
	move.l		\5,\8				;  0a:0C
	or.l		\6,\8				; +0e:0G = 0f:0f
	move.l		\8,\9
	swap		\8
	or.l		\9,\8
	not.l		\8				; f0:f0

	; write		
	if		(use_sprite_saverestore)
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\8,\9				; mask dst
	or.l		\5,\9				; emit
	move.l		\9,(\2)+
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\9,\8				; mask dst
	or.l		\6,\8
	move.l		\8,(\2)+			; emit	
	else	
	and.l		\8,(\2)				; mask dst
	or.l		\5,(\2)+			; emit
	and.l		(\2),\8				; mask dst
	or.l		\6,\8
	move.l		\8,(\2)+			; emit
	endc
	
*-------------------------------------------------------*
	endm
*-------------------------------------------------------*

*-------------------------------------------------------*
*	single-word, fixed low shift (0-7)
*-------------------------------------------------------*
shiftFLmaskC_S		macro
*-------------------------------------------------------*

	; fetch
	move.l		(\1)+,\5			; ab:CD
	move.l		(\1)+,\6			; ef:GH

	; shift	
	lsr.l		#.shift,\5			; Da:bC
	lsr.l		#.shift,\6			; He:fG
		
	; generate mask		
	move.l		\5,\8				;  0a:0C
	or.l		\6,\8				; +0e:0G = 0f:0f
	move.l		\8,\9
	swap		\8
	or.l		\9,\8
	not.l		\8				; f0:f0
		
	; write
	if		(use_sprite_saverestore)
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\8,\9				; mask dst
	or.l		\5,\9				; emit
	move.l		\9,(\2)+
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\9,\8				; mask dst
	or.l		\6,\8
	move.l		\8,(\2)+			; emit	
	else	
	and.l		\8,(\2)				; mask dst
	or.l		\5,(\2)+			; emit
	and.l		(\2),\8				; mask dst
	or.l		\6,\8
	move.l		\8,(\2)+			; emit
	endc
	
*-------------------------------------------------------*
	endm
*-------------------------------------------------------*

*-------------------------------------------------------*
*	initial word, fixed low shift (0-7)
*-------------------------------------------------------*
shiftFLmaskC_I		macro
*-------------------------------------------------------*

	; fetch
	move.l		(\1)+,\3			; ab:CD
	move.l		(\1)+,\4			; ef:GH
	
	; shift
	ror.l		#.shift,\3			; Da:bC
	ror.l		#.shift,\4			; He:fG
	
	; initial state
	move.l		\3,\5
	and.l		\7,\3				; ^D0:b0
	eor.l		\3,\5				; =Wa:XC
	move.l		\4,\6
	and.l		\7,\4				; ^H0:f0
	eor.l		\4,\6				; =Ye:ZG

	; generate mask
	move.l		\5,\8				;  0a:0C
	or.l		\6,\8				; +0e:0G = 0f:0f
	move.l		\8,\9
	swap		\8
	or.l		\9,\8
	not.l		\8				; f0:f0
		
	; write
	if		(use_sprite_saverestore)
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\8,\9				; mask dst
	or.l		\5,\9				; emit
	move.l		\9,(\2)+
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\9,\8				; mask dst
	or.l		\6,\8
	move.l		\8,(\2)+			; emit	
	else	
	and.l		\8,(\2)				; mask dst
	or.l		\5,(\2)+			; emit
	and.l		(\2),\8				; mask dst
	or.l		\6,\8
	move.l		\8,(\2)+			; emit
	endc
	
*-------------------------------------------------------*
	endm
*-------------------------------------------------------*

*-------------------------------------------------------*
*	final word, fixed low shift (0-7)
*-------------------------------------------------------*
shiftFLmaskC_F		macro
*-------------------------------------------------------*

	; fetch
	move.l		(\1)+,\3			; ab:CD
	move.l		(\1)+,\4			; ef:GH
	
	; shift
	ror.l		#.shift,\3			; Da:bC
	ror.l		#.shift,\4			; He:fG
	
	; rotate & combine state
	swap		\5
	swap		\6
	eor.l		\3,\5				; ^Da:bC
	and.l		\7,\3				; ^D0:b0
	eor.l		\3,\5				; =Wa:XC
	eor.l		\4,\6				; ^He:fG
	and.l		\7,\4				; ^H0:f0
	eor.l		\4,\6				; =Ye:ZG
	
	; generate mask
	move.l		\5,\8				;  0a:0C
	or.l		\6,\8				; +0e:0G = 0f:0f
	move.l		\8,\9
	swap		\8
	or.l		\9,\8
	not.l		\8				; f0:f0
		
	; write
	if		(use_sprite_saverestore)
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\8,\9				; mask dst
	or.l		\5,\9				; emit
	move.l		\9,(\2)+
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\9,\8				; mask dst
	or.l		\6,\8
	move.l		\8,(\2)+			; emit	
	else	
	and.l		\8,(\2)				; mask dst
	or.l		\5,(\2)+			; emit
	and.l		(\2),\8				; mask dst
	or.l		\6,\8
	move.l		\8,(\2)+			; emit
	endc
	
*-------------------------------------------------------*
	endm
*-------------------------------------------------------*

*-------------------------------------------------------*
*	final NFSR word, fixed low shift (0-7)
*-------------------------------------------------------*
shiftFLmaskC_N		macro
*-------------------------------------------------------*

	; rotate state
	swap		\5
	swap		\6
		
	; generate mask
	move.l		\5,\8				;  0a:0C
	or.l		\6,\8				; +0e:0G = 0f:0f
	move.l		\8,\9
	swap		\8
	or.l		\9,\8
	not.l		\8				; f0:f0
		
	; write
	if		(use_sprite_saverestore)
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\8,\9				; mask dst
	or.l		\5,\9				; emit
	move.l		\9,(\2)+
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\9,\8				; mask dst
	or.l		\6,\8
	move.l		\8,(\2)+			; emit	
	else	
	and.l		\8,(\2)				; mask dst
	or.l		\5,(\2)+			; emit
	and.l		(\2),\8				; mask dst
	or.l		\6,\8
	move.l		\8,(\2)+			; emit
	endc
	
*-------------------------------------------------------*
	endm
*-------------------------------------------------------*


	

*-------------------------------------------------------*
*	multiword, fixed hi shift (8-15)
*-------------------------------------------------------*
shiftFHmaskC		macro
*-------------------------------------------------------*

	; fetch
	move.l		(\1)+,\3			; ab:CD
	move.l		(\1)+,\4			; ef:GH
	
	; shift
	rol.l		#16-.shift,\3			; bC:Da
	rol.l		#16-.shift,\4			; fG:He
	swap		\3				; Da:bC
	swap		\4				; He:fG

	; rotate & combine state
	swap		\5
	swap		\6		
	eor.l		\3,\5				; ^Da:bC
	and.l		\7,\3				; ^D0:b0
	eor.l		\3,\5				; =Wa:XC
	eor.l		\4,\6				; ^He:fG
	and.l		\7,\4				; ^H0:f0
	eor.l		\4,\6				; =Ye:ZG
		
	; generate mask
	move.l		\5,\8				;  0a:0C
	or.l		\6,\8				; +0e:0G = 0f:0f
	move.l		\8,\9
	swap		\8
	or.l		\9,\8
	not.l		\8				; f0:f0
	
	; write	
	if		(use_sprite_saverestore)
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\8,\9				; mask dst
	or.l		\5,\9				; emit
	move.l		\9,(\2)+
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\9,\8				; mask dst
	or.l		\6,\8
	move.l		\8,(\2)+			; emit	
	else	
	and.l		\8,(\2)				; mask dst
	or.l		\5,(\2)+			; emit
	and.l		(\2),\8				; mask dst
	or.l		\6,\8
	move.l		\8,(\2)+			; emit
	endc
	
*-------------------------------------------------------*
	endm
*-------------------------------------------------------*

*-------------------------------------------------------*
*	single-word, fixed hi shift (8-15)
*-------------------------------------------------------*
shiftFHmaskC_S		macro
*-------------------------------------------------------*

	; fetch
	move.l		(\1)+,\5			; ab:CD
	move.l		(\1)+,\6			; ef:GH
	
	; shift
	rol.l		#16-.shift,\5			; bC:Da
	rol.l		#16-.shift,\6			; fG:He
	swap		\5				; Da:bC
	swap		\6				; He:fG
		
	; generate mask	
	move.l		\5,\8				;  0a:0C
	or.l		\6,\8				; +0e:0G = 0f:0f
	move.l		\8,\9
	swap		\8
	or.l		\9,\8
	not.l		\8				; f0:f0
		
	; write
	if		(use_sprite_saverestore)
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\8,\9				; mask dst
	or.l		\5,\9				; emit
	move.l		\9,(\2)+
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\9,\8				; mask dst
	or.l		\6,\8
	move.l		\8,(\2)+			; emit	
	else	
	and.l		\8,(\2)				; mask dst
	or.l		\5,(\2)+			; emit
	and.l		(\2),\8				; mask dst
	or.l		\6,\8
	move.l		\8,(\2)+			; emit
	endc
	
*-------------------------------------------------------*
	endm
*-------------------------------------------------------*

*-------------------------------------------------------*
*	initial word, fixed hi shift (8-15)
*-------------------------------------------------------*
shiftFHmaskC_I		macro
*-------------------------------------------------------*

	; fetch
	move.l		(\1)+,\3			; ab:CD
	move.l		(\1)+,\4			; ef:GH
	
	; shift
	rol.l		#16-.shift,\3			; bC:Da
	rol.l		#16-.shift,\4			; fG:He
	swap		\3				; Da:bC
	swap		\4				; He:fG
	
	; init state
	move.l		\3,\5				; ^Da:bC
	and.l		\7,\3				; ^D0:b0
	eor.l		\3,\5				; =Wa:XC
	move.l		\4,\6				; ^He:fG
	and.l		\7,\4				; ^H0:f0
	eor.l		\4,\6				; =Ye:ZG

	; generate mask
	move.l		\5,\8				;  0a:0C
	or.l		\6,\8				; +0e:0G = 0f:0f
	move.l		\8,\9
	swap		\8
	or.l		\9,\8
	not.l		\8				; f0:f0
	
	; write	
	if		(use_sprite_saverestore)
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\8,\9				; mask dst
	or.l		\5,\9				; emit
	move.l		\9,(\2)+
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\9,\8				; mask dst
	or.l		\6,\8
	move.l		\8,(\2)+			; emit	
	else	
	and.l		\8,(\2)				; mask dst
	or.l		\5,(\2)+			; emit
	and.l		(\2),\8				; mask dst
	or.l		\6,\8
	move.l		\8,(\2)+			; emit
	endc
	
*-------------------------------------------------------*
	endm
*-------------------------------------------------------*

*-------------------------------------------------------*
*	final word, fixed hi shift (8-15)
*-------------------------------------------------------*
shiftFHmaskC_F		macro
*-------------------------------------------------------*

	; fetch
	move.l		(\1)+,\3			; ab:CD
	move.l		(\1)+,\4			; ef:GH
	
	; shift
	rol.l		#16-.shift,\3			; Da:bC
	rol.l		#16-.shift,\4			; He:fG

	; rotate & combine state
	eor.l		\3,\5				; ^Da:bC
	and.l		\7,\3				; ^D0:b0
	eor.l		\3,\5				; =Wa:XC
	eor.l		\4,\6				; ^He:fG
	and.l		\7,\4				; ^H0:f0
	eor.l		\4,\6				; =Ye:ZG
	swap		\5
	swap		\6
	
	; generate mask
	move.l		\5,\8				;  0a:0C
	or.l		\6,\8				; +0e:0G = 0f:0f
	move.l		\8,\9
	swap		\8
	or.l		\9,\8
	not.l		\8				; f0:f0
		
	; write
	if		(use_sprite_saverestore)
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\8,\9				; mask dst
	or.l		\5,\9				; emit
	move.l		\9,(\2)+
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\9,\8				; mask dst
	or.l		\6,\8
	move.l		\8,(\2)+			; emit	
	else	
	and.l		\8,(\2)				; mask dst
	or.l		\5,(\2)+			; emit
	and.l		(\2),\8				; mask dst
	or.l		\6,\8
	move.l		\8,(\2)+			; emit
	endc
	
*-------------------------------------------------------*
	endm
*-------------------------------------------------------*

*-------------------------------------------------------*
*	final NFSR word, fixed hi shift (8-15)
*-------------------------------------------------------*
shiftFHmaskC_N		macro
*-------------------------------------------------------*

	; rotate state
	swap		\5
	swap		\6
		
	; generate mask
	move.l		\5,\8				;  0a:0C
	or.l		\6,\8				; +0e:0G = 0f:0f
	move.l		\8,\9
	swap		\8
	or.l		\9,\8
	not.l		\8				; f0:f0
		
	; write
	if		(use_sprite_saverestore)
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\8,\9				; mask dst
	or.l		\5,\9				; emit
	move.l		\9,(\2)+
	move.l		(\2),\9
	move.l		\9,(a5)+
	and.l		\9,\8				; mask dst
	or.l		\6,\8
	move.l		\8,(\2)+			; emit	
	else	
	and.l		\8,(\2)				; mask dst
	or.l		\5,(\2)+			; emit
	and.l		(\2),\8				; mask dst
	or.l		\6,\8
	move.l		\8,(\2)+			; emit
	endc
	
*-------------------------------------------------------*
	endm
*-------------------------------------------------------*
