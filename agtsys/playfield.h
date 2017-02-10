//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
//	playfield systems
//----------------------------------------------------------------------------------------------------------------------

#ifndef playfield_h_
#define playfield_h_

//----------------------------------------------------------------------------------------------------------------------

#include "drawcontext.h"

extern "C" s16 *dm_dirtymask;
extern "C" s16 *dm_dstaddr;
extern "C" s16 *dm_ptiles;
extern "C" s32 *dm_pmap;
extern "C" s32 dm_virtual_pagebytes;
extern "C" s16 dm_visible_linebytes;
extern "C" s16 dm_map_tiles_x;
extern "C" s16 dm_pi;
extern "C" s16 dm_pi2;
extern "C" s16 dm_pj;
extern "C" s16 dm_pj2;
extern "C" s16 dm_maskstride;
extern "C" s16 dm_lineskip;

extern "C" void do_dirtyrect_pqrs();
extern "C" void do_dirtyrect_pq();
extern "C" void do_dirtyrect_pr();
extern "C" void do_dirtyrect_p();

//----------------------------------------------------------------------------------------------------------------------

// scanwalk mode (consume padding scanlines instead of looping on h axis)

// PPPPPPPP*
// PPPPPPPP*
// PPPPPPPP*
// PPPPPPPP*
// *********
// RRRRRRRR*
// RRRRRRRR*
// RRRRRRRR*
// RRRRRRRR*
// *********
// xxxxxxxxx
// xxxxxxxxx padding lines for scanwalk
// xxxxxxxxx
// xxxxxxxxx

// full looping mode

// PPPPPPPP*QQQQQQQQ*
// PPPPPPPP*QQQQQQQQ*
// PPPPPPPP*QQQQQQQQ*
// PPPPPPPP*QQQQQQQQ*
// ******************
// RRRRRRRR*SSSSSSSS*
// RRRRRRRR*SSSSSSSS*
// RRRRRRRR*SSSSSSSS*
// RRRRRRRR*SSSSSSSS*
// ******************

// ---------------------------------------------------------------------------------------------------------------------

enum HScrollMode
{
	HScrollMode_NONE,
	HScrollMode_MARGIN,		// note: not yet supported
	HScrollMode_LOOPBACK,	// note: likely to be deprecated - scanwalk is cheaper
	HScrollMode_SCANWALK
};

enum VScrollMode
{
	VScrollMode_NONE,
	VScrollMode_MARGIN,		// note: not yet supported
	VScrollMode_LOOPBACK
};

enum RestoreMode
{
	RestoreMode_NONE,
	RestoreMode_SAVERESTORE,
	RestoreMode_PAGERESTORE
};

extern "C" s16 g_tshift;
extern "C" s16 g_displayservice_consumed_lines;
extern "C" s16 g_xpreshift;

extern "C" int STFM_MaskEdgeL(u16* _pfb, int _xoff, int _yoff, int _height);
extern "C" int STFM_MaskEdgeR(u16* _pfb, int _xoff, int _yoff, int _height);
//extern "C" int STFM_RestoreEdges(int _xoff);

// ---------------------------------------------------------------------------------------------------------------------

static inline void lcopytorow(s32* _src, s32* _dst, s16 _count)
{
	s32 *s = _src;
	s32 *d = _dst;
	s16 c = _count-1;
	do
	{
//		*d++ = *s++;
		__asm__ __volatile__
		( "move.l (%0)+,(%1)+;" : "+a"(s), "+a"(d) : : );
	} while (--c != -1);
}

static inline void lcopytocolumn(s32* _src, s32* _dst, s16 _dy, s16 _stride)
{
	s32 *s = _src;
	s32 *d = _dst;

	s16 yc = _dy-1;
	do
	{
//		*d = *s++;
    	__asm__ __volatile__
	   	( "move.l (%0)+,(%1);" : "+a"(s) : "a"(d) : );

		d += _stride;

	} while (--yc != -1);
}

static inline void lcopytorect(s32* _src, s32* _dst, s16 _dx, s16 _dy, s16 _stride)
{
	s32 *s = _src;
	s32 *d = _dst;

	s16 xcc = _dx-1;
	s16 yc = _dy-1;
	do
	{
		s32 *dt = d; d += _stride;
		s16 xc = xcc;
		do
		{
//			*dt++ = *s++;
    		__asm__ __volatile__
	    	( "move.l (%0)+,(%1)+;" : "+a"(s), "+a"(dt) : : );

		} while (--xc != -1);
	} while (--yc != -1);
}

// ---------------------------------------------------------------------------------------------------------------------

#define SPECULATIVE_FILL_METHOD (0)
//#define BLITTER_HOG
//#define WRAPAROUND_MAPS

// ---------------------------------------------------------------------------------------------------------------------

template 
<
	// tilesize in 2^n
	s16 _tilestep, 
	//
	s16 _max_scroll,
	// configure vertical scrolling
	VScrollMode _vscroll,
	// configure horizontal scrolling
	HScrollMode _hscroll,
	// configure background restore mode
	RestoreMode _restore,
	// enable speculative tile optimization (more complicated logic, less tile drawing)
	bool _speculative,
	// STE hardware scroll+linewid
	bool _ste_hwscroll,
	// use LJBK 4- or 8-pixel sync shift
	bool _stf_hsyncshift,
	// use blitter (STE/MegaST) [deprecated]
	//bool _blitter,
	// configure support for dynamic tiles (editing tile indices while displaying them)
	bool _dynamic
>
class playfield_template
{
public:
	
	// ---------------------------------------------------------------------------------------------------------------------
	// configure behaviour
	// ---------------------------------------------------------------------------------------------------------------------

	static const bool _blitter = false;

	// TODO: mostly hardwired for now
	static const int c_planes = 4;

	static const int c_preshift_stepshift_ = 2;
	static const int c_preshift_step_ = 1<<c_preshift_stepshift_;

	static const bool c_mapwrapping = false;
	static const bool c_dynamic_ = _dynamic;

	// buffer speculation is for writing tile slices to future buffers ahead of time
	static const bool c_hbufferspeculation_ = false;//(!_ste);
	static const bool c_hsyncshift_ = _stf_hsyncshift && (!_ste_hwscroll); // ljbk display only

	// tilesize
	static const s16 c_tilestep_ = _tilestep;

	// update method
	static const bool c_scanwalk = (_hscroll == HScrollMode_SCANWALK);

	// only update tile columns if full horizontal scrolling is possible
	static const bool c_updatecols = (_hscroll != HScrollMode_NONE) && (_hscroll != HScrollMode_MARGIN);
	// only update tile rows if full vertical scrolling is possible
	static const bool c_updaterows = (_vscroll != VScrollMode_NONE) && (_vscroll != VScrollMode_MARGIN);

	static const bool c_hdynamic_ = _dynamic && c_updatecols;
	static const bool c_vdynamic_ = _dynamic && c_updaterows;

	// only speculate on H axis if there is offscreen margin (only possible on STE with hscroll+linewid)
	static const bool c_hspeculative_ = _speculative && c_updatecols && _ste_hwscroll;
	// only speculate via buffers if we're using them (STF preshifted framebuffers)
//	static const bool c_hbufferspeculative_ = _speculative && c_updatecols && (!_ste);
	// always possible to speculate on V axis
	static const bool c_vspeculative_ = _speculative && c_updaterows;

	static const s16 c_hmargin_ = (s16)(int)c_hspeculative_;
	static const s16 c_vmargin_ = (s16)(int)c_vspeculative_;

	static const bool c_ste_ = _ste_hwscroll;
	static const bool c_hsoftscroll_ = !_ste_hwscroll;
	static const bool c_blitter_ = _blitter;

	static const bool c_hloop = (_hscroll == HScrollMode_LOOPBACK);
	static const bool c_vloop = (_vscroll == VScrollMode_LOOPBACK);

	// configure PQRS buffer update pattern
	static const bool c_updateP = c_updatecols || c_updaterows;
	static const bool c_updateQ = (_hscroll == HScrollMode_LOOPBACK);
	static const bool c_updateR = (_vscroll == VScrollMode_LOOPBACK) || (_restore == RestoreMode_PAGERESTORE);
	static const bool c_updateS = c_updateQ && c_updateR;

	// internal stuff for tiles
	static const s16 c_tilesize_ = 1 << c_tilestep_;
	static const s16 c_tilelinewords_ = c_tilesize_ >> 2;
	static const s16 c_tileplanewords_ = c_tilesize_ >> 4;
	static const s16 c_tilewords_ = c_tilelinewords_ * c_tilesize_;

	// maximum number of virtual tiles possible on any axis
//	static const s16 c_max_vt_ = 32768 / c_tilesize_;
	static const s16 c_max_vt_ = _max_scroll / c_tilesize_;

	// ---------------------------------------------------------------------------------------------------------------------

	class mapcontext
	{
	private:

		void release()
		{
			delete[] modulo_map_x_; modulo_map_x_ = 0;
			delete[] modulo_map_y_; modulo_map_y_ = 0;
			delete[] mul_map_x_; mul_map_x_ = 0;
		}

		void prepare()
		{
			srcywrap_ = (map_tiles_x_ * map_tiles_y_) << 2;	// todo: map element size		

			delete[] mul_map_x_; 
			mul_map_x_ = new s16[map_tiles_y_];
			for (s16 n = 0; n < map_tiles_y_; ++n)
				mul_map_x_[n] = n * map_tiles_x_;

			{
				s16 xo = c_max_vt_ / map_tiles_x_;
				s16 yo = c_max_vt_ / map_tiles_y_;
					
				delete[] modulo_map_x_; 
				delete[] modulo_map_y_; 

				modulo_map_x_ = new s16[c_max_vt_<<1];
				modulo_map_y_ = new s16[c_max_vt_<<1];

				for (s16 n = 0; n < c_max_vt_<<1; ++n)
				{
					s16 v = n-c_max_vt_;
					modulo_map_x_[n] = tmod16(v, map_tiles_x_);
					modulo_map_y_[n] = tmod16(v, map_tiles_y_);
				}
			}
		}

	public:

		mapcontext()
			: modulo_map_x_(0),
			  modulo_map_y_(0),
			  mul_map_x_(0),
			  map_tiles_x_(0),
			  map_tiles_y_(0),
			  srcywrap_(0),
			  map_(0)
		{
		}

		// owned: map addressing storage
		s16* modulo_map_x_;
		s16* modulo_map_y_;
		s16* mul_map_x_;

		// internal: shadowed externally for speed
		s16 map_tiles_x_;
		s16 map_tiles_y_;
		s16 srcywrap_;
		s32* map_;

		void setmap(s32* _map)
		{
			map_tiles_x_ = s16(_map[0]);
			map_tiles_y_ = s16(_map[1]);
			map_ = &_map[2];

			prepare();
		}

		void setmap(worldmap* _map)
		{
			map_tiles_x_ = _map->getwidth();
			map_tiles_y_ = _map->getheight();
			map_ = _map->get();

			prepare();
		}

		FORCEINLINE void multimod(s16 _x, s16 _y, s32 _rawidx)
		{
			if (c_dynamic_)
			{
				s32 index = mul_map_x_[_y] + _x;
				map_[index] = _rawidx;
			}
		}

		void multimod_row(s16 _x, s16 _y, s16 _dx, s32 *_source)
		{
			if (c_dynamic_)
			{
				// update map
				s32 index = mul_map_x_[_y] + _x;
				lcopytorow(_source, &map_[index], _dx);
			}
		}

		void multimod_column(s16 _x, s16 _y, s16 _dy, s32 *_source)
		{
			// todo: prototype version - move this stuff to 68k
			if (c_dynamic_)
			{
				// update map
				s32 index = mul_map_x_[_y] + _x;
				lcopytocolumn(_source, &map_[index], _dy, map_tiles_x_);
			}
		}

		void multimod_rect(s16 _x, s16 _y, s16 _dx, s16 _dy, s32 *_source)
		{
			if (c_dynamic_)
			{
				// update map
				s32 index = mul_map_x_[_y] + _x;
				lcopytorect(_source, &map_[index], _dx, _dy, map_tiles_x_);
			}
		}

		~mapcontext()
		{
			release();
		}
	};

	// ---------------------------------------------------------------------------------------------------------------------

	class fieldcontext
	{
	private:

		void release()
		{
			delete[] quant_vpage_x_; quant_vpage_x_ = 0;
			delete[] quant_vpage_y_; quant_vpage_y_ = 0;
			delete[] modulo_vpage_x_; modulo_vpage_x_ = 0;
			delete[] modulo_vpage_y_; modulo_vpage_y_ = 0;
			delete[] lineindex_; lineindex_ = 0;
			delete[] linemod_; linemod_ = 0;

			initialized_ = false;
		}

	public:

		fieldcontext()
			: initialized_(false)
		{
		}

		// owned: page addressing & wrap-mod storage
		s16* quant_vpage_x_;
		s16* quant_vpage_y_;
		s16* modulo_vpage_x_;
		s16* modulo_vpage_y_;
		s32 *lineindex_;
		s16 *linemod_;

		bool initialized_;

		void prepare(s16 vpage_tiles_x_, s16 vpage_tiles_y_)
		{
			if (!initialized_)
			{
				s16 xo = c_max_vt_ / vpage_tiles_x_;
				s16 yo = c_max_vt_ / vpage_tiles_y_;
				s16 maxlines = vpage_tiles_y_ * c_tilesize_ * (c_updateR ? 2 : 1);
				
				delete[] quant_vpage_x_; 
				delete[] quant_vpage_y_; 
				delete[] modulo_vpage_x_; 
				delete[] modulo_vpage_y_; 
				delete[] lineindex_;
				delete[] linemod_;

				quant_vpage_x_ = new s16[c_max_vt_<<1];
				quant_vpage_y_ = new s16[c_max_vt_<<1];
				modulo_vpage_x_ = new s16[c_max_vt_<<1];
				modulo_vpage_y_ = new s16[c_max_vt_<<1];
				lineindex_ = new s32[maxlines];
				linemod_ = new s16[maxlines];

				for (s16 n = 0; n < c_max_vt_<<1; ++n)
				{
					s16 v = n-c_max_vt_;
					quant_vpage_x_[n] = ((v + (xo * vpage_tiles_x_)) / vpage_tiles_x_) - xo;
					quant_vpage_y_[n] = ((v + (yo * vpage_tiles_y_)) / vpage_tiles_y_) - yo;
					modulo_vpage_x_[n] = tmod16(v, vpage_tiles_x_);
					modulo_vpage_y_[n] = tmod16(v, vpage_tiles_y_);
				}
			
				// generate playfield workbuffer scanline modulo & index
				s16 linewidth = (vpage_tiles_x_ * (c_updateQ ? 2 : 1)) << 3;
				for (s16 n = 0; n < maxlines; ++n)
				{
					lineindex_[n] = s32(n * linewidth);
					linemod_[n] = tmod16(n, (vpage_tiles_y_ * c_tilesize_));
				}

/*				for (s16 n = 0; n < c_max_vt_<<1; ++n)
				{
					s16 v = n-c_max_vt_;
				}
				
				for (s16 n = 0; n < c_max_vt_<<1; ++n)
				{
					s16 v = n-c_max_vt_;
				}
				
				for (s16 n = 0; n < c_max_vt_<<1; ++n)
				{
					s16 v = n-c_max_vt_;
				}
*/

				pprintf("playfield context initialized...\n");
				initialized_ = true;
			}
		}

