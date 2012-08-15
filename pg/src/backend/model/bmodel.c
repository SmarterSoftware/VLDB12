#include "model/bmodel.h"
#include "model/model.h"
#include "model/stl.h"
#include "model/cheb.h"

#define dabs(a)	   (((a) < 0) ? -(a) : (a))

int Global_id=0;

BModel *extra;
int myceil(double x) {
	int ix=(int)x;
	if(ix-x==0) return ix;
	return ix+1;
}
void decompose(BModel* model,int n, int freq, double* season_)
{
	int i,j,l,k,limit;
	double t1,t2;
	int swindow = 10 * n + 1;
	double* tt;
	double* y, *t,*s,*seasonal,*x;
	double *values=NULL;
	int *count;
	int nv=0;

	limit = myceil((double)n / freq);
	model->freq = freq;
	model->len = n;

	y = (double*)malloc(sizeof(double) * n);
	t = (double*)malloc(sizeof(double) * n);
	s = (double*)malloc(sizeof(double) * n);
	for (i = 0; i < n; i++) y[i] = model->ts->data[i];
	stl_compute(y, n, freq, swindow, t, s);
	free(y);
	//make seaonality perfect
	seasonal = (double *) malloc(sizeof(double)*freq);
	count = (int *) malloc(sizeof(int)*freq);
	for (i = 0; i < freq; i++)
	{
		seasonal[i] = 0;
		count[i ]=0;
	}

	for (i = 0; i < n; i++)
	{
		seasonal[i % freq] += s[i];
		count[i % freq]++;
	}
	for (i = 0; i < freq; i++)
	{
		season_[i] = seasonal[i % freq] / count[i % freq];
	}
	free(seasonal);
	free(count);

	free(s);
	k = 0;
	if (model->type == EXPLICIT|| model->type ==IMPLICIT)
	{
		values = (double *) malloc(sizeof(double)*limit);
		nv=limit;
		for (i = 0; i < limit; i++)
		{
			t1 = 0;
			t2 = 0;
			l = 0;
			for (j = 0; j < freq; j++)
			{
				if (k == n) break;
				l++;
				k++;
				t1 += model->ts->data[j + i * freq];
				t2 += y[j + i * freq];
			}
			t1 /= l;
			t2 /= l;
			values[i] = (t2 + t1) / 2;
		}
	}
	if (model->type ==IMPLICIT)
	{
		tt = ChebyshevReg_Solve(values, nv);
		if (values!= NULL) free(values);
		values = (double *) malloc(sizeof(double)*2);
		nv=2;
		values[0] = tt[0];
		values[1] = tt[1];
		free(tt);
	}
	else if (model->type == TREND)
	{
		x = Reg_solve(t,n);
		if (values!= NULL) free(values);
		values = (double *) malloc(sizeof(double)*2);
		nv=2;
		values[0] = x[0];
		values[1] = x[1];
		free(x);
	}
	/*model->trend = (double *) malloc(sizeof(double)*n);
	for (i = 0; i < n; i++)	 model->trend[i] = t[i];*/
	model->nv=nv;
	model->values = (double *) malloc(sizeof(double)*nv);
	for (i = 0; i < nv; i++)	 model->values[i] = values[i];

	//free(y);
	//free(s);
	//free(t);
	free(t);
	free(values);
}
BModel *initModel(struct _ts *ts)
{
	double error;
	int i,best;
	double rt,min; 
	BModel *m=(BModel*) malloc(sizeof(BModel));
	m->ts = copyTimeseries( ts);
	m->seasonal = NULL;
	m->values = NULL;
	m->errors = NULL;
	m->nv=0;
	best=0;
	//	m->nc=0;
	m->errors =(double*) malloc(sizeof(double)*ts->n);
	min=1000000;
	/*for( i =0;i<3;i++) {
	m->type = i;
	Solve(m);
	CalcError(m);
	error=Error(m,0.9);
	rt = Size(m) * error;
	if (min > rt) { min = rt; best = i; }
	}
	*/
	m->type = TREND;
	Solve(m);
	CalcError(m);
	error=Error(m,0.9);


	m->err=error;
	m->len = ts->n;
	return m;
}

