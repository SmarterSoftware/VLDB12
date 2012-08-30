/* this file reads a model from disk and passes the result to the scanner*/
#include "model/model.h"
#include "utils/guc.h"
#include "access/heapam.h"
#include "access/relscan.h"

void PrintModel(DModel *m) {
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
	DModel *m= (DModel*)&(models[j]);
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
double EvalProb(int j,int x,double err)  {
	int i=0;
	DModel *m= (DModel*)&(models[j]);
	double error=0;
//	// no need to compute the value
	//elog(WARNING, "Model error%f requested error %f",error,err);
	//btw, compute the next values and add them to the cache
	//	return y;
	if ((err >m->err)||(m->nc <= 0)) {
		//elog(WARNING," matches the error %p",cache);
		if((cache != NULL) && (m_cache>0)) {
			//    elog(WARNING, "Filling from %d len: %d",x+1+i,m_cache);	
       			cache_start=x+1;
			for(i=0;i<m->len;i++) {
			   if(i>=m_cache)break;
			   cache[i]=Eval(j,x+1+i,&error);
			 }
			 for(i=x+m->len;i<m_cache;i++) {
			    cache[i]=-1;
			 }
		}		
		double y= Eval(j,x,&error);
		return y; // found result within the error
	}
	//elog(WARNING,"here");
	DModel *mm;

	int l=0;
	l=m->children[0];
	mm= (DModel*)&(models[l]);
        int llen = mm->len;
        int li = x / llen;
	
        if (li >= m->nc) {
          li =m-> nc - 1;
	  l=m->children[li];
	  mm= (DModel*)&(models[l]);
          llen = mm->len;
            }
	l=m->children[li];
//	printf("l %d li %d\n",l,li);
        return EvalProb(l,x % llen, err);
}

double EvalProbL(int j,int x,int layers)  {
	int i=0;
	DModel *m= (DModel*)&(models[j]);
	double error=0;
	double y=0;
	int length=m->len;
	if  (m->len < x) {
//	elog(WARNING," Future %d %d",x,m->len);
}
//	double y= Eval(j,x,&error);// no need to compute the value
	//elog(WARNING, "Model error%f requested error %f",error,err);
	//btw, compute the next values and add them to the cache
	//	return y;

	if ((layers==0)||(m->nc <= 0)) {
		//elog(WARNING," matches the error %p",cache);
		if((cache != NULL) && (m_cache>0)) {
			//    elog(WARNING, "Filling from %d len: %d",x+1+i,m_cache);	
       			cache_start=x+1;
			for(i=0;i<m->len;i++) {
			   if(i>=m_cache)break;
			   cache[i]=Eval(j,x+1+i,&error);
			 }
			 for(i=x+m->len;i<m_cache;i++) {
			    cache[i]=-1;
			 }
		}	
	y= Eval(j,x,&error);	
//	elog(WARNING,"EvalProbL layers:%d val %lf",layers,y);
	return y; // found result within the error
	}
	//elog(WARNING,"here");
	DModel *mm;

	int l=0;
	l=m->children[0];
	mm= (DModel*)&(models[l]);
        int llen = mm->len;
        int li = x / llen;
	
        if (li >= m->nc) {
          li =m-> nc - 1;
	  l=m->children[li];
	  mm= (DModel*)&(models[l]);
          llen = mm->len;
            }
	l=m->children[li];
//	printf("l %d li %d\n",l,li);
        double yy= EvalProbL(l,x % llen, layers-1);
//	elog(WARNING, "Eval Prop L layers %d %lf",layers,yy);
	return yy;
}

double GetValue(int x) {
	//elog(WARNING," value of %d",x);
	if(m_cache==-1) return 199;
	else if(m_cache== 0) return EvalProb(0,x,error_level);
	if(cache_start==-1) {
	//elog(WARNING,"Cache has not not been initlized");
	return EvalProb(0,x,error_level);
	}
	if((x-cache_start)>=m_cache) return EvalProb(0,x,error_level);
	//elog(WARNING,"Use case value %d",x);
	double v=cache[x-cache_start];
	if(v==-1)return EvalProb(0,x,error_level); 
	else
	return v;
}


double GetValueL(int x) {
//	double xx= EvalProbL(0,x,m_layers);	

	if(m_cache==-1) return 199;
	else if(m_cache== 0) return EvalProbL(0,x,m_layers);
	if(cache_start==-1) {
	//elog(WARNING,"Cache has not not been initlized");
	return EvalProbL(0,x,m_layers);
	}
	if((x-cache_start)>=m_cache) return EvalProbL(0,x,m_layers);
	//elog(WARNING,"Use case value %d",x);
	double v=cache[x-cache_start];
	if(v==-1)return EvalProbL(0,x,m_layers); 
	else
	return v;
}

DModel* ReadModel(FILE* f,int j){
	DModel *m= (DModel*)&(models[j]);
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
	
//   /home/khalefa/model/uk2.b  uk2m.b
// /home/khalefa/D3.4/mdata/uk2m.b


//	FILE* f=fopen("/home/khalefa/D3.4/mdata/uk_100.b","r");
	FILE* f=fopen("/home/khalefa/VLDB12DEMO/mdata/uk.b","r");
	int n,i;
	fscanf(f,"%d\n",&n);

	models=(DModel *) malloc(sizeof(DModel)*n);

	for( i=0;i<n;i++){
		ReadModel(f,i);
	}

	fclose(f);

	cache=NULL;
	cache_start=-1;

 	points=(int*) malloc(sizeof(int)*96);
	newpoints=0;

	extra=NULL;
}
//return 0 -->use model
//return 1 -->use newpoints
//return 2 totlay done
int Prepare(HeapScanDesc scan) {
	int ll=0;
	int i;
	DModel * m=&(models[0]);
	double limit=m->len+newpoints; //0.95*
	int length=m->len;
	if (extra!=NULL) limit+=extra->len;	
	if (m_start!=0)	ll=0;
	else if(grp_len!=0) ll=m_start*grp_len;
	else ll=m_start;

	if(scan->index==ll) {
		//elog(WARNING,"Preparing cache");
		if(m_cache>0) {
		        if(cache!=NULL) free(cache);
		//elog(WARNING,"Preparing cache --");
			cache=(double *) malloc(sizeof(double)*m_cache);
			for(i=0;i<m_cache;i++) cache[i]=-1;
			cache_start=-1;
		} else {
			cache=NULL;
			cache_start=-1;
		}
	}
	
	if(m_fend==-1)if (scan->index>limit) return 2;
		      else if (scan->index>=length) return 1;	
	if(m_fend!=-1)
	if(scan->index>m_fend*grp_len) return 2;
	else if ((scan->index<limit) && (scan->index>=length)) return 1;
	if(m_fend==-1) {//i.e., we have not set up the limits
		if(m_end==-1) {
		if(scan->index>limit) return 2;		
		if(scan->index>length) return 1;		
		}
		else
		if(scan->index>m_end*grp_len) return 2;	
		else if ((scan->index<limit) && (scan->index>=length)) return 1;
	}
return 0;
}
ExprContext*	econtext=NULL;
HeapTuple ComputeNextTuple(HeapScanDesc scan) {
	DModel * m=&(models[0]);
	int i;
	double y=0;
	double o=0;
	int x;
	int len=0;
	int cnt=0;
	int length=m->len;

	// prepare the cache
	int   a=Prepare(scan);
       	if(a==2) {
		return NULL;
        }
	//if(a==1) return NULL;
	if (grp_fnc=='M') o=DBL_MAX;
	x=scan->index+1;
	for(;;){
		scan->index++;
		if(a==1) {
	//		elog(WARNING," item %d %d",scan->index-length,newpoints );
			//check to use the extra or the points
			if(extra==NULL)
			 y=points[scan->index-length-1];
			else {
			int yy=scan->index-length-1;
			if(yy>=extra->len)
			   y=points[yy-extra->len];
			else
			  y=EvalB(extra,yy);
			}	  	
			
			}
		else {
		if(m_layers==-1)
			y=GetValue(scan->index);
		else	
			y=GetValueL(scan->index);
		}
		if ((grp_fnc=='s')||(grp_fnc=='a')) { o=o+y; cnt++;}
		else if ((grp_fnc=='m') && (o<y)) o =y;
		else if ((grp_fnc=='M') && (o>y)) o=y;
		else if ((grp_fnc=='\0')||(grp_fnc=='n')) o=y;
		len++;
		if((len>=grp_len)&&(grp_len!=-1)) {
			if (grp_fnc=='a') o=o/cnt;
			break;	
		}
		if( m_fend==-1) {
			if ((scan->index>length)&&(grp_len==-1))  return NULL;
			if (scan->index>length) break;
		} 
	}

//	elog(WARNING,"x %d, o%f",x,o);
	if ((grp_len!=-1)||(grp_len!=0)) x=x/grp_len;
	return CreateTuple(scan->rs_rd,x, (int)o);

}


void ModelAddItem(int i){
/*	int j;
	DModel *m= (DModel*)&(models[i]);
	m->len++;
	if (m->nc>0){
         j=  m->children[m->nc-1];
	  insertItem(j);
	}*/
	points[newpoints]=i;
	newpoints++;
	if(newpoints>15) {
		Incremental();
	}
}
/*void ReLoadModules(char * filename) {
	
//   /home/khalefa/model/uk2.b
	FILE* f=fopen(filename,"r");
	int n,i;
	fscanf(f,"%d\n",&n);
        if(models != NULL) free(models);
	models=(DModel *) malloc(sizeof(DModel)*n);

	for( i=0;i<n;i++){
		ReadModel(f,i);
	}

	fclose(f);

}*/

/*int main() { 

	FILE* f=fopen("a.txt","r");
	int n,i;
	double xx=0;
	fscanf(f,"%d\n",&n);
	models=(DModel *) malloc(sizeof(DModel)*n);


	for( i=0;i<n;i++){
		ReadModel(f,i);
		//PrintModel(&models[i]);
	}
	fclose(f);

	//data=(double*)malloc(sizeof(double)*10);

	for(i=1;i<20;i++) {

		xx=EvalProb(0,i,5.0);
		xx++;
		printf("Totalx  %lf\n",xx);
	}
}*/
