#include <stdio.h>
#include <stdint.h>
#include <conio.h>
#include <Windows.h>

#define MAXBLOCKLEN 16
#define ROTL(a,b) ((a<<b)|(a>>(32-b)))

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

bool next(int* c, int sz, int m)
{
	int point = sz;
	do
	{
		point--;
		c[point] = (c[point] + 1) % m;
	} while (point && !c[point]);
	if (point == 0)
	{
		for (int i = 0; i < sz; i++)
		{
			if (c[i])
				return false;
		}
		return true;
	}
	return false;
}

inline void lin444_r2(uint32_t* din, uint32_t* dout, int c0[3])
{
	dout[0] = din[0] ^ ROTL(din[1], c0[0]) ^ ROTL(din[2], c0[1]) ^ ROTL(din[3], c0[2]);
	dout[1] = din[1] ^ ROTL(din[2], c0[0]) ^ ROTL(din[3], c0[1]) ^ ROTL(dout[0], c0[2]);
	dout[2] = din[2] ^ ROTL(din[3], c0[0]) ^ ROTL(dout[0], c0[1]) ^ ROTL(dout[1], c0[2]);
	dout[3] = din[3] ^ ROTL(dout[0], c0[0]) ^ ROTL(dout[1], c0[1]) ^ ROTL(dout[2], c0[2]);

	dout[0] = dout[0] ^ ROTL(dout[1], c0[0]) ^ ROTL(dout[2], c0[1]) ^ ROTL(dout[3], c0[2]);
	dout[1] = dout[1] ^ ROTL(dout[2], c0[0]) ^ ROTL(dout[3], c0[1]) ^ ROTL(dout[0], c0[2]);
	dout[2] = dout[2] ^ ROTL(dout[3], c0[0]) ^ ROTL(dout[0], c0[1]) ^ ROTL(dout[1], c0[2]);
	dout[3] = dout[3] ^ ROTL(dout[0], c0[0]) ^ ROTL(dout[1], c0[1]) ^ ROTL(dout[2], c0[2]);
}
inline void lin444_r1(uint32_t* din, uint32_t* dout, int c0[3])
{
	dout[0] = din[0] ^ ROTL(din[1], c0[0]) ^ ROTL(din[2], c0[1]) ^ ROTL(din[3], c0[2]);
	dout[1] = din[1] ^ ROTL(din[2], c0[0]) ^ ROTL(din[3], c0[1]) ^ ROTL(dout[0], c0[2]);
	dout[2] = din[2] ^ ROTL(din[3], c0[0]) ^ ROTL(dout[0], c0[1]) ^ ROTL(dout[1], c0[2]);
	dout[3] = din[3] ^ ROTL(dout[0], c0[0]) ^ ROTL(dout[1], c0[1]) ^ ROTL(dout[2], c0[2]);
}

struct max_s
{
	double max_avg;
	int max_BI;
	int max_act;
	char desc[16];
	int level;
	bool can_levelup;
	max_s* nextlevel;
	char color[6];
	void (*f)(uint32_t* din, uint32_t* dout, int c0[3]);
	int dmin;
	double best_score;
} r2 = { 0,0,0,"two-round\0", 1, false, NULL, ANSI_COLOR_YELLOW, lin444_r2, 0,0 },
r1 = { 0,0,0,"one-round\0", 0, true, &r2, ANSI_COLOR_GREEN, lin444_r1, 0,0 };

struct res_s
{
	double a1;
	double a2;
	double b1;
	double b2;
	int bmin;
	bool valid;
	double score;
	int c[3];
};

inline void score(res_s* r, double weights[5])
{
	double n = 0;
	for (int i = 0;i < 5;i++)
		n += weights[i];
	if (!r->valid)
	{
		r->score = 0;
		return;
	}
	r->score = (weights[0] * r->a1 + weights[1] * r->a2 + weights[2] * r->b1 + weights[3] * r->b2 + weights[4] * r->bmin) / n;
}

