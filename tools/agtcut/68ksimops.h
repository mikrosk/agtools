//---------------------------------------------------------------------------------------------------------------------
//	[A]tari [G]ame [T]ools / dml 2016
//---------------------------------------------------------------------------------------------------------------------
//	68k sim operations
//---------------------------------------------------------------------------------------------------------------------

class Operation
{
public:

	Operation
	(
		Context &_ctx, 
		OpKind _kind, 
		OpSize _opsize,
		OpSource _opsrc,
		OpSource _opdst,
		OpWrite _opwr,
		int _cost,
		int _tail,
		int _words
	)
		: ctx(_ctx),
		  kind(_kind),
		  opsize(_opsize),
		  opsrc(_opsrc),
		  opdst(_opdst),
		  opwr(_opwr),
		  cost(_cost),
		  tail(_tail),
		  words(_words),
		  sreg(-1),
		  dreg(-1),
		  areg(-1),
		  stagH(-1),
		  stagL(-1),
		  dtagH(-1),
		  dtagL(-1),
		  atag(-1)
	{
	}

	virtual void onExecute() = 0;
	virtual void onEmit() { };

	void execute()
	{
		onExecute();

		// mark parts of registers which become stable for reading/modifying/combining as a result of a replace-write
		if (opdst == OpSource_Register)
		{
			if (opwr == OpWrite_Replace)
			{
				ctx.registers[dreg].stableL = true;
				//ctx.registers[dreg].constant = false;
			}

			if (opsize == OpSize_32)
			{
				if (opwr == OpWrite_Replace || opwr == OpWrite_ReplaceH || opwr == OpWrite_ReplaceHCombineL)
				{
					ctx.registers[dreg].stableH = true;
					//ctx.registers[dreg].constant = false;
				}
			}
		}
	}

	void emit()
	{
		onEmit();
		if (g_allargs.verbose)
			printf("%s", ctx.buf);
		ctx.genout << ctx.buf;
	}

	std::string describe()
	{
		onEmit();
		return std::string(ctx.buf);
	}

	int alltagsH[max_registers];
	int alltagsL[max_registers];

	Context &ctx;
	OpKind kind;
	OpSize opsize;
	OpSource opsrc;
	OpSource opdst;
	OpWrite opwr;

	int sreg;
	int dreg;
	int areg;

	int atag;
	int stagH;
	int stagL;
	int dtagH;
	int dtagL;

	int cost;
	int tail;
	int words;
};

// --------------------------------------------------------------------

class NoOp : public Operation
{
public:

	NoOp(Context &_ctx)
		: Operation(_ctx, OpKind_Nop, OpSize_None, OpSource_None, OpSource_None, OpWrite_None, 4, 0, 1)
	{
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " nop\t;%d\n",
			cost
		);
	}

	virtual void onExecute()
	{
		ctx.cycles += cost;
	}
};

class EM_MoveI16 : public Operation
{
public:

	EM_MoveI16(Context &_ctx, int _em, uword_t _data, int _areg)
		: Operation(_ctx, OpKind_StoreI16, OpSize_16, OpSource_Immediate, OpSource_Memory, OpWrite_Replace, 12, 0, 2),
		  em(_em),
		  data(_data)
	{
		assert(_areg < 256);
		areg = _areg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " move.w #$%04x,%s\t;%d [EM:%d]\n", 
			data, 
			ctx.BLiTEM_names[em].c_str(), 
			cost,
			em+1
		);
	}

	virtual void onExecute()
	{
		ctx.blitter.em[em] = data;// ctx.registers[iLHS].L;
		ctx.cycles += cost;
		ctx.sim_track_datarefs(data);
	}

	int em;
	uword_t data;
};

class EM_MoveI32 : public Operation
{
public:

	EM_MoveI32(Context &_ctx, int _em, uword_t _dataH, uword_t _dataL, int _areg)
		: Operation(_ctx, OpKind_StoreI32, OpSize_32, OpSource_Immediate, OpSource_Memory, OpWrite_Replace, 20, 0, 2),
		  em(_em),
		  dataH(_dataH),
		  dataL(_dataL)
	{
		assert(_areg < 256);
		areg = _areg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " move.l #$%04x%04x,%s\t;%d [EM:%d:%d]\n", 
			dataH, 
			dataL, 
			ctx.BLiTEM_names[em].c_str(), 
			cost,
			em+1,em+2
		);
	}

	virtual void onExecute()
	{
		ctx.blitter.em[em + 0] = dataH;// ctx.registers[iLHS].L;
		ctx.blitter.em[em + 1] = dataL;// ctx.registers[iLHS].L;
		ctx.cycles += cost;
//		ctx.sim_track_datarefs(dataH);
//		ctx.sim_track_datarefs(dataL);
		ctx.sim_track_datarefs(dataH, dataL);
	}

	int em;
	uword_t dataH;
	uword_t dataL;
};

class EM_MoveR16 : public Operation
{
public:

	EM_MoveR16(Context &_ctx, int _em, int _reg, int _areg)
		: Operation(_ctx, OpKind_StoreR16, OpSize_16, OpSource_Register, OpSource_Memory, OpWrite_Replace, 8, 0, 1),
		  em(_em)
	{
		assert(_reg < 256);
		sreg = _reg;
		assert(_areg < 256);
		areg = _areg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " move.w %s,%s\t\t;%d [EM:%d] ($%04x)\n", 
			ctx.registers[sreg].name.c_str(), 
			ctx.BLiTEM_names[em].c_str(), 
			cost,
			em+1,
			ctx.registers[sreg].L);
	}

	virtual void onExecute()
	{
		ctx.blitter.em[em] = ctx.registers[sreg].L;
		ctx.cycles += cost;
		ctx.sim_track_datarefs(ctx.registers[sreg].L);
	}

	int em;
	uword_t data;
};

