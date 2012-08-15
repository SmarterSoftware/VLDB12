/*-------------------
* Claudio Hartmann	s2922905@mail.zih.tu-dresden.de
* Christopher Schildt	s2309365@mail.zih.tu-dresden.de
*--------------------
*/

 #include "forecast/maintenance.h"
 #include "forecast/modelGraph/modelGraph.h"
 #include "forecast/modelGraph/greedyAlgorithm.h"
 #include "forecast/methods/commonARIMA.h"
 #include "utils/guc.h"
 #include "forecast/methods/armodel.h"
 #include "forecast/algorithm.h"
 #include "nodes/nodes.h"
 
 /*
  * called before Modelcreation!
  * palloc + function pointers are set
  */
 void initMaintenanceMethod(ModelInfo *model,int maintenanceMethod,List* maintenanceParameters)
 {
	 MemoryContext old = NULL;
	 if(isModelGraphExistent())
	 {
		 old=MemoryContextSwitchTo(getModelGraphFillingContext());
	 }
	 else
	 {
//		 elog(WARNING,"Maintenance only implemented for modelgraph, but no modelgraph exists");
	 }
	 switch(maintenanceMethod)
	 {
		 case 0	: initEmptyMaintenance(model,maintenanceParameters); break;
		 case 1 : initSimpleReestMaintenance(model,maintenanceParameters);break;
		 case 2 : initArima_ondemandMaintenance(model,maintenanceParameters);break;
		 default: initSimpleReestMaintenance(model,maintenanceParameters);break;
	 }

	 if(old)
		 MemoryContextSwitchTo(old);
 }
 
