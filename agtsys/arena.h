//======================================================================================================================
//	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
// 	world arena: creates world instance from multiple playfield buffers each with separate state
//----------------------------------------------------------------------------------------------------------------------

#ifndef arena_h_
#define arena_h_

// ---------------------------------------------------------------------------------------------------------------------

extern "C" s16 g_linebytes;
extern "C" s16 g_xpreshift;

// ---------------------------------------------------------------------------------------------------------------------
// sprite clipping window (world coordinates)
// todo: this is now part of drawcontext struct and will soon be deprecated

extern "C" s16 g_spr_window_x1;
extern "C" s16 g_spr_window_y1;
extern "C" s16 g_spr_window_x2;
extern "C" s16 g_spr_window_y2;

// ---------------------------------------------------------------------------------------------------------------------
// arena

template<s16 _num_buffers, bool _singleframe, class _playfield_type>
class arena_template
{
public:

	static const int c_num_buffers_ = _num_buffers;
	static const bool c_singleframe = _singleframe;
		
	s16 display_width_;
	s16 display_height_;

	typedef _playfield_type playfield_type;

	// context holds memory resources shared between related playfields
	typename playfield_type::mapcontext mapctx0_;
	typename playfield_type::mapcontext mapctx1_;
	typename playfield_type::fieldcontext pfctx_;
	
	class buffer
	{
	public:
		
		playfield_type fields[c_max_shiftfields];	
	};
		
	buffer buffers_[c_num_buffers_];

	// todo: only single-field mapmod support so far
	s16 **dirtymaskindex_[c_num_buffers_];

	s16 buffer_index_;
	s16 field_index_;
	s16 dualfield_;
	buffer *pactive_;
	s16 xpos_;
	s16 ypos_;

	shifter_common& shift_;
	
	arena_template(shifter_common& _shift, s16 _display_width, s16 _display_height, int _dualfield)
		: shift_(_shift),
		  display_width_(_display_width),
		  display_height_(_display_height),
		  buffer_index_(0), field_index_(0),
		  xpos_(0), ypos_(0),
		  pactive_(0),
		  dualfield_(_dualfield)
	{
		// make sure all playfields share common context
		for (s16 b = 0; b < c_num_buffers_; b++)
		{
			if (c_singleframe)
			{
				// only one field, but still using two buffers to interlace display
				if (b & 1)
				{
					buffers_[b].fields[0].setcontext(&mapctx1_, &pfctx_);
					buffers_[b].fields[1].setcontext(&mapctx1_, &pfctx_);
				}
				else
				{
					buffers_[b].fields[0].setcontext(&mapctx0_, &pfctx_);
					buffers_[b].fields[1].setcontext(&mapctx0_, &pfctx_);
				}
			}
			else
			{
				// separate context for each field
				buffers_[b].fields[0].setcontext(&mapctx0_, &pfctx_);
				buffers_[b].fields[1].setcontext(&mapctx1_, &pfctx_);
			}

			//// shortcut to dirty tile masks
			//dirtymaskindex_[b] = buffers_[b].fields[0].dirtymasklines_;
		}
	}
	
	~arena_template()
	{
		dprintf("destroy arena\n");
	}

	inline void snapview(drawcontext_t& r_ctx, s16 _vpw, s16 _vph)
	{
		buffer& b = *pactive_;
		playfield_type& pf = b.fields[0];

		// generate loopback snapping adjustments
		if (pf.c_hloop_)
			r_ctx.pfsnapadjx = pf.xpos_ - tmod16(pf.xpos_, pf.wrap_width_>>0);
		else
			r_ctx.pfsnapadjx = 0;

		if (pf.c_vloop_)
			r_ctx.pfsnapadjy = pf.ypos_ - tmod16(pf.ypos_, pf.wrap_height_>>0);
		else
			r_ctx.pfsnapadjy = 0;

		// produce workbuffer page coordinates which follow any hops performed by the playfield engine
		drawcontext.vpagex = xpos_ - drawcontext.pfsnapadjx;
		drawcontext.vpagey = ypos_ - drawcontext.pfsnapadjy;

		// generate clipping window
		drawcontext.clip_window_x1 = drawcontext.vpagex & -16;
		drawcontext.clip_window_y1 = drawcontext.vpagey;
		drawcontext.clip_window_x2 = drawcontext.clip_window_x1 + _vpw + 16;
		drawcontext.clip_window_y2 = drawcontext.clip_window_y1 + _vph;

		// todo: remove
		g_spr_window_x1 = drawcontext.clip_window_x1;
		g_spr_window_y1 = drawcontext.clip_window_y1;
		g_spr_window_x2 = drawcontext.clip_window_x2;
		g_spr_window_y2 = drawcontext.clip_window_y2;
	}

