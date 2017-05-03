
dbraload		macro
	move.w		#(\1-\2)-2,\3
	endm

dbradef			macro
	dc.\0		(\1-\2)-2
	endm

dbramoda		macro
	move.w		#(\1-\2)-2,\2+2
	endm