void all_max(int* c, max_s* max_vals, res_s *fres, double weights[5])
{
	uint8_t data_in[MAXBLOCKLEN] = { 0 }, test[MAXBLOCKLEN];
	uint32_t* din = (uint32_t*)data_in, * tst = (uint32_t*)test;
	int w[16] = { 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4 };
	int diff = 0;
	int act_total = 0;
	fres->valid = true;
	int min = MAXBLOCKLEN;
	for (int b = 0;b < 4;b++)
	{
		for (int pos = 0;pos < 8;pos++)
		{
			din[b] = 1 << pos;
			max_vals->f(din, tst, c);
			int res = 0;
			for (int i = 0; i < 16; i++)
			{
				uint8_t temp = test[i];
				diff += w[temp & 0xf] + w[temp >> 4];
				if (temp)
					res++;
			}
			if (res < min)
			{
				if (res < max_vals->dmin)
				{
					fres->valid = false;
					return;
				}
				min = res;
			}
			act_total += res;
		}
		din[b] = 0;
	}
	double avgdiff = (double)diff / 32;
	if (max_vals->level == 0)
	{
		fres->a1 = avgdiff;
		fres->b1 = (double)act_total / 32.0;
		fres->bmin = min + 1;
	}
	if (max_vals->level == 1)
	{
		fres->a2 = avgdiff;
		fres->b2 = (double)act_total / 32.0;
	}
	if (max_vals->can_levelup)
	{
		all_max(c, max_vals->nextlevel, fres, weights);
	}
	else
	{
		fres->c[0] = c[0];
		fres->c[1] = c[1];
		fres->c[2] = c[2];
	}
	return;
}

void ind(int c[3], double weights[5])
{
	res_s tres;
	max_s dr2 = { 0,0,0,"two-round\0", 1, false, NULL, ANSI_COLOR_YELLOW, lin444_r2, 0 },
		dr1 = { 0,0,0,"one-round\0", 0, true, &dr2, ANSI_COLOR_GREEN, lin444_r1, 0 };
	all_max(c, &dr1, &tres, weights);
}

void print_res_header(double weights[5])
{
	printf("Choosen weights:\n");
	printf("Average single-round diffusion: %.2f\n", weights[0]);
	printf("Average double-round diffusion: %.2f\n", weights[1]);
	printf("Average single-round branch:    %.2f\n", weights[2]);
	printf("Average double-round branch:    %.2f\n", weights[3]);
	printf("Minimal single-bit diffusion:   %.2f\n\n", weights[4]);
	printf("a1\ta2\tb1\tb2\tbmin\tscore\tc\n");
}

void print_res(res_s *fres)
{
	printf("%.2f\t%.2f\t%.2f\t%.2f\t%d\t%.2f\t",
		fres->a1, fres->a2, fres->b1, fres->b2, fres->bmin, fres->score);
	for (int i = 0; i < 3; i++)
		printf("%d ", fres->c[i]);
	printf("\n");
}

int main()
{
	int c[3] = { 0 };
	res_s cur_res, best_res;
	best_res.score = 0;
	/*                      a1   a2   b1   b2   bmin */
	//double weights[5] = { 1.0, 0.0, 1.0, 0.0, 1.0 };
	//double weights[5] = { 0.0, 1.0, 0.0, 1.0, 1.0 };
	//double weights[5] = { 1.0, 0.5, 2.0, 0.8, 1.0 };
	double weights[5] = { 1.0, 2.0, 1.0, 2.0, 1.0 };
	
	print_res_header(weights);
	do
	{
		all_max(c, &r1, &cur_res, weights);
		if (cur_res.valid)
		{
			score(&cur_res, weights);
			if (cur_res.score > best_res.score) {
				best_res = cur_res;
				print_res(&best_res);
			}
		}
	} while (!next(c, 3, 32));
	print_res(&best_res);
	return 0;
}