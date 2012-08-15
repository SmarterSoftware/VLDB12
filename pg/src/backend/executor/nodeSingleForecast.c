/*-------------------------------------------------------------------------
 *
 * nodeSingleForecast.c
 *	  Routines to handle forecasting.
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/executor/nodeForecast.c,v 1.0 2009/08/26$
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "funcapi.h"
#include "utils/date.h"
#include "executor/execdebug.h"
#include "executor/nodeSingleForecast.h"
#include "utils/numeric.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/formatting.h"
#include "utils/timestamp.h"
#include "catalog/pg_model.h"
#include "catalog/pg_type.h"
#include "nodes/nodeFuncs.h"
#include "forecast/algorithm.h"
#include "forecast/modelindex/modelIndex.h"
#include "forecast/methods/forecastUtilitys.h"
#include "forecast/methods/ExpSmooth.h"


/* ----------------------------------------------------------------
 *		ExecForecast
 * ----------------------------------------------------------------
 */
TupleTableSlot *
ExecSingleForecast(SingleForecastState *forecastState)
{
	PlanState 		*outerPlan;
	TupleTableSlot 	*outSlot = NULL;
	Datum 			*values;
	bool 			*isnull;
	HeapTuple 		tuple;
	TupleDesc 		outerTupDesc;
	int				i;
	double 			value;
	ModelInfo 		*modelInfo;
	AttrNumber 		attrNumber;
	
	// we needn't do all the execution, if we just created a model
	if(forecastState->end == -2){

		forecastState->end = -100;
		return forecastState->ss.ps.ps_ResultTupleSlot;
	}

	//for testing purpose only, should be set through grammar
	modelInfo=(((ModelInfo*)(lfirst(list_head(forecastState->candidateModels)))));

	attrNumber = modelInfo->measure->resno - 1;

	// get information from the node
	outerPlan 		= outerPlanState(forecastState);



	if(forecastState->timestamp < forecastState->end)
	{
		if(exprType((Node*)modelInfo->time->expr) == DATEOID || exprType((Node*)modelInfo->time->expr) == TIMESTAMPOID)
			forecastState->timestamp = DatumGetTimeADT(DirectFunctionCall2(timestamp_pl_interval, TimestampGetDatum(forecastState->timestamp), IntervalPGetDatum(forecastState->interval)));
		else
			forecastState->timestamp += forecastState->interval->time;

		outerTupDesc = forecastState->ss.ps.ps_ResultTupleSlot->tts_tupleDescriptor;

		// init output tuple
		values = (Datum *) palloc0(outerTupDesc->natts * sizeof(Datum));
		isnull = (bool *) palloc0(outerTupDesc->natts * sizeof(bool));
		for (i=0; i<outerTupDesc->natts; i++) {
			isnull[i] = true;
		}
		

		if(forecastState->ss.ps.ps_ResultTupleSlot->tts_tupleDescriptor->attrs[modelInfo->time->resno-1]->atttypid==exprType((Node*) modelInfo->time->expr))
		{
			// set time column
			values[modelInfo->time->resno-1] = GetIntAsDatum(exprType((Node*) modelInfo->time->expr), forecastState->timestamp);


			isnull[modelInfo->time->resno-1] = false;
			
			// set measure column
			//set strathook here!
			value = forecastState->modelMergeStrategy(forecastState->candidateModels, forecastState->count++);
			values[modelInfo->measure->resno-1] = GetDoubleAsDatum(exprType((Node*)modelInfo->measure->expr), value);
			isnull[modelInfo->measure->resno-1] = false;
		}
		else
		{
				// set time column
			values[modelInfo->measure->resno-1] = GetIntAsDatum(exprType((Node*) modelInfo->time->expr), forecastState->timestamp);


			isnull[modelInfo->measure->resno-1] = false;
			
			// set measure column
			//set strathook here!
			value = forecastState->modelMergeStrategy(forecastState->candidateModels, forecastState->count++);
			values[modelInfo->time->resno-1] = GetDoubleAsDatum(exprType((Node*)modelInfo->measure->expr), value);
			isnull[modelInfo->time->resno-1] = false;
		}

		if(modelInfo->confidenceIntervalPercentage>0)
		{
			values[2] = Float8GetDatum(modelInfo->model->calculateTrustedInterval(modelInfo->model, forecastState->count, modelInfo->confidenceIntervalPercentage));
			isnull[2] = false;
		}
		// create output
		tuple = heap_form_tuple(outerTupDesc, values, isnull);
		outSlot=forecastState->ss.ps.ps_ResultTupleSlot;
		ExecStoreTuple(tuple,outSlot,InvalidBuffer,true);
	}
	return outSlot;
}