		~fieldcontext()
		{
			release();
		}
	};

	// ---------------------------------------------------------------------------------------------------------------------

	mapcontext *pmapcontext_;
	fieldcontext *pfieldcontext_;
	
	s16 display_width_;
	s16 display_height_;
	s16 virtual_width_;
	s16 virtual_height_;
	
	s16 visible_tiles_x_;
	s16 visible_tiles_y_;

	s16 vpage_tiles_x_;
	s16 vpage_tiles_y_;

	s16 scrollmargin_tiles_x_;
	s16 scrollmargin_tiles_y_;
	
	s16 virtual_tiles_x_;
	s16 virtual_tiles_y_;
	
	s16 wrap_width_;
	s16 wrap_height_;
	
	s16 virtual_linewords_;
//	s16 visible_linewords_;
	s16 vpage_linewords_;
	
	s16 xpos_;
	s16 ypos_;
	s16 last_xpos_;
	s16 last_ypos_;
		
	s16 txpos_;	
	s16 typos_;
	s16 last_txpos_;
	s16 last_typos_;

	s16 tx_last_dy_;
	s16 ty_last_dx_;
	
	s16 map_tiles_x_;
	s16 map_tiles_y_;
	s32 srcywrap_;
	s32* map_;

	// private, owned storage
	s16* quant_vpage_x_;
	s16* quant_vpage_y_;
	s16* modulo_vpage_x_;
	s16* modulo_vpage_y_;
	s16* modulo_map_x_;
	s16* modulo_map_y_;
	s16* mul_map_x_;
	u8 *display_alloc_;
	
	s16 rowskipwords_;
	s16 virtual_linebytes_;
	s16 visible_linebytes_;
	s32 virtual_pagebytes_;
	
	u32 bufferwords_;

	s16 buffer_;
	s16 field_;
	u16 *display_;
	u16 *framebuffer_;
		
	tileset* tiles_;
	const u16* tiledata_;
	
	s32 tilecount_;
	
	mersenne_twister twister_;

	s16 drawshift_;
	s16 xpreshift_;
	s16 last_slice_;
	s16 slice_;

	s32* dirtytiles_;
	s16* dirtymask_;
	s16 dirtynum_;
	s16 dirtymaskwidth_;
	s16 dirtymaskstride_;
	s16** dirtymasklines_;

	// ---------------------------------------------------------------------------------------------------------------------

//	playfield_template();	
//	~playfield_template();

//	void init(s16 _buffer, s16 _field, s16 _display_width, s16 _display_height);

	// ---------------------------------------------------------------------------------------------------------------------

	inline void setcontext(mapcontext *_pmctx, fieldcontext *_pfctx)
	{
		pmapcontext_ = _pmctx;
		pfieldcontext_ = _pfctx;
	}

	// associate fixed preshift offset with this playfield buffer
	inline void setpreshift(s16 _xps)
	{
		xpreshift_ = _xps;
	}

	// set speculative slice to render (for buffer speculation)
	inline void setslice(s16 _s)
	{
		slice_ = _s<<c_preshift_stepshift_;
	}

	inline void setpos(s16 _x, s16 _y)
	{
		xpos_ = _x;//(_x & 0xFFFC) + xpreshift_;
		ypos_ = _y;
		txpos_ = xpos_ >> c_tilestep_;
		typos_ = ypos_ >> c_tilestep_;

//#if defined(LJBK_SYNC)
//#if defined(LJBK_4SYNC)
////		s16 xr = _x & 0xf;
////		if (xr < 8)
////			g_tshift = xr & 3;
////		else
////			g_tshift = xr & 7;
//		g_tshift = xpos_ & 3;
//#else
//		g_tshift = xpos_ & 7;
//#endif
//#else
//		g_tshift = xpos_ & (c_tilesize_-1);
//#endif

		if (!c_ste_ && !c_hbufferspeculation_)
		{
			// suppress redrawing of shifted tilestrips for no effective change in tile position or shift in this buffer
			bool draw = (typos_ != last_typos_) || 
						(txpos_ != last_txpos_) || 
						((xpos_ & 3) != (last_xpos_ & 3));

			drawshift_ = draw;
		}

		drawshift_ = 1;
	}
		
	//void setmap(s32* _map)
	//{
	//	map_tiles_x_ = s16(_map[0]);
	//	map_tiles_y_ = s16(_map[1]);
	//	map_ = &_map[2];

	//	srcywrap_ = (map_tiles_x_ * map_tiles_y_) << 2;	// todo: map element size		

	//	delete[] mul_map_x_; 
	//	mul_map_x_ = new s16[map_tiles_y_];
	//	for (s16 n = 0; n < map_tiles_y_; ++n)
	//		mul_map_x_[n] = n * map_tiles_x_;
	//}

	//void setmap(worldmap* _map)
	//{
	//	map_tiles_x_ = _map->getwidth();
	//	map_tiles_y_ = _map->getheight();
	//	map_ = _map->get();

	//	srcywrap_ = (map_tiles_x_ * map_tiles_y_) << 2;	// todo: map element size		

	//	delete[] mul_map_x_; 
	//	mul_map_x_ = new s16[map_tiles_y_];
	//	for (s16 n = 0; n < map_tiles_y_; ++n)
	//		mul_map_x_[n] = n * map_tiles_x_;
	//}

	void settiles(tileset* _tiles)
	{	
		tiles_ = _tiles;
		tiledata_ = tiles_->get();
	}

	inline void select(shifter_common& _shift)
	{
		u32 addr = (u32)display_;

		s16 mx = xpos_;
		s16 my = ypos_;

		if (c_hloop)
			mx = tmod16(mx, wrap_width_);
		if (c_vloop)
			my = tmod16(my, wrap_height_);

		_shift.setdisplay(field_, addr, display_width_, virtual_width_, mx, my);

/*		if (c_scanwalk)
			// todo: fast mod here
//			_shift.setdisplay(field_, addr, display_width_, virtual_width_, xpos_, ypos_);
			_shift.setdisplay(field_, addr, display_width_, virtual_width_, xpos_, tmod(ypos_, wrap_height_));
		else
			_shift.setdisplay(field_, addr, display_width_, virtual_width_, tmod(xpos_, wrap_width_), tmod(ypos_, wrap_height_));
*/

		tilecount_ = 0;
	}
	
	void clear(u16* _addr, u32 _words)
	{
		for (u32 c = 0; c < _words; ++c)
			_addr[c] = 0;
	}
	
	void set(u16* _addr, u32 _words, u16 _word)
	{
		for (u32 c = 0; c < _words; ++c)
			_addr[c] = _word;
	}
	
	inline s16 quant_vpage_x(const s16 _vx) const
	{
		return quant_vpage_x_[_vx + c_max_vt_];
	}

	inline s16 quant_vpage_y(const s16 _vy) const
	{
		return quant_vpage_y_[_vy + c_max_vt_];
	}

	inline s16 modulo_vpage_x(const s16 _vx) const
	{
		return modulo_vpage_x_[_vx + c_max_vt_];
	}

	inline s16 modulo_vpage_y(const s16 _vy) const
	{
		return modulo_vpage_y_[_vy + c_max_vt_];
	}

	inline s16 modulo_map_x(const s16 _vx) const
	{
		return modulo_map_x_[_vx + c_max_vt_];
	}

	inline s16 modulo_map_y(const s16 _vy) const
	{
		return modulo_map_y_[_vy + c_max_vt_];
	}

	inline s16 mul_map_x(const s16 _mx) const
	{
		return mul_map_x_[_mx];
	}

	// ---------------------------------------------------------------------------------------------------------------------

	inline void prepare_blitter()
	{
		reg16(ffff8a36) = c_planes;		// tile planes (1-4)
		reg16(ffff8a28) = 0xffff;		// em1
		reg16(ffff8a2a) = 0xffff;		// em2
		reg16(ffff8a2c) = 0xffff;		// em3
		reg16(ffff8a20) = 2;			// sxi
		reg16(ffff8a22) = 2;			// syi
		reg16(ffff8a2e) = 2;			// dxi
		reg16(ffff8a30) = (virtual_linewords_<<1)-((c_tilesize_>>1)-2);	// dyi
		reg8(ffff8a3d) = 0;				// skew		
		reg8(ffff8a3a) = 2;				// hop
		reg8(ffff8a3b) = 3;				// op
		
	#ifdef FASTPAGE_TILES
		reg32(ffff8a24) = (u32)tiledata_;	// src
	#endif	
	}

#if (0)
	inline void dotile(s16 _t, s16 _i, s16 _j)		
	{
		_i = xmod(_i, virtual_tiles_x_);
		_j = xmod(_j, virtual_tiles_y_);
		
		const u16* srcaddr = &tiledata_[_t << 6];	// todo: c_tilesize_
		u16* dstaddr = display_ + (_j * c_tilesize_ * virtual_linewords_) + (_i * c_tilelinewords_);
		
		// blitter version...
#ifdef TIMING_RASTERS		
		reg16(ffff8240) = 0x00f;
#endif
		reg32(ffff8a24) = (u32)srcaddr;		// sa
		reg32(ffff8a32) = (u32)dstaddr;		// da
		reg16(ffff8a36) = c_tilelinewords_;	// xs
		reg16(ffff8a38) = c_tilesize_;			// ys
		reg8(ffff8a3c) = 0xc0;				// hog & go
#ifdef TIMING_RASTERS		
		reg16(ffff8240) = 0x000;
#endif
/*		
		// clunky C version...
		
		register u16 vlw = virtual_linewords_;
		for (register s16 yy = c_tilesize_ - 1; yy >= 0; --yy)
		{
			dstaddr[0] = srcaddr[0];
			dstaddr[1] = srcaddr[1];
			dstaddr[2] = srcaddr[2];
			dstaddr[3] = srcaddr[3];
			dstaddr += vlw;
			srcaddr += 4;
		}
*/		
	}
#endif

/*
	inline void fill_block(s16 _mi, s16 _mj, s16 _tx1, s16 _ty1, s16 _tx2, s16 _ty2)
	{
		s16 mj = _mj;
		for (s16 j = _ty1; j < _ty2; ++j, ++mj)
		{
			register s16 mi = _mi;
			for (register s16 i = _tx1; i < _tx2; ++i, ++mi)
			{
				register s16 mmi = xmod(mi, map_tiles_x_);
				register s16 mmj = xmod(mj, map_tiles_y_);				
				register u16 t = (u16)(map_[mmi + (mmj * map_tiles_x_)]);
				dotile(t,i,j);
			}
		}
	}
*/

	// =====================================================================================================================
	// optimized m68k implementations for tile fillers
	// =====================================================================================================================

	// STE and non-shifting versions
	#include "pf_block.inl"				// fill X*Y block at a time
	#include "pf_specc.inl"				// fill speculative column slices
	#include "pf_specr.inl"				// fill speculative row slices

	// STF tile-shifting versions
	#include "pf_cshift.inl"			// shift full column at a time
	#include "pf_specshiftc.inl"		// shift speculative column slices
	#include "pf_specshiftr.inl"		// shift speculative row slices

	#include "pf_dirty.inl"				// refill dirty tiles

	// =====================================================================================================================

	// debugging only - fill block using individual tile calls
	inline void fill_block_indirect(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pj2)
	{						
		for (s16 jj = 0; jj < (pj2-pj); ++jj)
		{
			for (s16 ii = 0; ii < (pi2-pi); ++ii)
			{
				if (c_updateP && c_updateQ && c_updateR && c_updateS)
					PQRS_fill_block_direct(_mi+ii, _mj+jj, pi+ii, pj+jj, pi+ii+1, pj+jj+1);
				else
				if (c_updateP && c_updateQ)
					PQ_fill_block_direct(_mi+ii, _mj+jj, pi+ii, pj+jj, pi+ii+1, pj+jj+1);
				else
				if (c_updateP && c_updateR)
					PR_fill_block_direct(_mi+ii, _mj+jj, pi+ii, pj+jj, pi+ii+1, pj+jj+1);
				else
				if (c_updateP)
					P_fill_block_direct(_mi+ii, _mj+jj, pi+ii, pj+jj, pi+ii+1, pj+jj+1);
			}
		}
	}

	inline void fill_specrow_vx_sane(s16 _mi, s16 _mj, s16 pi, s16 _vy1, s16 pi2, s16 pjy1, s16 pjy2)
	{
		if (c_vloop)
		{
			// v-wrapping case

			// start modulo virtual playfield dimensions
			s16 pj = modulo_vpage_y(_vy1);

			if (c_updateQ)
				PQRS_fill_specrow_direct(_mi, _mj, pi, pj, pi2, pjy1, pjy2);
			else
				PR_fill_specrow_direct(_mi, _mj, pi, pj, pi2, pjy1, pjy2);
		}
		else
		if (c_updateR)
		{
			// non-v-wrapping case which still requires shadow vpage R (pagerestore mode)

			if (c_updateQ)
				PQRS_fill_specrow_direct(_mi, _mj, pi, _vy1, pi2, pjy1, pjy2);
			else
				PR_fill_specrow_direct(_mi, _mj, pi, _vy1, pi2, pjy1, pjy2);
		}
		else
		{
			if (c_updateQ)
				PQ_fill_specrow_direct(_mi, _mj, pi, _vy1, pi2, pjy1, pjy2);
			else
				P_fill_specrow_direct(_mi, _mj, pi, _vy1, pi2, pjy1, pjy2);
		}
				
		//// block entirely within virtual playfield			
		//if (c_scanwalk)
		//	PR_fill_specrow_direct(_mi, _mj, pi, pj, pi2, pjy1, pjy2);
		//else
		//	PQRS_fill_specrow_direct(_mi, _mj, pi, pj, pi2, pjy1, pjy2);
	}

	inline void shift_specrow_vx_sane(s16 _mi, s16 _mj, s16 pi, s16 _vy1, s16 pi2, s16 pjy1, s16 pjy2)
	{
		if (c_vloop)
		{
			// v-wrapping case

			// start modulo virtual playfield dimensions
			s16 pj = modulo_vpage_y(_vy1);

			PR_shift_specrow_direct(_mi, _mj, pi, pj, pi2, pjy1, pjy2);
		}
		else
		if (c_updateR)
		{
			// non-v-wrapping case which still requires shadow vpage R (pagerestore mode)

			PR_shift_specrow_direct(_mi, _mj, pi, _vy1, pi2, pjy1, pjy2);
		}
		else
		{
			P_shift_specrow_direct(_mi, _mj, pi, _vy1, pi2, pjy1, pjy2);
		}
	}

