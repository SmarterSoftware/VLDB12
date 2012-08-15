#ifndef NODESINGLEFORECAST_H_
#define NODESINGLEFORECAST_H_

#include "nodes/execnodes.h"

extern int	ExecCountSlotsSingleForecast(SingleForecast *node);
extern int64 extractTheEndForDateStamps(int64 startTimestamp, Granularity modelGranularity, Granularity targetGranularity, int altLength, int64 *step);
extern int64 extractTheEndForTimeStamps(int64 startTimestamp, Granularity modelGranularity, Granularity targetGranularity, int altLength, Interval **i);
extern SingleForecastState *ExecInitSingleForecast(SingleForecast *node, EState *estate, int eflags);
extern TupleTableSlot *ExecSingleForecast(SingleForecastState *node);
extern void ExecEndSingleForecast(SingleForecastState *node);


#endif /*NODESINGLEFORECAST_H_*/
