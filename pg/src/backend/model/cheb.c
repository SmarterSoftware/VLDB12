/* cheb.f -- translated by f2c (version 20090411).
You must link the resulting object file with libf2c:
on Microsoft Windows system, link with libf2c.lib;
on Linux or Unix systems, link with .../path/to/libf2c.a -lm
or, if you install libf2c.a in a standard place, with -lf2c -lm
-- in that order, at the end of the command line, as in
cc *.o -lf2c -lm
Source for libf2c is in /netlib/f2c/libf2c.zip, e.g.,

http://www.netlib.org/f2c/libf2c.zip
*/
#include "model/cheb.h"
typedef double real;
typedef int integer;

#define dabs(a)	   (((a) < 0) ? -(a) : (a))
double r_sign(double *a, double *b)
{
	double x;
	double ax=*a;
	double bx=*b;	
	x = (ax >= 0 ? ax : -ax);
	return (bx >= 0 ? x : -x);
}

/* Table of constant values */

static real c_b44 = 1.f;

/* Subroutine */ int cheb_(integer *m, integer *n, integer *mdim, integer *
	ndim, real *a, real *b, real *tol, real *relerr, real *x, integer *
	rank, real *resmax, integer *iter, integer *ocode)
{
	/* Initialized data */

	static real big = 1e32f;

	/* System generated locals */
	integer a_dim1, a_offset, i__1, i__2;
	real r__1;

	/* Builtin functions */
	double r_sign(real *, real *);

	/* Local variables */
	static real d__;
	static integer i__, j, k;
	static real dd;
	static integer mm1, kp1, mp1, np1, np2, np3;
	static real val;
	static integer lev, mode, pcol, prow, np1mk, np1mr;
	static real pivot;
	static integer rankp1;
	static real reltmp, tpivot;

	/* THIS SUBROUTINE USES A MODIFICATION OF THE SIMPLEX METHOD */
	/* OF LINEAR PROGRAMMING TO CALCULATE A CHEBYSHEV SOLUTION TO */
	/* AN OVER-DETERMINED SYSTEM OF LINEAR EQUATIONS. */
	/* DESCRIPTION OF PARAMETERS. */
	/* M      NUMBER OF EQUATIONS. */
	/* N      NUMBER OF UNKNOWNS (N MUST NOT EXCEED M). */
	/* MDIM   THE NUMBER OF COLUMNS OF A, AT LEAST M+1. */
	/* NDIM   THE NUMBER OF ROWS OF A, AT LEAST N+3. */
	/* A      TWO DIMENSIONAL REAL ARRAY OF SIZE (NDIM,MDIM). */
	/*        ON ENTRY,THE TRANSPOSE OF THE MATRIX OF */
	/*        COEFFICIENTS OF THE OVER-DETERMINED SYSTEM MUST */
	/*        BE STORED IN THE FIRST M COLUMNS AND N ROWS OF A. */
	/*        THESE VALUES ARE DESTROYED BY THE SUBROUTINE. */
	/* B      ONE DIMENSIONAL REAL ARRAY OF SIZE MDIM. ON ENTRY, */
	/*        B MUST CONTAIN THE RIGHT-HAND SIDES OF THE */
	/*        EQUATIONS IN ITS FIRST M LOCATIONS. ON EXIT, B */
	/*        CONTAINS THE RESIDUALS FOR THE EQUATIONS IN ITS */
	/*        FIRST M LOCATIONS (SEE DESCRIPTION). */
	/* TOL    A SMALL POSITIVE TOLERANCE. EMPIRICAL EVIDENCE */
	/*        SUGGESTS TOL=10**(-D+1) WHERE D REPRESENTS THE */
	/*        NUMBER OF DECIMAL DIGITS OF ACCURACY AVAILABLE */
	/*        (SEE DESCRIPTION). */
	/* RELERR A REAL VARIABLE WHICH ON ENTRY MUST HAVE THE VALUE */
	/*        0.0 IF A CHEBYSHEV SOLUTION IS REQUIRED. IF RELERR */
	/*        IS POSITIVE, THE SUBROUTINE CALCULATES AN */
	/*        APPROXIMATE SOLUTION WITH RELERR AS AN UPPER BOUND */
	/*        ON THE RELATIVE ERROR OF ITS LARGEST RESIDUAL (SEE */
	/*        DESCRIPTION-INEQUALITY (2)). ON EXIT, THE VALUE OF */
	/*        RELERR GIVES A SMALLER UPPER BOUND FOR THIS */
	/*        RELATIVE ERROR. */
	/* X      ONE DIMENSIONAL REAL ARRAY OF SIZE NDIM. ON EXIT, */
	/*        THIS ARRAY CONTAINS A SOLUTION TO THE PROBLEM IN */
	/*        ITS FIRST N LOCATIONS. */
	/* RANK   AN INTEGER WHICH GIVES ON EXIT THE RANK OF THE */
	/*        MATRIX OF COEFFICIENTS. */
	/* RESMAX THE LARGEST RESIDUAL IN MAGNITUDE. */
	/* ITER   THE NUMBER OF SIMPLEX ITERATIONS PERFORMED. */
	/* OCODE  AN EXIT CODE WITH VALUES.. */
	/*              0 - OPTIMAL SOLUTION WHICH IS PROBABLY */
	/*                  NON-UNIQUE (SEE DESCRIPTION). */
	/*              1 - UNIQUE OPTIMAL SOLUTION. */
	/*              2 - CALCULATIONS TERMINATED PREMATURELY */
	/*                  DUE TO ROUNDING ERRORS. */
	/* IF YOUR FORTRAN COMPILER PERMITS A SINGLE COLUMN OF A TWO */
	/* DIMENSIONAL ARRAY TO BE PASSED TO A ONE DIMENSIONAL ARRAY */
	/* THROUGH A SUBROUTINE CALL, CONSIDERABLE SAVINGS IN */
	/* EXECUTION TIME MAY BE ACHIEVED THROUGH THE USE OF THE */
	/* FOLLOWING SUBROUTINE WHICH OPERATES ON COLUMN VECTORS. */
	/*     SUBROUTINE COL (V1,V2,MLT,NOTROW,I1,NP2) */
	/* THIS SUBROUTINE SUBTRACTS FROM THE VECTOR V1 A MULTIPLE OF */
	/* THE VECTOR V2 STARTING AT THE I1*TH ELEMENT UP TO THE */
	/* NP2*TH ELEMENT, EXCEPT FOR THE NOTROW*TH ELEMENT. */
	/*     REAL V1(NP2),V2(NP2),MLT */
	/*     DO 1 I=I1,NP2 */
	/*       IF(I.EQ.NOTROW) GO TO 1 */
	/*       V1(I)=V1(I)-MLT*V2(I) */
	/*   1   CONTINUE */
	/*     RETURN */
	/*     END */
	/* SEE COMMENTS FOLLOWING STATEMENT NUMBER 340 FOR */
	/* INSTRUCTIONS ON THE IMPLEMENTATION OF THIS MODIFICATION. */
	/* BIG MUST BE SET EQUAL TO ANY VERY LARGE REAL CONSTANT. */
	/* ITS VALUE HERE IS APPROPRIATE FOR THE IBM/370/145. */
	/* Parameter adjustments */
	--b;
	--x;
	a_dim1 = *ndim;
	a_offset = 1 + a_dim1;
	a -= a_offset;

	/* Function Body */
	/* INITIALIZATION. */
	mp1 = *m + 1;
	np1 = *n + 1;
	np2 = *n + 2;
	np3 = *n + 3;
	np1mr = 1;
	*rank = *n;
	reltmp = *relerr;
	*relerr = 0.f;
	i__1 = *m;
	for (j = 1; j <= i__1; ++j) {
		a[np1 + j * a_dim1] = 1.f;
		a[np2 + j * a_dim1] = -b[j];
		a[np3 + j * a_dim1] = (real) (*n + j);
		/* L10: */
	}
	a[np1 + mp1 * a_dim1] = 0.f;
	*iter = 0;
	*ocode = 1;
	i__1 = *n;
	for (i__ = 1; i__ <= i__1; ++i__) {
		x[i__] = 0.f;
		a[i__ + mp1 * a_dim1] = (real) i__;
		/* L20: */
	}
	/* LEVEL 1. */
	lev = 1;
	k = 0;
L30:
	++k;
	kp1 = k + 1;
	np1mk = np1 - k;
	mode = 0;
	i__1 = *m;
	for (j = k; j <= i__1; ++j) {
		b[j] = 1.f;
		/* L40: */
	}
	/* DETERMINE THE VECTOR TO ENTER THE BASIS. */
L50:
	d__ = -big;
	i__1 = *m;
	for (j = k; j <= i__1; ++j) {
		if (b[j] == 0.f) {
			goto L60;
		}
		dd = (r__1 = a[np2 + j * a_dim1], dabs(r__1));
		if (dd <= d__) {
			goto L60;
		}
		pcol = j;
		d__ = dd;
L60:
		;
	}
	if (k > 1) {
		goto L70;
	}
	/* TEST FOR ZERO RIGHT-HAND SIDE. */
	if (d__ > *tol) {
		goto L70;
	}
	*resmax = 0.f;
	mode = 2;
	goto L380;
	/* DETERMINE THE VECTOR TO LEAVE THE BASIS. */
L70:
	d__ = *tol;
	i__1 = np1mk;
	for (i__ = 1; i__ <= i__1; ++i__) {
		dd = (r__1 = a[i__ + pcol * a_dim1], dabs(r__1));
		if (dd <= d__) {
			goto L80;
		}
		prow = i__;
		d__ = dd;
L80:
		;
	}
	if (d__ > *tol) {
		goto L330;
	}
	/* CHECK FOR LINEAR DEPENDENCE IN LEVEL 1. */
	b[pcol] = 0.f;
	if (mode == 1) {
		goto L50;
	}
	i__1 = *m;
	for (j = k; j <= i__1; ++j) {
		if (b[j] == 0.f) {
			goto L100;
		}
		i__2 = np1mk;
		for (i__ = 1; i__ <= i__2; ++i__) {
			if ((r__1 = a[i__ + j * a_dim1], dabs(r__1)) <= *tol) {
				goto L90;
			}
			mode = 1;
			goto L50;
L90:
			;
		}
L100:
		;
	}
	*rank = k - 1;
	np1mr = np1 - *rank;
	*ocode = 0;
	goto L160;
L110:
	if (pcol == k) {
		goto L130;
	}
	/* INTERCHANGE COLUMNS IN LEVEL 1. */
	i__1 = np3;
	for (i__ = 1; i__ <= i__1; ++i__) {
		d__ = a[i__ + pcol * a_dim1];
		a[i__ + pcol * a_dim1] = a[i__ + k * a_dim1];
		a[i__ + k * a_dim1] = d__;
		/* L120: */
	}
L130:
	if (prow == np1mk) {
		goto L150;
	}
	/* INTERCHANGE ROWS IN LEVEL 1. */
	i__1 = mp1;
	for (j = 1; j <= i__1; ++j) {
		d__ = a[prow + j * a_dim1];
		a[prow + j * a_dim1] = a[np1mk + j * a_dim1];
		a[np1mk + j * a_dim1] = d__;
		/* L140: */
	}
L150:
	if (k < *n) {
		goto L30;
	}
L160:
	if (*rank == *m) {
		goto L380;
	}
	rankp1 = *rank + 1;
	/* LEVEL 2. */
	lev = 2;
	/* DETERMINE THE VECTOR TO ENTER THE BASIS */
	d__ = *tol;
	i__1 = *m;
	for (j = rankp1; j <= i__1; ++j) {
		dd = (r__1 = a[np2 + j * a_dim1], dabs(r__1));
		if (dd <= d__) {
			goto L170;
		}
		pcol = j;
		d__ = dd;
L170:
		;
	}
	/* COMPARE CHEBYSHEV ERROR WITH TOL. */
	if (d__ > *tol) {
		goto L180;
	}
	*resmax = 0.f;
	mode = 3;
	goto L380;
L180:
	if (a[np2 + pcol * a_dim1] < -(*tol)) {
		goto L200;
	}
	a[np1 + pcol * a_dim1] = 2.f - a[np1 + pcol * a_dim1];
	i__1 = np3;
	for (i__ = np1mr; i__ <= i__1; ++i__) {
		if (i__ == np1) {
			goto L190;
		}
		a[i__ + pcol * a_dim1] = -a[i__ + pcol * a_dim1];
L190:
		;
	}
	/* ARRANGE FOR ALL ENTRIES IN PIVOT COLUMN */
	/* (EXCEPT PIVOT) TO BE NEGATIVE. */
L200:
	i__1 = *n;
	for (i__ = np1mr; i__ <= i__1; ++i__) {
		if (a[i__ + pcol * a_dim1] < *tol) {
			goto L220;
		}
		i__2 = *m;
		for (j = 1; j <= i__2; ++j) {
			a[np1 + j * a_dim1] += a[i__ + j * a_dim1] * 2.f;
			a[i__ + j * a_dim1] = -a[i__ + j * a_dim1];
			/* L210: */
		}
		a[i__ + mp1 * a_dim1] = -a[i__ + mp1 * a_dim1];
L220:
		;
	}
	prow = np1;
	goto L330;
L230:
	if (rankp1 == *m) {
		goto L380;
	}
	if (pcol == *m) {
		goto L250;
	}
	/* INTERCHANGE COLUMNS IN LEVEL 2. */
	i__1 = np3;
	for (i__ = np1mr; i__ <= i__1; ++i__) {
		d__ = a[i__ + pcol * a_dim1];
		a[i__ + pcol * a_dim1] = a[i__ + *m * a_dim1];
		a[i__ + *m * a_dim1] = d__;
		/* L240: */
	}
L250:
	mm1 = *m - 1;
	/* LEVEL 3. */
	lev = 3;
	/* DETERMINE THE VECTOR TO ENTER THE BASIS. */
L260:
	d__ = -(*tol);
	val = a[np2 + *m * a_dim1] * 2.f;
	i__1 = mm1;
	for (j = rankp1; j <= i__1; ++j) {
		if (a[np2 + j * a_dim1] >= d__) {
			goto L270;
		}
		pcol = j;
		d__ = a[np2 + j * a_dim1];
		mode = 0;
		goto L280;
L270:
		dd = val - a[np2 + j * a_dim1];
		if (dd >= d__) {
			goto L280;
		}
		mode = 1;
		pcol = j;
		d__ = dd;
L280:
		;
	}
	if (d__ >= -(*tol)) {
		goto L380;
	}
	dd = -d__ / a[np2 + *m * a_dim1];
	if (dd >= reltmp) {
		goto L290;
	}
	*relerr = dd;
	mode = 4;
	goto L380;
L290:
	if (mode == 0) {
		goto L310;
	}
	i__1 = np1;
	for (i__ = np1mr; i__ <= i__1; ++i__) {
		a[i__ + pcol * a_dim1] = a[i__ + *m * a_dim1] * 2.f - a[i__ + pcol * 
			a_dim1];
		/* L300: */
	}
	a[np2 + pcol * a_dim1] = d__;
	a[np3 + pcol * a_dim1] = -a[np3 + pcol * a_dim1];
	/* DETERMINE THE VECTOR TO LEAVE THE BASIS. */
L310:
	d__ = big;
	i__1 = np1;
	for (i__ = np1mr; i__ <= i__1; ++i__) {
		if (a[i__ + pcol * a_dim1] <= *tol) {
			goto L320;
		}
		dd = a[i__ + *m * a_dim1] / a[i__ + pcol * a_dim1];
		if (dd >= d__) {
			goto L320;
		}
		prow = i__;
		d__ = dd;
L320:
		;
	}
	if (d__ < big) {
		goto L330;
	}
	*ocode = 2;
	goto L380;
	/* PIVOT ON A(PROW,PCOL). */
L330:
	pivot = a[prow + pcol * a_dim1];
	i__1 = *m;
	for (j = 1; j <= i__1; ++j) {
		a[prow + j * a_dim1] /= pivot;
		/* L340: */
	}
	/* IF PERMITTED, USE SUBROUTINE COL IN THE DESCRIPTION */
	/* SECTION AND REPLACE THE FOLLOWING EIGHT STATEMENTS DOWN TO */
	/* AND INCLUDING STATEMENT NUMBER 360 BY.. */
	/*     DO 360 J=1,M */
	/*       IF(J.EQ.PCOL) GO TO 360 */
	/*       CALL COL (A(1,J),A(1,PCOL),A(PROW,J),PROW,NP1MR,NP2) */
	/* 360 CONTINUE */
	i__1 = *m;
	for (j = 1; j <= i__1; ++j) {
		if (j == pcol) {
			goto L360;
		}
		d__ = a[prow + j * a_dim1];
		i__2 = np2;
		for (i__ = np1mr; i__ <= i__2; ++i__) {
			if (i__ == prow) {
				goto L350;
			}
			a[i__ + j * a_dim1] -= d__ * a[i__ + pcol * a_dim1];
L350:
			;
		}
L360:
		;
	}
	tpivot = -pivot;
	i__1 = np2;
	for (i__ = np1mr; i__ <= i__1; ++i__) {
		a[i__ + pcol * a_dim1] /= tpivot;
		/* L370: */
	}
	a[prow + pcol * a_dim1] = 1.f / pivot;
	d__ = a[prow + mp1 * a_dim1];
	a[prow + mp1 * a_dim1] = a[np3 + pcol * a_dim1];
	a[np3 + pcol * a_dim1] = d__;
	++(*iter);
	switch (lev) {
	case 1:  goto L110;
	case 2:  goto L230;
	case 3:  goto L260;
	}
	/* PREPARE OUTPUT. */
L380:
	i__1 = *m;
	for (j = 1; j <= i__1; ++j) {
		b[j] = 0.f;
		/* L390: */
	}
	if (mode == 2) {
		goto L450;
	}
	i__1 = *rank;
	for (j = 1; j <= i__1; ++j) {
		k = a[np3 + j * a_dim1];
		x[k] = a[np2 + j * a_dim1];
		/* L400: */
	}
	if (mode == 3 || *rank == *m) {
		goto L450;
	}
	i__1 = np1;
	for (i__ = np1mr; i__ <= i__1; ++i__) {
		k = (r__1 = a[i__ + mp1 * a_dim1], dabs(r__1)) - (real) (*n);
		b[k] = a[np2 + *m * a_dim1] * r_sign(&c_b44, &a[i__ + mp1 * a_dim1]);
		/* L410: */
	}
	if (rankp1 == *m) {
		goto L430;
	}
	i__1 = mm1;
	for (j = rankp1; j <= i__1; ++j) {
		k = (r__1 = a[np3 + j * a_dim1], dabs(r__1)) - (real) (*n);
		b[k] = (a[np2 + *m * a_dim1] - a[np2 + j * a_dim1]) * r_sign(&c_b44, &
			a[np3 + j * a_dim1]);
		/* L420: */
	}
	/* TEST FOR NON-UNIQUE SOLUTION. */
L430:
	i__1 = np1;
	for (i__ = np1mr; i__ <= i__1; ++i__) {
		if ((r__1 = a[i__ + *m * a_dim1], dabs(r__1)) > *tol) {
			goto L440;
		}
		*ocode = 0;
		goto L450;
L440:
		;
	}
L450:
	if (mode != 2 && mode != 3) {
		*resmax = a[np2 + *m * a_dim1];
	}
	if (*rank == *m) {
		*resmax = 0.f;
	}
	if (mode == 4) {
		*resmax -= d__;
	}
	return 0;
} /* cheb_ */