void initArima_ondemandMaintenance(ModelInfo* model,List* maintenanceParameters)
 {
		 model->maintenance=(MaintenanceInfo*)makeNode(ArOnDemandMaintenance);
		 model->maintenance->insert=&_insertArima_ondemandMaintenance; //called for every insert
		 model->maintenance->triggerMaintenance=&_triggerArima_ondemandMaintenance; //not used yet
		 model->maintenance->initMaintenance=&_initArima_ondemandMaintenance; //called after Modelcreation
		 ((ArOnDemandMaintenance*)(model->maintenance))->varianzCount=0;
		 ((ArOnDemandMaintenance*)(model->maintenance))->parameter=copyObject(maintenanceParameters);

 }
 
 
 int extractMaintenanceIntParameter(List* parameterList, char* searchKey)
 {
	 	 	 ListCell				*cell;
	
	foreach(cell,parameterList) {
		AlgorithmParameter		*param = lfirst(cell);
		/* parse maximum AR lag */
		if(strcmp(param->key,searchKey) == 0) {
			if(IsA(&(param->value->val),Integer)) {
				elog(INFO,"------------ %li",intVal(&param->value->val));
				return intVal(&param->value->val);
			} else
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("Parameter value has to be an Float value"),
						 errposition(param->value->location)));
		}
		
 }
		//key not found set Default Value
		if(strcmp("use_variance",searchKey) == 0)
		{
//			elog(WARNING,"No use_var set, using default value 1");
			return 1;
		}
		if(strcmp("reestinscnt",searchKey) == 0)
		{
//			elog(WARNING,"reestInsCnt, using default value upperbound");
			return -1;
		}
		if(strcmp("withthresh",searchKey) == 0)
		{
//			elog(WARNING,"withThres, using default value true");
			return 1;
		}
		
		else elog(ERROR,"Missing Paramatervalue :%s",searchKey);

		return -1;
 }
 
  double extractMaintenanceFloatParameter(List* parameterList, char* searchKey)
 {
	 	 ListCell				*cell;
	
	foreach(cell,parameterList) {
		AlgorithmParameter		*param = lfirst(cell);

		/* parse maximum AR lag */
		if(strcmp(param->key,searchKey) == 0) {
			if(IsA(&(param->value->val),Float)) {
				return floatVal(&param->value->val);
			} else
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("Parameter value has to be an Float value"),
						 errposition(param->value->location)));
		}
		
 }
		//key not found set Default Value
		if(strcmp("ssetolerance",searchKey) == 0)
		{
			//elog(WARNING,"No ssetolerance set, using default value 0");
			return 0.0;
		}
		//key not found set Default Value
		if(strcmp("tolerance",searchKey) == 0)
		{
			//elog(WARNING,"No tolerance set, using default value 0.01");

			return 0.001;
		}
		else elog(ERROR,"Missing Paramatervalue :%s",searchKey);

		return -1.0;
 }
 
 
 
 void _initArima_ondemandMaintenance(ModelInfo* model)
 {
	 //Init Structures
	 ArOnDemandMaintenance* aodm=(ArOnDemandMaintenance*)(model->maintenance);
	 int i;
	 double *olduhat;
	 double *var;
	 int	varCount;
	 double tolerance;
	 double varianzAll=0;
	 double (*optFunc)(unsigned n, const double *x, double *grad, void *my_func_data);
	 
	 ARModel *armodel=(ARModel*)model->model;
		elog(INFO,"Calculating Hypercube for model: %s", model->modelName);
	
	if(!aodm->sseHyperCubeUpSide)
		aodm->sseHyperCubeUpSide=palloc0(sizeof(double)*(armodel->super.p+armodel->super.sp+armodel->super.q+armodel->super.sq+armodel->super.includeConstant));
	if(!aodm->sseHyperCubeDownSide) 
	 aodm->sseHyperCubeDownSide=palloc0(sizeof(double)*(armodel->super.p+armodel->super.sp+armodel->super.q+armodel->super.sq+armodel->super.includeConstant));
	if(!aodm->stepwidth)  
	 aodm->stepwidth=palloc0(sizeof(double)*(armodel->super.p+armodel->super.sp+armodel->super.q+armodel->super.sq+armodel->super.includeConstant));
	if(!aodm->hyperCubeUhatsUp)   
	 aodm->hyperCubeUhatsUp=palloc0(sizeof(double*)*(armodel->super.p+armodel->super.sp+armodel->super.q+armodel->super.sq+armodel->super.includeConstant));
	 
	if(!aodm->hyperCubeUhatsDown)    
		aodm->hyperCubeUhatsDown=palloc0(sizeof(double*)*(armodel->super.p+armodel->super.sp+armodel->super.q+armodel->super.sq+armodel->super.includeConstant));
	if(!aodm->hyperSSEBarier)   	
		aodm->hyperSSEBarier=palloc0(sizeof(double)*(armodel->super.p+armodel->super.sp+armodel->super.q+armodel->super.sq+armodel->super.includeConstant));
	
	tolerance=extractMaintenanceFloatParameter(aodm->parameter,"tolerance");
	 
	 for(i=0;i<(armodel->super.p+armodel->super.sp+armodel->super.q+armodel->super.sq);i++)
		 aodm->hyperSSEBarier[i]=extractMaintenanceFloatParameter(aodm->parameter,"ssetolerance");
	 //calculate SSE Hypercube
	 olduhat=armodel->super.uhat;
	 if(armodel->super.q || armodel->super.sq)
		armodel->super.uhat=palloc((Max(armodel->super.q,armodel->super.sq*armodel->super.pd))*sizeof(double)); 
	 var=palloc0(sizeof(double)*(armodel->super.p+armodel->super.sp+armodel->super.q+armodel->super.sq+armodel->super.includeConstant));
	 //create var array
	 memcpy(var,armodel->super.phis, armodel->super.p*sizeof(double));
	 memcpy(&(var[armodel->super.p]),&(armodel->super.phis[armodel->super.p]), armodel->super.sp*sizeof(double));
	 memcpy(&var[armodel->super.p+armodel->super.sp],armodel->super.thetas, armodel->super.q*sizeof(double));
	 memcpy(&var[armodel->super.sp+armodel->super.p+armodel->super.q],&(armodel->super.thetas[armodel->super.q]), armodel->super.sq*sizeof(double));
	 if(armodel->super.includeConstant) {
		var[armodel->super.sp+armodel->super.p+armodel->super.q+armodel->super.sq]=armodel->super.constant[0];
	 }
	 
	 
	 
	 varCount=(armodel->super.p+armodel->super.sp+armodel->super.q+armodel->super.sq+armodel->super.includeConstant);
	 if(aodm->varianzCount==0){
	 	 aodm->value_sum=palloc0(sizeof(double)*(varCount));
		 aodm->quadratic_sum=palloc0(sizeof(double)*(varCount));
	 }
	 //save first expectetValueSum
	 aodm->varianzCount+=1;
	 for(i=0;i<varCount;i++)
	 {
		 aodm->value_sum[i]+=var[i];
		 aodm->quadratic_sum[i]+=var[i]*var[i];
	 }
	 
	 
	 if(aodm->varianzCount<=2)
	 {
		 varianzAll=1;
	 }
	 
	 else
	 {
		 for(i=0;i<varCount;i++)
		 {
			 int n=aodm->varianzCount;
			 //elog(INFO,"%i,%i,%lf,%lf + ---- %lf,%lf",i,n,aodm->quadratic_sum[i],aodm->value_sum[i],aodm->value_sum[4],aodm->value_sum[3]);
			 varianzAll+=(1.0/(n-1))*(aodm->quadratic_sum[i]-(1.0/(n)*aodm->value_sum[i]*aodm->value_sum[i]));
		 }
	 }
	 
	 for(i=0;i<(armodel->super.p+armodel->super.sp+armodel->super.q+armodel->super.sq);i++)
	 {
		 int n=aodm->varianzCount;
		 if(aodm->varianzCount==1 || !extractMaintenanceIntParameter(aodm->parameter,"use_variance"))
			 aodm->stepwidth[i]=tolerance;
		else
			aodm->stepwidth[i]=(1.0/(n-1))*(aodm->quadratic_sum[i]-(1.0/(n)*aodm->value_sum[i]*aodm->value_sum[i]))/(varianzAll*tolerance);
		if(aodm->stepwidth[i]==0)
			aodm->stepwidth[i]=tolerance;
	 }
	 
	 for(i=0;i<varCount;i++)
	 {
		 int n=aodm->varianzCount;
		 elog(INFO,"----------- %i: %lf,%lf,%lf",n, aodm->quadratic_sum[i],(1.0/(n))*aodm->value_sum[i]*aodm->value_sum[i],((1.0/(n-1))*(aodm->quadratic_sum[i]-(1.0/(n))*aodm->value_sum[i]*aodm->value_sum[i])));
		 elog(INFO,"Varianz Node %i: %lf",i,aodm->stepwidth[i]);
	 }
	 
	 
	 if(armodel->super.sp+armodel->super.sq && armodel->super.seasonType==0)
	 {
		 optFunc=&arima_AddSeasonal_CSS;
	 }
	 else if(!(armodel->super.sp+armodel->super.sq))
	 {
		 optFunc=&arima_nonSeasonal_CSS;
	 }
	 else
	 {
		 optFunc=&arima_MulSeasonal_CSS;
	 }
	 
	 armodel->super.error=(*optFunc)(varCount,(const double*)var,NULL,(armodel));
	 for(i=0;i<varCount;i++)
	 {
		if(armodel->super.q || armodel->super.sq)
			armodel->super.uhat=palloc((Max(armodel->super.q,armodel->super.sq*armodel->super.pd))*sizeof(double)); 
		var[i]+=aodm->stepwidth[i];
		if(!(var[i]>=1))
			aodm->sseHyperCubeUpSide[i]=(*optFunc)(varCount,(const double*)var,NULL,(armodel));
		else 
			aodm->sseHyperCubeUpSide[i]=armodel->super.error+1;
		aodm->hyperCubeUhatsUp[i]=armodel->super.uhat;
		
		if(armodel->super.q || armodel->super.sq)
			armodel->super.uhat=palloc((Max(armodel->super.q,armodel->super.sq*armodel->super.pd))*sizeof(double)); 
		var[i]-=(2*aodm->stepwidth[i]);
		if(!(var[i]>=1))
			aodm->sseHyperCubeDownSide[i]=(*optFunc)(varCount,(const double*)var,NULL,(armodel));
		else
			aodm->sseHyperCubeDownSide[i]=armodel->super.error+1;
		aodm->hyperCubeUhatsDown[i]=armodel->super.uhat;
		var[i]+=aodm->stepwidth[i]; //original 
	 }
	  for(i=0;i<(armodel->super.q+armodel->super.sq+armodel->super.p+armodel->super.sp);i++)
	 {
		if(armodel->super.error-aodm->sseHyperCubeUpSide[i]> aodm->hyperSSEBarier[i] || armodel->super.error-aodm->sseHyperCubeDownSide[i] > aodm->hyperSSEBarier[i] )
		{
			elog(WARNING,"THis should not happen, at least nto for every model: %s,%lf,%lf,%lf",model->modelName,armodel->super.error,aodm->sseHyperCubeUpSide[i],aodm->sseHyperCubeDownSide[i]);
		}
	 }
	 armodel->super.uhat=olduhat;
	 pfree(((ARModel*)model->model)->data);
 }
 
 
 double _insertArima_ondemandMaintenance(ModelInfo *model, double t)
 {
	double f;
	 double error;
	 double errorSmape;
	 double *phis,*thetas;
	 ARModel *armodel=(ARModel*)model->model;
	 ArOnDemandMaintenance* aodm=(ArOnDemandMaintenance*)model->maintenance;
	 int i;
	 initArimaForecast(&(armodel->super)); //init model
	 f = _get_next_Arimaforecast(&(armodel->super),armodel->super.phis,armodel->super.thetas,armodel->super.uhat);
	 error = SSE(f,t);
	 aodm->insertCount++;
	 //elog(WARNING,"BLABLA%s: %lf,%lf,%lf::::%p::::%lf,%lf",model->modelName,f,t,error,armodel->super.y,armodel->super.y[0],armodel->super.y[1]);
	 errorSmape=SMAPE(f,t);
	 ((ARModel*)model->model)->super.error+=error;
	 //update hypercube phis
	 phis=armodel->super.phis;
	 for(i=0;i<(armodel->super.p+armodel->super.sp);i++)
	 {	
		phis[i]+=aodm->stepwidth[i];
		if(!(phis[i]>=1)){
			f=_get_next_Arimaforecast(&(armodel->super),phis,armodel->super.thetas,aodm->hyperCubeUhatsUp[i]);
			error = SSE(f,t);
			aodm->sseHyperCubeUpSide[i]+=error;
		}
		else
		{			
			aodm->sseHyperCubeUpSide[i]=((ARModel*)model->model)->super.error+1;
		}
		phis[i]-=2*aodm->stepwidth[i];
		if(!(phis[i]>=1)){
			f=_get_next_Arimaforecast(&(armodel->super),phis,armodel->super.thetas,aodm->hyperCubeUhatsDown[i]);
			error = SSE(f,t);
			aodm->sseHyperCubeDownSide[i]+=error;
		}
		else
		{
			aodm->sseHyperCubeDownSide[i]=((ARModel*)model->model)->super.error+1;
		}
		phis[i]+=aodm->stepwidth[i];
//elog(WARNING,"%lf",aodm->stepwidth[i]);
	 }
	 thetas=armodel->super.thetas;
	//update hypercube thetas
	 for(i=0;i<(armodel->super.q+armodel->super.sq);i++)
	 {
		thetas[i]+=aodm->stepwidth[i];
		if(!(thetas[i]>=1)){
			f=_get_next_Arimaforecast(&(armodel->super),phis,thetas,aodm->hyperCubeUhatsUp[i]);
			error = SSE(f,t);
			aodm->sseHyperCubeUpSide[i]+=error;
		}
		else
		{
			aodm->sseHyperCubeUpSide[i]+=((ARModel*)model->model)->super.error+1;
		}
		thetas[i]-=2*aodm->stepwidth[i];
		if(!(thetas[i]>=1)){
			f=_get_next_Arimaforecast(&(armodel->super),phis,thetas,aodm->hyperCubeUhatsDown[i]);
			error = SSE(f,t);
			aodm->sseHyperCubeDownSide[i]+=error;
		}
		else
		{
			aodm->sseHyperCubeDownSide[i]+=((ARModel*)model->model)->super.error+1;
		}
		thetas[i]+=aodm->stepwidth[i];
		 
	 }
	 
	 for(i=0;i<(armodel->super.q+armodel->super.sq+armodel->super.p+armodel->super.sp);i++)
	 {
		if(armodel->super.error-aodm->sseHyperCubeUpSide[i]> aodm->hyperSSEBarier[i] || armodel->super.error-aodm->sseHyperCubeDownSide[i] > aodm->hyperSSEBarier[i] )
		{
			modelsToReestimate=lappend(modelsToReestimate,model);			
			elog(WARNING,"yap@model %s with insert: %i for hypernode:%i, SSE:%0.10g, hyperUp:%0.10g, hyerDown:%0.10g, hyperbarrier:%0.10g",model->modelName,aodm->insertCount,i,armodel->super.error,aodm->sseHyperCubeUpSide[i],aodm->sseHyperCubeDownSide[i],aodm->hyperSSEBarier[i]);
			break;
		}
		//else
		//	elog(WARNING,"nope@model %s with insert: %i for hypernode:%i, SSE:%0.10g, hyperUp:%0.10g, hyerDown:%0.10g, hyperbarrier:%0.10g",model->modelName,aodm->insertCount,i,armodel->super.error,aodm->sseHyperCubeUpSide[i],aodm->sseHyperCubeDownSide[i],aodm->hyperSSEBarier[i]);
	 }
//elog(WARNING,"------------");
	 _UpdateModelInfoErrorArrayEmpty(model, errorSmape);
	 return errorSmape;
 }
 
 void _triggerArima_ondemandMaintenance()
 {
	 elog(ERROR,"Not implemented.Codekey:maint4");
 }
 
 
 void initEmptyMaintenance(ModelInfo* model,List* maintenanceParameters)
 {
		 model->maintenance=(MaintenanceInfo*)makeNode(EmptyMaintenance);
		 model->maintenance->insert=&_insertEmptyMaintenance; //called for every insert
		 model->maintenance->triggerMaintenance=&_triggerEmptyMaintenance; //not used yet
		 model->maintenance->initMaintenance=&_initEmptyMaintenance; //called after Modelcreation
	

 }
 

