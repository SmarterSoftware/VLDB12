/*-------------------
* Claudio Hartmann	s2922905@mail.zih.tu-dresden.de
* Christopher Schildt	s2309365@mail.zih.tu-dresden.de
*--------------------
*/

#include "forecast/modelGraph/modelGraph.h"
#include "forecast/modelGraph/modelGraphReceiver.h"
#include "forecast/modelGraph/greedyAlgorithm.h"
#include "forecast/methods/ExpSmooth.h"
#include "nodes/nodeFuncs.h"
#include "utils/portal.h"
#include "utils/datum.h"
#include "utils/guc.h"
#include <time.h>

WorkloadElement *leastCommonParent;

char	*conf;
int 	actualTuple = 0;

/*
 * Returns a List* of WorkloadElements.
 */
List *
ExtractWorkload(void){
	//for the beginning just get all the childNodes of the ModelGraph, simulating a single reference of every Node
	//TODO: extract the relevant nodes from the workload, a separate structure will be needed: WorkloadElement{Node, relativeCount, Horizon}
	List		*init = GetAllChildren(NULL, true),
				*res = NIL;
	ListCell	*lc;
	Horizon 	*h;

	foreach(lc, init)
	{
		WorkloadElement	*w = palloc0(sizeof(WorkloadElement));
		w->mgin = (ModelGraphIndexNode *)lfirst(lc);
		h = palloc0(sizeof(Horizon));
		h->error = 0.0;
		h->horizon = 1;
		h->frequency = 1.0/list_length(init);
		h->stillCheck = true;
		w->horizons = lcons(h, w->horizons);

//		res = lcons(w, res);
		res = lappend(res, w);
	}

	return res;
}

/*
 * Returns the Error for the given configuration.
 *
 * conf - the configuration to evaluate, consisting o error values
 * lcpError - the Error of the configuration when only the least-common-parent owns a Model
 * modelCnt - the actual count of Models in the configuration (WITHOUT THE lcpModel)
 * maxModels - the maximum number of Models the configuration can own (|elementsConsidered|) (INCLUDING the lcpModel)
 */
double
EvaluateConfiguration(double *conf, double lcpError, int modelCnt, int maxModels){

	/*
	 *    sum(conf)           modelCnt
	 *  a*--------- + (1-a)*------------- (modelCnt already ignores the lcp-Model)
	 *    lcpError          maxModels - 1
	 */

	int i;
	double confError = 0.0,
			res = 0.0;

	for(i=0; i<maxModels; ++i){
		confError += conf[i];
	}
	
	
	
	res = ((alpha*(confError)/lcpError) + ((1.0-alpha)*(((double)modelCnt)/((double)maxModels-1))));


	return res;
}


/*
 * Executes a Query to fetch all Tuples that fit elem and subsequently trains a model for elem.
 *
 * stmt - the FillModelGgraphStmt the training_data are extracted from
 * elem - the WorkloadElement that receives the Tuples an the trained Model
 */
