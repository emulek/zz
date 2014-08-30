#include "matrix.hpp"
#include "getopt.hpp"
#include <stdarg.h>


void Matrix::print(bool f = false) const
{
	int j, k;
	if(!a)	return;
	for(j = 0; j < m; j++)
	{
		for(k = 0; k < n; k++)
			printf("%s ", a[j][k].print());
		if(f)
		{
			printf("\t");
			for(k = 0; k < n; k++)
				printf("%s ", inv[j][k].print());
		}
		printf("\n");
	}
}

const Matrix &Matrix::operator+=(const Matrix &y)
{
	int j, k;
	assert(a && y.a);
	assert(m == y.m && n == y.n);
	release();
	for(j = 0; j < m; j++)
		for(k = 0; k < n; k++)
			a[j][k] += y.a[j][k];
	return *this;
}

const Matrix &Matrix::operator*=(const Matrix &y)
{
	assert(a && y.a);
	release();
	*this = *this * y;
	return *this;
}

Matrix Matrix::operator*(const Matrix &y) const
{
	// Z(m,y.n) = X(m,n) * Y(y.m,y.n)
	assert(a && y.a);
	assert(n == y.m);
	Matrix z(m, y.n);// z = x*y
	int i, j, k;
	for(j = 0; j < m; j++)
		for(k = 0; k < y.n; k++)
		{
			RSD s = 0;
			for(i = 0; i < n; i++)
				s += a[j][i]*y.a[i][k];
			z.a[j][k] = s;
		}
	return z;
}

void Matrix::set_rnd()
{
	int j, k;
	release();
	for(j = 0; j < m; j++)
		for(k = 0; k < n; k++)
			a[j][k] = rnd()%(RS_M-1)+1;
}

RSD **Matrix::inv = NULL;
int Matrix::inv_n = 0;

int Matrix::matrix_count = 0;

int Matrix::inverse()
{
	int d, i, j, k;
	assert(n == m);
	assert(a);
	release();
	if(!inv || n != inv_n)
	{
		if(inv)
		{
			for(j = 0; j < inv_n; j++)
				delete[] inv[j];
			delete[] inv;
			inv = NULL;
		}
		inv = new RSD*[n];
		for(j = 0; j < n; j++)
			inv[j] = new RSD[n];
	}
	for(j = 0; j < n; j++)
		for(k = 0; k < n; k++)
			inv[j][k] = j==k ? 1 : 0;
	RSD f;
	//printf("Matrix::inverse(), this:\n");
	//print(true);
	for(d = 0; d < n; d++)
	{
		k = d;
		if(a[d][k] == 0)
		{
			printf("n=%d, a[%d][%d]=%s\n", n, d, k, a[d][k].print());
			if(d >= n-1)
			{
				printf("Can't inverse matrix, n-d=1\n");
				return 1;// ноль в посл. строке
			}
			for(j = d+1; j < n && a[j][k] == 0; j++);
			if(j==n)
			{
				printf("Can't inverse matrix, n-d=%d\n", n-d);
				return n-d;
			}
			swap_lines(a, d, j);
			swap_lines(inv, d, j);
		}
		f = a[d][k];
		//printf("a[%d][%d]=%s\n", d, k, f.print());
		for(i = 0; i < n; i++)
			a[d][i] /= f, inv[d][i] /= f;
		for(j = 0; j < n; j++)
		{
			if(j == d)	continue;
			f = a[j][d];
			for(k = 0; k < n; k++)
				a[j][k] -= f*a[d][k], inv[j][k] -= f*inv[d][k];
		}
		//print(true); printf("\n");
	}
	return 0;
}

Matrix::Matrix(int mm, int nn) : m(mm), n(nn)
{
	int j, k;
	assert(m > 0 && n > 0);
	a = new RSD*[m];
	for(j = 0; j < m; j++)
	{
		a[j] = new RSD[n];
		for(k = 0; k < n; k++)
			a[j][k] = 0;
	}
	links = new int(1);
	//printf("Matrix::Matrix(%d, %d) this=%p, a=%p, matrix_count=%d\n",
	//		m, n, this, a, matrix_count);
	matrix_count++;
}

Matrix::Matrix(int mm, int nn, int nm, ...) : m(mm), n(nn)
{
	int j, k;
	va_list ap;
	assert(m > 0 && n > 0);
	a = new RSD*[m];
	matrix_count++;
	if(options->verbose==9)
		printf("Matrix::Matrix(%d, %d, ...) this=%p, a=%p, matrix_count=%d\n",
				m, n, this, a, matrix_count);
	va_start(ap, nm);
	for(j = 0; j < m; j++)
	{
		a[j] = new RSD[n];
		for(k = 0; k < n; k++)
		{
			a[j][k] = va_arg(ap, int);
			nm--;
		}
	}
	va_end(ap);
	links = new int(1);
	if(options->verbose==9)	print(false);
	assert(nm == 0);
}