void old()
{
	/* M      NUMBER OF EQUATIONS. */
	/* N      NUMBER OF UNKNOWNS (N MUST NOT EXCEED M). */
	/* MDIM   THE NUMBER OF COLUMNS OF A, AT LEAST M+1. */
	/* NDIM   THE NUMBER OF ROWS OF A, AT LEAST N+3. */
	/* A      TWO DIMENSIONAL double ARRAY OF SIZE (NDIM,MDIM). */
	/*        ON ENTRY,THE TRANSPOSE OF THE MATRIX OF */
	/*        COEFFICIENTS OF THE OVER-DETERMINED SYSTEM MUST */
	/*        BE STORED IN THE FIRST M COLUMNS AND N ROWS OF A. */
	/*        THESE VALUES ARE DESTROYED BY THE SUBROUTINE. */
	/* B      ONE DIMENSIONAL double ARRAY OF SIZE MDIM. ON ENTRY, */
	/*        B MUST CONTAIN THE RIGHT-HAND SIDES OF THE */
	/*        EQUATIONS IN ITS FIRST M LOCATIONS. ON EXIT, B */
	/*        CONTAINS THE RESIDUALS FOR THE EQUATIONS IN ITS */
	/*        FIRST M LOCATIONS (SEE DESCRIPTION). */
	/* TOL    A SMALL POSITIVE TOLERANCE. EMPIRICAL EVIDENCE */
	/*        SUGGESTS TOL=10**(-D+1) WHERE D REPRESENTS THE */
	/*        NUMBER OF DECIMAL DIGITS OF ACCURACY AVAILABLE */
	/*        (SEE DESCRIPTION). */
	/* RELERR A double VARIABLE WHICH ON ENTRY MUST HAVE THE VALUE */
	/*        0.0 IF A CHEBYSHEV SOLUTION IS REQUIRED. IF RELERR */
	/*        IS POSITIVE, THE SUBROUTINE CALCULATES AN */
	/*        APPROXIMATE SOLUTION WITH RELERR AS AN UPPER BOUND */
	/*        ON THE RELATIVE ERROR OF ITS LARGEST RESIDUAL (SEE */
	/*        DESCRIPTION-INEQUALITY (2)). ON EXIT, THE VALUE OF */
	/*        RELERR GIVES A SMALLER UPPER BOUND FOR THIS */
	/*        RELATIVE ERROR. */
	/* X      ONE DIMENSIONAL double ARRAY OF SIZE NDIM. ON EXIT, */
	/*        THIS ARRAY CONTAINS A SOLUTION TO THE PROBLEM IN */
	/*        ITS FIRST N LOCATIONS. */
	/* RANK   AN int WHICH GIVES ON EXIT THE RANK OF THE */
	/*        MATRIX OF COEFFICIENTS. */
	/* RESMAX THE LARGEST RESIDUAL IN MAGNITUDE. */
	/* ITER   THE NUMBER OF SIMPLEX ITERATIONS PERFORMED. */
	int m = 3;
	int n = 2;
	int i,j;
	int mdim = m + 1 + 1;
	int ndim = n + 3 + 1;
	double* b;
	double* x;
	int rank = 0;
	double resmax = 0.0;
	int iter = 0;
	int opcode=1;
	double tol=1e-15;
	double relerr=0;

	double* a = (double*) malloc(5*6*sizeof(double));
	for(i=0;i<30;i++) a[i]=0; 
	a[1*5+1]=1;
	a[1*5+2]=1;
	a[1*5+3]=1;
	a[2*5+1]=1;
	a[2*5+2]=3;
	a[2*5+3]=4;
	for(i=0;i<5;i++) {
		for(j=0;j<6;j++) {
			printf("%lf " ,a[i*5+j]);
		}
		printf("\n");
	}
	b = (double*) malloc(5*sizeof(double));
	b[0]=0;
	b[1]=6;
	b[2]=7;
	b[3]=10;
	b[4]=0;

	x = (double*)malloc(5*sizeof(double));
	cheb_(&m, &n, &mdim, &ndim, a, b, &tol, &relerr, x, &rank, &resmax, &iter,&opcode);
	printf("rank %lf \n" ,rank);
	printf("iter %lf \n" ,iter);
	printf("opcode %lf \n" ,opcode);
	for(j=0;j<6;j++) {
		printf("%lf \n" ,x[j]);
	}

}


