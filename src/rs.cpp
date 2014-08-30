#include "rs.hpp"

int RSD::alpha = 0;

// αⁿ α=35
int RSD::pow_tbl[RS_M];
/*= {
	1,
	35,	4,	29,	16,	5,	27,
	20,	34,	6,	25,	24,	26,
	22,	30,	14,	9,	19,	36,
	2,	33,	8,	21,	32,	10,
	17,	3,	31,	12,	13,	11,
	15,	7,	23,	28,	18,	1
};*/

int RSD::log_tbl[RS_M];
/*= {
	-1,
	36,	19,	26,	2,	5,	9,
	32,	21,	16,	24,	30,	28,
	29,	15,	31,	4,	25,	35,
	17,	7,	22,	13,	33,	11,
	10,	12,	6,	34,	3,	14,
	27,	23,	20,	8,	1,	18
};*/

void RSD::init()
{
	if(alpha != 0)	return;
	// работает только умножение и сложение
	alpha = RS_M-2;
	// поиск первообразнго эл-та
	do{
		int mul_tbl[RS_M];// таблица умноженмия на α
		int j;
		// очистка таблицы логорифмов
		for(j = 0; j < RS_M; j++)	log_tbl[j] = -1;
		// заполнение таблицы умножения
		mul_tbl[0] = 0;// α*0 = 0
		for(j = 1; j < RS_M; j++)
		{
			mul_tbl[j] = mul_tbl[j-1]+alpha;
			if(mul_tbl[j] >= RS_M)
				mul_tbl[j] -= RS_M;
		}
		pow_tbl[0] = 1;// α^0 = 1
		for(j = 1; j < RS_M; j++)
		{
			int i = mul_tbl[pow_tbl[j-1]];
			pow_tbl[j] = i;
			if(log_tbl[i] < 0)
				log_tbl[i] = j;
			else
				break;
		}
		if(j >= RS_M)
		{
			//printf("RSD::init() α=%d\npow_tbl:\n", alpha);
			//for(j = 0; j < RS_M; j++)
			//	printf("%02d%c", pow_tbl[j], j%10==9? '\n': '\t');
			//printf("\nlog_tbl:\n");
			//for(j = 0; j < RS_M; j++)
			//	printf("%02d%c", log_tbl[j], j%10==9? '\n': '\t');
			//printf("\n");
			return;
		}
	}while(--alpha >= 1);
	assert(false);
}

const char *RSD::print() const
{
	static int i = 0;
	static char s[RSDPRN][10];
	if(++i >= RSDPRN) i = 0;
	if(d<0||d>=RS_M)
		strcpy(s[i], "☣");
	else
		sprintf(s[i], "%03X", d);
	return s[i];
}
