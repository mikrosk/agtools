//---------------------------------------------------------------------------------------------------------------------
//	[A]tari [G]ame [T]ools / dml 2016
//---------------------------------------------------------------------------------------------------------------------
//	EMX blitter sprite generator
//	Notes:
//	- EMX sprites uses code generation for each preshift (0-15) of a sprite frame, where a preshift consists of
//	  delta-compressed blitter endmask updates using the minimum number of data loads & stores
//	- EMX sprites can optionally embed the sprite-restore records needed to erase sprites after each update
//	- requires RMAC assembler V1.4.12  (or better) to post-process the frames
//---------------------------------------------------------------------------------------------------------------------

enum RegClass
{
	RegClass_Data,
	RegClass_Address
};

class dataregister
{
public:

	dataregister
	(
		const char* _name, 
		int _index, 
		RegClass _regclass, 
		bool _alloc = false, 
		bool _constant = false, 
		bool _locked = false,
		ulong_t _init = 0
	)
		: name(_name),
		  index(_index),
		  regclass(_regclass),
		  allocatedH(_alloc),
		  allocatedL(_alloc),
		  constant(_constant),
		  locked(_locked),
		  variable(false),
		  H(_init >> 16),
		  L(_init & 0xFFFF),
		  stableH(_alloc),
		  stableL(_alloc),
		  sspos(0)
	{
	}

	typedef struct state_s
	{
		bool stableH;
		bool stableL;
		uword_t H;
		uword_t L;

	} state_t;


	state_t statestack[4];
	int sspos;

	void push()
	{
		state_t &s = statestack[sspos++];
		s.L = L;
		s.H = H;
		s.stableL = stableL;
		s.stableH = stableH;
	}

	void pop()
	{
		const state_t &s = statestack[--sspos];
		L = s.L;
		H = s.H;
		stableL = s.stableL;
		stableH = s.stableH;
	}

	std::string name;
	int index;
	RegClass regclass;
	bool allocatedH;
	bool allocatedL;
	bool constant;
	bool locked;
	bool variable;
	bool stableH;
	bool stableL;
	uword_t H;
	uword_t L;
};

//---------------------------------------------------------------------------------------------------------------------

class BLiT
{
public:

	BLiT()
		: xcount(0),
		  dyinc(0),
		  skew(0)
	{
		em[0] = 0;
		em[1] = 0;
		em[2] = 0;
	}

	uword_t em[3];

	uword_t xcount;
	uword_t dyinc;
	ubyte_t skew;
};

//---------------------------------------------------------------------------------------------------------------------

typedef std::map<uword_t, int> relationtable;

class emx_metric_t
{
public:

	emx_metric_t()
		: incidence(0),
		  references(0),
		  name(-1),
		  //incidence_em0(0),
		  //incidence_em1(0),
		  //incidence_em2(0),
		  //
		  fincidence_score(0.0f),
		  fadjacency_score(0.0f)
	{
	}

	int incidence;
	int references;
	int name;
	//int incidence_em0;
	//int incidence_em1;
	//int incidence_em2;
	relationtable line_adjacencies;
	relationtable left_adjacencies;
	relationtable left_references;

	float fincidence_score;
	float fadjacency_score;

};

typedef std::map<uword_t, emx_metric_t> metricmap;

static const int max_virtual_registers = 100;
static const int max_registers = max_virtual_registers+28;

class Operation;

int DataSource_costs[] =
{
	(0<<8),
	(8<<8),
	(12<<8),
	(12<<8),
	(16<<8),
	(20<<8)+1					// small bias against long immediate because it's harder to peephole these vs short immediates
};

enum DataSource
{
	DataSource_None,
	DataSource_RegisterL,		// 8c
	DataSource_RegisterHL,		// 12c
	DataSource_Immediate16,		// 12c
	DataSource_RegisterLL,		// 16c
	DataSource_MixedIR,			// 20c
	DataSource_Immediate32,		// 20+1c
};

class preload
{
public:

	preload(uword_t _l)
		: H(0), L(_l), 
		  validH(false),
		  validL(true)
	{
	}

	uword_t H;
	uword_t L;
	bool validH;
	bool validL;
};

static inline int long_cost(uword_t _h, uword_t _l)
{
	// moveq
	long_t sA = (long_t(_h) << 16) | _l;
	if ((sA >= -128) && (sA < 128))
		return 4;

	// moveq/swap
	long_t sB = (long_t(_l) << 16) | _h;
	if ((sB >= -128) && (sB < 128))
		return 8;

	// todo: moveq+rotate(hidden)

	return 12;
}

//---------------------------------------------------------------------------------------------------------------------

enum OpKind
{
	OpKind_Other,
	//
	// generic ops
	//
	OpKind_Nop,
	//
	OpKind_LoadI16,
	OpKind_LoadI32,
	//
	OpKind_Lea16,
	OpKind_Lea32,
	//
	// blitter ops
	//
	OpKind_StoreI16,
	OpKind_StoreI32,
	OpKind_StoreR16,
	OpKind_StoreR32,
	//
	OpKind_DSTAddaR16,
	OpKind_DSTSubaR16,
	OpKind_DSTLea,
	//
	OpKind_SYSPageRestore,
	OpKind_BLITInit,
	OpKind_BLITSetup,
	OpKind_BLITGo,
	//
	OpKind_Terminate
};

enum OpWrite
{
	OpWrite_None,
	OpWrite_Replace,
	OpWrite_Modify,
	OpWrite_ReplaceH,				// upper word replaced/extended
	OpWrite_ReplaceHCombineL,		// upper word replaced/extended, lower word combined from another source
	OpWrite_Combine,
};

enum OpSource
{
	OpSource_None,
	OpSource_Immediate,
	OpSource_Register,
	OpSource_Memory,
};

enum OpSize
{
	OpSize_None,
//	OpSize_8,
	OpSize_16,
	OpSize_32,
};

//---------------------------------------------------------------------------------------------------------------------

class Context
{
public:

	void reset_registers()
	{
		int r = 0;

		registers.clear();

		if (0 && PREFER_TRANSFORMS)
		{
			char name[] = "r00";

			for (int v = 0; v < max_virtual_registers; v++)
			{
				name[1] = char((v / 10) + '0');
				name[2] = char((v % 10) + '0');

				registers.push_back(dataregister(name, r++, RegClass_Data));
			}
		}

		if (1 || !PREFER_TRANSFORMS)
		{
			if (srcww == 3)
				registers.push_back(dataregister("d0", r++, RegClass_Data, setconstants, setconstants, true,	0xC0000000 | 184));
			else
				registers.push_back(dataregister("d0", r++, RegClass_Data));

			registers.push_back(dataregister("d3", r++, RegClass_Data));
			registers.push_back(dataregister("d4", r++, RegClass_Data));
			registers.push_back(dataregister("d5", r++, RegClass_Data));
			registers.push_back(dataregister("d6", r++, RegClass_Data));
			registers.push_back(dataregister("d7", r++, RegClass_Data));

/*			for (int p = 0; (p < registers.size()) && (p < preloads.size()); p++)
			{
				if (preloads[p].validL)
				{
					registers[p].stableL = true;
					registers[p].allocatedL = true;
					registers[p].L = preloads[p].L;
				}
				if (preloads[p].validH)
				{
					registers[p].stableH = true;
					registers[p].allocatedH = true;
					registers[p].H = preloads[p].H;
				}
			}
*/
		}

		// todo: different ideal register pattern & usage for 1/2/3-wide cases

		SCRLINE_reg = r;
		registers.push_back(dataregister("a1", r++, RegClass_Address));
		registers[SCRLINE_reg].variable = true;

		DSTADDR_reg = r;
		registers.push_back(dataregister("a3", r++, RegClass_Address,	setconstants, setconstants, true,		0xffff8a32));
		YC_reg = r;
		registers.push_back(dataregister("a0", r++, RegClass_Address,	setconstants, setconstants, true,		0x00ff8a38));
		CTRL_reg = r;
		registers.push_back(dataregister("a5", r++, RegClass_Address,	setconstants, setconstants, true,		0xffff8a3c));

		// em1 addr always needed
		EM1_reg = r;
		registers.push_back(dataregister("a2", r++, RegClass_Address,	setconstants, setconstants, true,		0x00ff8a28));	// em1

		// em2,em3 addr depend on sprite width - free up registers if possible
		if (srcww == 3)
		{
			EM2_reg = r;
			registers.push_back(dataregister("a6", r++, RegClass_Address, setconstants, setconstants, true,		0xffff8a2a));	// em2
		}
		else
			registers.push_back(dataregister("a6", r++, RegClass_Address, setconstants, setconstants, true,		184));

		if (srcww > 1)
		{
			EM3_reg = r;
			registers.push_back(dataregister("a4", r++, RegClass_Address, setconstants, setconstants, true,		0x00ff8a2c));	// em3
		}
		else
			registers.push_back(dataregister("a4", r++, RegClass_Address));


		BHOG_reg = r;
		registers.push_back(dataregister("d1", r++, RegClass_Data,		setconstants, setconstants, true,		0x00ffffc0));

		PLANES_reg = r;
		registers.push_back(dataregister("d2", r++, RegClass_Data,		setconstants, setconstants, true,		0xFF000004));

//		registers.push_back(dataregister("sr", r++, true, true, 0x00002300));
	}

	Context(int _srcww, int _srch)
		: srcww(_srcww), srch(_srch),
		  dstww(3),
		  emaddr(0),
		  memaddr(0),
		  dstaddr(0),
		  cycles(0),
		  dstlinewid(0),
		  genout(genstr),
		  verbose(false),
		  setconstants(true),
		  SCRLINE_reg(0),
		  PLANES_reg(0),
		  BHOG_reg(0)
	{
		memory = new uword_t[dstww * srch];

//		memset(preloads, 0, sizeof(preloads));

//		blitter.xcount = srcww;

		dstlinewid = 184;

		//BLiTDST_name = "BLiTDST.w";
		//BLiTCTRL_name = "BLiTCTRL.w";
		//BLiTYC_name = "BLiTYC.w";
		BLiTDST_name = "(a3)";
		BLiTCTRL_name = "(a5)";
		BLiTYC_name = "(a0)";

//		BLiTEM_names.push_back("BLiTEM1.w");
//		BLiTEM_names.push_back("BLiTEM2.w");
//		BLiTEM_names.push_back("BLiTEM3.w");
		BLiTEM_names.push_back("(a2)");
		BLiTEM_names.push_back("(a6)");
		BLiTEM_names.push_back("(a4)");
//		BLiTEM1_name = "BLiTEM1.w";
//		BLiTEM2_name = "BLiTEM2.w";
//		BLiTEM3_name = "BLiTEM3.w";
		//BLiTEM1_name = "(a2)";
		//BLiTEM2_name = "(a6)";
		//BLiTEM3_name = "(a4)";

		BLiTDYI_name = "$ffff8a30.w";
		BLiTXC_name = "$ffff8a36.w";
		BLiTSKEW_name = "$ffff8a3d.w";

		DSTADDR_regname = "a1";
		BLiTCTRL_go_srcname = "d1";	//#$c0;
		BLiTYC_planes_srcname = "d2";	//#4;

		// free: d0,d3,d4,d5,d6,d7

//		reset_registers();

		//{
		//	char *name = "r00";
		//	for (int v = 0; v < max_virtual_registers; v++)
		//	{
		//		registers.push_back(dataregister(name, r++));

		//		name[1] = (v / 10) + '0';
		//		name[2] = (v % 10) + '0';
		//	}
		//}
	}

	~Context()
	{
		delete[] memory;
	}

	void reset_sim(bool _verbose)
	{
		verbose = _verbose;
		memset(memory, 0xAA, dstww * srch * sizeof(uword_t));
		emaddr = 0;
		memaddr = 0;
		dstaddr = 0;
		cycles = 0;
		blitter.xcount = srcww;
		reset_registers();
		metrics_sim = metrics;
	}

	void register_rename(std::list<Operation*>::iterator curr, int _rold, int _rnew);

	DataSource sim_resolve_data16(uword_t _data, int &r_reg_index, bool _injectops);
	DataSource sim_resolve_data32(uword_t _dataH, uword_t _dataL, int &r_regH_index, int &r_regL_index, bool _injectops);

