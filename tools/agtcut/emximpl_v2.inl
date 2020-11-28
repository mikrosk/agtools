

static const int c_max_buscycles_per_blit = 64;


// sort scan pattern into blocks according to:
// [preshift0 mask width] [span width] [span offset] [span masks]


void generate_emx2_preshift_analysis(const spriteframe_t &spr, int _frame, preshift_t &_shift, int _psi)
{
	//int full_src_wordwidth = ((spr.w_ + 15) & -16) >> 4;
	//int src_pixwidth = full_src_wordwidth << 4;

	// extract patterns of spans according to individual span constraints

	auto &sa = _shift.sa;
	sa.span_vpattern_.resize(spr.h_);

	for (int v = 0; v < spr.h_; v++)
	{
		auto &span_hseq = sa.span_vpattern_[v];

		int accumulated_bcycles = 0;
		int accumulated_words = 0;
		int nonzero_words = 0;
		int span_pos = -1;
		u16 last_word = 0;

		u16 em[3] = { 0, 0, 0 };

		int w = 0;
		while (w < _shift.wordwidth_)
		{
			// fetch next word
			u16 *pline = &_shift.data_[(v * _shift.wordwidth_)];
			u16 word = pline[w];

			int word_pos = w++;

			// detect start of new span
			if ((span_pos < 0) && (word != 0))
				span_pos = word_pos;
				
			if (word == 0)
			{
				// absorb zero masks, but emit pending span

				if (nonzero_words > 0)
				{
					// check reference to shifted previous data
					bool FISR = 
						(span_pos > 0) && 
						(u16(0xFFFF0000UL >> _shift.shift_) & pline[span_pos]);

					// check reference to current data (not shifted from previous)
					bool NFSR =
						(accumulated_words > 1) &&
						(u16(0x0000FFFFUL >> _shift.shift_) & pline[span_pos + accumulated_words - 1]) == 0;

					// gap of at least 4*3=12 bus cycles, terminate span here
					span_hseq.emplace_back
					(
						constrained_wordspan
						(
							/*ws=*/span_pos,
							/*ww=*/accumulated_words,
							/*cxs=*/FISR,
							/*cxe=*/NFSR,
							/*sline=*/v,
							/*sline_aw=*/spr.active_widths[v],
							/*pem=*/em,
							/*bccount=*/accumulated_bcycles
						)
					);

					span_pos = -1;
					accumulated_words = 0;
					nonzero_words = 0;
					accumulated_bcycles = 0;

					last_word = 0;
				}
			}
			else
			{
				// count bus cycles
				int word_bcycles = 2;
				if (word != 0xFFFF)
					word_bcycles++;

				if ((accumulated_bcycles + word_bcycles) > c_max_buscycles_per_blit)
				{
					// check reference to shifted previous data
					bool FISR =
						(span_pos > 0) &&
						(u16(0xFFFF0000UL >> _shift.shift_) & pline[span_pos]);

					// check reference to current data (not shifted from previous)
					bool NFSR =
						(accumulated_words > 1) &&
						(u16(0x0000FFFFUL >> _shift.shift_) & pline[span_pos + accumulated_words - 1]) == 0;

					// span constraints would be exceeded, emit span & continue
					span_hseq.emplace_back
					(
						constrained_wordspan
						(
							/*ws=*/span_pos,
							/*ww=*/accumulated_words,
							/*cxs=*/FISR,
							/*cxe=*/NFSR,
							/*sline=*/v,
							/*sline_aw=*/spr.active_widths[v],
							/*pem=*/em,
							/*bccount=*/accumulated_bcycles
						)
					);

					span_pos = -1;
					accumulated_words = 0;
					nonzero_words = 0;
					accumulated_bcycles = 0;

					last_word = 0;
				}

				// record changes in mask state, up to 3
				// force commit of new mask word (EM2) after first (EM1)
				if ((last_word != word) || (nonzero_words==1))
				{
					last_word = word;
					em[nonzero_words++] = word;
				}

				accumulated_words++;
				accumulated_bcycles += word_bcycles;

				// check constraints
				if
				(
					(w == _shift.wordwidth_) ||
					(nonzero_words == 3)
				)
				{
					// check reference to shifted previous data
					bool FISR =
						(span_pos > 0) &&
						(u16(0xFFFF0000UL >> _shift.shift_) & pline[span_pos]);

					// check reference to current data (not shifted from previous)
					bool NFSR = 
						(accumulated_words > 1) && 
						(u16(0x0000FFFFUL >> _shift.shift_) & pline[span_pos + accumulated_words - 1]) == 0;

					// span constraints would be exceeded, emit span & continue
					span_hseq.emplace_back
					(
						constrained_wordspan
						(
							/*ws=*/span_pos,
							/*ww=*/accumulated_words,
							/*cxs=*/FISR,
							/*cxe=*/NFSR,
							/*sline=*/v,
							/*sline_aw=*/spr.active_widths[v],
							/*pem=*/em,
							/*bccount=*/accumulated_bcycles
						)
					);

					span_pos = -1;
					accumulated_words = 0;
					nonzero_words = 0;
					accumulated_bcycles = 0;

					last_word = 0;
				}
			}
		}
	}

	// preview

	printf("\n");

	// all lines in a pattern
	int v = 0;
	for (auto &ln : sa.span_vpattern_)
	{
		int wx = 0;

		printf("AW:%d ", spr.active_widths[v++]);

		// all spans in a line
		for (auto &span : ln)
		{
			// offset
			{
				for (; wx < span.ws; wx++)
				{
					printf(".");
					printf(" ");
					for (int p = 0; p < 16; p++)
						printf("-");
					printf(" ");
				}
			}

			// words
			if (span.ww == 0)
			{
				printf("error!\n"); exit(1);
			}

			bool span_start = true;
			int lasti = span.ww - 1;
			for (int wi = 0; wi <= lasti; wi++, wx++)
			{
				u16 maskword = span.em[1];

				if (wi == 0)
					 maskword = span.em[0];
				else
				if (wi == lasti)
					 maskword = span.em[2];

				if (span_start)
					printf("%d", span.ww);
				else
					printf(".");

				if (span.cxs && (wi == 0))
					printf("!");
				else
					printf(":");

				if (span_start)
					span_start = false;

				for (int p = 0; p < 16; p++)
				{
					if (maskword & 0x8000)
						printf("1");
					else
						printf("0");

					maskword <<= 1;
				}

				if (span.cxe && (wi == lasti))
					printf("!");
				else
					printf(":");
			}

		}
		printf("\n");
	}

	printf("...\n");

	std::map<uint64_t, std::vector<constrained_wordspan*>> coherent_wordspans;

	v = 0;
	for (auto &ln : sa.span_vpattern_)
	{
		for (auto &span : ln)
		{
			uint64_t coherence_criteria =
/*				(uint64_t(span.em[1]) << (64-16)) |
				(uint64_t(span.em[2]) << (64-32)) |
				(uint64_t(span.em[0]) << (64-48)) |
				//(span.ww << 8) |
				(span.ws << 0)
*/
				(span.sline << 8) |
				(span.ws << 0)
			;

			coherent_wordspans[coherence_criteria].push_back(&span);

			int e = 0;
			for (; e < 3; e++)
			{
				printf(":");

				u16 maskword = span.em[e];

				if (maskword == 0)
				{
					printf("                ");
				}
				else
				{
					for (int p = 0; p < 16; p++)
					{
						if (maskword & 0x8000)
							printf("1");
						else
							printf("0");

						maskword <<= 1;
					}
				}
				printf(":");
			}
			printf("\n");
		}
	}

	printf("...\n");

	for (auto &ce : coherent_wordspans)
	{
		auto &spanvec = ce.second;

		for (constrained_wordspan* pspan : spanvec)
		{
			auto &span = *pspan;

			int e = 0;
			for (; e < 3; e++)
			{
				printf(":");

				u16 maskword = span.em[e];

				if (maskword == 0)
				{
					printf("                ");
				}
				else
				{
					for (int p = 0; p < 16; p++)
					{
						if (maskword & 0x8000)
							printf("1");
						else
							printf("0");

						maskword <<= 1;
					}
				}
				printf(":");
			}
			printf("\n");
		}
		printf("\n");
	}

}

