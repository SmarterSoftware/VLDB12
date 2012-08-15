#include "ts.h"
TS *initTimeSeries(int n,int l,int *freq)
{
 TS *r=(TS*)malloc(sizeof(TS));
 r->data = (int *) malloc(sizeof(double)*n);
 r->n=n;
 r->freq = freq;
 r->nf=l;
 return r;
}

TS* initTimeSeries2(int n,double *x,int l, int *freq)
{
 int i;
 TS *r=(TS*)malloc(sizeof(TS));
 r->data = (int *) malloc(sizeof(double)*n);
 r->freq = freq;
 r->n=n;
 r->nf=l;
 for (i = 0; i < n; i++) r->data[i] = x[i];
 return r;
}

