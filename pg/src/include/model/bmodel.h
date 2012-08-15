#ifndef BMODEL_H
#define BMODEL_H
#include <stdio.h>
#include "model/ts.h"
#include "model/model.h"
#include "utils/guc.h"
#include "access/heapam.h"
#include "access/relscan.h"

#define EXPLICIT 0
#define IMPLICIT 2
#define TREND 1
struct _bmodel {
 
int id;

int type; // type of the model

int freq;
int len;
double err;
int nv;
double *values; 
//double *trend;
double *errors;
struct _bmodel *seasonal;
struct _ts *ts;

//int start;
} ;

typedef struct _bmodel BModel;

double Error(BModel *m,double x);
void CalcError(BModel *m);
void Set(BModel *m);
int Type(BModel *m);
void decompose(BModel* model,int n, int freq, double* season_);
BModel *initModel(struct _ts *ts);
int countError(BModel *m,double error);
BModel * initEmptyModel();
void Solve(BModel *m);
void Clean(BModel *m);
double EvalB(BModel *m,int x);
void PrintBModel(BModel *m);
void Incremental();
extern BModel *extra;
#endif