	// retrieve digest of draw context fields for low level graphics functions
	inline void get_context(drawcontext_t& r_ctx, s16 _field, s16 _vpw, s16 _vph)
	{
		const playfield_type &pf = buffers_[buffer_index_].fields[_field];

		r_ctx.p_pfframebuffer = pf.framebuffer_;
		r_ctx.p_pflinemod = pf.pfieldcontext_->linemod_;
		r_ctx.p_pflineidx = pf.pfieldcontext_->lineindex_;

		r_ctx.pfvpagebytes = pf.virtual_pagebytes_;
		r_ctx.pfvpageh = pf.wrap_height_;

		snapview(drawcontext, _vpw, _vph);
	}

	void select_buffer(s16 _index, s16 _field)
	{
		buffer_index_ = _index;
		field_index_ = _field;
		pactive_ = &buffers_[buffer_index_];
		playfield_type& pf0 = pactive_->fields[0];

		// publish metrics for writing to this backbuffer
		g_linebytes = pf0.virtual_linewords_ << 1;
		g_xpreshift = pf0.xpreshift_;
	}
	
	void activate_buffer()
	{
		buffer& b = *pactive_; //buffers_[buffer_index_];
		b.fields[0].select(shift_);
		if (dualfield_)
			b.fields[1].select(shift_);
		shift_.advance();
	}

	void modview(s16 &r_x, s16 &r_y)
	{
		buffer& b = *pactive_;
		playfield_type& pf = b.fields[0];

		if (pf.c_hloop_)
			r_x = tmod16(pf.xpos_, pf.wrap_width_>>0);
		if (pf.c_vloop_)
			r_y = tmod16(pf.ypos_, pf.wrap_height_>>0);
	}

	void setpos(s16 _x, s16 _y)
	{
		// track current scroll position
		xpos_ = _x;
		ypos_ = _y;

		buffer& b = *pactive_; //buffers_[buffer_index_];
		b.fields[0].setpos(_x, _y);		
		if (dualfield_)
			b.fields[1].setpos(_x, _y);		
	}
		
	void setmap(s32* _map0, s32* _map1)
	{
		mapctx0_.setmap(_map0);
		mapctx1_.setmap(_map1);
		//buffer& b = buffers_[buffer_index_];
		//b.fields[0].setmap(_map0);
		//if (dualfield_)
		//	b.fields[1].setmap(_map1);
	}
	
	void setmap(worldmap* _map0, worldmap* _map1)
	{
		mapctx0_.setmap(_map0);
		mapctx1_.setmap(_map1);
		//buffer& b = buffers_[buffer_index_];
		//b.fields[0].setmap(_map0);
		//if (dualfield_)
		//	b.fields[1].setmap(_map1);
	}

	void setoverlaymap(s32* _map0, s32* _map1)
	{
		mapctx0_.setoverlaymap(_map0);
		mapctx1_.setoverlaymap(_map1);
	}
	
	void setoverlaymap(worldmap* _map0, worldmap* _map1)
	{
		mapctx0_.setoverlaymap(_map0);
		mapctx1_.setoverlaymap(_map1);
	}

	void settiles(tileset* _tiles0, tileset* _tiles1)
	{
		buffer& b = *pactive_;
		b.fields[0].settiles(_tiles0);
		if (dualfield_)
			b.fields[1].settiles(_tiles1);
	}	