	void sim_track_datarefs(uword_t _data)
	{
		emx_metric_t& m = metrics_sim[_data];
		m.references--;
		if (m.references == 0)
		{
//			m.incidence = 0;

			// this data is no longer required - free up whatever register(s) are assigned with it
			// look for existent value in active registers
			for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++)
			{
				if (!(it->locked))
				{
					if ((it->allocatedL) && (it->L == _data))
					{
						//if (verbose)
						//	printf("> released value [$%04x] from register [%s]L\n", _data, it->name.c_str());
						it->allocatedL = false;
					}

					if ((it->allocatedH) && (it->H == _data))
					{
						//if (verbose)
						//	printf("> released value [$%04x] from register [%s]H\n", _data, it->name.c_str());
						it->allocatedH = false;
					}
				}
			}
		}
	}

	void sim_track_datarefs(uword_t _dataH, uword_t _dataL)
	{
		emx_metric_t& m = metrics_sim[_dataH];
		m.left_references[_dataL]--;

		sim_track_datarefs(_dataH);
		sim_track_datarefs(_dataL);
	}

	void try_transforms(std::list<Operation*>::iterator &curr, OpSize _opsize, int _rold, uword_t _dataH, uword_t _dataL);

	bool try_4c_init_transforms(std::list<Operation*>::iterator &curr, OpSize _opsize, int _rold, uword_t _dataH, uword_t _dataL);
	bool try_8c_init_transforms(std::list<Operation*>::iterator &curr, OpSize _opsize, int _rold, uword_t _dataH, uword_t _dataL);
	bool try_12c_init_transforms(std::list<Operation*>::iterator &curr, OpSize _opsize, int _rold, uword_t _dataH, uword_t _dataL);

	bool try_reg_transforms(std::list<Operation*>::iterator &curr, std::vector<dataregister>::iterator it, OpSize _opsize, int _rold, uword_t _dataH, uword_t _dataL, int &r_limit, bool _2nd = false);
//	bool try_8c_reg_transforms(std::list<Operation*>::iterator &curr, std::vector<dataregister>::iterator it, OpSize _opsize, int _rold, uword_t _dataH, uword_t _dataL, bool _2nd = false);

	void transform_pass();
	void cleanup_pass();
	void reduce_pass();
	void tail_pass();

	void register_sweep();

	bool tag_collision(std::list<Operation*>::iterator _start, int _rold, int _rnew, OpSize _sz);
	bool tag_busy(std::list<Operation*>::iterator _start, int _r, OpSize _sz);
	int register_count_srcrefs(std::list<Operation*>::iterator _start, int _reg, OpSize _sz);
	std::list<Operation*>::iterator register_last_written(std::list<Operation*>::iterator _start, int _reg, OpSize _sz);

	void assemble(ubyte_t *&r_p68kbin, size_t &r_size68kbin)
	{
		ubyte_t *p68kbin = 0;

		std::string& str = genout.str();  
		std::ofstream cfile;
		cfile.open("tmp68k.s");  
		cfile << str;  
		cfile.close();

		// execute assembler, turning our temporary .s into binary 68k
		//		ShellExecute(NULL, "open", "rmac.exe", "-n -o tmp68k.bin tmp68k.s", NULL, SW_HIDE);

		{
			SHELLEXECUTEINFO ShExecInfo = { 0 };
			ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
			ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
			ShExecInfo.hwnd = NULL;
			ShExecInfo.lpVerb = NULL;
			ShExecInfo.lpFile = "rmac.exe";
			ShExecInfo.lpParameters = "-fa -ltmp68k.lst -o tmp68k.bin tmp68k.s";
			ShExecInfo.lpDirectory = NULL;
			ShExecInfo.nShow = SW_HIDE;
			ShExecInfo.hInstApp = NULL;
			ShellExecuteEx(&ShExecInfo);
			WaitForSingleObject(ShExecInfo.hProcess, INFINITE);

			DWORD exitcode = 0;
			GetExitCodeProcess(ShExecInfo.hProcess, &exitcode);

			if (exitcode != 0)
			{
				std::cerr << "error assembling intedmediate code!" << std::endl;
				getchar();

				exit(-1);
			}

			DeleteFileA("tmp68k.s");
		}

		// read code back in
		{
			FILE *h = fopen("tmp68k.bin", "rb");
			if (h)
			{
				fseek(h, 0, SEEK_END);
				r_size68kbin = ftell(h);
				fseek(h, 0, SEEK_SET);

				if (r_size68kbin <= 32)
				{
					std::cerr << "error sizing binary code!" << std::endl;
					getchar();
					exit(-1);
				}

				r_p68kbin = new ubyte_t[r_size68kbin];
				if (r_size68kbin != fread(r_p68kbin, 1, r_size68kbin, h))
				{
					std::cerr << "error reading binary code!" << std::endl;
					getchar();
					exit(-1);

					fclose(h);
				}

#if (USE_NOP_BINARY_VALIDATION)
				if ((r_p68kbin[32+0] != 0x4e) ||
					(r_p68kbin[32+1] != 0x71))
				{
					std::cerr << "error validating binary code begin!" << std::endl;
					getchar();
					exit(-1);

					fclose(h);
				}
#endif

				int oldsize = r_size68kbin;
				while (
					r_size68kbin &&
					(
						(r_p68kbin[r_size68kbin - 2] != 0x4e) ||
						(r_p68kbin[r_size68kbin - 1] != 0x75)
					)
				)
				{
					r_size68kbin--;
				}

				if (oldsize != r_size68kbin)
					std::cout << "trimmed binary code by " << (oldsize-r_size68kbin) << " bytes" << std::endl;


				if (r_size68kbin == 0)
				{
					std::cerr << "error validating binary code end!" << std::endl;
					getchar();
					exit(-1);

					fclose(h);
				}

				fclose(h);

				DeleteFileA("tmp68k.bin");
			}
			else
			{
				std::cerr << "error opening binary code!" << std::endl;
				getchar();
				exit(-1);
			}
		}
	}

	std::string genstr;
	std::stringstream genout;
	bool verbose;

	std::vector<preload> preloads;

	int srcww, dstww;
	int srch;
	int dstlinewid;

	metricmap metrics;
	metricmap metrics_sim;

	std::vector<dataregister> registers;

	std::list<Operation*> operations;

	std::string BLiTDST_name;
	std::string BLiTCTRL_name;
//	std::string BLiTEM1_name;
//	std::string BLiTEM2_name;
//	std::string BLiTEM3_name;
	std::string BLiTYC_name;
	std::string BLiTDYI_name;
	std::string BLiTXC_name;
	std::string BLiTSKEW_name;

	std::vector<std::string> BLiTEM_names;

	int SCRLINE_reg;
	int PLANES_reg;
	int BHOG_reg;
	int DSTADDR_reg;
	int YC_reg;
	int CTRL_reg;
	int EM1_reg;
	int EM2_reg;
	int EM3_reg;
	std::string DSTADDR_regname;
	std::string BLiTCTRL_go_srcname;
	std::string BLiTYC_planes_srcname;


	// blitter
	BLiT blitter;
	int emaddr;

	// framebuffer
	uword_t *memory;
	int memaddr;
	int dstaddr;

//	wordpair_t registers[16];
	int cycles;

	bool setconstants;

	char buf[1024];
};


#include "68ksimops.h"


// metrics: 
//	total incidence
//	incidence at EM0,EM1,EM2
//	line adjacency vs all others
//	left-adjacency vs all others

typedef std::map<u64, int> linemap;


void print_dataword(uword_t _data)
{
	uword_t v = _data;
	for (int i = 0; i < 16; i++)
	{
		if (v & 0x8000)
			putchar('1');
		else
			putchar('0');

		v <<= 1;
	}
}

void print_datalong(ulong_t _data)
{
	ulong_t v = _data;
	for (int i = 0; i < 32; i++)
	{
		if (v & 0x80000000)
			putchar('1');
		else
			putchar('0');

		v <<= 1;
	}
}

void print_data(u64 _data, int _count)
{
	u64 v = _data;
	u64 mask = (u64)1 << (_count - 1);
	for (int i = 0; i < _count; i++)
	{
		if (v & mask)
			putchar('1');
		else
			putchar('0');

		v <<= 1;
	}
}

//static void trylineseq(const std::vector<int> &_stack_free, const std::vector<int> &_stack, std::vector<int> &_stack_best)
//{
//	int freesize = _stack_free.size();
//	if (freesize == 0)
//		return;
//
//	std::vector<int> stack;
//	std::vector<int> stack_free;
//
//	for (int n = 0; n < freesize; n++)
//	{
//		// try all positions as next place
//		stack = _stack;
//		stack.push_back(_stack_free[n]);
//
//		stack_free = _stack_free;
//		stack_free[n] = _stack_free[freesize-1];
//		stack_free.pop_back();
//
//		trylineseq(stack_free, stack, _stack_best);
//	}
//}

DataSource Context::sim_resolve_data16(uword_t _data, int &r_reg_index, bool _injectops)
{
	// 1) look for existent value in active registers (allocated or otherwise)
	for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++)
	{
		if (!(it->variable) &&
			(it->stableL) && 
			(it->L == _data))
		{
			if (_injectops)
			{
				if (g_allargs.verbose)
					printf("> referenced value [$%04x] in register [%s]L\n", _data, it->name.c_str());
			}
			r_reg_index = it->index;
			return DataSource_RegisterL;
		}
	}

	if (1 && PREFER_TRANSFORMS)
	{
		int i = 0;
		for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++, i++)
		{
			if (!(it->locked) && !(it->variable))
			{
				if (!(it->stableL))
				{
					if (_injectops)
					{
						if (g_allargs.verbose)
							printf("> assigned value [$%04x] to register [%s]L\n", _data, it->name.c_str());
						operations.push_back(new DR_LoadI16(*this, it->index, _data));

						it->allocatedL = true;
						it->L = _data;

						operations.back()->execute();
					}

					r_reg_index = it->index;
					return DataSource_RegisterL;
				}
			}
		}
	}


	// 2) assign value to unallocated register if sufficient reuse expected
	if (metrics_sim[_data].references >= 2)
	{
		int best_refcount = 1<<30;
		int best_reg = -1;
		int i = 0;
		for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++, i++)
		{
			if (!(it->locked) && !(it->variable))
			{
				if (!(it->allocatedL))
				{
					if (_injectops)
					{
						if (g_allargs.verbose)
							printf("> assigned value [$%04x] to register [%s]L\n", _data, it->name.c_str());
						operations.push_back(new DR_LoadI16(*this, it->index, _data));

						it->allocatedL = true;
						it->L = _data;

						operations.back()->execute();
					}

					r_reg_index = it->index;
					return DataSource_RegisterL;
				}
				else
				{
					// meanwhile, look for the best still-active register to steal
					emx_metric_t &m = metrics_sim[it->L];
					if (m.references < best_refcount)
					{
						best_refcount = m.references;
						best_reg = i;
					}
				}
			}
		}

		// 3) steal allocated register, if estimated saving makes it worthwhile
		// factor +1 cost for reload
		if ((best_reg >= 0) && 
			(metrics_sim[_data].references > (best_refcount+1)))
		{
			dataregister &r = registers[best_reg];
			if (_injectops)
			{
				if (g_allargs.verbose)
					printf("> replaced value [$%04x] with [$%04x] at register [%s]L\n", r.L, _data, r.name.c_str());
				r.L = _data;
				// allows future choices to acknowledge the reuse incidence looks different from now on
//				metrics[_data].incidence = metrics[_data].references;
			}
			r_reg_index = r.index;

			//if (PREFER_TRANSFORMS)
			//	assert(false);

			return DataSource_RegisterL;			
		}
	}

	// single-use, or last resort - just load immediate data
	if (_injectops)
		if (g_allargs.verbose)
			printf("> resolved value [$%04x] as immediate\n", _data);

	return DataSource_Immediate16;
}

