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

#define DEBUG_REGISTER_TAGS (0)
#define DEBUG_REGISTER_TAGS2 (0)

#define DEBUG_REGISTER_STATES (0)
#define SIM_ALL_STAGES (0) // 1= don't just simulate final generated code. simulate after every pass, checking for errors (slow!)
#define DEBUG_FRAME (-1)
#define DEBUG_PRESHIFT (-1)

// 20 tiles / 320 pix
// +1 for hscroll margin
// +2 for speculative update margin

#define DISPLAY_LINEWIDTH (8*(20+1))	// +1 for finescroll

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
		  xcount_adj(0),
		  dyinc(0),
		  syinc(0),
		  skew(0)
	{
		em[0] = 0;
		em[1] = 0;
		em[2] = 0;
	}

	uword_t em[3];

	uword_t xcount;
	word_t xcount_adj;
	uword_t dyinc;
	uword_t syinc;
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
	(20<<8),
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
	OpKind_StoreMPI16,
	OpKind_StoreMPI32,
	//
	OpKind_DSTAddaR16,
	OpKind_DSTSubaR16,
	OpKind_DSTLea,
	OpKind_DSTAddaIL,
	//
	OpKind_SetSkew,
	//
	OpKind_SYSPageRestore,
	//
	OpKind_BLITInit,
	OpKind_BLITXCMod,
	OpKind_BLITSYIMod,
	OpKind_BLITDYIMod,
	OpKind_BLITSetup,
	OpKind_BLITGo,
	OpKind_SRCSkip,
	//
	OpKind_Terminate
};