	void setoverlaytiles(tileset* _tiles0, tileset* _tiles1)
	{
		buffer& b = *pactive_;
		b.fields[0].setoverlaytiles(_tiles0);
		if (dualfield_)
			b.fields[1].setoverlaytiles(_tiles1);
	}	

	void setpreshift(s16 _xps)
	{
		buffer& b = *pactive_;
		b.fields[0].setpreshift(_xps);
		if (dualfield_)
			b.fields[1].setpreshift(_xps);
	}

	//

	// todo: prototype version - move this stuff to 68k
	agt_hotcode agt_inline void multitouch(s16 _x, s16 _y)
	{
		if (playfield_type::c_dynamic_)
		{
			u16 pattern = 0x8000 >> (_x & 7);
			s16 xoffset = (_x >> 3);

			if (c_num_buffers_ > 0)
			{
				s16 *dirtyline = (dirtymaskindex_[0])[_y];
				dirtyline[xoffset] |= pattern;	
			}

			if (c_num_buffers_ > 1)
			{
				s16 *dirtyline = (dirtymaskindex_[1])[_y];
				dirtyline[xoffset] |= pattern;	
			}
		}
	}

	// todo: prototype version - move this stuff to 68k
	agt_hotcode void multitouch_row(s16 _x, s16 _y, s16 _dx)
	{
		if (playfield_type::c_dynamic_)
		{
			s16 *dirtyline0, *dirtyline1;
			s16 dirtymaskwidth = buffers_[0].fields[0].dirtymaskwidth_;

			// find mask rectangle width
			s16 bx1 = (_x & ~7);
			s16 bx2 = (_x + _dx + 7) & ~7;
			s16 bdx = (bx2 - bx1) >> 3;

			// we can deal with 8,16,24,32-wide tile bitmasks
			if ((bdx > 0)/* && (bdx < 4)*/)
			{				
				// access mask buffer
				s16 xoffset = (_x >> 3);
				s16 stride = dirtymaskwidth;

				// generate endmasks for all cases
				u16 em1 = (0xffff >> (_x & 7));
				u16 em3 = (0xffff0000UL >> (((_x + _dx - 1) & 7) + 1));

				if (c_num_buffers_ > 0)
					dirtyline0 = (dirtymaskindex_[0])[_y] + xoffset;
				if (c_num_buffers_ > 1)
					dirtyline1 = (dirtymaskindex_[1])[_y] + xoffset;

				switch (bdx)
				{
				case 1:
					em1 &= em3;
					{
						if (c_num_buffers_ > 0)
							dirtyline0[0] |= em1;
						if (c_num_buffers_ > 1)
							dirtyline1[0] |= em1;
					}
					break;
				case 2:
					{
						if (c_num_buffers_ > 0)
						{
							dirtyline0[0] |= em1;
							dirtyline0[1] |= em3;
						}
						if (c_num_buffers_ > 1)
						{
							dirtyline1[0] |= em1;
							dirtyline1[1] |= em3;
						}
					}
					break;
				case 3:
					{
						if (c_num_buffers_ > 0)
						{
							dirtyline0[0] |= em1;
							dirtyline0[1] = ~0;
							dirtyline0[2] |= em3;
						}

						if (c_num_buffers_ > 1)
						{
							dirtyline1[0] |= em1;
							dirtyline1[1] = ~0;
							dirtyline1[2] |= em3;
						}
					}
					break;
				default:
					{
						if (c_num_buffers_ > 0)
							*dirtyline0++ |= em1;
						if (c_num_buffers_ > 1)
							*dirtyline1++ |= em1;

						s16 c = (bdx - 2) - 1;
						do { 
							if (c_num_buffers_ > 0)
								*dirtyline0++ = ~0; 
							if (c_num_buffers_ > 1)
								*dirtyline1++ = ~0; 
						} while (--c != -1);

						if (c_num_buffers_ > 0)
							*dirtyline0++ |= em3;
						if (c_num_buffers_ > 1)
							*dirtyline1++ |= em3;
					}
					break;
				}
			}
		}
	}

