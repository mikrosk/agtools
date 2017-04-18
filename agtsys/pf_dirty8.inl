//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
//	playfield region update implementation
//----------------------------------------------------------------------------------------------------------------------

agt_inline void PQRS8_dirty_block_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pj2)
{
	// likely to be deprecated
}

agt_inline void PQ8_dirty_block_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pj2)
{						
	// likely to be deprecated
}

agt_inline void PR8_dirty_block_direct_(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pj2)
{						
	s16 mi = modulo_map_x(_mi);
	s16 mj = modulo_map_y(_mj);

	// start beyond rightmost tile of last 8bit block, as blocks are drawn right-to-left
	s16 mi_blockr = ((mi+pi2-pi)+7)&~7;
	const s32* pmap = &map_[mi_blockr + mul_map_x(mj)];	
	const u16* ptiles = &tiledata_[0];

	// start beyond rightmost 8block, not leftmost tile
	s16 pi_blockr = (pi2+7)&~7;
//	u16* dstaddr = &framebuffer_[(pj * rowskipwords_) + (pi_blockr * c_tilelinewords_)];
	u16* dstaddr = &framebuffer_[(pj * rowskipwords_) + (pi_blockr * 2)];
	s16 dstrowskip = (rowskipwords_ << 1);

	// todo: pass as struct

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
"										\
		jsr		_do_dirtyrect_pr8;		\
"
     	: 
		: 
		: "cc"
	);	
}

agt_inline void PR8_dirty_block_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pj2)
{
	// needs tested only once
	if ((pi == pi2) || (pj == pj2))
		return;

	// for dirty tile updates, we can only process columns of 32 tiles at a time (32 dirty bits)
	// and a typical 320-wide 8x8 tilemap leaves ((336+63)/64 pixels = 6 aligned tilespans visible (48 tiles)
	// to deal with this we break the update into two aligned columns starting with [pi]

	s16 pialign = pi & ~7;
	s16 pi2align = (pi2 + 7) & ~7;
	s16 dpialign = pi2align - pialign;

	if (dpialign > 32)
	{
		// split into two blocks at last aligned partition
		s16 pi2part = (pi&~7)+32;
		s16 mipart = _mi + (pi2part - pi);
		PR8_dirty_block_direct_(_mi, _mj, pi, pj, pi2part, pj2);
		PR8_dirty_block_direct_(mipart, _mj, pi2part, pj, pi2, pj2);
	}
	else
	{
		// single block is enough
		PR8_dirty_block_direct_(_mi, _mj, pi, pj, pi2, pj2);
	}
}

agt_inline void P8_dirty_block_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pj2)
{						
	// likely to be deprecated
}
