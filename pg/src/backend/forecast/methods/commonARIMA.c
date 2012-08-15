/*-------------------------------------------------------------------------
 *
 * commonARIMA.c
 *	  common functions for all ARIMA Models
 *
 * Portions Copyright (c) 1996-2009, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *	$PostgreSQL: pgsql/src/backend/forecast/methods/commonARIMA.c,v 1.10 2009/08/25
 *
 *-------------------------------------------------------------------------
 */

#include "forecast/methods/commonARIMA.h"

#define INIT_FORECASTS 10
#define EXPANSION_CONSTANT 2

static void expandYArray(ArimaModel *forecastModel,int to);




/*
 * return the next one-step ahead forecast for the state specified in the forecast model
 */
double get_next_forecast(ArimaModel *forecast_model)
{
	double 			ynew = forecast_model->constant[1];
	int				i;
	int 			p = forecast_model->p;
	int				q = forecast_model->q;
		int 			sp = forecast_model->sp;
	int				sq = forecast_model->sq;
		int				pd = forecast_model->pd;
	int				start = forecast_model->actualForecast;
	double			mu = forecast_model->constant[1];
	double			*phis = forecast_model->phis;
	double			*thetas = forecast_model->thetas;
	int jump=Max(forecast_model->p,forecast_model->sp*pd);

if(forecast_model->ycount <= Max(forecast_model->p,forecast_model->sp*pd)+forecast_model->actualForecast)
		expandYArray(forecast_model,forecast_model->ycount*EXPANSION_CONSTANT);
		
	if((forecast_model->p>0 ||forecast_model->q>0) &&(forecast_model->sp==0 && forecast_model->sq==0) )
	/* expand the array of the function values if necessary */
	{

	/* compute the AR contribution to the forecast */
	for(i = 0;i<p;i++) {
		ynew += phis[i] * (forecast_model->y[start+jump-(i*1)-1] - mu);
		//elog(WARNING,"-------------------- %lf",forecast_model->y[start+jump-(i*1)-1]);
	}
	
	//elog(WARNING,"\n\n");
	/* compute the MA contribution to the forecast */
	for(i = 0;i+start<q;i++) {
		ynew += thetas[i+start] * forecast_model->uhat[i];
	}

	/*
	 * store result for further forecast
	 */
	forecast_model->y[start+forecast_model->p+(forecast_model->sp*forecast_model->pd)] = ynew;

	/* advance point in time */
	forecast_model->actualForecast++;
	}
	//seasonal arima
	else if(forecast_model->seasonType==0) //seasonal additive
	{
	/* compute the AR contribution to the forecast */
	for(i = 0;i<sp;i++) {
		ynew += phis[i+p] * (forecast_model->y[start+jump-(i*pd)-pd] - mu);
	}

	for(i = 0;i<p;i++) {
		ynew += phis[i] * (forecast_model->y[start+jump-(i*1)-1] - mu);
	}
	/* compute the MA contribution to the forecast */
	for(i = 0;i+start<sq;i++) {
		ynew += thetas[i+start+q] * forecast_model->uhat[(i*pd)+pd];
	}
	for(i = 0;i+start<q;i++) {
		ynew += thetas[i+start] * forecast_model->uhat[i];
	}

	/*
	 * store result for further forecast
	 */
	forecast_model->y[start+jump] = ynew;

	/* advance point in time */
	forecast_model->actualForecast++;
	}
	else {
		elog(ERROR,"looks like a multiplicative model, cannot calculate this yet");
	}

	//nonseasonal arima
	if(forecast_model->d)
	{
		for(i=0;i<forecast_model->d;i++)
		{
			ynew+=forecast_model->diffhelp_temporal_ns[i];
			forecast_model->diffhelp_temporal_ns[i]=ynew;
		}

	}
		//nonseasonal arima
	if(forecast_model->sd)
	{
		for(i=0;i<forecast_model->sd;i++)
		{
			ynew+=forecast_model->diffhelp_temporal_s[i][0];
			forecast_model->diffhelp_temporal_s[forecast_model->sd-1-i][forecast_model->pd]=ynew;
			memmove(&(forecast_model->diffhelp_temporal_s[i][0]),&(forecast_model->diffhelp_temporal_s[i][1]),(forecast_model->pd)*sizeof(double));
		}
		
	
	}
	return ynew;
}