	// todo: prototype version - move this stuff to 68k
	agt_hotcode agt_inline void multitouch_column(s16 _x, s16 _y, s16 _dy)
	{
		if (playfield_type::c_dynamic_)
		{
			s16 *dirtyline0, *dirtyline1;
			s16 dirtymaskwidth = buffers_[0].fields[0].dirtymaskwidth_;

			// update dirty mask
			u16 pattern = 0x8000 >> (_x & 7);
			s16 xoffset = (_x >> 3);
			s16 stride = dirtymaskwidth;

			if (c_num_buffers_ > 0)
				dirtyline0 = dirtymaskindex_[0][_y] + xoffset;
			if (c_num_buffers_ > 1)
				dirtyline1 = dirtymaskindex_[1][_y] + xoffset;

			s16 y = _dy-1;
			do
			{
				if (c_num_buffers_ > 0)
				{ *dirtyline0 |= pattern; dirtyline0 += stride; }
				if (c_num_buffers_ > 1)
				{ *dirtyline1 |= pattern; dirtyline1 += stride; }

			} while (--y != -1);
		}
	}

	// todo: prototype version - move this stuff to 68k
	agt_hotcode void multitouch_rect(s16 _x, s16 _y, s16 _dx, s16 _dy)
	{
		if (playfield_type::c_dynamic_)
		{

			s16 *dirtyline0, *dirtyline1;
			s16 dirtymaskwidth = buffers_[0].fields[0].dirtymaskwidth_;

			// find mask rectangle width
			s16 bx1 = (_x & ~7);
			s16 bx2 = (_x + _dx + 7) & ~7;
			s16 bdx = (bx2 - bx1) >> 3;

			// we can deal with 8,16,24,32-wide tile bitmasks
			if ((bdx > 0)/* && (bdx < 4)*/)
			{				
				// access mask buffer
				s16 xoffset = (_x >> 3);
				s16 stride = dirtymaskwidth;

				// generate endmasks for all cases
				u16 em1 = (0xffff >> (_x & 7));
				u16 em3 = (0xffff0000UL >> (((_x + _dx - 1) & 7) + 1));

				if (c_num_buffers_ > 0)
					dirtyline0 = (dirtymaskindex_[0])[_y] + xoffset;
				if (c_num_buffers_ > 1)
					dirtyline1 = (dirtymaskindex_[1])[_y] + xoffset;

				s16 y = _dy-1;

				switch (bdx)
				{
				case 1:	
					em1 &= em3;
					do
					{
						if (c_num_buffers_ > 0)
						{ dirtyline0[0] |= em1; dirtyline0 += stride; }
						if (c_num_buffers_ > 1)
						{ dirtyline1[0] |= em1; dirtyline1 += stride; }

					} while (--y != -1);
					break;
				case 2:
					do
					{
						if (c_num_buffers_ > 0)
						{
							dirtyline0[0] |= em1;
							dirtyline0[1] |= em3;
							dirtyline0 += stride;
						}
						if (c_num_buffers_ > 1)
						{
							dirtyline1[0] |= em1;
							dirtyline1[1] |= em3;
							dirtyline1 += stride;
						}
					} while (--y != -1);
					break;
				case 3:
					do
					{
						if (c_num_buffers_ > 0)
						{
							dirtyline0[0] |= em1;
							dirtyline0[1] = ~0;
							dirtyline0[2] |= em3;
							dirtyline0 += stride;
						}

						if (c_num_buffers_ > 1)
						{
							dirtyline1[0] |= em1;
							dirtyline1[1] = ~0;
							dirtyline1[2] |= em3;
							dirtyline1 += stride;
						}
					} while (--y != -1);
					break;
				default:
					{
						s16 *pd0;
						s16 *pd1;

						s16 cc = (bdx - 2) - 1;
						do
						{
							if (c_num_buffers_ > 0)
							{ pd0 = dirtyline0; dirtyline0 += stride; }
							if (c_num_buffers_ > 1)
							{ pd1 = dirtyline1; dirtyline1 += stride; }

							if (c_num_buffers_ > 0)
								*pd0++ |= em1;
							if (c_num_buffers_ > 1)
								*pd1++ |= em1;

							s16 c = cc;
							do { 							
								if (c_num_buffers_ > 0)
									*pd0++ = ~0; 
								if (c_num_buffers_ > 1)
									*pd1++ = ~0;  
							} while (--c != -1);

							if (c_num_buffers_ > 0)
								*pd0++ |= em3;
							if (c_num_buffers_ > 1)
								*pd1++ |= em3;

						} while (--y != -1);
					}
					break;
				}
			}
		}
	}

#if (0)
	// signal multiple map tiles modified before next refresh: single tile at a time
	// note: no dualfield support yet - will implement shared dualfield maps first
	void multitouch(s16 _x, s16 _y)
	{
		// mark rectangle on all active fields
		s16 b = c_num_buffers_-1;
		do {
			buffer& rb = buffers_[b];
			rb.fields[0].multitouch(_x, _y);
			//if (dualfield_)
			//	rb.fields[1].multitouch(_x, _y);
		} while (--b != -1);		
	}