DataSource Context::sim_resolve_data32(uword_t _dataH, uword_t _dataL, int &r_regH_index, int &r_regL_index, bool _injectops)
{
	// 1) look for existent value in active registers (allocated or otherwise)
	for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++)
	{
		if (!(it->variable) &&
			(it->stableH) &&
			(it->H == _dataH) && 
			(it->L == _dataL)
		)
		{
			if (_injectops)
			{
				if (g_allargs.verbose)
					printf("> referenced value [$%04x%04x] in register [%s]HL\n", _dataH, _dataL, it->name.c_str());
			}
			r_regH_index = it->index;
			return DataSource_RegisterHL;
		}
	}

	if (1 && PREFER_TRANSFORMS)
	{
		int i = 0;
		for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++, i++)
		{
			// don't touch constant registers
			if (!(it->locked) && !(it->variable))
			{
				// don't touch allocated fields
				if (!(it->stableH) && 
					!(it->stableL))
				{
					if (_injectops)
					{
						if (g_allargs.verbose)
							printf("> assigned value [$%04x%04x] to register [%s]HL\n", _dataH, _dataL, it->name.c_str());

						operations.push_back(new DR_LoadI32(*this, it->index, _dataH, _dataL));

						it->allocatedH = true;
						it->allocatedL = true;
						it->H = _dataH;
						it->L = _dataL;

						operations.back()->execute();
					}

					r_regH_index = it->index;
					return DataSource_RegisterHL;
				}
			}
		}
	}

	// 2) assign value to unallocated register if sufficient reuse expected or if value is cheap to load
	if (
		((metrics_sim[_dataH].left_references[_dataL] >= 2) || 
		(metrics_sim[_dataL].references >= 2)) ||
		(long_cost(_dataH, _dataL) <= 8)
		)
	{
		int best_refcount = 1<<30;
		int best_reg = -1;
		int i = 0;
		for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++, i++)
		{
			// don't touch constant registers
			if (!(it->locked) && !(it->variable))
			{
				// don't touch allocated fields
				if (!(it->allocatedH) && 
					!(it->allocatedL))
				{
					if (_injectops)
					{
						if (g_allargs.verbose)
							printf("> assigned value [$%04x%04x] to register [%s]HL\n", _dataH, _dataL, it->name.c_str());

						operations.push_back(new DR_LoadI32(*this, it->index, _dataH, _dataL));

						it->allocatedH = true;
						it->allocatedL = true;
						it->H = _dataH;
						it->L = _dataL;

						operations.back()->execute();
					}

					r_regH_index = it->index;
					return DataSource_RegisterHL;
				}
				else
				{
					// meanwhile, look for the best still-active register to steal
					//emx_metric_t &m = metrics[it->L];
					//if (m.references < best_refcount)
					//{
					//	best_refcount = m.references;
					//	best_reg = i;
					//}
				}
			}
		}

		// 3) steal allocated register, if estimated saving makes it worthwhile
		// factor +1 cost for reload
		//if (best_reg >= 0 && metrics[_data].references > (best_refcount+1))
		//{
		//	dataregister &r = registers[best_reg];
		//	printf("> replaced value [$%04x] with [$%04x] at register [%s]L\n", r.L, _data, r.name.c_str());
		//	r.L = _data;
		//	// allows future choices to acknowledge the reuse incidence looks different from now on
		//	metrics[_data].incidence = metrics[_data].references;
		//	r_reg_index = r.index;
		//	return DataSource_RegisterL;			
		//}
	}

	// look for mixed combination (high immediate + low reg) for cases with nontrivial high word
	// since immediate words are easier to peephole than arbitrary immediate longs
	for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++)
	{
		if ((it->H != 0x0000) && 
			(it->H != 0xFFFF))
		{
			if (!(it->variable) &&
				(it->stableL) && 
				(it->L == _dataL))
			{
				if (_injectops)
				{
					if (g_allargs.verbose)
						printf("> referenced value [$%04x] in register [%s]L\n", _dataL, it->name.c_str());
				}
				r_regL_index = it->index;
				return DataSource_MixedIR;
			}
		}
	}

	// single-use, or last resort - just load immediate data
	if (_injectops)
		if (g_allargs.verbose)
			printf("> resolved value [$%04x%04x] as immediate\n", _dataH, _dataL);

	return DataSource_Immediate32;
}

void Context::register_rename(std::list<Operation*>::iterator _start, int _regold, int _regnew)
{
	if (_regold == _regnew)
		return;

	Operation* poper = *_start;

	// both 16 and 32bit users of a register will tag the low word so we use this to remap
	int tagold = -1;
	if (_regold == poper->sreg)
		tagold = poper->stagL;
	else
	if (_regold == poper->dreg)
		tagold = poper->dtagL;
	else
	if (_regold == poper->areg)
		tagold = poper->atag;

	if (tagold < 0)
		return;

	for (std::list<Operation*>::iterator it = operations.begin(); it != operations.end(); it++)
	{
		Operation *poper = *it;

		if (poper->stagL == tagold)
		{
			if (g_allargs.verbose)
				printf("replacing S:%s with %s\n", registers[poper->sreg].name.c_str(), registers[_regnew].name.c_str());
			poper->sreg = _regnew;
		}

		if (poper->dtagL == tagold)
		{
			if (g_allargs.verbose)
				printf("replacing D:%s with %s\n", registers[poper->dreg].name.c_str(), registers[_regnew].name.c_str());
			poper->dreg = _regnew;
		}

		if (poper->atag == tagold)
		{
			if (g_allargs.verbose)
				printf("replacing A:%s with %s\n", registers[poper->areg].name.c_str(), registers[_regnew].name.c_str());
			poper->areg = _regnew;
		}
	}
}

void Context::register_sweep()
{
	std::vector<std::pair<int,int> > tags;
	tags.resize(registers.size(), std::pair<int,int>(-1,-1));
	int newtag = 0;

	for (std::list<Operation*>::reverse_iterator rit = operations.rbegin(); rit != operations.rend(); rit++)
	{
		Operation *poper = *rit;

		int replacedH = -1;
		int replacedL = -1;

		// data source
		if (poper->sreg >= 0)
		{
			// if not yet tagged - generate new tag
			std::pair<int,int> &optag = tags[poper->sreg];

/*			23
			23
			23 16
			22
			22 32
			01
			01
			01 16
			00
			00
*/
			if (optag.first < 0)
			{
				optag.first = newtag++;

				if (poper->opsize == OpSize_32)
				{
					if (optag.second < 0)
						optag.second = optag.first;
				}
			}

			poper->stagH = optag.second;
			poper->stagL = optag.first;

			//printf("referenced: %s (%d:%d)\n", 
			//	registers[poper->sreg].name.c_str(), 
			//	poper->stagH, 
			//	poper->stagL
			//	);
		}

		// address source
		if (poper->areg >= 0)
		{
			// if not yet tagged - generate new tag
			std::pair<int,int> &optag = tags[poper->areg];

			if (optag.first < 0)
			{
				optag.first = newtag++;
				optag.second = optag.first;
			}

			poper->atag = optag.first;

			//printf("referenced: %s (%d)\n", 
			//	registers[poper->areg].name.c_str(), 
			//	poper->atag 
			//	);
		}

		// destination as source
		if ((poper->dreg >= 0) && 
			((poper->opwr == OpWrite_Modify) || 
			 (poper->opwr == OpWrite_Combine) ||
			 (poper->opwr == OpWrite_Replace) || 
			 (poper->opwr == OpWrite_ReplaceHCombineL))
		)
		{
			// if not yet tagged - generate new tag
			std::pair<int,int> &optag = tags[poper->dreg];

			if (optag.first < 0)
			{
				optag.first = newtag++;

				if (poper->opsize == OpSize_32)
				{
					if (optag.second < 0)
						optag.second = optag.first;
				}
			}

			poper->dtagH = optag.second;
			poper->dtagL = optag.first;

			//printf("modified: %s (%d:%d)\n", 
			//	registers[poper->dreg].name.c_str(), 
			//	poper->dtagH, 
			//	poper->dtagL
			//	);
		}

		//// destination replaced (rename)
		//if ((poper->dreg >= 0) && 
		//	((poper->opwr == OpWrite_Replace) || 
		//	(poper->opwr == OpWrite_ReplaceHCombineL))
		//	)
		//{
		//	// if not yet tagged - generate new tag
		//	std::pair<int,int> &optag = tags[poper->dreg];

		//	if (optag.first < 0)
		//	{
		//		optag.first = newtag++;
		//		if (poper->opsize == OpSize_32)
		//			optag.second = optag.first;
		//	}

		//	poper->dtagH = optag.second;
		//	poper->dtagL = optag.first;

		//	//printf("replaced: %s (%d:%d)\n", 
		//	//	registers[poper->dreg].name.c_str(), 
		//	//	poper->dtagH, 
		//	//	poper->dtagL
		//	//	);

		//	// lower word only replaced if a simple replace op (e.g. not a sign-extending combine, where only upper word is replaced)
		//	if (poper->opwr != OpWrite_ReplaceHCombineL)
		//		replacedL = poper->dreg;

		//	if (poper->opsize == OpSize_32)
		//		replacedH = poper->dreg;
		//}

		// lower word only replaced if a simple replace op (e.g. not a sign-extending combine, where only upper word is replaced)
		if (poper->opwr == OpWrite_Replace)
			replacedL = poper->dreg;

		if (poper->opsize == OpSize_32)
			if (poper->opwr == OpWrite_Replace || poper->opwr == OpWrite_ReplaceH || poper->opwr == OpWrite_ReplaceHCombineL)
				replacedH = poper->dreg;

		int r = 0;
		for (std::vector<std::pair<int,int> >::iterator it = tags.begin(); it != tags.end(); it++, r++)
		{
			std::pair<int,int> &tag = *it;

			// save full tag pattern for this state in operation to help with renaming
			poper->alltagsH[r] = tag.second;
			poper->alltagsL[r] = tag.first;

			//if (tag.first >= 0)
			//	printf("%d ", tag.first);
			//else
			//	printf(". ", tag.first);
		}

		// reset tags for replaced register BEFORE replace op (running backwards)
		if (replacedL >= 0)
			tags[replacedL].first = -1;
		if (replacedH >= 0)
			tags[replacedH].second = -1;

		//printf("\n");
	}
}

bool Context::tag_busy(std::list<Operation*>::iterator _start, int _r, OpSize _sz)
{
	// detect lifespan tag conflicts between two registers
	Operation *poper = *_start;

	if (poper->alltagsL[_r] >= 0)
		return true;

	if (_sz == OpSize_32)
		if (poper->alltagsH[_r] >= 0)
			return true;

	return false;
}

bool Context::tag_collision(std::list<Operation*>::iterator _start, int _rold, int _rnew, OpSize _sz)
{
	if (_rnew == _rold)
		return false;

	// detect lifespan tag conflicts between two registers
	for (std::list<Operation*>::iterator it = _start; it != operations.end(); it++)
	{
		Operation *poper = *it;

		if (poper->alltagsL[_rold] >= 0)
		{
			if (poper->alltagsL[_rnew] >= 0)
				return true;
		}

		if (_sz == OpSize_32)
		{
			if (poper->alltagsH[_rold] >= 0)
			{
				if (poper->alltagsH[_rnew] >= 0)
					return true;
			}
		}

	}

	return false;
}

#define USE_XFORM_ADDSUBQ (1)

#include "68kxforms.inl"



void Context::try_transforms(std::list<Operation*>::iterator &curr, OpSize _opsize, int _rold, uword_t _dataH, uword_t _dataL)
{
	// can't transform variable registers
	if (registers[_rold].variable)
		return;

	uword_t data16 = _dataL;
	ulong_t data32 = (ulong_t(_dataH) << 16) | _dataL;

	// try cheapest form of init (CLR/MOVEQ)
	if (try_4c_init_transforms(curr, _opsize, _rold, _dataH, _dataL))
		goto terminate;

	// look for cheapest transforms of free register values
	{
		// start with self
		std::vector<dataregister>::iterator itself = registers.begin() + _rold;

		int limit = 4;
		if (try_reg_transforms(curr, itself, _opsize, _rold, _dataH, _dataL, limit))
			goto terminate;

		for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++)
		{
			if (it == itself)
				continue;

			int limit = 4;
			if (try_reg_transforms(curr, it, _opsize, _rold, _dataH, _dataL, limit))
				goto terminate;
		}
	}

	// try MOVEQ for 16bit loads to registers pre-initialized H