void
FillWorkloadElementWithValuesAndModelInfo(FillModelGraphStmt *stmt, WorkloadElement *elem, List *workload){

	ModelGraphIndexNode	*iter = elem->mgin;
	WorkloadElement		*targetElem;
	ModelInfo			*modelInfo;
	List				*helpList = NIL,
						*exprList = NIL,
						*query_list = NIL,
						*planned_list = NIL,
						*tleDummyList;
	ListCell			*lc;
	A_Expr				*aexp;
	SelectStmt	 		*selStmt;
	TargetEntry			*tle;
	RangeTblEntry		*rte;
	Var					*v;
	char				*commandTag,
						*colName;
	Portal				portal;
	DestReceiver 		*tupledest;
	int16				resultTypLen;
	bool				resultTypByVal;
	int					errCalcTuples;
	ParseState 			*pstate;
	StringInfo 			name;
	char				*res,
						*sourceCpy;
	int					braketCount = 1,
						length = 0,
						helpcounter=0,
						actualWLE = 0;
	bool				importantContent = false;
	char				*ptr2;
	int64				time1,
						time2;

	//if the user entered some predicates within the training_data, kill 'em all, except they are JOINS
	if(((SelectStmt *)stmt->algorithm->trainingdata)->whereClause){
		exprList = ExtractJoins(((SelectStmt *)stmt->algorithm->trainingdata)->whereClause);
		elog(INFO, "All nonJOIN-predicates of the training_data have been deleted!");
	}

	while(iter->parents)
	{
		//if iter is an AggNode, don't add a predicate to the NEW whereClause
		if(iter->type != T_AggNode)
			exprList = lappend(exprList, CreateA_Expr(iter->target, iter->value));

		if(((ModelGraphIndexNode *)arlfirst(arlist_head(iter->parents)))->type == T_IndexNode){
			iter = (ModelGraphIndexNode *)arlfirst(arlist_head(iter->parents));
		}else{
			iter = (ModelGraphIndexNode *)arlfirst(arlist_tail(iter->parents));
		}
	}

	switch(list_length(exprList)){
	case 0:
		aexp = NULL;
		break;
	case 1:
		aexp = lfirst(list_head(exprList));
		break;
	default:
		aexp = RestructureA_Expr(exprList);
		break;
	}

	//instantiate a SelecctStmt to perform the ForecastStmt with, plan it and execute it with a Portal
	selStmt = copyObject(stmt->algorithm->trainingdata);
	selStmt->whereClause = (Node *)aexp;

	query_list = pg_analyze_and_rewrite((Node *)selStmt, stmt->sourcetext, NULL, 0);

	tleDummyList = ((Query *)lfirst(list_head(query_list)))->targetList; //use the TargetList of the rewritten Query to find the aggregated Measurecol
	pstate = make_parsestate(NULL);
	pstate->p_sourcetext = stmt->sourcetext;
	pstate->p_variableparams = false;

	//for ColumnRefs with prepended table
	pstate->p_relnamespace = ((Query *)lfirst(list_head(query_list)))->rtable;
	//for ColumnRefs without prepended table
	pstate->p_varnamespace = ((Query *)lfirst(list_head(query_list)))->rtable;
	//to obtain RTEs
	pstate->p_rtable = ((Query *)lfirst(list_head(query_list)))->rtable;

	//transform measure to TargetEntry
	//at this point these are still ColumnRefs!, but handling as TargetEntry saves some ugly casting
	tle = (TargetEntry *)stmt->measure;
	colName = ((Value *)lfirst(list_tail(((ColumnRef *)stmt->measure)->fields)))->val.str;
	//find the TargetEntry that fits the actual graphAttribute
	tle = findTargetlistEntry(pstate, (Node *)tle, &tleDummyList, ORDER_CLAUSE);

	if(!IsA(tle->expr, Aggref))//it is a TargetEntry
		elog(ERROR,"Meassurecolumn must be aggregated");
	else //Aggref case
		v = (Var *)lfirst(list_head(((Aggref *)tle->expr)->args));

	rte = (RangeTblEntry *)list_nth(pstate->p_rtable, v->varno-1);
	//forward the ColumnName an the RelationOid, so we don`t need any other structures later(it's not done automatically!)
	tle->resname = palloc0((strlen(colName)+1)*sizeof(char));
	tle->resname = strcpy(tle->resname,colName);
	tle->resorigtbl = rte->relid;

	//create the forecastmodel for elem
	name = makeStringInfo();
	appendStringInfo(name, "m%i", elem->mgin->id);
	modelInfo = initModelInfo((const char*)stmt->algorithm->algorithmname, (const char*)name->data,stmt->maintenanceMethod,stmt->maintenanceParameters);
	modelInfo->measure = tle;
	modelInfo->forecastMethod = getStringAsModelType(stmt->algorithm->algorithmname);
	modelInfo->storeModel = 4;

	//TODO: setzen der Standartparameter in initModel verlagern
	if(stmt->algorithm->algorithmparameter==NIL && (modelInfo->forecastMethod==GretlArima || modelInfo->forecastMethod==ArModel))
	{
		List 				*paraMeterList=NIL;
		A_Const 			*arConst;
		Value 				*arVal;
		AlgorithmParameter	*maP;
		A_Const 			*maConst;
		Value 				*maVal;
		AlgorithmParameter	*arP = makeNode(AlgorithmParameter);
		arP->key="ar";
		arConst = makeNode(A_Const);
		arConst->location=105;
		arVal = makeNode(Value);
		arVal->type=T_Integer;
		arVal->val.ival=1;
		arConst->val=*arVal;
		arP->value=arConst;
		paraMeterList=lappend(paraMeterList,arP);
		maP = makeNode(AlgorithmParameter);
		maP->key="ma";
		maConst = makeNode(A_Const);
		maConst->location=105;
		maVal = makeNode(Value);
		maVal->val.ival=0;
		maVal->type=T_Integer;
		maConst->val=*maVal;
		maP->value=maConst;
		paraMeterList=lappend(paraMeterList,maP);

		modelInfo->parameterList=paraMeterList;
	}else{
		modelInfo->parameterList = copyObject(stmt->algorithm->algorithmparameter);
	}

	//copy the trainingdata, need these to calculate the disAggKeys

	sourceCpy = palloc(strlen(stmt->sourcetext)+1);
	sourceCpy = strcpy(sourceCpy, stmt->sourcetext);

	while(sourceCpy[length])
	{

		if(importantContent)
		{
			length++;
			if((sourceCpy[length])=='\'')
				importantContent=false;

			continue;
		}
		else
		{
			char test='\'';
			(sourceCpy[length])=tolower(sourceCpy[length]);
			length++;
			if((sourceCpy[length])==test)
				importantContent=true;
		}

	}
	length=0;

	sourceCpy = strstr(sourceCpy, "training_data");

	sourceCpy = strpbrk(sourceCpy, "(");
	sourceCpy++;
	res = sourceCpy;
	while(braketCount)
	{
		if(*res == '(')
			braketCount++;
		else if(*res == ')')
			braketCount--;
		res++;
		length++;
		if(*res == '\0' && braketCount)
			elog(ERROR,"MUHAHAHA WITHOUT ME THIS COULD SEGFAULT HERE");
	}
	modelInfo->trainingData = palloc0(length);
	modelInfo->trainingData = strncpy(modelInfo->trainingData, sourceCpy, length-1);
	modelInfo->trainingData[length-1] = '\0';
	ptr2 = modelInfo->trainingData;

	while(*ptr2)
	{
		if(*ptr2=='\n')
			*ptr2=' ';
		ptr2++;
	}

	//MEASURE-CHANGE
	errCalcTuples = configTplCnt;

	planned_list = pg_plan_queries(query_list, 0, NULL);
	// create command Tag
	commandTag = "SELECT";

	// results are not needed here but only one Tuple is returned, so the overhead is bearable
	tupledest = CreateModelGraphDestReceiver();

	// Create a new portal to run the query in
	portal = CreateNewPortal();

	//Don't display the portal in pg_cursors, it is for internal use only
	portal->visible = false;

	PortalDefineQuery(portal, NULL, stmt->sourcetext, commandTag, planned_list, NULL);

	//  Start the portal.  No parameters here.
	PortalStart(portal, NULL, InvalidSnapshot);

	(void) PortalRun(portal, FETCH_ALL, false, tupledest, tupledest, NULL);

	//tupledest now contains the complete timeseries
	helpList = ((ModelGraphState *)tupledest)->tupleList;

	//train the model with all tuples we got, but save some to calculate the configuration
	initForecastModel(modelInfo, getModelGraphFillingContext());

	actualTuple = 0;

	foreach(lc, helpList)
	{
		processForecastModel(modelInfo, ((Datum *)lfirst(lc))[modelInfo->measure->resno-1]);

		//this way it should be faster than a for-loop with usage of list_nth
		if(++actualTuple>=list_length(helpList)-errCalcTuples)
			break;
	}

	finalizeForecastMode(modelInfo, false);

	//XXX: SEHR +/-1-Fehleranfällig, check actualTuple AND helpcounter

	elem->measureValues = palloc0(sizeof(Datum) * list_length(helpList)-actualTuple);
	elem->sizeOfMeasureValues = list_length(helpList)-actualTuple;
	for(helpcounter=actualTuple ;helpcounter<list_length(helpList); ++helpcounter){
		get_typlenbyval(v->vartype, &resultTypLen, &resultTypByVal);
		elem->measureValues[helpcounter-actualTuple] = datumCopy(((Datum *)list_nth(helpList, helpcounter))[modelInfo->measure->resno-1], resultTypByVal, resultTypLen);
	}

























	//at this point these are still ColumnRefs!, but handling as TargetEntry saves some ugly casting
	tle = (TargetEntry *)stmt->time;
	colName = ((Value *)lfirst(list_tail(((ColumnRef *)stmt->time)->fields)))->val.str;
	//find the TargetEntry that fits the actual graphAttribute
	tle = findTargetlistEntry(pstate, (Node *)tle, &tleDummyList, ORDER_CLAUSE);

	if(!IsA(tle->expr, Aggref))//it is a TargetEntry
		v = (Var *)tle->expr;
	else //Aggref case
		v = (Var *)lfirst(list_head(((Aggref *)tle->expr)->args));

	rte = (RangeTblEntry *)list_nth(pstate->p_rtable, v->varno-1);
	//forward the ColumnName an the RelationOid, so we don`t need any other structures later(it's not done automatically!)
	tle->resname = palloc0((strlen(colName)+1)*sizeof(char));
	tle->resname = strcpy(tle->resname,colName);
	tle->resorigtbl = rte->relid;

	modelInfo->time = tle;

	//calculate the model granualrity
	time1 = GetDatumAsInt(exprType((Node*) modelInfo->time->expr), ((Datum *)list_nth(helpList,0))[modelInfo->time->resno-1]);
	time2 = GetDatumAsInt(exprType((Node*) modelInfo->time->expr), ((Datum *)list_nth(helpList,1))[modelInfo->time->resno-1]);

	modelInfo->granularity = extractGranularity(exprType((Node*) modelInfo->time->expr), time1, time2);

	elem->modelInfo = modelInfo;

	(*tupledest->rDestroy) (tupledest);
	PortalDrop(portal, false);

	elem->keyNumerators = palloc0(sizeof(double) * (modelGraphIdx->maxid+1));
	elem->disAggErrors = palloc0(sizeof(double) * (modelGraphIdx->maxid+1));
	elem->disAggErrorForLaterUse = palloc0(sizeof(double) * (modelGraphIdx->maxid+1));

	//find all nodes that can be disaggregated from elem
	helpList = GetAllDisaggregaties(elem->mgin);

	//calculate the numerator-part of the DisAggKey
	foreach(lc, workload)
	{
		targetElem = lfirst(lc);

		if(!list_member_ptr(helpList, targetElem->mgin))
		{
			elem->keyNumerators[actualWLE] = 0.0;
			++actualWLE;
			continue;
		}

		if(elem->mgin->aggIndicator > targetElem->mgin->aggIndicator){

			//get the numerator-part of the DisAggKey
			elem->keyNumerators[actualWLE] = targetElem->modelInfo->disAggKeyDenominator;
		}
		else //otherwise enter 0.0
		{
			elem->keyNumerators[actualWLE] = 0.0;
		}
		++actualWLE;
	}

	if(aexp)
		FreeA_Expr(aexp, false);

	list_free_deep(query_list);
	list_free_deep(planned_list);
	list_free(exprList);
}

