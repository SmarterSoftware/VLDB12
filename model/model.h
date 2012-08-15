#include <stdio.h>

#define EXPLICIT 0
#define IMPLICIT 2
#define TREND 1

struct _model {
int id;
int seasonal;
int type; // type of the model

int freq;
int len;
double err;
int nv;
double *values; //length is in len

int nts;
double *ts;

int nc;
int *children;
};

typedef struct _model Model;

Model *models;