//#if (USE_MOVEQ_INITS)
	if (_opsize == OpSize_16)
	{
		// OPPORTUNITY: (MOVEQ.L #N,Rn.W == EXPECTED)
		if ((registers[_rold].regclass == RegClass_Data)
		)
		{
			for (int imm = -128; imm < 128; imm++)
			{
				if (data16 == uword_t(imm))
				{
					printf("transform: move.w #$%04x,%s -> moveq.l #%d,%s [$%04x]\n",
						data16,
						registers[_rold].name.c_str(),
						imm,
						registers[_rold].name.c_str(),
						registers[_rold].L
						);

					curr = operations.erase(curr);
					curr = operations.insert(curr, new DR_MoveQ32(*this, imm, _rold));
					goto terminate;
				}
			}
		}
	}
//#endif

	// look for compound replacements for MOVE.L #IMM,Rx
	if (try_8c_init_transforms(curr, _opsize, _rold, _dataH, _dataL))
		goto terminate;

	// look for more expensive transforms of free register values
	{
		// start with self
		std::vector<dataregister>::iterator itself = registers.begin() + _rold;

		int limit = 8;
		if (try_reg_transforms(curr, itself, _opsize, _rold, _dataH, _dataL, limit))
			goto terminate;

		for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++)
		{
			if (it == itself)
				continue;

			int limit = 8;
			if (try_reg_transforms(curr, it, _opsize, _rold, _dataH, _dataL, limit))
				goto terminate;
		}
	}

	// look for more expensive transforms of free register values
	{
		// start with self
		std::vector<dataregister>::iterator itself = registers.begin() + _rold;

		int limit = 12;
		if (try_reg_transforms(curr, itself, _opsize, _rold, _dataH, _dataL, limit))
			goto terminate;

		for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++)
		{
			if (it == itself)
				continue;

			int limit = 12;
			if (try_reg_transforms(curr, it, _opsize, _rold, _dataH, _dataL, limit))
				goto terminate;
		}
	}

	// look for most expensive transforms of free register values
	if (srcww > 2)
	{
		// start with self
		std::vector<dataregister>::iterator itself = registers.begin() + _rold;

		int limit = 100;
		if (try_reg_transforms(curr, itself, _opsize, _rold, _dataH, _dataL, limit))
			goto terminate;

		for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++)
		{
			if (it == itself)
				continue;

			int limit = 100;
			if (try_reg_transforms(curr, it, _opsize, _rold, _dataH, _dataL, limit))
				goto terminate;
		}
	}

	// try more expensive replacements for MOVE.L #IMM,Rx
	//if (try_12c_init_transforms(curr, _opsize, _rold, _dataH, _dataL))
	//	goto terminate;

terminate:;
}

void Context::transform_pass()
{
	reset_sim(true);

	register_sweep();

	printf("transform pass: replace word #immediate register loads with derived register values\n");

	for (std::list<Operation*>::iterator it = operations.begin(); it != operations.end();)
	{
		std::list<Operation*>::iterator curr = it++;
		Operation *pop = *curr;

		if (pop->kind == OpKind_LoadI16)
		{
			DR_LoadI16 *pactual = reinterpret_cast<DR_LoadI16*>(pop);
			int rold = pactual->dreg;

			try_transforms(curr, OpSize_16, rold, 0, pactual->data);
		}
		else
		if (pop->kind == OpKind_LoadI32)
		{
			DR_LoadI32 *pactual = reinterpret_cast<DR_LoadI32*>(pop);
			int rold = pactual->dreg;

			//if ((pactual->dataH == 0) && (pactual->dataL == 0xe0))
			//{
			//	int a = 0;
			//}

			try_transforms(curr, OpSize_32, rold, pactual->dataH, pactual->dataL);
		}

		// catch up with execution from last insert point
		while (curr != it)
		{
			register_sweep();
			(*curr)->execute(); 
			curr++;
		}
	}
}


void Context::cleanup_pass()
{
	reset_sim(true);

	register_sweep();

	printf("cleanup pass: replace #immediate EM stores with modified register values\n");

	for (std::list<Operation*>::iterator it = operations.begin(); it != operations.end();)
	{
		std::list<Operation*>::iterator curr = it++;
		Operation *pop = *curr;

		if (pop->kind == OpKind_DSTLea)
		{
			DST_Lea *pactual = reinterpret_cast<DST_Lea*>(pop);

			for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++)
			{
				int rnew = it->index;

				if (it->stableL &&
					(word_t(pactual->delta * dstlinewid) == word_t(it->L)))
				{
					if (g_allargs.verbose)
						printf("transform: lea.w %d(%s),%s -> adda.w %s,%s [$%04x]\n",
						pactual->delta * dstlinewid,
						registers[SCRLINE_reg].name.c_str(), registers[SCRLINE_reg].name.c_str(),
						registers[rnew].name.c_str(),
						registers[SCRLINE_reg].name.c_str(),
						registers[rnew].L
						);
					curr = operations.erase(curr);
					curr = operations.insert(curr, new DST_AddaR16(*this, rnew, SCRLINE_reg));
					goto terminate;
				}

				if (it->stableL &&
					(word_t(pactual->delta * dstlinewid) == -word_t(it->L)))
				{
					if (g_allargs.verbose)
					printf("transform: lea.w %d(%s),%s -> suba.w %s,%s [$%04x]\n",
						pactual->delta * dstlinewid,
						registers[SCRLINE_reg].name.c_str(), registers[SCRLINE_reg].name.c_str(),
						registers[rnew].name.c_str(),
						registers[SCRLINE_reg].name.c_str(),
						registers[rnew].L
						);
					curr = operations.erase(curr);
					curr = operations.insert(curr, new DST_SubaR16(*this, rnew, SCRLINE_reg));
					goto terminate;
				}
			}
		}

		if (pop->kind == OpKind_StoreI16)
		{
			EM_MoveI16 *pactual = reinterpret_cast<EM_MoveI16*>(pop);
			int emold = pactual->em;
			int arold = pactual->areg;

			// now look for transforms of free register values
			{
				for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++)
				{
					int rnew = it->index;

					if (registers[rnew].stableL &&
						(pactual->data == (it->L)))
					{
						if (g_allargs.verbose)
						printf("transform: move.w #$%04x,%s -> move.w %s,%s [$%04x]\n", 
							pactual->data, 
							BLiTEM_names[emold].c_str(),
							registers[rnew].name.c_str(),
							BLiTEM_names[emold].c_str(),
							registers[rnew].L
						);
						curr = operations.erase(curr);
						curr = operations.insert(curr, new EM_MoveR16(*this, emold, rnew, arold));
						goto terminate;
					}
				}
			}
		}

		if (pop->kind == OpKind_StoreI32)
		{
			EM_MoveI32 *pactual = reinterpret_cast<EM_MoveI32*>(pop);
			int emold = pactual->em;
			int arold = pactual->areg;

			// now look for transforms of free register values
			{
				for (std::vector<dataregister>::iterator it = registers.begin(); it != registers.end(); it++)
				{
					int rnew = it->index;

					if (registers[rnew].stableL && (pactual->dataL == (it->L)) &&
						registers[rnew].stableH && (pactual->dataH == (it->H))
					)
					{
						if (g_allargs.verbose)
						printf("transform: move.l #$%04x%04x,%s -> move.l %s,%s [$%04x%04x]\n", 
							pactual->dataH, pactual->dataL, 
							BLiTEM_names[emold].c_str(),
							registers[rnew].name.c_str(),
							BLiTEM_names[emold].c_str(),
							registers[rnew].H,
							registers[rnew].L
						);
						curr = operations.erase(curr);
						curr = operations.insert(curr, new EM_MoveR32(*this, emold, rnew, arold));
						goto terminate;
					}
				}
			}
		}

		// convert long address loads to lea
		if (pop->kind == OpKind_LoadI32)
		{
			DR_LoadI32 *pactual = reinterpret_cast<DR_LoadI32*>(pop);

			if (registers[pactual->dreg].regclass == RegClass_Address)
			{
				if ((registers[pactual->dreg].H == 0) && (!(registers[pactual->dreg].L & 0x8000)))
				{
					// short +ve
					curr = operations.erase(curr);
					curr = operations.insert(curr, new AR_Lea16(*this, pactual->dreg, registers[pactual->dreg].L));
					goto terminate;
				}
				else
				if ((registers[pactual->dreg].H == 0xFFFF) && (registers[pactual->dreg].L & 0x8000))
				{
					// short -ve
					curr = operations.erase(curr);
					curr = operations.insert(curr, new AR_Lea16(*this, pactual->dreg, registers[pactual->dreg].L));
					goto terminate;
				}
				else
				{
					curr = operations.erase(curr);
					curr = operations.insert(curr, new AR_Lea32(*this, pactual->dreg, registers[pactual->dreg].H, registers[pactual->dreg].L));
					goto terminate;
				}
			}
		}


terminate:

		// catch up with execution from last insert point
		while (curr != it)
		{
			register_sweep();
			(*curr)->execute(); 
			curr++;
		}
	}
}

std::list<Operation*>::iterator Context::register_last_written(std::list<Operation*>::iterator _start, int _reg, OpSize _sz)
{
	// get tag for register used at point of interest
	Operation *popfirst = *_start;
	int tagH = popfirst->alltagsH[_reg];
	int tagL = popfirst->alltagsL[_reg];

	std::list<Operation*>::iterator itlast = _start;

	// find last write to this tag as dest
	for (std::list<Operation*>::iterator it = operations.begin(); it != operations.end(); it++)
	{
		Operation *pop = *it;

		// look for writes
		if ((pop->opdst == OpSource_Register) &&
			(pop->opwr != OpWrite_None)
			)
		{
			bool matchH = (pop->alltagsH[pop->dreg] == tagH) && (tagH >= 0) && (pop->alltagsH[pop->dreg] >= 0);
			bool matchL = (pop->alltagsL[pop->dreg] == tagL) && (tagL >= 0) && (pop->alltagsL[pop->dreg] >= 0);

			// only care about current tag
			if (_sz == OpSize_32)
			{
				if (matchH && matchL)
				{
					itlast = it;
				}
			}
			else
			{
				if (matchL)
				{
					itlast = it;
				}
			}
		}
	}

	return itlast;
}

int Context::register_count_srcrefs(std::list<Operation*>::iterator _start, int _reg, OpSize _sz)
{
	// get tag for register used at point of interest
	Operation *popfirst = *_start;
	int tagH = popfirst->alltagsH[_reg];
	int tagL = popfirst->alltagsL[_reg];
	int refs = 0;

/*	00
	00
	00
	00
	23
	23
	23 <-
	22
	 1
	 1
*/

	// count references to this tag as a source
	for (std::list<Operation*>::iterator it = operations.begin(); it != operations.end(); it++)
	{
		Operation *pop = *it;

		if (_sz == OpSize_32)
		{
			if (pop->opsrc == OpSource_Register)
			{
				bool matchH = (pop->alltagsH[pop->sreg] == tagH) && (tagH >= 0) && (pop->alltagsH[pop->sreg] >= 0);
				bool matchL = (pop->alltagsL[pop->sreg] == tagL) && (tagL >= 0) && (pop->alltagsL[pop->sreg] >= 0);

//				if (matchH && matchL)
//					refs++;
				if (matchH || matchL)
					refs++;
				//if (matchH)
				//	refs++;
				//if (matchL)
				//	refs++;
			}
			if (pop->areg >= 0)
			{
				bool match = (pop->alltagsL[pop->areg] == tagL) && (tagL >= 0) && (pop->alltagsL[pop->areg] >= 0);

				if (match)
					refs++;
			}
			if (pop->opdst == OpSource_Register)
			{
				bool matchH = (pop->alltagsH[pop->dreg] == tagH) && (tagH >= 0) && (pop->alltagsH[pop->dreg] >= 0);
				bool matchL = (pop->alltagsL[pop->dreg] == tagL) && (tagL >= 0) && (pop->alltagsL[pop->dreg] >= 0);

				if ((pop->opwr == OpWrite_Modify) || (pop->opwr == OpWrite_Combine))
				{
					if (matchH || matchL)
						refs++;
					//if (matchH && matchL)
					//	refs++;
				}
			}
		}
		else
		{
			if (pop->opsrc == OpSource_Register)
			{
				bool matchL = (pop->alltagsL[pop->sreg] == tagL) && (tagL >= 0) && (pop->alltagsL[pop->sreg] >= 0);

				if (matchL)
					refs++;
			}
			if (pop->areg >= 0)
			{
				bool match = (pop->alltagsL[pop->areg] == tagL) && (tagL >= 0) && (pop->alltagsL[pop->areg] >= 0);

				if (match)
					refs++;
			}
			if (pop->opdst == OpSource_Register)
			{
				bool matchL = (pop->alltagsL[pop->dreg] == tagL) && (tagL >= 0) && (pop->alltagsL[pop->dreg] >= 0);

				if ((pop->opwr == OpWrite_Modify) || (pop->opwr == OpWrite_Combine) || (pop->opwr == OpWrite_ReplaceHCombineL))
					if (matchL)
						refs++;
			}
		}
	}

	return refs;
}