int64 extractTheEndForTimeStamps(int64 startTimestamp, Granularity modelGranularity, Granularity targetGranularity, int altLength, Interval **i){

	Interval *myInterval = (Interval *) palloc0(sizeof(Interval));

	if(modelGranularity > targetGranularity)
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE), errmsg("The demanded granularity is not allowed in combination with the actual model-granularity, probably it is too small!")));

				switch(targetGranularity)
	{
	case hour:
		myInterval->time = USECS_PER_HOUR * altLength;
		break;
	case day:
		myInterval->day = 1 * altLength;
		break;
	case week:
		myInterval->day = 7 * altLength;
		break;
	case month:
		myInterval->month = 1 * altLength;
		break;
	case quarter:
		myInterval->month = 3 * altLength;
		break;
	case year:
		myInterval->month = MONTHS_PER_YEAR * altLength;
		break;
	}

	*i = (Interval *) palloc0(sizeof(Interval));
	//find the right granularity of the used Model and the granularity of demanded interval an calculate the end-timestamp AND the step to add in every forecast
	switch(modelGranularity)
	{
	case hour:
	{
		(*i)->time = USECS_PER_HOUR;
		break;
	}
	case day:
	{
		(*i)->day = 1;
		break;
	}
	case week:
	{
		(*i)->day = 7;
		break;
	}
	case month:
	{
		(*i)->month = 1;
		break;
	}
	case quarter:
	{
		(*i)->month = 3;
		break;
	}
	case year:
	{
		(*i)->month = MONTHS_PER_YEAR;
		break;
	}
	}

	startTimestamp  = DatumGetTimestamp(DirectFunctionCall2(timestamp_pl_interval, TimestampGetDatum(startTimestamp), IntervalPGetDatum(myInterval)));

	return startTimestamp;
}

/* ----------------------------------------------------------------
 *		ExecInitForecast
 *
 *		Creates the run-time state information for the forecast node
 *		produced by the planner and the node's subplan.
 * ----------------------------------------------------------------
 */
