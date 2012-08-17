#include <stdio.h>

void compute(int id,double limit) {
FILE* f= fopen("c.txt","r");
int i=0;
int x=0;
double d;
printf("create table uk (id int, val int);\n");
 while (!feof(f)) {

i=fscanf(f,"%d",&d);
printf("insert into uk values (%d, %d);\n",x,d);
x++;

if(i==0) break;
if(x>limit) break;
}
fclose(f);

}
int main() {
compute(1,1000*1000);
/*compute(2,1000*1000);
compute(3,1500*1000);
compute(4,2000*1000);
compute(5,2500*1000);
compute(6,3000*1000);*/
}
