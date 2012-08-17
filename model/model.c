
/* this file reads a model from disk and passes the result to the scanner*/
#include "model.h"


void PrintModel(Model *m) {
	int i;
	printf("---------------------\n");
	printf("Id%d Type%d  len%d  Err%f freq%d\n",m->id,m->type,m->len,m->err,m->freq);
	printf("Values(%d):",m->nv);
	for(i=0;i<m->nv;i++)printf("%f ",m->values[i]);
	printf("\nTs(%d):",m->nts);
	for(i=0;i<m->nts;i++)printf("%f ",m->ts[i]);
	printf("\nChildren(%d):",m->nc);
	for(i=0;i<m->nc;i++)printf("%d ",m->children[i]);
	printf("\nSeasonal:%d",m->seasonal);

	printf("---------------------\n");
}

double Eval(int j,int x, double * error)  {
	double t, s;
	double xx,tt ;
//	printf("===");
	Model *m= (Model*)&(models[j]);
 	if (m->seasonal == -1 && m->nv == 0) { 
		return m->ts[x]; }
	if (m->seasonal == -1) { 
		return m->values[0] * x + m->values[1];}

	t = s = 0;
	if (m->type == EXPLICIT)  t = m->values[x / m->freq];
	else if (m->type == IMPLICIT)
		t = m->values[0] * x / m->freq + m->values[1];
	else t = m->values[0] * x + m->values[1];
	
	s = Eval(m->seasonal, x % m->freq,&tt);
	xx= s+t;
//	printf("Total  %lf\n",xx);
	*error=m->err;
	return xx;
}
double EvalProb(int j,int x,double error)  {
	double e=0;
	double y= Eval(j,x,&e);// no need to compute the value
	return y;
	if (e<error) return y; // found result within the error

	Model *m= (Model*)&(models[j]);
	Model *mm;

	if (m->nc <= 0) return y;
	int l=0;
	l=m->children[0];
	mm= (Model*)&(models[l]);
        int llen = mm->len;

         int li = x / llen;
	
            if (li >= m->nc)
            {
                li =m-> nc - 1;
		l=m->children[li];
		mm= (Model*)&(models[l]);
		
                llen = mm->len;
            }
		l=m->children[li];
//	printf("l %d li %d\n",l,li);
            return EvalProb(l,x % llen, error);
}


double GetValue(int x) {
return EvalProb(0,x,2);
}



Model* ReadModel(FILE* f,int j){
	Model *m= (Model*)&(models[j]);
	int ti;
	double tf;
	int i;
	fscanf(f, "%d", &ti);
	m->id=ti;
	fscanf(f, "%d", &ti);
	m->type=ti;
	fscanf(f, "%d", &ti);
	m->len=ti;

	fscanf(f, "%lf", &tf);
	m->err=tf;

	fscanf(f, "%d", &ti);
	m->freq=ti;

	fscanf(f, "%d", &ti);
	m->seasonal=ti;

	//read v
	fscanf(f, "%d", &ti);
	m->nv=ti;
	m->values=(double *) malloc(sizeof(double)*m->nv);
	for(i=0;i<m->nv;i++) {
		fscanf(f, "%lf", &tf);
		m->values[i]=tf;
	}
	//read ts
	fscanf(f, "%d", &ti);
	m->nts=ti;
	m->ts=(double *) malloc(sizeof(double)*m->nts);
	for(i=0;i<m->nts;i++) {
		fscanf(f, "%lf", &tf);
		m->ts[i]=tf;
	}
	//read c
	fscanf(f, "%d", &ti);
	m->nc=ti;
	m->children=(int *) malloc(sizeof(int)*m->nc);
	for(i=0;i<m->nc;i++) {
		fscanf(f, "%d", &ti);
		m->children[i]=ti;
	}
	fscanf(f, "\n");
	return m;
}

void LoadModules() {
	
/*	FILE* f=fopen("/home/khalefa/model/uk2.b","r");
	int n,i;
	double xx=0;
	fscanf(f,"%d\n",&n);

	models=(Model *) malloc(sizeof(Model)*n);

	for( i=0;i<n;i++){
		ReadModel(f,i);
	}
	fclose(f);
	for(i=0;i<10;i++)
	printf("i %d v %f\n", i, GetValue(i));*/
}

int main() { 
LoadModules();
}
