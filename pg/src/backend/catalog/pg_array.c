/*-------------------------------------------------------------------------
 *
 * pg_array.c
 *	  routines to support manipulation of the pg_array relation
 *
 * Portions Copyright (c) 1996-2009, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/catalog/pg_array.c,v 1.0 2009/09/14$
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "funcapi.h"

#include "catalog/catalog.h"
#include "catalog/indexing.h"
#include "catalog/pg_array.h"
#include "utils/array.h"
#include "utils/date.h"
#include "utils/builtins.h"
#include "utils/tqual.h"
#include "utils/syscache.h"
#include "utils/fmgroids.h"
#include "catalog/pg_tablespace.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_type.h"

int ar_step = 1;
 int ar_start=0;
int lastvalue(Oid arr) {
	FormData_pg_array row;
	int last_value = 0;
	GetPgArray(arr, &row);
	last_value = row.lastvalue;
	return last_value;
}

int tuplesperpage(Oid arr) {
	FormData_pg_array row;
	int tpp = 0;
	GetPgArray(arr, &row);
	tpp = row.number;
	return tpp;
}

int GetPgArray(Oid arr, Form_pg_array row) {
	Relation 	arel;
	ScanKeyData scanoid[1];


	HeapTuple 	tuple;
	HeapScanDesc result;
	Datum				*values;
	bool				*isNull;
	bool found = false;

	// open parameter relation
	arel = heap_open(ArrayRelationId, RowExclusiveLock);
	//elog(WARNING,"open relation pg_array for update");
	//ScanKeyInit(&scanoid[1],1,InvalidStrategy,F_OIDEQ,arr); // i do not like [1]
	result= heap_beginscan(arel,SnapshotNow,0,NULL); //was 1, scanoid

	while((tuple = heap_getnext(result, ForwardScanDirection)) != NULL)
	{
		values = (Datum *) palloc(RelationGetDescr(arel)->natts * sizeof(Datum));
		isNull = (bool *) palloc(RelationGetDescr(arel)->natts * sizeof(bool));
		heap_deform_tuple(tuple, RelationGetDescr(arel), values, isNull);
		//elog(WARNING, "%d",values[0]);
		if (values[0] == arr){
			found = true;
			row->table = values[0];
			row->size = values[1];
			row->number = values[2];
			row->lastvalue = values[3];
		}
		pfree(values);
		pfree(isNull);
	
	}
	heap_endscan(result);

	heap_close(arel, RowExclusiveLock);
	return found;
}

void DeleteArr(Oid arr) {

	Relation 	arel;
	ScanKeyData scanoid[1];

	HeapTuple 	tuple;
	HeapScanDesc result;
	Datum				*values;
	bool				*isNull;

	arel = heap_open(ArrayRelationId, RowExclusiveLock);
	result= heap_beginscan(arel,SnapshotNow,0,NULL);

	while((tuple = heap_getnext(result, ForwardScanDirection)) != NULL)
	{

		values = (Datum *) palloc(RelationGetDescr(arel)->natts * sizeof(Datum));
		isNull = (bool *) palloc(RelationGetDescr(arel)->natts * sizeof(bool));
		heap_deform_tuple(tuple, RelationGetDescr(arel), values, isNull);
		if (values[0]==arr){
			simple_heap_delete(arel, &tuple->t_self);

		}
		pfree(values);
		pfree(isNull);
	}
	heap_endscan(result);
	heap_close(arel, RowExclusiveLock);
}


void
UpdateLastValue(Oid arr, int size, int number)
{
	Relation 	arel;
	ScanKeyData scanoid[1];


	HeapTuple 	tuple;
	HeapScanDesc result;
	Datum				*values;
	bool				*isNull;

	// open parameter relation
	arel = heap_open(ArrayRelationId, RowExclusiveLock);
	//elog(WARNING,"open relation pg_array for update");
	//ScanKeyInit(&scanoid[1],1,InvalidStrategy,F_OIDEQ,arr); // i do not like [1]
	result= heap_beginscan(arel,SnapshotNow,0,NULL); //was 1, scanoid

	while((tuple = heap_getnext(result, ForwardScanDirection)) != NULL)
	{

		values = (Datum *) palloc(RelationGetDescr(arel)->natts * sizeof(Datum));
		isNull = (bool *) palloc(RelationGetDescr(arel)->natts * sizeof(bool));
		heap_deform_tuple(tuple, RelationGetDescr(arel), values, isNull);
		//elog(WARNING, "%d",values[0]);
		if (values[0]==arr){
			ItemPointerData otid = tuple->t_self;
			//elog(WARNING, " Offset %d",  otid.ip_posid);

			values[3]=1+values[3];
			values[2]=number;
			values[1]=size;
			/* This is not pretty, but simple_heap update does not work */
			//simple_heap_delete(arel, &tuple->t_self);
			HeapTupleSetOid(tuple, arr);
			tuple = heap_form_tuple(arel->rd_att, values, isNull);
			//simple_heap_insert(arel, tuple);
			heap_inplace_update_withid(arel, &otid, tuple);
//		tuple=heap_form_tuple(RelationGetDescr(arel),values,isNull);
//		simple_heap_update(arel,(ItemPointer)(&(tuple->t_self)),tuple);
//		elog(WARNING,"pg_array updated !!");

		}
		pfree(values);
		pfree(isNull);
	}
	heap_endscan(result);
	heap_close(arel, RowExclusiveLock);
}

/**
 * Stores   in the system catalog. Not working for some reason
 */
Oid
InsertArray(Oid oid,
		int size,
		int number,
		int lastvalue)
{
	Datum 		pgValues[Natts_pg_array];
	bool 		pgNulls[Natts_pg_array];
	Relation	arel;
	Oid		aoid;
	HeapTuple	pgTuple;

	// open model relation
	arel = heap_open(ArrayRelationId, RowExclusiveLock);
	//elog(WARNING,"insert array open");

	// generate new unique model oid
	aoid = GetNewOid(arel);
	//	elog(WARNING,"insert array getnewoid");
	// Build the new pg_model tuple.
	memset(pgNulls, false, sizeof(pgNulls));

	// look if array already exists
	//pgTuple = SearchSysCache(MODELNAME, PointerGetDatum(modelName), 0,0,0);
	//if(HeapTupleIsValid(pgTuple))
	//{
	//	ReleaseSysCache(pgTuple);
	//	ereport(ERROR, (errcode(ERRCODE_DUPLICATE_OBJECT), errmsg("model \"%s\" already exists", modelName)));
	//}
	pgValues[Anum_pg_array_oid - 1] = ObjectIdGetDatum(oid);
	pgValues[Anum_pg_array_size - 1] = Int32GetDatum(size);
	pgValues[Anum_pg_array_number - 1] = Int32GetDatum(number);
	pgValues[Anum_pg_array_lastvalue - 1] = Int32GetDatum(lastvalue);
	// create tuple
	pgTuple = heap_form_tuple(arel->rd_att, pgValues, pgNulls);
	elog(WARNING,"insert array create tuple");
	// force tuple to have the desired OID
	HeapTupleSetOid(pgTuple, aoid);
		elog(WARNING,"insert array set oid");
	// Insert tuple into pg_array
	simple_heap_insert(arel, pgTuple);
	elog(WARNING,"insert array simple heap insert");
	// Update indexes
	//CatalogUpdateIndexes(arel, pgTuple);

	// Release memory
	heap_freetuple(pgTuple);

	// Close relation
	heap_close(arel, RowExclusiveLock);

	return aoid;
}