static const char* const as_str[] =
{
	"OpKind_Other",
	//
	// generic ops
	//
	"OpKind_Nop",
	//
	"OpKind_LoadI16",
	"OpKind_LoadI32",
	//
	"OpKind_Lea16",
	"OpKind_Lea32",
	//
	// blitter ops
	//
	"OpKind_StoreI16",
	"OpKind_StoreI32",
	"OpKind_StoreR16",
	"OpKind_StoreR32",
	"OpKind_StoreMPI16",
	"OpKind_StoreMPI32",
	//
	"OpKind_DSTAddaR16",
	"OpKind_DSTSubaR16",
	"OpKind_DSTLea",
	"OpKind_DSTAddaIL",
	//
	"OpKind_SetSkew",
	//
	"OpKind_SYSPageRestore",
	//
	"OpKind_BLITInit",
	"OpKind_BLITXCMod",
	"OpKind_BLITSYIMod",
	"OpKind_BLITDYIMod",
	"OpKind_BLITSetup",
	"OpKind_BLITGo",
	"OpKind_SRCSkip",
	//
	"OpKind_Terminate"
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

		int metrics_words_allocated = 0;

		// sort datawords by incidence

		std::map<int, std::list<metricmap::iterator>> sort_metrics;
		std::map<int, metricmap::iterator> ranked_metrics;
		for (auto it = metrics.begin(); it != metrics.end(); it++)
		{
			sort_metrics[it->second.incidence].push_back(it);
		}

		int ip = 0;
		for (auto & in : sort_metrics)
		{
			for (auto & eit : in.second)
			{
				ranked_metrics[--ip] = eit;
			}
		}

/*		for (auto & rm : ranked_metrics)
		{
			std::cout << "metrics for word:0x" << std::hex << rm.second->first << " incidence:" << std::dec << rm.second->second.incidence << std::endl;
		}
*/
		//

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
			// a0: BLiT_YC
			// a1: scrline
			// a2: BLiT_EM1
			// a3: BLiT_DST
			// a4: BLiT_EM3 (?)
			// a5: BLiT_CTRL
			// a6: BLiT_EM2 (?)

			if ((srcww >= 3) && USE_DSTSKIP_REG_FIXED_PRELOAD)
			{
				// force dstlinewid into d0.w to optimize dst skips
				registers.push_back(dataregister("d0", r++, RegClass_Data, setconstants, setconstants, true, 0xC0000000 | dstlinewid));
			}
			else
			{
				// release d0 for general transforms
				registers.push_back(dataregister("d0", r++, RegClass_Data));
			}

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
		
		// em2 not used unless 3 words wide
		EM2_reg = 999;
		// em3 not used unless 2-3 words wide
		EM3_reg = 999;


		if (shared_mask_)
		{
			// not enough registers to allocate a6 for EM2 while also fetching masks from RAM
			// in shared-mask mode, we allocate it as the mask source buffer
			SM_reg = r;
			registers.push_back(dataregister("a6", r++, RegClass_Address));	// em2
			registers[SM_reg].variable = true;
		}
		else
		{

			// em2,em3 addr depend on sprite width - free up registers if possible
			if (srcww >= 3)
			{
				EM2_reg = r;
				registers.push_back(dataregister("a6", r++, RegClass_Address, setconstants, setconstants, true, 0xffff8a2a));	// em2
			}
			else
				if (1)
				{
					long_t data = dstlinewid;

					if (USE_DSTSKIP_REG_FIXED_PRELOAD_INCIDENCE)
					{
						int i = 0;
						for (auto rmit = ranked_metrics.begin(); rmit != ranked_metrics.end(); rmit++, i++)
						{
							if (metrics_words_allocated == i)
							{
								if (rmit->second->second.incidence > 2)
								{
									//std::cout << "preload high incidence word " << i << ":0x" << std::hex << rmit->second->first << " incidence:" << std::dec << rmit->second->second.incidence << std::endl;
									data = (long_t)(word_t)rmit->second->first;
									metrics_words_allocated++;
								}
								break;
							}
						}
						/*
										for (auto it = metrics.begin(); it != metrics.end(); it++, i++)
										{
											if (metrics_words_allocated == i)
											{
												if (it->second.incidence > 2)
												{
													std::cout << "preload high incidence word " << i << ":0x" << std::hex << it->first << " incidence:" << std::dec << it->second.incidence << std::endl;
													data = (long_t)(word_t)(it->first);
													metrics_words_allocated++;
												}
												break;
											}
										}
						*/
					}

					registers.push_back(dataregister("a6", r++, RegClass_Address, setconstants, setconstants, true, (ulong_t)data));
				}
				else
				{
					// leave a6 free
					registers.push_back(dataregister("a6", r++, RegClass_Address));
				}
		}

		if (srcww > 1)
		{
			EM3_reg = r;
			registers.push_back(dataregister("a4", r++, RegClass_Address, setconstants, setconstants, true,		0x00ff8a2c));	// em3
		}
		else
		{
			// leave a4 free?
			long_t data = 0;
			bool alloc = false;

			if (USE_DSTSKIP_REG_FIXED_PRELOAD_INCIDENCE)
			{
				int i = 0;
//				for (auto it = metrics.begin(); it != metrics.end(); it++, i++)
//				for (auto & rm : ranked_metrics)
				for (auto rmit = ranked_metrics.begin(); rmit != ranked_metrics.end(); rmit++, i++)
				{
					if (metrics_words_allocated == i)
					{
						if (rmit->second->second.incidence > 2)
						{
							//std::cout << "preload high incidence word " << i << ":0x" << std::hex << rmit->second->first << " incidence:" << std::dec << rmit->second->second.incidence << std::endl;
							data = (long_t)(word_t)rmit->second->first;
							metrics_words_allocated++;

							alloc = true;
						}
						break;
					}
				}
			}

			if (alloc)
				registers.push_back(dataregister("a4", r++, RegClass_Address, setconstants, setconstants, true, data));	// em3
			else
				registers.push_back(dataregister("a4", r++, RegClass_Address));
		}


		BHOG_reg = r;
//		registers.push_back(dataregister("d1", r++, RegClass_Data,		setconstants, setconstants, true,		0x00ffffc0));
		registers.push_back(dataregister("d1", r++, RegClass_Data,		setconstants, setconstants, true,		0x00ffc000));

		PLANES_reg = r;
		registers.push_back(dataregister("d2", r++, RegClass_Data,		setconstants, setconstants, true,		0xFF000004));

//		registers.push_back(dataregister("sr", r++, true, true, 0x00002300));
	}

	Context(int _srcww, int _srch, int _srccolww, int _dstww, int _guardxs, bool _shared_mask)
		: srcww(_srcww), 
		  srch(_srch),
		  srccolww(_srccolww),
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
		  BHOG_reg(0),
		  guardwords((_guardxs + (16-1)) >> 4),
		  shared_mask_(_shared_mask)
	{
		if (_dstww < 3)
			dstww = 3;
		else
			dstww = _dstww;

		printf("context allocated %d x %d words\n", srch, dstww);

		memory = new uword_t[dstww * srch];
		memset(memory, 0xAA, dstww * srch * sizeof(uword_t));

//		memset(preloads, 0, sizeof(preloads));

//		blitter.xcount = srcww;

		dstlinewid = DISPLAY_LINEWIDTH + (8 * guardwords);

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

		BLiTSYI_name = "BLiTSYI.w";
		BLiTDYI_name = "BLiTDYI.w";
		BLiTXC_name = "BLiTXC.w";
		BLiTSKEW_name = "BLiTSKEW.w";
		BLiTSRC_name = "BLiTSRC.w";
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
		blitter.xcount_adj = 0;
		reset_registers();
		metrics_sim = metrics;
		shared_mask_word_pos = 0;
	}

	int cycle_sum(int &r_program_size);

	void dump_tags();
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

	bool try_transforms(std::list<Operation*>::iterator &curr, OpSize _opsize, int _rold, uword_t _dataH, uword_t _dataL);

	bool try_4c_init_transforms(std::list<Operation*>::iterator &curr, OpSize _opsize, int _rold, uword_t _dataH, uword_t _dataL);
	bool try_8c_init_transforms(std::list<Operation*>::iterator &curr, OpSize _opsize, int _rold, uword_t _dataH, uword_t _dataL);
	bool try_12c_init_transforms(std::list<Operation*>::iterator &curr, OpSize _opsize, int _rold, uword_t _dataH, uword_t _dataL);

	bool try_reg_transforms(std::list<Operation*>::iterator &curr, std::vector<dataregister>::iterator it, OpSize _opsize, int _rold, uword_t _dataH, uword_t _dataL, int &r_limit, bool _2nd = false);