	inline void fill_speccolumn_vy_sane(s16 _mi, s16 _mj, s16 _vx1, s16 pj, s16 pj2, s16 pjy1, s16 pjy2)
	{
		if (c_hloop)
		{
			// start modulo virtual playfield dimensions
			//s16 pi = xmod<s16>(_vx1, vpage_tiles_x_);
			s16 pi = modulo_vpage_x(_vx1);

			if (c_updateR)
				PQRS_fill_speccolumn_direct(_mi, _mj, pi, pj, pj2, pjy1, pjy2);
			else
				PQ_fill_speccolumn_direct(_mi, _mj, pi, pj, pj2, pjy1, pjy2);
		}
		else
		{
			s16 pi = _vx1;

			if (c_updateR)
				PR_fill_speccolumn_direct(_mi, _mj, pi, pj, pj2, pjy1, pjy2);
			else
				P_fill_speccolumn_direct(_mi, _mj, pi, pj, pj2, pjy1, pjy2);
		}

/*		if (c_scanwalk)
		{
			// in this mode, columns are un-modded on x because virtual horizontal playfield is infinite/non-looping
			PR_fill_speccolumn_direct(_mi, _mj, _vx1, pj, pj2, pjy1, pjy2);
		}
		else
		{
			// start modulo virtual playfield dimensions
			//s16 pi = xmod<s16>(_vx1, vpage_tiles_x_);
			s16 pi = modulo_vpage_x(_vx1);
					
			// block entirely within virtual playfield			
			PQRS_fill_speccolumn_direct(_mi, _mj, pi, pj, pj2, pjy1, pjy2);
		}
*/
	}	

	inline void shift_speccolumn_vy_sane(s16 _mi, s16 _mj, s16 _vx1, s16 pj, s16 pj2, s16 pjy1, s16 pjy2)
	{
		s16 pi = _vx1;

		if (c_updateR)
			PR_shift_speccolumn_direct(_mi, _mj, pi, pj, pj2, pjy1, pjy2);
		else
			P_shift_speccolumn_direct(_mi, _mj, pi, pj, pj2, pjy1, pjy2);
	}	

	inline void fill_block_vx_sane(s16 _mi, s16 _mj, s16 pi, s16 _vy1, s16 pi2, s16 _vy2)
	{
		if (_vy1 == _vy2)
			return;

		if (c_vloop)
		{
			// v-wrapping case

			// start modulo virtual playfield dimensions
			//s16 pj = xmod<s16>(_vy1, vpage_tiles_y_);
			s16 pj = modulo_vpage_y(_vy1);

			// end modulo virtual playfield dimensions
			//s16 pj2 = xmod<s16>(_vy2-1, vpage_tiles_y_)+1;
			s16 pj2 = modulo_vpage_y(_vy2-1)+1;
					
			// ordering faults indicate playfield wrap @ destination, so we split the fill
			// into sub-blocks, potentially on both axes at once

			if (pj2 <= pj)
			{
				// block straddles virtual y-wrap, so split in two										
				s16 o = vpage_tiles_y_ - pj;
				if (c_updateQ)
				{
					PQRS_fill_block_direct(_mi, _mj+o, pi,  0, pi2, pj2);	
					PQRS_fill_block_direct(_mi, _mj,	pi, pj, pi2, vpage_tiles_y_);
				}
				else
				{
					PR_fill_block_direct(_mi, _mj+o, pi,  0, pi2, pj2);	
					PR_fill_block_direct(_mi, _mj,	pi, pj, pi2, vpage_tiles_y_);
				}
			}
			else		
			{
				// block entirely within virtual playfield			
				if (c_updateQ)
					PQRS_fill_block_direct(_mi, _mj, pi, pj, pi2, pj2);
				else
					PR_fill_block_direct(_mi, _mj, pi, pj, pi2, pj2);
			}
		}
		else
		if (c_updateR)
		{
			// non-v-wrapping case which still requires shadow vpage R (pagerestore mode)

			if (c_updateQ)
				PQRS_fill_block_direct(_mi, _mj, pi, _vy1, pi2, _vy2);
			else
				PR_fill_block_direct(_mi, _mj, pi, _vy1, pi2, _vy2);
		}
		else
		{
			if (c_updateQ)
				PQ_fill_block_direct(_mi, _mj, pi, _vy1, pi2, _vy2);
			else
				P_fill_block_direct(_mi, _mj, pi, _vy1, pi2, _vy2);
		}
	}

	inline void dirty_block_vx_sane(s16 _mi, s16 _mj, s16 pi, s16 _vy1, s16 pi2, s16 _vy2)
	{
		if (_vy1 == _vy2)
			return;

		if (c_vloop)
		{
			// v-wrapping case

			// start modulo virtual playfield dimensions
			//s16 pj = xmod<s16>(_vy1, vpage_tiles_y_);
			s16 pj = modulo_vpage_y(_vy1);

			// end modulo virtual playfield dimensions
			//s16 pj2 = xmod<s16>(_vy2-1, vpage_tiles_y_)+1;
			s16 pj2 = modulo_vpage_y(_vy2-1)+1;
					
			// ordering faults indicate playfield wrap @ destination, so we split the fill
			// into sub-blocks, potentially on both axes at once

			if (pj2 <= pj)
			{
				// block straddles virtual y-wrap, so split in two										
				s16 o = vpage_tiles_y_ - pj;
				if (c_updateQ)
				{
					PQRS_dirty_block_direct(_mi, _mj+o, pi,  0, pi2, pj2);	
					PQRS_dirty_block_direct(_mi, _mj,	pi, pj, pi2, vpage_tiles_y_);
				}
				else
				{
					PR_dirty_block_direct(_mi, _mj+o, pi,  0, pi2, pj2);	
					PR_dirty_block_direct(_mi, _mj,	pi, pj, pi2, vpage_tiles_y_);
				}
			}
			else		
			{
				// block entirely within virtual playfield			
				if (c_updateQ)
					PQRS_dirty_block_direct(_mi, _mj, pi, pj, pi2, pj2);
				else
					PR_dirty_block_direct(_mi, _mj, pi, pj, pi2, pj2);
			}
		}
		else
		if (c_updateR)
		{
			// non-v-wrapping case which still requires shadow vpage R (pagerestore mode)

			if (c_updateQ)
				PQRS_dirty_block_direct(_mi, _mj, pi, _vy1, pi2, _vy2);
			else
				PR_dirty_block_direct(_mi, _mj, pi, _vy1, pi2, _vy2);
		}
		else
		{
			if (c_updateQ)
				PQ_dirty_block_direct(_mi, _mj, pi, _vy1, pi2, _vy2);
			else
				P_dirty_block_direct(_mi, _mj, pi, _vy1, pi2, _vy2);
		}
	}

	inline void P_shift_block_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pj2)
	{
		s16 o = 0;
		for (s16 c = pi; c < pi2; c++, o++)
			P_shift_column_direct(_mi + o, _mj, c, pj, c, pj2);
	}

	inline void PR_shift_block_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pj2)
	{
		s16 o = 0;
		for (s16 c = pi; c < pi2; c++, o++)
			PR_shift_column_direct(_mi + o, _mj, c, pj, c, pj2);
	}

	inline void shift_block_vx_sane(s16 _mi, s16 _mj, s16 pi, s16 _vy1, s16 pi2, s16 _vy2)
	{
		if (_vy1 == _vy2)
			return;

		if (c_vloop)
		{
			// v-wrapping case

			// start modulo virtual playfield dimensions
			//s16 pj = xmod<s16>(_vy1, vpage_tiles_y_);
			s16 pj = modulo_vpage_y(_vy1);

			// end modulo virtual playfield dimensions
			//s16 pj2 = xmod<s16>(_vy2-1, vpage_tiles_y_)+1;
			s16 pj2 = modulo_vpage_y(_vy2-1)+1;
					
			// ordering faults indicate playfield wrap @ destination, so we split the fill
			// into sub-blocks, potentially on both axes at once

			if (pj2 <= pj)
			{
				// block straddles virtual y-wrap, so split in two										
				s16 o = vpage_tiles_y_ - pj;
				PR_shift_block_direct(_mi, _mj+o, pi,  0, pi2, pj2);	
				PR_shift_block_direct(_mi, _mj,	pi, pj, pi2, vpage_tiles_y_);
			}
			else		
			{
				// block entirely within virtual playfield			
				PR_shift_block_direct(_mi, _mj, pi, pj, pi2, pj2);
			}
		}
		else
		if (c_updateR)
		{
			// non-v-wrapping case which still requires shadow vpage R (pagerestore mode)

			PR_shift_block_direct(_mi, _mj, pi, _vy1, pi2, _vy2);
		}
		else
		{
			P_shift_block_direct(_mi, _mj, pi, _vy1, pi2, _vy2);
		}
	}


	inline void shift_column_vx_sane(s16 _mi, s16 _mj, s16 pi, s16 _vy1, s16 pi2, s16 _vy2)
	{
		if (_vy1 == _vy2)
			return;

		if (c_vloop)
		{
			// v-wrapping case

			// start modulo virtual playfield dimensions
			//s16 pj = xmod<s16>(_vy1, vpage_tiles_y_);
			s16 pj = modulo_vpage_y(_vy1);

			// end modulo virtual playfield dimensions
			//s16 pj2 = xmod<s16>(_vy2-1, vpage_tiles_y_)+1;
			s16 pj2 = modulo_vpage_y(_vy2-1)+1;
					
			// ordering faults indicate playfield wrap @ destination, so we split the fill
			// into sub-blocks, potentially on both axes at once

			if (pj2 <= pj)
			{
				// block straddles virtual y-wrap, so split in two										
				s16 o = vpage_tiles_y_ - pj;
				{
					PR_shift_column_direct(_mi, _mj+o, pi,  0, pi2, pj2);	
					PR_shift_column_direct(_mi, _mj,	pi, pj, pi2, vpage_tiles_y_);
				}
			}
			else		
			{
				// block entirely within virtual playfield			
				PR_shift_column_direct(_mi, _mj, pi, pj, pi2, pj2);
			}
		}
		else
		if (c_updateR)
		{
			// non-v-wrapping case which still requires shadow vpage R (pagerestore mode)

			PR_shift_column_direct(_mi, _mj, pi, _vy1, pi2, _vy2);
		}
		else
		{
			P_shift_column_direct(_mi, _mj, pi, _vy1, pi2, _vy2);
		}
	}


	inline void fill_specrow_virtual(s16 _mi, s16 _mj, s16 _vx1, s16 _vy1, s16 _vx2, s16 pjy1, s16 pjy2)
	{
		if (_vx1 == _vx2)
			return;

		if (!c_hloop)
		{
			// in this mode, rows are never clipped
			fill_specrow_vx_sane(_mi, _mj, _vx1, _vy1, _vx2, pjy1, pjy2);
		}
		else
		{
			// quantize row size into playfield pages
			s16 di1 = quant_vpage_x(_vx1);
			s16 di2 = quant_vpage_x(_vx2-1);
				
			s16 pi = modulo_vpage_x(_vx1);
			
			if (di2 > di1)
			{
				s16 vxR = (di1+1) * vpage_tiles_x_;
				s16 pi2 = vpage_tiles_x_;//modulo_vpage_x(vxR-1)+1;
				
				// split row at first partition
				//fill_specrow_virtual(_mi, _mj, _vx1, _vy1, vxR, pjy1, pjy2);
				fill_specrow_vx_sane(_mi, _mj, pi, _vy1, pi2, pjy1, pjy2);
				fill_specrow_virtual(_mi+(vxR-_vx1), _mj, vxR, _vy1, _vx2, pjy1, pjy2);
			}
			else
			{
				s16 pi2 = modulo_vpage_x(_vx2-1)+1;
				fill_specrow_vx_sane(_mi, _mj, pi, _vy1, pi2, pjy1, pjy2);
			}
		}
	}

	inline void shift_specrow_virtual(s16 _mi, s16 _mj, s16 _vx1, s16 _vy1, s16 _vx2, s16 pjy1, s16 pjy2)
	{
		if (_vx1 == _vx2)
			return;

		// in this mode, rows are never clipped
		shift_specrow_vx_sane(_mi, _mj, _vx1, _vy1, _vx2, pjy1, pjy2);
	}

	inline void fill_speccolumn_virtual(s16 _mi, s16 _mj, s16 _vx1, s16 _vy1, s16 _vy2, s16 pjy1, s16 pjy2)
	{
		if (_vy1 == _vy2)
			return;

		if (!c_vloop)
		{
			// in this mode, columns are never clipped
			fill_speccolumn_vy_sane(_mi, _mj, _vx1, _vy1, _vy2, pjy1, pjy2);
		}
		else
		{
			// quantize row size into playfield pages
			s16 di1 = quant_vpage_y(_vy1);
			s16 di2 = quant_vpage_y(_vy2-1);
			
			s16 pj = modulo_vpage_y(_vy1);
					
			if (di2 > di1)
			{
				s16 vyR = (s16(di1)+1) * vpage_tiles_y_;
				s16 pj2 = vpage_tiles_y_;//modulo_vpage_y(vyR-1)+1;
							
				// split row at first partition
				//fill_speccolumn_virtual(_mi, _mj, _vx1, _vy1, vyR, pjy1, pjy2);
				fill_speccolumn_vy_sane(_mi, _mj, 	_vx1, pj, pj2, 					pjy1, pjy2);
				fill_speccolumn_virtual(_mi, _mj+(vyR-_vy1), _vx1, vyR, _vy2, pjy1, pjy2);
			}
			else
			{
				s16 pj2 = modulo_vpage_y(_vy2-1)+1;
				fill_speccolumn_vy_sane(_mi, _mj, 	_vx1, pj, pj2, 					pjy1, pjy2);			
			}
		}
	}

	inline void shift_speccolumn_virtual(s16 _mi, s16 _mj, s16 _vx1, s16 _vy1, s16 _vy2, s16 pjy1, s16 pjy2)
	{
		if (_vy1 == _vy2)
			return;

		if (!c_vloop)
		{
			// in this mode, columns are never clipped
			shift_speccolumn_vy_sane(_mi, _mj, _vx1, _vy1, _vy2, pjy1, pjy2);
		}
		else
		{
			// quantize row size into playfield pages
			s16 di1 = quant_vpage_y(_vy1);
			s16 di2 = quant_vpage_y(_vy2-1);
			
			s16 pj = modulo_vpage_y(_vy1);
					
			if (di2 > di1)
			{
				s16 vyR = (s16(di1)+1) * vpage_tiles_y_;
				s16 pj2 = vpage_tiles_y_;//modulo_vpage_y(vyR-1)+1;
							
				// split row at first partition
				//fill_speccolumn_virtual(_mi, _mj, _vx1, _vy1, vyR, pjy1, pjy2);
				shift_speccolumn_vy_sane(_mi, _mj, 	_vx1, pj, pj2, 					pjy1, pjy2);
				shift_speccolumn_virtual(_mi, _mj+(vyR-_vy1), _vx1, vyR, _vy2, pjy1, pjy2);
			}
			else
			{
				s16 pj2 = modulo_vpage_y(_vy2-1)+1;
				shift_speccolumn_vy_sane(_mi, _mj, 	_vx1, pj, pj2, 					pjy1, pjy2);			
			}
		}
	}

	inline void fill_block_virtual(s16 _mi, s16 _mj, s16 _vx1, s16 _vy1, s16 _vx2, s16 _vy2)
	{
		if (_vx1 == _vx2)
			return;

		if (!c_hloop)
		{
			// in this mode, rows are never clipped
			fill_block_vx_sane(_mi, _mj, _vx1, _vy1, _vx2, _vy2);
		}
		else
		{
			// start modulo virtual playfield dimensions
			//s16 pi = xmod<s16>(_vx1, vpage_tiles_x_);
			s16 pi = modulo_vpage_x(_vx1);
			// end modulo virtual playfield dimensions
			//s16 pi2 = xmod<s16>(_vx2-1, vpage_tiles_x_)+1;
			s16 pi2 = modulo_vpage_x(_vx2-1)+1;
			
			// ordering faults indicate playfield wrap @ destination, so we split the fill
			// into sub-blocks, potentially on both axes at once

			if (pi2 <= pi)
			{				
				// block straddles virtual x-wrap, so split in two
				s16 o = vpage_tiles_x_ - pi;				
				fill_block_vx_sane(_mi+o, _mj,  0, _vy1, pi2, 		   _vy2);
				fill_block_vx_sane(_mi,   _mj, pi, _vy1, vpage_tiles_x_, _vy2);
			}
			else
			{
				// block entirely within virtual playfield
				fill_block_vx_sane(_mi, _mj, pi, _vy1, pi2, _vy2);
			}
		}
	}

	inline void dirty_block_virtual(s16 _mi, s16 _mj, s16 _vx1, s16 _vy1, s16 _vx2, s16 _vy2)
	{
		if (_vx1 == _vx2)
			return;

		if (!c_hloop)
		{
			// in this mode, rows are never clipped
			dirty_block_vx_sane(_mi, _mj, _vx1, _vy1, _vx2, _vy2);
		}
		else
		{
			// start modulo virtual playfield dimensions
			//s16 pi = xmod<s16>(_vx1, vpage_tiles_x_);
			s16 pi = modulo_vpage_x(_vx1);
			// end modulo virtual playfield dimensions
			//s16 pi2 = xmod<s16>(_vx2-1, vpage_tiles_x_)+1;
			s16 pi2 = modulo_vpage_x(_vx2-1)+1;
			
			// ordering faults indicate playfield wrap @ destination, so we split the fill
			// into sub-blocks, potentially on both axes at once

			if (pi2 <= pi)
			{				
				// block straddles virtual x-wrap, so split in two
				s16 o = vpage_tiles_x_ - pi;				
				dirty_block_vx_sane(_mi+o, _mj,  0, _vy1, pi2, 		   _vy2);
				dirty_block_vx_sane(_mi,   _mj, pi, _vy1, vpage_tiles_x_, _vy2);
			}
			else
			{
				// block entirely within virtual playfield
				dirty_block_vx_sane(_mi, _mj, pi, _vy1, pi2, _vy2);
			}
		}
	}


	inline void shift_block_virtual(s16 _mi, s16 _mj, s16 _vx1, s16 _vy1, s16 _vx2, s16 _vy2)
	{
		if (_vx1 == _vx2)
			return;

		if (!c_hloop)
		{
			// in this mode, rows are never clipped
			shift_block_vx_sane(_mi, _mj, _vx1, _vy1, _vx2, _vy2);
		}
		else
		{
			// start modulo virtual playfield dimensions
			//s16 pi = xmod<s16>(_vx1, vpage_tiles_x_);
			s16 pi = modulo_vpage_x(_vx1);
			// end modulo virtual playfield dimensions
			//s16 pi2 = xmod<s16>(_vx2-1, vpage_tiles_x_)+1;
			s16 pi2 = modulo_vpage_x(_vx2-1)+1;
			
			// ordering faults indicate playfield wrap @ destination, so we split the fill
			// into sub-blocks, potentially on both axes at once

			if (pi2 <= pi)
			{				
				// block straddles virtual x-wrap, so split in two
				s16 o = vpage_tiles_x_ - pi;				
				shift_block_vx_sane(_mi+o, _mj,  0, _vy1, pi2, 		   _vy2);
				shift_block_vx_sane(_mi,   _mj, pi, _vy1, vpage_tiles_x_, _vy2);
			}
			else
			{
				// block entirely within virtual playfield
				shift_block_vx_sane(_mi, _mj, pi, _vy1, pi2, _vy2);
			}
		}
	}

	inline void shift_columns_virtual(s16 _mi, s16 _mj, s16 _vx1, s16 _vy1, s16 _vx2, s16 _vy2)
	{
		if (_vx1 == _vx2)
			return;

		if (!c_hloop)
		{
			// in this mode, rows are never clipped
			shift_column_vx_sane(_mi, _mj, _vx1, _vy1, _vx2, _vy2);
		}
		else
		{
			// start modulo virtual playfield dimensions
			//s16 pi = xmod<s16>(_vx1, vpage_tiles_x_);
			s16 pi = modulo_vpage_x(_vx1);
			// end modulo virtual playfield dimensions
			//s16 pi2 = xmod<s16>(_vx2-1, vpage_tiles_x_)+1;
			s16 pi2 = modulo_vpage_x(_vx2-1)+1;
			
			// ordering faults indicate playfield wrap @ destination, so we split the fill
			// into sub-blocks, potentially on both axes at once

			if (pi2 <= pi)
			{				
				// block straddles virtual x-wrap, so split in two
				s16 o = vpage_tiles_x_ - pi;				
				shift_column_vx_sane(_mi+o, _mj,  0, _vy1, pi2, 		   _vy2);
				shift_column_vx_sane(_mi,   _mj, pi, _vy1, vpage_tiles_x_, _vy2);
			}
			else
			{
				// block entirely within virtual playfield
				shift_column_vx_sane(_mi, _mj, pi, _vy1, pi2, _vy2);
			}
		}
	}

