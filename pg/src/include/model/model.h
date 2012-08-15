#ifndef MODEL_H
#define MODEL_H
#include <stdio.h>
#include "model/ts.h"
#include "model/bmodel.h"

#define EXPLICIT 0
#define IMPLICIT 2
#define TREND 1
extern double error_level;
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

typedef struct _model DModel;

DModel *models;

int *points;
int newpoints;


//extern BModel *extra;
void LoadModules();
DModel* ReadModel(FILE* f,int j);
double EvalProb(int j,int x,double error);
double Eval(int j,int x, double * error) ;
double GetValue(int x);

double *cache;
int cache_start;
void insertItem(int i);


/*

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
*/

#endif