	// signal multiple map tiles modified before next refresh: map row
	// note: no dualfield support yet - will implement shared dualfield maps first
	void multitouch_row(s16 _x, s16 _y, s16 _dx)
	{
		// mark row on all active fields
		s16 b = c_num_buffers_-1;
		do {
			buffer& rb = buffers_[b];
			rb.fields[0].multitouch_row(_x, _y, _dx);
			//if (dualfield_)
			//	rb.fields[1].multitouch_row(_x, _y, _dx);
		} while (--b != -1);
	}

	// signal multiple map tiles modified before next refresh: map column
	// note: no dualfield support yet - will implement shared dualfield maps first
	void multitouch_column(s16 _x, s16 _y, s16 _dy)
	{
		// mark column  on all active fields
		s16 b = c_num_buffers_-1;
		do {
			buffer& rb = buffers_[b];
			rb.fields[0].multitouch_column(_x, _y, _dy);
			//if (dualfield_)
			//	rb.fields[1].multitouch_column(_x, _y, _dy);
		} while (--b != -1);
	}

	// signal multiple map tiles modified before next refresh: map rectangle
	// note: no dualfield support yet - will implement shared dualfield maps first
	void multitouch_rect(s16 _x, s16 _y, s16 _dx, s16 _dy)
	{
		// mark rect on all active fields
		// todo: 
		s16 b = c_num_buffers_-1;
		do {
			buffer& rb = buffers_[b];
			rb.fields[0].multitouch_rect(_x, _y, _dx, _dy);
			//if (dualfield_)
			//	rb.fields[1].multitouch_rect(_x, _y, _dx, _dy);
		} while (--b != -1);
	}
#endif

	//

	// modify multiple map tiles before next refresh: single tile at a time (using plain index, i.e. not pre-formatted for map)
	// note: no dualfield support yet - will implement shared dualfield maps first
	template<PFLayer _layercode = PFLayer_Base>
	void multimod_idx(s16 _x, s16 _y, s32 _i)
	{
		// update map tiles
		s32 rawidx = _i << (playfield_type::c_tileshift_b_ + (_layercode & PFLayer_Overlay) ? 1 : 0);

		mapctx0_.multimod<_layercode>(_x, _y, rawidx);
//		mapctx1_.multimod(_x, _y, rawidx); // todo: eliminate dualfield maps!

		multitouch(_x, _y);

		// mark element on all active fields
		//s16 b = c_num_buffers_-1;
		//do {
		//	buffer& rb = buffers_[b];
		//	rb.fields[0].multitouch(_x, _y);
		//	//if (dualfield_)
		//	//	rb.fields[1].multitouch(_x, _y);
		//} while (--b != -1);		
	}

