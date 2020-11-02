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
	OpKind_DSTAddaIL,
	//
	OpKind_SYSPageRestore,
	OpKind_BLITInit,
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
	//
	"OpKind_DSTAddaR16",
	"OpKind_DSTSubaR16",
	"OpKind_DSTLea",
	"OpKind_DSTAddaIL",
	//
	"OpKind_SYSPageRestore",
	"OpKind_BLITInit",
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
			if ((srcww == 3) && USE_DSTSKIP_REG_FIXED_PRELOAD)
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

		// em2,em3 addr depend on sprite width - free up registers if possible
		if (srcww == 3)
		{
			EM2_reg = r;
			registers.push_back(dataregister("a6", r++, RegClass_Address, setconstants, setconstants, true,		0xffff8a2a));	// em2
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

			registers.push_back(dataregister("a6", r++, RegClass_Address, setconstants, setconstants, true,		(ulong_t)data));
		}
		else
		{
			// leave a6 free
			registers.push_back(dataregister("a6", r++, RegClass_Address));
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
		registers.push_back(dataregister("d1", r++, RegClass_Data,		setconstants, setconstants, true,		0x00ffffc0));

		PLANES_reg = r;
		registers.push_back(dataregister("d2", r++, RegClass_Data,		setconstants, setconstants, true,		0xFF000004));

//		registers.push_back(dataregister("sr", r++, true, true, 0x00002300));
	}

	Context(int _srcww, int _srch, int _srccolww, int _guardxs)
		: srcww(_srcww), srch(_srch),
		  srccolww(_srccolww),
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
		  BHOG_reg(0),
		  guardwords((_guardxs + (16-1)) >> 4)
	{
		memory = new uword_t[dstww * srch];

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

		BLiTSYI_name = "$ffff8a22.w";
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

		const std::string& str = genout.str();
		std::ofstream cfile;
		cfile.open(tmp_src);  
		cfile << str;  
		cfile.close();
		
#if defined(_MSC_VER)
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

			DeleteFileA(tmp_src.c_str()/*"tmp68k.s"*/);
		}
#endif
        
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

	std::vector<dataregister> registers;

	std::list<Operation*> operations;

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
					changed = true;
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
			int delta = pactual->delta;
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
									printf("lea promote opportunity: reg %d [$%04x]\n", freereg, delta);

							curr = operations.erase(curr);
							curr = operations.insert(curr, new DST_AddaR16(*this, freereg, SCRLINE_reg));
							curr = operations.insert(curr, new DR_LoadI16(*this, freereg, delta*dstlinewid));
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
			BLiT_Go *pactual = reinterpret_cast<BLiT_Go*>(pop);

			std::list<Operation*>::iterator best_tailop = curr;

			std::list<Operation*>::iterator itscan = curr; itscan++;

			int best_tailval =  (*itscan)->tail;

			for (; itscan != operations.end(); itscan++)
			{
				OpKind kind = (*itscan)->kind;

				int trywords = (*itscan)->words;

				// special ops with nontrivial size/implementation, we don't reorder across these
				// note: covers most of the specific tests below, but we check them anyway
				if (trywords <= 0)
					break;

				// observe routine end
				if (kind == OpKind_Terminate)
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
				if (kind == OpKind_BLITSetup)
					break;

				int trytail = (*itscan)->tail;

				if (
					((trywords == 1) && (trytail > best_tailval)) 
				)
				{
					best_tailval = trytail;
					best_tailop = itscan;
				}
			}

			// if we found a better position, move the blit
			if (best_tailop != curr)
			{
				if (g_allargs.verbose)
					printf("relocating blit to hide tail=%d cycles\n", best_tailval);
				operations.erase(curr);
				operations.insert(best_tailop, pop);
			}
		}
	};

//terminate:;
}

