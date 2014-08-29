#include "zstring.hpp"
#include "getopt.hpp"
#include <stdio.h>

#define ZS_MEM_INIT 4

ZString::Str::Str(size_t l) : links(1)
{
	s_count++;
	for(lenght=ZS_MEM_INIT; lenght<l; lenght*=2);
	mem = new uint8_t[lenght];
#	ifdef ZDEBUG
	memset(mem, '`', lenght);
#	endif
}

ZString::Str::~Str()
{
	assert(s_count!=0);
	s_count--;
	//if(s_count == 0) printf("ZString::Str::~Str() s_count=0\n");
	delete[] mem;
}

#define ZS_N 20

int ZString::zs_count = 0;
char *ZString::zs_buf[ZS_N];
int ZString::zs_buf_count = 0;
int ZString::Str::s_count = 0;

const char *ZString::get() const
{
	zs_buf_count = zs_buf_count==0? ZS_N-1: zs_buf_count-1;
	//printf("*%d*", zs_buf_count);
	delete[] zs_buf[zs_buf_count];
	if(str)
	{
		zs_buf[zs_buf_count] = new char[len+1];
		memcpy(zs_buf[zs_buf_count], str->mem, len);
		zs_buf[zs_buf_count][len] = '\0';
	}
	else
	{
		zs_buf[zs_buf_count] = new char[1];
		zs_buf[zs_buf_count][0] = '\0';
	}
	return zs_buf[zs_buf_count];
}

#ifdef ZDEBUG
#define Z_BUF_LEN 4096
const char *ZString::debug_get() const
{
	zs_buf_count = zs_buf_count==0? ZS_N-1: zs_buf_count-1;
	delete[] zs_buf[zs_buf_count];
	zs_buf[zs_buf_count] = new char[Z_BUF_LEN];
	int c=0;
	if(str)
	{
		c += sprintf(zs_buf[zs_buf_count]+c,
				"%s%p%s [%u] %s%p/%p[%u](%d)%s '",
				ESC_GREN, this, ESC_NORMAL, (uint32_t)len,
				ESC_RED, str, str->mem, (uint32_t)str->lenght, str->links, ESC_NORMAL);
		size_t j;
		for(j=0; j<str->lenght; j++)
		{
			if(j>=len)
				c += sprintf(zs_buf[zs_buf_count]+c, "%s", ESC_BBLUE);
			if(str->mem[j]<0x20U)
				c += sprintf(zs_buf[zs_buf_count]+c, "·");
			else if(str->mem[j]>=0x80U)
				c += sprintf(zs_buf[zs_buf_count]+c, "☣");
			else
				c += sprintf(zs_buf[zs_buf_count]+c, "%c", str->mem[j]);
		}
		c += sprintf(zs_buf[zs_buf_count]+c, "%s'", ESC_NORMAL);
	}
	else
	{
		sprintf(zs_buf[zs_buf_count], "%s%p%s [%u] %sNULL%s",
				ESC_GREN, this, ESC_NORMAL, (uint32_t)len, ESC_RED, ESC_NORMAL);
	}
	return zs_buf[zs_buf_count];
}
#endif

ZString::ZString(const char *s) : str(NULL)
{
	zs_count++;
	//printf("ZString::ZString('%s') this=%p, zs_count=%d\n", s, this, zs_count);
	len = strlen(s);
	if(len == 0) return;
	str = new Str(len+1);
	memcpy(str->mem, s, len+1);
	//printf("str='%s', links=%d\n", str, *links);
}

void ZString::reset()
{
	if(!str)	return;
	if(str->links>1)
	{
		str->links--;
		str = NULL;
	}
	len = 0;
}

const ZString &ZString::operator+=(uint8_t b)
{
	if(!str || str->links>1 || len>=str->lenght)
	{
		//printf("ZString::operator+=('%c') create [%lu+1]\n", b, len);
		Str *str2 = new Str(len+1);
		if(str)
			memcpy(str2->mem, str->mem, len);
		str2->mem[len] = b;
		size_t l = len;
		unset();
		str = str2;
		len = l+1;
		return *this;
	}
	str->mem[len++] = b;
	return *this;
}

const ZString &ZString::operator+=(const ZString &y)
{
	if(!y.str)	return *this;// x += ""
	if(!str)
	{// "" += y
		operator=(y);
		return *this;
	}
	if(str->links>1 || len+y.len>=str->lenght)
	{
		Str *str2 = new Str(len+y.len);
		memcpy(str2->mem, str->mem, len);
		memcpy(str2->mem+len, y.str->mem, y.len);
		size_t l = len;
		unset();
		str = str2;
		len = l+y.len;
		return *this;
	}
	memcpy(str->mem+len, y.str->mem, y.len);
	len += y.len;
	return *this;
}

void ZString::append(const uint8_t *buf, size_t l)
{
	if(l==0)	return;
	if(!str || str->links>1 || len+l>str->lenght)
	{
		Str *str2 = new Str(len+l);
		if(str)
			memcpy(str2->mem, str->mem, len);
		memcpy(str2->mem+len, buf, l);
		l += len;
		unset();
		str = str2;
		len = l;
	}
	else
	{
		memcpy(str->mem+len, buf, l);
		len += l;
	}
}