void Context::reduce_pass()
{
	reset_sim(true);

//	return;

	register_sweep();

	printf("load reduction pass: replace redundant register load/stores with immediate stores\n");

	for (std::list<Operation*>::iterator it = operations.begin(); it != operations.end();)
	{
		std::list<Operation*>::iterator curr = it++;
		Operation *pop = *curr;

		int reg = -1;
		int em = -1;

		if (pop->kind == OpKind_StoreR16)
		{
			EM_MoveR16 *pactual = reinterpret_cast<EM_MoveR16*>(pop);
			reg = pactual->sreg;
			em = pactual->em;
		}

		if (pop->kind == OpKind_StoreR32)
		{
			EM_MoveR32 *pactual = reinterpret_cast<EM_MoveR32*>(pop);
			reg = pactual->sreg;
			em = pactual->em;
		}

		if (reg >= 0)
		{
			// this is a 16 or 32bit EM store from a register
			// now check to see if this register is used as a source elsewhere
			int refs = register_count_srcrefs(curr, reg, pop->opsize);
			if (!(registers[reg].locked) && (refs == 1))
			{
//				printf("possible reduce opportunity: reg %d [$%04x%04x]\n", reg, registers[reg].H, registers[reg].L);

				// this register is only referenced as a source once - here
				// now determine *where* the register was written, to see if it was an immediate load
				std::list<Operation*>::iterator itwrite = register_last_written(curr, reg, pop->opsize);
				Operation* pwrite = *itwrite;

				// ensure the write fully accounts for the read, so it may be safely dissolved
				if ((pwrite->kind == OpKind_LoadI32) || 
					((pwrite->kind == OpKind_LoadI16) && (pop->kind == OpKind_StoreR16))
				)
				{
					printf("reduce opportunity: reg %d [$%04x%04x]\n", reg, registers[reg].H, registers[reg].L);

					int arold = pwrite->areg;

					// remove load
					operations.erase(itwrite);

					// replace store
					if (pop->kind == OpKind_StoreR16)
					{
						curr = operations.erase(curr);
						curr = operations.insert(curr, new EM_MoveI16(*this, em, registers[reg].L, arold));
					}
					else
					{
						curr = operations.erase(curr);
						curr = operations.insert(curr, new EM_MoveI32(*this, em, registers[reg].H, registers[reg].L, arold));
					}
				}
			}
		}

terminate:

		// catch up with execution from last insert point
		while (curr != it)
		{
			register_sweep();
			(*curr)->execute(); 
			curr++;
		}
	}
}

void Context::tail_pass()
{
	reset_sim(true);

	register_sweep();

	printf("tail optimization: move BLiT operations to hide long tails...\n");

	// start with last operation, walk backwards using forward iterator in order to fork sub-search
	std::list<Operation*>::iterator it = operations.end(); it--;
	std::list<Operation*>::iterator itstop = operations.begin();

	while (it != itstop)
	{
		std::list<Operation*>::iterator curr = it--;
		Operation *pop = *curr;

		// encountered blit operation
		if (pop->kind == OpKind_BLITGo)
		{
			BLiT_Go *pactual = reinterpret_cast<BLiT_Go*>(pop);

			std::list<Operation*>::iterator best_tailop = curr;

			std::list<Operation*>::iterator itscan = curr; itscan++;

			int best_tailval =  (*itscan)->tail;

			for (; itscan != operations.end(); itscan++)
			{
				OpKind kind = (*itscan)->kind;

				// observe routine end
				if (kind == OpKind_Terminate)
					break;
				// can't reorder blit beyond register stores
				if (kind == OpKind_StoreI16)
					break;
				if (kind == OpKind_StoreI32)
					break;
				if (kind == OpKind_StoreR16)
					break;
				if (kind == OpKind_StoreR32)
					break;
				if (kind == OpKind_BLITSetup)
					break;

				int trytail = (*itscan)->tail;

				if (trytail > best_tailval)
				{
					best_tailval = trytail;
					best_tailop = itscan;
				}
			}

			// if we found a better position, move the blit
			if (best_tailop != curr)
			{
				printf("relocating blit to hide tail=%d cycles\n", best_tailval);
				operations.erase(curr);
				operations.insert(best_tailop, pop);
			}
		}
	};

terminate:;
}