SingleForecastState *
ExecInitSingleForecast(SingleForecast *node, EState *estate, int eflags)
{
	SingleForecastState 	*forecastState;
	Plan	   				*outerPlan;
	TupleTableSlot			*resultSlot;
	ModelInfo				*modelInfo;

	/*
	 * create state structure
	 */
	forecastState = makeNode(SingleForecastState);
	forecastState->ss.ps.plan = (Plan *) node;
	forecastState->ss.ps.state = estate;
	forecastState->modelMergeStrategy = getMergeStrat(node->choose);
	//copy all models
	forecastState->candidateModels = node->candidatemodelInfos;
	forecastState->sourcetext = node->sourcetext;
	forecastState->count=1;
	forecastState->count = 1;

	if(((ModelInfo*)(lfirst(list_head(node->candidatemodelInfos))))->buildModel) {

		outerPlan = outerPlan(node);
		outerPlanState(forecastState) = ExecInitNode(outerPlan, estate, eflags);

		//first create the model if we have to (eflag==1 means this is an expplainState, so we MUSTN'T EXECUTE ANYTHING)
		if (eflags != 1)
			resultSlot = ExecProcNode(outerPlanState(forecastState));
		else
			resultSlot=NULL;
               
	}


	modelInfo = ((ModelInfo*)(lfirst(list_head(node->candidatemodelInfos))));
	forecastState->timestamp = modelInfo->timestamp;
	/*
	 * Despite we checked this already in the planner, we still need this check here if the model creation is implicit.
	 * In this case there is only one Model in the candidatelist!
	 */
	if(node->end == -1)
	{
		if(node->targetDateString)//a forecast until a certain time is demanded
		{
			if(exprType((Node*)modelInfo->time->expr) == DATEOID)
			{
				forecastState->timestamp = DatumGetTimestamp(DirectFunctionCall1(date_timestamp, DateADTGetDatum(forecastState->timestamp)));

				forecastState->end = DatumGetTimestamp(DirectFunctionCall1(date_timestamp, DirectFunctionCall1(date_in, CStringGetDatum(node->targetDateString))));
			}
			else //then it's TIMESTAMPOID
			{
				forecastState->end = DatumGetInt64(DirectFunctionCall3(timestamp_in, CStringGetDatum(node->targetDateString), ObjectIdGetDatum(InvalidOid), Int32GetDatum(-1)));
			}

			if(DatumGetBool(DirectFunctionCall2(timestamp_le, TimestampGetDatum(forecastState->end), TimestampGetDatum(forecastState->timestamp)))) //targetDate is not prospective enough
			{
				ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE), errmsg("TargetTimestamp must be greater than last value of the training-data!")));
			}

			//is only used to set the right step, overpowered but it works
			node->targetGranularity = modelInfo->granularity;
			extractTheEndForTimeStamps(forecastState->timestamp, modelInfo->granularity, node->targetGranularity, node->altLength, &forecastState->interval);
		}
		else//a forecast for a certain interval is demanded
		{
			if(exprType((Node*)modelInfo->time->expr) == DATEOID)
			{
				forecastState->timestamp = DatumGetTimestamp(DirectFunctionCall1(date_timestamp, DateADTGetDatum(forecastState->timestamp)));
			}

			forecastState->end = extractTheEndForTimeStamps(forecastState->timestamp, modelInfo->granularity, node->targetGranularity, node->altLength, &forecastState->interval);
		}
	}
	else if(node->end > 0)
	{
		if(exprType((Node*)modelInfo->time->expr) == DATEOID || exprType((Node*)modelInfo->time->expr) == TIMESTAMPOID)
		{
			if(exprType((Node*)modelInfo->time->expr) == DATEOID)
			{
				forecastState->timestamp = DatumGetTimeADT(DirectFunctionCall1(date_timestamp, DateADTGetDatum(forecastState->timestamp)));
			}
			//is only used to set the right step, overpowered but it works
			node->targetGranularity = modelInfo->granularity;
			forecastState->end = extractTheEndForTimeStamps(forecastState->timestamp, modelInfo->granularity, node->targetGranularity, node->end, &forecastState->interval);

		}
		else
		{
			forecastState->interval = (Interval *) palloc0(sizeof(Interval));
			forecastState->interval->time = 1;
			forecastState->end += forecastState->timestamp + node->end;
		}
	}
			
        
	/*
	 * tuple table initialization
	 */
	#define FORECAST_NSLOTS 1
	ExecInitResultTupleSlot(estate, &forecastState->ss.ps);
	
	/*
	 * forecast nodes do no projections, so initialize projection info for this
	 * node appropriately
	 */
	ExecAssignResultTypeFromTL(&forecastState->ss.ps);
	// if we just created a model we only return a part of the modelInfo
	if(node->end == -2 && eflags!=1)
	{
		forecastState->ss.ps.ps_ResultTupleSlot = resultSlot;
		forecastState->end = -2;
	}

	forecastState->ss.ps.ps_ProjInfo = NULL;
	return forecastState;
}


int
ExecCountSlotsSingleForecast(SingleForecast *node)
{
	return ExecCountSlotsNode(outerPlan(node)) +
		ExecCountSlotsNode(innerPlan(node)) +
		FORECAST_NSLOTS+1;
}


/* ----------------------------------------------------------------
 *		ExecEndForecast(node)
 * ----------------------------------------------------------------
 */
void
ExecEndSingleForecast(SingleForecastState *node)
{
	ListCell *lc;
	/*
	 * clean out the tuple table
	 */
	if(node->ss.ps.ps_ResultTupleSlot)
		ExecClearTuple(node->ss.ps.ps_ResultTupleSlot);
	
	/*
	 *	free ModelNodes 
	 */
	
	foreach(lc,node->candidateModels)
	{
		pfree(lfirst(lc));
	}
	
	/*
	 * shut down the subplan
	 */
	ExecEndNode(outerPlanState(node));
        
}
