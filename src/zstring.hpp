#ifndef ZSTRING_HPP
#define ZSTRING_HPP

#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <sys/types.h>
#include <regex.h>

#include "rs.hpp"


#	define ESC_GREN "\x1B[32m"
#	define ESC_BOLD "\x1B[1m"
#	define ESC_YELLOW "\x1B[33;1m"
#	define ESC_NORMAL "\x1B[0m"
#	define ESC_BBLUE "\x1B[44m"
#	define ESC_BVIOLET "\x1B[45m"
#	define ESC_VIOLET "\x1B[35m"
#	define ESC_BRED "\x1B[41m"
#	define ESC_RED "\x1B[31m"

#define ZDEBUG
class ZString
{
	private:
		static int zs_count;
		static char* zs_buf[];
		static int zs_buf_count;
		static regex_t *preg;
	protected:
		class Str
		{
			private:
				Str() {}
				Str(const Str &) {}
				const Str &operator=(const Str &) { return *this; }
				static int s_count;
			public:
				uint8_t *mem;
				size_t lenght;
				int links;
				Str(size_t l);
				virtual ~Str();
		};
		Str *str;
		size_t len;
	public:
		ZString() : str(NULL), len(0) { zs_count++; }
		ZString(const char *s);
		ZString(const ZString &y) : str(y.str), len(y.len)
		{
			zs_count++;
			if(str)
				str->links++;
			//printf("ZString::ZString(const ZString& %p) this=%p\n", &y, this);
		}
		const ZString &operator=(const ZString &y)
		{
			if(this == &y)	return *this;// x=x
			unset();
			str = y.str;
			len = y.len;
			if(str)
				str->links++;
			return *this;
		}
		bool operator==(const ZString &y)	const
		{
			if(!str && !y.str)	return true;// "" == ""
			if(!str || !y.str)	return false;// "x" != "" || "" != "y"
			if(len != y.len)	return false;
			return memcmp(str->mem, y.str->mem, len)==0;
		}
		bool operator!=(const ZString &y)	const
		{ return !operator==(y); }
		const char *get() const;
#		ifdef ZDEBUG
		const char *debug_get() const;
#		endif
		size_t get_len() const
		{	return len; }
		void unset()
		{
			if(!str)	return;
			if(str->links > 1)
				str->links--;
			else
				delete str;
			str = NULL;
		}
		virtual void reset();
		const ZString &operator+=(uint8_t b);
		void append(const uint8_t *buf, size_t l);
		const ZString &operator+=(const ZString &y);
		const ZString operator+(const ZString &y) const
		{
			ZString z(*this);
			z += y;
			return z;
		}
		uint8_t operator[](size_t i) const
		{
			assert(str!=0 && i<len);
			return str->mem[i];
		}
		const uint8_t *get_raw_data() const
		{
			if(!str)	return NULL;
			return str->mem;
		}
		int compare(const ZString &y) const
		{//TODO
			return 0;
		}
		// compile a regular expression
		// default cflags=REG_EXTENDED
		static bool regcomp(const ZString &regex, int cflags=REG_EXTENDED);
		// Attempt  to  match  regexp against the pattern space.  If successful, replace that portion matched with replace-
		// ment.  The replacement may contain the special character & to refer to that portion of the pattern  space  which
		// matched,  and  the  special  escapes \1 through \9 to refer to the corresponding matching sub-expressions in the
		// regexp.
		// (see also sed(1))
		bool regsub(const ZString &replacement, int eflags=0);
		static const ZString convert_from_uints(const uint8_t *mb, size_t l);
		enum ECF{
			CF_DEC=0x000,// decimal
			CF_HEX=0x100,// hexadecimal
			CF_ZF=0x200,// zero filed
			CF_PF=0x400,// postfix (e.g. K=1024, M=1048576, k=1000, and so on)
			CF_L8=8// lengit 8 digits
		};
		static const ZString convert_from_int(uint64_t x, int cflags=CF_DEC);
		uint64_t convert_to_uint64(int cflags=CF_DEC) const;
		virtual ~ZString();
};

#endif