void generate_emx_frame(spriteframe_t &spr, int _frame)
{
	printf("frame [%d]:\n", _frame);

	// default line order
	std::vector<int> best_pattern;
	std::vector<int> best_invpattern;
	for (int f = 0; f < spr.h_; f++)
	{
		best_pattern.push_back(f);
	}

	// find common optimized line order
	if (USE_COMMON_LINEORDER_OPTIMIZATION && (spr.h_ > 1))
	{
		printf("optimizing common line order across all preshifts...\n");

		int shiftcount = spr.psmask_frames_.size();

		// make copy of data in terms of 3*y EM words
		uword_t *buffer = new uword_t[3 * spr.h_ * shiftcount];
		memset(buffer, 0, sizeof(uword_t) * 3 * spr.h_ * shiftcount);
		{
			int si = 0;
			preshiftlist_t::iterator pit = spr.psmask_frames_.begin();
			for (; pit != spr.psmask_frames_.end(); pit++, si++)
			{
				preshift_t &shift = *pit;

				uword_t *sbuffer = &buffer[3 * spr.h_ * si];
//				uword_t *sbuffer = buffer;

				for (int v = 0; v < spr.h_; v++)
				{
					sbuffer[(v * 3) + 0] = shift.data_[(v * shift.wordwidth_) + 0];

					if (shift.wordwidth_ == 2)
					{
						// note: em1,em3 used in 2-wide blits. em2 is skipped
						sbuffer[(v * 3) + 2] = shift.data_[(v * shift.wordwidth_) + 1];
					}
					else
					if (shift.wordwidth_ == 3)
					{
						sbuffer[(v * 3) + 1] = shift.data_[(v * shift.wordwidth_) + 1];
						sbuffer[(v * 3) + 2] = shift.data_[(v * shift.wordwidth_) + 2];
					}
				}
			}
		}

		// optimize the line order to minimize changes to EM registers
		{
			mersenne_twister twist;

			// begin with default line pattern (top-to-bottom)
			int best_cost = 1 << 20;
			int best_loadcost = best_cost;
			int last_improvement = 0;

			// optimize line pattern by reordering
			for (int trial = 0; trial < (1 << DARWIN_DEPTH); trial++)
			{
				std::vector<int> trial_pattern(best_pattern);

				// select mutation rate
				int mutrate = ((twist.genS32() >> 4) % 6) + 1;

				// mutate
				for (int mut = 0; mut < mutrate; mut++)
				{
					// select pair to exchange
					int choice1 = (twist.genS32() >> 4) % trial_pattern.size();
					int choice2 = (twist.genS32() >> 4) % trial_pattern.size();
					while (choice1 == choice2)
						choice2 = (twist.genS32() >> 4) % trial_pattern.size();

					// reorder
					int tmp = trial_pattern[choice1];
					trial_pattern[choice1] = trial_pattern[choice2];
					trial_pattern[choice2] = tmp;
				}

				// estimate efficiency of new pattern in terms of deltas per line
				{
					int totalcost = 0;
					int loadcost = 0;

					int shiftcount = spr.psmask_frames_.size();

					for (int si = 0; si < shiftcount; si++)
					{
						uword_t *sbuffer = &buffer[3 * spr.h_ * si];
//						uword_t *sbuffer = buffer;

						for (int v = 0; v < (spr.h_ - 1); v++)
						{
							int v0 = trial_pattern[v + 0];
							int v1 = trial_pattern[v + 1];
							int yp = v0 * 3;
							int yn = v1 * 3;
							int lc1 = (sbuffer[yp + 0] != sbuffer[yn + 0]) ? 1 : 0;
							int lc2 = (sbuffer[yp + 1] != sbuffer[yn + 1]) ? 1 : 0;
							int lc3 = (sbuffer[yp + 2] != sbuffer[yn + 2]) ? 1 : 0;

							totalcost += (lc1 + lc2 + lc3) << 8;
							loadcost += lc1 + lc2 + lc3;

							// incur a slight penalty for split longs
							if (lc1 == 1 && lc3 == 1 && lc2 == 0)
								totalcost++;

							// attempt to keep lines roughly in top-bottom order by including small cost for 'pointless' reordering
							// this provides more opportunity to use a fixed increment between lines (save space with register add vs lea)
							// notes: 
							// - some chance this could prevent a difficult-to-reach optimization via small incremental mutations
							// - perhaps a bit more likely to result in extra LEA at start, to reach first block of contiguous lines 

							// cost is same for forward or reverse ordering - delta value will be the same even if instruction isn't
							int dycost = (v1 - v0);
							if (dycost < 0)
								dycost = -dycost;
							if (dycost > 8)
								dycost = 8;

							totalcost += dycost;
						}
					}

					if (totalcost < best_cost)
					{
						best_cost = totalcost;
						best_loadcost = loadcost;
						best_pattern = trial_pattern;
						last_improvement = trial;
					}
				}
			}

			printf("\nconverged or exhausted at %d trials @ loadcost=%d (totalcost=%d)\n", last_improvement, best_cost, best_loadcost);

			// output the best pattern
			if (g_allargs.verbose)
			for (int v = 0; v < spr.h_; v++)
			{
				int y = best_pattern[v];
				print_dataword(buffer[(y * 3) + 0]);
				print_dataword(buffer[(y * 3) + 1]);
				print_dataword(buffer[(y * 3) + 2]);
				printf("\n");
			}
		}

		delete[] buffer;
	}

	spr.lineorder = best_pattern;

	{
		for (int v = 0; v < best_pattern.size(); v++)
		{
			for (int i = 0; i < best_pattern.size(); i++)
			{
				if (v == best_pattern[i])
				{
					best_invpattern.push_back(i);
					break;
				}
			}
		}
	}

	// generate metrics for each data word
	int pi = 0;
	preshiftlist_t::iterator pit = spr.psmask_frames_.begin();

//	for (int x = 0; x < 12; x++, pit++);

//	pit++; pi++;

//	preshiftlist_t::iterator pit = spr.psmask_frames_.end();
//	pit--;

	for (; pit != spr.psmask_frames_.end(); pit++, pi++)
	{
		preshift_t &shift = *pit;

		Context context(shift.wordwidth_, shift.height_);

		printf("\n\npreshift %d preview:\n", pi);

		// output the source image pattern
		if (g_allargs.verbose)
		{
			for (int v = 0; v < shift.height_; v++)
			{
				for (int h = 0; h < shift.wordwidth_; h++)
				{
					print_dataword(shift.data_[(v * shift.wordwidth_) + h]);
				}
				printf("\n");
			}
			printf("\n");
		}

		byte_t *changes = new byte_t[shift.planewords_ * shift.height_];
		memset(changes, 0, shift.planewords_ * shift.height_);

		// make copy of data in terms of 3*y EM words
		uword_t *buffer = new uword_t[3 * shift.height_];
		memset(buffer, 0, 2 * 3 * shift.height_);
		{
			for (int v = 0; v < shift.height_; v++)
			{
				buffer[(v * 3) + 0] = shift.data_[(v * shift.wordwidth_) + 0];

				if (shift.wordwidth_ == 2)
				{
					// note: em1,em3 used in 2-wide blits. em2 is skipped
					buffer[(v * 3) + 2] = shift.data_[(v * shift.wordwidth_) + 1];
				}
				else
				if (shift.wordwidth_ == 3)
				{
					buffer[(v * 3) + 1] = shift.data_[(v * shift.wordwidth_) + 1];
					buffer[(v * 3) + 2] = shift.data_[(v * shift.wordwidth_) + 2];
				}

				//for (int h = 0; h < shift.wordwidth_ && h < 3; h++)
				//{
				//	buffer[(v * 3) + h] = shift.data_[(v * shift.wordwidth_) + h];
				//}
			}
		}

		if (USE_PERSHIFT_LINEORDER_OPTIMIZATION && (shift.height_ > 1))
		{
			best_pattern.clear();
			for (int f = 0; f < shift.height_; f++)
			{
				best_pattern.push_back(f);
			}

			printf("optimizing line order...\n");

			// optimize the line order to minimize changes to EM registers
			{
				mersenne_twister twist;

				// begin with default line pattern (top-to-bottom)
				int best_cost = 1 << 20;
				int best_loadcost = best_cost;
				int last_improvement = 0;

				// optimize line pattern by reordering
				for (int trial = 0; trial < (1 << DARWIN_DEPTH); trial++)
				{
					std::vector<int> trial_pattern(best_pattern);

					// select mutation rate
					int mutrate = ((twist.genS32() >> 4) % 6) + 1;

					// mutate
					for (int mut = 0; mut < mutrate; mut++)
					{
						// select pair to exchange
						int choice1 = (twist.genS32() >> 4) % trial_pattern.size();
						int choice2 = (twist.genS32() >> 4) % trial_pattern.size();
						while (choice1 == choice2)
							choice2 = (twist.genS32() >> 4) % trial_pattern.size();

						// reorder
						int tmp = trial_pattern[choice1];
						trial_pattern[choice1] = trial_pattern[choice2];
						trial_pattern[choice2] = tmp;
					}

					// estimate efficiency of new pattern in terms of deltas per line
					{
						int totalcost = 0;
						int loadcost = 0;

						for (int v = 0; v < (shift.height_ - 1); v++)
						{
							int v0 = trial_pattern[v + 0];
							int v1 = trial_pattern[v + 1];
							int yp = v0 * 3;
							int yn = v1 * 3;
							int lc1 = (buffer[yp + 0] != buffer[yn + 0]) ? 1 : 0;
							int lc2 = (buffer[yp + 1] != buffer[yn + 1]) ? 1 : 0;
							int lc3 = (buffer[yp + 2] != buffer[yn + 2]) ? 1 : 0;

							totalcost += (lc1 + lc2 + lc3) << 8;
							loadcost += lc1 + lc2 + lc3;

							// incur a slight penalty for split longs
							if (lc1==1 && lc3==1 && lc2==0)
								totalcost++;

							// attempt to keep lines roughly in top-bottom order by including small cost for 'pointless' reordering
							// this provides more opportunity to use a fixed increment between lines (save space with register add vs lea)
							// notes: 
							// - some chance this could prevent a difficult-to-reach optimization via small incremental mutations
							// - perhaps a bit more likely to result in extra LEA at start, to reach first block of contiguous lines 

							// cost is same for forward or reverse ordering - delta value will be the same even if instruction isn't
							int dycost = (v1 - v0);
							if (dycost < 0)
								dycost = -dycost;
							if (dycost > 8)
								dycost = 8;

							totalcost += dycost;
						}

						if (totalcost < best_cost)
						{
							best_cost = totalcost;
							best_loadcost = loadcost;
							best_pattern = trial_pattern;
							last_improvement = trial;
						}
					}
				}

				printf("\nconverged or exhausted at %d trials @ loadcost=%d (totalcost=%d)\n", last_improvement, best_cost, best_loadcost);

				// output the best pattern
				if (g_allargs.verbose)
				for (int v = 0; v < shift.height_; v++)
				{
					int y = best_pattern[v];
					print_dataword(buffer[(y * 3) + 0]);
					print_dataword(buffer[(y * 3) + 1]);
					print_dataword(buffer[(y * 3) + 2]);
					printf("\n");
				}
			}
		}


		// generate map for only changing EM registers 
		// (excluding first output line - this is handled as dirty registers to minimize initial setup if the first line is empty)

		// mark first line as dirty
		//int y = best_pattern[0] * 3;
		//changes[y + 0] = 1;
		//if (shift.wordwidth_ >= 2)
		//	changes[y + 2] = 1;
		//if (shift.wordwidth_ == 3)
		//	changes[y + 1] = 1;

		if (USE_DELTA_OPTIMIZATION)
		{
			uword_t lastchanges[3];
			//bool got_first_line = false;

			for (int v = 0; v < shift.height_; v++)
			{
				// generate deltas for all but 1st line
				int yn = best_pattern[v + 0] * 3;
				int yp = yn;
				if (v > 0)
					yp = best_pattern[v - 1] * 3;

				// is this a real line?
				uword_t m = buffer[yn + 0];
				if (shift.wordwidth_ >= 2)
					m |= buffer[yn + 2];
				if (shift.wordwidth_ == 3)
					m |= buffer[yn + 1];

				// generate deltas for only real lines
				if (m)
				{
					changes[yn + 0] = (lastchanges[0] != buffer[yn + 0]) ? 1 : 0;
					if (shift.wordwidth_ == 3)
						changes[yn + 1] = (lastchanges[1] != buffer[yn + 1]) ? 1 : 0;
					if (shift.wordwidth_ >= 2)
						changes[yn + 2] = (lastchanges[2] != buffer[yn + 2]) ? 1 : 0;

					// accept new data for non-empty lines
					lastchanges[0] = buffer[yn + 0];
					lastchanges[1] = buffer[yn + 1];
					lastchanges[2] = buffer[yn + 2];
				}
			}
		}
		else
		{
			for (int v = 1; v < shift.height_; v++)
			{
				int y = best_pattern[v] * 3;

				changes[y + 0] = true;
				changes[y + 1] = (shift.wordwidth_ == 3);
				changes[y + 2] = (shift.wordwidth_ >= 2);
			}
		}

		bool em_dirty[3];

		printf("deltas...\n");

		// reset dirty EM registers for this blit width
		em_dirty[0] = true;
		em_dirty[1] = (shift.wordwidth_ > 2);
		em_dirty[2] = (shift.wordwidth_ > 1);

		for (int v = 0; v < shift.height_; v++)
		{
			int y = best_pattern[v] * 3;

			if (changes[y + 0] || em_dirty[0])
			{
				em_dirty[0] = false;
				if (g_allargs.verbose)
					print_dataword(buffer[y + 0]);
			}
			else
			{
				if (g_allargs.verbose)
					printf("................");
			}

			if (changes[y + 1] || em_dirty[1])
			{
				em_dirty[1] = false;
				if (g_allargs.verbose)
					print_dataword(buffer[y + 1]);
			}
			else
			{
				if (g_allargs.verbose)
					printf("................");
			}

			if (changes[y + 2] || em_dirty[2])
			{
				em_dirty[2] = false;
				if (g_allargs.verbose)
					print_dataword(buffer[y + 2]);
			}
			else
			{
				if (g_allargs.verbose)
					printf("................");
			}

			if (g_allargs.verbose)
				printf("\n");
		}

		class lineout
		{
		public:

			lineout(int s, int d)
				: sline(s), dline(d)
			{
			}

			int sline;
			int dline;
		};

		std::vector<lineout> active_lines;

		printf("\nsurviving datawords...\n");

		// reset dirty EM registers for this blit width
		em_dirty[0] = true;
		em_dirty[1] = (shift.wordwidth_ > 2);
		em_dirty[2] = (shift.wordwidth_ > 1);

//		int unique_datawords = 0; // each unique dataword has a uid so it can be reused
		{
//			linemap lines;
			for (int y = 0; y < shift.height_; y++)
			{
				uword_t *srcdata = &buffer[best_pattern[y] * 3];
				byte_t *linechanges = &changes[best_pattern[y] * 3];

				// identify duplicate lines in terms of complete EM state
				u64 line = 0;
				for (int x = 0; x < 3; x++)
				{
					line <<= 16;
					line |= srcdata[x];
				}

				// generate metrics for nonempty, unique lines only - lines where 1 or more EM registers must change
				if ((em_dirty[0] || em_dirty[1] || em_dirty[2]) /*|| 
					(line && (lines.find(line) == lines.end())*/
				)
				{

					// mark this line configuration as encountered
//					if (USE_LINEDUPE_OPTIMIZATION)
//						lines[line] = 1;

					// gather metrics for datawords on this line
					bool changed = false;
					for (int x = 0; x < 3; x++)
					{
						if (!linechanges[x] && !em_dirty[x])
						{
							if (g_allargs.verbose)
								printf("----------------");
							continue;
						}

						changed = true;

						uword_t dataword = srcdata[x];
						emx_metric_t &m = context.metrics[dataword];

						if (g_allargs.verbose)
							print_dataword(dataword);

						//if (m.name < 0)
						//	m.name = unique_datawords++;

						// incidence of this word
						m.incidence++;
						m.references++;

						//// incidence at each EM position
						//if (x == 0)
						//	m.incidence_em0++;
						//else
						//	if (x == 1)
						//		m.incidence_em1++;
						//	else
						//		if (x == 2)
						//			m.incidence_em2++;

						// incidence of this word relative to other words
						for (int lna = 0; lna < 3; lna++)
						{
							if ((x != lna) && (linechanges[lna]||em_dirty[lna]))
							{
								uword_t lnaword = srcdata[lna];

								// relative to all other words referenced on same line
								m.line_adjacencies[lnaword]++;

								// relative to other words used immediately to the right
								if (x == lna - 1)
								{
									m.left_adjacencies[lnaword]++;
									m.left_references[lnaword]++;
								}
							}
						}
					}

					// track this as a live line which changes
					if (changed)
						active_lines.push_back(lineout(y, best_pattern[y]));

					// all EM words on this line have been dealt with one way or another
					em_dirty[0] = em_dirty[1] = em_dirty[2] = false;

					if (g_allargs.verbose)
						printf("\n");
				}
			}
		}

		printf("total surviving = %d\n", context.metrics.size());

		printf("\nscoring:\n");

		// globally score each data word according to metrics
		int assigned_registers = 0;
		for (metricmap::iterator mit = context.metrics.begin(); mit != context.metrics.end(); mit++)
		{
			const uword_t &dataword = mit->first;

			if (g_allargs.verbose)
				print_dataword(dataword);

			emx_metric_t &m = mit->second;

			if ((m.name < 0) && (m.incidence > 1))
			{
				m.name = assigned_registers++;
			}

			float fscore = log(1.0 + (float)m.incidence);

			float sharing = 0.0f;
			for (relationtable::iterator rit = m.line_adjacencies.begin(); rit != m.line_adjacencies.end(); rit++)
				sharing += (float)rit->second;

			// score never modulates to 0 - can only increase with sharing
			fscore *= (1.0f + log(1.0 + sharing));

			m.fincidence_score = fscore;

			if (g_allargs.verbose)
				printf(" incidence_score: %.2f\n", fscore);
		}

		// globally score each data word according to its left-adjacency (upper word opportuntiy)
		for (metricmap::iterator mit = context.metrics.begin(); mit != context.metrics.end(); mit++)
		{
			const uword_t &dataword = mit->first;

			if (g_allargs.verbose)
				print_dataword(dataword);

			emx_metric_t &m = mit->second;

			float fscore = m.fincidence_score;

			float fadj = 0.0f;
			for (relationtable::iterator rit = m.left_adjacencies.begin(); rit != m.left_adjacencies.end(); rit++)
				fadj += (float)rit->second;

			// score can modulate to 0 if this dataword never appears on the left of any other
			fscore *= (0.0f + log(1.0 + fadj));

			m.fadjacency_score = fscore;

			if (g_allargs.verbose)
				printf(" adjacency_score: %.2f\n", fscore);
		}

		// find the most useful values

		typedef std::map<float, std::list<metricmap::iterator> > usemap;

		usemap usemap_lw;
		usemap usemap_hw;

		for (metricmap::iterator mit = context.metrics.begin(); mit != context.metrics.end(); mit++)
		{
			usemap_lw[mit->second.fincidence_score].push_back(mit);
			usemap_hw[mit->second.fadjacency_score].push_back(mit);
		}

		for (usemap::reverse_iterator uit = usemap_lw.rbegin(); uit != usemap_lw.rend(); uit++)
		{
			for (std::list<metricmap::iterator>::iterator lit = uit->second.begin(); lit != uit->second.end(); lit++)
			{
				const uword_t &dataword = (*lit)->first;

				emx_metric_t &m = context.metrics[dataword];

				if (m.incidence >= 2)
				{
					if (g_allargs.verbose)
						printf("L preload: [$%04x] iscore: %.2f\n", dataword, uit->first);
					context.preloads.push_back(preload(dataword));
				}
			}
		}

		for (usemap::reverse_iterator uit = usemap_hw.rbegin(); uit != usemap_hw.rend(); uit++)
		{
			float h_score = uit->first;

			for (std::list<metricmap::iterator>::iterator lit = uit->second.begin(); lit != uit->second.end(); lit++)
			{
				const uword_t &dataword = (*lit)->first;

				// metrics for this word relative to others
				emx_metric_t &m = context.metrics[dataword];

//				if (m.incidence > 1)
				{
					if (g_allargs.verbose)
						printf("H preload: [$%04x] ascore: %.2f\n", dataword, h_score);


					// sort adjacencies with indirection

					typedef std::map<int, std::list<relationtable::iterator> > hitmap;
					hitmap hits;

					for (relationtable::iterator relit = m.left_adjacencies.begin(); relit != m.left_adjacencies.end(); relit++)
					{
						int h = relit->second;
						hits[h].push_back(relit);
					}

					// scan through the most promising adjacents
					for (hitmap::reverse_iterator hit = hits.rbegin(); hit != hits.rend(); hit++)
					{
						std::list<relationtable::iterator> &competing = hit->second;

						while (!competing.empty())
						{
							relationtable::iterator relit = competing.back();
							competing.pop_back();

							uword_t adjacent_low = relit->first;

							if (g_allargs.verbose)
								printf("candidate adjacent low: [$%04x] hits: %d\n", adjacent_low, relit->second);

							// try to locate it in the existing pool of L preloads
							// if we find a match, bind it and break

							for (std::vector<preload>::iterator pit = context.preloads.begin(); pit != context.preloads.end(); pit++)
							{
								if (pit->validL && (pit->L == adjacent_low))
								{
									pit->H = dataword;
									pit->validH = true;

									if (g_allargs.verbose)
										printf("bound preload [$%04x]:[$%04x]\n", dataword, adjacent_low);

									goto break_bind;
								}
							}
						}
					}
				break_bind:;
				}
			}
		}


		// preload most useful values

		//context

		// data dependencies
		// process all *changing words* on *changing lines* in *optimized line order*

		class storedependency
		{
		public:
			storedependency(int _name, int _pos, int _job, int _type, uword_t _H, uword_t _L)
				: name(_name), pos(_pos), job(_job), type(_type), H(_H), L(_L)
			{
			}

			int name;
			int pos;
			int job;
			int type;
			uword_t H;
			uword_t L;
		};

		std::vector<storedependency> deps;
		std::vector<u64> lifespans;

		// todo: determine max regs available for final code
		int max_regs = 4;

		// 1)
		// put each piece of data in a new register in order of need, assuming an infinute number of free 
		// registers (to begin with).
		// note that in worst case, several changes may involve the same data at spaced intervals desite
		// minimization of deltas - because multiple words may change at once in any line. 
		// while such data probably can survive in a register throughout its needed lifespan, it may need 
		// to be reallocated if registers become exhausted - so we do a full register rename and reduce.

		// reset dirty EM registers for this blit width
		em_dirty[0] = true;
		em_dirty[1] = (shift.wordwidth_ > 2);
		em_dirty[2] = (shift.wordwidth_ > 1);

		std::vector<int> blits;

		int stage = 0;
//		int regcount = 0;
		u64 lifebits = 0;
		int job = 0;
		for (std::vector<lineout>::iterator lit = active_lines.begin(); lit != active_lines.end(); lit++, job++)
		{
			lineout &lo = *lit;
			int y = lo.dline;

			uword_t *srcdata = &buffer[y * 3];
			byte_t *linechanges = &changes[y * 3];

			blits.push_back(deps.size());

			for (int x = 0; x < 3; x++)
			{
				if (!(linechanges[x]||em_dirty[x]))
					continue;

				uword_t dataword = srcdata[x];

				emx_metric_t &m = context.metrics[dataword];

				// resource type: 0 = reused (register), 1 = single-use (#immediate/transform)
//				int regname = -1;
				int type = 1;
				if (m.incidence > 1)
				{
					type = 0;
//					regname = regcount++;
				}

				// todo: load H word with best candidate for each L data word (if any)
				deps.push_back(storedependency(m.name, stage, job, type, 0, dataword));
				if (m.name >= 0)
					lifebits |= (u64(1) << m.name);
				lifespans.push_back(lifebits);
				stage++;

				em_dirty[x] = false;
			}
		}

		// 2)
		// trim lifespans for each piece of data
		u64 cancelbits = 0;
		std::vector<u64>::reverse_iterator lit = lifespans.rbegin();
		for (std::vector<storedependency>::reverse_iterator rit = deps.rbegin(); rit != deps.rend(); rit++, lit++)
		{
			storedependency &dr = *rit;
			u64 &lifebits = *lit;

			if (dr.name >= 0)
			{
				// register name for this data
				u64 activebit = u64(1) << dr.name;

				// prevent cancelling of this data's lifespan from here backwards
				cancelbits |= activebit;
			}

			// cancel lifespans for data not yet encountered
			lifebits &= cancelbits;
		}

		if (g_allargs.verbose)
			printf("\n\ninitial data lifespans:\n");
		//for (std::vector<storedependency>::iterator rit = deps.begin(); rit != deps.end(); rit++)
		//{
		//	storedependency &dr = *rit;
		//	if (dr.type == 0)
		//		printf("R");
		//	else
		//	if (dr.type == 1)
		//		printf("I");
		//}
		//printf("\n");

		// report lifespan status for each state for each execution stage
		if (g_allargs.verbose)
		{
			std::vector<storedependency>::iterator dit = deps.begin();
			for (std::vector<u64>::iterator lit = lifespans.begin(); lit != lifespans.end(); lit++, dit++)
			{
				assert(dit != deps.end());
				print_data(*lit, /*unique_datawords*/assigned_registers); printf(" %s\n", (dit->type == 0) ? "R" : "U");
			}
		}


		// look for ways to derive unique values without loads, by transforming values with expired lifespan
			
		//for (std::vector<storedependency>::iterator dit = deps.begin(); dit != deps.end(); dit++)
		//{
		//	storedependency &dep = *dit;

		//	if (dep.type == 1)
		//	{
		//		// this dependency is isolated and expensive to load, so look for different ways to derive it from spent values
		//	}
		//}

		//for (std::vector<storedependency>::iterator dit = deps.begin(); dit != deps.end(); dit++)
		//{
		//	storedependency &dep = *dit;

		//	if (dep.type == 0)
		//	{
		//		// this value gets reused so place it in registers


		//	}
		//}


		// 3)
		// optimize assignments to fit available resources
		// - finish with no more than max_registers unique names
		// - avoid interrupting long lifespans (or lifespans of high incidence data)
		// - don't mess with immediate types
		// - minimize number of load events



		//std::vector<int>::reverse_iterator lit = lifespans.rbegin();
		//for (std::vector<dataregister>::reverse_iterator rit = regs.rbegin(); rit != regs.rend(); rit++, lit++)
		//{
		//	dataregister &dr = *rit;
		//	int &lifebits = *lit;

		//	// register name for this data
		//	int activebit = 1 << dr.name;

		//	// prevent cancelling of this data's lifespan from here backwards
		//	cancelbits |= activebit;

		//	// cancel lifespans for data not yet encountered
		//	lifebits &= cancelbits;
		//}


		// SKEW + NFSR, DYINC
		context.blitter.skew = (shift.shift_ & 15) | (((shift.wordwidth_<<4) == ((spr.w_ + 15) & -16)) ? 0x00 : 0x40);
		context.blitter.dyinc = (8 + 2) - (shift.wordwidth_ << 3);

		context.reset_sim(true);

		// reset dirty EM registers for this blit width
		em_dirty[0] = true;
		em_dirty[1] = (shift.wordwidth_ > 2);
		em_dirty[2] = (shift.wordwidth_ > 1);

		// generate operations
		int current_line = 0;
		int reg_index = 0;
		int reg_index2 = 0;
		int reg_index3 = 0;
		{
#if (USE_NOP_BINARY_VALIDATION)
			context.operations.push_back(new NoOp(context));
			context.operations.back()->execute();
#endif

			context.operations.push_back(new SYS_PageRestore(context));
			context.operations.back()->execute();

			context.operations.push_back(new BLiT_Init(context));
			context.operations.back()->execute();
			
//			context.operations.push_back(new Terminate(context));
//			context.operations.back()->execute();

			// load fixed registers
			if (1)
			{
				for (std::vector<dataregister>::iterator it = context.registers.begin(); it != context.registers.end(); it++)
				{
					if (it->constant)
					{
						// use standard immediate load - let the optimizer reduce it if possible
						context.operations.push_back(new DR_LoadI32(context, it->index, it->H, it->L));

						it->constant = false;

						it->allocatedH = true;
						it->allocatedL = true;

						context.operations.back()->execute();
					}
				}

				// on next reset, registers will no longer be constant/preallocated
				context.setconstants = false;
			}
			


			linemap lines;
			for (int y = 0; y < shift.height_; y++)
			{
				int dsty = best_pattern[y];

				uword_t *srcdata = &buffer[dsty * 3];
				byte_t *linechanges = &changes[dsty * 3];

				// bypass empty blits (all EMs == 0)
				u64 line = 0;
				for (int x = 0; x < 3; x++)
				{
					line <<= 16;
					line |= srcdata[x];
				}

				// is the blit going to output something?
				if (line)
				{
					// line hop +/-
					int linejump = dsty - current_line;

					// all non-empty blits require a dst address update
					// todo: linewidth should be an argument
					if (linejump)
					{
						context.operations.push_back(new DST_Lea(context, context.SCRLINE_reg, context.SCRLINE_reg, linejump));
						context.operations.back()->execute();
					}

					// check for all 3 changing
					if ((linechanges[0] || em_dirty[0]) &&
						(linechanges[1] || em_dirty[1]) &&
						(linechanges[2] || em_dirty[2]))
					{
						int sumA = 0, sumB = 0;

						// test-resolve EM1:EM2 and EM2:EM3, looking for the cheapest solution
						DataSource ds12 = context.sim_resolve_data32(srcdata[0], srcdata[1], reg_index, reg_index2, false);
						DataSource ds3 = context.sim_resolve_data16(srcdata[2], reg_index3, false);

						sumA += DataSource_costs[ds12];
						sumA += DataSource_costs[ds3];

						DataSource ds23 = context.sim_resolve_data32(srcdata[1], srcdata[2], reg_index, reg_index2, false);
						DataSource ds1 = context.sim_resolve_data16(srcdata[0], reg_index3, false);

						sumB += DataSource_costs[ds23];
						sumB += DataSource_costs[ds1];

						int chosen = -1;
						if (sumA == sumB)
						{
							chosen = 0;

							// competing longs on same line
							if (ds12 == DataSource_Immediate32)
							{
								// competing immediates - pick immediate more likely to be optimized
								if (long_cost(srcdata[1], srcdata[2]) < long_cost(srcdata[0], srcdata[1]))
								{
									chosen = 1;
									if (g_allargs.verbose)
										printf("preferred long immediate with optimal value [$%04x%04x]\n", srcdata[1],srcdata[2]);
								}
								else
								{
									if (g_allargs.verbose)
										printf("preferred long immediate with optimal value [$%04x%04x]\n", srcdata[0], srcdata[1]);
								}
							}
							else 
							if (ds12 == DataSource_MixedIR || 
								ds12 == DataSource_RegisterHL)
							{
								// prefer long with higher incidence L
								if (context.metrics_sim[srcdata[2]].incidence < context.metrics_sim[srcdata[1]].incidence)
								{
									chosen = 1;
									if (g_allargs.verbose)
										printf("preferred long register with optimal L value [$%04x%04x]\n", srcdata[1],srcdata[2]);
								}
								else
								{
									if (g_allargs.verbose)
										printf("preferred long register with optimal L value [$%04x%04x]\n", srcdata[0],srcdata[1]);
								}
							}
							else
							{
								// don't care
							}
						}

						if ((sumA < sumB) || (chosen == 0))
						{
							DataSource ds12 = context.sim_resolve_data32(srcdata[0], srcdata[1], reg_index, reg_index2, true);
							if (ds12 == DataSource_RegisterHL)
							{
								context.operations.push_back(new EM_MoveR32(context, 0, reg_index, context.EM1_reg));
								context.operations.back()->execute();
							}
							else
							if (ds12 == DataSource_RegisterLL)
							{
								context.operations.push_back(new EM_MoveR16(context, 0, reg_index, context.EM1_reg));
								context.operations.back()->execute();
								context.operations.push_back(new EM_MoveR16(context, 1, reg_index2, context.EM2_reg));
								context.operations.back()->execute();
							}
							else
							if (ds12 == DataSource_MixedIR)
							{
								context.operations.push_back(new EM_MoveI16(context, 0, srcdata[0], context.EM1_reg));
								context.operations.back()->execute();
								context.operations.push_back(new EM_MoveR16(context, 1, reg_index2, context.EM2_reg));
								context.operations.back()->execute();
							}
							else
							{
								context.operations.push_back(new EM_MoveI32(context, 0, srcdata[0], srcdata[1], context.EM1_reg));
								context.operations.back()->execute();
							}

							DataSource ds3 = context.sim_resolve_data16(srcdata[2], reg_index3, true);
							if (ds3 == DataSource_RegisterL)
							{
								context.operations.push_back(new EM_MoveR16(context, 2, reg_index3, context.EM3_reg));
								context.operations.back()->execute();
							}
							else
							{
								context.operations.push_back(new EM_MoveI16(context, 2, srcdata[2], context.EM3_reg));
								context.operations.back()->execute();
							}
						}
						else
						if ((sumA > sumB) || (chosen == 1))
						{
							DataSource ds23 = context.sim_resolve_data32(srcdata[1], srcdata[2], reg_index, reg_index2, true);
							if (ds23 == DataSource_RegisterHL)
							{
								context.operations.push_back(new EM_MoveR32(context, 1, reg_index, context.EM2_reg));
								context.operations.back()->execute();
							}
							else
							if (ds23 == DataSource_RegisterLL)
							{
								context.operations.push_back(new EM_MoveR16(context, 1, reg_index, context.EM2_reg));
								context.operations.back()->execute();
								context.operations.push_back(new EM_MoveR16(context, 2, reg_index2, context.EM3_reg));
								context.operations.back()->execute();
							}
							else
							if (ds23 == DataSource_MixedIR)
							{
								context.operations.push_back(new EM_MoveI16(context, 1, srcdata[1], context.EM2_reg));
								context.operations.back()->execute();
								context.operations.push_back(new EM_MoveR16(context, 2, reg_index2, context.EM3_reg));
								context.operations.back()->execute();
							}
							else
								context.operations.push_back(new EM_MoveI32(context, 1, srcdata[1], srcdata[2], context.EM2_reg));

							DataSource ds1 = context.sim_resolve_data16(srcdata[0], reg_index3, true);
							if (ds1 == DataSource_RegisterL)
							{
								context.operations.push_back(new EM_MoveR16(context, 0, reg_index3, context.EM1_reg));
								context.operations.back()->execute();
							}
							else
							{
								context.operations.push_back(new EM_MoveI16(context, 0, srcdata[0], context.EM1_reg));
								context.operations.back()->execute();
							}
						}

						//context.sim_track_datarefs(srcdata[0]);
						//context.sim_track_datarefs(srcdata[1]);
						//context.sim_track_datarefs(srcdata[2]);

						em_dirty[0] = em_dirty[1] = em_dirty[2] = false;
					}
					else
					if ((linechanges[0] || em_dirty[0]) && 
						(linechanges[1] || em_dirty[1]))
					{
//						context.operations.push_back(new EM_MoveI32(context, 0, srcdata[0], srcdata[1]));
						DataSource ds = context.sim_resolve_data32(srcdata[0], srcdata[1], reg_index, reg_index2, true);
						if (ds == DataSource_RegisterHL)
						{
							context.operations.push_back(new EM_MoveR32(context, 0, reg_index, context.EM1_reg));
							context.operations.back()->execute();
						}
						else
						if (ds == DataSource_RegisterLL)
						{
							context.operations.push_back(new EM_MoveR16(context, 0, reg_index, context.EM1_reg));
							context.operations.back()->execute();
							context.operations.push_back(new EM_MoveR16(context, 1, reg_index2, context.EM2_reg));
							context.operations.back()->execute();
						}
						else
						{
							context.operations.push_back(new EM_MoveI32(context, 0, srcdata[0], srcdata[1], context.EM1_reg));
							context.operations.back()->execute();
						}

						//context.sim_track_datarefs(srcdata[0]);
						//context.sim_track_datarefs(srcdata[1]);

						em_dirty[0] = em_dirty[1] = false;

//						if (linechanges[2] || em_dirty[2])
//						{
////							context.operations.push_back(new EM_MoveI16(context, 2, srcdata[2]));
//							if (context.resolve_data16(srcdata[2], reg_index, true) == DataSource_RegisterL)
//								context.operations.push_back(new EM_MoveR16(context, 2, reg_index));
//							else
//								context.operations.push_back(new EM_MoveI16(context, 2, srcdata[2]));
//
//							context.track_datarefs(srcdata[2]);
//
//							em_dirty[2] = false;
//						}
					}
					else
					if ((linechanges[1] || em_dirty[1]) && 
						(linechanges[2] || em_dirty[2]))
					{
						// longword to EM2
//						context.operations.push_back(new EM_MoveI32(context, 1, srcdata[1], srcdata[2]));
//						context.operations.back()->execute();

						//context.sim_track_datarefs(srcdata[1]);
						//context.sim_track_datarefs(srcdata[2]);

//						em_dirty[1] = em_dirty[2] = false;

						DataSource ds = context.sim_resolve_data32(srcdata[1], srcdata[2], reg_index, reg_index2, true);
						if (ds == DataSource_RegisterHL)
						{
							context.operations.push_back(new EM_MoveR32(context, 1, reg_index, context.EM2_reg));
							context.operations.back()->execute();
						}
						else
						if (ds == DataSource_RegisterLL)
						{
							context.operations.push_back(new EM_MoveR16(context, 1, reg_index, context.EM2_reg));
							context.operations.back()->execute();
							context.operations.push_back(new EM_MoveR16(context, 2, reg_index2, context.EM3_reg));
							context.operations.back()->execute();
						}
						else
						{
							context.operations.push_back(new EM_MoveI32(context, 1, srcdata[1], srcdata[2], context.EM2_reg));
							context.operations.back()->execute();
						}

						//context.sim_track_datarefs(srcdata[0]);
						//context.sim_track_datarefs(srcdata[1]);

						em_dirty[1] = em_dirty[2] = false;
					}
					else
					{
						// words only

						// this line involves at least one write to EM1/EM2/EM3
						for (int x = 0; x < 3; x++)
						{
							int EM_reg = context.EM1_reg;
							if (x == 1)
								EM_reg = context.EM2_reg;
							else
							if (x == 2)
								EM_reg = context.EM3_reg;

							if (linechanges[x] || em_dirty[x])
							{
								if (context.sim_resolve_data16(srcdata[x], reg_index, true) == DataSource_RegisterL)
								{
									context.operations.push_back(new EM_MoveR16(context, x, reg_index, EM_reg));
									context.operations.back()->execute();
								}
								else
								{
									context.operations.push_back(new EM_MoveI16(context, x, srcdata[x], EM_reg));
									context.operations.back()->execute();
								}

								//context.sim_track_datarefs(srcdata[x]);

								em_dirty[x] = false;
							}
						}
					}

					// update blitter dest/ycount
					context.operations.push_back(new BLiT_SetYC(context, context.PLANES_reg));
					context.operations.back()->execute();
					context.operations.push_back(new BLiT_SetDSTADDR(context, context.SCRLINE_reg));
					context.operations.back()->execute();

					// execute the blit for this line
					context.operations.push_back(new BLiT_Go(context, context.BHOG_reg));
					context.operations.back()->execute();

					// next linejump+blit will be relative to this one (and not an empty one)
					current_line = dsty;
				}
			}
		}

		//int job = 0;
		//for (std::vector<dataregister>::iterator rit = regs.begin(); rit != regs.end(); rit++)
		//{
		//	dataregister &dr = *rit;
		//	if (job != dr.job)
		//	{
		//		// previous job complete - emit blit operations
		//		job = dr.job;

		//		operations.push_back(new BLiT_Go(context));
		//	}
		//}
		// emit final blit operations
		//operations.push_back(new BLiT_Go(context));
		// emit terminal
		context.operations.push_back(new Terminate(context));
		context.operations.back()->execute();

		// peephole passes

		// 1) replace 16bit immediate loads to registers with register-derived values
		// clr.w,not.w,neg.w,and/or/eor.w,add/sub.w,addq/subq.w,scc.b,tas.b

		context.transform_pass();

		// 2) replace 16bit immediate loads to EMs with register-derived values

		context.cleanup_pass();

		context.reduce_pass();

		// 3) replace immediate loads with ops causing side-effects
		// moveq,swap,addq/subq.l,move-usp,ext.l

		context.tail_pass();

		//

		context.reset_sim(false);
		for (std::list<Operation*>::iterator it = context.operations.begin(); it != context.operations.end(); it++)
		{
			(*it)->execute();
			(*it)->emit();
		}

		// debug the blit
//		if (g_allargs.verbose)
		{
			printf("debug simulation...\n");
			int errors = 0;
			for (int v = 0; v < context.srch; v++)
			{
				// detect empty lines
				uword_t m = 0;
				for (int h = 0; h < shift.wordwidth_; h++)
					m |= shift.data_[(v * shift.wordwidth_) + h];

				for (int h = 0; h < shift.wordwidth_; h++)
				{
					uword_t data = context.memory[(v * context.dstww) + h];
					uword_t expected = shift.data_[(v * shift.wordwidth_) + h];

					if (m && (data != expected))
						errors++;

					if (m)
					{
						print_dataword(expected); printf(":"); print_dataword(data); printf("  ");
					}
					else
					{
						print_dataword(expected); printf(":----------------  ");
					}
				}
				printf(" blit=%d\n", best_invpattern[v]);
			}

			if (errors > 0)
			{
				std::cerr << "error: simulation failed on frame " << _frame << " preshift " << pi << std::endl;
				while (1) {};
				exit(1);
			}
		}

		ubyte_t *p68kbin = 0;
		size_t size68kbin = 0;
		
		context.assemble(p68kbin, size68kbin);

		if (p68kbin)
		{
			shift.p68kbin = p68kbin;
			shift.size68kbin = size68kbin;
		}
		else
		{
			std::cerr << "error generating 68k binary code!" << std::endl;
			getchar();
			exit(1);
		}

		delete[] changes; changes = 0;
		delete[] buffer; buffer = 0;
	}

	// generate the generator



}


bool generate_emx_sequence()
{
	// individual sprites follow
	spritelist_t::iterator it = g_spriteframes.begin();
//	for (int x = 0; x < 4; x++, it++);

	int f = 0;
	for (; it != g_spriteframes.end(); it++, f++)
	{
		generate_emx_frame(*it, f);
//		break;
	}

	return true;
}
