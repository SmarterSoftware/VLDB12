#ifndef TS_H
#define TS_H

struct _ts    {
        double *data;
        int *freq;
	int n;
	int nf;
};
typedef struct _ts TS;

TS *initTimeSeries(int n, int nf,int *freq);
TS* initTimeSeries2(int n,double *x,int nf, int *freq);

#endif
