//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
//	playfield region update implementation
//----------------------------------------------------------------------------------------------------------------------

inline void PQRS8_fill_specrow_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pjy1, s16 pjy2)
{
}

inline void PQ8_fill_specrow_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pjy1, s16 pjy2)
{
}

inline void PR8_fill_specrow_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pjy1, s16 pjy2)
{
	// start mi,mj modulo map dimensions

	if ((pi == pi2) || (pjy1 == pjy2))
		return;

	s16 mi = modulo_map_x(_mi);
	s16 mj = modulo_map_y(_mj);
	//const s32* pmap = &map_[mi + (mj * map_tiles_x_)];
	const s32* pmap = &map_[mi + mul_map_x(mj)];

	const u16* ptiles = &tiledata_[pjy1 << 1];

//	u16* dstaddr = &framebuffer_[(pj * rowskipwords_) + (pjy1 * virtual_linewords_) + (pi * c_tilelinewords_)];
	u16* dstaddr = &framebuffer_[(pj * rowskipwords_) + (pjy1 * virtual_linewords_) + ((pi & -2) * 2)];

	s16 dstrowskip = (rowskipwords_ << 1);// - ((pi2 - pi) << 3);


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
		move.w	%4,%%d5; 					\
		sub.w	%2,%%d5;		 			\
											\
		moveq	#8,%%d4;					\
		add.w	%0,%%d4;					\
		sub.w	%1,%%d4;					\
		move.w	%%d4,%%d0;					\
		add.w	%%d4,%%d4;					\
		add.w	%%d0,%%d4;					\
		lsl.w	#2,%%d4;					\
											\
		moveq	#1,%%d7;					\
		and.w	%2,%%d7;					\
		add.w	%%d7,a4;					\
		move.b	.tbl%=(%%pc,%%d7.w),%%d7;	\
											\
		lea		xl%=(%%pc,%%d4.w),%%a0;		\
		lea		2+smc%=(%%pc),%%a5;			\
		sub.l	%%a5,%%a0;					\
		move.w	%%a0,(%%a5);				\
											\
		move.w	%%d5,%%d3; 					\
		move.l	%%d1,%%a0;					\
		add.l	%%d2,%%d1;					\
											\
		move.l	%%a1,%%usp;					\
		bra		sxl%=; 						\
											\
.tbl%=:	dc.b	1;							\
		dc.b	7;							\
											\
xl%=:	move.l	4*7(%%a5),%%d0;				\
		movep.l	%%d0,184*7(%%a3);			\
		movep.l	%%d0,184*7(%%a1);			\
		move.l	4*6(%%a5),%%d0;				\
		movep.l	%%d0,184*6(%%a3);			\
		movep.l	%%d0,184*6(%%a1);			\
		move.l	4*5(%%a5),%%d0;				\
		movep.l	%%d0,184*5(%%a3);			\
		movep.l	%%d0,184*5(%%a1);			\
		move.l	4*4(%%a5),%%d0;				\
		movep.l	%%d0,184*4(%%a3);			\
		movep.l	%%d0,184*4(%%a1);			\
		move.l	4*3(%%a5),%%d0;				\
		movep.l	%%d0,184*3(%%a3);			\
		movep.l	%%d0,184*3(%%a1);			\
		move.l	4*2(%%a5),%%d0;				\
		movep.l	%%d0,184*2(%%a3);			\
		movep.l	%%d0,184*2(%%a1);			\
		move.l	4*1(%%a5),%%d0;				\
		movep.l	%%d0,184*1(%%a3);			\
		movep.l	%%d0,184*1(%%a1);			\
		move.l	4*0(%%a5),%%d0;				\
		movep.l	%%d0,184*0(%%a3);			\
		movep.l	%%d0,184*0(%%a1);			\
jt1%=:										\
											\
sxl%=:	move.l	%%a2,%%a5;					\
		add.l	(%%a0)+,%%a5;				\
											\
		move.l	%%a4,%%a3;					\
		move.l	%%a4,%%a1;					\
		add.l	%%d6,%%a1;					\
											\
		add.w	%%d7,%%a4;					\
		eor.w	#6,%%d7;					\
											\
smc%=:	dbra	%%d3,xl%=; 					\
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
		  "m"(pi2), "m"(pjy1),
		  "m"(mi), "m"(mj),
		  "m"(map_tiles_x_), "m"(map_tiles_y_),
		  "m"(pmap), "m"(ptiles), "m"(dstaddr), "m"(dstrowskip),
		  "m"(srcywrap_), "m"(virtual_linebytes_), "m"(visible_linebytes_), "m"(virtual_pagebytes_)
		: "%%d0", "%%d1", "%%d2", "%%d3", "%%d4", "%%d5", "%%d6", "%%d7",
		  "%%a0", "%%a2", "%%a3", "%%a4", "%%a5",
		  "cc"
	);
}

inline void P8_fill_specrow_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pjy1, s16 pjy2)
{
}


