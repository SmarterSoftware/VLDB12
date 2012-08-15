#include "model/ts.h"
struct _ts *initTimeSeries(int n,int l,int *freq)
{
 struct _ts *r=(struct _ts*)malloc(sizeof(struct _ts));
 r->data = (double *) malloc(sizeof(double)*n);
 r->n=n;
 r->freq = freq;
 r->nf=l;
 return r;
}

struct _ts* initTimeSeries2(int n,double *x,int nf, int *freq)
{
 
 struct _ts *ts=(struct _ts*)malloc(sizeof(struct _ts));
 int i;
 ts->data = (double *) malloc(sizeof(double)*n);
 ts->freq = (int *) malloc(sizeof(int)*nf);
 ts->n=n;
 ts->nf=nf;
 for (i = 0; i < n; i++) ts->data[i] = x[i];
 for (i = 0; i < nf; i++) ts->freq[i] = freq[i];
 //printTimeSeries(ts);
 return ts;
}

struct _ts* copyTimeseries(struct _ts *ts_) {
  int i;
 struct _ts *ts=(struct _ts*)malloc(sizeof(struct _ts));
 ts->data = (double *) malloc(sizeof(double)*ts_->n);
 ts->freq = (int *) malloc(sizeof(int)*ts_->nf);
 ts->n=ts_->n;
 ts->nf=ts_->nf;
 for (i = 0; i < ts->n; i++) ts->data[i] = ts_->data[i];
 for (i = 0; i < ts->nf; i++) ts->freq[i] = ts_->freq[i];
 return ts;
}
void printTimeSeries(struct _ts *ts)
{
 int i;
 printf ("n  %d nf %d data ",ts->n,ts->nf); 
 for (i = 0; i < ts->n; i++) printf(" %3.2lf ", ts->data[i]);
 printf(" freq ");
 for (i = 0; i < ts->nf; i++) printf(" %3.2d ", ts->freq[i]);
 printf("\n");
}

freeTimeSeries(struct _ts *ts){
	free(ts->data);
	free(ts->freq);
	free(ts);
}


void initTS(struct _ts *ts) {
}