//	bool try_8c_reg_transforms(std::list<Operation*>::iterator &curr, std::vector<dataregister>::iterator it, OpSize _opsize, int _rold, uword_t _dataH, uword_t _dataL, bool _2nd = false);

	bool safe_rename_H(std::list<Operation*>::iterator curr, std::vector<dataregister>::iterator it_treg, int _rold);
	bool safe_rename_H(std::list<Operation*>::iterator curr, uword_t dataH, int _rold);

	bool transform_pass();
	bool cleanup_pass();
	bool reduce_pass();
	bool promote_pass();
	bool promote_lea_pass();
	void tail_pass();

	void register_sweep();

	bool tag_collision(std::list<Operation*>::iterator _start, int _rold, int _rnew, OpSize _sz);
	bool tags_busy(std::list<Operation*>::iterator _start, int _r);
	int register_count_srcrefs(std::list<Operation*>::iterator _start, int _reg, OpSize _sz);
	std::list<Operation*>::iterator register_last_written(std::list<Operation*>::iterator _start, int _reg, OpSize _sz);

	void assemble(ubyte_t *&r_p68kbin, size_t &r_size68kbin, std::string &_tmpfile)
	{
//		ubyte_t *p68kbin = 0;
		std::string tmp_lst = _tmpfile + ".lst";
		std::string tmp_bin = _tmpfile + ".bin";
		std::string tmp_src = _tmpfile + ".s";
	
		//__debugbreak();

		const std::string& str = genout.str();
		std::ofstream cfile;
		cfile.open(tmp_src);  
		cfile << str;  
		cfile.close();
		
		// execute assembler, turning our temporary .s into binary 68k
		//		ShellExecute(NULL, "open", "rmac.exe", "-n -o tmp68k.bin tmp68k.s", NULL, SW_HIDE);
		{
			// aout format
			std::string tmpargs = "-fa +oall ";

			// emit .lst files only in verbose mode
			if (g_allargs.verbose)
			{
				tmpargs += "-l";
				tmpargs += tmp_lst;
				tmpargs += " ";
			}

			// output binary 68k
			tmpargs += "-o ";
			tmpargs += tmp_bin;

			// input temporary src file
			tmpargs += " ";
			tmpargs += tmp_src;

			std::string rmacpath = c_rmac_filename;
			if (g_allargs.rmacpath.length() > 1)
				rmacpath = g_allargs.rmacpath;

#if defined(_MSC_VER)

			// todo: replace this with exec() below
			
			SHELLEXECUTEINFO ShExecInfo = { 0 };
			ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
			ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
			ShExecInfo.hwnd = NULL;
			ShExecInfo.lpVerb = NULL;
			ShExecInfo.lpFile = rmacpath.c_str();
			ShExecInfo.lpParameters = tmpargs.c_str(); // "-fa -ltmp68k.lst -o tmp68k.bin tmp68k.s";
			ShExecInfo.lpDirectory = NULL;
			ShExecInfo.nShow = SW_HIDE;
			ShExecInfo.hInstApp = NULL;
			ShellExecuteEx(&ShExecInfo);
			WaitForSingleObject(ShExecInfo.hProcess, INFINITE);

			DWORD exitcode = 0;
			GetExitCodeProcess(ShExecInfo.hProcess, &exitcode);

			if (exitcode != 0)
			{
				std::cerr << "error assembling intermediate code using " << rmacpath << std::endl << std::flush;
				getchar();

				exit(-1);
			}

			//DeleteFileA(tmp_src.c_str()/*"tmp68k.s"*/);
#else
			std::string command = rmacpath + " " + tmpargs;
			
			std::string result_str;
			bool success = exec(command, result_str);
			if (!success)
			{
				std::cerr << "error assembling intermediate code using " << command << std::endl << std::flush;
				getchar();

				exit(-1);
			}
#endif
			
		}


        
		// read code back in
		{
			FILE *h = fopen(tmp_bin.c_str()/*"tmp68k.bin"*/, "rb");
			if (h)
			{
				fseek(h, 0, SEEK_END);
				r_size68kbin = ftell(h);
				fseek(h, 0, SEEK_SET);

				if (r_size68kbin <= 32)
				{
					fclose(h);

					std::cerr << "error sizing binary code!" << std::endl << std::flush;
					getchar();
					exit(-1);
				}

				r_p68kbin = new ubyte_t[r_size68kbin];
				if (r_size68kbin != fread(r_p68kbin, 1, r_size68kbin, h))
				{
					fclose(h);

					std::cerr << "error reading binary code!" << std::endl << std::flush;
					getchar();
					exit(-1);
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

				size_t oldsize = r_size68kbin;
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

				if (g_allargs.verbose)
					if (oldsize != r_size68kbin)
						std::cout << "trimmed binary code by " << (oldsize-r_size68kbin) << " bytes" << std::endl;


				if (r_size68kbin == 0)
				{
					fclose(h);

					std::cerr << "error validating binary code end!" << std::endl << std::flush;
					getchar();
					exit(-1);
				}

				fclose(h);

#if defined(_MSC_VER)
				DeleteFileA(tmp_bin.c_str()/*"tmp68k.bin"*/);
#endif
            }
			else
			{
				std::cerr << "error opening binary code!" << std::endl << std::flush;
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
	int srccolww;
	int srch;
	int dstlinewid;
	int guardwords;

	metricmap metrics;
	metricmap metrics_sim;

	std::vector<u16> shared_mask_words;
	int shared_mask_word_pos;

	std::vector<dataregister> registers;

	std::list<Operation*> operations;

	std::string BLiTSRC_name;
	std::string BLiTDST_name;
	std::string BLiTCTRL_name;
//	std::string BLiTEM1_name;
//	std::string BLiTEM2_name;
//	std::string BLiTEM3_name;
	std::string BLiTYC_name;
	std::string BLiTSYI_name;
	std::string BLiTDYI_name;
	std::string BLiTXC_name;
	std::string BLiTSKEW_name;

	std::vector<std::string> BLiTEM_names;

	bool shared_mask_;

	int SCRLINE_reg;
	int PLANES_reg;
	int BHOG_reg;
	int DSTADDR_reg;
	int YC_reg;
	int CTRL_reg;
	int EM1_reg;
	int EM2_reg;
	int EM3_reg;
	int SM_reg;
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
					const emx_metric_t &m = metrics_sim[it->L];
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
//				r.L = _data;
				// allows future choices to acknowledge the reuse incidence looks different from now on
//				metrics[_data].incidence = metrics[_data].references;

				operations.push_back(new DR_LoadI16(*this, best_reg, _data));

				r.L = _data;

				operations.back()->execute();
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

void Context::dump_tags()
{
	//register_sweep();

	for (std::list<Operation*>::iterator it = operations.begin(); it != operations.end(); it++)
	{
		Operation *poper = *it;

		printf("tags:");
		for (int r = 0; r < registers.size() && r < 6; r++)
		{
			printf(" %s[%02x:%02x](%08x)", 
				registers[r].name.c_str(), 
				ubyte_t(poper->alltagsH[r]), ubyte_t(poper->alltagsL[r]),
				(((ulong_t)registers[r].H << 16) | registers[r].L)
			);
		}
		printf("  %s", poper->describe().c_str());
	}
}

void Context::register_rename(std::list<Operation*>::iterator _start, int _regold, int _regnew)
{
	if (_regold == _regnew)
		return;

	Operation* poper = *_start;

	// caution: low and high words of same register can have different tags
	// also, low and high words of same register may independently live or dead (-1)

	int tagoldH = -1;
	int tagoldL = -1;
	if (_regold == poper->dreg) // give dreg priority over sreg for sake of single-register outputs (dreg = 'dreg)
	{
		tagoldH = poper->dtagH;
		tagoldL = poper->dtagL;
	}
	else
	if (_regold == poper->sreg)
	{
		tagoldH = poper->stagH;
		tagoldL = poper->stagL;
	}
	else
	if (_regold == poper->areg)
	{
		tagoldH = poper->atag;
		tagoldL = poper->atag;
	}

	// can only rename if some part is live
	if ((tagoldL < 0) && (tagoldH < 0))
		return;

	//dump_tags();

	for (std::list<Operation*>::iterator it = _start/*operations.begin()*/; it != operations.end(); it++)
	{
		Operation *poper = *it; 

		if ((poper->opdst == OpSource_Register) &&
			((poper->dtagL >= 0 ) && ((poper->dtagL == tagoldL) || (poper->dtagL == tagoldH))) ||
			((poper->dtagH >= 0 ) && (/*(poper->dtagH == tagoldL) ||*/ (poper->dtagH == tagoldH)))
		)
		{
			if (g_allargs.verbose)
				printf("replacing D:%s with %s\n", registers[poper->dreg].name.c_str(), registers[_regnew].name.c_str());

			if ((poper->opsrc == OpSource_None) && (poper->sreg == poper->dreg))
			{
				poper->sreg = _regnew;
			}

			poper->dreg = _regnew;
		}

		if ((poper->opsrc == OpSource_Register) &&
			((poper->stagL >= 0 ) && ((poper->stagL == tagoldL) || (poper->stagL == tagoldH))) ||
			((poper->stagH >= 0 ) && (/*(poper->stagH == tagoldL) ||*/ (poper->stagH == tagoldH)))
		)
		{
			if (g_allargs.verbose)
				printf("replacing S:%s with %s\n", registers[poper->sreg].name.c_str(), registers[_regnew].name.c_str());
			poper->sreg = _regnew;
		}


		if ((poper->atag >= 0) && (poper->atag == tagoldL))
		{
			if (g_allargs.verbose)
				printf("replacing A:%s with %s\n", registers[poper->areg].name.c_str(), registers[_regnew].name.c_str());
			poper->areg = _regnew;
		}
	}

	//register_sweep();

	//dump_tags();
}

void Context::register_sweep()
{
	static std::vector<std::pair<int,int> > s_tags;
	s_tags.clear();
	s_tags.resize(registers.size(), std::pair<int,int>(-1,-1));
	int newtag = 0;

#if (DEBUG_REGISTER_TAGS)
	printf("\nregister sweep...\n");
#endif

	for (std::list<Operation*>::reverse_iterator rit = operations.rbegin(); rit != operations.rend(); rit++)
	{
		Operation *poper = *rit;

		int replacedH = -1;
		int replacedL = -1;

		// data source
		if (poper->sreg >= 0)
		{
			assert(poper->sreg < s_tags.size());

			// if not yet tagged - generate new tag
			std::pair<int,int> &optag = s_tags[poper->sreg];

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
			}

			if (poper->opsize == OpSize_32)
			{
				if (optag.second < 0)
					optag.second = optag.first;
			}

			poper->stagH = optag.second;
			poper->stagL = optag.first;

#if (DEBUG_REGISTER_TAGS)
			if (poper->opsize == OpSize_32)
			printf("referenced: %s (%02d:%02d) [32] : %s\n", 
				registers[poper->sreg].name.c_str(), 
				poper->stagH, 
				poper->stagL,
				poper->describe().c_str()
				);
			else
			printf("referenced: %s (%02d:%02d) [16] : %s\n", 
				registers[poper->sreg].name.c_str(), 
				poper->stagH, 
				poper->stagL,
				poper->describe().c_str()
				);
#endif
		}

		// address source
		if (poper->areg >= 0)
		{
//			printf("areg: %d\n", poper->areg);
			assert(poper->areg < s_tags.size());

			// if not yet tagged - generate new tag
			std::pair<int,int> &optag = s_tags[poper->areg];

			if (optag.first < 0)
			{
				optag.first = newtag++;
				optag.second = optag.first;
			}

			poper->atag = optag.first;

#if (DEBUG_REGISTER_TAGS)
			printf("referenced: %s (%02d) : %s", 
				registers[poper->areg].name.c_str(), 
				poper->atag,
				poper->describe().c_str()
				);
#endif
		}

		// destination as source
		if ((poper->dreg >= 0) && 
			((poper->opwr == OpWrite_Modify) || 
			 (poper->opwr == OpWrite_Combine) ||
			 (poper->opwr == OpWrite_Replace) || 
			 (poper->opwr == OpWrite_ReplaceH) || 
			 (poper->opwr == OpWrite_ReplaceHCombineL))
		)
		{
			assert(poper->dreg < s_tags.size());

			// if not yet tagged - generate new tag
			std::pair<int,int> &optag = s_tags[poper->dreg];

			if (optag.first < 0)
			{
				optag.first = newtag++;
			}

				if (poper->opsize == OpSize_32)
				{
					if (optag.second < 0)
						optag.second = optag.first;
				}

			poper->dtagH = optag.second;
			poper->dtagL = optag.first;

#if (DEBUG_REGISTER_TAGS)
			if (poper->opwr == OpWrite_Combine)
			printf("combined: %s (%02d:%02d) : %s", 
				registers[poper->dreg].name.c_str(), 
				poper->dtagH, 
				poper->dtagL,
				poper->describe().c_str()
				);
			else
			if (poper->opwr == OpWrite_Replace && poper->opsize == OpSize_32)
			printf("replaced: %s (%02d:%02d) [32] : %s", 
				registers[poper->dreg].name.c_str(), 
				poper->dtagH, 
				poper->dtagL,
				poper->describe().c_str()
				);
			else
			if (poper->opwr == OpWrite_Replace && poper->opsize == OpSize_16)
			printf("replaced: %s (%02d:%02d) [16] : %s", 
				registers[poper->dreg].name.c_str(), 
				poper->dtagH, 
				poper->dtagL,
				poper->describe().c_str()
				);
			else
			if (poper->opwr == OpWrite_ReplaceHCombineL)
			printf("repHcmbL: %s (%02d:%02d) : %s", 
				registers[poper->dreg].name.c_str(), 
				poper->dtagH, 
				poper->dtagL,
				poper->describe().c_str()
				);
			else
			if (poper->opsize == OpSize_32)
			printf("modified: %s (%02d:%02d) [32] : %s", 
				registers[poper->dreg].name.c_str(), 
				poper->dtagH, 
				poper->dtagL,
				poper->describe().c_str()
				);
			else
			printf("modified: %s (%02d:%02d) [16] : %s", 
				registers[poper->dreg].name.c_str(), 
				poper->dtagH, 
				poper->dtagL,
				poper->describe().c_str()
				);
#endif
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
		for (std::vector<std::pair<int,int> >::iterator it = s_tags.begin(); it != s_tags.end(); it++, r++)
		{
			std::pair<int,int> &tag = *it;

			// save full tag pattern for this state in operation to help with renaming
			assert(r < sizeof(poper->alltagsH));
			poper->alltagsH[r] = tag.second;
			poper->alltagsL[r] = tag.first;

#if (DEBUG_REGISTER_TAGS2)
			if (tag.first >= 0)
				printf("%02d:%02d ", tag.second, tag.first);
			else
				printf("--:-- ");
#endif
		}

		// reset tags for replaced register BEFORE replace op (running backwards)
		if (replacedL >= 0)
		{
			assert(replacedL < s_tags.size());
			s_tags[replacedL].first = -1;
		}
		if (replacedH >= 0)
		{
			assert(replacedH < s_tags.size());
			s_tags[replacedH].second = -1;
		}

#if (DEBUG_REGISTER_TAGS)
		printf("\n");
#endif
	}
}

bool Context::tags_busy(std::list<Operation*>::iterator _exepos, int _r)
{
	// detect existing dependency on either part of register _r
	Operation *poper = *_exepos;

	assert(_r < sizeof(poper->alltagsL));

	if (poper->alltagsL[_r] >= 0)
		return true;

	if (poper->alltagsH[_r] >= 0)
		return true;

	return false;
}

bool Context::tag_collision(std::list<Operation*>::iterator _start, int _rold, int _rnew, OpSize _sz)
{
	// reg can't collide with itself
	if (_rnew == _rold)
		return false;

	assert(_rold >= 0);
	assert(_rnew >= 0);

	// detecttags lifespan tag conflicts between two registers
	for (std::list<Operation*>::iterator it = _start; it != operations.end(); it++)
	{
		Operation *poper = *it;
		assert(_rnew < sizeof(poper->alltagsL));
		assert(_rold < sizeof(poper->alltagsL));

		// check if upper word of target register is busy.
		if (poper->alltagsH[_rnew] >= 0)
		{
			// note: if busy, it may still be renamed but *only* if upper words already match!
			return true; // collision
		}

		// if lower word of target is busy, can't rename
		if (poper->alltagsL[_rnew] >= 0)
			return true; // collision

/*
		assert(_rold < sizeof(poper->alltagsL));
		if (poper->alltagsL[_rold] >= 0)
		{
			assert(_rnew < sizeof(poper->alltagsL));
			if (poper->alltagsL[_rnew] >= 0)
				return true;
		}

		if (_sz == OpSize_32)
		{
			assert(_rold < sizeof(poper->alltagsH));
			if (poper->alltagsH[_rold] >= 0)
			{
				assert(_rnew < sizeof(poper->alltagsH));
				if (poper->alltagsH[_rnew] >= 0)
					return true;
			}
		}
*/
	}

	return false;
}

#define USE_XFORM_ADDSUBQ (1)

#include "68kxforms.inl"



bool Context::try_transforms(std::list<Operation*>::iterator &curr, OpSize _opsize, int _rold, uword_t _dataH, uword_t _dataL)
{
	// can't transform variable registers
	if (registers[_rold].variable)
		return false;

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
//				if (data16 == uword_t(imm))
//				{
				uword_t imm_hw = uword_t(imm >> 16);
				uword_t imm_lw = uword_t(imm);

				if ((data16 == imm_lw) && 
					// don't perform moveq if it modifies a high word dependency
					(/*!registers[_rold].stableH*/((*curr)->alltagsH[_rold] < 0) || (registers[_rold].H == imm_hw)))
				{

					if (g_allargs.verbose)
						printf("transform: move.w #$%04x,%s -> moveq.l #%d,%s [$%04x:%04x]\n",
							data16,
							registers[_rold].name.c_str(),
							imm,
							registers[_rold].name.c_str(),
							registers[_rold].H,
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

	return false;

terminate:;

	return true;
}

bool Context::transform_pass()
{
	reset_sim(true);

	register_sweep();

	//dump_tags();

	if (!g_allargs.quickcut)
		if (g_allargs.verbose)
			printf("transform pass: replace word #immediate register loads with derived register values\n");

	for (std::list<Operation*>::iterator it = operations.begin(); it != operations.end();)
	{
		std::list<Operation*>::iterator curr = it++;
		Operation *pop = *curr;

		if (pop->kind == OpKind_LoadI16)
		{
			DR_LoadI16 *pactual = reinterpret_cast<DR_LoadI16*>(pop);
			int rold = pactual->dreg;

			if (try_transforms(curr, OpSize_16, rold, 0, pactual->data))
				return true;
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

			if (try_transforms(curr, OpSize_32, rold, pactual->dataH, pactual->dataL))
				return true;
		}

		// catch up with execution from last insert point
		while (curr != it)
		{
			register_sweep();
			(*curr)->execute(); 
			curr++;
		}
	}

	return false;
}


bool Context::cleanup_pass()
{
	reset_sim(true);

	register_sweep();

	bool changed = false;

	if (!g_allargs.quickcut)
		if (g_allargs.verbose)
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
					(word_t(pactual->delta(dstlinewid)) == word_t(it->L)))
				{
					if (g_allargs.verbose)
						printf("transform: lea.w %d(%s),%s -> adda.w %s,%s [$%04x]\n",
							pactual->delta(dstlinewid),
							registers[SCRLINE_reg].name.c_str(), registers[SCRLINE_reg].name.c_str(),
							registers[rnew].name.c_str(),
							registers[SCRLINE_reg].name.c_str(),
							registers[rnew].L
						);
					int wordjump = pactual->worddelta;
					int linejump = pactual->linedelta;
					curr = operations.erase(curr);
					curr = operations.insert(curr, new DST_AddaR16(*this, rnew, SCRLINE_reg, wordjump, linejump));
					changed = true;
					goto terminate;
				}

				if (it->stableL &&
					(word_t(pactual->delta(dstlinewid)) == -word_t(it->L)))
				{
					if (g_allargs.verbose)
						printf("transform: lea.w %d(%s),%s -> suba.w %s,%s [$%04x]\n",
							pactual->delta(dstlinewid),
							registers[SCRLINE_reg].name.c_str(), registers[SCRLINE_reg].name.c_str(),
							registers[rnew].name.c_str(),
							registers[SCRLINE_reg].name.c_str(),
							registers[rnew].L
							);
					int wordjump = pactual->worddelta;
					int linejump = pactual->linedelta;
					curr = operations.erase(curr);
					curr = operations.insert(curr, new DST_SubaR16(*this, rnew, SCRLINE_reg, wordjump, linejump));
					changed = true;
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
						changed = true;
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
						assert(arold < 256);
						curr = operations.insert(curr, new EM_MoveR32(*this, emold, rnew, arold));
						changed = true;
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
					changed = true;
					goto terminate;
				}
				else
				if ((registers[pactual->dreg].H == 0xFFFF) && (registers[pactual->dreg].L & 0x8000))
				{
					// short -ve
					curr = operations.erase(curr);
					curr = operations.insert(curr, new AR_Lea16(*this, pactual->dreg, registers[pactual->dreg].L));
					changed = true;
					goto terminate;
				}
				else
				{
					curr = operations.erase(curr);
					curr = operations.insert(curr, new AR_Lea32(*this, pactual->dreg, registers[pactual->dreg].H, registers[pactual->dreg].L));
					changed = true;
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

	return changed;
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
			bool matchH = (tagH >= 0) && ((pop->alltagsH[pop->dreg] == tagH) || (pop->alltagsL[pop->dreg] == tagH));
			bool matchL = (tagL >= 0) && ((pop->alltagsH[pop->dreg] == tagL) || (pop->alltagsL[pop->dreg] == tagL));

			// only care about tagL
			if (_sz == OpSize_32)
			{
				if (matchH || matchL)
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

/*			bool matchH = (pop->alltagsH[pop->dreg] == tagH) && (tagH >= 0) && (pop->alltagsH[pop->dreg] >= 0);
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
*/
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

bool Context::reduce_pass()
{
	reset_sim(true);

	register_sweep();

	bool changed = false;

	if (!g_allargs.quickcut)
		if (g_allargs.verbose)
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
					if (!g_allargs.quickcut)
						if (g_allargs.verbose)
							printf("reduce opportunity: reg %d [$%04x%04x]\n", reg, registers[reg].H, registers[reg].L);

					int arold = pwrite->areg;

					// remove load
					operations.erase(itwrite);

					// replace store
					if (pop->kind == OpKind_StoreR16)
					{
						curr = operations.erase(curr);
						curr = operations.insert(curr, new EM_MoveI16(*this, em, registers[reg].L, arold));
						changed = true;
					}
					else
					{
						curr = operations.erase(curr);
						curr = operations.insert(curr, new EM_MoveI32(*this, em, registers[reg].H, registers[reg].L, arold));
						changed = true;
					}
				}
			}
		}

//terminate:

		// catch up with execution from last insert point
		while (curr != it)
		{
			register_sweep();
			(*curr)->execute(); 
			curr++;
		}
	}

	return changed;
}

bool Context::promote_pass()
{
	reset_sim(true);

	register_sweep();

	bool changed = false;

	if (!g_allargs.quickcut)
		if (g_allargs.verbose)
			printf("promotion pass: replace immediate stores with load/store via free register\n");

	for (std::list<Operation*>::iterator it = operations.begin(); it != operations.end() && !changed;)
	{
		std::list<Operation*>::iterator curr = it++;
		Operation *pop = *curr;

		uword_t iH = -1;
		uword_t iL = -1;
		int em = -1;
		int arold = -1;

		if (pop->kind == OpKind_StoreI16)
		{
			EM_MoveI16 *pactual = reinterpret_cast<EM_MoveI16*>(pop);
			iL = pactual->data;
			em = pactual->em;
			arold = pactual->areg;
		}

		if (pop->kind == OpKind_StoreI32)
		{
			EM_MoveI32 *pactual = reinterpret_cast<EM_MoveI32*>(pop);
			iH = pactual->dataH;
			iL = pactual->dataL;
			em = pactual->em;
			arold = pactual->areg;
		}

		if (em >= 0)
		{
			// look for register with no burdens
			int freereg = -1;
			for (int r = 0; r < registers.size(); r++)
			{
				if (!(registers[r].locked) && !(registers[r].variable))
				{
					if (
						(pop->alltagsL[r] < 0) &&
						// incidence of replacement must be higher than incidence of existing
						//(metrics_sim[iL].references > metrics_sim[registers[r].L].references)
						1//(metrics_sim[iL].incidence > metrics_sim[registers[r].L].incidence)
					)
					{
						if (pop->kind == OpKind_StoreI16)
						{
							freereg = r;
							break;
						}
						else
						{
							// don't promote long stores unless upper data is trivial
							if ((pop->alltagsH[r] < 0) && ((iH == 0) || (iH == 0xFFFF)))
							{
								freereg = r;
								break;
							}
						}
					}
				}
			}

			if (freereg >= 0)
			{
				{
					//				printf("possible reduce opportunity: reg %d [$%04x%04x]\n", reg, registers[reg].H, registers[reg].L);

					{
						// replace store
						if (pop->kind == OpKind_StoreI16)
						{
							if (!g_allargs.quickcut)
								if (g_allargs.verbose)
									printf("promote opportunity: reg %d [$%04x]\n", freereg, iL);

							curr = operations.erase(curr);
							curr = operations.insert(curr, new EM_MoveR16(*this, em, freereg, arold));
							curr = operations.insert(curr, new DR_LoadI16(*this, freereg, iL));
							changed = true;
						}
						else
						{
							if (!g_allargs.quickcut)
								if (g_allargs.verbose)
									printf("promote opportunity: reg %d [$%04x%04x]\n", freereg, iH, iL);

							curr = operations.erase(curr);
							curr = operations.insert(curr, new EM_MoveR32(*this, em, freereg, arold));
							curr = operations.insert(curr, new DR_LoadI32(*this, freereg, iH, iL));
							changed = true;
						}
					}
				}
			}
		}


//terminate:

		// catch up with execution from last insert point
		while (curr != it)
		{
			register_sweep();
			(*curr)->execute(); 
			curr++;
		}
	}

	return changed;
}

bool Context::promote_lea_pass()
{
	reset_sim(true);

	register_sweep();

	bool changed = false;

	if (!g_allargs.quickcut)
		if (g_allargs.verbose)
			printf("promotion pass: replace lea dst skips with add/sub via free register\n");

	for (std::list<Operation*>::iterator it = operations.begin(); it != operations.end() && !changed;)
	{
		std::list<Operation*>::iterator curr = it++;
		Operation *pop = *curr;

		if (pop->kind == OpKind_DSTLea)
		{
			DST_Lea *pactual = reinterpret_cast<DST_Lea*>(pop);
			int delta = pactual->delta(dstlinewid);
			int linejump = pactual->linedelta;
			int wordjump = pactual->worddelta;
			int rold = pactual->dreg;

			// look for register with no burdens
			int freereg = -1;
			for (int r = 0; r < registers.size(); r++)
			{
				if (!(registers[r].locked) && !(registers[r].variable))
				{
					if (pop->alltagsL[r] < 0)
					{
						freereg = r;
						break;
					}
				}
			}

			if (freereg > 0)
			{
				{
					{
						// replace lea
						{
							if (!g_allargs.quickcut)
								if (g_allargs.verbose)
									printf("lea promote opportunity: reg %d [$%04x] (x:%d,y:%d)\n", freereg, delta, wordjump, linejump);

							curr = operations.erase(curr);
							curr = operations.insert(curr, new DST_AddaR16(*this, freereg, SCRLINE_reg, wordjump, linejump));
							curr = operations.insert(curr, new DR_LoadI16(*this, freereg, delta));
							changed = true;
						}					
					}
				}
			}
		}


//terminate:

		// catch up with execution from last insert point
		while (curr != it)
		{
			register_sweep();
			(*curr)->execute(); 
			curr++;
		}
	}

	return changed;
}

void Context::tail_pass()
{
	reset_sim(true);
	
	register_sweep();

	if (!g_allargs.quickcut)
		if (g_allargs.verbose)
			printf("tail optimization pass: move BLiT operations to hide long tails...\n");

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
			BLiT_GoSkew *pactual = reinterpret_cast<BLiT_GoSkew*>(pop);

			// self
			//std::list<Operation*>::iterator best_tailop = curr;

			// scan from next op onwards
			std::list<Operation*>::iterator itscan = curr; 
			if (itscan != operations.end())
				itscan++;

			// first op can be a barrier, but should not cause a change if chosen
			std::list<Operation*>::iterator first = itscan;
			std::list<Operation*>::iterator best_tailop = first;
			int best_tailval = (*best_tailop)->tail;

			//int best_tailval = (*itscan)->tail;
			for (; itscan != operations.end(); itscan++)
			{
				Operation *ptryop = *itscan;

				OpKind kind = ptryop->kind;

				int trywords = ptryop->words;

				// special ops with nontrivial size/implementation, we don't reorder across these
				// note: covers most of the specific tests below, but we check them anyway
//				if (trywords <= 0)
//					break;

				// don't reorder blit across modification of d1 (BLiT_GoSkew source register)
				if (ptryop->dreg == BHOG_reg)
					break;

				// observe routine end
				if (kind == OpKind_Terminate)
					break;

				// don't reorder past skew reg changes (in d1) since this is our source for BLITGo
				if (kind == OpKind_SetSkew)
					break;

				// can't reorder blit beyond HW register stores
				if (kind == OpKind_StoreI16)
					break;
				if (kind == OpKind_StoreI32)
					break;
				if (kind == OpKind_StoreR16)
					break;
				if (kind == OpKind_StoreR32)
					break;
				if (kind == OpKind_StoreMPI16)
					break;
				if (kind == OpKind_StoreMPI32)
					break;

				if (kind == OpKind_BLITInit)
					break;
				if (kind == OpKind_BLITXCMod)
					break;
				if (kind == OpKind_BLITSYIMod)
					break;
				if (kind == OpKind_BLITDYIMod)
					break;
				if (kind == OpKind_BLITSetup)
					break;
				if (kind == OpKind_BLITGo)
					break;
				if (kind == OpKind_SRCSkip)
					break;
				if (kind == OpKind_SYSPageRestore)
					break;

				//printf("kind: %s\n", as_str[kind]);

				int trytail = ptryop->tail;

				if (
					((trywords == 1) && (trytail > best_tailval)) 
				)
				{
					best_tailval = trytail;
					best_tailop = itscan;
				}
			}

			// if we found a good position and it is not the first position, move the blit
			if ((best_tailop != curr) && 
				(best_tailop != first))
			{
				if (g_allargs.verbose)
				{
					printf("relocating blit to hide tail=%d cycles [op:%s]\n", best_tailval, (*best_tailop)->describe());
				}
				operations.erase(curr);
				operations.insert(best_tailop, pop);
			}
		}
	};

//terminate:;
}

void run_sim
(
	Context &context, 
	ubyte_t *livelines, 
	int _frame, 
	int pi, preshift_t &shift, 
	std::vector<int> &linemap_src_2_opt,
	bool _generate
)
{
	context.reset_sim(false);
	for (std::list<Operation*>::iterator it = context.operations.begin(); it != context.operations.end(); it++)
	{
		Operation *poper = *it;

		poper->execute();
		if (_generate)
			poper->emit();

#if (DEBUG_REGISTER_STATES)
		printf("tags:");
		for (int r = 0; r < context.registers.size() && r < 6; r++)
		{
			printf(" %s[%02x:%02x](%08x)", 
				context.registers[r].name.c_str(), 
				ubyte_t(poper->alltagsH[r]), ubyte_t(poper->alltagsL[r]),
				(((ulong_t)context.registers[r].H << 16) | context.registers[r].L)
			);
		}
		printf("  %s", poper->describe().c_str());
#endif
	}

//	context.dump_tags();

	// debug the blit
	//		if (g_allargs.verbose)
	{
		if (!g_allargs.quickcut)
			if (g_allargs.verbose)
				printf("debug simulation... %dx%d words (%dx%d buffer)\n", 
					context.srcww, context.srch,
					context.dstww, context.srch
				);

		int errors = 0;
		for (int v = 0; v < context.srch; v++)
		{
			// detect empty lines
			//uword_t m = 0;
			//for (int h = 0; h < shift.wordwidth_; h++)
			//	m |= shift.data_[(v * shift.wordwidth_) + h];

			//ubyte_t m = livelines[v];

			//for (int h = 0; h < shift.wordwidth_; h++)
			for (int h = 0; h < shift.wordwidth_; h++)
			{
				uword_t data = context.memory[(v * context.dstww) + h];
				uword_t expected = shift.data_[(v * shift.wordwidth_) + h];
				bool error = false;

				if (/*m &&*/
					(data != expected) &&
					// tolerate unwritten words for empty source
					!((data == 0xaaaa) && (expected == 0))
					)
				{
					errors++;
					error = true;
				}

				if (!g_allargs.quickcut)
					if (g_allargs.verbose)
					{
						//if (m)
						{
							print_dataword(expected); printf(":"); print_dataword(data); 
							if (error)
								printf("< ");
							else
								printf("  ");
						}
						/*else
						{
							print_dataword(expected); printf(":----------------  ");
						}*/
					}
			}
			if (!g_allargs.quickcut)
				if (g_allargs.verbose)
					printf(" optsrc=%d\n", linemap_src_2_opt[v]);
		}

		if (errors > 0)
		{
			std::cerr << "*************************************************" << std::endl;
			std::cerr << "error: simulation failed on frame " << _frame << " preshift " << pi << std::endl;
			std::cerr << "this is a bug in agtcut! contact the dml with your datas..." << std::endl;
			std::cerr << "quickfix: play with -emxopt <level>, or switch to EMS format for this file" << std::endl;
			printf("press any key...\n");
			getchar();
			exit(1);
		}
	}
}

static int word_le(uword_t w)
{
	int le = 0;
	while ((w & 0x8000) == 0)
	{
		w <<= 1;
		le++;
	}
	return le;
}

static int word_re(uword_t w)
{
	int re = 0;
	while ((w & 0x8000) != 0)
	{
		w <<= 1;
		re++;
	}
	return re;
}

int Context::cycle_sum(int &r_program_size)
{
	int cycles = 0;
	int last_tail = 0;

	r_program_size = 0;

	for (std::list<Operation*>::iterator it = operations.begin(); it != operations.end(); it++)
	{
		Operation *poper = *it;

		// sum cycles
		cycles += poper->cost;

		// sum words
		r_program_size += poper->words;

		// only interested in tails overlapping blitter starts
		int blitter_tail = (poper->kind == OpKind_BLITGo) ? last_tail : 0;

		// only interested in tail time for 1-word ops
		last_tail = (poper->words == 1) ? poper->tail : 0;

		// account for tails
		cycles -= blitter_tail;
	}

	return cycles;
}

