
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     ABORT_P = 258,
     ABSOLUTE_P = 259,
     ACCESS = 260,
     ACTION = 261,
     ADD_P = 262,
     ADMIN = 263,
     AFTER = 264,
     AGGREGATE = 265,
     ALGORITHM = 266,
     ALL = 267,
     ALSO = 268,
     ALTER = 269,
     ALWAYS = 270,
     ANALYSE = 271,
     ANALYZE = 272,
     AND = 273,
     ANY = 274,
     ARRAY = 275,
     AS = 276,
     ASC = 277,
     ASOF = 278,
     ASSERTION = 279,
     ASSIGNMENT = 280,
     ASYMMETRIC = 281,
     AT = 282,
     ATTRIBUTES = 283,
     AUTHORIZATION = 284,
     AVERAGE = 285,
     BACKWARD = 286,
     BEFORE = 287,
     BEGIN_P = 288,
     BETWEEN = 289,
     BIGINT = 290,
     BINARY = 291,
     BIT = 292,
     BOOLEAN_P = 293,
     BOTH = 294,
     BOTTOMUP = 295,
     BY = 296,
     CACHE = 297,
     CALLED = 298,
     CASCADE = 299,
     CASCADED = 300,
     CASE = 301,
     CAST = 302,
     CATALOG_P = 303,
     CHAIN = 304,
     CHAR_P = 305,
     CHARACTER = 306,
     CHARACTERISTICS = 307,
     CHECK = 308,
     CHECKPOINT = 309,
     CHOOSE = 310,
     CLASS = 311,
     CLOSE = 312,
     CLUSTER = 313,
     COALESCE = 314,
     COLLATE = 315,
     COLUMN = 316,
     COMMENT = 317,
     COMMIT = 318,
     COMMITTED = 319,
     COMPLETE = 320,
     CONCURRENTLY = 321,
     CONFIDENCE = 322,
     CONFIGURATION = 323,
     CONNECTION = 324,
     CONSTRAINT = 325,
     CONSTRAINTS = 326,
     CONTENT_P = 327,
     CONTINUE_P = 328,
     CONVERSION_P = 329,
     COPY = 330,
     CORRELATION = 331,
     COST = 332,
     CREATE = 333,
     CREATEDB = 334,
     CREATEROLE = 335,
     CREATEUSER = 336,
     CROSS = 337,
     CSV = 338,
     CURRENT_P = 339,
     CURRENT_CATALOG = 340,
     CURRENT_DATE = 341,
     CURRENT_ROLE = 342,
     CURRENT_SCHEMA = 343,
     CURRENT_TIME = 344,
     CURRENT_TIMESTAMP = 345,
     CURRENT_USER = 346,
     CURSOR = 347,
     CYCLE = 348,
     DATA_P = 349,
     DATABASE = 350,
     DAY_P = 351,
     DEALLOCATE = 352,
     DEC = 353,
     DECIMAL_P = 354,
     DECLARE = 355,
     DECOMPOSE = 356,
     DEFAULT = 357,
     DEFAULTS = 358,
     DEFERRABLE = 359,
     DEFERRED = 360,
     DEFINER = 361,
     DELETE_P = 362,
     DELIMITER = 363,
     DELIMITERS = 364,
     DESC = 365,
     DICTIONARY = 366,
     DIMENSION = 367,
     DISABLE_P = 368,
     DISAGGREGATE = 369,
     DISAGGSCHEME = 370,
     DISCARD = 371,
     DISTINCT = 372,
     DO = 373,
     DOCUMENT_P = 374,
     DOMAIN_P = 375,
     DOUBLE_P = 376,
     DROP = 377,
     EACH = 378,
     ELSE = 379,
     ENABLE_P = 380,
     ENCODING = 381,
     ENCRYPTED = 382,
     END_P = 383,
     ENUM_P = 384,
     ERRORLIMIT = 385,
     ESCAPE = 386,
     EXCEPT = 387,
     EXCLUDING = 388,
     EXCLUSIVE = 389,
     EXECUTE = 390,
     EXISTS = 391,
     EXPLAIN = 392,
     EXTERNAL = 393,
     EXTRACT = 394,
     FALSE_P = 395,
     FAMILY = 396,
     FETCH = 397,
     FILL = 398,
     FILLING = 399,
     FIRST = 400,
     FIRST_P = 401,
     FLOAT_P = 402,
     FOLLOWING = 403,
     FOR = 404,
     FORCE = 405,
     FORECAST = 406,
     FOREIGN = 407,
     FORWARD = 408,
     FREEZE = 409,
     FROM = 410,
     FULL = 411,
     FUNC = 412,
     FUNCTION = 413,
     GLOBAL = 414,
     GRANT = 415,
     GRANTED = 416,
     GREATEST = 417,
     GREEDY = 418,
     GROUP_P = 419,
     HANDLER = 420,
     HAVING = 421,
     HEADER_P = 422,
     HOLD = 423,
     HOUR_P = 424,
     IDENTITY_P = 425,
     IF_P = 426,
     ILIKE = 427,
     IMMEDIATE = 428,
     IMMUTABLE = 429,
     IMPLICIT_P = 430,
     IN_P = 431,
     INCLUDING = 432,
     INCREMENT = 433,
     INDEX = 434,
     INDEXES = 435,
     INHERIT = 436,
     INHERITS = 437,
     INITIALLY = 438,
     INNER_P = 439,
     INOUT = 440,
     INPUT_P = 441,
     INSENSITIVE = 442,
     INSERT = 443,
     INSTEAD = 444,
     INT_P = 445,
     INTEGER = 446,
     INTERSECT = 447,
     INTERVAL = 448,
     INTERVALP = 449,
     INTO = 450,
     INVOKER = 451,
     IS = 452,
     ISNULL = 453,
     ISOLATION = 454,
     JOIN = 455,
     KEY = 456,
     KEYS = 457,
     LANCOMPILER = 458,
     LANGUAGE = 459,
     LARGE_P = 460,
     LAST_P = 461,
     LAYERS = 462,
     LC_COLLATE_P = 463,
     LC_CTYPE_P = 464,
     LEADING = 465,
     LEAST = 466,
     LEFT = 467,
     LEVEL = 468,
     LIKE = 469,
     LIMIT = 470,
     LISTEN = 471,
     LOAD = 472,
     LOCAL = 473,
     LOCALTIME = 474,
     LOCALTIMESTAMP = 475,
     LOCATION = 476,
     LOCK_P = 477,
     LOGIN_P = 478,
     MAINTENANCE = 479,
     MAPPING = 480,
     MATCH = 481,
     MAXVALUE = 482,
     METHOD = 483,
     MINUTE_P = 484,
     MINVALUE = 485,
     MODE = 486,
     MODEL = 487,
     MODELGRAPH = 488,
     MODELINDEX = 489,
     MONTH_P = 490,
     MOVE = 491,
     MULT = 492,
     NAME_P = 493,
     NAMES = 494,
     NATIONAL = 495,
     NATURAL = 496,
     NCHAR = 497,
     NEW = 498,
     NEXT = 499,
     NO = 500,
     NOCREATEDB = 501,
     NOCREATEROLE = 502,
     NOCREATEUSER = 503,
     NOINHERIT = 504,
     NOLOGIN_P = 505,
     NONE = 506,
     NOSUPERUSER = 507,
     NOT = 508,
     NOTHING = 509,
     NOTIFY = 510,
     NOTNULL = 511,
     NOWAIT = 512,
     NULL_P = 513,
     NULLIF = 514,
     NULLS_P = 515,
     NUMBER = 516,
     NUMERIC = 517,
     OBJECT_P = 518,
     OF = 519,
     OFF = 520,
     OFFSET = 521,
     OIDS = 522,
     OLD = 523,
     ON = 524,
     ONDEMAND = 525,
     ONLY = 526,
     OPERATOR = 527,
     OPTION = 528,
     OPTIONS = 529,
     OR = 530,
     ORDER = 531,
     OUT_P = 532,
     OUTER_P = 533,
     OVER = 534,
     OVERLAPS = 535,
     OVERLAY = 536,
     OWNED = 537,
     OWNER = 538,
     PARAMETERS = 539,
     PARSER = 540,
     PARTIAL = 541,
     PARTITION = 542,
     PASSWORD = 543,
     PLACING = 544,
     PLANS = 545,
     POSITION = 546,
     PRECEDING = 547,
     PRECISION = 548,
     PRESERVE = 549,
     PREPARE = 550,
     PREPARED = 551,
     PRIMARY = 552,
     PRINT = 553,
     PRIOR = 554,
     PRIVILEGES = 555,
     PROCEDURAL = 556,
     PROCEDURE = 557,
     QCACHE = 558,
     QUARTER_P = 559,
     QUOTE = 560,
     RANGE = 561,
     READ = 562,
     REAL = 563,
     REASSIGN = 564,
     RECHECK = 565,
     RECURSIVE = 566,
     REESTIMATE = 567,
     REFERENCES = 568,
     REINDEX = 569,
     RELATIVE_P = 570,
     RELEASE = 571,
     RENAME = 572,
     REPEATABLE = 573,
     REPLACE = 574,
     REPLICA = 575,
     RESET = 576,
     RESTART = 577,
     RESTORE = 578,
     RESTRICT = 579,
     RETURNING = 580,
     RETURNS = 581,
     REVOKE = 582,
     RIGHT = 583,
     ROLE = 584,
     ROLLBACK = 585,
     ROW = 586,
     ROWS = 587,
     RULE = 588,
     SAVEPOINT = 589,
     SCHEMA = 590,
     SCROLL = 591,
     SEARCH = 592,
     SEASON = 593,
     SECOND_P = 594,
     SECURITY = 595,
     SELECT = 596,
     SEQUENCE = 597,
     SERIALIZABLE = 598,
     SERVER = 599,
     SESSION = 600,
     SESSION_USER = 601,
     SET = 602,
     SETOF = 603,
     SHARE = 604,
     SHOW = 605,
     SIMILAR = 606,
     SIMPLE = 607,
     SIMPLEREESTIMATE = 608,
     SMALLINT = 609,
     SOME = 610,
     STABLE = 611,
     STANDALONE_P = 612,
     START = 613,
     STATEMENT = 614,
     STATISTICS = 615,
     STRATEGY = 616,
     STDIN = 617,
     STDOUT = 618,
     STORAGE = 619,
     STORE = 620,
     STRICT_P = 621,
     STRIP_P = 622,
     SUBSTRING = 623,
     SUPERUSER_P = 624,
     SYMMETRIC = 625,
     SYSID = 626,
     SYSTEM_P = 627,
     TABLE = 628,
     TABLESPACE = 629,
     TEMP = 630,
     TEMPLATE = 631,
     TEMPORARY = 632,
     TEXT_P = 633,
     THEN = 634,
     TIME = 635,
     TIMESERIES = 636,
     TIMESTAMP = 637,
     TO = 638,
     TOPDOWN = 639,
     TRAILING = 640,
     TRAINING_DATA = 641,
     TRANSACTION = 642,
     TREAT = 643,
     TRIGGER = 644,
     TRIM = 645,
     TRUE_P = 646,
     TRUNCATE = 647,
     TRUSTED = 648,
     TYPE_P = 649,
     UNBOUNDED = 650,
     UNCOMMITTED = 651,
     UNENCRYPTED = 652,
     UNION = 653,
     UNIQUE = 654,
     UNKNOWN = 655,
     UNLISTEN = 656,
     UNTIL = 657,
     UPDATE = 658,
     USER = 659,
     USING = 660,
     VACUUM = 661,
     VALID = 662,
     VALIDATOR = 663,
     VALUE_P = 664,
     VALUES = 665,
     VARCHAR = 666,
     VARIADIC = 667,
     VARYING = 668,
     VERBOSE = 669,
     VERSION_P = 670,
     VIEW = 671,
     VOLATILE = 672,
     WEEK_P = 673,
     WHEN = 674,
     WHERE = 675,
     WHITESPACE_P = 676,
     WINDOW = 677,
     WITH = 678,
     WITHOUT = 679,
     WORK = 680,
     WRAPPER = 681,
     WRITE = 682,
     XML_P = 683,
     XMLATTRIBUTES = 684,
     XMLCONCAT = 685,
     XMLELEMENT = 686,
     XMLFOREST = 687,
     XMLPARSE = 688,
     XMLPI = 689,
     XMLROOT = 690,
     XMLSERIALIZE = 691,
     YEAR_P = 692,
     YES_P = 693,
     ZONE = 694,
     NULLS_FIRST = 695,
     NULLS_LAST = 696,
     WITH_TIME = 697,
     IDENT = 698,
     FCONST = 699,
     SCONST = 700,
     BCONST = 701,
     XCONST = 702,
     Op = 703,
     ICONST = 704,
     PARAM = 705,
     POSTFIXOP = 706,
     UMINUS = 707,
     TYPECAST = 708
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 151 "gram.y"

	int					ival;
	char				chr;
	char				*str;
	const char			*keyword;
	bool				boolean;
	JoinType			jtype;
	DropBehavior		dbehavior;
	OnCommitAction		oncommit;
	List				*list;
	Node				*node;
	Value				*value;
	ObjectType			objtype;

	TypeName			*typnam;
	FunctionParameter   *fun_param;
	FunctionParameterMode fun_param_mode;
	FuncWithArgs		*funwithargs;
	DefElem				*defelt;
	SortBy				*sortby;
	WindowDef			*windef;
	JoinExpr			*jexpr;
	IndexElem			*ielem;
	Alias				*alias;
	RangeVar			*range;
	IntoClause			*into;
	WithClause			*with;
	A_Indices			*aind;
	ResTarget			*target;
	struct PrivTarget	*privtarget;
	AccessPriv			*accesspriv;

	InsertStmt			*istmt;
	VariableSetStmt		*vsetstmt;
	
	GraphAttribute		*graphAttr;



/* Line 1676 of yacc.c  */
#line 545 "gram.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE base_yylval;

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif

extern YYLTYPE base_yylloc;

