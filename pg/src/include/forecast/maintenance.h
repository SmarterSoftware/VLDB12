/*--------------------------------------------------------------------------
 * maintenance.h
 *	  header file for forecast maintenance interface.
 *
 *
 *	$PostgreSQL: pgsql/src/include/forecast/maintenance.h$
 *--------------------------------------------------------------------------
 */
#ifndef MAINTENANCE_H
#define MAINTENANCE_H

#include "forecast/algorithm.h"


struct ModelInfo;

typedef struct MaintenanceInfo
{
	NodeTag		type;
	double (*insert)(struct ModelInfo*, double);
	void (*triggerMaintenance)(void);
	void (*initMaintenance)(struct ModelInfo* model);
	List *parameter;
	int 			insertCount;
} MaintenanceInfo;


typedef struct ArOnDemandMaintenance
{
	NodeTag 		type;
	double (*insert)(struct ModelInfo*, double);
	void (*triggerMaintenance)(void);
	void (*initMaintenance)(struct ModelInfo* model);
	List *parameter;
	int 			insertCount;
	//how many INSERTs may affect the model until its parameter are reestimated
	int				reestInsCnt;
		//remembers the error after Greedy to trigger an error-based parameter-reestimation
	double			errorReference;
	//stepwidtharray
	double*			stepwidth;
	//sseSynopseArray
	double*			sseHyperCubeUpSide;
	double*			sseHyperCubeDownSide;
	double**		hyperCubeUhatsUp;
	double**		hyperCubeUhatsDown;
	double*			hyperSSEBarier;
	double			varianzCount;
	double*			value_sum;
	double*			quadratic_sum;
}ArOnDemandMaintenance;



typedef struct EmptyMaintenance
{
	NodeTag 		type;
	double (*insert)(struct ModelInfo*, double);
	void (*triggerMaintenance)(void);
	void (*initMaintenance)(struct ModelInfo* model);
	List *parameter;
	int 			insertCount;
	//how many INSERTs may affect the model until its parameter are reestimated
	int				reestInsCnt;
		//remembers the error after Greedy to trigger an error-based parameter-reestimation
	double			errorReference;
}EmptyMaintenance;

typedef struct SimpleReestMaintenance
{
	NodeTag 		type;
	double (*insert)(struct ModelInfo*, double);
	void (*triggerMaintenance)(void);
	void (*initMaintenance)(struct ModelInfo* model);
	List *parameter;
	int 			insertCount;
	//how many INSERTs may affect the model until its parameter are reestimated
	int				reestInsCnt;
		//remembers the error after Greedy to trigger an error-based parameter-reestimation
	double			errorReference;
	int				withThres;
	int				withTime;
}SimpleReestMaintenance;


extern void initMaintenanceMethod(struct ModelInfo *model,int maintenanceMethod,List* maintenanceParameters);
extern void initSimpleReestMaintenance(struct ModelInfo* model,List* maintenanceParameters);
extern double _insertSimpleReestMaintenance(struct ModelInfo *model, double value);
extern void _triggerSimpleReestMaintenance(void);
extern void _initSimpleReestMaintenance(struct ModelInfo* model);
extern void initEmptyMaintenance(struct ModelInfo* model,List* maintenanceParameters);
extern double _insertEmptyMaintenance(struct ModelInfo *model, double value);
extern void _triggerEmptyMaintenance(void);
extern void _initEmptyMaintenance(struct ModelInfo* model);
extern void _UpdateModelInfoErrorArray(struct ModelInfo *model, double error);
extern void _UpdateModelInfoErrorArrayEmpty(struct ModelInfo *model, double error);
extern void initArima_ondemandMaintenance(struct ModelInfo* model,List* maintenanceParameters);
extern void _initArima_ondemandMaintenance(struct ModelInfo* model);
extern double _insertArima_ondemandMaintenance(struct ModelInfo *model, double t);
extern  int extractMaintenanceIntParameter(List* parameterList, char* searchKey);
extern double extractMaintenanceFloatParameter(List* parameterList, char* searchKey);
extern  void _triggerArima_ondemandMaintenance(void);

#endif   /* MAINTENANCE_H */