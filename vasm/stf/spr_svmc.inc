
*-------------------------------------------------------*
*	multiword, variable shift
*-------------------------------------------------------*
shiftVMC		macro
*-------------------------------------------------------*

	; fetch
	move.l		(\1)+,\3			; ab:CD
	move.l		(\1)+,\4			; ef:GH
	
	; shift
	ror.l		d7,\3				; Da:bC
	ror.l		d7,\4				; He:fG
	
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
*	multiword FISR case, variable shift
*-------------------------------------------------------*
shiftVMC2		macro
*-------------------------------------------------------*

	; fetch
	move.l		(\1)+,\3			; ab:CD
	move.l		(\1)+,\4			; ef:GH
	
	; shift
	ror.l		d7,\3				; Da:bC
	ror.l		d7,\4				; He:fG
	
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

	; alternate state manually
	move.l		\3,\5				; Da:bC
	move.l		\4,\6				; He:fG
	
*-------------------------------------------------------*
	endm
*-------------------------------------------------------*

*-------------------------------------------------------*
*	single-word, variable shift
*-------------------------------------------------------*
shiftVMC_S		macro
*-------------------------------------------------------*

	; fetch
	move.l		(\1)+,\5			; ab:CD
	move.l		(\1)+,\6			; ef:GH
	
	; shift
	lsr.l		d7,\5				; Da:bC
	lsr.l		d7,\6				; He:fG	
		
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
*	initial word, variable shift
*-------------------------------------------------------*
shiftVMC_I		macro
*-------------------------------------------------------*

	; fetch
	move.l		(\1)+,\3			; ab:CD
	move.l		(\1)+,\4			; ef:GH
	
	; shift
	ror.l		#.shift,\3			; Da:bC
	ror.l		#.shift,\4			; He:fG
		
	; init state
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
*	final word, variable shift
*-------------------------------------------------------*
shiftVMC_F		macro
*-------------------------------------------------------*

	; fetch
	move.l		(\1)+,\3			; ab:CD
	move.l		(\1)+,\4			; ef:GH
	
	; shift
	ror.l		d7,\3				; Da:bC
	ror.l		d7,\4				; He:fG

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
*	final NFSR word, variable shift
*-------------------------------------------------------*
shiftVMC_N		macro
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
*	initial FISR fetch, variable shift
*-------------------------------------------------------*
shiftVMC_FISR		macro
*-------------------------------------------------------*

	; fetch
	move.l		(\1)+,\5			; ab:CD
	move.l		(\1)+,\6			; ef:GH
	
	; shift
	ror.l		d7,\5				; Da:bC
	ror.l		d7,\6				; He:fG

	; init state
	and.l		\7,\5				; D0:b0
	and.l		\7,\6				; H0:f0
	
*-------------------------------------------------------*
	endm
*-------------------------------------------------------*