class EM_MoveR32 : public Operation
{
public:

	EM_MoveR32(Context &_ctx, int _em, int _reg, int _areg)
		: Operation(_ctx, OpKind_StoreR32, OpSize_32, OpSource_Register, OpSource_Memory, OpWrite_Replace, 12, 0, 1),
		  em(_em)
	{
		assert(_reg < 256);
		sreg = _reg;
		assert(_areg < 256);
		areg = _areg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " move.l %s,%s\t\t;%d [EM:%d:%d] ($%04x%04x)\n", 
			ctx.registers[sreg].name.c_str(), 
			ctx.BLiTEM_names[em].c_str(), 
			cost,
			em+1,em+2,
			ctx.registers[sreg].H, ctx.registers[sreg].L
		);
	}

	virtual void onExecute()
	{
		ctx.blitter.em[em + 0] = ctx.registers[sreg].H;
		ctx.blitter.em[em + 1] = ctx.registers[sreg].L;
		ctx.cycles += cost;
//		ctx.sim_track_datarefs(ctx.registers[sreg].H);
//		ctx.sim_track_datarefs(ctx.registers[sreg].L);
		ctx.sim_track_datarefs(ctx.registers[sreg].H, ctx.registers[sreg].L);
	}

	int em;
};

// --------------------------------------------------------------------

class AR_Lea16 : public Operation
{
public:

	AR_Lea16(Context &_ctx, int _reg, uword_t _data)
		: Operation(_ctx, OpKind_Lea16, OpSize_32, OpSource_Immediate, OpSource_Register, OpWrite_Replace, 8, 2, 2),
		  data(_data)
	{
		dreg = _reg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " lea $%04x%04x.w,%s\t;%d\n", 
			uword_t(word_t(data)>>15), data, 
			ctx.registers[dreg].name.c_str(), 
			cost
		);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].H = uword_t(word_t(data) >> 15);
		ctx.registers[dreg].L = data;
		ctx.registers[dreg].allocatedH = true;
		ctx.registers[dreg].allocatedL = true;
		ctx.cycles += cost;
	}

	uword_t data;
};

class AR_Lea32 : public Operation
{
public:

	AR_Lea32(Context &_ctx, int _reg, uword_t _dataH, uword_t _dataL)
		: Operation(_ctx, OpKind_Lea32, OpSize_32, OpSource_Immediate, OpSource_Register, OpWrite_Replace, 12, 0, 3),
		  dataH(_dataH),
		  dataL(_dataL)
	{
		dreg = _reg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " lea $%04x%04x,%s\t;%d\n", 
			dataH, dataL, 
			ctx.registers[dreg].name.c_str(), 
			cost
		);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].H = dataH;
		ctx.registers[dreg].L = dataL;
		ctx.registers[dreg].allocatedH = true;
		ctx.registers[dreg].allocatedL = true;
		ctx.cycles += cost;
	}

	uword_t dataH;
	uword_t dataL;
};

// --------------------------------------------------------------------

class DR_LoadI16 : public Operation
{
public:

	DR_LoadI16(Context &_ctx, int _reg, uword_t _data)
		: Operation(_ctx, OpKind_LoadI16, OpSize_16, OpSource_Immediate, OpSource_Register, OpWrite_Replace, 8, 2, 2),
		  data(_data)
	{
		dreg = _reg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " move.w #$%04x,%s\t;%d\n", 
			data, 
			ctx.registers[dreg].name.c_str(), 
			cost
		);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = data;// ctx.registers[iLHS].L;
		ctx.registers[dreg].allocatedL = true;
		ctx.cycles += cost;
	}

	uword_t data;
};

class DR_Clr8 : public Operation
{
public:

	DR_Clr8(Context &_ctx, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_None, OpSource_Register, OpWrite_Modify, 4, 0, 1)
	{
		dreg = _reg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " clr.b %s\t;%d [$%04x]\n", 
			ctx.registers[dreg].name.c_str(), 
			cost,
			ctx.registers[dreg].L
		);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L &= 0xFF00;
		ctx.cycles += cost;
	}
};

class DR_Clr16 : public Operation
{
public:

	DR_Clr16(Context &_ctx, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_None, OpSource_Register, OpWrite_Replace, 4, 0, 1)
	{
		dreg = _reg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " clr.w %s\t;%d\n", 
			ctx.registers[dreg].name.c_str(), 
			cost
		);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = 0;
		ctx.registers[dreg].allocatedL = true;
		ctx.cycles += cost;
	}
};

class DR_MoveQ32 : public Operation
{
public:

	DR_MoveQ32(Context &_ctx, int _imm, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_32, OpSource_None, OpSource_Register, OpWrite_Replace, 4, 0, 1),
		  imm(_imm)
	{
		dreg = _reg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " moveq.l #%d,%s\t;%d\n", 
			imm,
			ctx.registers[dreg].name.c_str(), 
			cost
		);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].H = ulong_t(imm) >> 16;
		ctx.registers[dreg].L = ulong_t(imm) & 0xFFFF;
		ctx.registers[dreg].allocatedH = true;
		ctx.registers[dreg].allocatedL = true;
		ctx.cycles += cost;
	}

	int imm;
};

class DR_Not32 : public Operation
{
public:

	DR_Not32(Context &_ctx, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_32, OpSource_None, OpSource_Register, OpWrite_Modify, 8, 4, 1)
	{
		sreg = dreg = _reg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " not.l %s\t;%d [$%04x%04x]\n", 
			ctx.registers[dreg].name.c_str(), 
			cost,
			ctx.registers[dreg].H, ctx.registers[dreg].L
		);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].H = ~ctx.registers[dreg].H;
		ctx.registers[dreg].L = ~ctx.registers[dreg].L;
		ctx.cycles += cost;
	}
};

class DR_Not16 : public Operation
{
public:

	DR_Not16(Context &_ctx, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_None, OpSource_Register, OpWrite_Modify, 4, 0, 1)
	{
		sreg = dreg = _reg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " not.w %s\t;%d [$%04x]\n", 
			ctx.registers[dreg].name.c_str(), 
			cost,
			ctx.registers[dreg].L
		);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = ~ctx.registers[dreg].L;
		ctx.cycles += cost;
	}
};

class DR_Not8 : public Operation
{
public:

	DR_Not8(Context &_ctx, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_None, OpSource_Register, OpWrite_Modify, 4, 0, 1)
	{
		sreg = dreg = _reg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " not.b %s\t;%d [$%04x]\n", 
			ctx.registers[dreg].name.c_str(), 
			cost,
			ctx.registers[dreg].L
		);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = ctx.registers[dreg].L ^ 0x00FF;
		ctx.cycles += cost;
	}
};

class DR_Neg32 : public Operation
{
public:

	DR_Neg32(Context &_ctx, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_32, OpSource_None, OpSource_Register, OpWrite_Modify, 8, 4, 1)
	{
		sreg = dreg = _reg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " neg.l %s\t;%d [$%04x%04x]\n", 
			ctx.registers[dreg].name.c_str(), 
			cost,
			ctx.registers[dreg].H, ctx.registers[dreg].L
		);
	}

	virtual void onExecute()
	{
		long_t v = long_t(ctx.registers[dreg].H << 16) | ctx.registers[dreg].L;
		v = -v;
		ctx.registers[dreg].H = uword_t(ulong_t(v >> 16) & 0x0000FFFF);
		ctx.registers[dreg].L = uword_t(ulong_t(v) & 0x0000FFFF);
		ctx.cycles += cost;
	}
};

class DR_Neg16 : public Operation
{
public:

	DR_Neg16(Context &_ctx, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_None, OpSource_Register, OpWrite_Modify, 4, 0, 1)
	{
		sreg = dreg = _reg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " neg.w %s\t;%d [$%04x]\n", 
			ctx.registers[dreg].name.c_str(), 
			cost,
			ctx.registers[dreg].L
		);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = uword_t(-(word_t(ctx.registers[dreg].L)));
		ctx.cycles += cost;
	}
};

class DR_Neg8 : public Operation
{
public:

	DR_Neg8(Context &_ctx, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_None, OpSource_Register, OpWrite_Modify, 4, 0, 1)
	{
		sreg = dreg = _reg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " neg.b %s\t;%d [$%04x]\n", 
			ctx.registers[dreg].name.c_str(), 
			cost,
			ctx.registers[dreg].L
		);
	}

	virtual void onExecute()
	{
		byte_t v = byte_t(ctx.registers[dreg].L & 0x00FF);
		v = -v;
		ctx.registers[dreg].L = (ctx.registers[dreg].L & 0xFF00) | ubyte_t(v);
		ctx.cycles += cost;
	}
};

class DR_AddSubQ32 : public Operation
{
public:

	DR_AddSubQ32(Context &_ctx, int _imm, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_32, OpSource_Immediate, OpSource_Register, OpWrite_Modify, 4, 0, 1),
		  imm(_imm)
	{
		sreg = dreg = _reg;
	}

	virtual void onEmit()
	{
		if (imm > 0)
		{
			sprintf(ctx.buf, " addq.l #%d,%s\t;%d [$%04x%04x]\n",
				imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].H, ctx.registers[dreg].L
				);
		}
		else
		{
			sprintf(ctx.buf, " subq.l #%d,%s\t;%d [$%04x%04x]\n",
				-imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].H, ctx.registers[dreg].L
				);
		}
	}

	virtual void onExecute()
	{
		long_t v = long_t(ctx.registers[dreg].H << 16) | ctx.registers[dreg].L;
		v = v + imm;

		ctx.registers[dreg].H = uword_t(ulong_t(v) >> 16);
		ctx.registers[dreg].L = uword_t(ulong_t(v) & 0xFFFF);;
		ctx.cycles += cost;
	}

	int imm;
};

class DR_AddSubQ16 : public Operation
{
public:

	DR_AddSubQ16(Context &_ctx, int _imm, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Immediate, OpSource_Register, OpWrite_Modify, 4, 0, 1),
		  imm(_imm)
	{
		sreg = dreg = _reg;
	}