/*
 * return the next one-step ahead forecast for special cases, internal use only
 */
double _get_next_Arimaforecast(ArimaModel *forecast_model,double* phis,double* thetas,double* uhat)
{
	double 			ynew = forecast_model->constant[1];
	int				i;
	int 			p = forecast_model->p;
	int				q = forecast_model->q;
		int 			sp = forecast_model->sp;
	int				sq = forecast_model->sq;
		int				pd = forecast_model->pd;
	int				start = 0;
	double			mu = forecast_model->constant[1];
	int jump=Max(forecast_model->p,forecast_model->sp*pd);

if(forecast_model->ycount <= Max(forecast_model->p,forecast_model->sp*pd)+0)
		expandYArray(forecast_model,forecast_model->ycount*EXPANSION_CONSTANT);
		
	if((forecast_model->p>0 ||forecast_model->q>0) &&(forecast_model->sp==0 && forecast_model->sq==0) )
	/* expand the array of the function values if necessary */
	{

	/* compute the AR contribution to the forecast */
	for(i = 0;i<p;i++) {
		ynew += phis[i] * (forecast_model->y[start+jump-(i*1)-1] - mu);
	}

	/* compute the MA contribution to the forecast */
	for(i = 0;i+start<q;i++) {
		ynew += thetas[i+start] * uhat[i];
	}

	/*
	 * store result for further forecast
	 */
	forecast_model->y[start+forecast_model->p+(forecast_model->sp*forecast_model->pd)] = ynew;


	}
	//seasonal arima
	else if(forecast_model->seasonType==0) //seasonal additive
	{
	/* compute the AR contribution to the forecast */
	for(i = 0;i<sp;i++) {
		ynew += phis[i+p] * (forecast_model->y[start+jump-(i*pd)-pd] - mu);
	}

	for(i = 0;i<p;i++) {
		ynew += phis[i] * (forecast_model->y[start+jump-(i*1)-1] - mu);
	}
	/* compute the MA contribution to the forecast */
	for(i = 0;i+start<sq;i++) {
		ynew += thetas[i+start+q] * uhat[(i*pd)+pd];
	}
	for(i = 0;i+start<q;i++) {
		ynew += thetas[i+start] * uhat[i];
	}

	/*
	 * store result for further forecast
	 */
	forecast_model->y[start+jump] = ynew;

	}
	else {
		elog(ERROR,"looks like a multiplicative model, cannot calculate this yet");
	}

	//nonseasonal arima
	if(forecast_model->d)
	{
		for(i=0;i<forecast_model->d;i++)
		{
			ynew+=forecast_model->diffhelp_temporal_ns[i];
			forecast_model->diffhelp_temporal_ns[i]=ynew;
		}

	}
		//nonseasonal arima
	if(forecast_model->sd)
	{
		for(i=0;i<forecast_model->sd;i++)
		{
			ynew+=forecast_model->diffhelp_temporal_s[i][0];
			forecast_model->diffhelp_temporal_s[forecast_model->sd-1-i][forecast_model->pd]=ynew;
		}
		
	
	}
	return ynew;
}


/*
 * Initializes arrays in the model structure
 * and resets the state of possible previous forecasts
 */
void
initArimaForecast(ArimaModel *forecast_model)
{
	int i;
	//int number_of_max_forecasts = forecast_model->ycount-forecast_model->p;
	forecast_model->actualForecast = 0;
	if(forecast_model->d)
		memcpy(forecast_model->diffhelp_temporal_ns,forecast_model->diffhelp_ns,sizeof(double)*forecast_model->d);
	if(forecast_model->sd)
	{
		for(i=0;i<forecast_model->sd;i++)
			memcpy((forecast_model->diffhelp_temporal_s[i]),(forecast_model->diffhelp_s[i]),sizeof(double)*(forecast_model->pd+1));
	}
	expandYArray(forecast_model,forecast_model->p+INIT_FORECASTS);

}

void
expandYArray(ArimaModel *forecastModel,int to)
{
	forecastModel->y = repalloc(forecastModel->y,to * sizeof(*forecastModel->y));
	forecastModel->ycount = to;
}
