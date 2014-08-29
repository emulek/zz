#ifndef RS_HPP
#define RS_HPP

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

//#define RS_M 2063
//#define RS_M 1543
#define RS_M 1399

#define RSDPRN 8
class RSD
{
	private:
		int d;
		static int alpha;// α
		static int pow_tbl[RS_M];
		static int log_tbl[RS_M];
		static void init();
	protected:
	public:
		RSD(const RSD &y) : d(y.d) {}
		const RSD &operator=(const RSD &y)
		{ d = y.d; return *this; }
		const char *print() const;
		bool operator==(const RSD &y) const	{ return d == y.d; }
		bool operator!=(const RSD &y) const	{ return d != y.d; }
		bool operator>=(const RSD &y) const	{ return d >= y.d; }
		const RSD &operator+=(const RSD &y)
		{
			d += y.d;
			if(d >= RS_M)	d -= RS_M;
			return *this;
		}
		const RSD &operator-=(const RSD &y)
		{
			d -= y.d;
			if(d < 0)	d += RS_M;
			return *this;
		}
		RSD operator+(const RSD &y) const
		{
			RSD z(*this);
			z += y;
			return z;
		}
		RSD operator-(const RSD &y) const
		{
			RSD z(*this);
			z -= y;
			return z;
		}
		RSD operator-() const
		{
			RSD z(0);
			if(d != 0)
				z.d = RS_M-d;
			return z;
		}
		const RSD &operator*=(const RSD &y)
		{
			if(d == 0 || y.d == 0)
			{
				d = 0;
				return *this;
			}
			if(y.d == 1)
				return *this;
			if(d == 1)
			{
				d = y.d;
				return *this;
			}
			int p = log_tbl[d] + log_tbl[y.d];
			d = pow_tbl[p<RS_M? p: p-RS_M+1];
			return *this;
		}
		RSD operator*(const RSD &y) const
		{
			RSD z(*this);
			z *= y;
			return z;
		}
		RSD operator^(int y) const
		{
			assert(d || y);// 0⁰
			int j;
			RSD z(1);
			for(j = 0; j < y; j++)
				z *= *this;
			return z;
		}
		const RSD &operator/=(const RSD &y)
		{
			assert(y.d); // x/0
			if(d == 0 || y.d == 1)
				return *this;
			int p = log_tbl[d] - log_tbl[y.d];
			d = pow_tbl[p>=0? p: p+RS_M-1];
			return *this;
		}
		RSD operator/(const RSD &y) const
		{
			assert(y.d);
			RSD z(*this);
			z /= y;
			return z;
		}
		const RSD &operator++()
		{
			if(++d >= RS_M)	d = 0;
			//printf("RSD::operator++() d=%d\n", d);
			return *this;
		}
		const RSD &operator--()
		{
			if(--d < 0)	d = RS_M-1;
			return *this;
		}
		int get() const { return d; }
		RSD() : d(0) { init(); }
		RSD(int i)
		{
			init();
			assert(i >= -1 && i < RS_M);
			d = i;
		}
		~RSD() {}
};

// число слов в RS коде.
#define RS_N 12

// число информационных символов
#define RS_K 4

// число проверочных символов
//#define RS_R (RS_N-RS_K)
#define RS_R 6


#endif