double _insertEmptyMaintenance(ModelInfo *model, double t)
 {
	 double f;
	 double error;
	// ARModel *armodel=(ARModel*)model->model;
	 f = getNextValue(model, 1);
	((EmptyMaintenance*)model->maintenance)->insertCount++;
	
	 error = SMAPE(f,t);

	// elog(WARNING,"BLABLA%s: %lf,%lf,%lf::::%p::::%lf,%lf",model->modelName,f,t,error,armodel->super.y,armodel->super.y[0],armodel->super.y[1]);
	  _UpdateModelInfoErrorArrayEmpty(model, error);
	 return error;
	 
 }
 
void _triggerEmptyMaintenance()
 {
	 elog(ERROR,"Not implemented.Codekey:maint2");
 }
 
 void _initEmptyMaintenance(ModelInfo* model)
 {
	 ((SimpleReestMaintenance*)model->maintenance)->reestInsCnt = model->upperBound;
 }
 
 
 void initSimpleReestMaintenance(ModelInfo* model, List* maintenanceParameters)
 {
		 model->maintenance=(MaintenanceInfo*)makeNode(SimpleReestMaintenance);
		 model->maintenance->insert=&_insertSimpleReestMaintenance;   //called for every insert
		 model->maintenance->triggerMaintenance=&_triggerSimpleReestMaintenance; //not used yet
		 model->maintenance->initMaintenance=&_initSimpleReestMaintenance; //called after Modelcreation
		 ((SimpleReestMaintenance*)model->maintenance)->insertCount=0;
		 	 ((SimpleReestMaintenance*)(model->maintenance))->parameter=copyObject(maintenanceParameters);
		 
 }

 double _insertSimpleReestMaintenance(ModelInfo *model, double t)
 {
	 double f;
	 double error;

	 f = getNextValue(model, 1);

	 error = SMAPE(f,t);

	 _UpdateModelInfoErrorArray(model, error);
	 ((SimpleReestMaintenance*)model->maintenance)->insertCount++;
	 return error;
	 
 }
 