	virtual void onEmit()
	{
		if (imm > 0)
		{
			sprintf(ctx.buf, " addq.w #%d,%s\t;%d [$%04x]\n",
				imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
		else
		{
			sprintf(ctx.buf, " subq.w #%d,%s\t;%d [$%04x]\n",
				-imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = uword_t(word_t(ctx.registers[dreg].L) + word_t(imm));
		ctx.cycles += cost;
	}

	int imm;
};

class DR_AddSubQ8 : public Operation
{
public:

	DR_AddSubQ8(Context &_ctx, int _imm, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Immediate, OpSource_Register, OpWrite_Modify, 4, 0, 1),
		  imm(_imm)
	{
		sreg = dreg = _reg;
	}

	virtual void onEmit()
	{
		if (imm > 0)
		{
			sprintf(ctx.buf, " addq.b #%d,%s\t;%d [$%04x]\n",
				imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
		else
		{
			sprintf(ctx.buf, " subq.b #%d,%s\t;%d [$%04x]\n",
				-imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = (ctx.registers[dreg].L & 0xFF00) | 
								(uword_t(ubyte_t(byte_t(ctx.registers[dreg].L & 0x00FF) + byte_t(imm))) & 0x00FF);
		ctx.cycles += cost;
	}

	int imm;
};

class DR_LSI16 : public Operation
{
public:

	DR_LSI16(Context &_ctx, int _imm, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Immediate, OpSource_Register, OpWrite_Modify, 12, 8, 1),
		  imm(_imm)
	{
		sreg = dreg = _reg;
	}

	virtual void onEmit()
	{
		if (imm > 0)
		{
			sprintf(ctx.buf, " lsr.w #%d,%s\t;%d [$%04x]\n",
				imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
		else
		{
			sprintf(ctx.buf, " lsl.w #%d,%s\t;%d [$%04x]\n",
				-imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
	}

	virtual void onExecute()
	{
		if (imm < 0)
			ctx.registers[dreg].L = ctx.registers[dreg].L << (-imm);
		else
			ctx.registers[dreg].L = ctx.registers[dreg].L >> imm;

		ctx.cycles += cost;
	}

	int imm;
};

class DR_LSI32 : public Operation
{
public:

	DR_LSI32(Context &_ctx, int _imm, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_32, OpSource_Immediate, OpSource_Register, OpWrite_Modify, 12, 8, 1),
		  imm(_imm)
	{
		sreg = dreg = _reg;
	}

	virtual void onEmit()
	{
		if (imm > 0)
		{
			sprintf(ctx.buf, " lsr.l #%d,%s\t;%d [$%04x%04x]\n",
				imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].H,
				ctx.registers[dreg].L
				);
		}
		else
		{
			sprintf(ctx.buf, " lsl.l #%d,%s\t;%d [$%04x%04x]\n",
				-imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].H,
				ctx.registers[dreg].L
				);
		}
	}

	virtual void onExecute()
	{
		ulong_t data32 = (ulong_t(ctx.registers[dreg].H) << 16) | ctx.registers[dreg].L;

		if (imm < 0)
			data32 = data32 << (-imm);
		else
			data32 = data32 >> (imm);

		ctx.registers[dreg].H = data32 >> 16;
		ctx.registers[dreg].L = data32 & 0xFFFF;

		ctx.cycles += cost;
	}

	int imm;
};

class DR_ASI16 : public Operation
{
public:

	DR_ASI16(Context &_ctx, int _imm, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Immediate, OpSource_Register, OpWrite_Modify, 12, 8, 1),
		  imm(_imm)
	{
		sreg = dreg = _reg;
	}

	virtual void onEmit()
	{
		if (imm > 0)
		{
			sprintf(ctx.buf, " asr.w #%d,%s\t;%d [$%04x]\n",
				imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
		else
		{
			sprintf(ctx.buf, " asl.w #%d,%s\t;%d [$%04x]\n",
				-imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
	}

	virtual void onExecute()
	{
		if (imm < 0)
			ctx.registers[dreg].L = uword_t(word_t(ctx.registers[dreg].L) << (-imm));
		else
			ctx.registers[dreg].L = uword_t(word_t(ctx.registers[dreg].L) >> imm);

		ctx.cycles += cost;
	}

	int imm;
};

class DR_ASI32 : public Operation
{
public:

	DR_ASI32(Context &_ctx, int _imm, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_32, OpSource_Immediate, OpSource_Register, OpWrite_Modify, 12, 8, 1),
		  imm(_imm)
	{
		sreg = dreg = _reg;
	}

	virtual void onEmit()
	{
		if (imm > 0)
		{
			sprintf(ctx.buf, " asr.l #%d,%s\t;%d [$%04x%04x]\n",
				imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].H,
				ctx.registers[dreg].L
				);
		}
		else
		{
			sprintf(ctx.buf, " asl.l #%d,%s\t;%d [$%04x%04x]\n",
				-imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].H,
				ctx.registers[dreg].L
				);
		}
	}

	virtual void onExecute()
	{
		long_t data32 = long_t((ulong_t(ctx.registers[dreg].H) << 16) | ctx.registers[dreg].L);

		if (imm < 0)
			data32 = data32 << (-imm);
		else
			data32 = data32 >> (imm);

		ctx.registers[dreg].H = long_t(data32) >> 16;
		ctx.registers[dreg].L = long_t(data32) & 0xFFFF;

		ctx.cycles += cost;
	}

	int imm;
};

class DR_ROI16 : public Operation
{
public:

	DR_ROI16(Context &_ctx, int _imm, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Immediate, OpSource_Register, OpWrite_Modify, 12, 8, 1),
		  imm(_imm)
	{
		sreg = dreg = _reg;
	}

	virtual void onEmit()
	{
		if (imm > 0)
		{
			sprintf(ctx.buf, " ror.w #%d,%s\t;%d [$%04x]\n",
				imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
		else
		{
			sprintf(ctx.buf, " rol.w #%d,%s\t;%d [$%04x]\n",
				-imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
	}

	virtual void onExecute()
	{
		if (imm < 0)
			ctx.registers[dreg].L = lofl16(ctx.registers[dreg].L, (-imm));
		else
			ctx.registers[dreg].L = rofl16(ctx.registers[dreg].L, imm);

		ctx.cycles += cost;
	}

	int imm;
};

class DR_ROI32 : public Operation
{
public:

	DR_ROI32(Context &_ctx, int _imm, int _reg)
		: Operation(_ctx, OpKind_Other, OpSize_32, OpSource_Immediate, OpSource_Register, OpWrite_Modify, 12, 8, 1),
		  imm(_imm)
	{
		sreg = dreg = _reg;
	}

	virtual void onEmit()
	{
		if (imm > 0)
		{
			sprintf(ctx.buf, " ror.l #%d,%s\t;%d [$%04x%04x]\n",
				imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].H,
				ctx.registers[dreg].L
				);
		}
		else
		{
			sprintf(ctx.buf, " rol.l #%d,%s\t;%d [$%04x%04x]\n",
				-imm,
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].H,
				ctx.registers[dreg].L
				);
		}
	}

	virtual void onExecute()
	{
		ulong_t data32 = (ulong_t(ctx.registers[dreg].H) << 16) | ctx.registers[dreg].L;

		if (imm < 0)
			data32 = lofl32(data32, (-imm));
		else
			data32 = rofl32(data32, (imm));

		ctx.registers[dreg].H = data32 >> 16;
		ctx.registers[dreg].L = data32 & 0xFFFF;

		ctx.cycles += cost;
	}

	int imm;
};


class DR_MoveR8 : public Operation
{
public:

	DR_MoveR8(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_Combine, 4, 0, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " move.b %s,%s\t;%d [$%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		uword_t d = ctx.registers[dreg].L;
		uword_t s = ctx.registers[sreg].L;

		uword_t v = (d & 0xFF00) | ubyte_t(s & 0xFF);

		ctx.registers[dreg].L = v;

		ctx.cycles += cost;
	}
};

class DR_MoveR16 : public Operation
{
public:

	DR_MoveR16(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_Replace, 4, 0, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " move.w %s,%s\t;%d [$%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = ctx.registers[sreg].L;

		ctx.cycles += cost;
	}
};

class DR_MoveR32 : public Operation
{
public:

	DR_MoveR32(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_32, OpSource_Register, OpSource_Register, OpWrite_Replace, 4, 0, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " move.l %s,%s\t;%d [$%04x%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].H,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].H = ctx.registers[sreg].H;
		ctx.registers[dreg].L = ctx.registers[sreg].L;

		ctx.cycles += cost;
	}
};

class DR_AddR32 : public Operation
{
public:

	DR_AddR32(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_32, OpSource_Register, OpSource_Register, OpWrite_Combine, 8, 4, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " add.l %s,%s\t;%d [$%04x%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].H, ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		long_t d = (ulong_t(ctx.registers[dreg].H) << 16) | ctx.registers[dreg].L;
		long_t s = (ulong_t(ctx.registers[sreg].H) << 16) | ctx.registers[sreg].L;
		long_t v = d + s;

		ctx.registers[dreg].H = ulong_t(v) >> 16;
		ctx.registers[dreg].L = ulong_t(v) & 0xFFFF;
		ctx.cycles += cost;
	}
};

class DR_AddR16 : public Operation
{
public:

	DR_AddR16(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_Combine, 4, 0, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " add.w %s,%s\t;%d [$%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = uword_t(word_t(ctx.registers[dreg].L) + word_t(ctx.registers[sreg].L));
		ctx.cycles += cost;
	}
};

class DR_AddR8 : public Operation
{
public:

	DR_AddR8(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_Combine, 4, 0, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " add.b %s,%s\t;%d [$%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		uword_t d = ctx.registers[dreg].L;
		uword_t s = ctx.registers[sreg].L;

		uword_t v = (d & 0xFF00) | ubyte_t(byte_t(d & 0xFF) + byte_t(s & 0xFF));

		ctx.registers[dreg].L = v;

		ctx.cycles += cost;
	}
};

class DR_SubR32 : public Operation
{
public:

	DR_SubR32(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_32, OpSource_Register, OpSource_Register, OpWrite_Combine, 8, 4, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " sub.l %s,%s\t;%d [$%04x%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].H, ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		long_t d = (ulong_t(ctx.registers[dreg].H) << 16) | ctx.registers[dreg].L;
		long_t s = (ulong_t(ctx.registers[sreg].H) << 16) | ctx.registers[sreg].L;
		long_t v = d - s;

		ctx.registers[dreg].H = ulong_t(v) >> 16;
		ctx.registers[dreg].L = ulong_t(v) & 0xFFFF;
		ctx.cycles += cost;
	}
};

class DR_SubR16 : public Operation
{
public:

	DR_SubR16(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_Combine, 4, 0, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " sub.w %s,%s\t;%d [$%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = uword_t(word_t(ctx.registers[dreg].L) - word_t(ctx.registers[sreg].L));
		ctx.cycles += cost;
	}
};

class DR_SubR8 : public Operation
{
public:

	DR_SubR8(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_Combine, 4, 0, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " sub.b %s,%s\t;%d [$%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		uword_t d = ctx.registers[dreg].L;
		uword_t s = ctx.registers[sreg].L;

		uword_t v = (d & 0xFF00) | ubyte_t(byte_t(d & 0xFF) - byte_t(s & 0xFF));

		ctx.registers[dreg].L = v;

		ctx.cycles += cost;
	}
};

class DR_AndR32 : public Operation
{
public:

	DR_AndR32(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_32, OpSource_Register, OpSource_Register, OpWrite_Combine, 8, 4, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " and.l %s,%s\t;%d [$%04x%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].H, ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ulong_t d = (ulong_t(ctx.registers[dreg].H) << 16) | ctx.registers[dreg].L;
		ulong_t s = (ulong_t(ctx.registers[sreg].H) << 16) | ctx.registers[sreg].L;
		ulong_t v = d & s;

		ctx.registers[dreg].H = ulong_t(v) >> 16;
		ctx.registers[dreg].L = ulong_t(v) & 0xFFFF;
		ctx.cycles += cost;
	}
};

class DR_AndR16 : public Operation
{
public:

	DR_AndR16(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_Combine, 4, 0, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " and.w %s,%s\t;%d [$%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = ctx.registers[dreg].L & ctx.registers[sreg].L;
		ctx.cycles += cost;
	}
};

class DR_AndR8 : public Operation
{
public:

	DR_AndR8(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_Combine, 4, 0, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " and.b %s,%s\t;%d [$%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		uword_t d = ctx.registers[dreg].L;
		uword_t s = ctx.registers[sreg].L;

		uword_t v = (d & 0xFF00) | ((d & s) & 0xFF);

		ctx.registers[dreg].L = v;

		ctx.cycles += cost;
	}
};

class DR_OrR32 : public Operation
{
public:

	DR_OrR32(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_32, OpSource_Register, OpSource_Register, OpWrite_Combine, 8, 4, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " or.l %s,%s\t;%d [$%04x%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].H, ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ulong_t d = (ulong_t(ctx.registers[dreg].H) << 16) | ctx.registers[dreg].L;
		ulong_t s = (ulong_t(ctx.registers[sreg].H) << 16) | ctx.registers[sreg].L;
		ulong_t v = d | s;

		ctx.registers[dreg].H = ulong_t(v) >> 16;
		ctx.registers[dreg].L = ulong_t(v) & 0xFFFF;
		ctx.cycles += cost;
	}
};

class DR_OrR16 : public Operation
{
public:

	DR_OrR16(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_Combine, 4, 0, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " or.w %s,%s\t;%d [$%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = ctx.registers[dreg].L | ctx.registers[sreg].L;
		ctx.cycles += cost;
	}
};

class DR_OrR8 : public Operation
{
public:

	DR_OrR8(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_Combine, 4, 0, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " or.b %s,%s\t;%d [$%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		uword_t d = ctx.registers[dreg].L;
		uword_t s = ctx.registers[sreg].L;

		uword_t v = (d & 0xFF00) | ((d | s) & 0xFF);

		ctx.registers[dreg].L = v;

		ctx.cycles += cost;
	}
};

class DR_XorR32 : public Operation
{
public:

	DR_XorR32(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_32, OpSource_Register, OpSource_Register, OpWrite_Combine, 8, 4, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " eor.l %s,%s\t;%d [$%04x%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].H, ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ulong_t d = (ulong_t(ctx.registers[dreg].H) << 16) | ctx.registers[dreg].L;
		ulong_t s = (ulong_t(ctx.registers[sreg].H) << 16) | ctx.registers[sreg].L;
		ulong_t v = d ^ s;

		ctx.registers[dreg].H = ulong_t(v) >> 16;
		ctx.registers[dreg].L = ulong_t(v) & 0xFFFF;
		ctx.cycles += cost;
	}
};

class DR_XorR16 : public Operation
{
public:

	DR_XorR16(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_Combine, 4, 0, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " eor.w %s,%s\t;%d [$%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = ctx.registers[dreg].L ^ ctx.registers[sreg].L;
		ctx.cycles += cost;
	}
};

class DR_XorR8 : public Operation
{
public:

	DR_XorR8(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_Combine, 4, 0, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " eor.b %s,%s\t;%d [$%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		uword_t d = ctx.registers[dreg].L;
		uword_t s = ctx.registers[sreg].L;

		uword_t v = (d & 0xFF00) | ((d ^ s) & 0xFF);

		ctx.registers[dreg].L = v;

		ctx.cycles += cost;
	}
};

class DR_SccR8 : public Operation
{
public:

	DR_SccR8(Context &_ctx, bool _cond, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_None, OpSource_Register, OpWrite_Modify, 8, 4, 1),
		  cond(_cond)
	{
		sreg = dreg = _dreg;
	}

	virtual void onEmit()
	{
		if (cond)
		{
			sprintf(ctx.buf, " st %s\t;%d [$%04x]\n",
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
		else
		{
			sprintf(ctx.buf, " sf %s\t;%d [$%04x]\n",
				ctx.registers[dreg].name.c_str(),
				cost,
				ctx.registers[dreg].L
				);
		}
	}

	virtual void onExecute()
	{
		if (cond)
			ctx.registers[dreg].L = ctx.registers[dreg].L | 0x00FF;
		else
			ctx.registers[dreg].L = ctx.registers[dreg].L & 0xFF00;

		ctx.cycles += cost;
	}

	bool cond;
};

class DR_TasR8 : public Operation
{
public:

	DR_TasR8(Context &_ctx, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_None, OpSource_Register, OpWrite_Modify, 4, 0, 1)
	{
		sreg = dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " tas.b %s\t;%d [$%04x]\n",
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = ctx.registers[dreg].L | 0x0080;
		ctx.cycles += cost;
	}
};

class DR_ExtR16 : public Operation
{
public:

	DR_ExtR16(Context &_ctx, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_None, OpSource_Register, OpWrite_Modify, 4, 0, 1)
	{
		sreg = dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " ext.w %s\t;%d [$%04x]\n",
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].L = uword_t(word_t(byte_t(ctx.registers[dreg].L)));
		ctx.cycles += cost;
	}
};

class DR_ExtR32 : public Operation
{
public:

	DR_ExtR32(Context &_ctx, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_32, OpSource_None, OpSource_Register, OpWrite_ReplaceH, 4, 0, 1)
	{
		sreg = dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " ext.l %s\t;%d [$%04x%04x]\n",
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].H, ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ulong_t v = ulong_t(long_t(word_t(ctx.registers[dreg].L)));

		ctx.registers[dreg].H = ulong_t(v) >> 16;
		ctx.registers[dreg].L = ulong_t(v) & 0xFFFF;
		ctx.cycles += cost;
	}
};
class DR_SwapR32 : public Operation
{
public:

	DR_SwapR32(Context &_ctx, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_32, OpSource_None, OpSource_Register, OpWrite_Modify, 4, 0, 1)
		// note: swap acts like a replace since for each word, the new value doesn't depend on the previous value
		// however it is not injected if the word to be used (usually upper) is not stable in the first place
	{
		sreg = dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " swap %s\t;%d [$%04x%04x]\n",
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].H, ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		uword_t tmp = ctx.registers[dreg].L;
		ctx.registers[dreg].L = ctx.registers[dreg].H;
		ctx.registers[dreg].H = tmp;
		ctx.cycles += cost;

		// special handling of stale state
		bool sH = ctx.registers[dreg].stableH;
		ctx.registers[dreg].stableH = ctx.registers[dreg].stableL;
		ctx.registers[dreg].stableL = sH;
	}
};

class DR_BchgR : public Operation
{
public:

	DR_BchgR(Context &_ctx, OpSize _opsize, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, _opsize, OpSource_Register, OpSource_Register, OpWrite_Combine, 8, 4, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " bchg %s,%s\t;%d [$%04x%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].H, ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].H = ctx.registers[dreg].H ^ ((1 << (ctx.registers[sreg].L & 31)) >> 16);
		ctx.registers[dreg].L = ctx.registers[dreg].L ^ ((1 << (ctx.registers[sreg].L & 31)) & 0xFFFF);
		ctx.cycles += cost;
	}
};

class DR_MuluR16 : public Operation
{
public:

	DR_MuluR16(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_ReplaceHCombineL, 70, 64, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " mulu.w %s,%s\t;%d [$%04x%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].H, ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		ulong_t result = ctx.registers[dreg].L * ctx.registers[sreg].L;
		ctx.registers[dreg].H = result >> 16;
		ctx.registers[dreg].L = result & 0xFFFF;
		ctx.cycles += cost;
	}
};

class DR_MulsR16 : public Operation
{
public:

	DR_MulsR16(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_Other, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_ReplaceHCombineL, 70, 64, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " muls.w %s,%s\t;%d [$%04x%04x]\n",
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			cost,
			ctx.registers[dreg].H, ctx.registers[dreg].L
			);
	}

	virtual void onExecute()
	{
		long_t result = word_t(ctx.registers[dreg].L) * word_t(ctx.registers[sreg].L);
		ctx.registers[dreg].H = uword_t(ulong_t(result) >> 16);
		ctx.registers[dreg].L = uword_t(ulong_t(result) & 0x0000FFFF);
		ctx.cycles += cost;
	}
};

class DR_LoadI32 : public Operation
{
public:

	DR_LoadI32(Context &_ctx, int _reg, uword_t _dataH, uword_t _dataL)
		: Operation(_ctx, OpKind_LoadI32, OpSize_32, OpSource_Immediate, OpSource_Register, OpWrite_Replace, 12, 0, 1),
		  dataH(_dataH),
		  dataL(_dataL)
	{
		dreg = _reg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " move.l #$%04x%04x,%s\t;%d\n", dataH, dataL, ctx.registers[dreg].name.c_str(), cost);
	}

	virtual void onExecute()
	{
		ctx.registers[dreg].H = dataH;
		ctx.registers[dreg].L = dataL;
		ctx.registers[dreg].allocatedH = true;
		ctx.registers[dreg].allocatedL = true;
		ctx.cycles += cost;
	}

	uword_t dataH;
	uword_t dataL;
};

class BLiT_Init : public Operation
{
public:

	BLiT_Init(Context &_ctx)
		: Operation(_ctx, OpKind_BLITInit, OpSize_None, OpSource_None, OpSource_None, OpWrite_None, 0, 0, -1)
	{
	}

	virtual void onEmit()
	{
		char buf[1024];
		sprintf(ctx.buf,	" move.w #$%04x,%s\t;%d\n", ctx.blitter.dyinc, ctx.BLiTDYI_name.c_str(), 16);
		sprintf(buf,		" move.w #$%04x,%s\t;%d\n", ctx.blitter.syinc, ctx.BLiTSYI_name.c_str(), 16);
		strcat(ctx.buf, buf);
		sprintf(buf,		" move.w #$%04x,%s\t;%d\n", ctx.blitter.xcount, ctx.BLiTXC_name.c_str(), 16);
		strcat(ctx.buf, buf);
		sprintf(buf,		" move.b #$%02x,%s\t;%d\n", ctx.blitter.skew, ctx.BLiTSKEW_name.c_str(), 16);
		strcat(ctx.buf, buf);
	}

	virtual void onExecute()
	{
	}
};


class SYS_PageRestore : public Operation
{
public:

	SYS_PageRestore(Context &_ctx)
		: Operation(_ctx, OpKind_SYSPageRestore, OpSize_None, OpSource_None, OpSource_None, OpWrite_None, 0, 0, -1)
	{
	}

	virtual void onEmit()
	{
		char buf[1024];
		sprintf(ctx.buf,	" move.w #$%04x,(a5)+\t; dstskip\n", ctx.dstlinewid - (ctx.blitter.xcount << 3));
		sprintf(buf,		" move.w #$%04x,(a5)+\t; dstww2\n", ctx.blitter.xcount * 2);
		strcat(ctx.buf, buf);
		sprintf(buf,		" move.w d4,(a5)+\t; lines\n");
		strcat(ctx.buf, buf);
		sprintf(buf,		" move.l d5,(a5)+\t; record link\n");
		strcat(ctx.buf, buf);
		sprintf(buf,		" move.l a5,usp\t; save ctx\n");
		strcat(ctx.buf, buf);
	}

	virtual void onExecute()
	{
	}
};


class BLiT_SetYC : public Operation
{
public:

	BLiT_SetYC(Context &_ctx, int _sreg)
		: Operation(_ctx, OpKind_BLITSetup, OpSize_16, OpSource_Register, OpSource_Memory, OpWrite_None, 8, 0, -1)
	{
		sreg = _sreg;
	}

	virtual void onEmit()
	{
		sprintf
		(
			ctx.buf,	
			" move.w %s,%s\t;%d\n", 
			ctx.registers[sreg].name.c_str(), 
			ctx.BLiTYC_name.c_str(), 
			cost
		);
	}

	virtual void onExecute()
	{
	}
};

class BLiT_SetDSTADDR : public Operation
{
public:

	BLiT_SetDSTADDR(Context &_ctx, int _sreg)
		: Operation(_ctx, OpKind_BLITSetup, OpSize_32, OpSource_Register, OpSource_Memory, OpWrite_None, 8, 0, -1)
	{
		sreg = _sreg;
	}

	virtual void onEmit()
	{
		sprintf
		(
			ctx.buf,
			" move.l %s,%s\t;%d\n",
			ctx.registers[sreg].name.c_str(),
			ctx.BLiTDST_name.c_str(),
			cost
		);
	}

	virtual void onExecute()
	{
		ctx.dstaddr = ctx.memaddr;
	}
};

class BLiT_Go : public Operation
{
public:

	BLiT_Go(Context &_ctx, int _sreg, int _sline, int _dline)
		: Operation(_ctx, OpKind_BLITGo, OpSize_None, OpSource_None, OpSource_None, OpWrite_None, 8, 0, -1),
		  sline(_sline),
		  dline(_dline)
	{
		sreg = _sreg;
	}

	virtual void onEmit()
	{
		sprintf
			(
			ctx.buf,
			" move.b %s,%s\t;%d BLiT(s=%d,d=%d)\n",
			ctx.registers[sreg].name.c_str(), 
			ctx.BLiTCTRL_name.c_str(), 
			8,
			sline, dline
		);
	}

	virtual void onExecute()
	{
		ctx.memory[ctx.dstaddr + 0] = ctx.blitter.em[0];

		if (ctx.blitter.xcount == 2)
		{
			ctx.memory[ctx.dstaddr + 1] = ctx.blitter.em[2];
		}
		else
		if (ctx.blitter.xcount == 3)
		{
			ctx.memory[ctx.dstaddr + 1] = ctx.blitter.em[1];
			ctx.memory[ctx.dstaddr + 2] = ctx.blitter.em[2];
		}
	}

	int sline;
	int dline;
};

class SRC_Skip : public Operation
{
public:

	SRC_Skip(Context &_ctx, int _skips)
		: Operation(_ctx, OpKind_SRCSkip, OpSize_None, OpSource_None, OpSource_None, OpWrite_None, 8, 2, -1),
		  delta(_skips)
	{
	}

	virtual void onEmit()
	{
		sprintf
			(
			ctx.buf,
			" add.l #%d,$ffff8a24\n",
			delta * (ctx.srccolww * 8)
		);
	}

	virtual void onExecute()
	{
		//ctx.memaddr += delta * ctx.dstww;
	}

	int delta;
};

class DST_Lea : public Operation
{
public:

	DST_Lea(Context &_ctx, int _sreg, int _dreg, int _delta)
		: Operation(_ctx, OpKind_DSTLea, OpSize_None, OpSource_None, OpSource_None, OpWrite_None, 8, 2, 2),
		  delta(_delta)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf
		(
			ctx.buf, 
			" lea %d(%s),%s\t;%d\n", 
			delta * ctx.dstlinewid, 
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(),
			//ctx.DSTADDR_regname.c_str(), 
			//ctx.DSTADDR_regname.c_str(), 
			cost
		);
	}

	virtual void onExecute()
	{
		ctx.memaddr += delta * ctx.dstww;
	}

	int delta;
};

class DST_AddaR16 : public Operation
{
public:

	DST_AddaR16(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_DSTAddaR16, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_Combine, 8, 4, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " adda.w %s,%s\t;%d\n", 
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(), 
			cost
			);
	}

	virtual void onExecute()
	{
		ctx.memaddr += word_t((word_t(ctx.registers[sreg].L) / ctx.dstlinewid) * ctx.dstww);
	}
};

class DST_SubaR16 : public Operation
{
public:

	DST_SubaR16(Context &_ctx, int _sreg, int _dreg)
		: Operation(_ctx, OpKind_DSTSubaR16, OpSize_16, OpSource_Register, OpSource_Register, OpWrite_Combine, 8, 4, 1)
	{
		sreg = _sreg;
		dreg = _dreg;
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " suba.w %s,%s\t;%d\n", 
			ctx.registers[sreg].name.c_str(),
			ctx.registers[dreg].name.c_str(), 
			cost
			);
	}

	virtual void onExecute()
	{
		ctx.memaddr -= word_t((word_t(ctx.registers[sreg].L) / ctx.dstlinewid) * ctx.dstww);
	}
};

class Terminate : public Operation
{
public:

	Terminate(Context &_ctx)
		: Operation(_ctx, OpKind_Terminate, OpSize_None, OpSource_None, OpSource_None, OpWrite_None, 0, 0, -1)
	{
	}

	virtual void onEmit()
	{
		sprintf(ctx.buf, " rts\n");
	}

	virtual void onExecute()
	{
	}
};