void generate_emx2_active_linewidths(spriteframe_t &spr)
{
	spr.active_starts.clear();
	spr.active_starts.resize(spr.h_, 0);
	spr.active_widths.clear();
	spr.active_widths.resize(spr.h_, 0);

	// process only preshift[0] to establish source colour active widths
	{
		std::vector<preshiftlist_t>::iterator pmit = spr.psmask_frames_.begin();

		// compute active mask widths
		{
			preshiftlist_t::iterator cit = pmit->begin();
			preshift_t &shift = *cit;

			for (int v = 0; v < spr.h_; v++)
			{
				// find start
				int w = 0;
				int start = -1;
				while (w < shift.wordwidth_)
				{
					// fetch next word
					u16 *pline = &shift.data_[(v * shift.wordwidth_)];
					u16 word = pline[w];
					int word_pos = w++;

					if (word != 0)
					{
						start = word_pos;
						break;
					}
				}

				// find end
				w = shift.wordwidth_ - 1;
				int end = -1;
				while (w >= 0)
				{
					// fetch next word
					u16 *pline = &shift.data_[(v * shift.wordwidth_)];
					u16 word = pline[w];
					int word_pos = w--;

					if (word != 0)
					{
						end = word_pos;
						break;
					}
				}

				if ((start >= 0) && (end >= 0))
				{
					spr.active_starts[v] = start;
					spr.active_widths[v] = (end + 1) - start;
				}
			}
		}
	}
}