#define fill_rows_virtual fill_block_virtual
#define fill_columns_virtual fill_block_virtual

playfield_template()
	: pmapcontext_(0),
	  display_width_(0), display_height_(0),
	  virtual_width_(0), virtual_height_(0),
	  visible_tiles_x_(0), visible_tiles_y_(0),
	  virtual_tiles_x_(0), virtual_tiles_y_(0),
	  vpage_tiles_x_(0), vpage_tiles_y_(0),
	  scrollmargin_tiles_x_(0), scrollmargin_tiles_y_(0),
	  wrap_width_(0), wrap_height_(0),
	  bufferwords_(0),
	  display_alloc_(0), display_(0),
	  buffer_(0), field_(0),
	  xpos_(0), ypos_(0), last_xpos_(0), last_ypos_(0),
	  txpos_(0), last_txpos_(0),
	  typos_(0), last_typos_(0),
	  ty_last_dx_(0), tx_last_dy_(0),
	  tiles_(0), map_(0), map_tiles_x_(0), map_tiles_y_(0),
	  tilecount_(0),
	  quant_vpage_x_(0), quant_vpage_y_(0),
	  modulo_vpage_x_(0), modulo_vpage_y_(0),
	  modulo_map_x_(0), modulo_map_y_(0),
	  mul_map_x_(0),
	  xpreshift_(0), drawshift_(1), 
	  slice_(0), last_slice_(0),
	  dirtytiles_(0),
	  dirtynum_(0),
	  dirtymask_(0),
	  dirtymasklines_(0),
	  dirtymaskwidth_(0),
	  dirtymaskstride_(0)
{
}

~playfield_template()
{
	dprintf("destroy playfield\n");
	delete[] display_alloc_;

	if (c_dynamic_)
	{
		delete[] dirtytiles_;
		delete[] dirtymask_;
		delete[] dirtymasklines_;
	}
}

void init(s16 _buffer, s16 _field, s16 _display_width, s16 _display_height, s16 _x, s16 _y)
{
	buffer_ = _buffer;
	field_ = _field;

	// shadow map context info
	modulo_map_x_ = pmapcontext_->modulo_map_x_;
	modulo_map_y_ = pmapcontext_->modulo_map_y_;
	mul_map_x_ = pmapcontext_->mul_map_x_;
	map_tiles_x_ = pmapcontext_->map_tiles_x_;
	map_tiles_y_ = pmapcontext_->map_tiles_y_;
	srcywrap_ = pmapcontext_->srcywrap_;
	map_ = pmapcontext_->map_;

	last_xpos_ = 0;
	last_ypos_ = 0;
	last_txpos_ = 0;
	last_typos_ = 0;
	setpos(_x, _y);

/*	g_tshift = _x & (c_tilesize_-1);

	xpos_ = last_xpos_ = _x;
	ypos_ = last_ypos_ = _y;

	txpos_ =  last_txpos_ = _x / c_tilesize_;
	typos_ =  last_typos_ = _y / c_tilesize_;
*/

	display_width_ = _display_width;
	display_height_ = _display_height;

	// dimensions for tiles potentially visible at any time - active display_ area only
	// notes:
	// STE adds one extra visible word-column when hscroll is active
	// all systems add one extra visible row when vscroll is active
	// vertical height always rounded up to next tile
	visible_tiles_x_ = ((display_width_ + (c_tilesize_-1)) / c_tilesize_) + ((/*c_updatecols &&*/ c_ste_) ? 1 : 0);  // keep horizontal margin for fixed 184 byte line
	visible_tiles_y_ = ((display_height_ + (c_tilesize_-1)) / c_tilesize_) + (c_updaterows ? 1 : 0);

	// dimensions for virtual page size used for tile updating - larger than display area + scroll margin
	vpage_tiles_x_ = visible_tiles_x_ + 2;//(c_hspeculative_ ? 2 : 0); // keep horizontal margin for fixed 184 byte line
	vpage_tiles_y_ = visible_tiles_y_ + (c_vspeculative_ ? 2 : 0);

	pfieldcontext_->prepare(vpage_tiles_x_, vpage_tiles_y_);
	quant_vpage_x_ = pfieldcontext_->quant_vpage_x_;
	quant_vpage_y_ = pfieldcontext_->quant_vpage_y_;
	modulo_vpage_x_ = pfieldcontext_->modulo_vpage_x_; 
	modulo_vpage_y_ = pfieldcontext_->modulo_vpage_y_; 

	scrollmargin_tiles_x_ = c_hspeculative_ ? visible_tiles_x_ : vpage_tiles_x_;
	scrollmargin_tiles_y_ = c_vspeculative_ ? visible_tiles_y_ : vpage_tiles_y_;
	
	// dimensions for virtual playfield  (not same as map dimensions)
	// visible plus row/column update margin, x2 for circular loop
	if (!c_updateQ)
		virtual_tiles_x_ = vpage_tiles_x_;
	else
		virtual_tiles_x_ = vpage_tiles_x_ * 2;

	if (!c_updateR)	
		virtual_tiles_y_ = vpage_tiles_y_;
	else
		virtual_tiles_y_ = vpage_tiles_y_ * 2;
	
	// todo: ?
	wrap_width_ = vpage_tiles_x_ * c_tilesize_;
	wrap_height_ = vpage_tiles_y_ * c_tilesize_;
			
	// pixel dimensions of virtual playfield (not same as map dimensions)
	virtual_width_ = virtual_tiles_x_ * c_tilesize_;
	virtual_height_ = virtual_tiles_y_ * c_tilesize_;

	// words per virtual playfield 'scanline'
	virtual_linewords_ = virtual_tiles_x_ * c_tilelinewords_;
	//visible_linewords_ = visible_tiles_x_ * c_tilelinewords_;
	vpage_linewords_ = vpage_tiles_x_ * c_tilelinewords_;
		
	rowskipwords_ = (c_tilesize_ * virtual_linewords_);
	virtual_linebytes_ = virtual_linewords_ << 1;
	visible_linebytes_ = vpage_linewords_ << 1;
	virtual_pagebytes_ = virtual_linebytes_ * wrap_height_;	
	
	// words per virtual playfield buffer
	if (c_scanwalk)
	{
		// pad by ~100 extra scanlines in scanwalk mode, to eliminate loopback buffer
		bufferwords_ = (virtual_linewords_ * virtual_height_) + (c_max_vt_ * c_tilelinewords_);
	}
	else
		bufferwords_ = virtual_linewords_ * virtual_height_;


	pprintf("init playfield:\n");
#if defined(REPORT_PLAYFIELD_METRICS)
	printf("dw:%d, vw:%d, dtw:%d, vtw%d\n", (int)display_width_, (int)virtual_width_, (int)visible_tiles_x_, (int)virtual_tiles_x_);
	printf("dh:%d, vh:%d, dth:%d, vth%d\n", (int)display_height_, (int)virtual_height_, (int)visible_tiles_y_, (int)virtual_tiles_y_);
	printf("wrw:%d, wrh:%d\n", (int)wrap_width_, (int)wrap_height_);
	printf("c_tilesize_:%d\n", (int)c_tilesize_);
	printf("c_tileplanewords_:%d\n", (int)c_tileplanewords_);
	printf("c_tilelinewords_:%d\n", (int)c_tilelinewords_);
	printf("virtual_linewords_ = %d\n", (int)virtual_linewords_);
	printf("virtual_height_ = %d\n", (int)virtual_height_);
	printf("virtual_pagebytes_ = %d\n", (int)virtual_pagebytes_);
	printf("bufferwords_:%d\n", (int)bufferwords_);
	if (c_scanwalk) printf("scanwalk padding words = %d\n", (int)(c_max_vt_ * c_tilelinewords_));
	printf("c_hmargin_:%d\n", (int)c_hmargin_);
	printf("c_vmargin_:%d\n", (int)c_vmargin_);
#endif

	// todo: can be shared between connected playfields
/*	{	
		s16 xo = c_max_vt_ / vpage_tiles_x_;
		s16 yo = c_max_vt_ / vpage_tiles_y_;
		
		//di1 = ((_vx1 + (vpage_tiles_x_ << 3)) / vpage_tiles_x_) - 8
		delete[] quant_vpage_x_; 
		quant_vpage_x_ = new s16[c_max_vt_<<1];
		for (s16 n = 0; n < c_max_vt_<<1; ++n)
		{
			s16 v = n-c_max_vt_;
			quant_vpage_x_[n] = ((v + (xo * vpage_tiles_x_)) / vpage_tiles_x_) - xo;
		}
	
		delete[] quant_vpage_y_; 
		quant_vpage_y_ = new s16[c_max_vt_<<1];
		for (s16 n = 0; n < c_max_vt_<<1; ++n)
		{
			s16 v = n-c_max_vt_;
			quant_vpage_y_[n] = ((v + (yo * vpage_tiles_y_)) / vpage_tiles_y_) - yo;
		}
		
		delete[] modulo_vpage_x_; 
		modulo_vpage_x_ = new s16[c_max_vt_<<1];
		for (s16 n = 0; n < c_max_vt_<<1; ++n)
		{
			s16 v = n-c_max_vt_;
			modulo_vpage_x_[n] = tmod16(v, vpage_tiles_x_);
		}
		
		delete[] modulo_vpage_y_; 
		modulo_vpage_y_ = new s16[c_max_vt_<<1];
		for (s16 n = 0; n < c_max_vt_<<1; ++n)
		{
			s16 v = n-c_max_vt_;
			modulo_vpage_y_[n] = tmod16(v, vpage_tiles_y_);
		}
	}
*/
	// todo: can be shared between connected playfields
	//{
	//	s16 xo = c_max_vt_ / map_tiles_x_;
	//	s16 yo = c_max_vt_ / map_tiles_y_;
	//		
	//	delete[] modulo_map_x_; 
	//	modulo_map_x_ = new s16[c_max_vt_<<1];
	//	for (s16 n = 0; n < c_max_vt_<<1; ++n)
	//	{
	//		s16 v = n-c_max_vt_;
	//		modulo_map_x_[n] = tmod16(v, map_tiles_x_);
	//	}
	//	
	//	delete[] modulo_map_y_; 
	//	modulo_map_y_ = new s16[c_max_vt_<<1];
	//	for (s16 n = 0; n < c_max_vt_<<1; ++n)
	//	{
	//		s16 v = n-c_max_vt_;
	//		modulo_map_y_[n] = tmod16(v, map_tiles_y_);
	//	}
	//}

	// dirty tile management: per-playfield state

	if (c_dynamic_)
	{
		delete[] dirtytiles_;
		dirtytiles_ = new s32[256];
		dirtynum_ = 0;

		// 1 word per 8 bits of mask (8 unused bits)
		delete[] dirtymask_;
		s32 dmsize = ((map_tiles_x_+7)>>3) * map_tiles_y_;
		dirtymask_ = new s16[dmsize];
//		qmemset(dirtymask_, 0xff, dmsize<<1);
		qmemclr(dirtymask_, dmsize<<1);
		dirtymaskwidth_ = ((map_tiles_x_+7)>>3);
		dirtymaskstride_ = dirtymaskwidth_<<1;

		delete[] dirtymasklines_; 
		dirtymasklines_ = new s16*[map_tiles_y_];
		for (s16 n = 0; n < map_tiles_y_; ++n)
			dirtymasklines_[n] = &dirtymask_[n * dirtymaskwidth_];

		//// test: checker pattern for mask
		//for (s16 y = 0; y < map_tiles_y_; y++)
		//{
		//	for (s16 x = 0; x < map_tiles_x_/4; x++)
		//	{
		//		if (((x^y) & 1) || (x < 2) || (y < 2))
		//		{
		//			s16 xf = x & 7;
		//			s16 xb = x >> 3;

		//			dirtymask_[xb + (y*dirtymaskstride_>>1)] |= (0x8000 >> xf);
		//		}
		//	}
		//}
	}

	delete[] display_alloc_;
	int display_alloc_bytes = (bufferwords_<<1) + 256;
	display_alloc_ = new u8[display_alloc_bytes];
	qmemclr(display_alloc_, display_alloc_bytes);

//	display_ = (u16*)(((u32)(display_alloc_) + (160*14) + 255) & -256);
//	framebuffer_ = display_ + (160*7);

	// the displayservice may steal some invisible lines from the true physical display, resulting
	// in the display starting 'late' in memory. e.g. STF synscroll always steals some lines as does 
	// the 240 line overscan mode on STE. 
	// to compensate for this we allocate the same amount of memory as usual, but we offset the
	// display_ base by the missing line count before alignment, and add it back to the framebuffer
	// address for writing graphics. in this way no memory is wasted.

	s16 displaysvc_consumed_words = g_displayservice_consumed_lines * virtual_linewords_;
	display_ = (u16*)(((u32)(display_alloc_) - (displaysvc_consumed_words<<1) + 255) & -256);
	framebuffer_ = display_ + displaysvc_consumed_words;

//	qmemclr(display_, bufferwords_<<1);
//	clear(display_, bufferwords_);
	//set(display_, bufferwords_, 0xAAAA);
	
	if (c_blitter_)
		prepare_blitter();

#if (1)

	s16 txp = txpos_-c_hmargin_;
	s16 typ = typos_-c_vmargin_;

	if (c_scanwalk && txp < 0)
		txp = 0;

	// prefill all buffers, prepared for wrap
	fill_rows_virtual
	(
		txp, typ,
		txp, typ, 
		txpos_-c_hmargin_ + vpage_tiles_x_, typos_-c_vmargin_ + vpage_tiles_y_
	);

#endif

#if (0)

	s16 txp = txpos_;
	s16 typ = typos_;

	// prefill all buffers, prepared for wrap
	fill_rows_virtual
	(
		txp, typ,
		txp, typ, 
		txp + vpage_tiles_x_, typ + vpage_tiles_y_
	);

#endif

}

