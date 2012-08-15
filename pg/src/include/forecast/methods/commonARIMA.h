/*-------------------------------------------------------------------------
 *
 * commonARIMA.h
 *	  functions and structures that are common for all ARIMA Models.
 *
 *
 * $PostgreSQL: pgsql/src/include/forecast/methods/commomARIMA.h,v 1.0 2010/08/25
 *  *
 *-------------------------------------------------------------------------
 */

#ifndef COMMONARIMA_H
#define COMMONARIMA_H

#include "forecast/algorithm.h"
#include <time.h>

typedef struct ArimaModel
{
	int				p; /* non seasonal AR Order*/
	int 			d; /* non-seasonal difference */
	int 			q; /* max non-seasonal MA order */
	int 			sp; /* seasonal AR order */
	int 			sd; /* seasonal difference */
	int 			sq; /* seasonal MA order */
	int 			pd;/* periodicity of data */

	int				automatic; /* determine parameters automatically */

	double			*phis;   /* coefficients for the AR part of the model */
	double			*thetas; /* coefficients for the MA part of the model */


	double          *uhat;   /* regression residuals MA part of the model*/
	

	double			*y;      /* the last measures for the AR part of the model */
	
	int				ycount;  /* size of the function value array */

	int				actualForecast;  /* the point of time from which to forecast */

	double 			constant[2]; /* the constant term of the model if there is one*/
	int				includeConstant; /* 1 if a constant is included */
	char*			errorfunction;
	int				seasonType;
	double			*diffhelp_ns; /* value to allow nonseasonal integrating */
	double			*diffhelp_temporal_ns; /* value to allow nonseasonal integrating */
	double			**diffhelp_s; /* value to allow seasonal integrating */
	double			**diffhelp_temporal_s; /* value to allow seasonal integrating */
	double			error;

}ArimaModel;

extern double get_next_forecast(ArimaModel *forecast_model);
extern double _get_next_Arimaforecast(ArimaModel *forecast_model,double* phis,double* thetas,double* uhat);
extern void	initArimaForecast(ArimaModel *forecast_model);

#endif /* COMMONARIMA_H_ */