double * ChebyshevReg_Solve(double *bx, int len)
{
	int start=0;
	int m = len;
	int n = 2;
	int mdim = m + 1 + 1;
	int ndim = n + 3 + 1;
	int rank = 0;
	double resmax = 0.0;
	int iter = 0;
	int i,j;
	int opcode=1;
	double tol=1e-15;
	double relerr=0;
	double *bb;

	double *a = (double*)malloc(ndim*mdim*sizeof(double));
	double  *x = (double*)malloc(ndim*sizeof(double)); //new double[ndim];
	double *b = (double*)malloc(mdim*sizeof(double)); //new double[mdim];
	for(j=0;j<ndim*mdim;j++) a[j]=0;
	for (j = 0; j < m; j++)
	{
		a[(0 + 1)*mdim+ j + 1] = j+start;
		a[(1 + 1)*mdim+ j + 1] = 1;
	}
	b[0]=0;
	for (i = 0; i < m; i++)
		b[i + 1] = bx[i];
	cheb_(&m, &n, &mdim, &ndim, a, b, &tol, &relerr, x, &rank, &resmax, &iter,&opcode);

	bb =(double*)malloc(n*sizeof(double)); //new double[mdim];
	for (i = 0; i < n; i++) {
		bb[i] = x[i];
		printf("bb %d %lf\n",i,bb[i]);
	}
	free(a);
	free(x);
	free(b);
	return bb;
}

double *Reg_solve(double *x, int len){
	int i;
	double *bb =(double*)malloc(2*sizeof(double)); //new double[mdim];
	double asum = 0;
	double bsum = 0;

	for ( i = 0; i < len; i++)
	{
		asum += ((i + 1) - ((len + 1) / 2)) * x[i];
		bsum += ((i + 1) - ((2 * len + 1) / 3)) * x[i];
	}
	bb[0] = 12 * asum / len / (len + 1) / (len - 1);
	bb[1] = 6 * bsum / len / (1 -len);
	return bb;
}


/*void main() {

double* b = (double*) malloc(3*sizeof(double));
b[0]=0;
b[1]=7;
b[2]=8;

Solve(b, 3);
}*/