// ---------------------------------------------------------------------------------------------------------------------

void fill_abs()
{
	s16 txp = txpos_-c_hmargin_;
	s16 typ = typos_-c_vmargin_;

	if (c_scanwalk && txp < 0)
		txp = 0;

	// prefill all buffers, prepared for wrap
	fill_rows_virtual
	(
		txp, typ,
		txp, typ, 
		txpos_-c_hmargin_ + vpage_tiles_x_, typos_-c_vmargin_ + vpage_tiles_y_
	);

	last_xpos_ = xpos_;
	last_ypos_ = ypos_;
		
	last_txpos_ = txpos_;
	last_typos_ = typos_;
}

void fill()
{		
	// load normal deltas for scrolling >= 1 tile
	s16 tx = txpos_;
	s16 ty = typos_;
	s16 txl = last_txpos_;
	s16 tyl = last_typos_;
	s16 tdx = (tx - txl);
	s16 tdy = (ty - tyl);
	
	if ((tdx == 0) && 
		(tdy == 0))
		return;

	if (c_blitter_)
		prepare_blitter();

	if ((xabs(tdx) >= visible_tiles_x_) || 
		(xabs(tdy) >= visible_tiles_y_))
	{
#ifdef TIMING_RASTERS_MAIN		
		reg16(ffff8240) = 0x400;
#endif		

#if (1)
		s16 txp = txpos_;//-c_hmargin_;
		s16 typ = typos_;//-c_vmargin_;

		//if (c_scanwalk && txp < 0)
		//	txp = 0;

		// prefill all buffers, prepared for wrap
		fill_rows_virtual
		(
			txp, typ,
			txp, typ, 
			txp + vpage_tiles_x_, typ + vpage_tiles_y_
		);
#endif

/*
		// todo: use c_hmargin_, c_vmargin_ for properly configured offsets
	
		s16 xo = xmax(txpos_-c_hmargin_, 0);
		s16 yo = xmax(typos_-c_vmargin_, 0);

		yo = typos_;
*/
/*		// prefill all 4 buffers, prepared for wrap
		fill_rows_virtual
		(
			xo, yo,
			xo, yo, 
			xo + vpage_tiles_x_, yo + vpage_tiles_y_
		);				
*/

#ifdef TIMING_RASTERS_MAIN		
		reg16(ffff8240) = 0xf00;
#endif		

		last_txpos_ = txpos_;
		last_typos_ = typos_;	
		last_xpos_ = xpos_;
		last_ypos_ = ypos_;		
		return;
	}

	//	last_txpos_ = txpos_;
	//	last_typos_ = typos_;	
	//	last_xpos_ = xpos_;
	//	last_ypos_ = ypos_;		
	//return;

	// initial hscroll fill column, assuming no change in y
	s16 hs_fill_y1 = ty;
	s16 hs_fill_y2 = ty + vpage_tiles_y_;
	s16 vs_fill_x1 = tx;
	s16 vs_fill_x2 = tx + vpage_tiles_x_;
	
	//if (tdy > 0)
	//{
	//	// don't ever fill more than a visible screen's worth
	//	if (tdy > vpage_tiles_y_)
	//		tdy = vpage_tiles_y_;

	//	// adjust height of hscrollfill column for change in y
	//	//hs_fill_y2 -= tdy;
	//}			
	//else if (tdy < 0)
	//{
	//	// don't ever fill more than a visible screen's worth
	//	if (tdy < -vpage_tiles_y_)
	//		tdy = -vpage_tiles_y_;

	//	// adjust height of hscrollfill column for change in y
	//	//hs_fill_y1 -= tdy;
	//}

	//if (tdx > 0)
	//{
	//	// don't ever fill more than a visible screen's worth
	//	if (tdx > vpage_tiles_x_)
	//		tdx = vpage_tiles_x_;

	//	// adjust width of vscrollfill row for change in x
	//	vs_fill_x2 -= tdx;
	//}			
	//else if (tdx < 0)
	//{
	//	// don't ever fill more than a visible screen's worth
	//	if (tdx < -vpage_tiles_x_)
	//		tdx = -vpage_tiles_x_;

	//	// adjust width of vscrollfill row for change in x
	//	vs_fill_x1 -= tdx;
	//}

	if (tdy > 0)
	{	
		// fill bottom edge
		fill_rows_virtual(
			vs_fill_x1, (tyl + vpage_tiles_y_),
			vs_fill_x1, (tyl + vpage_tiles_y_), 
			vs_fill_x2, (ty + vpage_tiles_y_)
		);			
	}
	else if (tdy < 0)
	{
		// fill top edge
		fill_rows_virtual(
			vs_fill_x1, ty,
			vs_fill_x1, ty, 
			vs_fill_x2, tyl
		);			
	}

	if (tdx > 0)
	{
		// fill right edge
		fill_columns_virtual(
			(txl + vpage_tiles_x_), hs_fill_y1, 
			(txl + vpage_tiles_x_), hs_fill_y1, 
			(tx + vpage_tiles_x_), hs_fill_y2
		);
	}
	else if (tdx < 0)
	{
		// fill left edge
		fill_columns_virtual(
			tx, hs_fill_y1, 
			tx, hs_fill_y1, 
			txl, hs_fill_y2
		);
	}

	last_xpos_ = xpos_;
	last_ypos_ = ypos_;
		
	last_txpos_ = txpos_;
	last_typos_ = typos_;
}

// ---------------------------------------------------------------------------------------------------------------------

