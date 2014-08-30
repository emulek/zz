#ifndef MATRIX_HPP
#define MATRIX_HPP

#include "rnd.hpp"
//#include "drs66.hpp"
#include "rnd.hpp"
#include "rs.hpp"


class Matrix
{
	private:
		int m;// высота
		int n;// ширина
		RSD **a;
		int *links;
		void swap_lines(RSD **at, int j0, int j1)
		{// замена строк j0 ←→ j1
			//printf("Matrix::swap(%p, %d, %d)\n", at, j0, j1);
			RSD *t = at[j0];
			at[j0] = at[j1];
			at[j1] = t;
		}
		static RSD **inv;
		static int inv_n;
		static int matrix_count;
	protected:
	public:
		void release()
		{
			if(!a)	return;
			if(*links == 1)	return;
			(*links)--;
			links = new int(1);
			RSD **at = new RSD*[m];
			int j, k;
			for(j = 0; j < m; j++)
			{
				at[j] = new RSD[n];
				for(k = 0; k < n; k++)	at[j][k] = a[j][k];
			}
			a = at;
			matrix_count++;
			//printf("Matrix::release() this=%p, a=%p, links=%d, matrix_count=%d\n",
			//		this, a, a? *links: -1, matrix_count);
		}
		void set0()
		{
			int j, k;
			assert(a);
			release();
			for(j = 0; j < m; j++)
				for(k = 0; k < n; k++)
					a[j][k] = 0;
		}
		void set1()
		{
			int j, k;
			assert(a);
			assert(n == m);
			release();
			for(j = 0; j < m; j++)
				for(k = 0; k < n; k++)
					a[j][k] = j==k? 1: 0;
		}
		void set_elem(int y, int x, const RSD &elem)
		{
			assert(a);
			assert(x >= 0 && x < n && y >= 0 && y < m);
			release();
			a[y][x] = elem;
		}
		const RSD &get_elem(int y, int x) const
		{
			assert(x >= 0 && x < n && y >= 0 && y < m);
			return a[y][x];
		}
		void set_rnd();
		void print(bool) const;
		void print() const	{ print(false); }
		const Matrix &operator+=(const Matrix &y);
		const Matrix &operator*=(const Matrix &y);
		Matrix operator*(const Matrix &y) const;
		int inverse();
		Matrix operator~() const
		{
			assert(a);
			assert(n == m);
			Matrix z(*this);
			z.release();
			int r = z.inverse();
			assert(!r);
			int j, k;
			for(j = 0; j < m; j++)
				for(k = 0; k < n; k++)
					z.a[j][k] = inv[j][k];
			return z;
		}
		bool operator!=(int y) const
		{
			assert(a);
			assert(y == 0 || y == 1);
			int j, k;
			for(j = 0; j < m; j++)
				for(k = 0; k < n; k++)
				{
					if(a[j][k] != ((y && j==k)? 1: 0))
						return true;
				}
			return false;
		}
		const RSD &operator[](int i) const
		{
			assert(a);
			assert(m == 1 && i >= 0 && i < n);
			return a[0][i];
		}
		RSD &operator[](int i)
		{
			assert(a);
			assert(m == 1 && i >= 0 && i < n);
			release();
			return a[0][i];
		}
		void unset()
		{
			//printf("Matrix::unset() this=%p, a=%p, links=%d, matrix_count=%d\n",
			//		this, a, a? *links: -1, matrix_count);
			if(!a)	return;
			if(*links > 1)
			{
				(*links)--;
				return;
			}
			else
			{
				int j;
				for(j = 0; j < m; j++)
					delete[] a[j];
				delete[] a;
				delete links;
				if(--matrix_count == 0)
				{
					//printf("Matrix::matrix_count=0\n");
					if(inv)
					{
						for(j = 0; j < inv_n; j++)	delete[] inv[j];
						delete[] inv;
					}
				}
			}
			a = NULL;
			links = NULL;
			m = n = 0;
		}
		const Matrix &operator=(const Matrix &y)
		{
			//printf("Matrix::operator=(%p,%p) this=%p, a=%p, links=%d, matrix_count=%d\n",
			//		&y, y.a, this, a, a? *links: -1, matrix_count);
			if(this == &y)	return *this;
			unset();
			a = y.a;
			links = y.links;
			if(*links)	(*links)++;
			n = y.n, m = y.m;
			return *this;
		}
		Matrix(const Matrix &y) : m(y.m), n(y.n), a(y.a), links(y.links)
		{
			if(*links)	(*links)++;
		}
		//Matrix(int mm, int nn, const int *ar);
		Matrix() : m(0), n(0), a(NULL), links(NULL) {}
		Matrix(int mm, int nn, int nm, ...);
		Matrix(int mm, int nn);
		Matrix(int nn) : Matrix(nn, nn) {}
		virtual ~Matrix()	{ unset(); }
};

#endif