List *GetAllDisaggregaties(ModelGraphIndexNode *elem){

	ModelGraphIndexNode 	*helpMgin = elem;
	List					*helpList = NIL,
							*referenceList = NIL,
							*helpList3 = NIL,
							*helpList4 = NIL;
	ListCell				*lc1,
							*lc2;
	ArListCell				*alc1;
	Var						*v1;
	int						levelCount = 0;

	//put the Mgins from Root to elem in a referenceList
	while(arlist_length(helpMgin->parents)>0){
		referenceList = lcons(helpMgin, referenceList);
		helpMgin = arlinitial(helpMgin->parents);
	}

	//prepare helpList3, this List will be used for collecting all relevant values of the next level of the ModelGraph
	helpList3 = lappend(helpList3, getModelGraphRoot(true));

	//walk down the ModelGraph Level by Level
	do{
		if(helpList){
//			list_free(helpList);
			helpList = NIL;
		}

		foreach(lc1, helpList3)//foreach ModelGraphIndexNode in helpList3...
		{
			if(helpList4)
			{
//XXX: missing free, but it will help to survive
//				list_free(helpList4);
				helpList4 = NIL;
			}

			//...collect all children...
			arforeach(alc1, ((ModelGraphIndexNode *)lfirst(lc1))->children)
					helpList4 = lappend(helpList4, arlfirst(alc1));
			//...including the aggChild...
			helpList4 = lappend(helpList4, ((ModelGraphIndexNode *)lfirst(lc1))->aggChild);

			//...and check if a DisAggregation is still possible, and in case it is, add it to helpList
			foreach(lc2, helpList4){

				//is the referencenode in referenceList an AggNode, just take the nodes of helpList4...
				if(((ModelGraphIndexNode *)list_nth(referenceList, levelCount))->type == T_AggNode){
					if(((!(list_nth(referenceList, levelCount) == lfirst(lc2))) || (((ModelGraphIndexNode *)lfirst(lc2))->children)))//...but exclude the node himself
					{
						helpList = lappend(helpList, lfirst(lc2));
					}
				}
				else //otherwise check if the actual node matches the value of the reference node in referenceList (AggNodes are skipped)
				{
					//skip AggNodes
					if(((ModelGraphIndexNode *)lfirst(lc2))->type == T_AggNode)
						continue;

					v1 = getTEVar(((ModelGraphIndexNode *)list_nth(referenceList, levelCount))->target);
					if(compareDatum(((ModelGraphIndexNode *)list_nth(referenceList, levelCount))->value, ((ModelGraphIndexNode *)lfirst(lc2))->value, v1->vartype) &&
							!((list_nth(referenceList, levelCount) == lfirst(lc2)) && !(((ModelGraphIndexNode *)lfirst(lc2))->children)) &&
							((ModelGraphIndexNode *)llast(referenceList))->aggIndicator > ((ModelGraphIndexNode *)lfirst(lc2))->aggIndicator)
					{
						helpList = lappend(helpList, lfirst(lc2));
					}
				}
			}
		}
//		list_free(helpList3);
		helpList3 = list_copy(helpList);

		levelCount++;
	}while(helpList3 && ((ModelGraphIndexNode *)linitial(helpList3))->children); //while the lowest level in the ModelGraph is not reached

	releaseModelGraphRoot();

	return helpList;
}

