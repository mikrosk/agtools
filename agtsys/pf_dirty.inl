//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
//	playfield region update implementation
//----------------------------------------------------------------------------------------------------------------------

	inline void PQRS_dirty_block_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pj2)
	{
		if ((pi == pi2) || (pj == pj2))
			return;

		s16 mi = modulo_map_x(_mi);
		s16 mj = modulo_map_y(_mj);

		// start beyond rightmost tile of last 8bit block, as blocks are drawn right-to-left
		s16 mi_blockr = ((mi+pi2-pi)+7)&~7;
		const s32* pmap = &map_[mi_blockr + mul_map_x(mj)];	
		const u16* ptiles = &tiledata_[0];

		// start beyond rightmost 8block, not leftmost tile
		s16 pi_blockr = (pi2+7)&~7;
		u16* dstaddr = &framebuffer_[(pj * rowskipwords_) + (pi_blockr * c_tilelinewords_)];
		s16 dstrowskip = (rowskipwords_ << 1);

		//

		dm_dstaddr = (s16*)dstaddr;
		dm_ptiles = (s16*)ptiles;
		dm_pmap = (s32*)pmap;
		dm_virtual_pagebytes = virtual_pagebytes_;
		dm_visible_linebytes = visible_linebytes_;
		dm_map_tiles_x = map_tiles_x_;
		dm_pi = pi;
		dm_pi2 = pi2;
		dm_pj = pj;
		dm_pj2 = pj2;
		dm_lineskip = dstrowskip;

		// mask is dirtymaskstride_ bytes wide (or dirtymaskstride_/2 fields wide: each field is 16bits, only top 8 occupied)
		dm_dirtymask = &dirtymask_[((mi+7)>>3) + (mj*dirtymaskwidth_)];
		dm_maskstride = dirtymaskstride_;

		__asm__ __volatile__ (
"											\
			jsr		_do_dirtyrect_pqrs;		\
"
	     	: 
			: 
			: "cc"
		);	
	}

	inline void PQ_dirty_block_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pj2)
	{						
		if ((pi == pi2) || (pj == pj2))
			return;

		s16 mi = modulo_map_x(_mi);
		s16 mj = modulo_map_y(_mj);

		// start beyond rightmost tile of last 8bit block, as blocks are drawn right-to-left
		s16 mi_blockr = ((mi+pi2-pi)+7)&~7;
		const s32* pmap = &map_[mi_blockr + mul_map_x(mj)];	
		const u16* ptiles = &tiledata_[0];

		// start beyond rightmost 8block, not leftmost tile
		s16 pi_blockr = (pi2+7)&~7;
		u16* dstaddr = &framebuffer_[(pj * rowskipwords_) + (pi_blockr * c_tilelinewords_)];
		s16 dstrowskip = (rowskipwords_ << 1);

		//

		dm_dstaddr = (s16*)dstaddr;
		dm_ptiles = (s16*)ptiles;
		dm_pmap = (s32*)pmap;
		dm_virtual_pagebytes = virtual_pagebytes_;
		dm_visible_linebytes = visible_linebytes_;
		dm_map_tiles_x = map_tiles_x_;
		dm_pi = pi;
		dm_pi2 = pi2;
		dm_pj = pj;
		dm_pj2 = pj2;
		dm_lineskip = dstrowskip;

		// mask is dirtymaskstride_ bytes wide (or dirtymaskstride_/2 fields wide: each field is 16bits, only top 8 occupied)
		dm_dirtymask = &dirtymask_[((mi+7)>>3) + (mj*dirtymaskwidth_)];
		dm_maskstride = dirtymaskstride_;

		__asm__ __volatile__ (
"											\
			jsr		_do_dirtyrect_pq;		\
"
	     	: 
			: 
			: "cc"
		);	
	}

	inline void PR_dirty_block_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pj2)
	{						
		if ((pi == pi2) || (pj == pj2))
			return;

		s16 mi = modulo_map_x(_mi);
		s16 mj = modulo_map_y(_mj);

		// start beyond rightmost tile of last 8bit block, as blocks are drawn right-to-left
		s16 mi_blockr = ((mi+pi2-pi)+7)&~7;
		const s32* pmap = &map_[mi_blockr + mul_map_x(mj)];	
		const u16* ptiles = &tiledata_[0];

		// start beyond rightmost 8block, not leftmost tile
		s16 pi_blockr = (pi2+7)&~7;
		u16* dstaddr = &framebuffer_[(pj * rowskipwords_) + (pi_blockr * c_tilelinewords_)];
		s16 dstrowskip = (rowskipwords_ << 1);

		//

		dm_dstaddr = (s16*)dstaddr;
		dm_ptiles = (s16*)ptiles;
		dm_pmap = (s32*)pmap;
		dm_virtual_pagebytes = virtual_pagebytes_;
		dm_visible_linebytes = visible_linebytes_;
		dm_map_tiles_x = map_tiles_x_;
		dm_pi = pi;
		dm_pi2 = pi2;
		dm_pj = pj;
		dm_pj2 = pj2;
		dm_lineskip = dstrowskip;

		// mask is dirtymaskstride_ bytes wide (or dirtymaskstride_/2 fields wide: each field is 16bits, only top 8 occupied)
		dm_dirtymask = &dirtymask_[((mi+7)>>3) + (mj*dirtymaskwidth_)];
		dm_maskstride = dirtymaskstride_;

		__asm__ __volatile__ (
"											\
			jsr		_do_dirtyrect_pr;		\
"
	     	: 
			: 
			: "cc"
		);	
	}

	inline void P_dirty_block_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pj2)
	{						
		if ((pi == pi2) || (pj == pj2))
			return;

		s16 mi = modulo_map_x(_mi);
		s16 mj = modulo_map_y(_mj);

		// start beyond rightmost tile of last 8bit block, as blocks are drawn right-to-left
		s16 mi_blockr = ((mi+pi2-pi)+7)&~7;
		const s32* pmap = &map_[mi_blockr + mul_map_x(mj)];	
		const u16* ptiles = &tiledata_[0];

		// start beyond rightmost 8block, not leftmost tile
		s16 pi_blockr = (pi2+7)&~7;
		u16* dstaddr = &framebuffer_[(pj * rowskipwords_) + (pi_blockr * c_tilelinewords_)];
		s16 dstrowskip = (rowskipwords_ << 1);

		//

		dm_dstaddr = (s16*)dstaddr;
		dm_ptiles = (s16*)ptiles;
		dm_pmap = (s32*)pmap;
		dm_virtual_pagebytes = virtual_pagebytes_;
		dm_visible_linebytes = visible_linebytes_;
		dm_map_tiles_x = map_tiles_x_;
		dm_pi = pi;
		dm_pi2 = pi2;
		dm_pj = pj;
		dm_pj2 = pj2;
		dm_lineskip = dstrowskip;

		// mask is dirtymaskstride_ bytes wide (or dirtymaskstride_/2 fields wide: each field is 16bits, only top 8 occupied)
		dm_dirtymask = &dirtymask_[((mi+7)>>3) + (mj*dirtymaskwidth_)];
		dm_maskstride = dirtymaskstride_;

		__asm__ __volatile__ (
"											\
			jsr		_do_dirtyrect_p;		\
"
	     	: 
			: 
			: "cc"
		);		
	}