	// modify multiple map tiles before next refresh: single tile at a time
	// note: no dualfield support yet - will implement shared dualfield maps first
	template<PFLayer _layercode = PFLayer_Base>
	agt_inline void multimod(s16 _x, s16 _y, s32 _rawidx)
	{
		// update map tiles
		mapctx0_.multimod<_layercode>(_x, _y, _rawidx);
//		mapctx1_.multimod(_x, _y, _rawidx); // todo: eliminate dualfield maps!

		multitouch(_x, _y);

		//// mark element on all active fields
		//s16 b = c_num_buffers_-1;
		//do {
		//	buffer& rb = buffers_[b];
		//	rb.fields[0].multitouch(_x, _y);
		//	//if (dualfield_)
		//	//	rb.fields[1].multitouch(_x, _y);
		//} while (--b != -1);
	}

	// modify multiple map tiles before next refresh: map row
	// note: no dualfield support yet - will implement shared dualfield maps first
	template<PFLayer _layercode = PFLayer_Base>
	void multimod_row(s16 _x, s16 _y, s16 _dx, s32 *_source)
	{
		// update map tiles
		mapctx0_.multimod_row<_layercode>(_x, _y, _dx, _source);
//		mapctx1_.multimod_row(_x, _y, _dx, _source); // todo: eliminate dualfield maps!

		multitouch_row(_x, _y, _dx);

		//// mark row on all active fields
		//s16 b = c_num_buffers_-1;
		//do {
		//	buffer& rb = buffers_[b];
		//	rb.fields[0].multitouch_row(_x, _y, _dx);
		//	//if (dualfield_)
		//	//	rb.fields[1].multitouch_row(_x, _y, _dx);
		//} while (--b != -1);
	}

	// modify multiple map tiles before next refresh: map column
	// note: no dualfield support yet - will implement shared dualfield maps first
	template<PFLayer _layercode = PFLayer_Base>
	void multimod_column(s16 _x, s16 _y, s16 _dy, s32 *_source)
	{
		mapctx0_.multimod_column<_layercode>(_x, _y, _dy, _source);
//		mapctx1_.multimod_column(_x, _y, _dy, _source); // todo: eliminate dualfield maps!

		multitouch_column(_x, _y, _dy);

		//// mark column on all active fields
		//s16 b = c_num_buffers_-1;
		//do {
		//	buffer& rb = buffers_[b];
		//	rb.fields[0].multitouch_column(_x, _y, _dy);
		//	//if (dualfield_)
		//	//	rb.fields[1].multitouch_column(_x, _y, _dy);
		//} while (--b != -1);
	}

	// modify multiple map tiles before next refresh: map rectangle
	// note: no dualfield support yet - will implement shared dualfield maps first
	template<PFLayer _layercode = PFLayer_Base>
	void multimod_rect(s16 _x, s16 _y, s16 _dx, s16 _dy, s32 *_source)
	{
		mapctx0_.multimod_rect<_layercode>(_x, _y, _dx, _dy, _source);
		//mapctx1_.multimod_rect(_x, _y, _dx, _dy, _source); // todo: eliminate dualfield maps!

		multitouch_rect(_x, _y, _dx, _dy);

		//// mark rect on all active fields
		//s16 b = c_num_buffers_-1;
		//do {
		//	buffer& rb = buffers_[b];
		//	rb.fields[0].multitouch_rect(_x, _y, _dx, _dy);
		//	//if (dualfield_)
		//	//	rb.fields[1].multitouch_rect(_x, _y, _dx, _dy);
		//} while (--b != -1);
	}

	//

	void fill_abs(bool _force)
	{
		buffer& b = *pactive_; //buffers_[buffer_index_];

		if (dualfield_)
		{
			if (c_singleframe && !_force)
			{
				b.fields[field_index_].fill_abs();
			}
			else
			{
				b.fields[0].fill_abs();
				b.fields[1].fill_abs();
			}
		}
		else
		{
			b.fields[0].fill_abs();
		}
	}	