void speculative_fill()
{
	s16 dx = (xpos_ - last_xpos_);
	s16 dy = (ypos_ - last_ypos_);

	if ((dx == 0) && 
		(dy == 0))
	{
		return;
	}

	if (c_blitter_)
		prepare_blitter();

/*	
	s16 dxsign = 0;
	if (dx > 0)
		dxsign = 1;
	else if (dx < 0)
		dxsign = -1;

	s16 dysign = 0;
	if (dy > 0)
		dysign = 1;
	else if (dy < 0)
		dysign = -1;
*/				
	s16 lxf = last_xpos_ & (c_tilesize_-1);
	s16 lyf = last_ypos_ & (c_tilesize_-1);
	s16 xf = xpos_ & (c_tilesize_-1);
	s16 yf = ypos_ & (c_tilesize_-1);
	
/*
	// round up tile delta for any nonzero movement so we can draw some part of a row
			
	if (dx > 0)
		tdx = (((dx + (c_tilesize_ - 1)) & -c_tilesize_) >> 4);
	else
		tdx = -(((-dx + (c_tilesize_ - 1)) & -c_tilesize_) >> 4);

	if (dy > 0)
		tdy = (((dy + (c_tilesize_ - 1)) & -c_tilesize_) >> 4);
	else
		tdy = -(((-dy + (c_tilesize_ - 1)) & -c_tilesize_) >> 4);
*/			
			
	s16 tx = txpos_;
	s16 ty = typos_;
	s16 txl = last_txpos_;
	s16 tyl = last_typos_;
	s16 tdx = (tx - txl);
	s16 tdy = (ty - tyl);

	if ((xabs(tdx) >= visible_tiles_x_) || 
		(xabs(tdy) >= visible_tiles_y_))
	{
#ifdef TIMING_RASTERS_MAIN		
		reg16(ffff8240) = 0x400;
#endif		
//		prepare_blitter();

		// prefill all 4 buffers, prepared for wrap
/*		fill_rows_virtual
		(
			txpos_-1, typos_-1,
			txpos_-1, typos_-1, 
			txpos_-1 + vpage_tiles_x_, typos_-1 + vpage_tiles_y_
		);				
*/

		s16 txp = txpos_-c_hmargin_;
		s16 typ = typos_-c_vmargin_;

		if (c_scanwalk && txp < 0)
			txp = 0;

		// prefill all buffers, prepared for wrap
		fill_rows_virtual
		(
			txp, typ,
			txp, typ, 
			txpos_-c_hmargin_ + vpage_tiles_x_, typos_-c_vmargin_ + vpage_tiles_y_
		);

#ifdef TIMING_RASTERS_MAIN		
		reg16(ffff8240) = 0xf00;
#endif		

		last_txpos_ = txpos_;
		last_typos_ = typos_;	
		last_xpos_ = xpos_;
		last_ypos_ = ypos_;		
		return;
	}
		
	// speculative fill column/row, assuming one axis at a time
//		s16 hs_fill_y1 = ty + ((dysign < 0) ? dysign : 0);
//		s16 hs_fill_y2 = ty + vpage_tiles_y_ + ((dysign > 0) ? dysign : 0);
//		s16 vs_fill_x1 = tx + ((dxsign < 0) ? dxsign : 0);
//		s16 vs_fill_x2 = tx + vpage_tiles_x_ + ((dxsign > 0) ? dxsign : 0);

	s16 x1 = tx;
	s16 y1 = ty;
	s16 x2 = tx + visible_tiles_x_;
	s16 y2 = ty + visible_tiles_y_;

	s16 hs_fill_y1 = y1;
	s16 hs_fill_y2 = y2;
	s16 vs_fill_x1 = x1;
	s16 vs_fill_x2 = x2;
	
#if !(SPECULATIVE_FILL_METHOD-0)
	// method 0: easy but expensive...
	// use longer rows/columns which overlap at ends to cover missed speculations at corners
	hs_fill_y1 -= 1;
	hs_fill_y2 += 1;
	vs_fill_x1 -= 1;
	vs_fill_x2 += 1;
#endif

/*		
	if (tdy > 0)
	{
		// don't ever fill more than a visible screen's worth
		if (tdy > vpage_tiles_y_)
			tdy = vpage_tiles_y_;

		// adjust height of hscrollfill column for change in y
		//hs_fill_y2 -= tdy;
	}			
	else if (tdy < 0)
	{
		// don't ever fill more than a visible screen's worth
		if (tdy < -vpage_tiles_y_)
			tdy = -vpage_tiles_y_;

		// adjust height of hscrollfill column for change in y
		//hs_fill_y1 -= tdy;
	}
*/

	if (dx > 0)
	{
		//vs_fill_x2 += 1;
	}			
	else if (dx < 0)
	{
		//vs_fill_x1 -= 1;
	}
/*
	if (dy > 0)
	{
		// adjust width of vscrollfill row for change in x
		hs_fill_y2 += 1;
	}			
	else if (dy < 0)
	{
		// adjust width of vscrollfill row for change in x
		hs_fill_y1 -= 1;
	}
*/

//	prepare_blitter();

	if (dy > 0)
	{	
		// fill bottom edge

#if !(SPECULATIVE_FILL_METHOD-1)
		// calculate portion which may have missed previous speculative updates
		if (tx_last_dy_ < tx)
		{
			// 00000000*
			// 00000000*
			// 00000000*
			// 00000000*
			// *****XXXX
			
			// we have scrolled right since the last DY
			// right tiles may be spoiled
			
			s16 patch_x1 = tx_last_dy_ + visible_tiles_x_;
			s16 patch_x2 = x2 + 1;
			s16 patch_y = y2;
			
			fill_specrow_virtual(
				patch_x1, patch_y,
				patch_x1, patch_y, 
				patch_x2,
				0, 16//lyf, lyf+dy
			);			
		}
		else if (tx_last_dy_ > tx) // patch lower-left diagonal
		{
			// *00000000
			// *00000000
			// *00000000
			// *00000000
			// XXXX*****

			// we have scrolled left since the last DX
			// left tiles may be spoiled
			
			s16 patch_x1 = x1 - 1;
			s16 patch_x2 = tx_last_dy_;
			s16 patch_y = y2;
						
			fill_specrow_virtual(
				patch_x1, patch_y,
				patch_x1, patch_y, 
				patch_x2,
				0, 16//lyf, lyf+dy
			);			
		}
		else
		{
			// no change on other axis - speculation should be correct
		}
#endif
					
		// 00000000
		// 00000000
		// 00000000
		// 00000000
		// XXXXXXXX

		// amortize any single-pixel remainder to minimize double-column updates
		if (lyf == 15)
			lyf = 16;
		if (yf == 15)
			yf = 16;

		if (yf == lyf)
		{
		}									
		else if (yf > lyf)
		{
			fill_specrow_virtual(
				vs_fill_x1, y2,
				vs_fill_x1, y2, 
				vs_fill_x2,
				lyf, yf	//lyf+dy
			);	
		}
		else
		{		
			if (lyf < c_tilesize_)
			fill_specrow_virtual(
				vs_fill_x1, y2 - 1,
				vs_fill_x1, y2 - 1, 
				vs_fill_x2, 
				lyf, c_tilesize_
			);

			if (yf > 0)	
			fill_specrow_virtual(
				vs_fill_x1, y2,
				vs_fill_x1, y2, 
				vs_fill_x2, 
				0, yf
			);														
		}		
	}
	else if (dy < 0)
	{
#if !(SPECULATIVE_FILL_METHOD-1)
		// calculate portion which may have missed previous speculative updates
		if (tx_last_dy_ < tx)
		{
			// *****XXXX
			// 00000000*
			// 00000000*
			// 00000000*
			// 00000000*
						
			s16 patch_x1 = tx_last_dy_ + visible_tiles_x_;
			s16 patch_x2 = x2 + 1;
			s16 patch_y = y1 - 1;
									
			// we have scrolled right since the last DX
			// right tiles may be spoiled
			fill_specrow_virtual(
				patch_x1, patch_y,
				patch_x1, patch_y, 
				patch_x2,
				0, 16//yf, yf-dy
			);	
		}
		else if (tx_last_dy_ > tx)	// patch upper-left diagonal
		{
			// XXXX*****
			// *00000000
			// *00000000
			// *00000000
			// *00000000

			s16 patch_x1 = x1 - 1;
			s16 patch_x2 = tx_last_dy_;
			s16 patch_y = y1 - 1;			
						
			// we have scrolled left since the last DX
			// left tiles may be spoiled
			fill_specrow_virtual(
				patch_x1, patch_y,
				patch_x1, patch_y, 
				patch_x2,
				0, 16//yf, yf-dy
			);		
		}
		else
		{
			// no change on other axis - speculation should be correct
		}	
#endif
		
		// XXXXXXXX
		// 00000000
		// 00000000
		// 00000000
		// 00000000

		// amortize any single-pixel remainder to minimize double-column updates
		if (lyf == 1)
			lyf = 0;
		if (yf == 1)
			yf = 0;

		if (yf == lyf)
		{
		}									
		else if (yf < lyf)
		{
			fill_specrow_virtual(
				vs_fill_x1, y1 - 1,
				vs_fill_x1, y1 - 1, 
				vs_fill_x2,
				yf, lyf	//yf-dy
			);	
		}
		else
		{		
			if (lyf > 0)	
			fill_specrow_virtual(
				vs_fill_x1, y1,
				vs_fill_x1, y1, 
				vs_fill_x2, 
				0, lyf
			);

			if (yf < c_tilesize_)	
			fill_specrow_virtual(
				vs_fill_x1, y1 - 1,
				vs_fill_x1, y1 - 1, 
				vs_fill_x2,
				yf, c_tilesize_
			);														
		}		
		
		
		// fill top edge
//			fill_rows_virtual(
//				vs_fill_x1, ty,
//				vs_fill_x1, ty, 
//				vs_fill_x2, tyl
//			);			
	}

//	prepare_blitter();

	if (dx > 0)
	{		
		// fill right edge
		
#if !(SPECULATIVE_FILL_METHOD-1)
		// calculate portion which may have missed previous speculative updates
		if (ty_last_dx_ < ty)
		{
			// 00000000*
			// 00000000*
			// 00000000X
			// 00000000X
			// ********X

			s16 patch_x = x2;
			s16 patch_y1 = ty_last_dx_ + visible_tiles_y_;
			s16 patch_y2 = y2 + 1;
					
			// we have scrolled down since the last DX
			// lower tiles may be spoiled
			fill_speccolumn_virtual(
				patch_x, patch_y1, 
				patch_x, patch_y1, patch_y2,
				0, 16//lxf+dx
			);				
		}
		else if (ty_last_dx_ > ty)
		{
			// ********X
			// 00000000X
			// 00000000X
			// 00000000*
			// 00000000*
			
			s16 patch_x = x2;
			s16 patch_y1 = y1 - 1;
			s16 patch_y2 = ty_last_dx_;
			
			// we have scrolled up since the last DX
			// upper tiles may be spoiled
			fill_speccolumn_virtual(
				patch_x, patch_y1, 
				patch_x, patch_y1, patch_y2,
				0, 16//lxf+dx
			);				
		}
		else
		{
			// no change on other axis - speculation should be correct
		}
#endif
		
		// 00000000X
		// 00000000X
		// 00000000X
		// 00000000X

		// amortize any single-pixel remainder to minimize double-column updates
		if (lxf == 15)
			lxf = 16;
		if (xf == 15)
			xf = 16;

		if (xf == lxf)
		{
		}
		else if (xf > lxf)
		{
#ifdef TIMING_RASTERS_MAIN
			reg16(ffff8240) = 0x0ff;
#endif		
			fill_speccolumn_virtual(
				x2, hs_fill_y1, 
				x2, hs_fill_y1, hs_fill_y2,
				lxf, xf//lxf+dx
			);		
		}
		else
		{

#ifdef TIMING_RASTERS_MAIN		
			reg16(ffff8240) = 0xfff;
#endif		
			if (lxf < c_tilesize_)	
			fill_speccolumn_virtual(
				x2 - 1, hs_fill_y1, 
				x2 - 1, hs_fill_y1, hs_fill_y2,
				lxf, c_tilesize_
			);		

#ifdef TIMING_RASTERS_MAIN		
			reg16(ffff8240) = 0x444;
#endif		
			if (xf > 0)	
			fill_speccolumn_virtual(
				x2, hs_fill_y1, 
				x2, hs_fill_y1, hs_fill_y2,
				0, xf
			);		

		}			
		
//			fill_columns_virtual(
//				(txl + vpage_tiles_x_), hs_fill_y1, 
//				(txl + vpage_tiles_x_), hs_fill_y1, 
//				(tx + vpage_tiles_x_), hs_fill_y2
//			);

	}
	else if (dx < 0)
	{
	
#if !(SPECULATIVE_FILL_METHOD-1)
		// calculate portion which may have missed previous speculative updates
		if (ty_last_dx_ < ty)
		{
			// *00000000
			// *00000000
			// X00000000
			// X00000000
			// X********

			s16 patch_x = x1 - 1;
			s16 patch_y1 = ty_last_dx_ + visible_tiles_y_;
			s16 patch_y2 = y2 + 1;
									
			// we have scrolled down since the last DX
			// lower tiles may be spoiled
			fill_speccolumn_virtual(
				patch_x, patch_y1, 
				patch_x, patch_y1, patch_y2,
				0, 16//xf-dx
			);				
		}
		else if (ty_last_dx_ > ty)
		{
			// X********
			// X00000000
			// X00000000
			// *00000000
			// *00000000

			s16 patch_x = x1 - 1;
			s16 patch_y1 = y1 - 1;
			s16 patch_y2 = ty_last_dx_;
						
			// we have scrolled up since the last DX
			// upper tiles may be spoiled
			fill_speccolumn_virtual(
				patch_x, patch_y1, 
				patch_x, patch_y1, patch_y2,
				0, 16//xf-dx
			);					
		}
		else
		{
			// no change on other axis - speculation should be correct
		}	
#endif
	
		// X00000000
		// X00000000
		// X00000000
		// X00000000

		// amortize any single-pixel remainder to minimize double-column updates
		if (lxf == 1)
			lxf = 0;
		if (xf == 1)
			xf = 0;

		if (xf == lxf)
		{
		}
		else if (xf < lxf)
		{
#ifdef TIMING_RASTERS_MAIN		
			reg16(ffff8240) = 0xff0;
#endif		
			fill_speccolumn_virtual(
				x1 - 1, hs_fill_y1, 
				x1 - 1, hs_fill_y1, hs_fill_y2,
				xf, lxf	//xf-dx
			);		
		}
		else
		{
#ifdef TIMING_RASTERS_MAIN		
			reg16(ffff8240) = 0xff0;
#endif		
			if (lxf > 0)	
			fill_speccolumn_virtual(
				x1, hs_fill_y1, 
				x1, hs_fill_y1, hs_fill_y2,
				0, lxf
			);		

#ifdef TIMING_RASTERS_MAIN		
			reg16(ffff8240) = 0x440;
#endif		
			if (xf < c_tilesize_)	
			fill_speccolumn_virtual(
				x1 - 1, hs_fill_y1, 
				x1 - 1, hs_fill_y1, hs_fill_y2,
				xf, c_tilesize_
			);		
		}			
		
		
		// fill left edge
//			fill_columns_virtual(
//				tx, hs_fill_y1, 
//				tx, hs_fill_y1, 
//				txl, hs_fill_y2
//			);
	}

	// speculative update requires tracking changes in opposite axis since
	// new tiles appearing on the other axis will have been missed
		
	if ((dy > 0) || (dy < 0))
	{
		tx_last_dy_ = tx;
	}

	if ((dx > 0) || (dx < 0))
	{
		ty_last_dx_ = ty;
	}

#if (0)
//	prepare_blitter();

	if (tdx != 0 || tdy != 0)
	{		
		
	if (tdy > 0)
	{	
		// fill bottom edge
		fill_rows_virtual(
			vs_fill_x1, (tyl + vpage_tiles_y_),
			vs_fill_x1, (tyl + vpage_tiles_y_), 
			vs_fill_x2, (ty + vpage_tiles_y_)
		);			
	}
	else if (tdy < 0)
	{
		// fill top edge
		fill_rows_virtual(
			tx, ty,
			tx, ty, 
			tx + 1, tyl
		);			

		fill_rows_virtual(
			tx + vpage_tiles_x_, ty,
			tx + vpage_tiles_x_, ty, 
			tx + vpage_tiles_x_ + 1, tyl
		);			
	}

	if (tdx > 0)
	{
		// fill right edge
		fill_columns_virtual(
			(txl + vpage_tiles_x_), hs_fill_y1, 
			(txl + vpage_tiles_x_), hs_fill_y1, 
			(tx + vpage_tiles_x_), hs_fill_y2
		);
	}
	else if (tdx < 0)
	{
		// fill left edge
		fill_columns_virtual(
			tx, hs_fill_y1, 
			tx, hs_fill_y1, 
			txl, hs_fill_y2
		);
	}

	last_txpos_ = txpos_;
	last_typos_ = typos_;

	}
#else

	last_txpos_ = txpos_;
	last_typos_ = typos_;

#endif
	
	last_xpos_ = xpos_;
	last_ypos_ = ypos_;
}

// ---------------------------------------------------------------------------------------------------------------------

