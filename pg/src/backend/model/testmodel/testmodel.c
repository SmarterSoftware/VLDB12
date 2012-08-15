#include "bmodel.h"
#include "ts.h"

int main() {
int i,n;
double *x=(double *)malloc(100*sizeof(double));
FILE* f=fopen("uk","r");
for(i=0;i<100;i++){
 fscanf(f,"%d\n",&n);
 x[i]=n;
}
int *freq=(int *) malloc(sizeof(int)*2);
freq[0]=96*2;
freq[1]=96;
TS *ts=initTimeSeries2(100,x,2,freq);
BModel *m=initModel(ts);
//PrintBModel(m);

}