/*
 * Calculates the ModelError and the DisAggKeys & -Error for elem.
 *
 * elem - the WorkloadElement the stuff is calculated for
 * workload -
 */
void
FillWorkloadElementWithDisAggSchemesAndErrors(WorkloadElement *elem, List *workload){

	ListCell			*lc1,
						*lc2;
	WorkloadElement		*targetElem;
	Var					*vElem;
	int					actualWLE = 0; //counts the Workloadelements
	Horizon				*horizon;
	int					actualTupleInWindow,
						windowCount = 1;
	double				t,
						f,
						sumPerWindow = 0.0;
	List				*helpList = NIL;

	/*
	 * THIS IS EXTREMELY SENSITIVE, never change anything at the newMaigcVariable or actualTuple without life-threatening demand of it
	 */
	int newMagicVariable;









	vElem = getTEVar(elem->modelInfo->measure);

	//find all nodes that can be disaggregated from elem
	helpList = GetAllDisaggregaties(elem->mgin);


	//calculate the forecastError of the Model of elem via SMAPE with MovingWindows on the last 10% of the timeseries and in the same way the DisAggErrors
	/*
	 *        /   /|f_h-t_h|\ \
	 *       |sum|-----------| |
	 *    sum| h  \ f_h+t_h /  |
	 *     w |-----------------|
	 *        \       h       /
	 *    ----------------------
	 *               w
	 */



	for(newMagicVariable=0	;newMagicVariable<elem->sizeOfMeasureValues; ++newMagicVariable)
	{

		if(elem->horizons)
		{
			//first the ForecastError of the Model of elem for the GreedyAlgorithm
			foreach(lc1, elem->horizons)
			{
				horizon = (Horizon *)lfirst(lc1);
				if(!horizon->stillCheck)
					continue;

				if(elem->sizeOfMeasureValues-newMagicVariable-horizon->horizon<0)
				{
					horizon->stillCheck=false;
					continue;
				}
				sumPerWindow = 0;
				for(actualTupleInWindow=0; actualTupleInWindow<horizon->horizon; ++actualTupleInWindow)
				{
					t = GetDatumAsDouble(vElem->vartype, elem->measureValues[newMagicVariable + actualTupleInWindow]);
					f = getNextValue(elem->modelInfo, actualTupleInWindow+1);

					sumPerWindow += SMAPE(f,t);
				}

				horizon->error = ((horizon->error*(windowCount-1))+(sumPerWindow/horizon->horizon))/windowCount;
			}
		}

		//second the disAggErrors
		if (elem->mgin->aggIndicator>0)
		{
			actualWLE = 0;
			foreach(lc1, workload)//foreach element in Worklaod
			{
				targetElem = lfirst(lc1);
				//targetElem can't be disaggregated from elem
				if(!list_member_ptr(helpList, targetElem->mgin))
				{
					++actualWLE;
					continue;
				}

				if (elem->keyNumerators[actualWLE]>0)
				{
					foreach(lc2, targetElem->horizons)//foreach horizon of the actual WorkloadElement
					{
						horizon = (Horizon *)lfirst(lc2);

						//mark the horizon as not to be checked if there are no tuples left to calculate an error with
						if(elem->sizeOfMeasureValues-newMagicVariable-horizon->horizon<0)
						{
							horizon->stillCheck=false;
							continue;
						}
						sumPerWindow = 0;
						for(actualTupleInWindow=0; actualTupleInWindow<horizon->horizon; ++actualTupleInWindow)
						{
							t = GetDatumAsDouble(vElem->vartype, targetElem->measureValues[newMagicVariable + actualTupleInWindow]);
//							elog(INFO, "Real:%f", t);
							//get the disaggregated forecastvalue
//							elog(INFO, "OEst:%f", getNextValue(elem->modelInfo, actualTupleInWindow+1));
//							elog(INFO, "OKey:%f", elem->keyNumerators[actualWLE]);
//							elog(INFO, "UKey:%f", elem->keyDenominator);
//							elog(INFO, "Key:%f", elem->keyNumerators[actualWLE]/elem->keyDenominator);
							f = getNextValue(elem->modelInfo, actualTupleInWindow+1) * (elem->keyNumerators[actualWLE]/elem->modelInfo->disAggKeyDenominator);
//							elog(INFO, "Est:%f", f);

							sumPerWindow += SMAPE(f,t);
						}

						horizon->error = ((horizon->error*(windowCount-1))+(sumPerWindow/horizon->horizon))/windowCount;
					}

					//update the keyNumerator with the recently checked tuple
					t = GetDatumAsDouble(vElem->vartype, targetElem->measureValues[newMagicVariable]);
					elem->keyNumerators[actualWLE] += t;
				}

				++actualWLE;
			}
		}

		//after the surrounding loop the model is completely updated, but parameters have to be reestimated
		incrementalUpdate(elem->modelInfo, GetDatumAsDouble(vElem->vartype, elem->measureValues[newMagicVariable]), elem->modelInfo->timestamp+1);

		windowCount++;
	}

	if(elem->horizons){
		foreach(lc1, elem->horizons)
		{
			elem->disAggErrorForLaterUse[elem->mgin->id] += ((Horizon *)lfirst(lc1))->error;
			elem->modelInfo->errorSMAPE += ((((Horizon *)lfirst(lc1))->error)*((Horizon *)lfirst(lc1))->frequency);
		}
		elem->disAggErrorForLaterUse[elem->mgin->id] = elem->disAggErrorForLaterUse[elem->mgin->id]/list_length(elem->horizons);
		elem->modelInfo->errorSMAPE = elem->modelInfo->errorSMAPE/list_length(elem->horizons);
//		elog(INFO, "ModellFehler: %f", elem->disAggErrorForLaterUse[elem->mgin->id]);
	}

	actualWLE = 0;
	foreach(lc1, workload){
		targetElem = lfirst(lc1);

		if(!list_member_ptr(helpList, targetElem->mgin))
		{
			//put the ModelError of the Node himself to its corresponding place in its DisAggErrors
			//(this seems weird but is usefull for the evaluation)
			if(targetElem->mgin == elem->mgin)
				elem->disAggErrors[actualWLE] += elem->modelInfo->errorSMAPE;

			++actualWLE;
			continue;
		}

//TODO: KOMMENTAR
		foreach(lc2, targetElem->horizons)
		{
			horizon = lfirst(lc2);
			elem->disAggErrorForLaterUse[actualWLE] += horizon->error;
			elem->disAggErrors[actualWLE] += ((horizon->error)*horizon->frequency);
			horizon->error = 0.0;
			horizon->stillCheck = true;
		}
//		elog(INFO, "ERR:%f", elem->disAggErrorForLaterUse[actualWLE]);
		elem->disAggErrorForLaterUse[actualWLE] /= list_length(targetElem->horizons);
		elem->disAggErrors[actualWLE] /= list_length(targetElem->horizons);
		++actualWLE;
	}
//	elog(INFO, "HLL:%i", list_length(helpList));

	list_free(helpList);
}

