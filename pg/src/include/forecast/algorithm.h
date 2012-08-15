/*--------------------------------------------------------------------------
 * algorithm.h
 *	  header file for forecast algorithm interface.
 *
 *
 *	$PostgreSQL: pgsql/src/include/forecast/algorithm.h,v 1.34 2009/06/11 14:49:08 momjian Exp $
 *--------------------------------------------------------------------------
 */
#ifndef ALGORITHM_H
#define ALGORITHM_H

#include "postgres.h"
#include "fmgr.h"
#include "nodes/nodes.h"
#include "nodes/primnodes.h"
#include "executor/tuptable.h"
#include <nlopt.h>
#include "utils/snapshot.h"
#include "maintenance.h"


typedef enum ModelType {
	Undefined = 0,
	Medium = 1,
	R = 2,
	LinReg = 3,
	HwModel = 4,
	GretlArima = 5,
	ArModel = 6
} ModelType;

typedef enum Granularity {
	hour,
	day,
	week,
	month,
	quarter,
	year
} Granularity;



 
 
typedef struct AlgorithmInfoData
{
	const char		*algName;				/* name of the algorithm */
	FmgrInfo		algInitModel;
	FmgrInfo		algLoadModelParameters;
	FmgrInfo		algProcessForecast;
	FmgrInfo		algFinalizeForecastModel;
	FmgrInfo		algStoreModelParameters;
	FmgrInfo		algGetNextValue;
	FmgrInfo		algIncrementalUpdate;
	FmgrInfo		algIncrementalUpdate2;
	FmgrInfo		algReestimateUpdate;
}AlgorithmInfoData;

typedef struct AlgorithmInfoData *AlgorithmInfo;


typedef struct Model
{
	NodeTag		type;
	int			trainingTupleCount;
	double 		(*calculateTrustedInterval)(struct Model*, int, double);
} Model;




typedef struct ModelInfo
{
	NodeTag					type;
	
	// forecast method
	ModelType				forecastMethod;
	MemoryContext			modelMemCtx;
	
	// optional name of this model
	const char				*modelName;
	
	// meta information
	TargetEntry				*time;
	TargetEntry				*measure;
	Oid						aggType;
	Granularity				granularity;
	
	// timestamp until this model was built/home/schildt/Documents/postgres/src/include/forecast
	int64					timestamp;

	// associated model parameters
	Model					*model;
	
	// number of values to forecast
	int						numForecastValues;
	
	// needs this model to be build?
	bool					buildModel;
	
	// where to store this model; 0: no storage, 1: model index (main memory), 2: hash table(main memory), 3: system table
	short					storeModel;
	
	// storage specific information; pointer to node in model index OF in the modelgraph
	Node					*mix;
	
	// temporary
	Oid						modelOid;
	
	// A generic List of parsed Parameters
	List					*parameterList;

	// The Info to the algorithm which created this model
	AlgorithmInfo			algInfo;

	char					*trainingData;
	double					disaggkey; //used in the executor

	double					disAggKeyDenominator;

	double					errorSMAPE;
	double					errorSSE;
	double					errorML;

	int						lowerBound;
	int						upperBound;
	int						sizeOfErrorArray;
	double					*errorArray;
	double					**otherErrors;
	
	int						deletionTicketCnt;
	struct ModelInfo		**errorsForDropingDecision;

	struct MaintenanceInfo	*maintenance;
	
	double					confidenceIntervalPercentage;

} ModelInfo;


typedef struct MediumModel
{
	//Model			model;
	NodeTag			type;
	int				count;
	double 			(*calculateTrustedInterval)(struct Model*, int, double);

	double			sum;
} MediumModel;


typedef struct RModel
{
	Model			model;
	
	char*			functionName;
	int				period;
	int				numForecastValues;
	Datum			*newvalues;
	
	// used for value storage
	Datum	 	*memvalues;		/* array of pointers to palloc'd values */
	int			memtupcount;	/* number of values currently present */
	int			memtupsize;		/* allocated length of memvalues array */
	long		availMem;		/* remaining memory available, in bytes */
} RModel;


typedef struct LinRegModel
{
	//Model			model;
	NodeTag			type;
	int				count;
	double 			(*calculateTrustedInterval)(struct Model*, int, double);

	double			yTotal;
	double			xyTotal;
	double			a;
	double			b;
} LinRegModel;


typedef struct AdditiveDec
{
	Model		model;
	int			tupcount;
	double 		(*calculateTrustedInterval)(struct Model*, int, double);

	int			windowSize;
	int			period;
	int			currentPos;
	double		*sums;
	short		status;
	short		even;

	double	 	*values;
	double		valuessize;
	double		*trend;
	double		trendsize;
	double		*season;
	double		*rest;
} AdditiveDec;


typedef double(*ModelMergeStrategy)(List*,int);

/*
 *
 * Interface definition for the forecast algorithms. Every Forecast Algorithm has to implement
 * these methods and has to register their implementation in the system catalog (pg_proc and pg_fcalg)
 *
 */
extern TupleTableSlot *BuildModelInfoTupleTableSlot(ModelInfo *modelInfo);
extern ModelMergeStrategy getMergeStrat(int stratIdentifier);
extern double firstCandidate(List* candidates,int current);
extern double lastCandidate(List* candidates,int current);
extern double averageCand(List* candidates,int current);
extern double sumCand(List* candidates,int current);
extern double sumWithDisaggCand(List* candidates,int current);
extern double modelGraphSum(List* candidates,int current);
extern ModelInfo *initModelInfo(const char *algName,const char *modelName,int maintenanceMethod,List* maintenanceParameters);
extern void freeModelInfo(ModelInfo *modelInfo);
extern void initForecastModel(ModelInfo *modelInfo, MemoryContext memoryContext);
extern void processForecastModel(ModelInfo *model, Datum value);
extern void finalizeForecastModel(ModelInfo *model);
extern void finalizeForecastMode(ModelInfo *model, bool final);
extern void storeModelParameters(ModelInfo *model, Oid modelOid);
extern void loadModelParameters(ModelInfo *model, Oid modelOid);
extern double getNextValue(ModelInfo *model, int num);
extern void incrementalUpdate(ModelInfo *modelInfo, double value, int64 timestamp);
extern void reestimateParameters(ModelInfo *modelInfo, Node *whereExpr);

extern ModelType getStringAsModelType(char* type);

extern char* getModelTypeAsString(ModelType type);

extern char* getGranularityAsString(Granularity type);
extern Datum GetDoubleAsDatum(Oid source, double value);
extern Datum GetIntAsDatum(Oid source, int64 value);
extern double GetDatumAsDouble(Oid source, Datum value);
extern int64 GetDatumAsInt(Oid source, Datum value);

extern double SSE(double xhat, double x);
extern double SMAPE(double xhat, double x);
extern double ABS(double xhat, double x);

void printDebug(const char * filePath, char *message);
void printDebug2(const char * filePath, char *message);
extern nlopt_algorithm getOptimMethod(int value);
double inverse_normal_cdf(double p);
/*
 *
 * Utility Methods
 *
 */
extern AlgorithmInfo initAlgorithmInfo(const char *algName);

extern Granularity extractGranularity(Oid timeType, int64 time1, int64 time2);


#endif   /* ALGORITHM_H */