void hybrid_fill()
{
	s16 dx = (xpos_ - last_xpos_);
	s16 dy = (ypos_ - last_ypos_);
	g_xpreshift = xpreshift_;

	if ((dx == 0) && c_hbufferspeculation_)
		dx = tsign16(last_slice_ - slice_);

	//if ((dx == 0) && 
	//	(dy == 0))
	//{
	//	return;
	//}

	if (c_blitter_)
		prepare_blitter();		

	// obtain tile scroll fragments (remainder of tile-quantized position)
	s16 lxf = last_xpos_ & (c_tilesize_-1);
	s16 lyf = last_ypos_ & (c_tilesize_-1);
	s16 xf = xpos_ & (c_tilesize_-1);
	s16 yf = ypos_ & (c_tilesize_-1);

/*
	// round up tile delta for any nonzero movement so we can draw some part of a row
			
	if (dx > 0)
		tdx = (((dx + (c_tilesize_ - 1)) & -c_tilesize_) >> 4);
	else
		tdx = -(((-dx + (c_tilesize_ - 1)) & -c_tilesize_) >> 4);

	if (dy > 0)
		tdy = (((dy + (c_tilesize_ - 1)) & -c_tilesize_) >> 4);
	else
		tdy = -(((-dy + (c_tilesize_ - 1)) & -c_tilesize_) >> 4);
*/			

	// refresh dirty tiles before incremental updates
	if (c_ste_ && c_dynamic_)
	{
		s16 txp = txpos_-c_hmargin_;
		s16 typ = typos_-c_vmargin_;

		if (c_scanwalk && txp < 0)
			txp = 0;

		dirty_block_virtual
		(
			txp, typ,
			txp, typ, 
			txpos_-(c_hmargin_) + vpage_tiles_x_, 
			typos_-(c_vmargin_) + vpage_tiles_y_
		);
	}

	// shortcut incremental if no movement
	if ((dx == 0) && 
		(dy == 0))
	{
		return;
	}

	// determine draw delta in terms of tile rows/columns
	s16 tx = txpos_;
	s16 ty = typos_;
	s16 tdx = (tx - last_txpos_);
	s16 tdy = (ty - last_typos_);

	// software h-scroll requires shifted tiles to be redrawn on any pixel delta
	if (c_hsoftscroll_)
		tdx = tsign16(dx);

	// 'last' scroll position derived from new tile deltas
	s16 txl = tx - tdx;
	s16 tyl = ty - tdy;

	if ((xabs(tdx) >= visible_tiles_x_) || 
		(xabs(tdy) >= visible_tiles_y_))
	{
#ifdef TIMING_RASTERS_MAIN		
		reg16(ffff8240) = 0x400;
#endif		

		// prefill all buffers, prepared for wrap
#if (1)
		s16 txp = txpos_-c_hmargin_;
		s16 typ = typos_-c_vmargin_;

		if (c_scanwalk && txp < 0)
			txp = 0;

	//	int qqq = 200000; while (qqq--) { };
	
		if (c_ste_)
		fill_block_virtual
		(
			txp, typ,
			txp, typ, 
			txpos_-c_hmargin_ + vpage_tiles_x_, 
			typos_-c_vmargin_ + vpage_tiles_y_
		);
		else
		shift_block_virtual
		(
			txp, typ,
			txp, typ, 
			txpos_-c_hmargin_ + vpage_tiles_x_, 
			typos_-c_vmargin_ + vpage_tiles_y_
		);
#endif
#ifdef TIMING_RASTERS_MAIN		
		reg16(ffff8240) = 0xf00;
#endif		

		if (c_hsyncshift_ && c_updatecols)
		{
			STFM_MaskEdgeL(framebuffer_, xpos_, ypos_, display_height_);
			STFM_MaskEdgeR(framebuffer_, xpos_, ypos_, display_height_);
		}

		last_txpos_ = txpos_;
		last_typos_ = typos_;	
		last_xpos_ = xpos_;
		last_ypos_ = ypos_;		
		last_slice_ = slice_;
		return;
	}














	s16 x1 = tx;
	s16 y1 = ty;
	s16 x2 = tx + scrollmargin_tiles_x_;
	s16 y2 = ty + scrollmargin_tiles_y_;

//	printf("x1:%d, x2:%d\n", x1, x2);
//	printf("y1:%d, y2:%d\n", y1, y2);

	s16 hs_fill_y1 = y1;
	s16 hs_fill_y2 = y2;
	s16 vs_fill_x1 = x1;
	s16 vs_fill_x2 = x2;


	//// initial hscroll fill column, assuming no change in y
	//s16 hs_fill_y1 = ty;
	//s16 hs_fill_y2 = ty + vpage_tiles_y_;
	//s16 vs_fill_x1 = tx;
	//s16 vs_fill_x2 = tx + vpage_tiles_x_;

	//if (!c_vspeculative_)
	//{
	//	if (tdy > 0)
	//	{
	//		// don't ever fill more than a visible screen's worth
	//		if (tdy > vpage_tiles_y_)
	//			tdy = vpage_tiles_y_;

	//		// adjust height of hscrollfill column for change in y
	//		//hs_fill_y2 -= tdy;
	//	}			
	//	else if (tdy < 0)
	//	{
	//		// don't ever fill more than a visible screen's worth
	//		if (tdy < -vpage_tiles_y_)
	//			tdy = -vpage_tiles_y_;

	//		// adjust height of hscrollfill column for change in y
	//		//hs_fill_y1 -= tdy;
	//	}
	//}

	//if (!c_hspeculative_)
	//{
	//	if (tdx > 0)
	//	{
	//		// don't ever fill more than a visible screen's worth
	//		if (tdx > vpage_tiles_x_)
	//			tdx = vpage_tiles_x_;

	//		// adjust width of vscrollfill row for change in x
	//		vs_fill_x2 -= tdx;
	//	}			
	//	else if (tdx < 0)
	//	{
	//		// don't ever fill more than a visible screen's worth
	//		if (tdx < -vpage_tiles_x_)
	//			tdx = -vpage_tiles_x_;

	//		// adjust width of vscrollfill row for change in x
	//		vs_fill_x1 -= tdx;
	//	}
	//}


#if !(SPECULATIVE_FILL_METHOD-0)
	// method 0: easy but expensive...
	// use longer rows/columns which overlap at ends to cover missed speculations at corners
	if (c_vspeculative_ && c_updatecols)
	{
		hs_fill_y1 -= 1;
		hs_fill_y2 += 1;
	}
	if (c_hspeculative_ && c_updaterows)
	{
		vs_fill_x1 -= 1;
		vs_fill_x2 += 1;
	}
#endif

//	printf("x1:%d, x2:%d\n", vs_fill_x1, vs_fill_x2);
//	printf("y1:%d, y2:%d\n", hs_fill_y1, hs_fill_y2);

/*		
	if (tdy > 0)
	{
		// don't ever fill more than a visible screen's worth
		if (tdy > vpage_tiles_y_)
			tdy = vpage_tiles_y_;

		// adjust height of hscrollfill column for change in y
		//hs_fill_y2 -= tdy;
	}			
	else if (tdy < 0)
	{
		// don't ever fill more than a visible screen's worth
		if (tdy < -vpage_tiles_y_)
			tdy = -vpage_tiles_y_;

		// adjust height of hscrollfill column for change in y
		//hs_fill_y1 -= tdy;
	}
*/

	if (c_ste_ || drawshift_)
	if (c_updaterows)
	if (c_vspeculative_)
	{
	if (dy > 0)
	{	
		// fill bottom edge

#if !(SPECULATIVE_FILL_METHOD-1)
		// calculate portion which may have missed previous speculative updates
		if (tx_last_dy_ < tx)
		{
			// 00000000*
			// 00000000*
			// 00000000*
			// 00000000*
			// *****XXXX
			
			// we have scrolled right since the last DY
			// right tiles may be spoiled
			
			s16 patch_x1 = tx_last_dy_ + visible_tiles_x_;
			s16 patch_x2 = x2 + 1;
			s16 patch_y = y2;

			if (c_ste_)
			fill_specrow_virtual(
				patch_x1, patch_y,
				patch_x1, patch_y, 
				patch_x2,
				0, 16//lyf, lyf+dy
			);		
			else
			shift_specrow_virtual(
				patch_x1, patch_y,
				patch_x1, patch_y, 
				patch_x2,
				0, 16//lyf, lyf+dy
			);		
		}
		else if (tx_last_dy_ > tx) // patch lower-left diagonal
		{
			// *00000000
			// *00000000
			// *00000000
			// *00000000
			// XXXX*****

			// we have scrolled left since the last DX
			// left tiles may be spoiled
			
			s16 patch_x1 = x1 - 1;
			s16 patch_x2 = tx_last_dy_;
			s16 patch_y = y2;
						
			if (c_ste_)
			fill_specrow_virtual(
				patch_x1, patch_y,
				patch_x1, patch_y, 
				patch_x2,
				0, 16//lyf, lyf+dy
			);			
			else
			shift_specrow_virtual(
				patch_x1, patch_y,
				patch_x1, patch_y, 
				patch_x2,
				0, 16//lyf, lyf+dy
			);			
		}
		else
		{
			// no change on other axis - speculation should be correct
		}
#endif
					
		// 00000000
		// 00000000
		// 00000000
		// 00000000
		// XXXXXXXX

		// amortize any single-pixel remainder to minimize double-column updates
		if (lyf == 15)
			lyf = 16;
		if (yf == 15)
			yf = 16;

		if (yf == lyf)
		{
		}									
		else if (yf > lyf)
		{
			if (c_ste_)
			fill_specrow_virtual(
				vs_fill_x1, y2,
				vs_fill_x1, y2, 
				vs_fill_x2,
				lyf, yf	//lyf+dy
			);	
			else
			shift_specrow_virtual(
				vs_fill_x1, y2,
				vs_fill_x1, y2, 
				vs_fill_x2,
				lyf, yf	//lyf+dy
			);	
		}
		else
		{		
			if (lyf < c_tilesize_)
			if (c_ste_)
			fill_specrow_virtual(
				vs_fill_x1, y2 - 1,
				vs_fill_x1, y2 - 1, 
				vs_fill_x2, 
				lyf, c_tilesize_
			);
			else
			shift_specrow_virtual(
				vs_fill_x1, y2 - 1,
				vs_fill_x1, y2 - 1, 
				vs_fill_x2, 
				lyf, c_tilesize_
			);

			if (yf > 0)	
			if (c_ste_)
			fill_specrow_virtual(
				vs_fill_x1, y2,
				vs_fill_x1, y2, 
				vs_fill_x2, 
				0, yf
			);		
			else
			shift_specrow_virtual(
				vs_fill_x1, y2,
				vs_fill_x1, y2, 
				vs_fill_x2, 
				0, yf
			);	
		}		
	}
	else if (dy < 0)
	{
#if !(SPECULATIVE_FILL_METHOD-1)
		// calculate portion which may have missed previous speculative updates
		if (tx_last_dy_ < tx)
		{
			// *****XXXX
			// 00000000*
			// 00000000*
			// 00000000*
			// 00000000*
						
			s16 patch_x1 = tx_last_dy_ + visible_tiles_x_;
			s16 patch_x2 = x2 + 1;
			s16 patch_y = y1 - 1;
									
			// we have scrolled right since the last DX
			// right tiles may be spoiled
			if (c_ste_)
			fill_specrow_virtual(
				patch_x1, patch_y,
				patch_x1, patch_y, 
				patch_x2,
				0, 16//yf, yf-dy
			);	
			else
			shift_specrow_virtual(
				patch_x1, patch_y,
				patch_x1, patch_y, 
				patch_x2,
				0, 16//yf, yf-dy
			);	
		}
		else if (tx_last_dy_ > tx)	// patch upper-left diagonal
		{
			// XXXX*****
			// *00000000
			// *00000000
			// *00000000
			// *00000000

			s16 patch_x1 = x1 - 1;
			s16 patch_x2 = tx_last_dy_;
			s16 patch_y = y1 - 1;			
						
			// we have scrolled left since the last DX
			// left tiles may be spoiled
			if (c_ste_)
			fill_specrow_virtual(
				patch_x1, patch_y,
				patch_x1, patch_y, 
				patch_x2,
				0, 16//yf, yf-dy
			);		
			else
			shift_specrow_virtual(
				patch_x1, patch_y,
				patch_x1, patch_y, 
				patch_x2,
				0, 16//yf, yf-dy
			);		
		}
		else
		{
			// no change on other axis - speculation should be correct
		}	
#endif
		
		// XXXXXXXX
		// 00000000
		// 00000000
		// 00000000
		// 00000000

		// amortize any single-pixel remainder to minimize double-column updates
		if (lyf == 1)
			lyf = 0;
		if (yf == 1)
			yf = 0;

		if (yf == lyf)
		{
		}									
		else if (yf < lyf)
		{
			if (c_ste_)
			fill_specrow_virtual(
				vs_fill_x1, y1 - 1,
				vs_fill_x1, y1 - 1, 
				vs_fill_x2,
				yf, lyf	//yf-dy
			);	
			else
			shift_specrow_virtual(
				vs_fill_x1, y1 - 1,
				vs_fill_x1, y1 - 1, 
				vs_fill_x2,
				yf, lyf	//yf-dy
			);	
		}
		else
		{		
			if (lyf > 0)	
			if (c_ste_)
			fill_specrow_virtual(
				vs_fill_x1, y1,
				vs_fill_x1, y1, 
				vs_fill_x2, 
				0, lyf
			);
			else
			shift_specrow_virtual(
				vs_fill_x1, y1,
				vs_fill_x1, y1, 
				vs_fill_x2, 
				0, lyf
			);

			if (yf < c_tilesize_)	
			if (c_ste_)
			fill_specrow_virtual(
				vs_fill_x1, y1 - 1,
				vs_fill_x1, y1 - 1, 
				vs_fill_x2,
				yf, c_tilesize_
			);		
			else
			shift_specrow_virtual(
				vs_fill_x1, y1 - 1,
				vs_fill_x1, y1 - 1, 
				vs_fill_x2,
				yf, c_tilesize_
			);		
		}		
	}
	}
	else
	{
		if (tdy > 0)
		{	
			// fill bottom edge
			if (c_ste_)
			fill_rows_virtual(
				vs_fill_x1, (tyl + vpage_tiles_y_),
				vs_fill_x1, (tyl + vpage_tiles_y_), 
				vs_fill_x2, (ty + vpage_tiles_y_)
			);			
/*			else
			shift_rows_virtual(
				vs_fill_x1, (tyl + vpage_tiles_y_),
				vs_fill_x1, (tyl + vpage_tiles_y_), 
				vs_fill_x2, (ty + vpage_tiles_y_)
			);	*/		
		}
		else if (tdy < 0)
		{
			// fill top edge
			if (c_ste_)
			fill_rows_virtual(
				vs_fill_x1, ty,
				vs_fill_x1, ty, 
				vs_fill_x2, tyl
			);			
/*			else
			shift_rows_virtual(
				vs_fill_x1, ty,
				vs_fill_x1, ty, 
				vs_fill_x2, tyl
			);*/			
		}
	}

	if (c_ste_ || drawshift_)
	if (c_updatecols)
	if (c_hspeculative_ /*|| c_hbufferspeculation_*/)
	{
	if (dx > 0)
	{		
		// fill right edge
		
#if !(SPECULATIVE_FILL_METHOD-1)
		// calculate portion which may have missed previous speculative updates
		if (ty_last_dx_ < ty)
		{
			// 00000000*
			// 00000000*
			// 00000000X
			// 00000000X
			// ********X

			s16 patch_x = x2;
			s16 patch_y1 = ty_last_dx_ + visible_tiles_y_;
			s16 patch_y2 = y2 + 1;
					
			// we have scrolled down since the last DX
			// lower tiles may be spoiled
			//if (c_hbufferspeculation_)
			//shift_speccolumn_virtual(
			//	patch_x, patch_y1, 
			//	patch_x, patch_y1, patch_y2,
			//	0, 16//lxf+dx
			//);				
			//else
			fill_speccolumn_virtual(
				patch_x, patch_y1, 
				patch_x, patch_y1, patch_y2,
				0, 16//lxf+dx
			);				
		}
		else if (ty_last_dx_ > ty)
		{
			// ********X
			// 00000000X
			// 00000000X
			// 00000000*
			// 00000000*
			
			s16 patch_x = x2;
			s16 patch_y1 = y1 - 1;
			s16 patch_y2 = ty_last_dx_;
			
			// we have scrolled up since the last DX
			// upper tiles may be spoiled
			//if (c_hbufferspeculation_)
			//shift_speccolumn_virtual(
			//	patch_x, patch_y1, 
			//	patch_x, patch_y1, patch_y2,
			//	0, 16//lxf+dx
			//);				
			//else
			fill_speccolumn_virtual(
				patch_x, patch_y1, 
				patch_x, patch_y1, patch_y2,
				0, 16//lxf+dx
			);				
		}
		else
		{
			// no change on other axis - speculation should be correct
		}
#endif
		
		// 00000000X
		// 00000000X
		// 00000000X
		// 00000000X

		// amortize any single-pixel remainder to minimize double-column updates
		if (lxf == 15)
			lxf = 16;
		if (xf == 15)
			xf = 16;

		if (xf == lxf)
		{
		}
		else if (xf > lxf)
		{
#ifdef TIMING_RASTERS_MAIN
			reg16(ffff8240) = 0x0ff;
#endif		
			//if (c_hbufferspeculation_)
			//shift_speccolumn_virtual(
			//	x2, hs_fill_y1, 
			//	x2, hs_fill_y1, hs_fill_y2,
			//	lxf, xf//lxf+dx
			//);		
			//else
			fill_speccolumn_virtual(
				x2, hs_fill_y1, 
				x2, hs_fill_y1, hs_fill_y2,
				lxf, xf//lxf+dx
			);		
		}
		else
		{

#ifdef TIMING_RASTERS_MAIN		
			reg16(ffff8240) = 0xfff;
#endif		
			if (lxf < c_tilesize_)	
			//if (c_hbufferspeculation_)
			//shift_speccolumn_virtual(
			//	x2 - 1, hs_fill_y1, 
			//	x2 - 1, hs_fill_y1, hs_fill_y2,
			//	lxf, c_tilesize_
			//);		
			//else
			fill_speccolumn_virtual(
				x2 - 1, hs_fill_y1, 
				x2 - 1, hs_fill_y1, hs_fill_y2,
				lxf, c_tilesize_
			);		

#ifdef TIMING_RASTERS_MAIN		
			reg16(ffff8240) = 0x444;
#endif		
			if (xf > 0)	
			//if (c_hbufferspeculation_)
			//shift_speccolumn_virtual(
			//	x2, hs_fill_y1, 
			//	x2, hs_fill_y1, hs_fill_y2,
			//	0, xf
			//);		
			//else
			fill_speccolumn_virtual(
				x2, hs_fill_y1, 
				x2, hs_fill_y1, hs_fill_y2,
				0, xf
			);		

		}			
	}
	else if (dx < 0)
	{
	
#if !(SPECULATIVE_FILL_METHOD-1)
		// calculate portion which may have missed previous speculative updates
		if (ty_last_dx_ < ty)
		{
			// *00000000
			// *00000000
			// X00000000
			// X00000000
			// X********

			s16 patch_x = x1 - 1;
			s16 patch_y1 = ty_last_dx_ + visible_tiles_y_;
			s16 patch_y2 = y2 + 1;
									
			// we have scrolled down since the last DX
			// lower tiles may be spoiled
			//if (c_hbufferspeculation_)
			//shift_speccolumn_virtual(
			//	patch_x, patch_y1, 
			//	patch_x, patch_y1, patch_y2,
			//	0, 16//xf-dx
			//);				
			//else
			fill_speccolumn_virtual(
				patch_x, patch_y1, 
				patch_x, patch_y1, patch_y2,
				0, 16//xf-dx
			);				
		}
		else if (ty_last_dx_ > ty)
		{
			// X********
			// X00000000
			// X00000000
			// *00000000
			// *00000000

			s16 patch_x = x1 - 1;
			s16 patch_y1 = y1 - 1;
			s16 patch_y2 = ty_last_dx_;
						
			// we have scrolled up since the last DX
			// upper tiles may be spoiled
			//if (c_hbufferspeculation_)
			//shift_speccolumn_virtual(
			//	patch_x, patch_y1, 
			//	patch_x, patch_y1, patch_y2,
			//	0, 16//xf-dx
			//);					
			//else
			fill_speccolumn_virtual(
				patch_x, patch_y1, 
				patch_x, patch_y1, patch_y2,
				0, 16//xf-dx
			);					
		}
		else
		{
			// no change on other axis - speculation should be correct
		}	
#endif
	
		// X00000000
		// X00000000
		// X00000000
		// X00000000

		// amortize any single-pixel remainder to minimize double-column updates
		if (lxf == 1)
			lxf = 0;
		if (xf == 1)
			xf = 0;

		if (xf == lxf)
		{
		}
		else if (xf < lxf)
		{
#ifdef TIMING_RASTERS_MAIN		
			reg16(ffff8240) = 0xff0;
#endif		
			//if (c_hbufferspeculation_)
			//shift_speccolumn_virtual(
			//	x1 - 1, hs_fill_y1, 
			//	x1 - 1, hs_fill_y1, hs_fill_y2,
			//	xf, lxf	//xf-dx
			//);		
			//else
			fill_speccolumn_virtual(
				x1 - 1, hs_fill_y1, 
				x1 - 1, hs_fill_y1, hs_fill_y2,
				xf, lxf	//xf-dx
			);		
		}
		else
		{
#ifdef TIMING_RASTERS_MAIN		
			reg16(ffff8240) = 0xff0;
#endif		
			if (lxf > 0)	
			//if (c_hbufferspeculation_)
			//shift_speccolumn_virtual(
			//	x1, hs_fill_y1, 
			//	x1, hs_fill_y1, hs_fill_y2,
			//	0, lxf
			//);		
			//else
			fill_speccolumn_virtual(
				x1, hs_fill_y1, 
				x1, hs_fill_y1, hs_fill_y2,
				0, lxf
			);		

#ifdef TIMING_RASTERS_MAIN		
			reg16(ffff8240) = 0x440;
#endif		
			if (xf < c_tilesize_)
			//if (c_hbufferspeculation_)
			//shift_speccolumn_virtual(
			//	x1 - 1, hs_fill_y1, 
			//	x1 - 1, hs_fill_y1, hs_fill_y2,
			//	xf, c_tilesize_
			//);		
			//else
			fill_speccolumn_virtual(
				x1 - 1, hs_fill_y1, 
				x1 - 1, hs_fill_y1, hs_fill_y2,
				xf, c_tilesize_
			);		
		}			
	}
	}
	else // c_hspecultive_
	{
		if (tdx > 0)
		{
			// fill right edge
			if (c_ste_)
			fill_columns_virtual(
				(txl + vpage_tiles_x_), hs_fill_y1, 
				(txl + vpage_tiles_x_), hs_fill_y1, 
				(tx  + vpage_tiles_x_), hs_fill_y2
			);
			else
			{
/*				if (c_hbufferspeculation_)
				shift_speccolumn_virtual(
					(txl + vpage_tiles_x_), hs_fill_y1, 
					(txl + vpage_tiles_x_), hs_fill_y1, hs_fill_y2,
					slice_+0, slice_+c_preshift_step_
				);		
				else
*/				shift_columns_virtual(
					(txl + vpage_tiles_x_), hs_fill_y1, 
					(txl + vpage_tiles_x_), hs_fill_y1, 
					(tx  + vpage_tiles_x_), hs_fill_y2
				);
				if (c_hsyncshift_)
					STFM_MaskEdgeR(framebuffer_, xpos_, ypos_, display_height_);
			}
		}
		else if (tdx < 0)
		{
			// fill left edge
			if (c_ste_)
			fill_columns_virtual(
				tx,  hs_fill_y1, 
				tx,  hs_fill_y1, 
				txl, hs_fill_y2
			);
			else
			{
				shift_columns_virtual(
					tx,  hs_fill_y1, 
					tx,  hs_fill_y1, 
					txl, hs_fill_y2
				);

				if (c_hsyncshift_)
					STFM_MaskEdgeL(framebuffer_, xpos_, ypos_, display_height_);
			}
		}
	}

	// erase trailing edge
	if (c_hsyncshift_ && c_updatecols)
	{
		if (dx > 0)
			STFM_MaskEdgeL(framebuffer_, xpos_, ypos_, display_height_);
		else
		if (dx < 0)
			STFM_MaskEdgeR(framebuffer_, xpos_, ypos_, display_height_);
	}


#if !(SPECULATIVE_FILL_METHOD-1)

	// speculative update requires tracking changes in opposite axis since
	// new tiles appearing on the other axis will have been missed

	if (c_hspeculative_ && c_vspeculative_)
	if ((dy > 0) || (dy < 0))
	{
		tx_last_dy_ = tx;
	}

	if (c_hspeculative_ && c_vspeculative_)
	if ((dx > 0) || (dx < 0))
	{
		ty_last_dx_ = ty;
	}
#endif

	last_txpos_ = txpos_;
	last_typos_ = typos_;	
	last_xpos_ = xpos_;
	last_ypos_ = ypos_;
	last_slice_ = slice_;
}