int generate_emx2_emdata_scoring(Context &context)
{
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

	return assigned_registers;
}

void generate_emx2_common_lineorder(spriteframe_t &spr, std::vector<int> &best_pattern, std::vector<int> &best_invpattern)
{
	static const int c_max_line_spans = 320 / 48;

	// default line order

	best_pattern.clear();
	best_invpattern.clear();
	for (int f = 0; f < spr.h_; f++)
	{
		best_pattern.push_back(f);
	}

	ubyte_t *emptylines = new ubyte_t[spr.h_];
	memset(emptylines, 0, spr.h_);

	ubyte_t *livelines = new ubyte_t[spr.h_];
	memset(livelines, 0, spr.h_);

	// find common optimized line order
	if (0 && USE_COMMON_LINEORDER_OPTIMIZATION && (spr.h_ > 2))
	{
		if (g_allargs.verbose)
			std::cout << "optimizing common line order across all preshifts..." << std::endl << std::flush;

		int shiftcount = spr.psmask_frames_.size();

		// make copy of data in terms of 3*y EM words
		uword_t *buffer = new uword_t[3 * spr.h_ * shiftcount * c_max_line_spans];
		memset(buffer, 0, sizeof(uword_t) * 3 * spr.h_ * shiftcount * c_max_line_spans);

		int max_ci = 0;
		{
			int si = 0;
			std::vector<preshiftlist_t>::iterator pmit = spr.psmask_frames_.begin();
			for (; pmit != spr.psmask_frames_.end(); pmit++, si++)
			{
				// components within a preshift
				//int ci = 0;
				//				for (preshiftlist_t::iterator cit = pmit->begin(); cit != pmit->end(); cit++, ci++)
				preshiftlist_t::iterator cit = pmit->begin();
				{
					preshift_t &shift = *cit;

					static const int pagesize = (3 * spr.h_);

					for (int v = 0; v < spr.h_; v++)
					{
						span_hseq_t &spanlist = shift.sa.span_vpattern_[v];

						int ci = -1;
						for (auto e : spanlist)
						{
							ci++;
							if (ci > max_ci)
								max_ci = ci;

							constrained_wordspan & span = e;

							int page = ((ci * shiftcount) + si) * pagesize;
							uword_t *sbuffer = &buffer[page];

							sbuffer[(v * 3) + 0] = span.em[0];
							sbuffer[(v * 3) + 1] = span.em[1];
							sbuffer[(v * 3) + 2] = span.em[2];

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
		}

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
			std::vector<std::pair<ulong_t, int>> sorting;
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
				int ordering = v;
				int orig_y = best_pattern[ordering];
				uword_t *line = &spr.mask_[(orig_y * full_src_wordwidth)];

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
				for (int x = src_pixwidth - 1; x >= 0; x--)
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

		if (g_allargs.verbose)
		{
			printf("live lines = %d\n", live_line_count);
			//printf("partial lines = %d\n", partial_line_count);
			printf("empty lines = %d\n", empty_line_count);
		}



		// can optimize ordering here




		// show reordering
		{
			int full_src_wordwidth = ((spr.w_ + 15) & -16) >> 4;
			int src_pixwidth = full_src_wordwidth << 4;

			for (int v = 0; v < live_line_count; v++)
			{
				int order = v;
				int orig_y = best_pattern[order];
				uword_t *line = &spr.mask_[(orig_y * full_src_wordwidth)];

				for (int w = 0; w < full_src_wordwidth; w++)
				{
					u16 maskword = ~line[w];

					printf(":");
					if (maskword == 0)
					{
						printf("                ");
					}
					else
					{
						for (int p = 0; p < 16; p++)
						{
							if (maskword & 0x8000)
								printf("1");
							else
								printf("0");

							maskword <<= 1;
						}
					}
					printf(":");
				}
				printf("\n");
			}
		}

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

		// reconstruct order
		{
			int full_src_wordwidth = ((spr.w_ + 15) & -16) >> 4;
			int src_pixwidth = full_src_wordwidth << 4;

			for (int v = 0; v < spr.h_; v++)
			{
				int orig_y = v;

				if (orig_y >= best_invpattern.size())
				{
					printf(" ...\n");
				}
				else
				{
					int order = best_invpattern[orig_y];

					int src_y = best_pattern[order];

					uword_t *line = &spr.mask_[(src_y * full_src_wordwidth)];

					for (int w = 0; w < full_src_wordwidth; w++)
					{
						u16 maskword = ~line[w];

						printf(":");
						if (maskword == 0)
						{
							printf("                ");
						}
						else
						{
							for (int p = 0; p < 16; p++)
							{
								if (maskword & 0x8000)
									printf("1");
								else
									printf("0");

								maskword <<= 1;
							}
						}
						printf(":");
					}
				}
				printf("\n");
			}
		}

		//

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

		printf("...\n");

		delete[] buffer; buffer = 0;
	}
	else
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
}

void generate_emx2_emdata_metrics
(
	Context &context, 
	const std::vector<constrained_wordspan*> &stacked_spans,
	ubyte_t *changes,
	uword_t *buffer,
	std::vector<constrained_wordspan*> &active_spans
)
{
	if (!g_allargs.quickcut)
		if (g_allargs.verbose)
			printf("\nsurviving datawords...\n");

	// reset dirty EM registers for this blit width
	//em_dirty[0] = true;
	//em_dirty[1] = (shift.wordwidth_ > 2);
	//em_dirty[2] = (shift.wordwidth_ > 1);

	//		int unique_datawords = 0; // each unique dataword has a uid so it can be reused
	{
		int v = -1;
		for (auto pspan : stacked_spans)
		{
			v++;
			int dsty = v;

			uword_t *srcdata = &buffer[dsty * 3];
			ubyte_t *linechanges = &changes[dsty * 3];
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
					active_spans.push_back(pspan);
				//	active_lines.push_back(lineout(v, dsty));

				if (!g_allargs.quickcut)
					if (g_allargs.verbose)
						printf("\n");
			}
		}
	}

	if (!g_allargs.quickcut)
		if (g_allargs.verbose)
			printf("total surviving = %d\n", context.metrics.size());
}

void generate_emx2_extract_deltas(std::vector<constrained_wordspan*> &stacked_spans, ubyte_t *changes, uword_t *buffer)
{
	// generate map for only changing EM registers 
	// (excluding first output line - this is handled as dirty registers to minimize initial setup if the first line is empty)

	if (USE_DELTA_OPTIMIZATION)
	{
		uword_t lastchanges[3];
		bool dirtyline = true;

		//printf("shift wordwidth: %d", shift.wordwidth_);

		int v = -1;
		for (auto pspan : stacked_spans)
		{
			v++;
			int dsty = v;
			//int dsty = best_pattern[v];

			// generate deltas for all but 1st BUSY line
			int yn3 = dsty * 3;
			int yp3 = yn3;
			if (v > 0)
				yp3 = (v - 1) * 3;// best_pattern[v - 1] * 3;

			// generate deltas for only real lines
			{
				if (1)
					changes[yn3 + 0] = (dirtyline || (lastchanges[0] != buffer[yn3 + 0])) ? 1 : 0;
				if (pspan->ww >= 3)
					changes[yn3 + 1] = (dirtyline || (lastchanges[1] != buffer[yn3 + 1])) ? 1 : 0;
				if (pspan->ww >= 2)
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
		int v = -1;
		for (auto pspan : stacked_spans)
		{
			v++;
			int dsty = v;
			//int dsty = best_pattern[v];

			//if (component_livelines[dsty])
			{
				int dsty3 = dsty * 3;
				changes[dsty3 + 0] = true;
				changes[dsty3 + 1] = (pspan->ww >= 3);
				changes[dsty3 + 2] = (pspan->ww >= 2);
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

	//			for (int v = 0; v < shift.height_; v++)
	//			{
	//				int dsty3 = best_pattern[v] * 3;

	int v = -1;
	for (auto pspan : stacked_spans)
	{
		v++;
		int dsty3 = v * 3;

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
}

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

void generate_emx2_store_dependencies
(
	Context &context, 
	ubyte_t *changes,
	uword_t *buffer,
	int assigned_registers,
	const std::vector<constrained_wordspan*> &active_spans,
	std::vector<storedependency>& deps, 
	std::vector<u64>& lifespans
)
{
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
	//			for (std::vector<lineout>::iterator lit = active_lines.begin(); lit != active_lines.end(); lit++, job++)
	for (auto pspan : active_spans)
	{
		//				lineout &lo = *lit;
		//				int dsty = lo.dline;
		int dsty = pspan->draw_order;

		// only live lines in active_lines table

		uword_t *srcdata = &buffer[dsty * 3];
		ubyte_t *linechanges = &changes[dsty * 3];

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
}

void generate_emx2_metaprogram(spriteframe_t &spr, int _frame)
{
	// generate metrics for each data word
	int pi = 0;
	for (std::vector<preshiftlist_t>::iterator pmit = spr.psmask_frames_.begin(); pmit != spr.psmask_frames_.end(); pmit++, pi++)
	{
		//for (preshiftlist_t::iterator pit = pmit->begin(); pit != pmit->end(); pit++)
		preshiftlist_t::iterator pit = pmit->begin();
		{
			preshift_t &shift = *pit;

			if ((DEBUG_PRESHIFT >= 0) && (pi != DEBUG_PRESHIFT))
				continue;

			// stack all spans for state change optimization
			std::map<uint32_t, std::vector<constrained_wordspan*>> ordered_spans;
			for (auto & spanlist : shift.sa.span_vpattern_)
			{
				int horizontal_index = 0;
				for (auto &span : spanlist)
				{
					span.reordered_sline = spr.invlineorder[span.sline];

					// span index has priority so spans rendered vertically for edge coherence, never horizontally
					// otherwise, frame common lineorder prevails
					uint32_t ordering_key =
						(uint32_t(horizontal_index) << 16) |
						(uint32_t(span.reordered_sline) << 0)
						;

					ordered_spans[ordering_key].push_back(&span);

					horizontal_index++;
				}
			}

			std::vector<constrained_wordspan*> stacked_spans;
			int o = 0;
			for (auto & group : ordered_spans)
			{
				for (auto pspan : group.second)
				{
					pspan->draw_order = o++;
					stacked_spans.push_back(pspan);
				}
			}
					
			size_t spancount = stacked_spans.size();

			//if (g_allargs.verbose)
			{
				printf("\n\nframe %d spans %d preview:\n", _frame, pi);
				for (auto pspan : stacked_spans)
				{
					print_dataword(pspan->em[0]);
					print_dataword(pspan->em[1]);
					print_dataword(pspan->em[2]);
					printf("\n");
				}
				printf("\n");
			}

			Context context
			(
				/*masksrcww=*/shift.wordwidth_, 
				/*masksrch=*/shift.height_, 
				/*srccol_wordwidth*/((spr.w_ + 15) >> 4), 
				/*simdstww=*/shift.wordwidth_, 
				/*guardxs=*/g_allargs.emguardx
			);


			ubyte_t *changes = new ubyte_t[3 * spancount/*shift.height_*/];
			memset(changes, 0, 3 * spancount/*shift.height_*/);

			// make copy of data in terms of 3*y EM words
			uword_t *buffer = new uword_t[3 * spancount/*shift.height_*/];
			memset(buffer, 0, 2 * 3 * spancount/*shift.height_*/);
			{
				for (auto pspan : stacked_spans)
				{
					int v = pspan->draw_order;
					buffer[(v * 3) + 0] = pspan->em[0];
					buffer[(v * 3) + 1] = pspan->em[1];
					buffer[(v * 3) + 2] = pspan->em[2];
				}
			}

			ubyte_t *component_livelines = new ubyte_t[shift.height_];
			memset(component_livelines, 0, shift.height_);

			// find nonempty lines
			for (int y = 0; y < shift.height_; y++)
			{
				int dsty = spr.invlineorder[y];

				if (USE_EMPTYLINE_OPTIMIZATION)
				{
					uword_t *srcdata = &buffer[dsty * 3];

					// look for empty blits (all EMs == 0)
					u64 line = 0;
					for (int x = 0; x < shift.wordwidth_; x++)
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

			generate_emx2_extract_deltas(stacked_spans, changes, buffer);


/*

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
*/

			std::vector<constrained_wordspan*> active_spans;
			generate_emx2_emdata_metrics(context, stacked_spans, changes, buffer, active_spans);

			int assigned_registers = generate_emx2_emdata_scoring(context);


			// preload most useful values

			//context

			// data dependencies
			// process all *changing words* on *changing lines* in *optimized line order*


			std::vector<storedependency> deps;
			std::vector<u64> lifespans;

			generate_emx2_store_dependencies
			(
				context, 
				changes,
				buffer,
				assigned_registers,
				active_spans,
				deps, 
				lifespans
			);



#if 0

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
#endif


			// (8 + 2) - (shift.wordwidth_ << 3);
#if 0
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
#endif

			//__debugbreak();
			context.reset_sim(true);

			// reset dirty EM registers for this blit width
			//em_dirty[0] = true;
			//em_dirty[1] = (shift.wordwidth_ > 2);
			//em_dirty[2] = (shift.wordwidth_ > 1);

			bool NFSR = false;
				//(shift.wordwidth_ > 1) && // NFSR is meaningless for wordwidth==1
				//(shift.wordsremaining_ == 0) &&
				//(((spr.w_ + 15) >> 4) != (shift.wordwidth_ + shift.wordoffset_));

			bool FISR = false;// (shift.wordoffset_ > 0);

			u8 current_skew =
				//false;// ((shift.wordwidth_ << 4) == ((spr.w_ + 15) & -16));
				(shift.shift_ & 15) |
				(NFSR ? 0x40 : 0x00) |
				(FISR ? 0x80 : 0x00);

			s16 current_skew_syinc_adj =
				(NFSR ? 1 : 0) -
				(FISR ? 1 : 0);

			s16 current_skew_syoff_adj =
				0 -
				(FISR ? 1 : 0);

			// SYINC = difference between source wordwidth & preshift wordwidth, + 2
			// note: compensate for NFSR!
			s16 current_syinc =
				(((((spr.w_ + 15) >> 4) - shift.wordwidth_) << 1) + 2)
				+ (current_skew_syinc_adj * 2);
//				+ (NFSR ? 2 : 0)
//				- (FISR ? 2 : 0);

			s16 current_dyinc = (8 + 2) - (shift.wordwidth_ << 3);

			context.blitter.skew = current_skew;
			context.blitter.syinc = current_syinc;
			context.blitter.dyinc = current_dyinc;

			// generate operations
			int current_dline = 0;
			int current_dline_wordoffset = 0;
			int current_sline = 0;
			int current_sline_wordoffset = 0;
			int current_ww = context.blitter.xcount;

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
			

//				int src_lines_skipped = 0;

//				int curr_line_words = shift.wordwidth_;

//				linemap lines;
//				for (int y = 0; y < shift.height_; y++)
//				{
//					int dsty = best_pattern[y];

				for (auto pspan : stacked_spans)
				{
					//__debugbreak();

					int span_draw_order = pspan->draw_order;
					int span_orig_sline = pspan->sline;
					int span_source_sline = pspan->reordered_sline;
					int span_dline = span_orig_sline;
					int span_syi_change = 0;

					int dsty = span_draw_order;
					uword_t *srcdata = &buffer[dsty * 3];
					ubyte_t *linechanges = &changes[dsty * 3];

					// FISR is set when span begins with colour data from previous a word 
					bool span_FISR = pspan->cxs;

					// NFSR can be set on FINAL spans where scrolled width exceeds defined source colour width
					bool span_NFSR = pspan->cxe;
//						(shift.wordwidth_ > 1) && // NFSR is meaningless for wordwidth==1
//						(shift.wordsremaining_ == 0) &&
//						(((spr.w_ + 15) >> 4) != (shift.wordwidth_ + shift.wordoffset_));

					u8 span_skew =
						(shift.shift_ & 15) |
						(span_NFSR ? 0x40 : 0x00) |
						(span_FISR ? 0x80 : 0x00);

					// SYINC = difference between source wordwidth & preshift wordwidth, + 2
					// note: compensate for NFSR/FISR!
					s16 span_skew_syinc_adj =
						(span_NFSR ? 1 : 0) -
						(span_FISR ? 1 : 0);

					s16 span_skew_syoff_adj =
						0 -
						(span_FISR ? 1 : 0);

					int span_ww_change = pspan->ww - current_ww;

					if (span_ww_change)
					{
						context.operations.push_back(new BLiT_XCMod(context, span_ww_change));
						context.operations.back()->execute();
						context.operations.push_back(new BLiT_DYIMod(context, -span_ww_change));
						context.operations.back()->execute();

						span_syi_change -= span_ww_change;

						current_ww += span_ww_change;
					}

					// emit SYI changes
					{
						// incorporate SKEW FISR/NFSR SYI changes
						s16 span_skew_syinc_change = span_skew_syinc_adj - current_skew_syinc_adj;
						current_skew_syinc_adj = span_skew_syinc_adj;
						span_syi_change += span_skew_syinc_change;

						if (span_syi_change)
						{
							context.operations.push_back(new BLiT_SYIMod(context, span_syi_change));
							context.operations.back()->execute();
						}
					}

					// emit SKEW changes
					{
						u8 span_skew_change = current_skew ^ span_skew;
						current_skew = span_skew;

						if (span_skew_change)
						{
							context.operations.push_back(new BLiT_Skew(context, span_skew));
							context.operations.back()->execute();
						}
					}

					// determine true [offset,width] for this line only
					{
						// source hop +/-
						int src_linejump = span_source_sline - current_sline;
						current_sline = span_source_sline;

						int src_wordjump = pspan->ws - current_sline_wordoffset;
						current_sline_wordoffset = pspan->ws;

						// incorporate FISR adjust
						src_wordjump += span_skew_syoff_adj - current_skew_syoff_adj;
						current_skew_syoff_adj = span_skew_syoff_adj;

						if (src_linejump || src_wordjump)
						{
							context.operations.push_back(new SRC_Skip(context, src_wordjump, src_linejump));
							context.operations.back()->execute();

							//src_lines_skipped += src_linejump;
						}

						// destination hop +/-
						int dst_linejump = span_dline - current_dline;
						int dst_wordjump = pspan->ws - current_dline_wordoffset;

						//__debugbreak();

						if (dst_linejump || dst_wordjump)
						{
							// caution: can't use LEA for jumps > +/-32k
							int addressjump = (dst_linejump * context.dstlinewid) + (dst_wordjump*8);
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
										context.operations.push_back(new DST_AddaR16(context, reg_src, context.SCRLINE_reg, dst_wordjump, dst_linejump));
									}
									else
									{
										// for -ve steps, look for +ve equivalent register and apply SUB operation
										// only allocates register if value does not pre-exist 
										ds = context.sim_resolve_data16(uword_t(-addressjump), reg_src, true);
										if (ds == DataSource_RegisterL)
										{
											context.operations.push_back(new DST_SubaR16(context, reg_src, context.SCRLINE_reg, dst_wordjump, dst_linejump));
										}
										else
										{
											// pointless allocation - fall back to LEA
											context.operations.push_back(new DST_Lea(context, context.SCRLINE_reg, context.SCRLINE_reg, dst_wordjump, dst_linejump));
										}
									}
								}
								else
								{
									context.operations.push_back(new DST_Lea(context, context.SCRLINE_reg, context.SCRLINE_reg, dst_wordjump, dst_linejump));
								}
							}
							else
							{
								context.operations.push_back(new DST_AddaIL(context, context.SCRLINE_reg, dst_wordjump, dst_linejump));
							}
							context.operations.back()->execute();

							current_dline = span_dline;
							current_dline_wordoffset = pspan->ws;
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
						context.operations.push_back(new BLiT_Go(context, context.BHOG_reg, span_orig_sline, span_dline, span_draw_order));
						context.operations.back()->execute();

						// SYI accounts for this automatically, except for real skips
						current_sline++;

						// next linejump+blit will be relative to this one (and not an empty one)
						//span_dline = dsty;
					}
					//else
					//{
					//	src_lines_skipped++;
					//}
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

#if 0
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
#endif

			if (!g_allargs.quickcut && g_allargs.verbose)
				std::cout << "preshift " << pi << " codegen optimization transforms: " << xform_passes << std::endl;
		
			//
			run_sim(context, component_livelines, _frame, pi, shift, spr.invlineorder, /*generate=*/true);



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

			//delete[] component_livelines; component_livelines = 0;
//#endif

		} // components
	} // preshifts

//	delete[] livelines; livelines = 0;
	// generate the generator
}

void generate_emx2_frame(spriteframe_t &spr, int _frame)
{
	if (g_allargs.verbose)
		std::cout << "frame [" << _frame << "]" << std::endl << std::flush;

	// find min source colour spans
	generate_emx2_active_linewidths(spr);

	// analyse all preshifts within this frame
	{
		std::vector<preshiftlist_t>::iterator pmit = spr.psmask_frames_.begin();

		int psi = 0;
		for (; pmit != spr.psmask_frames_.end(); pmit++, psi++)
		{
			// first component within a preshift
			preshiftlist_t::iterator cit = pmit->begin();
			{
				preshift_t &shift = *cit;

				generate_emx2_preshift_analysis(spr, _frame, shift, psi);
			}
		}
	}

	// find common line order
	{
		std::vector<int> best_pattern;
		std::vector<int> best_invpattern;

		generate_emx2_common_lineorder(spr, best_pattern, best_invpattern);

		spr.lineorder = best_pattern;
		spr.invlineorder = best_invpattern;
	}

	generate_emx2_metaprogram(spr, _frame);

	//
}


bool generate_emx2_sequence()
{
	std::cout << "code-generating EMX frames: " << std::endl;

	// individual sprites follow
	spritelist_t::iterator it = g_spriteframes.begin();

	int f = 0;
	for (; it != g_spriteframes.end(); it++, f++)
	{
		if ((DEBUG_FRAME >= 0) && (f != DEBUG_FRAME))
			continue;
		generate_emx2_frame(*it, f);
	}

	return true;
}
