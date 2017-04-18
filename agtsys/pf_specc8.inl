//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
//	playfield region update implementation
//----------------------------------------------------------------------------------------------------------------------

inline void PQRS8_fill_speccolumn_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pj2, s16 pjy1, s16 pjy2)
{						
}

inline void PQ8_fill_speccolumn_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pj2, s16 pjy1, s16 pjy2)
{						
}

inline void PR8_fill_speccolumn_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pj2, s16 pjy1, s16 pjy2)
{						
	// start mi,mj modulo map dimensions
	
	if ((pj == pj2) || (pjy1 == pjy2))
		return;

	s16 mi = modulo_map_x(_mi);
	s16 mj = modulo_map_y(_mj);
	//const s32* pmap = &map_[mi + (mj * map_tiles_x_)];	
	const s32* pmap = &map_[mi + mul_map_x(mj)];	
	
	const u16* ptiles = &tiledata_[pjy1 << 1];	// c_tilesize_
	//u16* dstaddr = &framebuffer_[(pj * rowskipwords_) + (pjy1 * virtual_linewords_) + (pi * c_tilelinewords_)];
	u16* dstaddr = &framebuffer_[(pj * rowskipwords_) + (pjy1 * virtual_linewords_) + ((pi & -2) * 2)];
		
	// calculate number of word-based skips
	s16 dstrowskip = (rowskipwords_ << 1);

	__asm__ __volatile__ (
#ifdef TIMING_RASTERS
"											\
		move.w	#0xf00,0xffff8240.w;		\
		move.w	#0xf00,0x64.w;				\
"										
#endif
"											\
		move.l	%12,%%a4;				 	\
		move.l	%11,%%a2;				 	\
		move.l	%10,%%d1;				 	\
		move.l	%17,%%d6;					\
		moveq	#0,%%d2;					\
		move.w	%8,%%d2;					\
		lsl.l	#2,%%d2;					\
											\
		moveq	#8,%%d3;					\
		add.w	%0,%%d3;					\
		sub.w	%1,%%d3;					\
		move.w	%%d3,%%d0;					\
		add.w	%%d3,%%d3;					\
		add.w	%%d0,%%d3;					\
		lsl.w	#2,%%d3;					\
											\
		move.w	%5,%%d4; 					\
		sub.w	%3,%%d4; 					\
											\
		moveq	#1,%%d0;					\
		and.w	%2,%%d0;					\
		add.w	%%d0,%%a4;					\
											\
		lea		yle%=(%%pc,%%d3.w),%%a0;	\
		lea		2+smce%=(%%pc),%%a5;		\
		sub.l	%%a5,%%a0;					\
		move.w	%%a0,(%%a5);				\
											\
		move.l	%%a1,%%usp;					\
		bra		syle%=;						\
											\
yle%=:	move.l	7*4(%%a5),%%d0;				\
		movep.l	%%d0,7*184(%%a3);			\
		movep.l	%%d0,7*184(%%a1);			\
		move.l	6*4(%%a5),%%d0;				\
		movep.l	%%d0,6*184(%%a3);			\
		movep.l	%%d0,6*184(%%a1);			\
		move.l	5*4(%%a5),%%d0;				\
		movep.l	%%d0,5*184(%%a3);			\
		movep.l	%%d0,5*184(%%a1);			\
		move.l	4*4(%%a5),%%d0;				\
		movep.l	%%d0,4*184(%%a3);			\
		movep.l	%%d0,4*184(%%a1);			\
		move.l	3*4(%%a5),%%d0;				\
		movep.l	%%d0,3*184(%%a3);			\
		movep.l	%%d0,3*184(%%a1);			\
		move.l	2*4(%%a5),%%d0;				\
		movep.l	%%d0,2*184(%%a3);			\
		movep.l	%%d0,2*184(%%a1);			\
		move.l	1*4(%%a5),%%d0;				\
		movep.l	%%d0,1*184(%%a3);			\
		movep.l	%%d0,1*184(%%a1);			\
		move.l	0*4(%%a5),%%d0;				\
		movep.l	%%d0,0*184(%%a3);			\
		movep.l	%%d0,0*184(%%a1);			\
											\
		adda.w	%13,%%a4;					\
											\
syle%=:	move.l	%%d1,%%a0;					\
		add.l	%%d2,%%d1;					\
											\
		move.l	%%a2,%%a5;					\
		add.l	(%%a0)+,%%a5;				\
											\
		move.l	%%a4,%%a3;					\
		move.l	%%a4,%%a1;					\
		add.l	%%d6,%%a1;					\
											\
smce%=:	dbra	%%d4,yle%=;					\
											\
		move.l	%%usp,%%a1;					\
"
#ifdef TIMING_RASTERS
"											\
		move.w	#0x000,0xffff8240.w;		\
		move.w	#0x000,0x64.w;				\
"										
#endif
     	: 
		: "m"(pjy1), "m"(pjy2), 
		  "m"(pi), "m"(pj), 
		  "m"(pi), "m"(pj2),
		  "m"(mi), "m"(mj), 
		  "m"(map_tiles_x_), "m"(map_tiles_y_), 
		  "m"(pmap), "m"(ptiles), "m"(dstaddr), "m"(dstrowskip),
		  "m"(srcywrap_), "m"(virtual_linebytes_), "m"(visible_linebytes_), "m"(virtual_pagebytes_)
		: "%%d0", "%%d1", "%%d2", "%%d3", "%%d4", "%%d5", "%%d6", "%%d7",
		  "%%a0", "%%a2", "%%a3", "%%a4", "%%a5",
		  "cc"
	);
}

inline void P8_fill_speccolumn_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pj2, s16 pjy1, s16 pjy2)
{						
}