int countError(BModel *m,double error)
{
	int count = 0;
	int i;
	CalcError(m);

	for (i = 0; i < m->ts->n; i++)
	{
		if (m->errors[i] < error) count++;
	}

	return count;
}
BModel * initEmptyModel()
{
	BModel *m=(BModel*) malloc(sizeof(BModel));
	m->ts = NULL;
	m->seasonal = NULL;
	m->values = NULL;
	m->errors = NULL;
	m->nv=0;
	m->type=TREND;
	return m;
}
void Solve(BModel *m)
{
	int i,j,x,freq,l;
	int *f;
	int n = m->ts->n;
	double* season_; 
	//printf("Solve %d",n);

	if ((m->ts->freq == NULL))
	{
		// ts data is used
		x = 9;
		x++;
	}
	else
	{
		freq = m->ts->freq[0];
		//printf("freq %d\n",freq);
		l = 0;
		while (freq > m->ts->n) { freq = m->ts->freq[l++]; }
		//printf("freq %d\n",freq);
		if (freq == 0)
		{
			//use regression
			//printf("use regression\n");
			m->values = ChebyshevReg_Solve(m->ts->data,m->ts->n);
			m->nv=2;
			m->seasonal = NULL;
		}
		else
		{
			season_ = (double *) malloc(sizeof(double)*freq);
			decompose(m,n, freq, season_);
			m->freq = freq;			

			if (m->ts->nf== 1) f = NULL;
			else
			{

				f = (int*) malloc(sizeof(int)*(m->ts->nf - 1));
				for (i = 0; i < m->ts->nf - 1; i++) f[i] = m->ts->freq[i + 1];
			}
			//for(j=0;j<freq;j++) printf("season_ %d %lf\n",j,season_[j]);
			m->seasonal = initEmptyModel();
			m->seasonal->ts =initTimeSeries2(freq,season_, m->ts->nf-1,f);

			if (f != NULL){
				Solve(m->seasonal);
			}
		}
	}
}


void Clean(BModel *m)
{
	if(m->errors!=NULL) free(m->errors);
	//	if(m->trend!=NULL) free(m->trend);
	m->errors = NULL;
	//m->trend = NULL;
	if ((m->seasonal != NULL) && (m->values != NULL)) { free(m->ts); m->ts = NULL; }
	if (m->seasonal != NULL) Clean(m->seasonal); m->seasonal=NULL;
}
double EvalB(BModel *m,int x)
{
	double t, s;
	if (m->seasonal == NULL && m->values == NULL) return m->ts->data[x];

	if (m->seasonal == NULL) return m->values[0] * x + m->values[1];

	t = s = 0;
	if (m->type == EXPLICIT)
		t = m->values[x / m->freq];
	else if (m->type ==IMPLICIT)
		t = m->values[0] * x / m->freq + m->values[1];
	else t = m->values[0] * x + m->values[1];

	s = EvalB(m->seasonal,x % m->freq);
	return s + t;
}
int compare (const void * a, const void * b)
{
	return ( *(double*)a - *(double*)b );
}

void CalcError(BModel *m)
{
	int i;
	//    errors = (double *) malloc(sizeof(double)*ts.Length];
	for (i = 0; i < m->ts->n; i++)
	{
		m->errors[i] = dabs(m->ts->data[i] - EvalB(m,i));// /ts.data[i] * 100;
		//	printf("Error %d %4.2lf",i,m->errors[i]);
	}
	qsort (m->errors, m->ts->n, sizeof(double), compare);
}
double Error(BModel *m,double x)
{
	return m->errors[(int)(x * (m->ts->n - 1))];
}
int BSize(BModel *m)
{
	if (m->seasonal == NULL && m->values == NULL) return m->ts->n;
	if (m->seasonal == NULL) return 2;
	return m->len + BSize(m->seasonal);
}

void Set(BModel *m)
{
	m->err = Error(m,0.9);
	m->id = Global_id;
	Global_id++;
	if (m->seasonal != NULL) Set(m->seasonal);
}
int Type(BModel *m)
{
	if (m->type == EXPLICIT) return 0;
	if (m->type == TREND) return 1;
	return 2;
}


void PrintBModel(BModel *m) {
	int i;
	printf("---------------------\n");
	printf("Id%d Type%d  len%d  Err%f freq%d\n",m->id,m->type,m->len,m->err,m->freq);
	printf("Values(%d):%p",m->nv, m->values);
	for(i=0;i<m->nv;i++)printf("%3.3f ",m->values[i]);
	printf("\nTs(%d):",m->ts->n);
	for(i=0;i<m->ts->n;i++)printf("%3.2f ",m->ts->data[i]);
	printf("\nSeasonal:%d",m->seasonal);
	if(m->seasonal!=NULL) PrintBModel(m->seasonal);
	printf("---------------------\n");
}

void Incremental() {
int i,n;
int *freq;
struct _ts *ts;


double *x=(double *)malloc(newpoints*sizeof(double));
elog(WARNING,"Adding a new model");
for(i=0;i<newpoints;i++){
x[i]=points[i];
}

freq=(int *) malloc(sizeof(int)*2);
freq[0]=96*2;
freq[1]=4;
ts=initTimeSeries2(newpoints,x,2,freq);
extra=initModel(ts);
freeTimeSeries(ts);
free(x);
newpoints=0;
/*for(i=0;i<newpoints;i++){

elog(WARNING, "new model %d %lf",i,EvalB(m,i));
}*/
}
