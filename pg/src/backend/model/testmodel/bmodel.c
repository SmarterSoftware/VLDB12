#include "bmodel.h"
#include "stl.h"
#include "cheb.h"

#define dabs(a)	   (((a) < 0) ? -(a) : (a))

int Global_id=0;

/*int ceil(double x) {
int ix=(int)x;
if(ix-x==0) return ix;
return ix+1;
}*/
void decompose(BModel* model,int n, int freq, double* season_)
{
   int i,j,l,k;
   double t1,t2;
   int swindow = 10 * n + 1;
   double *seasonal = (double *) malloc(sizeof(double)*freq);
   int *count = (int *) malloc(sizeof(int)*freq);
   int limit = ceil((double)n / freq);
   model->freq = freq;
   model->len = n;
        double* y = (double*)malloc(sizeof(double) * n);
        double* t = (double*)malloc(sizeof(double) * n);
        double* s = (double*)malloc(sizeof(double) * n);
        for (i = 0; i < n; i++) y[i] = model->ts->data[i];
        stl_compute(y, n, freq, swindow, t, s);
        k = 0;
        if (model->type == EXPLICIT|| model->type ==IMPLICIT)
        {
            model->values = (double *) malloc(sizeof(double)*limit);
	    model->nv=limit;
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
                model->values[i] = (t2 + t1) / 2;
            }
        }
        if (model->type ==IMPLICIT)
        {
            double* tt = ChebyshevReg_Solve(model->values, limit);
	    free(model->values);
            model->values = (double *) malloc(sizeof(double)*2);
	    model->nv=2;
            model->values[0] = tt[0];
            model->values[1] = tt[1];
	    free(tt);
        }
        else if (model->type == TREND)
        {
            double* t_ = (double *) malloc(sizeof(double)*n);
            for ( i = 0; i < n; i++)
            {
                t_[i] = t[i];
            }

            double* x = ChebyshevReg_Solve(t_,n);
	    free(t_);
            model->values = (double *) malloc(sizeof(double)*2);
	    model->nv=2;
            model->values[0] = x[0];
            model->values[1] = x[1];
	    free(x);
        }
        //make seaonality perfect
        for (i = 0; i < n; i++)
        {
            seasonal[i % freq] += s[i];
            count[i % freq]++;
        }
        for (i = 0; i < freq; i++)
        {
            season_[i] = seasonal[i % freq] / count[i % freq];
        }

        model->trend = (double *) malloc(sizeof(double)*n);
        model->errors = (double *) malloc(sizeof(double)*n);
        for (i = 0; i < n; i++)
        {
            model->trend[i] = t[i];
            model->errors[i] = model->ts->data[i] - (t[i] + s[i % freq]);
        }

        free(y);
        free(s);
        free(t);
	free(seasonal);
	free(count);    
}
BModel *initModel(TS *ts)
{
    BModel *m=(BModel*) malloc(sizeof(BModel));
    m->ts = ts;
    m->seasonal = NULL;
    m->values = NULL;
    m->errors = NULL;
    m->nv=0;
    m->nc=0;
    
    m->type = TREND;
    Solve(m);
    CalcError(m);
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
    return m;
}
void Solve(BModel *m)
{

    int n = m->ts->n;
    printf("Solve %d",n);
    int i,j;
    if ((m->ts->freq == NULL))
    {
        // ts data is used
        int x = 9;
        x++;
    }
    else
    {
        int freq = m->ts->freq[0];
	printf("freq %d\n",freq);
        int l = 0;
        while (freq > m->ts->n) { freq = m->ts->freq[l++]; }
	printf("freq %d\n",freq);
        if (freq == 0)
        {
            //use regression
	printf("use regression\n");
            m->values = ChebyshevReg_Solve(m->ts->data,m->ts->n);
            m->nv=2;
	    m->seasonal = NULL;
        }
        else
        {
            double* season_ = (double *) malloc(sizeof(double)*freq);
            decompose(m,n, freq, season_);
            m->freq = freq;
            m->seasonal = initEmptyModel();
             int* f;
             if (m->ts->nf== 1) f = NULL;
              else
                {
                    f = (int*) malloc(sizeof(int)*(m->ts->nf - 1));
                    for (i = 0; i < m->ts->nf - 1; i++) f[i] = m->ts->freq[i + 1];
                }
                m->seasonal->ts = initTimeSeries2(freq,season_, m->ts->nf-1,f);
		for(j=0;j<freq;j++) printf("season_ %d %lf\n",j,season_[j]);
                if (f != NULL)
                    Solve(m->seasonal);
            }
        }
    }


void Clean(BModel *m)
{
    if(m->errors!=NULL) free(m->errors);
    if(m->trend!=NULL) free(m->trend);
    m->errors = NULL;
    m->trend = NULL;
    if ((m->seasonal != NULL) && (m->values != NULL)) { free(m->ts); m->ts = NULL; }
    if (m->seasonal != NULL) Clean(m->seasonal); m->seasonal=NULL;
}
double EvalB(BModel *m,int x)
{
    if (m->seasonal == NULL && m->values == NULL) return m->ts->data[x];

    if (m->seasonal == NULL) return m->values[0] * x + m->values[1];
    double t, s;
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
    }
  qsort (m->errors, m->ts->n, sizeof(double), compare);
}
double Error(BModel *m,double x)
{
    return m->errors[(int)(x * (m->ts->n - 1))];
}
int Size(BModel *m)
{
    if (m->seasonal == NULL && m->values == NULL) return m->ts->n;
    if (m->seasonal == NULL) return 2;
    return m->len + Size(m->seasonal);
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
	printf("Values(%d):",m->nv);
	for(i=0;i<m->nv;i++)printf("%f ",m->values[i]);
	printf("\nTs(%d):",m->ts->n);
	for(i=0;i<m->ts->n;i++)printf("%f ",m->ts->data[i]);
	printf("\nChildren(%d):",m->nc);
	for(i=0;i<m->nc;i++)printf("%d ",m->children[i]);
	printf("\nSeasonal:%d",m->seasonal);
        if(m->seasonal!=NULL) PrintBModel(m->seasonal);
	printf("---------------------\n");
}

