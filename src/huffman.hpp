#ifndef HUFFMAN_HPP
#define HUFFMAN_HPP

#include <stdint.h>
#include "rs.hpp"
#include "zstring.hpp"

#define HUF_N_LONG_VALUE 0x1BBAA2U

//#define HDEBUG

class Huffman : public ZString
{
	private:
		const Huffman &operator=(const Huffman&) { return *this; }
		mutable RSD rsd_hold;
		mutable uint32_t bbuffer;
		mutable uint32_t bcount;
		mutable size_t bpos;
		bool last_pair_ic;// true if last pair incomplete
		RSD decode21(uint32_t value) const
		{//value == [0..1DDD51} return low, hi
			RSD rsd;
			uint64_t t = value;
			t *= 0x2ED84B;// 2^32/RS_M
			//printf("value=%05X t=%016lX", value, t);
			rsd_hold = t>>32;// hi part
			value = t;
			value >>= 11;
			value *= RS_M;
			//printf(" %09lX\n", (uint64_t)value<<3);
			rsd = (value>>21)+((value>>20)&1);// low part
			return rsd;
		}
		RSD decode21() const
		{// return hi part
			RSD rsd(rsd_hold);
			rsd_hold = -1;
			return rsd;
		}
		uint32_t encode21(const RSD& rsd0, const RSD& rsd1)
		{
			uint32_t value = rsd1.get();
			value *= RS_M;
			value += rsd0.get();
			return value;
		}
	protected:
	public:
		bool get_last_pair_ic() const { return last_pair_ic; }
		void encode(const RSD& rsd)
		{
#			ifdef HDEBUG
			printf("Huffman::encode(%s%p %s%s%s)",
					ESC_GREN, this,
					ESC_VIOLET, rsd.print(), ESC_NORMAL);
#			endif
			uint32_t value;
			if(rsd == -1)
			{// end encode
				if(rsd_hold != -1)
				{// save end low part
					last_pair_ic = true;
					value = encode21(rsd_hold, 0);// allways long value
					bbuffer |= value<<(11-bcount);
					bcount += 21;
#					ifdef HDEBUG
					printf(" v=%05X bb=%08X bc=%u", value, bbuffer, bcount);
#					endif
					while(bcount >= 8)
					{
#						ifdef HDEBUG
						printf(" %s%02X%s", ESC_GREN, (uint8_t)(bbuffer>>24), ESC_NORMAL);
#						endif
						operator+=((uint8_t)(bbuffer>>24));
						bcount -= 8;
						bbuffer <<= 8;
					}
				}
				if(bcount != 0)
				{
#					ifdef HDEBUG
					printf(" bb=%08X bc=%u %s%02X%s",
							bbuffer, bcount, ESC_GREN, (uint8_t)(bbuffer>>24), ESC_NORMAL);
#					endif
					operator+=((uint8_t)(bbuffer>>24));// save last bits
				}
#				ifdef HDEBUG
				printf(" exit line %d\n", __LINE__);
#				endif
				return;
			}
			if(rsd_hold == -1)
			{// save low part
				rsd_hold = rsd;
#				ifdef HDEBUG
				printf(" exit line %d\n", __LINE__);
#				endif
				return;
			}
			value = encode21(rsd_hold, rsd);
			rsd_hold = -1;
			if(value >= HUF_N_LONG_VALUE)
			{// short value (20 bits)
#				ifdef HDEBUG
				printf(" sv");
#				endif
				value -= HUF_N_LONG_VALUE/2;
				bbuffer |= value<<(12-bcount);
				bcount += 20;
			}
			else
			{// long value (21 bits)
#				ifdef HDEBUG
				printf(" lv");
#				endif
				bbuffer |= value<<(11-bcount);
				bcount += 21;
			}
#			ifdef HDEBUG
			printf(" v=%05X bb=%08X bc=%u", value, bbuffer, bcount);
#			endif
			while(bcount >= 8)
			{
#				ifdef HDEBUG
				printf(" %s%02X%s", ESC_GREN, (uint8_t)(bbuffer>>24), ESC_NORMAL);
#				endif
				operator+=((uint8_t)(bbuffer>>24));
				bcount -= 8;
				bbuffer <<= 8;
			}
#			ifdef HDEBUG
			printf(" bb=%08X bc=%u len=%lu exit line %d\n", bbuffer, bcount, len, __LINE__);
#			endif
		}
		void set_last_pair_ic(bool lpi) { last_pair_ic = lpi; }
		RSD decode() const
		{
#			ifdef HDEBUG
			printf("Huffman::decode()");
#			endif
			RSD rsd(rsd_hold);
			if(rsd != -1)
			{// if aviable hi part
				if(bpos>=len && last_pair_ic)
				{// last pair incomplete
#					ifdef HDEBUG
					printf(" %s%s%s exit %u\n", ESC_VIOLET, ((RSD)-1).print(), ESC_NORMAL, __LINE__);
#					endif
					return -1;
				}
				rsd_hold = -1;
#				ifdef HDEBUG
				printf(" %s%s%s exit %d\n", ESC_VIOLET, rsd.print(), ESC_NORMAL, __LINE__);
#				endif
				return rsd;// hi part
			}
			bool is_end_decode = (bpos>=len);
			if(is_end_decode && bcount==0)
			{
#				ifdef HDEBUG
				printf(" %s%s%s exit %d\n", ESC_VIOLET, ((RSD)-1).print(), ESC_NORMAL, __LINE__);
#				endif
				return -1;// end decode
			}
			uint32_t value;
#			ifdef HDEBUG
			printf(" |%s", ESC_GREN);
#			endif
			while(bcount < 21)
			{// load bits
				value = bpos<len? str->mem[bpos++]: 0;
#				ifdef HDEBUG
				printf(" %02X", value);
#				endif
				bbuffer <<= 8;
				bbuffer |= value;
				bcount += 8;
			}
#			ifdef HDEBUG
			printf("%s|", ESC_NORMAL);
			printf(" bb=%08X", bbuffer);
#			endif
			value = bbuffer>>(bcount-21);// long value
			if(value < HUF_N_LONG_VALUE)
			{// long value (21 bits)
#				ifdef HDEBUG
				printf(" lv");
#				endif
				bcount -= 21;
			}
			else
			{// short value
#				ifdef HDEBUG
				printf(" sv");
#				endif
				bcount -= 20;
				value = bbuffer>>bcount;
				value += HUF_N_LONG_VALUE/2;
			}
			if(is_end_decode)	bcount=0;
			bbuffer = bcount? bbuffer&((1U<<bcount)-1): 0;
#			ifdef HDEBUG
			printf(" v=%05X bb=%08X bc=%u bp=%lu l=%lu", value, bbuffer, bcount, bpos, len);
#			endif
			rsd = decode21(value);// low part
			rsd_hold = decode21();// hi part
#			ifdef HDEBUG
			printf(" %s%s%s exit %d\n", ESC_VIOLET, rsd.print(), ESC_NORMAL, __LINE__);
#			endif
			return rsd;
		}
		const Huffman &operator=(const char *s)
		{
			ZString::unset();
			ZString::operator=(s);
			rsd_hold = -1;
			bbuffer = 0;
			bcount = 0;
			bpos = 0;
			last_pair_ic = false;
			return *this;
		}
		Huffman() : rsd_hold(-1), bbuffer(0), bcount(0), bpos(0), last_pair_ic(false)
		{}
		Huffman(const ZString& y) : ZString(y), rsd_hold(-1), bbuffer(0), bcount(0), bpos(0), last_pair_ic(false)
		{}
		virtual void reset()
		{
			ZString::reset();
			rsd_hold = -1;
			bbuffer = 0;
			bcount = 0;
			bpos = 0;
			last_pair_ic = false;
		}
		Huffman(const Huffman& y) : ZString(y), rsd_hold(-1), bbuffer(0), bcount(0), bpos(0), last_pair_ic(false)
		{
			//printf("Huffman::Huffman(const Huffman&)\n");
		}
		virtual ~Huffman()
		{}
};

#endif
