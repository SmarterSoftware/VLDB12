/*-------------------------------------------------------------------------
 *
 * pg_array.h
 *	  definition of the system "array" relation (pg_array)
 *	  along with the relation's initial contents.
 *
 *
 * Portions Copyright (c) 1996-2009, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/catalog/pg_array.h,v 1.0 2009/09/02 14:35:00 momjian Exp $
 *
 * NOTES
 *	  the genbki.sh script reads this file and generates .bki
 *	  information from the DATA() statements.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_ARRAY_H
#define PG_ARRAY_H

#include "catalog/genbki.h"

#include "nodes/primnodes.h"
#include "forecast/algorithm.h"
#include "catalog/pg_parameter.h"

/* ----------------
 *		pg_array definition.	cpp turns this into
 *		typedef struct FormData_pg_array
 * ----------------
 */
#define ArrayRelationId  7000
//#define ParameterRelationId  7001

CATALOG(pg_array,7000)
{
	Oid 		table;					/* relation array is attached to */
	int4		size;
	int4		number;
	int4		lastvalue;
} FormData_pg_array;

/* ----------------
 *		Form_pg_array corresponds to a pointer to a tuple with
 *		the format of pg_array relation.
 * ----------------
 */
typedef FormData_pg_array *Form_pg_array;

/* ----------------
 *		compiler constants for pg_array
 * ----------------
 */
#define Natts_pg_array			4

#define Anum_pg_array_oid		1
#define Anum_pg_array_size		2
#define Anum_pg_array_number		3
#define Anum_pg_array_lastvalue		4

Oid InsertArray(Oid oid, int size, int number, int lastvalue);
void UpdateLastValue(Oid arr, int size, int number);
int lastvalue(Oid arr);
int GetPgArray(Oid arr, Form_pg_array row);
int tuplesperpage(Oid arr);
#endif   /* PG_array_H */

