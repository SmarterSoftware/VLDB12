#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for backend/executor/model
#
# IDENTIFICATION
#    $PostgreSQL: pgsql/src/backend/model/Makefile$
#
#-------------------------------------------------------------------------
all:
	rm -f testmodel
	gcc bmodel.c ts.c stl.c cheb.c testmodel.c -o testmodel -lm

run:
	./testmodel