void run_sim(Context &context, ubyte_t *livelines, int _frame, int pi, preshift_t &shift, std::vector<int> &best_invpattern, bool _generate)
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
				printf("debug simulation...\n");
		int errors = 0;
		for (int v = 0; v < context.srch; v++)
		{
			// detect empty lines
			//uword_t m = 0;
			//for (int h = 0; h < shift.wordwidth_; h++)
			//	m |= shift.data_[(v * shift.wordwidth_) + h];

			ubyte_t m = livelines[v];

			for (int h = 0; h < shift.wordwidth_; h++)
			{
				uword_t data = context.memory[(v * context.dstww) + h];
				uword_t expected = shift.data_[(v * shift.wordwidth_) + h];

				if (m && (data != expected))
					errors++;

				if (!g_allargs.quickcut)
					if (g_allargs.verbose)
					{
						if (m)
						{
							print_dataword(expected); printf(":"); print_dataword(data); printf("  ");
						}
						else
						{
							print_dataword(expected); printf(":----------------  ");
						}
					}
			}
			if (!g_allargs.quickcut)
				if (g_allargs.verbose)
					printf(" blit=%d\n", best_invpattern[v]);
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

void generate_emx_frame(spriteframe_t &spr, int _frame)
{
	if (g_allargs.verbose)
		std::cout << "frame [" << _frame << "]" << std::endl << std::flush;
//	else
//		std::cout << "." << std::flush;

	// default line order
	std::vector<int> best_pattern;
	std::vector<int> best_invpattern;
	for (int f = 0; f < spr.h_; f++)
	{
		best_pattern.push_back(f);
	}

	static const int c_max_ems_components = 320 / 48;

	ubyte_t *emptylines = new ubyte_t[spr.h_];
	memset(emptylines, 0, spr.h_);

	ubyte_t *livelines = new ubyte_t[spr.h_];
	memset(livelines, 0, spr.h_);

	// find common optimized line order
	if (USE_COMMON_LINEORDER_OPTIMIZATION && (spr.h_ > 1))
	{
		if (g_allargs.verbose)
			std::cout << "optimizing common line order across all preshifts..." << std::endl << std::flush;

		int shiftcount = spr.psmask_frames_.size();

		// make copy of data in terms of 3*y EM words
		uword_t *buffer = new uword_t[3 * spr.h_ * shiftcount * c_max_ems_components];
		memset(buffer, 0, sizeof(uword_t) * 3 * spr.h_ * shiftcount * c_max_ems_components);

		int max_ci = 0;
		{
			int si = 0;
			std::vector<preshiftlist_t>::iterator pmit = spr.psmask_frames_.begin();
			for (; pmit != spr.psmask_frames_.end(); pmit++, si++)
			{
				// components within a preshift
				int ci = 0;
				for (preshiftlist_t::iterator cit = pmit->begin(); cit != pmit->end(); cit++, ci++)
				{
					if (ci > max_ci)
						max_ci = ci;

					preshift_t &shift = *cit;

					static const int pagesize = (3 * spr.h_);
					int page = ((ci * shiftcount) + si) * pagesize;
					uword_t *sbuffer = &buffer[page];

					//				uword_t *sbuffer = buffer;

					for (int v = 0; v < spr.h_; v++)
					{
						sbuffer[(v * 3) + 0] = shift.data_[(v * shift.wordwidth_) + 0];

						if (shift.wordwidth_ == 2)
						{
							// note: em1,em3 used in 2-wide blits. em2 is skipped
							sbuffer[(v * 3) + 1] = 0;
							sbuffer[(v * 3) + 2] = shift.data_[(v * shift.wordwidth_) + 1];
						}
						else
							if (shift.wordwidth_ == 3)
							{
								sbuffer[(v * 3) + 1] = shift.data_[(v * shift.wordwidth_) + 1];
								sbuffer[(v * 3) + 2] = shift.data_[(v * shift.wordwidth_) + 2];
							}

						// track all non-empty lines in any preshift
						if (USE_EMPTYLINE_OPTIMIZATION)
						{
							if (sbuffer[(v * 3) + 0] || sbuffer[(v * 3) + 1] || sbuffer[(v * 3) + 2])
								livelines[v] = 1;
							else
								emptylines[v] = 1;
						}
						else
						{
							livelines[v] = 1;
							emptylines[v] = 0;
						}
					}
				}
			}
		}

		// force partially (& fully) empty lines to end of pattern
#if (0)
		{
			bool busy;
			do
			{
				busy = false;

				for (int v = 0; v < (spr.h_ - 1); v++)
				{
					int v0 = best_pattern[v + 0];
					int v1 = best_pattern[v + 1];

//					if (!livelines[v0] && livelines[v1])
					if (emptylines[v0] && !emptylines[v1])
					{
						best_pattern[v + 0] = v1;
						best_pattern[v + 1] = v0;
						busy = true;
						break;
					}
				}

			} while (busy);
		}
#endif

#if (1)
		// force fully empty lines to end of pattern
		{
			bool busy;
			do
			{
				busy = false;

				for (int v = 0; v < (spr.h_ - 1); v++)
				{
					int v0 = best_pattern[v + 0];
					int v1 = best_pattern[v + 1];

					if (!livelines[v0] && livelines[v1])
					{
						best_pattern[v + 0] = v1;
						best_pattern[v + 1] = v0;
						busy = true;
						break;
					}
				}

			} while (busy);
		}
#endif

		// find active region
		int live_line_count = best_pattern.size();
		for (int v = 0; v < best_pattern.size(); v++)
		{
			if (!livelines[best_pattern[v]])
			{
				live_line_count = v;
				break;
			}
		}
		int empty_line_count = spr.h_ - live_line_count;


		// initial sort of mask edges
		if (USE_INITIAL_EDGE_SORT)
		{
			std::vector<std::pair<ulong_t,int>> sorting;
			sorting.resize(live_line_count);
			for (int v = 0; v < live_line_count; v++)
			{
				auto &s = sorting[v];
				s.first = 0;
				s.second = best_pattern[v];
			}

			int full_src_wordwidth = ((spr.w_ + 15) & -16) >> 4;
			int src_pixwidth = full_src_wordwidth << 4;

			for (int v = 0; v < live_line_count; v++)
			{
				int dsty = v;// best_pattern[v];
				uword_t *line = &spr.mask_[(dsty * full_src_wordwidth)];

				int le = 0;
				int re = 0;
				int mle = 0;
				int mre = 0;

				for (int x = 0; x < src_pixwidth; x++)
				{
					uword_t m = ~(line[x >> 4]);
					if (m & (0x8000 >> (x & 0xF)))
					{
						le = x;

						for (int xx = x; xx < src_pixwidth; xx++)
						{
							uword_t m = ~(line[xx >> 4]);
							if (!(m & (0x8000 >> (xx & 0xF))))
							{
								mle = xx; 
								break;
							}
						}

						break;
					}
				}
				for (int x = src_pixwidth-1; x >= 0; x--)
				{
					uword_t m = ~(line[x >> 4]);
					if (m & (0x8000 >> (x & 0xF)))
					{
						re = x; 

						for (int xx = x; xx >= 0; xx--)
						{
							uword_t m = ~(line[xx >> 4]);
							if (!(m & (0x8000 >> (xx & 0xF))))
							{
								mre = xx; 
								break;
							}
						}

						break;
					}
				}

				sorting[v].first =
					((le & 0x1FF) << 23) | 
					((re & 0x1FF) << 14) | 
					((mle & 0x7F) << 7) | 
					((mre & 0x7F) << 0);

				//std::cout << "le:" << le << " re:" << re << std::endl;
			}
			std::sort(sorting.begin(), sorting.end());
			for (int vnew = 0; vnew < live_line_count; vnew++)
			{
				best_pattern[vnew] = sorting[vnew].second;
			}
		}

//		exit(1);




/*
		int partial_line_count = 0;
		for (int v = 0; v < best_pattern.size(); v++)
		{
			if (livelines[best_pattern[v]] && emptylines[best_pattern[v]])
				partial_line_count++;
		}
*/
		if (g_allargs.verbose)
		{
			printf("live lines = %d\n", live_line_count);
			//printf("partial lines = %d\n", partial_line_count);
			printf("empty lines = %d\n", empty_line_count);
		}

		// optimize the line order for NON EMPTY LINES to minimize changes to EM registers
		// the empty lines are already at the end of the sequence so optimization works on a shorter sequence (up to live_line_count)
	
		{
			// optimize line pattern by reordering
			
			if (!g_allargs.quickcut)
			{
				mersenne_twister twist;

#if (USE_NOISY_REORDERING)
				unsigned long noise_factor = (1 << (32-7));
#endif
				double initial_dbest_cost = -1;
				int initial_best_loadcost = -1;

				// begin with default line pattern (top-to-bottom)
				double dbest_cost = double(1e20);
				int best_loadcost = 1<<20;
				ulong_t last_improvement = 0;

				std::vector<int> trial_pattern(best_pattern);

				ulong_t darwin_limit = 1 << g_allargs.emxoptlevel;
				for (ulong_t trial = 0, depth = 0; depth < darwin_limit; trial++, depth++)
				{
					for (int r = best_pattern.size() - 1; r >= 0; r--)
						trial_pattern[r] = best_pattern[r];

					// select mutation rate
					//				int mutrate = ((twist.genS32() >> 4) % 4) + 1;

					// we want to measure improvement from initial sort, so trial 0
					// skips mutation
					if (trial > 0)
					{

						int mutrate = 1 + ((twist.genS32() >> 4) % 8);
						mutrate = 1 + ((mutrate * mutrate) >> 3);

						int choice1 = 0;
						int choice2 = 0;

						for (int mut = 0; mut < mutrate; mut++)
						{
							do
							{
								// select pair to exchange
								choice1 = (twist.genU32() >> 4) % live_line_count;
								choice2 = (twist.genU32() >> 4) % live_line_count;

								// don't reorder against self
							} while ((choice1 == choice2) ||
								// don't place an empty line as the first 
								// line to render, otherwise individual components won't skip initial colour data
								((choice2 == 0) && (0 == livelines[trial_pattern[choice1]])) ||
								((choice1 == 0) && (0 == livelines[trial_pattern[choice2]])));

							// reorder
							//						int tmp = trial_pattern[choice1];
							//						trial_pattern[choice1] = trial_pattern[choice2];
							//						trial_pattern[choice2] = tmp;

							// bias sorting in one direction
							if (choice1 > choice2)
							{
								choice1 ^= choice2;
								choice2 ^= choice1;
								choice1 ^= choice2;
							}

							//if (choice1 < choice2)
							{
								// A		A
								// B <c1	D \
															// C		B | o
								// D <c2	C / o+1  tmp
								// E		E

								// push choice2 int choice1, moving all downwards
								int tmp = trial_pattern[choice2];
								for (int o = choice2 - 1; o >= choice1; o--)
									trial_pattern[o + 1] = trial_pattern[o];
								trial_pattern[choice1] = tmp;
							}
							/*	
							else
							{
							// A		A
							// B <c2	C \ o-1
							// C		D | o
							// D <c1	B /      tmp
							// E		E

							// push choice2 into choice1, moving all upwards
							int tmp = trial_pattern[choice2];
							for (int o = choice2 + 1; o <= choice1; o++)
							trial_pattern[o - 1] = trial_pattern[o];
							trial_pattern[choice1] = tmp;

							// swap
							//							int tmp = trial_pattern[choice1];
							//							trial_pattern[choice1] = trial_pattern[choice2];
							//							trial_pattern[choice2] = tmp;
							}
							*/
						} // for mutrate

					} // trial > 0

					// estimate efficiency of new pattern in terms of deltas per line
					{
						//int totalcost = 0;
						double dtotalcost = 0;
						int loadcost = 0;

						int shiftcount = spr.psmask_frames_.size();
						for (int si = 0; si < shiftcount; si++)
						{
							for (int ci = 0; ci <= max_ci; ci++)
							{
								static const int pagesize = (3 * spr.h_);
								int page = ((ci * shiftcount) + si) * pagesize;
								uword_t *sbuffer = &buffer[page];

								//						uword_t *sbuffer = &buffer[3 * spr.h_ * si * ci];
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

									int error = (lc1 + lc2 + lc3);
									loadcost += error;
									dtotalcost += error * 256.0;

									// incur a slight penalty for split longs
									if (lc1 == 1 && lc3 == 1 && lc2 == 0)
										dtotalcost += 1.0;

									// attempt to keep lines roughly in top-bottom order by including small cost for 'pointless' reordering
									// this provides more opportunity to use a fixed increment between lines (save space with register add vs lea)
									// notes: 
									// - some chance this could prevent a difficult-to-reach optimization via small incremental mutations
									// - perhaps a bit more likely to result in extra LEA at start, to reach first block of contiguous lines 

									{
										// cost is same for forward or reverse ordering - delta value will be the same even if instruction isn't
										int dycost = (v1 - v0);
										if (dycost < 0)
										{
											dycost = -dycost;
											//dycost++;
										}
										dycost--;	// no cost for adjacents

										double dypenalty = log(1.0 + dycost);
										if (v1 < v0)
											dypenalty *= 1.1;

//										if (dycost > 8)
//											dycost = 8;
//										totalcost += dycost;

										dtotalcost += dypenalty;
									}
								}
							}
						}

						// include benign changes (cost delta == 0) to assist Darwin...
						if ((dtotalcost <= dbest_cost)
#if (USE_NOISY_REORDERING)
							||
							// include random acceptance early in trials, with cutoff
							(/*(depth < (darwin_limit>>1))*/(trial < 10000) && (twist.genU32() < noise_factor))
#endif
						)
						{
							// reset the search depth on any 'real' improvement
							if (dtotalcost < dbest_cost)
							{
								last_improvement = trial;
								depth = 0;
	#if (USE_NOISY_REORDERING)
								//noise_factor >>= 1;
	#endif
							}

							dbest_cost = dtotalcost;
							best_loadcost = loadcost;
							//						best_pattern = trial_pattern;
							for (int r = best_pattern.size() - 1; r >= 0; r--)
								best_pattern[r] = trial_pattern[r];

						}

						if (initial_dbest_cost < 0)
						{
							initial_dbest_cost = dbest_cost;
							initial_best_loadcost = best_loadcost;
						}
					}

#if (USE_NOISY_REORDERING)
					// decay noise factor - allows scoring to gradually dominate over randomness with each iteration
					noise_factor = (unsigned long)(((unsigned long long)noise_factor * 0xfff00000) >> 32);
					//std::cout << "noise factor:" << std::hex << noise_factor << std::endl;
#endif
				}

				//if (g_allargs.verbose)
				{
					if (dbest_cost < initial_dbest_cost)
					{
						double improvement_pct = 100.0 * (dbest_cost / initial_dbest_cost);

						printf("reduction = %.2f%% in %d trials @ loadcost=%d (totalcost=%f)\n",
							improvement_pct,
							last_improvement,
							best_loadcost,
							dbest_cost
							);
						std::cout << std::flush;
					}
					else
					{
						printf("no improvement in %d trials @ loadcost=%d (totalcost=%f)\n",
							last_improvement,
							best_loadcost,
							dbest_cost
							);
						std::cout << std::flush;
					}
				}
			}

			if (g_allargs.verbose)
			{
				// output the best pattern

				printf("\nordered:\n");
				for (int v = 0; v < spr.h_; v++)
				{
					int dsty = v;
					printf("s:%02d ", dsty);
					for (int ci = 0; ci <= max_ci; ci++)
					{
						static const int pagesize = (3 * spr.h_);
						int page = ((ci * shiftcount) + 0) * pagesize;
						uword_t *sbuffer = &buffer[page];

						print_dataword(sbuffer[(dsty * 3) + 0]);
						print_dataword(sbuffer[(dsty * 3) + 1]);
						print_dataword(sbuffer[(dsty * 3) + 2]);
						printf("+");
					}
					if (livelines[dsty])
						printf(" L");
					printf("\n");
				}

				printf("\nreordered:\n");
				for (int v = 0; v < spr.h_; v++)
				{
					int dsty = best_pattern[v];
					printf("s:%02d ", dsty);
					for (int ci = 0; ci <= max_ci; ci++)
					{
						static const int pagesize = (3 * spr.h_);
						int page = ((ci * shiftcount) + 0) * pagesize;
						uword_t *sbuffer = &buffer[page];

						print_dataword(sbuffer[(dsty * 3) + 0]);
						print_dataword(sbuffer[(dsty * 3) + 1]);
						print_dataword(sbuffer[(dsty * 3) + 2]);
						printf("+");
					}
					if (livelines[dsty])
						printf(" L");
					printf("\n");
				}
			}
		}

		delete[] buffer; buffer = 0;
		//delete[] livelines; livelines = 0;
		//delete[] emptylines; emptylines = 0;
	}


	spr.lineorder = best_pattern;

	//exit(-1);

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
	for (std::vector<preshiftlist_t>::iterator pmit = spr.psmask_frames_.begin(); pmit != spr.psmask_frames_.end(); pmit++, pi++)
	{
		for (preshiftlist_t::iterator pit = pmit->begin(); pit != pmit->end(); pit++)
		{
			preshift_t &shift = *pit;

			if ((DEBUG_PRESHIFT >= 0) && (pi != DEBUG_PRESHIFT))
				continue;

			Context context(shift.wordwidth_, shift.height_, /*srccol_wordwidth*/((spr.w_ + 15) >> 4), /*guardxs=*/g_allargs.emguardx);

			// output the source image pattern
			if (!g_allargs.quickcut)
			if (g_allargs.verbose)
			{
				printf("\n\nframe %d preshift %d preview:\n", _frame, pi);
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

	//		byte_t *changes = new byte_t[shift.planewords_ * shift.height_];
	//		memset(changes, 0, shift.planewords_ * shift.height_);
			byte_t *changes = new byte_t[3 * shift.height_];
			memset(changes, 0, 3 * shift.height_);

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

	#if (0)
			if (!g_allargs.quickcut)
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

					if (g_allargs.verbose)
					{
						printf("\nconverged or exhausted at %d trials @ loadcost=%d (totalcost=%d)\n", last_improvement, best_cost, best_loadcost);

						// output the best pattern
						for (int v = 0; v < shift.height_; v++)
						{
							int dsty = best_pattern[v];
							print_dataword(buffer[(dsty * 3) + 0]);
							print_dataword(buffer[(dsty * 3) + 1]);
							print_dataword(buffer[(dsty * 3) + 2]);
							printf("\n");
						}
					}
				}
			}
	#endif

			ubyte_t *component_livelines = new ubyte_t[shift.height_];
			memset(component_livelines, 0, shift.height_);

			// find nonempty lines
			for (int y = 0; y < shift.height_; y++)
			{
				int dsty = best_pattern[y];

				if (USE_EMPTYLINE_OPTIMIZATION)
				{
					uword_t *srcdata = &buffer[dsty * 3];

					// look for empty blits (all EMs == 0)
					u64 line = 0;
					for (int x = 0; x < 3; x++)
					{
						line <<= 16;
						line |= srcdata[x];
					}

					component_livelines[dsty] = (line != 0);
				}
				else
				{
					component_livelines[dsty] = 1;
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
				bool dirtyline = true;

				for (int v = 0; v < shift.height_; v++)
				{
					int dsty = best_pattern[v];

					// generate deltas for all but 1st BUSY line
					int yn3 = dsty * 3;
					int yp3 = yn3;
					if (v > 0)
						yp3 = best_pattern[v - 1] * 3;

					// is this a real line?
					//uword_t m = buffer[yn + 0];
					//if (shift.wordwidth_ >= 2)
					//	m |= buffer[yn + 2];
					//if (shift.wordwidth_ == 3)
					//	m |= buffer[yn + 1];

					// generate deltas for only real lines
					//if (m)
					if (component_livelines[dsty])
					{
						if (1)
							changes[yn3 + 0] = (dirtyline || (lastchanges[0] != buffer[yn3 + 0])) ? 1 : 0;
						if (shift.wordwidth_ == 3)
							changes[yn3 + 1] = (dirtyline || (lastchanges[1] != buffer[yn3 + 1])) ? 1 : 0;
						if (shift.wordwidth_ >= 2)
							changes[yn3 + 2] = (dirtyline || (lastchanges[2] != buffer[yn3 + 2])) ? 1 : 0;

						// accept new data for non-empty lines
						lastchanges[0] = buffer[yn3 + 0];
						lastchanges[1] = buffer[yn3 + 1];
						lastchanges[2] = buffer[yn3 + 2];

						dirtyline = false;
					}
				}
			}
			else
			{
				for (int v = 0; v < shift.height_; v++)
				{
					int dsty = best_pattern[v];

					if (component_livelines[dsty])
					{
						int dsty3 = dsty * 3;
						changes[dsty3 + 0] = true;
						changes[dsty3 + 1] = (shift.wordwidth_ == 3);
						changes[dsty3 + 2] = (shift.wordwidth_ >= 2);
					}
				}
			}


			// by this point, changes[y:x] marks all dirty EM registers

			//bool em_dirty[3];

			if (!g_allargs.quickcut)
				if (g_allargs.verbose)
					printf("deltas...\n");

			// reset dirty EM registers for this blit width
			//em_dirty[0] = true;
			//em_dirty[1] = (shift.wordwidth_ > 2);
			//em_dirty[2] = (shift.wordwidth_ > 1);

			for (int v = 0; v < shift.height_; v++)
			{
				int dsty3 = best_pattern[v] * 3;

				if (changes[dsty3 + 0]/* || em_dirty[0]*/)
				{
					//em_dirty[0] = false;
					if (!g_allargs.quickcut)
						if (g_allargs.verbose)
							print_dataword(buffer[dsty3 + 0]);
				}
				else
				{
					if (!g_allargs.quickcut)
						if (g_allargs.verbose)
						{
							if (buffer[dsty3 + 0] == 0)
								printf("////////////////");
							else
								printf("................");
						}
				}

				if (changes[dsty3 + 1]/* || em_dirty[1]*/)
				{
					//em_dirty[1] = false;
					if (!g_allargs.quickcut)
						if (g_allargs.verbose)
							print_dataword(buffer[dsty3 + 1]);
				}
				else
				{
					if (!g_allargs.quickcut)
						if (g_allargs.verbose)
						{
							if (buffer[dsty3 + 1] == 0)
								printf("////////////////");
							else
								printf("................");
						}
				}

			
				if (changes[dsty3 + 2]/* || em_dirty[2]*/)
				{
					//em_dirty[2] = false;
					if (!g_allargs.quickcut)
						if (g_allargs.verbose)
							print_dataword(buffer[dsty3 + 2]);
				}
				else
				{
					if (!g_allargs.quickcut)
						if (g_allargs.verbose)
						{
							if (buffer[dsty3 + 2] == 0)
								printf("////////////////");
							else
								printf("................");
						}
				}

				if (!g_allargs.quickcut)
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

			if (!g_allargs.quickcut)
				if (g_allargs.verbose)
					printf("\nsurviving datawords...\n");

			// reset dirty EM registers for this blit width
			//em_dirty[0] = true;
			//em_dirty[1] = (shift.wordwidth_ > 2);
			//em_dirty[2] = (shift.wordwidth_ > 1);

	//		int unique_datawords = 0; // each unique dataword has a uid so it can be reused
			{
				for (int v = 0; v < shift.height_; v++)
				{
					int dsty = best_pattern[v];

					if (!component_livelines[dsty])
						continue;

					uword_t *srcdata = &buffer[dsty * 3];
					byte_t *linechanges = &changes[dsty * 3];
					// generate metrics for nonempty, unique lines only - lines where 1 or more EM registers must change
					if (linechanges[0] || linechanges[1] || linechanges[2])
					{
						// gather metrics for datawords on this line
						bool line_changed = false;
					for (int x = 0; x < 3; x++)
					{
							if (!linechanges[x])
							{
								if (!g_allargs.quickcut)
									if (g_allargs.verbose)
										printf("----------------");
								continue;
							}

							line_changed = true;

							uword_t dataword = srcdata[x];
							emx_metric_t &m = context.metrics[dataword];

							if (!g_allargs.quickcut)
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
								if ((x != lna) && (linechanges[lna]))
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
						if (line_changed)
							active_lines.push_back(lineout(v, dsty));

						if (!g_allargs.quickcut)
							if (g_allargs.verbose)
								printf("\n");
					}
				}
			}

			if (!g_allargs.quickcut)
				if (g_allargs.verbose)
					printf("total surviving = %d\n", context.metrics.size());

			if (!g_allargs.quickcut)
				if (g_allargs.verbose)
					printf("\nscoring:\n");

			// globally score each data word according to metrics
			int assigned_registers = 0;
			for (metricmap::iterator mit = context.metrics.begin(); mit != context.metrics.end(); mit++)
			{
				const uword_t &dataword = mit->first;

				if (!g_allargs.quickcut)
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

				if (!g_allargs.quickcut)
					if (g_allargs.verbose)
						printf(" incidence_score: %.2f\n", fscore);
			}

			// globally score each data word according to its left-adjacency (upper word opportuntiy)
			for (metricmap::iterator mit = context.metrics.begin(); mit != context.metrics.end(); mit++)
			{
				const uword_t &dataword = mit->first;

				if (!g_allargs.quickcut)
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

				if (!g_allargs.quickcut)
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
						if (!g_allargs.quickcut)
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
						if (!g_allargs.quickcut)
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

								if (!g_allargs.quickcut)
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

										if (!g_allargs.quickcut)
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
			//em_dirty[0] = true;
			//em_dirty[1] = (shift.wordwidth_ > 2);
			//em_dirty[2] = (shift.wordwidth_ > 1);

			std::vector<int> blits;

			int stage = 0;
	//		int regcount = 0;
			u64 lifebits = 0;
			int job = 0;
			for (std::vector<lineout>::iterator lit = active_lines.begin(); lit != active_lines.end(); lit++, job++)
			{
				lineout &lo = *lit;
				int dsty = lo.dline;

				// only live lines in active_lines table

				uword_t *srcdata = &buffer[dsty * 3];
				byte_t *linechanges = &changes[dsty * 3];

				blits.push_back(deps.size());

				for (int x = 0; x < 3; x++)
				{
					if (!(linechanges[x]/*||em_dirty[x]*/))
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

					//em_dirty[x] = false;
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

			if (!g_allargs.quickcut)
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
			if (!g_allargs.quickcut)
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

			// todo: enable dynamic NFSR with single or final component

			// SKEW + NFSR, DYINC
	//		context.blitter.skew = (shift.shift_ & 15) | (((shift.wordwidth_ << 4) == ((spr.w_ + 15) & -16)) ? 0x00 : 0x40);

			bool NFSR = 
				(shift.wordwidth_ > 1) && // NFSR is meaningless for wordwidth==1
				(shift.wordsremaining_ == 0) && 
				(((spr.w_ + 15) >> 4) != (shift.wordwidth_ + shift.wordoffset_));

			bool FISR = (shift.wordoffset_ > 0);

				//false;// ((shift.wordwidth_ << 4) == ((spr.w_ + 15) & -16));
			context.blitter.skew = 
				(shift.shift_ & 15) | 
				(NFSR ? 0x40 : 0x00) | 
				(FISR ? 0x80 : 0x00);

			context.blitter.dyinc = (8 + 2) - (shift.wordwidth_ << 3);

			// SYINC = difference between source wordwidth & preshift wordwidth, + 2
			// note: compensate for NFSR!
			context.blitter.syinc = 
				(((((spr.w_ + 15) >> 4) - shift.wordwidth_) << 1) + 2) 
					+ (NFSR ? 2 : 0) 
					- (FISR ? 2 : 0);
		
			// (8 + 2) - (shift.wordwidth_ << 3);

			// gather metrics on LEA skip values, for loading into registers
			// must be done before sim reset!
			if (USE_DSTSKIP_REG_OPTIMIZATION)
			{
				int current_line = 0;

				for (int y = 0; y < shift.height_; y++)
				{
					int dsty = best_pattern[y];

					// is the blit going to output something?
					if (component_livelines[dsty])
					{
						// line hop +/-
						int linejump = dsty - current_line;

						// all non-empty blits require a dst address update
						// todo: linewidth should be an argument
						if (linejump)
						{
							int addressjump = (linejump * context.dstlinewid);
							if (addressjump < 32768 && addressjump >= -32768)
							{
								// caution: don't abs() -32768 case as it has will have no 16bit representation
								int reduced_dataword = addressjump;
								if (reduced_dataword > -32768)
									reduced_dataword = abs(reduced_dataword);
									
								uword_t metrics_dataword = (uword_t)reduced_dataword;

								context.metrics[metrics_dataword].incidence++;
								context.metrics[metrics_dataword].references++;
							}
						}

						current_line = dsty;
					}
				}
			}

			context.reset_sim(true);

			// reset dirty EM registers for this blit width
			//em_dirty[0] = true;
			//em_dirty[1] = (shift.wordwidth_ > 2);
			//em_dirty[2] = (shift.wordwidth_ > 1);

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

//				context.operations.push_back(new SYS_PageRestore(context));
//				context.operations.back()->execute();

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
			

				int src_lines_skipped = 0;

				linemap lines;
				for (int y = 0; y < shift.height_; y++)
				{
					int dsty = best_pattern[y];

					uword_t *srcdata = &buffer[dsty * 3];
					byte_t *linechanges = &changes[dsty * 3];

					// bypass empty blits (all EMs == 0)
					//u64 line = 0;
					//for (int x = 0; x < 3; x++)
					//{
					//	line <<= 16;
					//	line |= srcdata[x];
					//}

					// is the blit going to output something?
					//if (line)
					if (component_livelines[dsty])
					{
						// line hop +/-
						int linejump = dsty - current_line;

						// all non-empty blits require a dst address update
						// todo: linewidth should be an argument
						if (linejump)
						{
							// caution: can't use LEA for jumps > +/-32k
							int addressjump = (linejump * context.dstlinewid);
							if (addressjump < 32768 && addressjump >= -32768)
							{
								if (USE_DSTSKIP_REG_OPTIMIZATION)
								{

									int reg_src = -1;
									DataSource ds;

									ds = context.sim_resolve_data16(uword_t(addressjump), reg_src, true);
									if (ds == DataSource_RegisterL)
									{
										// for +ve steps, look for +ve equivalent register and apply ADD operation
										// only allocates register if value does not pre-exist 
										context.operations.push_back(new DST_AddaR16(context, reg_src, context.SCRLINE_reg));
									}
									else
									{
										// for -ve steps, look for +ve equivalent register and apply SUB operation
										// only allocates register if value does not pre-exist 
										ds = context.sim_resolve_data16(uword_t(-addressjump), reg_src, true);
										if (ds == DataSource_RegisterL)
										{
											context.operations.push_back(new DST_SubaR16(context, reg_src, context.SCRLINE_reg));
										}
										else
										{
											// pointless allocation - fall back to LEA
											context.operations.push_back(new DST_Lea(context, context.SCRLINE_reg, context.SCRLINE_reg, linejump));
										}
									}
								}
								else
								{
									context.operations.push_back(new DST_Lea(context, context.SCRLINE_reg, context.SCRLINE_reg, linejump));
								}
							}
							else
							{
								context.operations.push_back(new DST_AddaIL(context, context.SCRLINE_reg, linejump));
							}
							context.operations.back()->execute();
							if (src_lines_skipped > 0)
							{
								context.operations.push_back(new SRC_Skip(context, src_lines_skipped));
								context.operations.back()->execute();
								src_lines_skipped = 0;
							}
						}

						// check for all 3 changing
						if ((linechanges[0]/* || em_dirty[0]*/) &&
							(linechanges[1]/* || em_dirty[1]*/) &&
							(linechanges[2]/* || em_dirty[2]*/))
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

							//em_dirty[0] = em_dirty[1] = em_dirty[2] = false;
						}
						else
						if ((linechanges[0]/* || em_dirty[0]*/) && 
							(linechanges[1]/* || em_dirty[1]*/))
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

							//em_dirty[0] = em_dirty[1] = false;

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
						if ((linechanges[1]/* || em_dirty[1]*/) && 
							(linechanges[2]/* || em_dirty[2]*/))
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

							//em_dirty[1] = em_dirty[2] = false;
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

								if (linechanges[x]/* || em_dirty[x]*/)
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

									//em_dirty[x] = false;
								}
							}
						}

						// update blitter dest/ycount
						context.operations.push_back(new BLiT_SetYC(context, context.PLANES_reg));
						context.operations.back()->execute();
						context.operations.push_back(new BLiT_SetDSTADDR(context, context.SCRLINE_reg));
						context.operations.back()->execute();

						// execute the blit for this line
						context.operations.push_back(new BLiT_Go(context, context.BHOG_reg, y, dsty));
						context.operations.back()->execute();

						// next linejump+blit will be relative to this one (and not an empty one)
						current_line = dsty;
					}
					else
					{
						src_lines_skipped++;
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

	#if (SIM_ALL_STAGES)
			run_sim(context, component_livelines, _frame, pi, shift, best_invpattern, /*generate=*/false);
	#endif
			// peephole passes

			bool changed = !g_allargs.quickcut;
			int xform_passes = 0;

	//		if (!g_allargs.quickcut)

			while (changed)
			{
				changed = false;

				// immediate loads to registers with register-derived values
	
				while (context.transform_pass())
				{
					changed = true;
					xform_passes++;
	#if (SIM_ALL_STAGES)
					run_sim(context, component_livelines, _frame, pi, shift, best_invpattern, /*generate=*/false);
	#endif
				}

				changed |= context.cleanup_pass();
	#if (SIM_ALL_STAGES)
				run_sim(context, component_livelines, _frame, pi, shift, best_invpattern, /*generate=*/false);
	#endif
				{
					while (context.promote_pass())
					{
						changed = true;
	#if (SIM_ALL_STAGES)
						run_sim(context, component_livelines, _frame, pi, shift, best_invpattern, /*generate=*/false);
	#endif
					}
				}
			}
			
#if 0
// not working yet - should probably be performed as fallback case for a failed transform of LEA
// so subsequent LEAs can take advantage of reused register load
			{
				while (context.promote_lea_pass())
				{
#if (SIM_ALL_STAGES)
					run_sim(context, component_livelines, _frame, pi, shift, best_invpattern, /*generate=*/false);
#endif
				}
			}
#endif

			//if (!g_allargs.quickcut)
			{
				context.reduce_pass();
	#if (SIM_ALL_STAGES)
				run_sim(context, component_livelines, _frame, pi, shift, best_invpattern, /*generate=*/false);
	#endif
			}


			// optimize instruction tail / blitter overlaps
			if (!g_allargs.quickcut)
				context.tail_pass();

			if (!g_allargs.quickcut && g_allargs.verbose)
				std::cout << "preshift " << pi << " codegen optimization transforms: " << xform_passes << std::endl;
		
			//
			run_sim(context, component_livelines, _frame, pi, shift, best_invpattern, /*generate=*/true);



			ubyte_t *p68kbin = 0;
			size_t size68kbin = 0;
		
			context.assemble(p68kbin, size68kbin, g_allargs.mactmp);

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

			delete[] component_livelines; component_livelines = 0;
		} // components
	} // preshifts

	delete[] livelines; livelines = 0;
	// generate the generator



}


bool generate_emx_sequence()
{
	std::cout << "code-generating EMX frames: " << std::endl;

	// individual sprites follow
	spritelist_t::iterator it = g_spriteframes.begin();
//	for (int x = 0; x < 4; x++, it++);

	int f = 0;
	for (; it != g_spriteframes.end(); it++, f++)
	{
		if ((DEBUG_FRAME >= 0) && (f != DEBUG_FRAME))
			continue;
		generate_emx_frame(*it, f);
	}

	return true;
}
