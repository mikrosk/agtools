//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
//	playfield region update implementation
//----------------------------------------------------------------------------------------------------------------------

	agt_inline void PQRS8_fill_block_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pj2)
	{
	}

	agt_inline void PQ8_fill_block_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pj2)
	{						
	}

	agt_inline void PR8_fill_block_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pj2)
	{						
		// start mi,mj modulo map dimensions
		
		if ((pi == pi2) || (pj == pj2))
			return;
	
		s16 mi = modulo_map_x(_mi);
		s16 mj = modulo_map_y(_mj);

		//const s32* pmap = &map_[mi + (mj * map_tiles_x_)];	
		const s32* pmap = &map_[mi + mul_map_x(mj)];	
		const u16* ptiles = &tiledata_[0];

		u16* dstaddr = &framebuffer_[(pj * rowskipwords_) + ((pi & -2) * 2)];

//		01 01 01 01 01 01 6
//		01 01 01 01 01 0  5
//		 1 01 01 01 01 01 6
//		 1 01 01 01 01 0  5

		// calculate number of word-based skips
		// note: 8x8 tiles are effectively 'half-words' with a skip on every 2nd (odd) tile
		// so word skip calculations must be based on floor(i)
		s16 nskips = ((pi2 & -2) - (pi & -2)) << (3-1);
		s16 dstrowskip = (rowskipwords_ << 1) - nskips;

		// todo:
		// - create 2 paths for starting odd/even on all rows (versus testing on each row)
		__asm__ __volatile__ (
#ifdef TIMING_RASTERS
"												\
			move.w	#0xf00,0xffff8240.w;		\
			move.w	#0xf00,0x64.w;				\
"										
#endif
"												\
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
			move.w	%5,%%d4; 					\
			sub.w	%3,%%d4; 					\
												\
			move.w	%13,smc%=+2;				\
			move.l	%%a1,%%usp;					\
			bra		syl%=; 						\
yl%=:											\
			move.w	%%d5,%%d3; 					\
			move.l	%%d1,%%a0;					\
			add.l	%%d2,%%d1;					\
												\
			moveq	#1,%%d0;					\
			and.w	%2,%%d0;					\
			bne		sodd%=;						\
			bra		sevn%=;						\
												\
levn%=:		move.l	%%a2,%%a5;					\
			add.l	(%%a0)+,%%a5;				\
												\
			move.l	%%a4,%%a3;					\
			move.l	%%a4,%%a1;					\
			add.l	%%d6,%%a1;					\
												\
			move.l	(%%a5)+,%%d0;				\
			movep.l	%%d0,0*184(%%a3);			\
			movep.l	%%d0,0*184(%%a1);			\
			move.l	(%%a5)+,%%d0;				\
			movep.l	%%d0,1*184(%%a3);			\
			movep.l	%%d0,1*184(%%a1);			\
			move.l	(%%a5)+,%%d0;				\
			movep.l	%%d0,2*184(%%a3);			\
			movep.l	%%d0,2*184(%%a1);			\
			move.l	(%%a5)+,%%d0;				\
			movep.l	%%d0,3*184(%%a3);			\
			movep.l	%%d0,3*184(%%a1);			\
			move.l	(%%a5)+,%%d0;				\
			movep.l	%%d0,4*184(%%a3);			\
			movep.l	%%d0,4*184(%%a1);			\
			move.l	(%%a5)+,%%d0;				\
			movep.l	%%d0,5*184(%%a3);			\
			movep.l	%%d0,5*184(%%a1);			\
			move.l	(%%a5)+,%%d0;				\
			movep.l	%%d0,6*184(%%a3);			\
			movep.l	%%d0,6*184(%%a1);			\
			move.l	(%%a5)+,%%d0;				\
			movep.l	%%d0,7*184(%%a3);			\
			movep.l	%%d0,7*184(%%a1);			\
												\
sodd%=:		dbra	%%d3,lodd%=;				\
			bra		smc%=;						\
												\
lodd%=:		move.l	%%a2,%%a5;					\
			add.l	(%%a0)+,%%a5;				\
												\
			move.l	%%a4,%%a3;					\
			move.l	%%a4,%%a1;					\
			add.l	%%d6,%%a1;					\
												\
			move.l	(%%a5)+,%%d0;				\
			movep.l	%%d0,0*184+1(%%a3);			\
			movep.l	%%d0,0*184+1(%%a1);			\
			move.l	(%%a5)+,%%d0;				\
			movep.l	%%d0,1*184+1(%%a3);			\
			movep.l	%%d0,1*184+1(%%a1);			\
			move.l	(%%a5)+,%%d0;				\
			movep.l	%%d0,2*184+1(%%a3);			\
			movep.l	%%d0,2*184+1(%%a1);			\
			move.l	(%%a5)+,%%d0;				\
			movep.l	%%d0,3*184+1(%%a3);			\
			movep.l	%%d0,3*184+1(%%a1);			\
			move.l	(%%a5)+,%%d0;				\
			movep.l	%%d0,4*184+1(%%a3);			\
			movep.l	%%d0,4*184+1(%%a1);			\
			move.l	(%%a5)+,%%d0;				\
			movep.l	%%d0,5*184+1(%%a3);			\
			movep.l	%%d0,5*184+1(%%a1);			\
			move.l	(%%a5)+,%%d0;				\
			movep.l	%%d0,6*184+1(%%a3);			\
			movep.l	%%d0,6*184+1(%%a1);			\
			move.l	(%%a5)+,%%d0;				\
			movep.l	%%d0,7*184+1(%%a3);			\
			movep.l	%%d0,7*184+1(%%a1);			\
												\
			addq.l	#8,%%a4;					\
												\
sevn%=:		dbra	%%d3,levn%=;				\
												\
smc%=:		lea		0x1234(%%a4),%%a4;			\
syl%=:		dbra	%%d4,yl%=; 					\
												\
			move.l	%%usp,%%a1;					\
												\
"
#ifdef TIMING_RASTERS
"												\
			move.w	#0x000,0xffff8240.w;		\
			move.w	#0x000,0x64.w;				\
"										
#endif
	     	: 
			: "m"(_mi), "m"(_mj), 
			  "m"(pi), "m"(pj), 
			  "m"(pi2), "m"(pj2),
			  "m"(mi), "m"(mj), 
			  "m"(map_tiles_x_), "m"(map_tiles_y_), 
			  "m"(pmap), "m"(ptiles), "m"(dstaddr), "m"(dstrowskip),
			  "m"(srcywrap_), "m"(virtual_linebytes_), "m"(visible_linebytes_), "m"(virtual_pagebytes_)
			: "%%d0", "%%d1", "%%d2", "%%d3", "%%d4", "%%d5", "%%d6", "%%d7",
			  "%%a0", "%%a2", "%%a3", "%%a4", "%%a5",
			  "cc"
		);	
	}

	agt_inline void P8_fill_block_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pj2)
	{						
	}