#define ERRBUF_SIZE 2048
regex_t *ZString::preg = NULL;
/* static */ bool ZString::regcomp(const ZString &regex, int cflags)
{
	if(preg)
	{
		regfree(preg);
		delete preg;
	}
	preg = new regex_t;
	int errcode = ::regcomp(preg, regex.get(), cflags);
	if(errcode)
	{
		char errbuf[ERRBUF_SIZE];
		regerror(errcode, preg, errbuf, ERRBUF_SIZE);
		sprintf(errbuf+strlen(errbuf), " regex='%s'", regex.get());
		options->set_error(1, errbuf);
		return false;
	}
	return true;
}

#define REG_NMATCH 10
bool ZString::regsub(const ZString &replacement, int eflags)
{
	const char *s = get();
	regmatch_t pmatch[REG_NMATCH];
	//printf("ZString::regsub() replacement='%s', s='%s'\n", replacement.get(), s);
	int errcode = regexec(preg, s, REG_NMATCH, pmatch, eflags);
	if(errcode)	return false;// no match
	size_t j, k;
	ZString ss;
	uint8_t nb;
	//printf("pmatch[0].rm_so=%d, pmatch[0].rm_eo=%d\n", pmatch[0].rm_so, pmatch[0].rm_eo);
	//printf("pmatch[1].rm_so=%d, pmatch[1].rm_eo=%d\n", pmatch[1].rm_so, pmatch[1].rm_eo);
	for(k=0; k<(size_t)pmatch[0].rm_so; k++)
	{
		ss += s[k];
		//printf("s[%lu]-> ss=%s\n", k, ss.debug_get());
	}
	if(replacement.str)
	{
		for(j=0; j<replacement.len; j++)
		{
			if(replacement.str->mem[j]=='&')
			{// &
				for(k=pmatch[0].rm_so; k<(size_t)pmatch[0].rm_eo; k++)
				{
					ss += s[k];
					//printf("s[%lu]-> ss=%s\n", k, ss.debug_get());
				}
			}
			else if(replacement.str->mem[j]=='\\')
			{
				nb = j+1<replacement.len? replacement.str->mem[++j]: '\0';
				switch(nb)
				{
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						// \{0..9}
						nb -= '0';
						if(pmatch[nb].rm_so != -1)
							for(k=pmatch[nb].rm_so; k<(size_t)pmatch[nb].rm_eo; k++)
							{
								ss += s[k];
								//printf("s[%lu]-> ss=%s\n", k, ss.debug_get());
							}
						break;
						//TODO можно добавить \L и прочее как в sed(1)
					default:
						// \z
						ss += nb;
						//printf("nb-> ss=%s\n", ss.debug_get());
						break;
				}
			}
			else
			{
				ss += replacement.str->mem[j];
				//printf("rep->mem[%lu]-> ss=%s\n", j, ss.debug_get());
			}
		}
	}
	for(k=pmatch[0].rm_eo; s[k]!='\0'; k++)
	{
		ss += s[k];
		//printf("s[%lu]-> ss=%s\n", k, ss.debug_get());
	}
	operator=(ss);
	return true;
}

const ZString ZString::convert_from_uints(const uint8_t *mb, size_t l)
{// converting l digits to chars (5 bits to symbol: [0-9A-V])
	uint32_t bb = 0;
	uint32_t bc = 0;
	assert(l>0);
	ZString r;
	mb += l-1;
	do{
		bb |= *mb-- <<(24-bc);
		bc += 8;
		while(bc>=5)
		{
			uint8_t b = bb>>27;
			b += b<10? '0': 'A'-10;
			r += b;
			bb <<= 5;
			bc -= 5;
		}
	}while(--l);
	return r;
}

#define ZSCVB 256
const ZString ZString::convert_from_int(uint64_t x, int cflags)
{
	ZString s;
	char buf[ZSCVB];
	uint64_t M = cflags&CF_HEX? 16: 10;
	char* p = buf+ZSCVB-1;
	unsigned sc = cflags&0xFF;
	do{
		uint8_t c = x%M;
		x /= M;
		*p-- = c<10? c+'0': c-10+'A';
		if(sc)	--sc;
	}while(x || sc);
	while(++p < buf+ZSCVB)
		s += *p;
	return s;
}

uint64_t ZString::convert_to_uint64(int cflags) const
{
	assert(cflags==CF_DEC || cflags&CF_PF);
	uint64_t x = 0;
	if(!str || len==0)	return x;
	const uint8_t *p = str->mem;
	size_t l = len;
	do{
		if(*p<'0' || *p>'9')
		{
			if(x!=0 && cflags&CF_PF)
			{
				switch(*p)
				{
					case 'T':
						x *= 1024;
					case 'G':
						x *= 1024;
					case 'M':
						x *= 1024;
					case 'K':
						x *= 1024;
						p++, l--;
						break;
					case 't':
						x *= 1000;
					case 'g':
						x *= 1000;
					case 'm':
						x *= 1000;
					case 'k':
						x *= 1000;
						p++, l--;
						break;
					default:
						break;
				}
			}
			break;
		}
		x *= 10;
		x += *p++ - '0';
	}while(--l);
	return x;
}

ZString::~ZString()
{
	unset();
	if(--zs_count == 0)
	{
		for(zs_buf_count=0; zs_buf_count<ZS_N; zs_buf_count++)
		{
			delete[] zs_buf[zs_buf_count];
			zs_buf[zs_buf_count] = NULL;
		}
		if(preg)
		{
			regfree(preg);
			delete preg;
		}
		//printf("ZString::~ZString() zs_count=0\n");
	}
}