	void fill(bool _force)
	{
		buffer& b = *pactive_; //buffers_[buffer_index_];
	
		if (dualfield_)
		{
			if (c_singleframe && !_force)
			{
				b.fields[field_index_].fill();
			}
			else
			{
				b.fields[0].fill();
				b.fields[1].fill();
			}
		}
		else
		{
			b.fields[0].fill();
		}		
		//b.fields[0].fill();
		//if (dualfield_)
		//	b.fields[1].fill();
	}
	
	void speculative_fill(bool _force)
	{
		buffer& b = *pactive_; //buffers_[buffer_index_];

		if (dualfield_)
		{
			if (c_singleframe && !_force)
			{
				b.fields[field_index_].speculative_fill();
			}
			else
			{
				b.fields[0].speculative_fill();
				b.fields[1].speculative_fill();
			}
		}
		else
		{
			b.fields[0].speculative_fill();
		}
//		b.fields[0].speculative_fill();
//		if (dualfield_)
//			b.fields[1].speculative_fill();
	}	

	void hybrid_fill(bool _force)
	{
		buffer& b = *pactive_; //buffers_[buffer_index_];

		if (dualfield_)
		{
			if (c_singleframe && !_force)
			{
				b.fields[field_index_].hybrid_fill();
			}
			else
			{
				b.fields[0].hybrid_fill();
				b.fields[1].hybrid_fill();
			}
		}
		else
		{
			b.fields[0].hybrid_fill();
		}
		//b.fields[0].hybrid_fill();
		//if (dualfield_)
		//	b.fields[1].hybrid_fill();
	}	

	void hybrid_fill_bufferspec()
	{
		buffer& b = *pactive_; //buffers_[buffer_index_];
		s16 x = b.fields[0].xpos_;
		s16 y = b.fields[0].ypos_;

		s16 i = buffer_index_;

		for (s16 s = 0; s < c_num_buffers_; s++)
		{
			buffer& sb = buffers_[i++];
			i &= (c_num_buffers_-1);

			sb.fields[0].setpos(x++,y);
			sb.fields[0].setslice(s);
			sb.fields[0].hybrid_fill();
		}
	}	

	void reset(s16 x, s16 y, bool _nowait = false)
	{
		s16 tb = buffer_index_;
		//s16 tf = field_index_;
		s16 curr_vbl = g_vbl;
		for (int b = c_num_buffers_-1; b >= 0; b--)
		{
			if (!_nowait)
			{
				while (g_vbl == curr_vbl) { /* spin until counter changes */ }
				curr_vbl = g_vbl;
			}

			select_buffer((tb + b) % c_num_buffers_,0);
			setpos(x,y);
			fill_abs(true);

			if (!_nowait)
				activate_buffer();
		}
		//select_buffer(tb,tf);
	}

	void init(s16 x, s16 y)
	{
		for (s16 b = 0; b < c_num_buffers_; ++b)
		{
			buffer& rb = buffers_[b];

//			if (dualfield_)
//			{
//				//if (c_singleframe)
//				//{
//				//	rb.fields[0].init(b, 0, display_width_, display_height_, x, y);
//				//	rb.fields[1].init(b, 1, display_width_, display_height_, x, y);
//				//}
//				//else
//				{
////					b.fields[0].speculative_fill();
////					b.fields[1].speculative_fill();
//					rb.fields[0].init(b, 0, display_width_, display_height_, x, y);
//					rb.fields[1].init(b, 1, display_width_, display_height_, x, y);
//				}
//			}
//			else
//			{
//				rb.fields[0].init(b, 0, display_width_, display_height_, x, y);
//			}

			rb.fields[0].init(b, 0, display_width_, display_height_, x, y);
			if (dualfield_)
				rb.fields[1].init(b, 1, display_width_, display_height_, x, y);

			// shortcut to dirty tile masks
			dirtymaskindex_[b] = buffers_[b].fields[0].dirtymasklines_;
		}
		
		select_buffer(buffer_index_, 0);
		setpos(x,y);
		//activate_buffer();	
	}

		
};

// ---------------------------------------------------------------------------------------------------------------------

#endif // arena_h_