// ---------------------------------------------------------------------------------------------------------------------


//// signal tile @ (x,y) has been modified by editing map array directly
//FORCEINLINE void touch(s16 _x, s16 _y)
//{
//	if (c_dynamic_)
//	{
//		s32 index = mul_map_x_[_y] + _x;
//		xassert(dirtynum_ < 256);
//		dirtytiles_[dirtynum_++] = index;
//	}
//}
//
//// set tile @ (x,y) by index
//FORCEINLINE void mod(s16 _x, s16 _y, s32 _rawidx)
//{
//	if (c_dynamic_)
//	{
//		s32 index = mul_map_x_[_y] + _x;
//		map_[index] = _rawidx;
//		xassert(dirtynum_ < 256);
//		dirtytiles_[dirtynum_++] = index;
//	}
//}

FORCEINLINE void multitouch(s16 _x, s16 _y)
{
	if (c_dynamic_)
	{
		u16 pattern = 0x8000 >> (_x & 7);
		s16 xoffset = (_x >> 3);
		s16 *dirtyline = dirtymasklines_[_y];
		dirtyline[xoffset] |= pattern;
	}
}

void multitouch_row(s16 _x, s16 _y, s16 _dx/*, s32 *_source*/)
{
	if (c_dynamic_)
	{
		// find mask rectangle width
		s16 bx1 = (_x & ~7);
		s16 bx2 = (_x + _dx + 7) & ~7;
		s16 bdx = (bx2 - bx1) >> 3;

		// we can deal with 8,16,24-wide tile bitmasks
		if ((bdx > 0) && (bdx < 4))
		{				
			// access mask buffer
			s16 xoffset = (_x >> 3);
			s16 *dirtyline = dirtymasklines_[_y];
			dirtyline = &dirtyline[xoffset];
			s16 stride = dirtymaskwidth_;

			// generate endmasks for all cases
			u16 em1 = (0xffff >> (_x & 7));
			u16 em3 = (0xffff0000UL >> (((_x - 1) & 7) + 1));

			switch (bdx)
			{
			case 1:
				em1 |= em3;
				dirtyline[0] |= em1;
				break;
			case 2:
				dirtyline[0] |= em1;
				dirtyline[1] |= em3;
				break;
			case 3:
				dirtyline[0] |= em1;
				dirtyline[1] = ~0;
				dirtyline[2] |= em3;
				break;
			// no default:
			}
		}
	}
}

void multitouch_column(s16 _x, s16 _y, s16 _dy/*, s32 *_source*/)
{
	// todo: prototype version - move this stuff to 68k
	if (c_dynamic_)
	{
		// update dirty mask
		u16 pattern = 0x8000 >> (_x & 7);
		s16 xoffset = (_x >> 3);
		s16 *dirtyline = dirtymasklines_[_y];
		dirtyline = &dirtyline[xoffset];
		s16 stride = dirtymaskwidth_;

		s16 y = _dy-1;
		do
		{
			*dirtyline |= pattern;
			dirtyline += stride;
		} while (--y != -1);
	}
}

void multitouch_rect(s16 _x, s16 _y, s16 _dx, s16 _dy/*, s32 *_source*/)
{
	if (c_dynamic_)
	{
		// find mask rectangle width
		s16 bx1 = (_x & ~7);
		s16 bx2 = (_x + _dx + 7) & ~7;
		s16 bdx = (bx2 - bx1) >> 3;

		// we can deal with 8,16,24-wide tile bitmasks
		if ((bdx > 0) && (bdx < 4))
		{				
			// access mask buffer
			s16 xoffset = (_x >> 3);
			s16 *dirtyline = dirtymasklines_[_y];
			dirtyline = &dirtyline[xoffset];
			s16 stride = dirtymaskwidth_;

			// generate endmasks for all cases
			u16 em1 = (0xffff >> (_x & 7));
			u16 em3 = (0xffff0000UL >> (((_x - 1) & 7) + 1));

			// loopsize for all cases
			s16 y = _dy-1;

			switch (bdx)
			{
			case 1:
				em1 |= em3;
				do
				{
					dirtyline[0] |= em1;
					dirtyline += stride;
				} while (--y != -1);
				break;
			case 2:
				do
				{
					dirtyline[0] |= em1;
					dirtyline[1] |= em3;
					dirtyline += stride;
				} while (--y != -1);
				break;
			case 3:
				do
				{
					dirtyline[0] |= em1;
					dirtyline[1] = ~0;
					dirtyline[2] |= em3;
					dirtyline += stride;
				} while (--y != -1);
				break;
			// no default:
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------

#if (0)
void settiles(tileset* _tiles)
{	
	tiles_ = _tiles;
	tiledata_ = tiles_->get();
/*
	prepare_blitter();

	// prefill all 4 buffers, prepared for wrap
	fill_rows_virtual
	(
		txpos_-1, typos_-1,
		txpos_-1, typos_-1, 
		txpos_-1 + vpage_tiles_x_, typos_-1 + visible_tiles_y_
	);	
*/
					
/*		
	if (index_ == 1) return;
			
	for (s16 i = 0; i < virtual_tiles_x_; ++i)
	{
		for (s16 j = 0; j < virtual_tiles_y_; ++j)
		{
			//s16 t = (i + j) & 3;
			//s16 t = twister_.genU32() % 100;

			s16 mi = i % map_tiles_x_;
			s16 mj = j % map_tiles_y_;				
			u16 t = u16(map_[mi + (mj * map_tiles_x_)]);
			
			dotile(t,i,j);
		}
	}
*/		
}
#endif

};

// ---------------------------------------------------------------------------------------------------------------------

#if (0)
typedef playfield_template
<
	/*tilesize=*/4, 
	/*vscroll=*/VScrollMode_NONE, 
	/*hscroll=*/HScrollMode_SCANWALK, 
	/*speculative=*/true, 
	/*ste_hwscroll=*/true, 
	/*stf_hsyncshift=*/false, 
	/*blitter=*/false
> playfield;
#endif

// ---------------------------------------------------------------------------------------------------------------------

#endif // playfield_h_