double
meanOfDoubleArray(double *array, int arrayLength)
{
	double	res = 0;
	int 	i;
	for(i=0; i<arrayLength; ++i){
		res += array[i];
	}
	return res/arrayLength;
}

void
FreeWorkloadElement(WorkloadElement *elem){

	int i;
	Var	*v;
	bool	typByVal;
	int16		typLen;

	v=getTEVar(elem->modelInfo->measure);
	get_typlenbyval(v->vartype, &typLen, &typByVal);
	for(i=0; i<elem->sizeOfMeasureValues; ++i)
	{
		datumFree(elem->measureValues[i], typByVal, typLen);
	}

	pfree(elem->disAggErrorForLaterUse);
	pfree(elem->disAggErrors);
	list_free_deep(elem->horizons);
	pfree(elem->keyNumerators);
	pfree(elem->measureValues);
	if(!elem->isInBestConf && elem->mgin->id!=modelGraphIdx->maxid)
		freeModelInfo(elem->modelInfo);

	pfree(elem);

}

void
FillModelGraphGreedy(FillModelGraphStmt *stmt){

	List				*elementsConsidered = ExtractWorkload(), //List of WorkloadElements
						*evalConf = GetAllChildren(NULL, true),
						*helpList2=NIL;
	ArList				*helpList = ARNIL;

	double				*currentConf,
						*bestConf,
						*memConf,
						lcpOnlyConfError = 0,
						bestEval = alpha,
						currentEval,
						confErr = 0.0;
	ListCell			*lc1,
						*lc2,
						*lc3;
	ArListCell			*alc1;
	int					maxModels,
						modelCnt = 1,
						i,
						actualWLE;
	bool				stop = false;
	WorkloadElement		*currentElem,
						*newModel; //the best Model in one iteration over all elementsConsidered

	ModelGraphIndexNode	*iter = getModelGraphRoot(true),
						*flag = NULL;
	bool 				lcpFound = false;
	DisAggModel			*dam;

	leastCommonParent = palloc0(sizeof(WorkloadElement));
	conf = palloc0(sizeof(char)*(modelGraphIdx->maxid+1));
	for(i=0;i<list_length(elementsConsidered)-1;++i)
	{
		conf[i]='0';
	}

	if(!(((ModelGraphIndexNode *)arlinitial(iter->children))->children)){
		lcpFound = true;
	}

	while(!lcpFound){
		helpList = arlist_copy(iter->children);
		helpList = arlappend(helpList, iter->aggChild);

			arforeach(alc1, helpList){
				helpList2 = GetAllChildren(arlfirst(alc1), true);
				foreach(lc2, helpList2){
					foreach(lc3, elementsConsidered){
						if(lfirst(lc2) == ((WorkloadElement *)lfirst(lc3))->mgin){
							if(flag){
								lc2 = list_tail(helpList2);
								alc1 = arlist_tail(helpList);
								lcpFound = true;
								break;
							}

							flag = arlfirst(alc1);
							lc2 = list_tail(helpList2);
							break;
						}
					}
				}
			}


		if(!lcpFound)
			iter = flag;

		arlist_free(helpList);
	}

	leastCommonParent->mgin = iter;

	while(leastCommonParent->mgin->aggChild)
	{
		leastCommonParent->mgin = leastCommonParent->mgin->aggChild;
	}

	//if leastCommonParent is in the workload we have to pay attention on this, otherwise its errors won't be measured in the Greedy-Algo
	lcpFound = false;
	foreach(lc1, elementsConsidered){
		if(((WorkloadElement *)lfirst(lc1))->mgin == leastCommonParent->mgin)
		{
			leastCommonParent = lfirst(lc1);
			lcpFound = true;
			break;
		}
	}

	//now elementsConsidered has its final length
	currentConf = palloc0(list_length(elementsConsidered) * sizeof(double));
	bestConf = palloc0(list_length(elementsConsidered) * sizeof(double));
	memConf = palloc0(list_length(elementsConsidered) * sizeof(double));

	maxModels = list_length(elementsConsidered);

	//create a Model for the least-common-parent
	if(!lcpFound)
		FillWorkloadElementWithValuesAndModelInfo(stmt, leastCommonParent, elementsConsidered);

	//create the Models for all elementsConsidered
	foreach(lc1, elementsConsidered){
		FillWorkloadElementWithValuesAndModelInfo(stmt, lfirst(lc1), elementsConsidered);
	}

	//calculated the errors for lcp and elementsConsidered
	if(!lcpFound)
		FillWorkloadElementWithDisAggSchemesAndErrors(leastCommonParent, elementsConsidered);

	foreach(lc1, elementsConsidered){
		FillWorkloadElementWithDisAggSchemesAndErrors(lfirst(lc1), elementsConsidered);
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////// XXX: ERRORS AND DISAGGERRORS in den WLElements...

	//StringInfo data1 = makeStringInfo();
	//foreach(lc1, elementsConsidered){
	//	WorkloadElement *wl=lfirst(lc1);
	//	appendStringInfo(data1, "%lf\t",wl->modelInfo->);		
	//}
	//appendStringInfo(data1, "\n");
	//printDebug2("/home/schildt/DIPLOM/Daten/Sales/model_und_disaggFehler.txt",data1->data);		
//	foreach(lc1, elementsConsidered){
//		WorkloadElement *wl=lfirst(lc1);
//		StringInfo data1 = makeStringInfo();
//		for(i=0; i<list_length(elementsConsidered); ++i)
//		{
//			appendStringInfo(data1, "%lf\t",wl->disAggErrorForLaterUse[i]);
//		}
//		appendStringInfo(data1, "\n");
//		printDebug2("/home/schildt/DIPLOM/Daten/Sales/model_und_disaggFehler.txt",data1->data);
//	}



	//use the errors of the lcp as initial configuration
	for(i=0; i<list_length(elementsConsidered); ++i)
	{
		bestConf[i] = leastCommonParent->disAggErrors[i];
		lcpOnlyConfError += leastCommonParent->disAggErrors[i];
	}

//					StringInfo data1 = makeStringInfo();
//					appendStringInfo(data1, "%lf\n", lcpOnlyConfError);
//					printDebug("/home/hartmann/Desktop/BelegSVN/ClaudioHartmann/data/Potentialanalyse/lcpError.txt",data1->data);

//	elog(INFO, "lcpOnlyConfError: %lf", lcpOnlyConfError);

	while(!stop){
		stop = true;

		newModel = NULL;

		memcpy(currentConf, bestConf, sizeof(double)* maxModels);
		memcpy(memConf, bestConf, sizeof(double)* maxModels);

		//find the most advanced Model in all elementsConsidered
		foreach(lc1, elementsConsidered)//foreach Node in elementsConsidered
		{
			currentElem = lfirst(lc1);

			if (!(currentElem->isInBestConf))//don't check an element of elementsConsidered again, if its already in the configuration
			{
				//in currentConf alles einfügen was in lc1 besser ist
				for(i=0; i<list_length(elementsConsidered); ++i)
				{
					//check the DisAggregationErrors
					if((currentElem->disAggErrors[i] > 0) && (currentElem->disAggErrors[i] < currentConf[i]))
						currentConf[i] = currentElem->disAggErrors[i];
				}


				//check the ArrgregationErrors

//				foreach(lc2, ((ModelGraphIndexNode *)linitial(currentElem->mgin->parents))->children)//for every sibling node of currentElem
//				{
//					allSiblings = allSiblings;
//				}
				//TODO:nur prüfen, wenn auch Geschwister-Knoten da sind

				//TODO:Wenn sich ein Knoten Aggregieren lässt, prüfen ob das auch mit seinen geschwistern geht



//TODO: AN DIESER STELLE NUN NOCH DIE AGGREGATIONEN ÜBERPRÜFEN, UND FEHLER REINNEHMEN; WENN ER BESSER IST
//TODO: DABEI AUCH DEN LCP MIT BEACHTEN, aber nur, wenn er vorher im Worklaod war


				//evaluate the current configuration by evaluateConfiguration
				currentEval = EvaluateConfiguration(currentConf, lcpOnlyConfError, modelCnt, maxModels);

				//if the current configuration is better than the best
				if(currentEval<bestEval)
				{
					memcpy(bestConf, currentConf, sizeof(double)* maxModels);
					bestEval = currentEval;
					newModel = (WorkloadElement *)lfirst(lc1);
					stop = false;
				}

				//reset currentConf for the next iteration
				memcpy(currentConf, memConf, sizeof(double)* maxModels);
			}
		}
		//remove that element from elementsConsidered which delivered the best new Model (it's no real removal, we just mark it as "don't look at it again", but we don't tell anybody*hihihi*)
		if(newModel){
			newModel->isInBestConf = true;
			modelCnt++;

			conf[newModel->mgin->id] = '1';
		}
	}
	conf[list_length(elementsConsidered)-1] = '1';

	leastCommonParent->modelInfo->mix = (Node *)leastCommonParent->mgin;
	leastCommonParent->modelInfo->errorSMAPE = leastCommonParent->disAggErrorForLaterUse[leastCommonParent->mgin->id];
	ReestimateModelGraphModel(leastCommonParent->modelInfo);

	leastCommonParent->mgin->models = lappend(leastCommonParent->mgin->models, leastCommonParent->modelInfo);
	updateModelGraphIdx(leastCommonParent->modelInfo);
//TODO: this seems to be the right place to calculate the borders for the relative errors that determine when to restart the Greedy-Algo
	//set all DisAggModels that DisAggregate from the LCP
	actualWLE = 0;
	foreach(lc1, elementsConsidered)//foreach elementsConsidered
	{
		if(((WorkloadElement *)lfirst(lc1))->mgin == leastCommonParent->mgin)
		{
			actualWLE++;
			continue;
		}

		dam = palloc0(sizeof(DisAggModel));
		dam->disAggKeyNumerator = leastCommonParent->keyNumerators[actualWLE];
		dam->model = leastCommonParent->modelInfo;
		dam->errorSMAPE = leastCommonParent->disAggErrorForLaterUse[actualWLE];
		dam->upperBound = leastCommonParent->modelInfo->upperBound;
		dam->lowerBound = leastCommonParent->modelInfo->lowerBound;
		dam->sizeOfErrorArray = 0;
		dam->errorArray = palloc0(dam->upperBound*sizeof(double));

		((WorkloadElement *)lfirst(lc1))->mgin->disAggModels = lappend(((WorkloadElement *)lfirst(lc1))->mgin->disAggModels, dam);
		actualWLE++;
	}

	foreach(lc1, elementsConsidered)//foreach elementsConsidered
	{
		currentElem = lfirst(lc1);

		if (currentElem->isInBestConf)//don't check an element of elementsConsidered that don't delivered a good Model
		{
			//put the Model of the current element to the ModelGraph
			currentElem->modelInfo->mix = (Node *)currentElem->mgin;
			currentElem->modelInfo->errorSMAPE = currentElem->disAggErrorForLaterUse[currentElem->mgin->id];
			ReestimateModelGraphModel(currentElem->modelInfo);

			currentElem->mgin->models = lappend(currentElem->mgin->models, currentElem->modelInfo);

			updateModelGraphIdx(currentElem->modelInfo);

			actualWLE = 0;
			foreach(lc2, elementsConsidered)//enter every elementConsidered in the ModelGraph
			{
				//don't build a DisAggModel for the Mgin himself
				if(((WorkloadElement *)lfirst(lc2))->mgin == currentElem->mgin){
//					if((currentElem->disAggErrorForLaterUse[actualWLE] > 0) && currentElem->mgin->disAggModels && (currentElem->disAggErrorForLaterUse[actualWLE] <= ((DisAggModel *)linitial(currentElem->mgin->disAggModels))->errorSMAPE)){
//						//this won't work for multiple DisAggModels in one  MGIN
//						currentElem->mgin->disAggModels=NIL;
//					}
					actualWLE++;
					continue;
				}

				if((currentElem->disAggErrors[actualWLE] > 0) && (currentElem->disAggErrors[actualWLE] == bestConf[actualWLE])){

					((DisAggModel *)linitial(((WorkloadElement *)lfirst(lc2))->mgin->disAggModels))->model = currentElem->modelInfo;
					((DisAggModel *)linitial(((WorkloadElement *)lfirst(lc2))->mgin->disAggModels))->disAggKeyNumerator = currentElem->keyNumerators[actualWLE];
					((DisAggModel *)linitial(((WorkloadElement *)lfirst(lc2))->mgin->disAggModels))->errorSMAPE = currentElem->disAggErrorForLaterUse[actualWLE];
					((DisAggModel *)linitial(((WorkloadElement *)lfirst(lc2))->mgin->disAggModels))->upperBound = currentElem->modelInfo->upperBound;
					((DisAggModel *)linitial(((WorkloadElement *)lfirst(lc2))->mgin->disAggModels))->lowerBound = currentElem->modelInfo->lowerBound;
					((DisAggModel *)linitial(((WorkloadElement *)lfirst(lc2))->mgin->disAggModels))->sizeOfErrorArray = 0;
					pfree(((DisAggModel *)linitial(((WorkloadElement *)lfirst(lc2))->mgin->disAggModels))->errorArray);
					((DisAggModel *)linitial(((WorkloadElement *)lfirst(lc2))->mgin->disAggModels))->errorArray = palloc0(currentElem->modelInfo->upperBound*sizeof(double));
				}
				actualWLE++;
			}
		}
	}

	foreach(lc1, evalConf){
		flag = lfirst(lc1);
		if(flag->models){
			if(flag->disAggModels){
				confErr += Min(((ModelInfo *)linitial(flag->models))->errorSMAPE, ((DisAggModel *)linitial(flag->disAggModels))->errorSMAPE);
				//			elog(INFO, "1+2: %f", Min(((ModelInfo *)linitial(flag->models))->errorSMAPE, ((DisAggModel *)linitial(flag->disAggModels))->errorSMAPE));
			}
			else{
				confErr += ((ModelInfo *)linitial(flag->models))->errorSMAPE;
				//			elog(INFO, "1: %f", ((ModelInfo *)linitial(flag->models))->errorSMAPE);
			}
		}else{
			confErr += ((DisAggModel *)linitial(flag->disAggModels))->errorSMAPE;
			//		elog(INFO, "2: %f", ((DisAggModel *)linitial(flag->disAggModels))->errorSMAPE);
		}
	}


//	StringInfo output = makeStringInfo();
//	appendStringInfo(output, "%lf\t%lf\n", ((confErr/list_length(evalConf))),((modelGraphIdx->models-1.0)));
//	printDebug2("/home/schildt/Documents/ChristopherSchildt/Diplom/Daten/Sales/greedy_Eval_alpha05.dat",output->data);
//	pfree(output);


	elog(INFO, "--------------------------------------------------");
	elog(INFO, "standardized configuration-error: %f", confErr/list_length(evalConf));
	//	elog(INFO, "LCP-Error: %f", lcpOnlyConfError);
	elog(INFO, "final result of the evaluation-function: %f", bestEval);
	//elog(INFO, "configuration: %s", conf);
	elog(INFO, "created models: %i", modelGraphIdx->models);
	elog(INFO, "--------------------------------------------------");

	setConfError(confErr);

	//set the startvalue for restartCounter for the GreedyAlgo
	modelGraphIdx->reBound = modelGraphIdx->modellist[0]->upperBound;

	if(!modelGraphIdx->restartInsCnt)
		modelGraphIdx->restartInsCnt = modelGraphIdx->reBound*2;


//	StringInfo data = makeStringInfo();
	//appendStringInfo(data, "%f\n", confErr);
	//printDebug("/home/hartmann/Desktop/BelegSVN/ClaudioHartmann/data/Potentialanalyse/potentialAnalyse.txt",data->data);

//	resetStringInfo(data);
//	appendStringInfo(data, "%s\n", conf);
//	printDebug("/home/hartmann/Desktop/BelegSVN/ClaudioHartmann/data/Potentialanalyse/potentialAnalyseConfigs.txt",data->data);
//	pfree(data);

	foreach(lc1, elementsConsidered)
	{
		FreeWorkloadElement((WorkloadElement *)lfirst(lc1));
	}

	releaseModelGraphRoot();

}
