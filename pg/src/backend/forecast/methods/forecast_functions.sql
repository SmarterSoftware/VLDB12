CREATE OR REPLACE FUNCTION autoArima(vals float8[], season integer) RETURNS float8[] AS '
    library(forecast)
	m <- auto.arima(ts(vals,frequency=season), allowdrift=FALSE)
	return(as.vector(m$arma))
' LANGUAGE 'plr' STRICT;



CREATE OR REPLACE FUNCTION etsForecast(vals float8[], num integer, season integer) RETURNS float8[] AS '
    library(forecast)
	prog <- forecast(ets(ts(vals, frequency=season)),h=num)
	return(prog$mean)
' LANGUAGE 'plr' STRICT;