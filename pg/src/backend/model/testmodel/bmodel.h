#ifndef BMODEL_H
#define BMODEL_H
#include <stdio.h>
#include "ts.h"

#define EXPLICIT 0
#define IMPLICIT 2
#define TREND 1
struct _bmodel {
 
int id;
struct _bmodel *seasonal;
int type; // type of the model

int freq;
int len;
double err;
int nv;
double *values; //length is in len
double *trend;

int nts;
//double *ts;

int nc;
int *children;

double *errors;

TS *ts;

int start;
};

typedef struct _bmodel BModel;

double Error(BModel *m,double x);
void CalcError(BModel *m);
void Set(BModel *m);
int Type(BModel *m);
void decompose(BModel* model,int n, int freq, double* season_);
BModel *initModel(TS *ts);
int countError(BModel *m,double error);
BModel * initEmptyModel();
void Solve(BModel *m);
void Clean(BModel *m);
double EvalB(BModel *m,int x);
void PrintBModel(BModel *m);

#endif