void _triggerSimpleReestMaintenance()
 {
	 elog(ERROR,"Not implemented.Codekey:maint2");
 }
 
 void _initSimpleReestMaintenance(ModelInfo* model)
 {
	 
	 int temp;
	 SimpleReestMaintenance* aodm=(SimpleReestMaintenance*)model->maintenance;
	 if ((temp=extractMaintenanceIntParameter(aodm->parameter,"reestinscnt"))>0)
		 {
			 ((SimpleReestMaintenance*)model->maintenance)->reestInsCnt = temp;
			 ((SimpleReestMaintenance*)model->maintenance)->withTime=1;
		 }
	else
		((SimpleReestMaintenance*)model->maintenance)->reestInsCnt = model->upperBound;
		
		
	aodm->withThres=extractMaintenanceIntParameter(aodm->parameter,"withthresh");
		
 }
 
 void _UpdateModelInfoErrorArray(ModelInfo *model, double error){

	if(model->sizeOfErrorArray<model->upperBound && ((SimpleReestMaintenance*)model->maintenance)->insertCount+1 < ((SimpleReestMaintenance*)model->maintenance)->reestInsCnt)//calculate errorValue, but no shift of the array is neccesarry
	{
		model->errorArray[model->sizeOfErrorArray++] = error;
		model->errorSMAPE = meanOfDoubleArray(model->errorArray, model->sizeOfErrorArray);

		if(((SimpleReestMaintenance*)model->maintenance)->insertCount+1==model->upperBound)
		{
			((SimpleReestMaintenance*)model->maintenance)->errorReference = model->errorSMAPE;
		}
	}
	else
	{
		//shift the array before the error is inserted
		
		if(model->sizeOfErrorArray>=model->upperBound)
			memmove(model->errorArray, &(model->errorArray[1]), (model->sizeOfErrorArray-1)*sizeof(double));
		model->errorArray[model->sizeOfErrorArray-1] = error;
		model->errorSMAPE = meanOfDoubleArray(model->errorArray, model->sizeOfErrorArray);

		if(((SimpleReestMaintenance*)model->maintenance)->insertCount+1==modelGraphIdx->reBound && !((SimpleReestMaintenance*)model->maintenance)->withTime)
		{
			if(model->errorSMAPE < ((SimpleReestMaintenance*)model->maintenance)->errorReference)
			{
				elog(INFO, "%s häufiger neuschätzen, war %i", model->modelName, ((SimpleReestMaintenance*)model->maintenance)->reestInsCnt);
				if(((SimpleReestMaintenance*)model->maintenance)->reestInsCnt > model->upperBound)
					((SimpleReestMaintenance*)model->maintenance)->reestInsCnt /= 2;
			}
			else
			{
				elog(INFO, "%s seltener neuschätzen, war %i", model->modelName, ((SimpleReestMaintenance*)model->maintenance)->reestInsCnt);
				((SimpleReestMaintenance*)model->maintenance)->reestInsCnt *= 2;
			}
			((SimpleReestMaintenance*)model->maintenance)->errorReference = model->errorSMAPE;
		}

		//if the error is higher than the referenceError OR enough new Tuples were inserted...
		if((model->errorSMAPE > ((SimpleReestMaintenance*)model->maintenance)->errorReference*(1+(0.05/alpha)) && ((SimpleReestMaintenance*)model->maintenance)->withThres==1 && ((SimpleReestMaintenance*)model->maintenance)->insertCount+1 >= model->upperBound) ||
				((SimpleReestMaintenance*)model->maintenance)->insertCount+1 >= ((SimpleReestMaintenance*)model->maintenance)->reestInsCnt)
		{
			/*
			 * ...add the model to the List of models we want to reestimate if:
			 * 		(1) its actual errorSMAPE is higher than the errorReference*(1+(0.05/alpha)) AND
			 * 			there was a certain amount of INSERTs
			 * 		(2) there were so many INSERTs that reestimation is triggered by time
			 */
			if((model->errorSMAPE > ((SimpleReestMaintenance*)model->maintenance)->errorReference*(1+(0.05/alpha)) && ((SimpleReestMaintenance*)model->maintenance)->insertCount+1 >= model->upperBound))
				elog(INFO, "ReestModel %s because of error!", model->modelName);
			else
				elog(INFO, "ReestModel %s because of time!", model->modelName);

			modelsToReestimate = lappend(modelsToReestimate, model);
			((SimpleReestMaintenance*)model->maintenance)->insertCount = 0;

			//remember the last error to set the size of the next tupleCount to wait for reestimation
			((SimpleReestMaintenance*)model->maintenance)->errorReference = model->errorSMAPE;
		}
	}
}
 void _UpdateModelInfoErrorArrayEmpty(ModelInfo *model, double error){

	if(model->sizeOfErrorArray<model->upperBound)//calculate errorValue, but no shift of the array is neccesarry
	{
		model->errorArray[model->sizeOfErrorArray++] = error;
		model->errorSMAPE = meanOfDoubleArray(model->errorArray, model->sizeOfErrorArray);

		if(((SimpleReestMaintenance*)model->maintenance)->insertCount+1==model->upperBound)
		{
			((SimpleReestMaintenance*)model->maintenance)->errorReference = model->errorSMAPE;
		}
	}
	else
	{
		//shift the array before the error is inserted
		memmove(model->errorArray, &(model->errorArray[1]), (model->sizeOfErrorArray-1)*sizeof(double));
		model->errorArray[model->sizeOfErrorArray-1] = error;
		model->errorSMAPE = meanOfDoubleArray(model->errorArray, model->sizeOfErrorArray);

		if(((SimpleReestMaintenance*)model->maintenance)->insertCount+1==modelGraphIdx->reBound)
		{
			if(model->errorSMAPE < ((SimpleReestMaintenance*)model->maintenance)->errorReference)
			{
				elog(INFO, "%s häufiger neuschätzen, war %i", model->modelName, ((SimpleReestMaintenance*)model->maintenance)->reestInsCnt);
				if(((SimpleReestMaintenance*)model->maintenance)->reestInsCnt > model->upperBound)
					((SimpleReestMaintenance*)model->maintenance)->reestInsCnt /= 2;
			}
			else
			{
				elog(INFO, "%s seltener neuschätzen, war %i", model->modelName, ((SimpleReestMaintenance*)model->maintenance)->reestInsCnt);
				((SimpleReestMaintenance*)model->maintenance)->reestInsCnt *= 2;
			}
			((SimpleReestMaintenance*)model->maintenance)->errorReference = model->errorSMAPE;
		}

		//if the error is higher than the referenceError OR enough new Tuples were inserted...
		if((model->errorSMAPE > ((SimpleReestMaintenance*)model->maintenance)->errorReference*(1+(0.05/alpha)) && ((SimpleReestMaintenance*)model->maintenance)->insertCount+1 >= model->upperBound) ||
				((SimpleReestMaintenance*)model->maintenance)->insertCount+1 >= ((SimpleReestMaintenance*)model->maintenance)->reestInsCnt)
		{

			//remember the last error to set the size of the next tupleCount to wait for reestimation
			((SimpleReestMaintenance*)model->maintenance)->errorReference = model->errorSMAPE;
		}
	}
}

