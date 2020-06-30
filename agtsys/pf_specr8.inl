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

	u16* dstaddr = &framebuffer_[(pj * rowskipwords_) + (pjy1 * virtual_linewords_) + ((pi & -2) * c_tilelinewords_)];

	s16 dstrowskip = (rowskipwords_ << 1);

	__asm__ __volatile__ (
#ifdef TIMING_RASTERS
"												\
			move.w	#0xf00,0xffff8240.w;		\
			move.w	#0xf00,0x64.w;				\
"
#endif
"												\
			move.l	%[dstaddr],%%a4;			\
			move.l	%[virtual_pagebytes],%%d6;	\
												\
			move.w	%[pi2],%%d3;				\
			sub.w	%[pi],%%d3;		 			\
												\
			moveq	#8,%%d4;					\
			add.w	%[pjy1],%%d4;				\
			sub.w	%[pjy2],%%d4;				\
			move.w	%%d4,%%d0;					\
			add.w	%%d4,%%d4;					\
			add.w	%%d0,%%d4;					\
			lsl.w	#2,%%d4;					\
												\
			lea		xl%=(%%pc,%%d4.w),%%a0;		\
			lea		2+smc%=(%%pc),%%a5;			\
			sub.l	%%a5,%%a0;					\
			move.w	%%a0,(%%a5);				\
												\
			moveq	#1,%%d4;					\
			and.w	%[pi],%%d4;					\
			add.w	%%d4,%%a4;					\
			move.b	.tbl%=(%%pc,%%d4.w),%%d4;	\
												\
			move.l	%[pmap1],%%a0;			 	\
			move.l	%[ptiles1],%%d5;		 	\
												\
			move.l	%%a1,%%usp;					\
			bra		sxl%=; 						\
												\
.tbl%=:		dc.b	1;							\
			dc.b	7;							\
												\
xl%=:		move.l	4*7(%%a5),%%d0;				\
			movep.l	%%d0,%[c_dlw]*7(%%a3);			\
			movep.l	%%d0,%[c_dlw]*7(%%a1);			\
			move.l	4*6(%%a5),%%d0;				\
			movep.l	%%d0,%[c_dlw]*6(%%a3);			\
			movep.l	%%d0,%[c_dlw]*6(%%a1);			\
			move.l	4*5(%%a5),%%d0;				\
			movep.l	%%d0,%[c_dlw]*5(%%a3);			\
			movep.l	%%d0,%[c_dlw]*5(%%a1);			\
			move.l	4*4(%%a5),%%d0;				\
			movep.l	%%d0,%[c_dlw]*4(%%a3);			\
			movep.l	%%d0,%[c_dlw]*4(%%a1);			\
			move.l	4*3(%%a5),%%d0;				\
			movep.l	%%d0,%[c_dlw]*3(%%a3);			\
			movep.l	%%d0,%[c_dlw]*3(%%a1);			\
			move.l	4*2(%%a5),%%d0;				\
			movep.l	%%d0,%[c_dlw]*2(%%a3);			\
			movep.l	%%d0,%[c_dlw]*2(%%a1);			\
			move.l	4*1(%%a5),%%d0;				\
			movep.l	%%d0,%[c_dlw]*1(%%a3);			\
			movep.l	%%d0,%[c_dlw]*1(%%a1);			\
			move.l	4*0(%%a5),%%d0;				\
			movep.l	%%d0,%[c_dlw]*0(%%a3);			\
			movep.l	%%d0,%[c_dlw]*0(%%a1);			\
jt1%=:											\
												\
sxl%=:		move.l	%%d5,%%a5;					\
			add.l	(%%a0)+,%%a5;				\
												\
			move.l	%%a4,%%a3;					\
			move.l	%%a4,%%a1;					\
			add.l	%%d6,%%a1;					\
												\
			add.w	%%d4,%%a4;					\
			eor.w	#6,%%d4;					\
												\
smc%=:		dbra	%%d3,xl%=; 					\
												\
			move.l	%%usp,%%a1;					\
"
#ifdef TIMING_RASTERS
"												\
			move.w	#0x000,0xffff8240.w;		\
			move.w	#0x000,0x64.w;				\
"
#endif
     	:
		: [pjy1] "m"(pjy1), 
		  [pjy2] "m"(pjy2),
		  [pi] "m"(pi), 
		  [pi2] "m"(pi2), 
		  [map_tiles_x] "m"(map_tiles_x_), 
		  [pmap1] "m"(pmap), 
		  [ptiles1] "m"(ptiles),   
		  [dstaddr] "m"(dstaddr), 
		  [virtual_pagebytes] "m"(virtual_pagebytes_),
		  [c_dlw] "g" (c_dst_linewid)
		: "%%d0", "%%d1", "%%d2", "%%d3", "%%d4", "%%d5", "%%d6", "%%d7",
		  "%%a0", "%%a2", "%%a3", "%%a4", "%%a5",
		  "cc"
	);
}

inline void P8_fill_specrow_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pjy1, s16 pjy2)
{
}


