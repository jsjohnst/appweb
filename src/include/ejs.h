/*
 *  ejsTune.h - Tunable parameters for the C VM and compiler
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

#ifndef _h_EJS_TUNE
#define _h_EJS_TUNE 1

/********************************* Includes ***********************************/

#include    "mpr.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************************** Defines ***********************************/
/*
 *  Tunable Constants
 *  TODO - consistency of names is poor.
 */
#if BLD_TUNE == MPR_TUNE_SIZE || DOXYGEN
    /*
     *  Tune for size
     */
    #define EJS_GC_WORK_QUOTA       512             /**< Allocations required before garbage colllection */
    #define EJS_NUM_PROP            8               /**< Default object number of properties */
    #define EJS_NUM_GLOBAL          256             /**< Number of globals slots to pre-create */
    #define EJS_LOTSA_PROP          256             /**< Object with lots of properties. Grow by bigger chunks */
    #define EJS_E4X_BUF_MAX         (256 * 1024)    /* Max XML document size */
    #define EJS_MAX_RECURSION       10000           /* Maximum recursion */
    #define EJS_MAX_REGEX_MATCHES   32              /* Maximum regular sub-expressions */
    #define EJS_MAX_DB_MEM          (2*1024*1024)   /* Maximum regular sub-expressions */

    #define E4X_BUF_SIZE            512             /* Initial buffer size for tokens */
    #define E4X_BUF_MAX             (32 * 1024)     /* Max size for tokens */
    #define E4X_MAX_NODE_DEPTH      24              /* Max nesting of tags */

    #define EJS_WEB_TOK_INCR        1024
    #define EJS_WEB_MAX_HEADER      1024
    #define EJS_MAX_DEBUG_NAME      32
    #define EJS_MAX_TYPE            256             /**< Maximum number of types */
    #define EJS_NUM_CROSS_GEN       256             /* Number of cross generational GC root objects */

    #define EJS_CGI_MIN_BUF         (32 * 1024)     /* CGI output buffering */
    #define EJS_CGI_MAX_BUF         (128 * 1024)
    #define EJS_CGI_HDR_HASH        (31)

#elif BLD_TUNE == MPR_TUNE_BALANCED

    /*
     *  Tune balancing speed and size
     */
    #define EJS_GC_WORK_QUOTA       1024
    #define EJS_NUM_PROP            8
    #define EJS_NUM_GLOBAL          512
    #define EJS_LOTSA_PROP          256
    #define EJS_E4X_BUF_MAX         (1024 * 1024)
    #define EJS_MAX_RECURSION       (1000000)
    #define EJS_MAX_REGEX_MATCHES   64
    #define EJS_MAX_DB_MEM          (20*1024*1024)

    #define E4X_BUF_SIZE            4096
    #define E4X_BUF_MAX             (128 * 1024)
    #define E4X_MAX_NODE_DEPTH      128

    #define EJS_WEB_TOK_INCR        4096
    #define EJS_WEB_MAX_HEADER      4096
    #define EJS_MAX_DEBUG_NAME      64
    #define EJS_MAX_TYPE            512
    #define EJS_NUM_CROSS_GEN       1024 

    #define EJS_CGI_MIN_BUF         (64 * 1024)     /* CGI output buffering */
    #define EJS_CGI_MAX_BUF         (256 * 1024)
    #define EJS_CGI_HDR_HASH        (51)

#else
    /*
     *  Tune for speed
     */
    #define EJS_GC_WORK_QUOTA       2048
    #define EJS_NUM_PROP            8
    #define EJS_NUM_GLOBAL          1024
    #define EJS_LOTSA_PROP          1024
    #define EJS_E4X_BUF_MAX         (1024 * 1024)
    #define EJS_MAX_RECURSION       (1000000)
    #define EJS_MAX_REGEX_MATCHES   128
    #define EJS_MAX_DB_MEM          (20*1024*1024)

    #define E4X_BUF_SIZE            4096
    #define E4X_BUF_MAX             (128 * 1024)
    #define E4X_MAX_NODE_DEPTH      128

    #define EJS_WEB_TOK_INCR        4096
    #define EJS_WEB_MAX_HEADER      4096
    #define EJS_MAX_DEBUG_NAME      96
    #define EJS_MAX_TYPE            1024
    #define EJS_NUM_CROSS_GEN       4096 

    #define EJS_CGI_MIN_BUF         (128 * 1024)     /* CGI output buffering */
    #define EJS_CGI_MAX_BUF         (512 * 1024)
    #define EJS_CGI_HDR_HASH        (101)
#endif

#define EJS_SESSION_TIMEOUT         1800
#define EJS_TIMER_PERIOD            1000            /* Timer checks ever 1 second */

/*
 *  Object Property hash constants
 */
#define EJS_HASH_MIN_PROP           8               /**< Min props to hash */

#if BLD_FEATURE_MMU
    #if BLD_TUNE == MPR_TUNE_SIZE
        #define EJS_STACK_MAX       (1024 * 1024)   /* Stack size on virtual memory systems */
    #elif BLD_TUNE == MPR_TUNE_BALANCED
        #define EJS_STACK_MAX       (1024 * 1024 * 4)
    #else
        #define EJS_STACK_MAX       (1024 * 1024 * 16)
    #endif
#else
    /*
     *  Highly recursive workloads may need to increase the stack values
     */
    #if BLD_TUNE == MPR_TUNE_SIZE
        #define EJS_STACK_MAX       (1024 * 32)     /* Stack size without MMU */
    #elif BLD_TUNE == MPR_TUNE_BALANCED
        #define EJS_STACK_MAX       (1024 * 64)
    #else
        #define EJS_STACK_MAX       (1024 * 128)
    #endif
#endif

#if UNUSED && KEEP
/*
 *  Number of objects instances to create per type when allocating
 */
#define EJS_NUM_BOOLEAN         2
#define EJS_NUM_CMETHOD         1
#define EJS_NUM_DATE            1
#define EJS_NUM_DOUBLE          1
#define EJS_NUM_GLOBAL          1
#define EJS_NUM_INTEGER         32
#define EJS_NUM_LONG            32
#define EJS_NUM_NAMESPACE       8
#define EJS_NUM_NULL            1
#define EJS_NUM_NUMBER          1
#define EJS_NUM_OBJECT          64
#define EJS_NUM_METHOD          64
#define EJS_NUM_STRING          64
#define EJS_NUM_POINTER         1
#define EJS_NUM_TYPE            48
#define EJS_NUM_VOID            1
#endif

/*
 *  Sanity constants. Only for sanity checking. Set large enough to never be a
 *  real limit but low enough to catch some errors in development.
 */
#define EJS_MAX_PROP            (8192)          /* Max properties per class */
#define EJS_MAX_POOL            (4*1024*1024)   /* Size of constant pool */
#define EJS_MAX_ARGS            (1024)          /* Max number of args */
#define EJS_MAX_LOCALS          (10*1024)       /* Max number of locals */
#define EJS_MAX_EXCEPTIONS      (1024)          /* Max number of exceptions */
#define EJS_MAX_TRAITS          (0x7fff)        /* Max number of declared properties per block */

/*
 *  Should not need to change these
 */
#define EJS_INC_ARGS            8               /* Frame stack increment */


#define EJS_MAX_BASE_CLASSES    256             /* Max inheritance chain */
#define EJS_DOC_HASH_SIZE       10007           /* Size of doc hash table */


/*
 *  Compiler constants
 */
#define EC_MAX_INCLUDE          32              /* Max number of nested includes */
#define EC_LINE_INCR            1024            /* Growth increment for input lines */
#define EC_TOKEN_INCR           256             /* Growth increment for tokens */
#define EC_MAX_LOOK_AHEAD       8
#define EC_BUFSIZE              4096            /* General buffer size */
#define EC_MAX_ERRORS           25              /* Max compilation errors before giving up */

//  TODO - resize smaller
#define EC_CODE_BUFSIZE         (64*1024)       /* Initial size of code gen buffer */
#define EC_NUM_PAK_PROP         32              /* Initial number of properties */

/*
 *  File extensions
 */
#define EJS_MODULE_EXT          ".mod"
#define EJS_SOURCE_EXT          ".es"
#define EJS_LISTING_EXT         ".lst"

#ifdef __cplusplus
}
#endif
#endif /* _h_EJS_TUNE */

/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/**
 *  ejsByteCode.h - Ejscript virtual machine byte code
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

#ifndef _h_EJS_BYTECODE
#define _h_EJS_BYTECODE 1

#ifdef __cplusplus
extern "C" {
#endif

/********************************** Defines ***********************************/
/**
 *  Ejscript Byte Codes
 *  @description This enumeration defines the supported Ejscript byte code. Order matters here, don't sort. 
 *  Code generation and the interpreter will do arithmetic on these opcodes and rely on the ordering below 
 *  when calculating LOAD to slot N style op codes. In particular, the BR*_8 must be grouped after the word branches.
 *  @stability Prototype
 *  @see
 */
typedef enum EjsOpCode {

    EJS_OP_ADD,
    EJS_OP_ADD_NAMESPACE,
    EJS_OP_ADD_NAMESPACE_REF,
    EJS_OP_AND,
    EJS_OP_BRANCH_EQ,
    EJS_OP_BRANCH_STRICTLY_EQ,
    EJS_OP_BRANCH_FALSE,
    EJS_OP_BRANCH_GE,
    EJS_OP_BRANCH_GT,
    EJS_OP_BRANCH_LE,
    EJS_OP_BRANCH_LT,
    EJS_OP_BRANCH_NE,
    EJS_OP_BRANCH_STRICTLY_NE,
    EJS_OP_BRANCH_NULL,
    EJS_OP_BRANCH_NOT_ZERO,
    EJS_OP_BRANCH_TRUE,
    EJS_OP_BRANCH_UNDEFINED,
    EJS_OP_BRANCH_ZERO,
    EJS_OP_BRANCH_FALSE_8,
    EJS_OP_BRANCH_TRUE_8,
    EJS_OP_BREAKPOINT,

    EJS_OP_CALL,
    EJS_OP_CALL_GLOBAL_SLOT,
    EJS_OP_CALL_OBJ_SLOT,
    EJS_OP_CALL_THIS_SLOT,
    EJS_OP_CALL_BLOCK_SLOT,
    EJS_OP_CALL_OBJ_INSTANCE_SLOT,
    EJS_OP_CALL_OBJ_STATIC_SLOT,
    EJS_OP_CALL_THIS_STATIC_SLOT,
    EJS_OP_CALL_OBJ_NAME,
    EJS_OP_CALL_SCOPED_NAME,
    EJS_OP_CALL_CONSTRUCTOR,
    EJS_OP_CALL_NEXT_CONSTRUCTOR,

    EJS_OP_CAST,
    EJS_OP_CAST_BOOLEAN,
    EJS_OP_CLOSE_BLOCK,
    EJS_OP_CLOSE_WITH,
    EJS_OP_COMPARE_EQ,
    EJS_OP_COMPARE_STRICTLY_EQ,
    EJS_OP_COMPARE_FALSE,
    EJS_OP_COMPARE_GE,
    EJS_OP_COMPARE_GT,
    EJS_OP_COMPARE_LE,
    EJS_OP_COMPARE_LT,
    EJS_OP_COMPARE_NE,
    EJS_OP_COMPARE_STRICTLY_NE,
    EJS_OP_COMPARE_NULL,
    EJS_OP_COMPARE_NOT_ZERO,
    EJS_OP_COMPARE_TRUE,
    EJS_OP_COMPARE_UNDEFINED,
    EJS_OP_COMPARE_ZERO,
    EJS_OP_DEBUG,
    EJS_OP_DEFINE_CLASS,
    EJS_OP_DEFINE_FUNCTION,
    EJS_OP_DEFINE_GLOBAL_FUNCTION,
    EJS_OP_DELETE_NAME_EXPR,
    EJS_OP_DELETE,
    EJS_OP_DELETE_NAME,
    EJS_OP_DIV,
    EJS_OP_DUP,
    EJS_OP_DUP2,
    EJS_OP_END_CODE,
    EJS_OP_END_EXCEPTION,
    EJS_OP_GOTO,
    EJS_OP_GOTO_8,
    EJS_OP_INC,
    EJS_OP_INIT_DEFAULT_ARGS,
    EJS_OP_INIT_DEFAULT_ARGS_8,
    EJS_OP_INST_OF,
    EJS_OP_IS_A,
    EJS_OP_LOAD_0,
    EJS_OP_LOAD_1,
    EJS_OP_LOAD_2,
    EJS_OP_LOAD_3,
    EJS_OP_LOAD_4,
    EJS_OP_LOAD_5,
    EJS_OP_LOAD_6,
    EJS_OP_LOAD_7,
    EJS_OP_LOAD_8,
    EJS_OP_LOAD_9,
    EJS_OP_LOAD_DOUBLE,
    EJS_OP_LOAD_FALSE,
    EJS_OP_LOAD_GLOBAL,
    EJS_OP_LOAD_INT_16,
    EJS_OP_LOAD_INT_32,
    EJS_OP_LOAD_INT_64,
    EJS_OP_LOAD_INT_8,
    EJS_OP_LOAD_M1,
    EJS_OP_LOAD_NAME,
    EJS_OP_LOAD_NAMESPACE,
    EJS_OP_LOAD_NULL,
    EJS_OP_LOAD_REGEXP,
    EJS_OP_LOAD_STRING,
    EJS_OP_LOAD_THIS,
    EJS_OP_LOAD_TRUE,
    EJS_OP_LOAD_UNDEFINED,
    EJS_OP_LOAD_XML,
    EJS_OP_GET_LOCAL_SLOT_0,
    EJS_OP_GET_LOCAL_SLOT_1,
    EJS_OP_GET_LOCAL_SLOT_2,
    EJS_OP_GET_LOCAL_SLOT_3,
    EJS_OP_GET_LOCAL_SLOT_4,
    EJS_OP_GET_LOCAL_SLOT_5,
    EJS_OP_GET_LOCAL_SLOT_6,
    EJS_OP_GET_LOCAL_SLOT_7,
    EJS_OP_GET_LOCAL_SLOT_8,
    EJS_OP_GET_LOCAL_SLOT_9,
    EJS_OP_GET_OBJ_SLOT_0,
    EJS_OP_GET_OBJ_SLOT_1,
    EJS_OP_GET_OBJ_SLOT_2,
    EJS_OP_GET_OBJ_SLOT_3,
    EJS_OP_GET_OBJ_SLOT_4,
    EJS_OP_GET_OBJ_SLOT_5,
    EJS_OP_GET_OBJ_SLOT_6,
    EJS_OP_GET_OBJ_SLOT_7,
    EJS_OP_GET_OBJ_SLOT_8,
    EJS_OP_GET_OBJ_SLOT_9,
    EJS_OP_GET_THIS_SLOT_0,
    EJS_OP_GET_THIS_SLOT_1,
    EJS_OP_GET_THIS_SLOT_2,
    EJS_OP_GET_THIS_SLOT_3,
    EJS_OP_GET_THIS_SLOT_4,
    EJS_OP_GET_THIS_SLOT_5,
    EJS_OP_GET_THIS_SLOT_6,
    EJS_OP_GET_THIS_SLOT_7,
    EJS_OP_GET_THIS_SLOT_8,
    EJS_OP_GET_THIS_SLOT_9,
    EJS_OP_GET_SCOPED_NAME,
    EJS_OP_GET_OBJ_NAME,
    EJS_OP_GET_OBJ_NAME_EXPR,
    EJS_OP_GET_BLOCK_SLOT,
    EJS_OP_GET_GLOBAL_SLOT,
    EJS_OP_GET_LOCAL_SLOT,
    EJS_OP_GET_OBJ_SLOT,
    EJS_OP_GET_THIS_SLOT,
    EJS_OP_GET_TYPE_SLOT,
    EJS_OP_GET_THIS_TYPE_SLOT,
    EJS_OP_IN,
    EJS_OP_LIKE,
    EJS_OP_LOGICAL_NOT,
    EJS_OP_MUL,
    EJS_OP_NEG,
    EJS_OP_NEW,
    EJS_OP_NEW_ARRAY,
    EJS_OP_NEW_OBJECT,
    EJS_OP_NOP,
    EJS_OP_NOT,
    EJS_OP_OPEN_BLOCK,
    EJS_OP_OPEN_WITH,
    EJS_OP_OR,
    EJS_OP_POP,
    EJS_OP_POP_ITEMS,
    EJS_OP_PUSH_CATCH_ARG,
    EJS_OP_PUSH_RESULT,
    EJS_OP_PUT_LOCAL_SLOT_0,
    EJS_OP_PUT_LOCAL_SLOT_1,
    EJS_OP_PUT_LOCAL_SLOT_2,
    EJS_OP_PUT_LOCAL_SLOT_3,
    EJS_OP_PUT_LOCAL_SLOT_4,
    EJS_OP_PUT_LOCAL_SLOT_5,
    EJS_OP_PUT_LOCAL_SLOT_6,
    EJS_OP_PUT_LOCAL_SLOT_7,
    EJS_OP_PUT_LOCAL_SLOT_8,
    EJS_OP_PUT_LOCAL_SLOT_9,
    EJS_OP_PUT_OBJ_SLOT_0,
    EJS_OP_PUT_OBJ_SLOT_1,
    EJS_OP_PUT_OBJ_SLOT_2,
    EJS_OP_PUT_OBJ_SLOT_3,
    EJS_OP_PUT_OBJ_SLOT_4,
    EJS_OP_PUT_OBJ_SLOT_5,
    EJS_OP_PUT_OBJ_SLOT_6,
    EJS_OP_PUT_OBJ_SLOT_7,
    EJS_OP_PUT_OBJ_SLOT_8,
    EJS_OP_PUT_OBJ_SLOT_9,
    EJS_OP_PUT_THIS_SLOT_0,
    EJS_OP_PUT_THIS_SLOT_1,
    EJS_OP_PUT_THIS_SLOT_2,
    EJS_OP_PUT_THIS_SLOT_3,
    EJS_OP_PUT_THIS_SLOT_4,
    EJS_OP_PUT_THIS_SLOT_5,
    EJS_OP_PUT_THIS_SLOT_6,
    EJS_OP_PUT_THIS_SLOT_7,
    EJS_OP_PUT_THIS_SLOT_8,
    EJS_OP_PUT_THIS_SLOT_9,
    EJS_OP_PUT_OBJ_NAME_EXPR,
    EJS_OP_PUT_SCOPED_NAME_EXPR,
    EJS_OP_PUT_OBJ_NAME,
    EJS_OP_PUT_SCOPED_NAME,
    EJS_OP_PUT_BLOCK_SLOT,
    EJS_OP_PUT_GLOBAL_SLOT,
    EJS_OP_PUT_LOCAL_SLOT,
    EJS_OP_PUT_OBJ_SLOT,
    EJS_OP_PUT_THIS_SLOT,
    EJS_OP_PUT_TYPE_SLOT,
    EJS_OP_PUT_THIS_TYPE_SLOT,
    EJS_OP_REM,
    EJS_OP_RETURN,
    EJS_OP_RETURN_VALUE,
    EJS_OP_SAVE_RESULT,
    EJS_OP_SHL,
    EJS_OP_SHR,
    EJS_OP_SUB,
    EJS_OP_SUPER,
    EJS_OP_SWAP,
    EJS_OP_THROW,
    EJS_OP_TYPE_OF,
    EJS_OP_USHR,
    EJS_OP_XOR,

} EjsOpCode;

#define ejsIsCall(opcode) (EJS_OP_CALL <= opcode && opcode <= EJS_OP_CALL_NEXT_CONSTRUCTOR

#ifdef __cplusplus
}
#endif

#endif /* _h_EJS_BYTECODE */

/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/*
 *  ejsGC.h - Garbage collection for the Ejscript virtual machine.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

#ifndef _h_EJS_GC
#define _h_EJS_GC 1

/********************************** Includes **********************************/

#ifdef __cplusplus
extern "C" {
#endif

/********************************** Defines ***********************************/
#if !DOXYGEN
/*
 *  Forward declare types
 */
struct Ejs;
#endif

/*
 *  Default GC tune factors
 *  TODO -- move to ejsTune.h
 */
#define EJS_MIN_TIME_FOR_GC         300     /* Need 1/3 sec for GC */
#define EJS_GC_MIN_WORK_QUOTA       50      /* Min to stop thrashing */
    
/** 
 * Magic number when allocated 
 */
#define EJS_GC_IN_USE       0xe801e2ec
#define EJS_GC_FREE         0xe701e3ea

/*
 *  Object generations
 */
#define EJS_GEN_NEW         0           /* New objects */
#define EJS_GEN_YOUNG       1           /* Young objects, survived one collection */
#define EJS_GEN_OLD         2           /* Longer lived application objects, survived multiple collections */
#define EJS_GEN_ETERNAL     3           /* Builtin objects that live forever */
#define EJS_MAX_GEN         4           /* Number of generations for object allocation */

/*
 *  Collection modes
 */
#define EJS_GC_QUICK        0           /* Only collect the newest garbage */
#define EJS_GC_SMART        1           /* Collect from generations that have activity */
#define EJS_GC_ALL          2           /* Collect all garbage */

/*
 *  Per generation structure
 */
typedef struct EjsGen
{
    struct EjsVar   *next;              /* Queue of objects in this generation */
    struct EjsVar   **roots;            /* Cross generational roots for this generation */
    struct EjsVar   **nextRoot;         /* Reference to the next free slot in roots */
    struct EjsVar   **lastRoot;         /* Reference to the last +1 slot in roots */
    struct EjsVar   **peakRoot;         /* Peak root usage */
    int             rootCount;          /* Convenience count of root objects in use */
    int             inUse;              /* Count of objects marked as being used */
    uint            newlyCreated;       /* Count of new objects since last sweep of this generation */
    uint            totalReclaimed;     /* Total blocks reclaimed on sweeps */
    uint            totalSweeps;        /* Total sweeps */
} EjsGen;



/*
 *  Pool of free objects of a given type. Each type maintains a free pool for faster allocations.
 *  Types in the pool have a weak reference and may be reclaimed.
 */
typedef struct EjsPool
{
    struct EjsVar   *next;              /* Next free in pool */
    struct EjsType  *type;              /* Type corresponding to this pool */
    int             allocated;          /* Count of instances created */
    int             peakAllocated;      /* High water mark for allocated */
    int             count;              /* Count in pool */
    int             peakCount;          /* High water mark for count */
    int             reuse;              /* Count of reuses */
} EjsPool;



/*
 *  Garbage collector control
 */
typedef struct EjsGC {

    EjsGen      generations[EJS_MAX_GEN];

    EjsPool     *pools;                 /* Object pools */
    int         numPools;               /* Count of object pools */
    
    uint        allocGeneration;        /* Current generation accepting objects */
    uint        collectGeneration;      /* Current generation doing GC */
    uint        markGenRef;             /* Generation to mark objects */
    uint        firstGlobal;            /* First global slots to examine */

    bool        collecting;             /* Running garbage collection */
    bool        enabled;                /* GC is enabled */
    bool        required;               /* GC is now required */
    bool        enableDemandCollect;    /* Enable GC on demand */
    bool        enableIdleCollect;      /* Enable GC at idle time */

    int         degraded;               /* Have exceeded redlineMemory */
    int         overflow;               /* Cross generational overflow - must do full gc */
    int         workQuota;              /* Quota of work before GC */
    int         workDone;               /* Count of allocations */

    uint        allocatedTypes;         /* Count of types allocated */
    uint        peakAllocatedTypes;     /* Peak allocated types */ 
    uint        allocatedObjects;       /* Count of objects allocated */
    uint        peakAllocatedObjects;   /* Peak allocated */ 
    uint        peakMemory;             /* Peak memory usage */
    uint        totalAllocated;         /* Total count of allocation calls */
    uint        totalReclaimed;         /* Total blocks reclaimed on sweeps */
    uint        totalOverflows;         /* Total overflows  */
    uint        totalRedlines;          /* Total times redline limit exceeded */
    uint        totalSweeps;            /* Total sweeps */

#if BLD_DEBUG
    int         indent;                 /* Indent formatting */
#endif

} EjsGC;

#if FUTURE
#define ejsSetReference(ejs, obj, value) \
    if ((value) && ((struct EjsVar*) (obj))->generation < ((struct EjsVar*) (value))->linkTo) { \
        ((struct EjsVar*) (value))->linkTo = ((struct EjsVar*) (obj))->generation; \
        ejsSetCrossGenRef(ejs, ((struct EjsVar*)(value))->generation, ((struct EjsVar*) (obj))); \
    }
#else
//DDD
extern void ejsSetReference(struct Ejs *ejs, struct EjsVar *obj, struct EjsVar *value);
#endif

extern void ejsSetCrossGenRef(struct Ejs *ejs, int generation, struct EjsVar *obj);
extern int  ejsSetGeneration(struct Ejs *ejs, int generation);

/******************************** Internal API ********************************/

extern void     ejsAnalyzeGlobal(struct Ejs *ejs);
extern int      ejsCreateGCService(struct Ejs *ejs);
//DDD
extern int      ejsIsTimeForGC(struct Ejs *ejs, int timeTillNextEvent);
//DDD
extern void     ejsCollectGarbage(struct Ejs *ejs, int mode);
extern void     ejsEnableGC(struct Ejs *ejs, bool on);
extern void     ejsTraceMark(struct Ejs *ejs, struct EjsVar *vp);
extern void     ejsGracefulDegrade(struct Ejs *ejs);
//DDD
extern void     ejsPrintAllocReport(struct Ejs *ejs);
//DDD
extern void     ejsMakePermanent(struct Ejs *ejs, struct EjsVar *vp);
//DDD
extern void     ejsMakeTransient(struct Ejs *ejs, struct EjsVar *vp);

#if UNUSED
/*
 *  Could possibly make these routines public
 */
extern int      ejsSetGCMaxMemory(struct Ejs *ejs, uint maxMemory);
#endif

/********************************* Public API *********************************/

#ifdef __cplusplus
}
#endif
#endif /* _h_EJS_GC */

/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/*
 *  ejsVm.h - Virtual Machine header.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#ifndef _h_EJS_VM_h
#define _h_EJS_VM_h 1

#include    "mpr.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************************** Defines ***********************************/
#if !DOXYGEN
/*
 *  Forward declare types
 */
struct Ejs;
struct EjsBlock;
struct EjsList;
struct EjsFrame;
struct EjsFunction;
struct EjsModule;
struct EjsStack;
struct EjsVar;
#endif

/*
 *  Language compliance levels
 */
#define EJS_SPEC_ECMA           0           /**< Run in strict ECMA-262 compliance mode */
#define EJS_SPEC_PLUS           1           /**< Run in enhanced ECMA-262 with non-breaking changes */
#define EJS_SPEC_FIXED          2           /**< Run with ECMA-262 plus enhancements and add breaking fixes */

#define LANG_ECMA(cp)       (cp->fileState->mode == PRAGMA_MODE_STANDARD)
#define LANG_PLUS(cp)         (cp->fileState->mode == PRAGMA_MODE_STRICT)
#define LANG_FIXED(cp)         (cp->fileState->mode == PRAGMA_MODE_STRICT)

/*
 *  Interpreter flags
 */
#define EJS_FLAG_EVENT          0x1         /**< Event pending */
#define EJS_FLAG_EMPTY          0x2         /**< Create an empty interpreter without native elements */
#define EJS_FLAG_COMPILER       0x4         /**< Running inside the compiler */
#define EJS_FLAG_NO_EXE         0x8         /**< VM will not execute code. Used by compiler without --run */
#define EJS_FLAG_MASTER         0x20        /**< Create a master interpreter */
#define EJS_FLAG_DOC            0x40        /**< Load documentation from modules */
#define EJS_FLAG_EXIT           0x80        /**< Interpreter should exit */
#define EJS_FLAG_LOADING        0x100       /**< Loading a module */
#define EJS_FLAG_NOEXIT         0x200       /**< App should service events and not exit */

#define EJS_STACK_ARG           -1          /* Offset to locate first arg */

#define EJS_FRAME_SIZE          MPR_ALLOC_ALIGN(sizeof(EjsFrame))
#define EJS_FRAME_VAR_SIZE      (MPR_ALLOC_ALIGN(sizeof(EjsFrame)) / sizeof(EjsVar*))

/**
 *  Qualified name structure
 *  @description All names in Ejscript consist of a property name and a name space. Namespaces provide discrete
 *      spaces to manage and minimize name conflicts. These names will soon be converted to unicode.
 *  @stability Prototype
 *  @defgroup EjsName EjsName
 *  @see EjsName ejsName ejsAllocName ejsDupName ejsCopyName
 */       
typedef struct EjsName {
    cchar       *name;                          /**< Property name */
    cchar       *space;                         /**< Property namespace */
} EjsName;


/**
 *  Initialize a Qualified Name structure
 *  @description Initialize the statically allocated qualified name structure using a name and namespace.
 *  @param qname Reference to an existing, uninitialized EjsName structure
 *  @param space Namespace string
 *  @param name Name string
 *  @return A reference to the qname structure
 *  @ingroup EjsName
 */
extern EjsName *ejsName(struct EjsName *qname, cchar *space, cchar *name);

/**
 *  Allocate and Initialize  a Qualified Name structure
 *  @description Create and initialize a qualified name structure using a name and namespace.
 *  @param ctx Any memory context returned by mprAlloc
 *  @param space Namespace string
 *  @param name Name string
 *  @return A reference to an allocated EjsName structure. Caller must free.
 *  @ingroup EjsName
 */
extern EjsName *ejsAllocName(MprCtx ctx, cchar *space, cchar *name);

extern EjsName *ejsDupName(MprCtx ctx, EjsName *qname);
extern EjsName ejsCopyName(MprCtx ctx, EjsName *qname);

/**
 *  Evaluation stack. 
 *  The VM Stacks grow forward in memory. A push is done by incrementing first, then storing. ie.
 *      *++top = value
 *  A pop is done by extraction then decrement. ie.
 *      value = *top--
 *  @ingroup EjsVm
 */
typedef struct EjsStack {
    struct EjsVar   **bottom;               /* Pointer to start of stack mem */
    struct EjsVar   **top;                  /* Last element pushed */
    struct EjsVar   **end;                  /* Only used on non-virtual memory systems */
    int             size;
} EjsStack;


/**
 *  Lookup State.
 *  @description Location information returned when looking up properties.
 *  @ingroup EjsVm
 */
typedef struct EjsLookup
{
    struct EjsVar   *obj;                   /* Final object / Type containing the variable */
    int             slotNum;                /* Final slot in obj containing the variable reference */

    uint            nthBase: 8;             /* Property on Nth super type -- count from the object */
    uint            nthBlock: 8;            /* Property on Nth block in the scope chain -- count from the end */
    uint            useThis: 1;             /* Property accessible via "this." */
    uint            instanceProperty: 1;    /* Property is an instance property */
    uint            ownerIsType: 1;         /* Original object owning the property is a type */

    /*
     *  Just for the compiler
     */
    struct EjsVar   *originalObj;           /* Original object used for the search */
    struct EjsVar   *ref;                   /* Actual property reference */
    struct EjsTrait *trait;                 /* Property trait describing the property */
    struct EjsName  name;                   /* Name and namespace used to find the property */

} EjsLookup;


/**
 *  Ejsript Interperter Management
 *  @description The Ejs structure contains the state for a single interpreter. The #ejsCreate routine may be used
 *      to create multiple interpreters and returns a reference to be used in subsequent Ejscript API calls.
 *  @stability Prototype.
 *  @defgroup Ejs Ejs
 *  @see ejsCreate, ejsCreateService, ejsSetSearchPath, ejsEvalFile, ejsEvalScript, ejsExit
 */
typedef struct Ejs {
    struct EjsFrame     *frame;             /* Current frame */
    struct EjsVar       *result;            /* Last expression result */
    struct EjsStack     stack;              /* Evaluation operand stack */
    struct EjsService   *service;           /* Back pointer to the service */
    struct Ejs          *master;            /* Inherit builtin types from the master */
    EjsGC               gc;                 /* Garbage collector state */

    /*
     *  Essential types
     */
    struct EjsType      *arrayType;         /* Array type */
    struct EjsType      *blockType;         /* Block type */
    struct EjsType      *booleanType;       /* Boolean type */
    struct EjsType      *byteArrayType;     /* ByteArray type */
    struct EjsType      *dateType;          /* DAte type */
    struct EjsType      *errorType;         /* Error type */
    struct EjsType      *functionType;      /* Function type */
    struct EjsType      *iteratorType;      /* Iterator type */
    struct EjsType      *namespaceType;     /* Namespace type */
    struct EjsType      *nullType;          /* Null type */
    struct EjsType      *numberType;        /* Default numeric type */
    struct EjsType      *objectType;        /* Object type */
    struct EjsType      *regExpType;        /* RegExp type */
    struct EjsType      *stringType;        /* String type */
    struct EjsType      *stopIterationType; /* StopIteration type */
    struct EjsType      *typeType;          /* Type type */
    struct EjsType      *voidType;          /* Void type */
    struct EjsType      *xmlType;           /* XML type */
    struct EjsType      *xmlListType;       /* XMLList type */

    /*
     *  Key values
     */
    struct EjsVar       *global;            /* The "global" object as an EjsVar */
    struct EjsBlock     *globalBlock;       /* The "global" object as an EjsBlock */

    struct EjsString    *emptyStringValue;  /* "" value */
    struct EjsBoolean   *falseValue;        /* The "false" value */
    struct EjsNumber    *infinityValue;     /* The infinity number value */
    struct EjsNumber    *maxValue;          /* Maximum number value */
    struct EjsNumber    *minValue;          /* Minimum number value */
    struct EjsNumber    *minusOneValue;     /* The -1 number value */
    struct EjsNumber    *nanValue;          /* The "NaN" value if floating point numbers, else zero */
    struct EjsNumber    *negativeInfinityValue; /* The negative infinity number value */
    struct EjsVar       *nullValue;         /* The "null" value */
    struct EjsNumber    *oneValue;          /* The 1 number value */
    struct EjsBoolean   *trueValue;         /* The "true" value */
    struct EjsVar       *undefinedValue;    /* The "void" value */
    struct EjsNumber    *zeroValue;         /* The 0 number value */

    struct EjsNamespace *configSpace;       /* CONFIG namespace */
    struct EjsNamespace *emptySpace;        /* Empty namespace */
    struct EjsNamespace *intrinsicSpace;    /* Intrinsic namespace */
    struct EjsNamespace *iteratorSpace;     /* Iterator namespace */
    struct EjsNamespace *internalSpace;     /* Internal namespace */
    struct EjsNamespace *publicSpace;       /* Public namespace */
    struct EjsNamespace *eventsSpace;       /* ejs.events namespace */
    struct EjsNamespace *ioSpace;           /* ejs.io namespace */
    struct EjsNamespace *sysSpace;          /* ejs.sys namespace */

    char                *castTemp;          /* Temporary string for casting */
    char                *errorMsg;          /* Error message */
    char                **argv;             /* Command line args */
    int                 argc;               /* Count of command line args */
    int                 state;              /* Execution state */
    int                 flags;              /* Execution flags */
    int                 exitStatus;         /* Status to exit() */
    int                 initialized;        /* Interpreter fully initialized */
    int                 hasError;           /* Interpreter has an initialization error */
    int                 serializeDepth;     /* Serialization depth */

    struct EjsVar       *exception;         /* Pointer to exception object */
    uint                noExceptions: 1;    /* Suppress exceptions */
    bool                attention;          /* VM needs attention */

    struct EjsFrame     *frames;            /* Free list of frames */

    struct EjsTypeHelpers *defaultHelpers;  /* Default EjsVar helpers */
    struct EjsTypeHelpers *blockHelpers;    /* EjsBlock helpers */
    struct EjsTypeHelpers *objectHelpers;   /* EjsObject helpers */

    MprList             *modules;           /* Loaded modules */
    MprList             *typeFixups;        /* Loaded types to fixup */

    void                (*loaderCallback)(struct Ejs *ejs, int kind, ...);
    void                *userData;          /* User data */
    void                *handle;            /* Hosting environment handle */

    MprHashTable        *nativeModules;     /* Native module initialization callbacks */
    MprHashTable        *coreTypes;         /* Core type instances */
    MprHashTable        *standardSpaces;    /* Hash of standard namespaces (global namespaces) */

    MprHashTable        *doc;               /* Documentation */
} Ejs;


/**
 *  Ejscript Service structure
 *  @description The Ejscript service manages the overall language runtime. It 
 *      is the factory that creates interpreter instances via #ejsCreate.
 *  @ingroup Ejs
 */
typedef struct EjsService {
    char        *ejsPath;           /* Module load search path */
} EjsService;


/*********************************** Prototypes *******************************/
/**
 *  Open the Ejscript service
 *  @description One Ejscript service object is required per application. From this service, interpreters
 *      can be created.
 *  @param ctx Any memory context returned by mprAlloc
 *  @return An ejs service object
 *  @ingroup Ejs
 */
extern EjsService *ejsCreateService(MprCtx ctx);

/**
 *  Create an ejs interpreter
 *  @description Create an interpreter object to evalute Ejscript programs. Ejscript supports multiple interpreters.
 *      One interpreter can be designated as a master interpreter and then it can be cloned by supplying the master
 *      interpreter to this call. A master interpreter provides the standard system types and clone interpreters can
 *      quickly be created an utilize the master interpreter's types. This saves memory and speeds initialization.
 *  @param ctx Any memory context returned by mprAlloc
 *  @param master Optional master interpreter to clone.
 *  @param flags Optional flags to modify the interpreter behavior. Valid flags are:
 *      @li    EJS_FLAG_COMPILER       - Interpreter will compile code from source
 *      @li    EJS_FLAG_NO_EXE         - Don't execute any code. Just compile.
 *      @li    EJS_FLAG_MASTER         - Create a master interpreter
 *      @li    EJS_FLAG_DOC            - Load documentation from modules
 *      @li    EJS_FLAG_NOEXIT         - App should service events and not exit unless explicitly instructed
 *  @return A new interpreter
 *  @ingroup Ejs
 */
extern Ejs *ejsCreate(MprCtx ctx, struct Ejs *master, int flags);

/**
 *  Set the module search path
 *  @description Set the ejs module search path. The search path is by default set to the value of the EJSPATH
 *      environment directory. Ejsript will search for modules by name. The search strategy is:
 *      Given a name "a.b.c", scan for:
 *      @li File named a.b.c
 *      @li File named a/b/c
 *      @li File named a.b.c in EJSPATH
 *      @li File named a/b/c in EJSPATH
 *      @li File named c in EJSPATH
 *
 *  Ejs will search for files with no extension and also search for modules with a ".mod" extension. If there is
 *  a shared library of the same name with a shared library extension (.so, .dll, .dylib) and the module requires 
 *  native code, then the shared library will also be loaded.
 *  @param sp Ejs service returned from #ejsCreateService
 *  @param ejsPath Search path. This is a colon (or semicolon on Windows) separated string of directories.
 *  @ingroup Ejs
 */
extern void ejsSetSearchPath(EjsService *sp, cchar *ejsPath);

/**
 *  Evaluate a file
 *  @description Evaluate a file containing an Ejscript. This requires linking with the Ejscript compiler library (libec). 
 *  @param path Filename of the script to evaluate
 *  @return Return zero on success. Otherwise return a negative Mpr error code.
 *  @ingroup Ejs
 */
extern int ejsEvalFile(cchar *path);

//  TODO - document
extern int ejsLoadScript(Ejs *ejs, cchar *path, int flags);

/**
 *  Evaluate a module
 *  @description Evaluate a module containing compiled Ejscript.
 *  @param path Filename of the module to evaluate.
 *  @return Return zero on success. Otherwise return a negative Mpr error code.
 *  @ingroup Ejs
 */
extern int ejsEvalModule(cchar *path);

/**
 *  Evaluate a script
 *  @description Evaluate a script. This requires linking with the Ejscript compiler library (libec). 
 *  @param script Script to evaluate
 *  @return Return zero on success. Otherwise return a negative Mpr error code.
 *  @ingroup Ejs
 */
extern int ejsEvalScript(cchar *script);

/**
 *  Instruct the interpreter to exit.
 *  @description This will instruct the interpreter to cease interpreting any further script code.
 *  @param ejs Interpeter object returned from #ejsCreate
 *  @param status Reserved and ignored
 *  @ingroup Ejs
 */
extern void ejsExit(Ejs *ejs, int status);

/**
 *  Get the hosting handle
 *  @description The interpreter can store a hosting handle. This is typically a web server object if hosted inside
 *      a web server
 *  @param ejs Interpeter object returned from #ejsCreate
 *  @return Hosting handle
 *  @ingroup Ejs
 */
extern void *ejsGetHandle(Ejs *ejs);

/**
 *  Run a script
 *  @description Run a script that has previously ben compiled by ecCompile
 *  @param ejs Interpeter object returned from #ejsCreate
 *  @return Zero if successful, otherwise a non-zero Mpr error code.
 */
extern int ejsRun(Ejs *ejs);

/**
 *  Throw an exception
 *  @description Throw an exception object 
 *  @param ejs Interpeter object returned from #ejsCreate
 *  @param error Exception argument object.
 *  @return The exception argument for chaining.
 *  @ingroup Ejs
 */
extern struct EjsVar *ejsThrowException(Ejs *ejs, struct EjsVar *error);

/**
 *  Report an error message using the MprLog error channel
 *  @description This will emit an error message of the format:
 *      @li program:line:errorCode:SEVERITY: message
 *  @param ejs Interpeter object returned from #ejsCreate
 *  @param fmt Is an alternate printf style format to emit if the interpreter has no valid error message.
 *  @param ... Arguments for fmt
 *  @ingroup Ejs
 */
extern void ejsReportError(Ejs *ejs, char *fmt, ...);

extern int ejsAddModule(Ejs *ejs, struct EjsModule *up);
extern struct EjsVar *ejsCastOperands(Ejs *ejs, struct EjsVar *lhs, int opcode,  struct EjsVar *rhs);
extern int ejsCheckModuleLoaded(Ejs *ejs, cchar *name);
extern void ejsClearExiting(Ejs *ejs);
extern struct EjsVar *ejsCreateException(Ejs *ejs, int slot, cchar *fmt, va_list fmtArgs);
extern MprList *ejsGetModuleList(Ejs *ejs);
extern struct EjsVar *ejsGetVarByName(Ejs *ejs, struct EjsVar *vp, EjsName *name, bool anySpace, EjsLookup *lookup);
extern int ejsInitStack(Ejs *ejs);
extern void ejsLog(Ejs *ejs, cchar *fmt, ...);
extern int ejsLookupVar(Ejs *ejs, struct EjsVar *vp, EjsName *name, bool anySpace, EjsLookup *lookup);
extern int ejsLookupVarInBlock(Ejs *ejs, struct EjsVar *vp, EjsName *name, bool anySpace, EjsLookup *lookup);
extern struct EjsModule *ejsLookupModule(Ejs *ejs, cchar *name);
extern int ejsLookupScope(Ejs *ejs, EjsName *name, bool anySpace, EjsLookup *lookup);
extern void ejsMemoryFailure(MprCtx ctx, uint size, uint total, bool granted);
extern int ejsPushBlock(MprCtx ctx, struct EjsList *scopeChain, struct EjsBlock *scopeBlock);
extern struct EjsVar *ejsPopBlock(struct EjsList *scopeChain);
extern struct EjsVar *ejsPopBlock(struct EjsList *scopeChain);
extern int ejsRemoveModule(Ejs *ejs, struct EjsModule *up);
extern int ejsRunProgram(Ejs *ejs, cchar *className, cchar *methodName);
extern void ejsSetHandle(Ejs *ejs, void *handle);
extern void ejsShowCurrentScope(Ejs *ejs);
extern void ejsShowStack(Ejs *ejs);
extern void ejsShowBlockScope(Ejs *ejs, struct EjsBlock *block);
extern int ejsStartLogging(Mpr *mpr, char *logSpec);
extern struct EjsTypeHelpers *ejsGetDefaultHelpers(Ejs *ejs);
extern struct EjsTypeHelpers *ejsGetObjectHelpers(Ejs *ejs);
extern struct EjsTypeHelpers *ejsGetBlockHelpers(Ejs *ejs);

#ifdef __cplusplus
}
#endif

#endif /* _h_EJS_VM_h */

/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/*
 *  ejsCore.h - Header for the core types.
 *
 *  The VM provides core types like numbers, strings and objects. This header provides their API.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#ifndef _h_EJS_CORE
#define _h_EJS_CORE 1

#include    "mpr.h"

/*
 *  Include the standard slot definitions
 */
#if BLD_FEATURE_EJS_DB
#endif
#if BLD_FEATURE_EJS_WEB
#endif


#ifdef __cplusplus
extern "C" {
#endif

/********************************* Defines ************************************/

#if !DOXYGEN
/*
 *  Forward declare types
 */
struct EjsBlock;
struct EjsGC;
struct EjsNames;
struct EjsModule;
struct EjsNamespace;
struct EjsObject;
struct EjsService;
struct EjsString;
struct EjsType;
struct EjsTypeHelpers;
struct EjsVar;
#endif

/*
 *  Property Trait Attributes. These are used in EjsTrait.
 *  TODO - split attributes into type, function and property attributes. Some will be shared
 *  TODO - prefix then with ATYPE, ABLOCK, AFUNCTION ...
 */
/*
 *  Trait attributes applying to all properties
 */
#define EJS_ATTR_NATIVE                 (1 << 1)                /**< Property is builtin */
#define EJS_ATTR_PROTOTYPE              (1 << 2)                /**< TODO - not supported yet (KEEP) */

/*
 *  Type flags. These bits are stored in EjsType. We make them attributes so a single attribute mask can
 *  store all attribute information.
 */
#define EJS_ATTR_INITIALIZER            (1 << 3)                /**< Initializer code */
#define EJS_ATTR_HAS_CONSTRUCTOR        (1 << 4)                /**< Type has a constructor */

//  TODO - not used
#define EJS_ATTR_OPER_OVERLOAD          (1 << 5)                /**< Type overloaded operators */
#define EJS_ATTR_OBJECT_HELPERS         (1 << 6)                /**< Use object helper methods */
#define EJS_ATTR_BLOCK_HELPERS          (1 << 7)                /**< Use block helper methods */

#define EJS_ATTR_SLOTS_NEED_FIXUP       (1 << 8)                /**< Missing inherited slots */
#define EJS_ATTR_HAS_INITIALIZER        (1 << 9)                /**< Type has an initializer */
#define EJS_ATTR_HAS_STATIC_INITIALIZER (1 << 10)               /**< Type has an initializer */
#define EJS_ATTR_CALLS_SUPER            (1 << 11)               /**< Constructor calls super() */
#define EJS_ATTR_BUILTIN                (1 << 12)               /**< Type is builtin */
#define EJS_ATTR_NO_BIND                (1 << 13)               /**< Type properties are never bound */

/*
 *  Attributes on classes [final, native, override, prototype]
 *  Attributes on properties [const, dynamic, enumerable, native, readonly, static]
 *  Attributes for methods [get set operator]
 */
#define EJS_ATTR_FINAL                  (1 << 14)               /**< Type can't be subclassed */
#define EJS_ATTR_OVERRIDE               (1 << 15)               /**< Override base type */
#define EJS_ATTR_CONST                  (1 << 16)               /**< Property is constant after first assignment */
#define EJS_ATTR_DYNAMIC_INSTANCE       (1 << 17)               /**< Instances are not sealed */
#define EJS_ATTR_OBJECT                 (1 << 18)               /**< Instances are based on EjsObject */
#define EJS_ATTR_ENUMERABLE             (1 << 19)               /**< Property is visible */
#define EJS_ATTR_GETTER                 (1 << 20)               /**< Property is a getter */
#define EJS_ATTR_LITERAL_GETTER         (1 << 21)               /**< Property is a getter in object literal */
#define EJS_ATTR_OPERATOR               (1 << 22)               /**< Operator */
#define EJS_ATTR_READONLY               (1 << 23)               /**< Readonly outside class */
#define EJS_ATTR_SETTER                 (1 << 24)               /**< Property is a settter */
#define EJS_ATTR_STATIC                 (1 << 25)               /**< Class static property */
#define EJS_ATTR_CONSTRUCTOR            (1 << 26)               /**< Method is a constructor */
#define EJS_ATTR_REST                   (1 << 27)               /**< Parameter is a "..." rest */
#define EJS_ATTR_FULL_SCOPE             (1 << 28)               /**< Function needs closure when defined */
#define EJS_ATTR_HAS_RETURN             (1 << 29)               /**< Function has a return statement */
#define EJS_ATTR_INTERFACE              (1 << 30)               /**< Class is an interface */

/*
 *  Bits used just by the loader
 */
#define EJS_ATTR_HAS_VALUE              (1 << 31)               /**< Property has a value record */

/**
 *  Configured numeric type. The configure program will define BLD_FEATURE_NUM_TYPE to be either 
 *  double, float, int or int64
 */
typedef BLD_FEATURE_NUM_TYPE MprNumber;

/*
 *  Sizes (in bytes) of encoded types in a ByteArray. TODO - remove and make configuration driven.
 */
#define EJS_SIZE_BOOLEAN        1
#define EJS_SIZE_SHORT          2
#define EJS_SIZE_INT            4
#define EJS_SIZE_LONG           8
#define EJS_SIZE_DOUBLE         8
#define EJS_SIZE_DATE           8

/*
 *  Language enhancement modes
 */
#define EJS_LANG_ECMA           0               /* Strict Ecma-262 */
#define EJS_LANG_PLUS           1               /* Ecma plus enhancements */
#define EJS_LANG_FIXED          2               /* Ecma plus enhancements and breaking fixes */

/*
 *  Reserved and system Namespaces
 *  TODO - these should be unforgeable
 */
#define EJS_EMPTY_NAMESPACE         ""
#define EJS_ES4_NAMESPACE           "__ES4__"
#define EJS_INTERNAL_NAMESPACE      "internal"
#define EJS_PRIVATE_NAMESPACE       "private"
#define EJS_PROTECTED_NAMESPACE     "protected"
#define EJS_PUBLIC_NAMESPACE        "public"
#define EJS_IO_NAMESPACE            "ejs.io"
#define EJS_EVENTS_NAMESPACE        "ejs.events"
#define EJS_SYS_NAMESPACE           "ejs.sys"
#define EJS_INTRINSIC_NAMESPACE     "intrinsic"
#define EJS_ITERATOR_NAMESPACE      "iterator"
#define EJS_CONFIG_NAMESPACE        "CONFIG"
#define EJS_BLOCK_NAMESPACE         "-block-"
#define EJS_INIT_NAMESPACE          "-initializer-"
#define EJS_CONSTRUCTOR_NAMESPACE   "-constructor-"

/*
 *  File access modes. Must match the constants in File.es
 */
#define EJS_FILE_OPEN           0x1
#define EJS_FILE_READ           0x2
#define EJS_FILE_WRITE          0x4
#define EJS_FILE_APPEND         0x8
#define EJS_FILE_CREATE         0x10
#define EJS_FILE_TRUNCATE       0x20

/*
 *  Flags for fast comparison of namespaces
 */
#define EJS_NSP_PRIVATE         0x1
#define EJS_NSP_PROTECTED       0x2

/*
 *  Return codes for EjsFastLookupProperty
 */
#define EJS_OBJ_NOT_FOUND       -1              /* No matching property found */
#define EJS_OBJ_MULTIPLE        -2              /* Multiple properties of the same name */

/*
 *  When allocating slots, name hashes and traits, we optimize by rounding up allocations
 */
#define EJS_PROP_ROUNDUP(x) (((x) + EJS_NUM_PROP - 1) / EJS_NUM_PROP * EJS_NUM_PROP)

/*
 *  Property enumeration flags
 */
#define EJS_FLAGS_ENUM_INHERITED 0x1            /**< Enumerate inherited base classes */
#define EJS_FLAGS_ENUM_ALL      0x2             /**< Enumerate non-enumerable and fixture properties */


#if FUTURE
    #define EJS_FLAGS_ENUM_DATA 0x0             /** Enumerate data properties */
    #define EJS_FLAGS_ENUM_ALL  0x3             /** Enumerate all properties */

    /*
     *  Enumeration flags
     */
    #define EJS_ENUM_DATA       EJS_FLAGS_ENUM_DATA
    #define EJS_ENUM_INHERITED  EJS_FLAGS_ENUM_INHERITED
    #define EJS_ENUM_HIDDEN     EJS_FLAGS_ENUM_HIDDEN
    #define EJS_ENUM_ALL        EJS_FLAGS_ENUM_ALL
#endif

/*
 *  Exception flags and structure
 */
#define EJS_EX_CATCH            0x1             /* Definition is a catch block */
#define EJS_EX_FINALLY          0x2             /* Definition is a finally block */
#define EJS_EX_ITERATION        0x4             /* Definition is an iteration catch block */
#define EJS_EX_INC              4               /* Growth increment for exception handlers */

/*
 *  Ejscript return codes.
 */
#define EJS_SUCCESS             MPR_ERR_OK
#define EJS_ERR                 MPR_ERR_GENERAL
#define EJS_EXCEPTION           (MPR_ERR_MAX - 1)

/*
 *  Xml defines
 */
#define E4X_MAX_ELT_SIZE        (E4X_BUF_MAX-1)
#define E4X_TEXT_PROPERTY       "-txt"
#define E4X_TAG_NAME_PROPERTY   "-tag"
#define E4X_COMMENT_PROPERTY    "-com"
#define E4X_ATTRIBUTES_PROPERTY "-att"
#define E4X_PI_PROPERTY         "-pi"
#define E4X_PARENT_PROPERTY     "-parent"

#define EJS_XML_FLAGS_TEXT      0x1             /* Node is a text node */
#define EJS_XML_FLAGS_PI        0x2             /* Node is a processing instruction */
#define EJS_XML_FLAGS_COMMENT   0x4             /* Node is a comment */
#define EJS_XML_FLAGS_ATTRIBUTE 0x8             /* Node is an attribute */
#define EJS_XML_FLAGS_ELEMENT   0x10            /* Node is an element */

/*
 *  XML node kinds
 */
#define EJS_XML_LIST        1
#define EJS_XML_ELEMENT     2
#define EJS_XML_ATTRIBUTE   3
#define EJS_XML_TEXT        4
#define EJS_XML_COMMENT     5
#define EJS_XML_PROCESSING  6


/*
 *  Convenient slot aliases
 */
#define EJSLOT_CONSTRUCTOR      EJSLOT_Object___constructor__

/*
 *  Default names
 */
#define EJS_GLOBAL                  "global"
#define EJS_DEFAULT_MODULE          "default"
#define EJS_DEFAULT_MODULE_NAME     EJS_DEFAULT_MODULE EJS_MODULE_EXT
#define EJS_BUILTIN_MODULE_NAME     "ejs"  EJS_MODULE_EXT
#define EJS_DEFAULT_CLASS_NAME      "__defaultClass__"
#define EJS_INITIALIZER_NAME        "__initializer__"

#if BLD_APPWEB_PRODUCT
#define EJS_NAME                    "ajs"
#define EJS_MOD                     "ajs.mod"
#else
#define EJS_NAME                    "ejs"
#define EJS_MOD                     "ejs.mod"
#endif

/*
 *  Compare if a variable is an instance of a given type as described by the type's global slot.
 */
#define ejsIs(vp, slot)         (vp && (((EjsVar*) (vp))->type->id == slot))

/* TODO - merge with MprList */
/**
 *  List type
 *  @description    The MprList is a dynamic growable array suitable for storing pointers to arbitrary objects.
 *  @stability      Prototype.
 *  @see            EjsList mprCreateList mprFree MprBuf
 */
typedef struct EjsList {
    void    **items;                    /* List item data */
    int     length;                     /* Count of used items */
    int     maxSize;                    /* Maximum capacity */
} EjsList;

//  TODO - FUTURE document these
extern void     ejsInitList(EjsList *list);
extern int      ejsAddItem(MprCtx ctx, EjsList *list, cvoid *item);
extern int      ejsAddItemToSharedList(MprCtx ctx, EjsList *list, cvoid *item);
extern EjsList  *ejsAppendList(MprCtx ctx, EjsList *list, EjsList *add);
extern int      ejsCopyList(MprCtx ctx, EjsList *dest, EjsList *src);
extern void     ejsClearList(EjsList *lp);
extern void     *ejsGetItem(EjsList *lp, int index);
extern void     *ejsGetLastItem(EjsList *lp);
extern int      ejsGetListCount(EjsList *lp);
extern void     *ejsGetNextItem(EjsList *lp, int *lastIndex);
extern void     *ejsGetPrevItem(EjsList *lp, int *lastIndex);
extern int      ejsRemoveItemAtPos(EjsList *lp, int index);
extern int      ejsRemoveLastItem(EjsList *lp);
extern int      ejsSetListDetails(MprCtx ctx, EjsList *list, int initialSize, int maxSize);


//  TODO - could to have a shorter name than EjsNativeFunction - EjsProc
/**
 *  Native Function signature
 *  @description This is the calling signature for C Functions.
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param obj Reference to the "this" object. (The object containing the method).
 *  @param argc Number of arguments.
 *  @param argv Array of arguments.
 *  @returns Returns a result variable or NULL on errors and exceptions.
 *  @stability Prototype.
 */
typedef struct EjsVar *(*EjsNativeFunction)(Ejs *ejs, struct EjsVar *vp, int argc, struct EjsVar **argv);

/*
 *  Callback for loading native modules
 */
typedef int (*EjsNativeCallback)(Ejs *ejs, struct EjsModule *mp, cchar *path);
typedef int (*EjsSortFn)(Ejs *ejs, struct EjsVar *p1, struct EjsVar *p2, cchar *name, int order);

/******************************************* Ejscript Classes *******************************************/
/**
 *  Foundation Variable Interface
 *  @description The EjsVar variable interface forms the most basic contract between Ejscript variables and
 *      the virtual machine. It defines the essential attributes, base type and state for managing a variable.
 *      Ejscript variables can have properties and methods and participate as full objects in Ejscript programs. 
 *      All Ejscript data types implement the EjsVar interface either directly or indirectly. Primitive types
 *      will implement EjsVar by including it as the first field in their structure. They often don't need 
 *      a property hash and by implementing EjsVar directly, they can save memory space. Class that do
 *      require property hashes will typically implement this interface by composing EjsObject which is a 
 *      concrete implementation of EjsVar.
 *  @stability Evolving.
 *  @defgroup EjsVar EjsVar
 *  @see EjsVar ejsGetVarType ejsAllocVar ejsFreeVar ejsCastVar ejsCloneVar ejsCreateInstance ejsCreateVar
 *      ejsDestroyVar ejsDefineProperty ejsDeleteProperty ejsDeletePropertyByName ejsFinalizeVar
 *      ejsGetProperty ejsLookupProperty ejsMarkVar ejsSetProperty ejsSetPropertyByName ejsSetPropertyName
 *      ejsSetPropertyTrait ejsSerialize ejsDeserialize ejsParseVar
 */
typedef struct EjsVar {
#if BLD_DEBUG
    char debugName[EJS_MAX_DEBUG_NAME];         /**< Common name for this variable */
#endif

    struct EjsType  *type;                      /**< Type of this object (not base type). ie. type for Object is EjsType  */
    struct EjsVar   *next;                      /**< GC linkage */

    uint    generation        :  2;             /**< GC generation for this var */
    uint    rootLinks         :  3;             /**< Cross generational root mask */
    uint    refLinks          :  3;             /**< Cross generational reference mask */

    uint    builtin           :  1;             /**< Property is always present */
    uint    dynamic           :  1;             /**< Object may add properties. Derrived from type */
    uint    hasGetterSetter   :  1;             /**< Class has getter/setter functions */
    uint    isFunction        :  1;             /**< Instance is a function */

    uint    isObject          :  1;             /**< Instance is an Object */
    uint    isInstanceBlock   :  1;             /**< Object is a type instance block object */
    uint    isType            :  1;             /**< Instance is a type object (TODO Could do without this bit) */
    uint    isFrame           :  1;             /**< Is a frame (stack-based var) */

    uint    hidden            :  1;             /**< Not enumerable via for/in */
    uint    marked            :  1;             /**< GC marked in use */
    uint    native            :  1;             /**< Native property backed by C/Java implementation */

    /* TODO move to EjsFunction */
    uint    nativeProc        :  1;             /**< Method has native proc */

    uint    permanent         :  1;             /**< Object is immune from GC */
    uint    survived          :  1;             /**< Object has survived one GC pass */
    uint    visited           :  1;             /**< Has been traversed */
    uint    reserved          :  1;             /**< Future expansion */

#if BLD_DEBUG
    int             magic;                      /**< Magic signature for GC */
    int             seqno;                      /**< Unique sequence number. Helpful for GC */
#endif

} EjsVar;

#if DOXYGEN
    /**
     *  Get a variables type
     *  @description Get the base type for a variable
     *  @param vp Variable reference
     *  @returns A reference to the variables type object
     *  @ingroup EjsVar
     */
    extern EjsType *ejsGetVarType(EjsVar *vp);
#else
    #define ejsGetVarType(vp)       (vp->type)
    #if BLD_DEBUG
    #define ejsSetDebugName(vp, name) mprStrcpy(((EjsVar*) (vp))->debugName, sizeof(((EjsVar*) (vp))->debugName), name)
    #define ejsSetFmtDebugName(vp, fmt, arg) mprSprintf(((EjsVar*) (vp))->debugName, \
            sizeof(((EjsVar*) (vp))->debugName), fmt, arg);
    #else
    #define ejsSetDebugName(vp, name)
    #define ejsSetFmtDebugName(vp, fmt, arg)
    #endif
#endif

/**
 *  Allocate a new variable
 *  @description This will allocate space for a bare variable. This routine should only be called by type factories
 *      when implementing the createVar helper.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param type Type object from which to create an object instance
 *  @param size Size of extra property slots to reserve. This is used for dynamic objects.
 *  @return A newly allocated variable of the requested type. Caller must not free as the GC will manage the lifecycle
 *      of the variable.
 *  @ingroup EjsVar
 */
extern EjsVar *ejsAllocVar(Ejs *ejs, struct EjsType *type, int size);

/**
 *  Free a new variable
 *  @description This should typically only be called by the destroyVar type helper which is invoked by the GC when
 *      a variable is no longer needed. It should not be called by normal code.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param vp Variable to free
 *  @ingroup EjsVar
 */
extern void ejsFreeVar(Ejs *ejs, EjsVar *vp);

/**
 *  Cast a variable to a new type
 *  @description Cast a variable and return a new variable of the required type.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param vp Variable to cast
 *  @param type Type to cast to
 *  @return A newly allocated variable of the requested type. Caller must not free as the GC will manage the lifecycle
 *      of the variable.
 *  @ingroup EjsVar
 */
extern EjsVar *ejsCastVar(Ejs *ejs, EjsVar *vp, struct EjsType *type);

/**
 *  Clone a variable
 *  @description Copy a variable and create a new copy. This may do a shallow or deep copy. A shallow copy
 *      will not copy the property instances, rather it will only duplicate the property reference. A deep copy
 *      will recursively clone all the properties of the variable.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param vp Variable to clone
 *  @param deep Set to true to do a deep copy.
 *  @return A newly allocated variable of the requested type. Caller must not free as the GC will manage the lifecycle
 *      of the variable.
 *  @ingroup EjsVar
 */
extern EjsVar *ejsCloneVar(Ejs *ejs, EjsVar *vp, bool deep);

/**
 *  Create a new variable instance 
 *  @description Create a new variable instance and invoke any required constructors with the given arguments.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param type Type from which to create a new instance
 *  @param argc Count of args in argv
 *  @param argv Vector of arguments. Each arg is an EjsVar.
 *  @return A newly allocated variable of the requested type. Caller must not free as the GC will manage the lifecycle
 *      of the variable.
 *  @ingroup EjsVar
 */
extern EjsVar *ejsCreateInstance(Ejs *ejs, struct EjsType *type, int argc, EjsVar **argv);

/**
 *  Create a variable
 *  @description Create a variable of the required type. This invokes the createVar helper method for the specified type.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param type Type to cast to
 *  @param numSlots Size of extra property slots to reserve. This is used for dynamic objects.
 *  @return A newly allocated variable of the requested type. Caller must not free as the GC will manage the lifecycle
 *      of the variable.
 *  @ingroup EjsVar
 */
extern EjsVar *ejsCreateVar(Ejs *ejs, struct EjsType *type, int numSlots);

/**
 *  Destroy a variable
 *  @description Destroy a variable of the required type. This invokes the destroyVar helper method for the specified type.
 *      The default action for the destroyVar helper is to simply invoke ejsFreeVar which will return the variable
 *      storage to a type pool or return the memory to the heap.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param vp Varaible to destroy
 *  @ingroup EjsVar
 */
extern void ejsDestroyVar(Ejs *ejs, EjsVar *vp);

/**
 *  Define a property
 *  @description Define a property in a variable and give it a name, base type, attributes and default value.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param vp Variable in which to define a property
 *  @param slotNum Slot number in the variable for the property. Slots are numbered sequentially from zero. Set to
 *      -1 to request the next available slot number.
 *  @param qname Qualified name containing a name and a namespace.
 *  @param type Base type of the property. Set to ejs->voidType to leave as untyped.
 *  @param attributes Attribute traits. Useful attributes include:
 *      @li EJS_ATTR_OVERRIDE
 *      @li EJS_ATTR_CONST
 *      @li EJS_ATTR_ENUMERABLE
 *  @param value Initial value of the property
 *  @return A postitive slot number or a negative MPR error code.
 *  @ingroup EjsVar
 */
extern int ejsDefineProperty(Ejs *ejs, EjsVar *vp, int slotNum, EjsName *qname, struct EjsType *type, 
    int attributes, EjsVar *value);

/**
 *  Delete a property
 *  @description Delete a variable's property and set its slot to null. The slot is not reclaimed and subsequent properties
 *      are not compacted.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param vp Variable in which to delete the property
 *  @param slotNum Slot number in the variable for the property to delete.
 *  @return Zero if successful, otherwise a negative MPR error code.
 *  @ingroup EjsVar
 */
extern int ejsDeleteProperty(Ejs *ejs, EjsVar *vp, int slotNum);

/**
 *  Delete a property by name
 *  @description Delete a variable's property by name and set its slot to null. The property is resolved by using 
 *      ejsLookupProperty with the specified name. Once deleted, the slot is not reclaimed and subsequent properties
 *      are not compacted.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param vp Variable in which to delete the property
 *  @param qname Qualified name for the property including name and namespace.
 *  @return Zero if successful, otherwise a negative MPR error code.
 *  @ingroup EjsVar
 */
extern int ejsDeletePropertyByName(Ejs *ejs, EjsVar *vp, EjsName *qname);

/**
 *  Finalize a variable before destroying
 *  @description Finalize a variable. Variables can have finalization routines to release any acquired resources
 *      before being destroyed.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param vp Variable to cast
 *  @ingroup EjsVar
 */
extern void ejsFinalizeVar(Ejs *ejs, EjsVar *vp);

/**
 *  Get a property
 *  @description Get a property from a variable at a given slot.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param vp Variable to examine
 *  @param slotNum Slot number for the requested property.
 *  @return The variable property stored at the nominated slot.
 *  @ingroup EjsVar
 */
extern EjsVar *ejsGetProperty(Ejs *ejs, EjsVar *vp, int slotNum);

/**
 *  Get a count of properties in a variable
 *  @description Get a property from a variable at a given slot.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param vp Variable to examine
 *  @return A positive integer count of the properties stored by the variable. 
 *  @ingroup EjsVar
 */
extern int ejsGetPropertyCount(Ejs *ejs, EjsVar *vp);

/**
 *  Get a variable property's name
 *  @description Get a property name for the property at a given slot in the  variable.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param vp Variable to examine
 *  @param slotNum Slot number for the requested property.
 *  @return The qualified property name including namespace and name. Caller must not free.
 *  @ingroup EjsVar
 */
extern EjsName ejsGetPropertyName(Ejs *ejs, EjsVar *vp, int slotNum);

/**
 *  Get a property by name
 *  @description Get a property from a variable by name.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param vp Variable to examine
 *  @param qname Qualified name specifying both a namespace and name.
 *  @return The variable property stored at the nominated slot.
 *  @ingroup EjsVar
 */
extern EjsVar *ejsGetPropertyByName(Ejs *ejs, EjsVar *vp, EjsName *qname);

/**
 *  Get a property's traits
 *  @description Get a property's trait description. The property traits define the properties base type,
 *      and access attributes.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param vp Variable to examine
 *  @param slotNum Slot number for the requested property.
 *  @return A trait structure reference for the property.
 *  @ingroup EjsVar
 */
extern struct EjsTrait *ejsGetPropertyTrait(Ejs *ejs, EjsVar *vp, int slotNum);

/**
 *  Invoke an opcode on a native type.
 *  @description Invoke an Ejscript byte code operator on the specified variable given the expression right hand side.
 *      Native types would normally implement the invokeOperator helper function to respond to this function call.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param vp Variable to examine
 *  @param opCode Byte ope code to execute
 *  @param rhs Expression right hand side for binary expression op codes. May be null for other op codes.
 *  @return The result of the op code or NULL if the opcode does not require a result.
 *  @ingroup EjsVar
 */
extern EjsVar *ejsInvokeOperator(Ejs *ejs, EjsVar *vp, int opCode, EjsVar *rhs);

/**
 *  Lookup a property by name
 *  @description Search for a property by name in the given variable.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param vp Variable to examine
 *  @param qname Qualified name of the property to search for.
 *  @return The slot number containing the property. Then use #ejsGetProperty to retrieve the property or alternatively
 *      use ejsGetPropertyByName to lookup and retrieve in one step.
 *  @ingroup EjsVar
 */
extern int ejsLookupProperty(Ejs *ejs, EjsVar *vp, EjsName *qname);

/**
 *  Mark a variable as currently in use.
 *  @description Mark a variables as currently active so the garbage collector will preserve it. This routine should
 *      be called by native types in their markVar helper.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param parent Owning variable for the property
 *  @param vp Variable to mark as currently being used.
 *  @ingroup EjsVar
 */
extern void ejsMarkVar(Ejs *ejs, EjsVar *parent, EjsVar *vp);

/**
 *  Set a property's value
 *  @description Set a value for a property at a given slot in the specified variable.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param vp Variable to examine
 *  @param slotNum Slot number for the requested property.
 *  @param value Reference to a value to store.
 *  @return The slot number of the property updated.
 *  @ingroup EjsVar
 */
extern int ejsSetProperty(Ejs *ejs, EjsVar *vp, int slotNum, EjsVar *value);

/**
 *  Set a property's value 
 *  @description Set a value for a property. The property is located by name in the specified variable.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param vp Variable to examine
 *  @param qname Qualified property name.
 *  @param value Reference to a value to store.
 *  @return The slot number of the property updated.
 *  @ingroup EjsVar
 */
extern int ejsSetPropertyByName(Ejs *ejs, EjsVar *vp, EjsName *qname, EjsVar *value);

/**
 *  Set a property's name 
 *  @description Set a qualified name for a property at the specified slot in the variable. The qualified name
 *      consists of a namespace and name - both of which must be persistent. A typical paradigm is for these name
 *      strings to be owned by the memory context of the variable.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param vp Variable to examine
 *  @param slotNum Slot number of the property in the variable.
 *  @param qname Qualified property name.
 *  @return The slot number of the property updated.
 *  @ingroup EjsVar
 */
extern int ejsSetPropertyName(Ejs *ejs, EjsVar *vp, int slotNum, EjsName *qname);

/**
 *  Set a property's traits
 *  @description Set the traits describing a property. These include the property's base type and access attributes.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param vp Variable to examine
 *  @param slotNum Slot number of the property in the variable.
 *  @param type Base type for the property. Set to NULL for an untyped property.
 *  @param attributes Integer mask of access attributes.
 *  @return The slot number of the property updated.
 *  @ingroup EjsVar
 */
extern int ejsSetPropertyTrait(Ejs *ejs, EjsVar *vp, int slotNum, struct EjsType *type, int attributes);

/**
 *  Serialize a variable
 *  @description Serialize a variable into a string representation
 *  @ingroup EjsVar
 */
extern EjsVar  *ejsSerialize(Ejs *ejs, EjsVar *value, int maxDepth, bool showAll, bool showBase);
extern EjsVar  *ejsDeserialize(Ejs *ejs, EjsVar *value);
extern EjsVar *ejsParseVar(Ejs *ejs, cchar *str,  int prefType);
extern void ejsZeroSlots(Ejs *ejs, EjsVar **slots, int count);

/**
 *  Hash entry for a property. 
 *  @description Properties are indexed by hash entries. These store the property name and a reference to the 
 *      next slot in the hash collision chain.
 *  @ingroup EjsObject
 */
typedef struct EjsHashEntry {
    EjsName         qname;                  /**< Property name */
    int             nextSlot;               /**< Next property in hash chain */
} EjsHashEntry;


/**
 *  Property Names
 *  @description This structure stores the names of all the properties in an object and holds the hash table state.
 *  @ingroup EjsObject
 */
typedef struct EjsNames {
    EjsHashEntry    *entries;               /**< Hash entries */
    int             *buckets;               /**< Hash buckets and head of link chains */
    int             sizeBuckets;            /**< Size of buckets */
    int             sizeEntries;            /**< Size of entries array in elements */
} EjsNames;


/**
 *  Object Type. Base type for all objects.
 *  @description The EjsObject type is used as the foundation for types, blocks, functions and all scripted classes. 
 *      It implements the EjsVar interface and provides storage and hashed lookup for properties. It supports dynamic
 *      objects that can grow the number of properties they store.
 *      \n\n
 *      Note that native classes may or may not be based on EjsObject. Some native classes may implement the EjsVar 
 *      interface so they can optimally store their properties - often as native types themselves. Numbers are such a
 *      case. 
 *
 *      EjsObject stores properties in an array of slots. These slots store a reference to the property value. 
 *      Property names are stored in a names hash. Dynamic objects own their own name hash. Sealed object instances 
 *      of a type, will simply refer to the hash of names owned by the type.
 *
 *      EjsObjects may be either dynamic or sealed. Dynamic objects can grow the number of properties. Sealed 
 *      objects cannot. Sealed objects will store the slot array as part of the EjsObject memory chunk. Dynamic 
 *      objects will perform a separate allocation for the slot array so that it can grow.
 *  @stability Evolving.
 *  @defgroup EjsObject EjsObject
 *  @see EjsObject ejsIsObject ejsCreateSimpleObject ejsCreateObject ejsCreateObjectEx ejsCopyObject
 *      ejsGrowObject ejsMarkObject
 */
typedef struct EjsObject {
    EjsVar          var;                    /**< Implement EjsVar interface */
    EjsVar          **slots;                /**< Vector of slots containing var references */
    EjsNames        *names;                 /**< Hash table of property names */
    int             numProp;                /**< Number of properties */
    int             capacity;               /**< Current capacity for properties */
} EjsObject;

//  TODO - remove all these ejsIsXXX routines and only use ejsIs()
#if DOXYGEN
    /**
     *  Determine if a variable is an EjsObject.
     *  @description This call tests if the variable is based on EjsObject. Note that all variables are logically
     *      objects in that they implement the EjsVar interface. However, only those that are composed of EjsObject
     *      will return true for this call. Types, Blocks, Functions and all scripted classes are based on EjsObject
     *      and will test true.
     *  @param vp Variable to test
     *  @returns True if the variable is based on EjsObject
     *  @ingroup EjsObject
     */
    extern bool ejsIsObject(EjsVar *vp);
#else
    #define ejsIsObject(vp)         (vp && (((EjsVar*) (vp))->isObject))
#endif

/**
 *  Create a simple object
 *  @description Create a simple object using Object as its base type.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @return A new object instance
 *  @ingroup EjsObject
 */
extern EjsObject *ejsCreateSimpleObject(Ejs *ejs);

/**
 *  Create an object instance of the specified type
 *  @description Create a new object using the specified type as a base class. 
 *      Note: the constructor is not called. If you require the constructor to be invoked, use #ejsCreateInstance
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param type Base type to use when creating the object instance
 *  @param size Number of extra slots to allocate when creating the object
 *  @return A new object instance
 *  @ingroup EjsObject
 */
extern EjsObject *ejsCreateObject(Ejs *ejs, struct EjsType *type, int size);

/**
 *  Create an object instance of the specified type
 *  @description Create a new object using the specified type as a base class. 
 *      Note: the constructor is not called. If you require the constructor to be invoked, use #ejsCreateInstance
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param type Base type to use when creating the object instance
 *  @param size Number of extra slots to allocate when creating the object
 *  @param dynamic Set to true to make the object as being dynamic. Dynamic objects can create new properties and grow
 *      at any time.
 *  @return A new object instance
 *  @ingroup EjsObject
 */
extern EjsObject *ejsCreateObjectEx(Ejs *ejs, struct EjsType *type, int size, bool dynamic);

//  TODO - inconsistent naming vs ejsCloneVar (clone vs copy)
//
/**
 *  Copy an object
 *  @description Copy an object create a new instance. This may do a shallow or deep copy depending on the value of 
 *      \a deep. A shallow copy will not copy the property instances, rather it will only duplicate the property 
 *      reference. A deep copy will recursively clone all the properties of the variable.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param src Source object to copy
 *  @param deep Set to true to do a deep copy.
 *  @return A newly allocated object. Caller must not free as the GC will manage the lifecycle of the variable.
 *  @ingroup EjsVar
 */
extern EjsObject *ejsCopyObject(Ejs *ejs, EjsObject *src, bool deep);

/**
 *  Grow an object
 *  @description Grow the slot storage for an object. Object properties are stored in slots. To store more 
 *      properties, you need to grow the slots.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param obj Object reference to grow
 *  @param size New minimum count of properties. If size is less than the current number of properties, the call
 *      will be ignored, i.e. it will not shrink objects.
 *  @return A new object instance
 *  @ingroup EjsObject
 */
extern int      ejsGrowObject(Ejs *ejs, EjsObject *obj, int size);

/**
 *  Mark an object as currently in use.
 *  @description Mark an object as currently active so the garbage collector will preserve it. This routine should
 *      be called by native types that extend EjsObject in their markVar helper.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param parent Owning variable for the property
 *  @param obj Object to mark as currently being used.
 *  @ingroup EjsObject
 */
extern void     ejsMarkObject(Ejs *ejs, EjsVar *parent, EjsObject *obj);

extern int      ejsCheckObjSlot(Ejs *ejs, EjsObject *obj, int slotNum);
extern EjsVar   *ejsCoerceOperands(Ejs *ejs, EjsVar *lhs, int opcode, EjsVar *rhs);
extern int      ejsComputeHashCode(struct EjsNames *hash, EjsName *qname);
extern int      ejsGetHashSize(int numProp);
extern void     ejsInitializeObjectHelpers(struct EjsTypeHelpers *helpers);
extern int      ejsInsertGrowObject(Ejs *ejs, EjsObject *obj, int size, int offset);
extern int      ejsLookupSingleProperty(Ejs *ejs, EjsObject *obj, EjsName *qname);
extern void     ejsMakePropertyDontDelete(EjsVar *vp, int dontDelete);
extern int      ejsMakePropertyEnumerable(EjsVar *vp, bool enumerable);
extern void     ejsMakePropertyReadOnly(EjsVar *vp, int readonly);
extern EjsVar   *ejsObjectOperator(Ejs *ejs, EjsVar *lhs, int opcode, EjsVar *rhs);
extern int      ejsRebuildHash(Ejs *ejs, EjsObject *obj);
extern void     ejsRemoveSlot(Ejs *ejs, EjsObject *slots, int slotNum, int compact);
extern void     ejsSerializeHelper(Ejs *ejs, int argc, EjsVar **argv, int *maxDepth, int *flags);
extern void     ejsSetAllocIncrement(Ejs *ejs, struct EjsType *type, int increment);
extern EjsVar   *ejsToSource(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv);


/**
 *  Property traits. 
 *  @description Property traits describe the type and access attributes of a property. The Trait structure
 *      is used by EjsBlock to describe the attributes of properties defined within a block.
 *      Note: These traits apply to a property definition and not to the referenced object. ie. two property 
 *      definitions may have different traits but will refer to the same object.
 *  @stability Evolving
 *  @ingroup EjsBlock
 */
typedef struct EjsTrait {
    struct EjsType  *type;                          /**< Property type */
    int             attributes;                     /**< Property attributes */
} EjsTrait;


/**
 *  Block class
 *  @description The block class is the base class for all program block scope objects. This is an internal class
 *      and not exposed to the script programmer.
 *  Blocks (including types) may describe their properties via traits. The traits store the property 
 *  type and access attributes and are stored in EjsBlock which is a sub class of EjsObject. See ejsBlock.c for details.
 *  @stability Evolving
 *  @defgroup EjsBlock EjsBlock
 *  @see EjsBlock ejsIsBlock ejsBindFunction
 */
typedef struct EjsBlock {
    EjsObject       obj;                            /**< Extends Object - Property storage */
    EjsList         namespaces;                     /**< Current list of namespaces open in this block of properties */
    struct EjsBlock *scopeChain;                    /**< Lexical scope chain for this block */
    cchar           *name;                          /**< Block name */
    EjsTrait        *traits;                        /**< Actual property traits */
    int             numTraits: 16;                  /**< Number of allocated traits */
    int             sizeTraits: 16;                 /**< Size of traits */
    int             numInherited: 16;               /**< Number of inherited traits */
    uint            hasScriptFunctions: 1;          /**< Block has non-native functions requiring namespaces */
    uint            reserved: 15;
} EjsBlock;


#if DOXYGEN
    /**
     *  Determine if a variable is a block.
     *  @description This call tests if the variable is a block.
     *  @param vp Variable to test
     *  @returns True if the variable is based on EjsBlock
     *  @ingroup EjsBlock
     */
    extern bool ejsIsBlock(EjsVar *vp);
#else
    #define ejsIsBlock(vp) (ejsIs(vp, ES_Block) || ejsIs(vp, ES_Function) || ejsIs(vp, ES_Type))
#endif

/**
 *  Bind a native C function to a function property
 *  @description Bind a native C function to an existing javascript function. Functions are typically created
 *      by compiling a script file of native function definitions into a mod file. When loaded, this mod file 
 *      will create the function properties. This routine will then bind the specified C function to the 
 *      function property.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param block Block containing the function property to bind.
 *  @param slotNum Slot number of the method property
 *  @param fn Native C function to bind
 *  @return Zero if successful, otherwise a negative MPR error code.
 *  @ingroup EjsType
 */
extern int ejsBindFunction(Ejs *ejs, EjsBlock *block, int slotNum, EjsNativeFunction fn);

extern void ejsSetTraitType(struct EjsTrait *trait, struct EjsType *type);
extern void ejsSetTraitAttributes(struct EjsTrait *trait, int attributes);

/*
 *  This is all an internal API. Native types should probably not be using these routines. Speak up if you find
 *  you need these routines in your code.
 */
extern int      ejsAddNamespaceToBlock(Ejs *ejs, EjsBlock *blockRef, struct EjsNamespace *namespace);
extern int      ejsAddScope(MprCtx ctx, EjsBlock *block, EjsBlock *scopeBlock);
extern EjsBlock *ejsCreateBlock(Ejs *ejs, cchar *name, int numSlots);
//  TODO - why do we have ejsCopyObject, ejsCopyBlock ... Surely ejsCloneVar is sufficient?
extern EjsBlock *ejsCopyBlock(Ejs *ejs, EjsBlock *src, bool deep);
//  TODO - this should be pushed into the helpers as ejsGrowVar. Then we need only one variant.
extern int      ejsCaptureScope(Ejs *ejs, EjsBlock *block, EjsList *scopeChain);
extern int      ejsCopyScope(EjsBlock *block, EjsList *chain);
extern int      ejsGrowBlock(Ejs *ejs, EjsBlock *block, int numSlots);
extern int      ejsGetNamespaceCount(EjsBlock *block);
extern int      ejsGetNumTraits(EjsBlock *block);
extern int      ejsGetNumInheritedTraits(EjsBlock *block);
extern int      ejsGetSizeTraits(EjsBlock *block);
extern EjsBlock *ejsGetTopScope(EjsBlock *block);
extern EjsTrait *ejsGetTrait(EjsBlock *block, int slotNum);
extern int      ejsGetTraitAttributes(EjsBlock *block, int slotNum);
extern struct EjsType *ejsGetTraitType(EjsBlock *block, int slotNum);
extern int      ejsInheritTraits(Ejs *ejs, EjsBlock *block, EjsBlock *baseBlock, int count, int offset, bool implementing);
extern int      ejsInsertGrowBlock(Ejs *ejs, EjsBlock *block, int numSlots, int offset);
extern void     ejsMarkBlock(Ejs *ejs, EjsVar *parent, EjsBlock *block);
extern void     ejsPopBlockNamespaces(EjsBlock *block, int count);
extern int      ejsRemoveProperty(Ejs *ejs, EjsBlock *block, int slotNum);
extern EjsBlock *ejsRemoveScope(EjsBlock *block);
extern void     ejsResetBlockNamespaces(Ejs *ejs, EjsBlock *block);
extern void     ejsSetNumInheritedTraits(EjsBlock *block, int numInheritedTraits);
extern int      ejsSetTrait(EjsBlock *block, int slotNum, struct EjsType *type, int attributes);
extern void     ejsSetTraitName(EjsBlock *block, int slotNum, cchar *name);

/**
 *  Type class
 *  @description Classes in Ejscript are represented by instances of an EjsType. 
 *      Types are templates for creating instances of the given type, but they are also are runtime accessible objects.
 *      Types contain the static properties and methods for objects and store these in their object slots array. 
 *      They store the instance properties in the type->instance object. EjsType inherits from EjsBlock, EjsObject 
 *      and EjsVar. 
 *  @stability Evolving
 *  @defgroup EjsType EjsType
 *  @see EjsType ejsIsType ejsIsInstanceBlock ejsCreateType ejsDefineFunction ejsIsA ejsIsTypeSubType 
 *      ejsBindMethod ejsDefineInstanceProperty ejsGetType
 */
typedef struct EjsType {
    EjsBlock        block;                          /**< Type properties (functions and static properties) */
    EjsBlock        *instanceBlock;                 /**< Instance properties template */
    EjsName         qname;                          /**< Qualified name of the type. Type name and namespace */
    struct EjsType  *baseType;                      /**< Base class */
    MprList         *implements;                    /**< List of implemented interfaces */
        
    uint            subTypeCount            :  8;   /**< Length of baseType chain Governed by EJS_MAX_BASE_CLASS */
    uint            numAlloc                :  4;   /**< Allocation chunking increment */
    uint            callsSuper              :  1;   /**< Constructor calls super() */
    uint            dynamicInstance         :  1;   /**< Instances may add properties */
    uint            separateInstanceSlots   :  1;   /**< Instances slots allocated separately to object */
    uint            final                   :  1;   /**< Type is final */
    uint            fixupDone               :  1;   /**< Slot fixup performed */
    uint            hasBaseConstructors     :  1;   /**< Base types has constructors */
    uint            hasBaseInitializers     :  1;   /**< Base types have initializers */
    uint            hasBaseStaticInitializers: 1;   /**< Base types have initializers */
    uint            hasConstructor          :  1;   /**< Type has a constructor */
    uint            hasFinalizer            :  1;   /**< Instances need finalization */
    uint            hasInitializer          :  1;   /**< Type has instance level initialization code */
    uint            hasObject               :  1;   /**< Type based on EjsObject */
    uint            hasStaticInitializer    :  1;   /**< Type has static level initialization code */
    uint            initialized             :  1;   /**< Static initializer has run */
    uint            isInterface             :  1;   /**< Interface vs class */
    uint            operatorOverload        :  1;   /**< Using overloaded operators - TODO not used */
    uint            needFixup               :  1;   /**< Slots need fixup */
    uint            nobind                  :  1;   /**< Don't bind properties for this type to slots */
    uint            numericIndicies         :  1;   /**< Instances support direct numeric indicies */
    uint            skipScope               :  1;   /**< Skip examining this object when searching the scope chain */
    
    short           id;                             /**< Unique type id */
    ushort          instanceSize;                   /**< Size of instances in bytes */
    struct EjsTypeHelpers *helpers;                 /**< Type helper methods */
    struct EjsModule *module;                       /**< Module owning the type - stores the constant pool */
    void            *typeData;                      /**< Type specific data */
} EjsType;


#if DOXYGEN
    /**
     *  Determine if a variable is an type
     *  @param vp Variable to test
     *  @return True if the variable is a type
     *  @ingroup EjsType
     */
    extern bool ejsIsType(EjsVar *vp);

    /**
     *  Determine if a variable is an instance block. Types store the template for instance properties in an instance
     *      block object.
     *  @param vp Variable to test
     *  @return True if the variable is an instance block object
     *  @ingroup EjsType
     */
    extern bool ejsIsInstanceBlock(EjsVar *vp);
#else
    #define ejsIsType(vp)           (vp && (((EjsVar*) (vp))->isType))
    #define ejsIsInstanceBlock(vp)  (vp && (((EjsVar*) (vp))->isInstanceBlock))
#endif


/**
 *  Create a new type object
 *  @description Create a new type object 
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param name Qualified name to give the type. This name is merely referenced by the type and must be persistent.
 *      This name is not used to define the type as a global property.
 *  @param up Reference to a module that will own the type. Set to null if not owned by any module.
 *  @param baseType Base type for this type.
 *  @param size Size of instances. This is the size in bytes of an instance object.
 *  @param slotNum Slot number that the type will be installed at. This is used by core types to define a unique type ID. 
 *      For non-core types, set to -1.
 *  @param numTypeProp Number of type (class) properties for the type. These include static properties and methods.
 *  @param numInstanceProp Number of instance properties.
 *  @param attributes Attribute mask to modify how the type is initialized. Valid values include:
 *      @li EJS_ATTR_BLOCK_HELPERS - Type uses EjsBlock helpers
 *      @li EJS_ATTR_CALLS_SUPER - Type calls super()
 *      @li EJS_ATTR_DYNAMIC_INSTANCE - Instance objects are dynamic
 *      @li EJS_ATTR_FINAL - Type will be a final class
 *      @li EJS_ATTR_INTERFACE - Type is an interface
 *      @li EJS_ATTR_HAS_CONSTRUCTOR - Type has a constructor to call
 *      @li EJS_ATTR_HAS_INITIALIZER - Type has an initializer
 *      @li EJS_ATTR_HAS_STATIC_INITIALIZER - Type has a static initializer
 *      @li EJS_ATTR_NO_BIND - Instruct the compiler to never bind any property references to slots
 *      @li EJS_ATTR_OBJECT - Type instances are based on EjsObject
 *      @li EJS_ATTR_OPER_OVERLOAD - Type uses operator overload
 *      @li EJS_ATTR_OBJECT_HELPERS - Type uses EjsObject helpers
 *      @li EJS_ATTR_SLOT_NEEDS_FIXUP - Slots will need fixup. Typically because the base type is unknown
 *  @param data
 *  @ingroup EjsType EjsType
 */
extern EjsType *ejsCreateType(Ejs *ejs, EjsName *name, struct EjsModule *up, EjsType *baseType, int size, 
    int slotNum, int numTypeProp, int numInstanceProp, int attributes, void *data);

/**
 *  Define a global function
 *  @description Define a global public function and bind it to the C native function. This is a simple one liner
 *      to define a public global function. The more typical paradigm to define functions is to create a script file
 *      of native method definitions and and compile it. This results in a mod file that can be loaded which will
 *      create the function/method definitions. Then use #ejsBindMethod to associate a C function with a property.
 *  @ingroup EjsType
 */
extern int ejsDefineGlobalFunction(Ejs *ejs, cchar *name, EjsNativeFunction fn);


/**
 *  Test if an variable is an instance of a given type
 *  @description Perform an "is a" test. This tests if a variable is a direct instance or subclass of a given base type.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param target Target variable to test.
 *  @param type Type to compare with the target
 *  @return True if target is an instance of "type" or an instance of a subclass of "type".
 *  @ingroup EjsType
 */
extern bool ejsIsA(Ejs *ejs, EjsVar *target, EjsType *type);

/**
 *  Test if a type is a derived type of a given base type.
 *  @description Test if a type subclasses a base type.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param target Target type to test.
 *  @param baseType Base class to see if the target subclasses it.
 *  @return True if target is a "baseType" or a subclass of "baseType".
 *  @ingroup EjsType
 */
extern bool ejsIsTypeSubType(Ejs *ejs, EjsType *target, EjsType *baseType);

/**
 *  Bind a native C function to a method property
 *  @description Bind a native C function to an existing javascript method. Method functions are typically created
 *      by compiling a script file of native method definitions into a mod file. When loaded, this mod file will create
 *      the method properties. This routine will then bind the specified C function to the method property.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param type Type containing the function property to bind.
 *  @param slotNum Slot number of the method property
 *  @param fn Native C function to bind
 *  @return Zero if successful, otherwise a negative MPR error code.
 *  @ingroup EjsType
 */
extern int ejsBindMethod(Ejs *ejs, EjsType *type, int slotNum, EjsNativeFunction fn);

/**
 *  Define an instance property
 *  @description Define an instance property on a type. This routine should not normally be called manually. Instance
 *      properties are best created by creating a script file of native property definitions and then loading the resultant
 *      mod file.
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param type Type in which to create the instance property
 *  @param slotNum Instance slot number in the type that will hold the property. Set to -1 to allocate the next available
 *      free slot.
 *  @param name Qualified name for the property including namespace and name.
 *  @param propType Type of the instance property.
 *  @param attributes Integer mask of access attributes.
 *  @param value Initial value of the instance property.
 *  @return The slot number used for the property.
 *  @ingroup EjsType
 */
extern int ejsDefineInstanceProperty(Ejs *ejs, EjsType *type, int slotNum, EjsName *name, EjsType *propType, 
                    int attributes, EjsVar *value);

/**
 *  Get a type
 *  @description Get the type installed at the given slot number. All core-types are installed a specific global slots.
 *      When Ejscript is built, these slots are converted into C program defines of the form: ES_TYPE where TYPE is the 
 *      name of the type concerned. For example, you can get the String type object via:
 *      @pre
 *      ejsGetType(ejs, ES_String)
 *  @param ejs Interpreter instance returned from #ejsCreate
 *  @param slotNum Slot number of the type to retrieve. Use ES_TYPE defines. 
 *  @return A type object if successful or zero if the type could not be found
 *  @ingroup EjsType
 */
extern EjsType  *ejsGetType(Ejs *ejs, int slotNum);

extern int      ejsCompactClass(Ejs *ejs, EjsType *type);
extern int      ejsCopyBaseProperties(Ejs *ejs, EjsType *type, EjsType *baseType);
extern EjsType  *ejsCreateCoreType(Ejs *ejs, EjsName *name, EjsType *extendsType, int size, int slotNum, 
                    int numTypeProp, int numInstanceProp, int attributes);
extern EjsBlock *ejsCreateTypeInstanceBlock(Ejs *ejs, EjsType* type, int numInstanceProp);
extern void     ejsDefineTypeNamespaces(Ejs *ejs, EjsType *type);
extern int      ejsFixupBlock(Ejs *ejs, EjsBlock *block, EjsBlock *baseBlock, MprList *implements, int makeRoom);
extern int      ejsFixupClass(Ejs *ejs, EjsType *type, EjsType *baseType, MprList *implements, int makeRoom);
extern int      ejsGetTypePropertyAttributes(Ejs *ejs, EjsVar *vp, int slot);
extern void     ejsInitializeBlockHelpers(struct EjsTypeHelpers *helpers);

extern void     ejsSetTypeName(Ejs *ejs, EjsType *type, EjsName *qname);
extern void     ejsTypeNeedsFixup(Ejs *ejs, EjsType *type);
extern int      ejsGetTypeSize(Ejs *ejs, EjsType *type);


// TODO - OPT. Should this be compressed via bit fields for flags Could use short for these offsets.
/**
 *  Exception Handler Record
 *  @description Each exception handler has an exception handler record allocated that describes it.
 *  @ingroup EjsFunction
 */
typedef struct EjsEx {
    struct EjsType  *catchType;             /**< Type of error to catch */
    uint            flags;                  /**< Exception flags */
    uint            tryStart;               /**< Ptr to start of try block */
    uint            tryEnd;                 /**< Ptr to one past the end */
    uint            handlerStart;           /**< Ptr to start of catch block */
    uint            handlerEnd;             /**< Ptr to one past the end */
} EjsEx;


// TODO OPT. Could compress this.
/**
 *  Byte code
 *  @description This structure describes a sequence of byte code for a function. It also defines a set of
 *      execption handlers pertaining to this byte code.
 *  @ingroup EjsFunction
 */
typedef struct EjsCode {
    uchar           *byteCode;              /**< Byte code */
    int             codeLen;                /**< Byte code length */
    struct EjsConst *constants;             /**< Constant pool. Reference to module constant pool */
    int             numHandlers;            /**< Number of exception handlers */
    int             sizeHandlers;           /**< Size of handlers array */
    struct EjsEx    **handlers;             /**< Exception handlers */
    int             finallyIndex;           /**< Index in handlers for finally handler */

} EjsCode;


/**
 *  Function class
 *  @description The Function type is used to represent closures, function expressions and class methods. 
 *      It contains a reference to the code to execute, the execution scope and possibly a bound "this" reference.
 *  @stability Evolving
 *  @defgroup EjsFunction EjsFunction
 *  @see EjsFunction ejsIsFunction ejsIsNativeFunction ejsIsInitializer ejsCreateFunction ejsCopyFunction
 *      ejsRunFunctionBySlot ejsRunFunction ejsRunInitializer
 */
typedef struct EjsFunction {
    EjsBlock        block;                  /**< Base block. Only used for closures */
    union {
        EjsCode     code;                   /**< Byte code */
        EjsNativeFunction proc;             /**< Native function pointer */
    } body;

    struct EjsType  *resultType;            /**< Return type of method */
    EjsVar          *thisObj;               /**< Bound "this" for closures */
    EjsVar          *owner;                 /**< Back reference to original owning block */
    int             slotNum;                /**< Slot number in owner for this function */

    uint            numArgs: 8;              /**< Count of parameters */
    uint            numDefault: 8;           /**< Count of parameters with default initializers */
    int             nextSlot: 16;            /**< Next multimethod or getter/setter */

    uint            getter: 1;               /**< Function is a getter */
    uint            setter: 1;               /**< Function is a setter */
    uint            staticMethod: 1;         /**< Function is a static method */
    uint            constructor: 1;          /**< Function is a constructor */
    uint            hasReturn: 1;            /**< Function has a return stmt */
    uint            isInitializer: 1;        /**< Function is an initializer function */
    uint            literalGetter: 1;        /**< Function is in an object literal */
    uint            override: 1;             /**< Function overrides a base class method */
    uint            rest: 1;                 /**< Function has a "..." rest of args parameter */
    uint            fullScope: 1;            /**< Closures must capture full scope */
    uint            lang: 2;                 /**< Language compliance level: ecma|plus|fixed */
} EjsFunction;

#if DOXYGEN
    /**
     *  Determine if a variable is a function. This will return true if the variable is a function of any kind, including
     *      methods, native and script functions or initializers.
     *  @param vp Variable to test
     *  @return True if the variable is a function
     *  @ingroup EjsFunction
     */
    extern bool ejsIsFunction(EjsVar *vp);

    /**
     *  Determine if the function is a native function. Functions can be either native - meaning the implementation is
     *      via a C function, or can be scripted.
     *  @param vp Variable to test
     *  @return True if the variable is a native function.
     *  @ingroup EjsFunction
     */
    extern bool ejsIsNativeFunction(EjsVar *vp);

    /**
     *  Determine if the function is an initializer. Initializers are special functions created by the compiler to do
     *      static and instance initialization of classes during construction.
     *  @param vp Variable to test
     *  @return True if the variable is an initializer
     *  @ingroup EjsFunction
     */
    extern bool ejsIsInitializer(EjsVar *vp);
#else
    #define ejsIsFunction(vp) ejsIs(vp, ES_Function)
    #define ejsIsNativeFunction(vp) (vp && (((EjsVar*) (vp))->nativeProc))
    #define ejsIsInitializer(vp)    (ejsIsFunction(vp) && (((EjsFunction*) vp)->isInitializer)
#endif

/**
 *  Create a function object
 *  @description This creates a function object and optionally associates byte code with the function.
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param code Pointer to the byte code. The byte code is not copied so this must be a persistent pointer.
 *  @param codeLen Length of the code.
 *  @param numArgs Number of formal arguments to the function.
 *  @param numExceptions Number of exception handlers
 *  @param returnType Return type of the function. Set to NULL for no defined type.
 *  @param attributes Integer mask of access attributes.
 *  @param constants Reference to the module constant pool. Some byte code opcodes contain references into the
 *      constant pool
 *  @param scope Reference to the chain of blocks that that comprises the lexical scope chain for this function.
 *  @param lang Language level (ecma|plus|fixed). Use constants EJS_SPEC_ECMA, EJS_SPEC_PLUS, EJS_SPEC_FIXED
 *  @return An initialized function object
 *  @ingroup EjsFunction
 */
extern EjsFunction *ejsCreateFunction(Ejs *ejs, const uchar *code, int codeLen, int numArgs,
    int numExceptions, EjsType *returnType, int attributes, struct EjsConst *constants, EjsBlock *scope, int lang);

/**
 *  Run the initializer for a module
 *  @description A module's initializer runs global code defined in the module
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param module Module object reference
 *  @return The last expression result of global code executed
 *  @ingroup EjsFunction
 */
extern EjsVar *ejsRunInitializer(Ejs *ejs, struct EjsModule *module);

/**
 *  Run a function
 *  @description Run a function with the given actual parameters
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param fn Function object to run
 *  @param obj Object to use as the "this" object when running the function
 *  @param argc Count of actual parameters
 *  @param argv Vector of actual parameters
 *  @return The return value from the function. If an exception is thrown, NULL will be returned and ejs->exception
 *      will be set to the exception object.
 *  @ingroup EjsFunction
 */
extern EjsVar *ejsRunFunction(Ejs *ejs, EjsFunction *fn, EjsVar *obj, int argc, EjsVar **argv);

/**
 *  Run a function by slot number
 *  @description Run a function identified by slot number with the given actual parameters. This will run the function
 *      stored at \a slotNum in the \a obj variable. 
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param obj Object that holds the function at its "slotNum" slot. Also use this object as the "this" object 
 *      when running the function.
 *  @param slotNum Slot number in \a obj that contains the function to run.
 *  @param argc Count of actual parameters
 *  @param argv Vector of actual parameters
 *  @return The return value from the function. If an exception is thrown, NULL will be returned and ejs->exception
 *      will be set to the exception object.
 *  @ingroup EjsFunction
 */
extern EjsVar *ejsRunFunctionBySlot(Ejs *ejs, EjsVar *obj, int slotNum, int argc, EjsVar **argv);

extern EjsEx *ejsAddException(EjsFunction *mp, uint tryStart, uint tryEnd, struct EjsType *catchType,
    uint handlerStart, uint handlerEnd, int flags, int preferredIndex);
extern EjsFunction *ejsCopyFunction(Ejs *ejs, EjsFunction *src);
extern int ejsDefineException(Ejs *ejs, struct EjsType *vp, int slot, uint tryOffset,
    uint tryLength, uint handlerOffset, uint handlerLength, int flags);
extern void ejsOffsetExceptions(EjsFunction *mp, int offset);
extern int  ejsSetFunctionCode(EjsFunction *mp, uchar *byteCode, int len);
extern void ejsSetFunctionLocation(EjsFunction *mp, EjsVar *obj, int slotNum);
extern void ejsSetNextFunction(EjsFunction *fun, int nextSlot);


/**
 *  Virtual Machine Execution Frame. 
 *  @description A Frame object is created for each method and block. Frames represent a local activation object
 *      that stores local variables and function arguments. Frames are created from the execution stack and not
 *      from the object heap.
 *  @stability Prototype
 *  @defgroup EjsVm EjsVm
 *  @see EjsFrame EjsLookup EjsStack
 */
typedef struct EjsFrame {
    EjsFunction     function;                   /**< Current function */

#if BLD_DEBUG
    char            *debugName;                 /**< Debug name */
    EjsBlock        *debugScopeChain;           /**< Previous scope chain == function.block.scopeChain */
#endif

    struct EjsFrame *prev;                      /**< Previous block or function */
    struct EjsFrame *caller;                    /**< Previous invoking function frame */
    EjsBlock        *templateBlock;             /**< Original block from which this frame was created */
    struct EjsFrame *saveFrame;                 /**< Saved frame when native code recalls the interpreter */
    EjsFunction     *currentFunction;           /**< Current function */
    EjsVar          *thisObj;                   /**< Set to global for global functions or the type for static methods */
    EjsList         needClosure;                /**< List of blocks needing closures on frame exit */
    int             returnFrame;                /**< Must return from VM when this frame is reached */

    /*
     *  Debug source information
     */ 
    cchar           *currentLine;               /**< Current source code line */
    cchar           *fileName;                  /**< Source code file name */
    int             lineNumber;                 /**< Source code line number */
    int             depth;                      /**< Call depth to limit recursion */
    EjsVar          **prevStackTop;             /**< Frame return mark */
    EjsVar          **stackBase;                /**< Start of the slot and evaluation stack for this frame */
    int             argc;                       /**< Count of function args */
    EjsVar          *returnValue;               /**< Function return value */
    EjsCode         *code;                      /**< Optimized reference to currentFunction->code.*/

    /*
     *  Exception details
     */
    uchar           *endException;              /**< Location just beyond the exception blocks */
    EjsVar          *exceptionArg;              /**< Exception object for catch block */
    EjsVar          *saveException;             /**< Save pending exception */
    EjsEx           *ex;                        /**< Exception handler being executed */
    int             inCatch;                    /**< Executing catch block */
    int             inFinally;                  /**< Executing finally block */

    Ejs             *ejs;                       /**< Optimized reference to speed routines in the VM */

    uchar           *pc;                        /**< Byte oriented program counter */

#if UNUSED
    /*
     *  Various views on the program counter
     */
    union {
        ushort      *spc;
        uint        *ipc;
        uint64      *lpc;
#if BLD_FEATURE_FLOATING_POINT
        double      *dpc;
#endif
        uint64      *llpc;
    };
#endif

} EjsFrame;

extern EjsFrame *ejsPopFrame(Ejs *ejs);
extern EjsFrame *ejsPushFrame(Ejs *ejs, EjsBlock *block);


/**
 *  Array class
 *  @description Arrays provide a growable, integer indexed, in-memory store for objects. An array can be treated as a 
 *      stack (FIFO or LIFO) or a list (ordered). Insertions can be done at the beginning or end of the stack or at an 
 *      indexed location within a list. The Array class can store objects with numerical indicies and can also store 
 *      any named properties. The named properties are stored in the obj field, whereas the numeric indexed values are
 *      stored in the data field. Array extends EjsObject and has all the capabilities of EjsObject.
 *  @stability Evolving
 *  @defgroup EjsArray EjsArray
 *  @see EjsArray ejsCreateArray ejsIsArray
 */
typedef struct EjsArray {
    EjsObject       obj;                /**< Extends Object */
    EjsVar          **data;             /**< Array elements */
    int             length;             /**< Array length property */
} EjsArray;


/**
 *  Create an array
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param size Initial size of the array
 *  @return A new array object
 *  @ingroup EjsArray
 */
extern EjsArray *ejsCreateArray(Ejs *ejs, int size);

#if DOXYGEN
    /**
     *  Determine if a variable is an array
     *  @param vp Variable to test
     *  @return True if the variable is an array
     *  @ingroup EjsArray
     */
    extern bool ejsIsArray(EjsVar *vp);
#else
    #define ejsIsArray(vp) ejsIs(vp, ES_Array)
#endif

/**
 *  Boolean class
 *  @description The Boolean class provides the base class for the boolean values "true" and "false".
 *      EjsBoolean is a primitive native type and extends EjsVar. It is still logically an Object, but implements
 *      Object properties and methods itself. Only two instances of the boolean class are ever created created
 *      these are referenced as ejs->trueValue and ejs->falseValue.
 *  @stability Evolving
 *  @defgroup EjsBoolean EjsBoolean
 *  @see EjsBoolean ejsCreateBoolean ejsIsBoolean ejsGetBoolean
 */
typedef struct EjsBoolean {
    EjsVar          var;                /**< Logically extends Object */
    bool            value;              /**< Boolean value */
} EjsBoolean;

/**
 *  Create a boolean
 *  @description Create a boolean value. This will not actually create a new boolean instance as there can only ever
 *      be two boolean instances (true and false). Boolean properties are immutable in Ejscript and so this routine
 *      will simply return the appropriate pre-created true or false boolean value.
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param value Desired boolean value. Set to 1 for true and zero for false.
 *  @ingroup EjsBoolean
 */
extern EjsBoolean *ejsCreateBoolean(Ejs *ejs, int value);

/**
 *  Cast a variable to a boolean 
 *  @description
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param vp Variable to cast
 *  @return A new boolean object
 *  @ingroup EjsBoolean
 */
extern EjsBoolean *ejsToBoolean(Ejs *ejs, EjsVar *vp);

#if DOXYGEN
    /**
     *  Determine if a variable is a boolean
     *  @param vp Variable to test
     *  @return True if the variable is a boolean
     *  @ingroup EjsBoolean
     */
    extern bool ejsIsBoolean(EjsVar *vp);

    /**
     *  Get the C boolean value from a boolean object
     *  @param vp Boolean variable to access
     *  @return True or false
     *  @ingroup EjsBoolean
     */
    extern bool ejsGetBoolean(EjsVar *vp);
#else
    #define ejsIsBoolean(vp) ejsIs(vp, ES_Boolean)
    #define ejsGetBoolean(vp) (((EjsBoolean*) (vp))->value)
#endif

/**
 *  ByteArray class
 *  @description ByteArrays provide a growable, integer indexed, in-memory store for bytes. ByteArrays are a powerful 
 *  data type that can be used as a simple array to store and encode data as bytes or it can be used as a Stream 
 *  implementing the Stream interface.
 *  \n\n
 *  When used as a simple byte array, the ByteArray class offers a low level set of methods to insert and 
 *  extract bytes. The index operator [] can be used to access individual bytes and the copyIn and copyOut methods 
 *  can be used to get and put blocks of data. In this mode, the read and write position properties are ignored. 
 *  Accesses to the byte array are from index zero up to the size defined by the length property. When constructed, 
 *  the ByteArray can be designated as growable, in which case the initial size will grow as required to accomodate 
 *  data and the length property will be updated accordingly.
 *  \n\n
 *  When used as a Stream, the byte array offers various read and write methods which store data at the location 
 *  specified by the write position property and they read data from the read position. The available method 
 *  indicates how much data is available between the read and write position pointers. The flush method will 
 *  reset the pointers to the start of the array. The length property is unchanged in behavior from when used as 
 *  a simple byte array and it specifies the overall storage capacity of the byte array. As numeric values are 
 *  read or written, they will be encoded according to the value of the endian property which can be set to 
 *  either LittleEndian or BigEndian. When used with for/in, ByteArrays will iterate or enumerate over the 
 *  available data between the read and write pointers.
 *  \n\n
 *  In Stream mode ByteArrays can be configured with input and output callbacks to provide or consume data to other 
 *  streams or components. These callbacks will automatically be invoked as required when the various read/write 
 *  methods are called.
 *  \n\n
 *  Unlike the Array class, ByteArray can only store data in numeric indicies. It is much efficient than
 *  EjsByteArray is a primitive native type and extends EjsVar. It is still logically an Object, but implements
 *  Object properties and methods itself. Only two instances of the boolean class are ever created created
 *  these are referenced as ejs->trueValue and ejs->falseValue.
 *  @stability Evolving
 *  @defgroup EjsByteArray EjsByteArray
 *  @see EjsByteArray ejsIsByteArray ejsCreateByteArray ejsSetByteArrayPositions ejsCopyToByteArray
 */
typedef struct EjsByteArray {
    EjsVar          var;                /**< Logically extends Object */
    uchar           *value;             /**< Data bytes in the array */
    int             length;             /**< Length property */
    bool            growable;           /**< Aray is growable */
    int             endian;             /**< Endian encoding */
    int             swap;               /**< I/O must swap bytes due to endian byte ordering */
    int             growInc;            /**< Current read position */
    int             readPosition;       /**< Current read position */
    int             writePosition;      /**< Current write position */
    EjsFunction     *input;             /**< Input callback function to get more data */
    EjsFunction     *output;            /**< Output callback function send data */
} EjsByteArray;

#if DOXYGEN
    /**
     *  Determine if a variable is a byte array
     *  @param vp Variable to test
     *  @return True if the variable is a byte array
     *  @ingroup EjsByteArray
     */
    extern bool ejsIsByteArray(EjsVar *vp);
#else
    #define ejsIsByteArray(vp) ejsIs(vp, ES_ByteArray)
#endif

/**
 *  Create a byte array
 *  @description Create a new byte array instance.
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param size Initial size of the byte array
 *  @return A new byte array instance
 *  @ingroup EjsByteArray
 */
extern EjsByteArray *ejsCreateByteArray(Ejs *ejs, int size);

/**
 *  Set the I/O byte array positions
 *  @description Set the read and/or write positions into the byte array. ByteArrays implement the Stream interface
 *      and support sequential and random access reading and writing of data in the array. The byte array maintains
 *      read and write positions that are automatically updated as data is read or written from or to the array. 
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param ap Byte array object
 *  @param readPosition New read position to set
 *  @param writePosition New write position to set
 *  @ingroup EjsByteArray
 */
extern void ejsSetByteArrayPositions(Ejs *ejs, EjsByteArray *ap, int readPosition, int writePosition);

/**
 *  Copy data into a byte array
 *  @description Copy data into a byte array at a specified \a offset. 
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param ap Byte array object
 *  @param offset Offset in the byte array to which to copy the data.
 *  @param data Pointer to the source data
 *  @param length Length of the data to copy
 *  @return Zero if successful, otherwise a negative MPR error code.
 */
extern int ejsCopyToByteArray(Ejs *ejs, EjsByteArray *ap, int offset, char *data, int length);

extern struct EjsNumber *ejsWriteToByteArray(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv);


/**
 *  Date class
 *  @description The Date class is a general purpose class for working with dates and times. 
 *      is a a primitive native type and extends EjsVar. It is still logically an Object, but implements Object 
 *      properties and methods itself. 
 *  @stability Evolving
 *  @defgroup EjsDate EjsDate
 *  @see EjsDate EjsIsDate ejsCreateDate
 */
typedef struct EjsDate {
    EjsVar          var;                /**< Logically extends Object */
    MprTime         value;              /**< Time in milliseconds since "1970/01/01 GMT" */
} EjsDate;

#if DOXYGEN
    /**
     *  Determine if a variable is a Date
     *  @param vp Variable to test
     *  @return True if the variable is a date
     *  @ingroup EjsDate
     */
    bool ejsIsDate(EjsVar *vp);
#else
    #define ejsIsDate(vp) ejsIs(vp, ES_Date)
#endif

/**
 *  Create a new date instance
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param value Date/time value to set the new date instance to
 *  @return An initialized date instance
 *  @ingroup EjsDate
 */
extern EjsDate *ejsCreateDate(Ejs *ejs, MprTime value);

/**
 *  Error classes
 *  @description Base class for error exception objects. Exception objects are created by programs and by the system 
 *  as part of changing the normal flow of execution when some error condition occurs. 
 *  When an exception is created and acted upon ("thrown"), the system transfers the flow of control to a 
 *  pre-defined instruction stream (the handler or "catch" code). The handler may return processing to the 
 *  point at which the exception was thrown or not. It may re-throw the exception or pass control up the call stack.
 *  @stability Evolving.
 *  @defgroup EjsError EjsError ejsFormatStack ejsGetErrorMsg ejsHasException ejsThrowArgError ejsThrowAssertError
 *      ejsThrowArithmeticError ejsThrowInstructionError ejsThrowError ejsThrowInternalError ejsThrowIOError
 *      ejsThrowMemoryError ejsThrowOutOfBoundsError ejsThrowReferenceError ejsThrowResourceError ejsThrowStateError
 *      ejsThrowStopIteration ejsThrowSyntaxError ejsThrowTypeError
 */
typedef struct EjsError {
    EjsObject       obj;                /**< Extends Object */
    char            *message;           /**< Exception message */
    char            *stack;             /**< Execution stack back trace */
    int             code;               /**< Unique error lookup code */
} EjsError;

#define ejsIsError(vp) ejsIs(vp, ES_Error)

/**
 *  Format the stack backtrace
 *  @description Return a string containing the current interpreter stack backtrace
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @return A string containing the stack backtrace. The caller must free.
 *  @ingroup EjsError
 */
extern char *ejsFormatStack(Ejs *ejs);

/**
 *  Get the interpreter error message
 *  @description Return a string containing the current interpreter error message
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param withStack Set to 1 to include a stack backtrace in the error message
 *  @return A string containing the error message. The caller must free.
 *  @ingroup EjsError
 */
extern char *ejsGetErrorMsg(Ejs *ejs, int withStack);

/**
 *  Determine if an exception has been thrown
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @return True if an exception has been thrown
 *  @ingroup EjsError
 */
extern bool ejsHasException(Ejs *ejs);

// TODO
extern EjsVar *ejsGetException(Ejs *ejs);

/**
 *  Throw an argument exception
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param fmt Printf style format string to use for the error message
 *  @param ... Message arguments
 *  @ingroup EjsError
 */
extern EjsVar *ejsThrowArgError(Ejs *ejs, cchar *fmt, ...) PRINTF_ATTRIBUTE(2,3);

/**
 *  Throw an assertion exception
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param fmt Printf style format string to use for the error message
 *  @param ... Message arguments
 *  @ingroup EjsError
 */
extern EjsVar *ejsThrowAssertError(Ejs *ejs, cchar *fmt, ...) PRINTF_ATTRIBUTE(2,3);

/**
 *  Throw an math exception
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param fmt Printf style format string to use for the error message
 *  @param ... Message arguments
 *  @ingroup EjsError
 */
extern EjsVar *ejsThrowArithmeticError(Ejs *ejs, cchar *fmt, ...) PRINTF_ATTRIBUTE(2,3);

/**
 *  Throw an instruction code exception
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param fmt Printf style format string to use for the error message
 *  @param ... Message arguments
 *  @ingroup EjsError
 */
extern EjsVar *ejsThrowInstructionError(Ejs *ejs, cchar *fmt, ...) PRINTF_ATTRIBUTE(2,3);

/**
 *  Throw an general error exception
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param fmt Printf style format string to use for the error message
 *  @param ... Message arguments
 *  @ingroup EjsError
 */
extern EjsVar *ejsThrowError(Ejs *ejs, cchar *fmt, ...) PRINTF_ATTRIBUTE(2,3);

/**
 *  Throw an internal error exception
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param fmt Printf style format string to use for the error message
 *  @param ... Message arguments
 *  @ingroup EjsError
 */
extern EjsVar *ejsThrowInternalError(Ejs *ejs, cchar *fmt, ...) PRINTF_ATTRIBUTE(2,3);

/**
 *  Throw an IO exception
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param fmt Printf style format string to use for the error message
 *  @param ... Message arguments
 *  @ingroup EjsError
 */
extern EjsVar *ejsThrowIOError(Ejs *ejs, cchar *fmt, ...) PRINTF_ATTRIBUTE(2,3);

/**
 *  Throw an Memory depletion exception
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @ingroup EjsError
 */
extern EjsVar *ejsThrowMemoryError(Ejs *ejs);

/**
 *  Throw an out of bounds exception
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param fmt Printf style format string to use for the error message
 *  @param ... Message arguments
 *  @ingroup EjsError
 */
extern EjsVar *ejsThrowOutOfBoundsError(Ejs *ejs, cchar *fmt, ...) PRINTF_ATTRIBUTE(2,3);

/**
 *  Throw an reference exception
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param fmt Printf style format string to use for the error message
 *  @param ... Message arguments
 *  @ingroup EjsError
 */
extern EjsVar *ejsThrowReferenceError(Ejs *ejs, cchar *fmt, ...) PRINTF_ATTRIBUTE(2,3);

/**
 *  Throw an resource exception
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param fmt Printf style format string to use for the error message
 *  @param ... Message arguments
 *  @ingroup EjsError
 */
extern EjsVar *ejsThrowResourceError(Ejs *ejs, cchar *fmt, ...) PRINTF_ATTRIBUTE(2,3);

/**
 *  Throw an state exception
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param fmt Printf style format string to use for the error message
 *  @param ... Message arguments
 *  @ingroup EjsError
 */
extern EjsVar *ejsThrowStateError(Ejs *ejs, cchar *fmt, ...) PRINTF_ATTRIBUTE(2,3);

/**
 *  Throw an stop iteration exception
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @ingroup EjsError
 */
extern EjsVar *ejsThrowStopIteration(Ejs *ejs);

/**
 *  Throw an syntax error exception
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param fmt Printf style format string to use for the error message
 *  @param ... Message arguments
 *  @ingroup EjsError
 */
extern EjsVar *ejsThrowSyntaxError(Ejs *ejs, cchar *fmt, ...) PRINTF_ATTRIBUTE(2,3);

/**
 *  Throw an type error exception
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param fmt Printf style format string to use for the error message
 *  @param ... Message arguments
 *  @ingroup EjsError
 */
extern EjsVar *ejsThrowTypeError(Ejs *ejs, cchar *fmt, ...) PRINTF_ATTRIBUTE(2,3);


/**
 *  File class
 *  @description The File class provides a foundation of I/O services to interact with physical files and directories.
 *  Each File object represents a single file or directory and provides methods for creating, opening, reading, writing 
 *  and deleting files, and for accessing and modifying information about the file.
 *  @stability Prototype
 *  @defgroup EjsFile EjsFile 
 *  @see EjsFile ejsCreateFile ejsIsFile
 */
typedef struct EjsFile {
    EjsObject       obj;                /**< Extends Object */
    MprFile         *file;              /**< Open file handle */
    MprFileInfo     info;               /**< Cached file info */
    char            *path;              /**< Filename path */
    int             mode;               /**< Current open mode */
#if FUTURE
    cchar           *cygdrive;          /**< Cygwin drive directory (c:/cygdrive) */
    cchar           *newline;           /**< Newline delimiters */
    int             delimiter;          /**< Path delimiter ('/' or '\\') */
    int             hasDriveSpecs;      /**< Paths on this file system have a drive spec */
#endif
#if BLD_FEATURE_MMU && FUTURE
    uchar           *mapped;            /**< Memory mapped file */
#endif
} EjsFile;

/**
 *  Create a File object
 *  @description Create a file object associated with the given filename. The filename is not opened, just stored.
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param filename Filename to associate with the file object
 *  @return A new file object
 *  @ingroup EjsFile
 */
extern EjsFile *ejsCreateFile(Ejs *ejs, cchar *filename);

#if DOXYGEN
    /**
     *  Determine if a variable is a File
     *  @param vp Variable to test
     *  @return True if the variable is a File
     *  @ingroup File
     */
    extern bool ejsIsFile(EjsVar *vp);
#else
    #define ejsIsFile(vp) ejsIs(vp, ES_ejs_io_File)
#endif


/**
 *  EjsGlobal cass
 *  @description The Global class is the base class for the global object. The global object is the top level
 *      scoping object.
 *  @stability Stable
 *  @defgroup EjsGlobal EjsGlobal
 */
typedef EjsObject EjsGlobal;

extern EjsVar *ejsCreateGlobal(Ejs *ejs);


#if BLD_FEATURE_HTTP_CLIENT
/**
 *  Http Class
 *  @description
 *      Http objects represents a Hypertext Transfer Protocol version 1.1 client connection and are used 
 *      HTTP requests and capture responses. This class supports the HTTP/1.1 standard including methods for GET, POST, 
 *      PUT, DELETE, OPTIONS, and TRACE. It also supports Keep-Alive and SSL connections. 
 *  @stability Prototype
 *  @defgroup EjsHttp EjsHttp
 *  @see EjsHttp ejsCreateHttp ejsIsHttp
 */
typedef struct EjsHttp {
    EjsObject       obj;                        /**< Extends Object */
    MprHttp         *http;                      /**< Underlying MPR http object */
    Ejs             *ejs;                       /**< Convenience access to ejs interpreter instance */
    EjsFunction     *callback;                  /**< Async callback function */
    MprBuf          *content;                   /**< Response data */
    char            *uri;                       /**< Target uri */
    char            *method;                    /**< HTTP method */
    char            *keyFile;                   /**< SSL key file */
    char            *certFile;                  /**< SSL certificate file */
    char            *postData;                  /**< Post data supplied via post() method */
    int             contentLength;              /**< Expected content length for post/put data */
    int             readOffset;                 /**< Read response I/O ptr */
    uint            requestStarted: 1;          /**< Request started and connection to server is open */
    uint            gotResponse: 1;             /**< Request has been sent and response headers have been received */
    uint            followRedirects: 1;         /**< Transparently follow 30X redirects */
} EjsHttp;

/**
 *  Create a new Http object
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @return a new Http object
 *  @ingroup EjsHttp
 */
extern EjsHttp *ejsCreateHttp(Ejs *ejs);

#if DOXYGEN
    extern bool ejsIsHttp(EjsVar *vp);
#else
    #define ejsIsHttp(vp) ejsIs(vp, ES_ejs_io_Http)
#endif
#endif /* BLD_FEATURE_HTTP_CLIENT */

/**
 *  Iterator Class
 *  @description Iterator is a helper class to implement iterators in other native classes
 *  @stability Prototype
 *  @defgroup EjsIterator EjsIterator
 *  @see EjsIterator ejsCreateIterator
 */
typedef struct EjsIterator {
    EjsVar              var;                        /**< Logically extends Object */
    EjsVar              *target;                    /**< Object to be enumerated */
    EjsNativeFunction   nativeNext;                 /**< Native next function */
    bool                deep;                       /**< Iterator deep (recursively over all properties) */
    EjsArray            *namespaces;                /**< Namespaces to consider in iteration */

    /*
     *  Convenient data store
     */
    int                 index;                      /**< Current index */
    EjsVar              *indexVar;                  /**< Reference to current item */
} EjsIterator;

/**
 *  Create an iterator object
 *  @description The EjsIterator object is a helper class for native types to implement iteration and enumeration.
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param target Target variable to iterate or enumerate 
 *  @param next Function to invoke to step to the next element
 *  @param deep Set to true to do a deep iteration/enumeration
 *  @param namespaces Reserved and not used. Supply NULL.
 *  @return A new EjsIterator object
 *  @ingroup EjsIterator
 */
extern EjsIterator *ejsCreateIterator(Ejs *ejs, EjsVar *target, EjsNativeFunction next, bool deep, EjsArray *namespaces);

/**
 *  Namespace Class
 *  @description Namespaces are used to qualify names into discrete spaces.
 *  @stability Evolving
 *  @defgroup EjsNamespace EjsNamespace
 *  @see EjsNamespace ejsIsNamespace ejsCreateNamespace ejsLookupNamespace ejsDefineReservedNamespace 
 *      ejsCreateReservedNamespace ejsFormatReservedNamespace 
 */
typedef struct EjsNamespace {
    EjsVar      var;                                /**< Logically extends Object */
    char        *name;                              /**< Textual name of the namespace */
    char        *uri;                               /**< Optional uri definition */
    int         flags;                              /**< Fast comparison flags */
} EjsNamespace;

/**
 *  Create a namespace object
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param name Space name to use for the namespace
 *  @param uri URI to associate with the namespace
 *  @return A new namespace object
 *  @ingroup EjsNamespace
 */
extern EjsNamespace *ejsCreateNamespace(Ejs *ejs, cchar *name, cchar *uri);

#if DOXYGEN
    /**
     *  Determine if a variable is a namespace
     *  @return True if the variable is a namespace
     *  @ingroup EjsNamespace
     */
    extern bool ejsIsNamespace(EjsVar *vp)
#else
    #define ejsIsNamespace(vp) ejsIs(vp, ES_Namespace)
#endif

extern EjsNamespace *ejsDefineReservedNamespace(Ejs *ejs, EjsBlock *block, EjsName *typeName, cchar *name);
extern EjsNamespace *ejsCreateReservedNamespace(Ejs *ejs, EjsName *typeName, cchar *name);
extern char *ejsFormatReservedNamespace(MprCtx ctx, EjsName *typeName, cchar *spaceName);

/**
 *  Null Class
 *  @description The Null class provides the base class for the singleton null instance. This instance is stored
 *      in ejs->nullValue.
 *  @stability Evolving
 *  @defgroup EjsNull EjsNull
 *  @see EjsNull ejsCreateIsNull
 */
typedef EjsVar EjsNull;

/**
 *  Determine if a variable is a null
 *  @return True if a variable is a null
 *  @ingroup EjsNull
 */
#define ejsIsNull(vp) ejsIs(vp, ES_Null)

extern EjsNull *ejsCreateNull(Ejs *ejs);

/**
 *  Number class
 *  @description The Number class provide the base class for all numeric values. 
 *      The primitive number storage data type may be set via the configure program to be either double, float, int
 *      or int64. 
 *  @stability Evolving
 *  @defgroup EjsNumber EjsNumber
 *  @see EjsNumber ejsToNumber ejsCreateNumber ejsIsNumber ejsGetNumber ejsGetInt ejsGetDouble ejsIsInfinite ejsIsNan
 */
typedef struct EjsNumber {
    EjsVar      var;                                /**< Logically extends Object */
    MprNumber   value;                              /**< Numeric value */
} EjsNumber;


/**
 *  Create a number object
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param value Numeric value to initialize the number object
 *  @return A number object
 *  @ingroup EjsNumber
 */
extern EjsNumber *ejsCreateNumber(Ejs *ejs, MprNumber value);

/**
 *  Cast a variable to a number
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param vp Variable to cast
 *  @return A number object
 *  @ingroup EjsNumber
 */
extern struct EjsNumber *ejsToNumber(Ejs *ejs, EjsVar *vp);

#if DOXYGEN
    /**
     *  Determine if a variable is a number
     *  @param vp Variable to examine
     *  @return True if the variable is a number
     *  @ingroup EjsNumber
     */
    extern bool ejsIsNumber(EjsVar *vp);

    /**
     *  Get the numeric value stored in a EjsNumber object
     *  @param vp Variable to examine
     *  @return A numeric value
     *  @ingroup EjsNumber
     */
    extern MprNumber ejsGetNumber(EjsVar *vp);

    /**
     *  Get the numeric value stored in a EjsNumber object
     *  @param vp Variable to examine
     *  @return An integer value
     *  @ingroup EjsNumber
     */
    extern int ejsGetInt(EjsVar *vp);

    /**
     *  Get the numeric value stored in a EjsNumber object
     *  @param vp Variable to examine
     *  @return A double value
     *  @ingroup EjsNumber
     */
    extern int ejsGetDouble(EjsVar *vp);
#else
    #define ejsIsNumber(vp) ejsIs(vp, ES_Number)
    #define ejsGetNumber(vp) ((vp) ? ((EjsNumber*) (vp))->value: 0)
    #define ejsGetInt(vp) ((vp) ? ((int) (((EjsNumber*) (vp))->value)): 0)
    #if BLD_FEATURE_FLOATING_POINT
        #define ejsGetDouble(vp) ((vp) ? ((double) (((EjsNumber*) (vp))->value)): 0)
    #endif
#endif

#if BLD_FEATURE_FLOATING_POINT
extern bool ejsIsInfinite(MprNumber f);
#if WIN
#define ejsIsNan(f) (_isnan(f))
#elif MACOSX
    #define ejsIsNan(f) isnan(f)
#elif VXWORKS
    #define ejsIsNan(f) isnan(f)
#else
    #define ejsIsNan(f) (f == FP_NAN)
#endif
#endif

/**
 *  Reflect Class
 *  @description The Reflect class permits introspection into the type and attributes of objects and properties.
 *  @stability Evolving
 *  @defgroup EjsNamespace EjsNamespace
 *  @see EjsReflect
 */
typedef struct EjsReflect {
    EjsVar      var;                                /**< Logically extends Object */
    EjsVar      *subject;                           /**< Object under examination */
} EjsReflect;


extern EjsVar *ejsGetTypeName(Ejs *ejs, EjsVar *vp);
extern EjsVar *ejsGetTypeOf(Ejs *ejs, EjsVar *vp);


/**
 *  RegExp Class
 *  @description The regular expression class provides string pattern matching and substitution.
 *  @stability Evolving
 *  @defgroup EjsRegExp EjsRegExp
 *  @see EjsRegExp ejsCreateRegExp ejsIsRegExp
 */
typedef struct EjsRegExp {
    EjsVar          var;                            /**< Logically extends Object */
    char            *pattern;                       /**< Pattern to match with */
    void            *compiled;                      /**< Compiled pattern */
    bool            global;                         /**< Search for pattern globally (multiple times) */
    bool            ignoreCase;                     /**< Do case insensitive matching */
    bool            multiline;                      /**< Match patterns over multiple lines */
    bool            sticky;
    int             endLastMatch;                   /**< End of the last match (one past end) */
    int             startLastMatch;                 /**< Start of the last match */
    struct EjsString *matched;                      /**< Last matched component */
} EjsRegExp;


/**
 *  Create a new regular expression object
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param pattern Regular expression pattern string
 *  @return a EjsRegExp object
 *  @ingroup EjsRegExp
 */
extern EjsRegExp *ejsCreateRegExp(Ejs *ejs, cchar *pattern);

#if DOXYGEN
    /**
     *  Determine if the variable is a regular expression
     *  @return True if the variable is a regular expression
     *  @ingroup EjsRegExp
     */
    extern bool ejsIsRegExp(EjsVar *vp);
#else
    #define ejsIsRegExp(vp) ejsIs(vp, ES_RegExp)
#endif

/**
 *  String Class
 *  @description The String class provides the base class for all strings. Each String object represents a single 
 *  immutable linear sequence of characters. Strings have operators for: comparison, concatenation, copying, 
 *  searching, conversion, matching, replacement, and, subsetting.
 *  \n\n
 *  Strings are currently sequences of UTF-8 characters. They will soon be upgraded to UTF-16.
 *  @stability Evolving
 *  @defgroup EjsString EjsString
 *  @see EjsString ejsToString ejsCreateString ejsCreateBareString ejsCreateStringWithLength ejsDupString
 *      ejsVarToString ejsStrdup ejsStrcat ejsIsString ejsGetString
 */
typedef struct EjsString {
    EjsVar      var;                                /**< Logically extentends Object */
    int         length;                             /**< String length (sans null) */
    char        *value;                             /**< String value. Currently UTF-8. Will upgrade to UTF-16 soon */
} EjsString;


/**
 *  Cast a variable to a string
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param vp Variable to cast
 *  @return A string object
 *  @ingroup EjsString
 */
extern EjsString *ejsToString(Ejs *ejs, EjsVar *vp);

/**
 *  Create a string object
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param value C string value to define for the string object. Note: this will soon be changed to unicode.
 *  @stability Prototype
 *  @return A string object
 *  @ingroup EjsString
 */
extern EjsString *ejsCreateString(Ejs *ejs, cchar *value);

/**
 *  Create an empty string object
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param len Length of space to reserve for future string data
 *  @return A string object
 *  @ingroup EjsString
 */
extern EjsString *ejsCreateBareString(Ejs *ejs, int len);

/**
 *  Create a string and reserve extra room.
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param value C string value to define for the string object. Note: this will soon be changed to unicode.
 *  @param len Length of the string storage to allocate.
 *  @return A string object
 *  @ingroup EjsString
 */
extern EjsString *ejsCreateStringWithLength(Ejs *ejs, cchar *value, int len);

//  TODO - why do we need this. Why not just use ejsCloneVar?
/**
 *  Duplicate a string object
 *  @param ejs Ejs reference returned from #ejsCreate
 *  @param sp String value to copy
 *  @return A string object
 *  @ingroup EjsString
 */
extern EjsString *ejsDupString(Ejs *ejs, EjsString *sp);

#if DOXYGEN
    bool ejsIsString(EjsVar *vp);
    cchar *ejsGetString(EjsVar *vp);
#else
    #define ejsIsString(vp) ejsIs(vp, ES_String)
    #define ejsGetString(vp) ((vp) ? (((EjsString*) vp)->value): "")
#endif

extern int ejsStrdup(MprCtx ctx, uchar **dest, const void *src, int nbytes);
extern int ejsStrcat(Ejs *ejs, EjsString *dest, EjsVar *src);


/**
 *  Timer Class
 *  @description Timers manage the scheduling and execution of Ejscript functions. Timers run repeatedly 
 *      until stopped by calling the stop method and are scheduled with a granularity of 1 millisecond. 
 *  @stability Evolving
 *  @defgroup EjsTimer EjsTimer
 *  @see EjsTimer
 */
typedef struct EjsTimer {
    EjsObject       obj;                            /**< Extends Object */
    Ejs     *ejs;                           /**< Need interpreter reference in callback */
    MprEvent        *event;                         /**< MPR event for the timer */
    int             drift;                          /**< Event is allowed to drift */
    int             period;                         /**< Time in msec between invocations */          
    EjsFunction     *callback;                      /**< Callback function */
} EjsTimer;


/**
 *  Void class
 *  @description The Void class provides the base class for the singleton "undefined" instance. This instance is stored
 *      in ejs->undefinedValue..
 *  @stability Evolving
 *  @defgroup EjsVoid EjsVoid
 *  @see EjsVoid
 */
typedef EjsVar EjsVoid;

extern EjsVoid  *ejsCreateUndefined(Ejs *ejs);
#define ejsIsUndefined(vp) ejsIs(vp, ES_Void)


#if BLD_FEATURE_EJS_E4X
/*
 *  Xml tag state
 */
typedef struct EjsXmlTagState {
    struct EjsXML   *obj;
    //  TODO these two should be XML also
    EjsVar          *attributes;
    EjsVar          *comments;
} EjsXmlTagState;


/*
 *  Xml Parser state
 */
typedef struct EjsXmlState {
    EjsXmlTagState  nodeStack[E4X_MAX_NODE_DEPTH];
    Ejs     *ejs;
    EjsType         *xmlType;
    EjsType         *xmlListType;
    int             topOfStack;
    long            inputSize;
    long            inputPos;
    cchar           *inputBuf;
    cchar           *filename;
} EjsXmlState;


/**
 *  XML class
 *  @description The XML class and API is based on ECMA-357 -- ECMAScript for XML (E4X). The XML class is a 
 *  core class in the E4X specification; it provides the ability to load, parse and save XML documents.
 *  @stability Evolving
 *  @defgroup EjsXML EjsXML
 *  @see EjsXML ejsIsXML ejsConfigureXML ejsCreateXML ejsLoadXMLString ejsDeepCopyXML ejsXMLDescendants
 */
typedef struct EjsXML {
    EjsVar          var;                    /**< Extends Object (logically) */
    EjsName         qname;                  /**< XML node name (e.g. tagName) */
    int             kind;                   /**< Kind of XML node */
    MprList         *elements;              /**< List elements or child nodes */
    MprList         *attributes;            /**< Node attributes */
    MprList         *namespaces;            /**< List of namespaces as Namespace objects */
    struct EjsXML   *parent;                /**< Parent node reference (XML or XMLList) */
    struct EjsXML   *targetObject;          /**< XML/XMLList object modified when items inserted into an empty list */
    EjsName         targetProperty;         /**< XML property modified when items inserted into an empty list */
    char            *value;                 /**< String vale of text|attribute|comment|pi */
    int             flags;
} EjsXML;


#if DOXYGEN
    /**
     *  Determine if a variable is an XML object
     *  @return true if the variable is an XML or XMLList object
     *  @ingroup EjsXML
     */
    extern boolean ejsIsXML(EjsVar *vp);
#else
    #define ejsIsXML(vp) (ejsIs(vp, ES_XML) || ejsIs(vp, ES_XMLList))
#endif

extern EjsXML *ejsCreateXML(Ejs *ejs, int kind, EjsName *name, EjsXML *parent, cchar *value);
extern void  ejsLoadXMLString(Ejs *ejs, EjsXML *xml, cchar *xmlString);
extern EjsXML *ejsConfigureXML(Ejs *ejs, EjsXML *xml, int kind, cchar *name, EjsXML *parent, cchar *value);
extern EjsXML *ejsDeepCopyXML(Ejs *ejs, EjsXML *xml);
extern EjsXML *ejsXMLDescendants(Ejs *ejs, EjsXML *xml, EjsName *qname);

/*
 *  Xml private prototypes
 */
extern void ejsMarkXML(Ejs *ejs, EjsVar *parent, EjsXML *xml);
extern MprXml *ejsCreateXmlParser(Ejs *ejs, EjsXML *xml, cchar *filename);
extern int ejsXMLToString(Ejs *ejs, MprBuf *buf, EjsXML *xml, int indentLevel);
extern EjsXML *ejsAppendToXML(Ejs *ejs, EjsXML *xml, EjsXML *node);
extern EjsXML *ejsSetXML(Ejs *ejs, EjsXML *xml, int index, EjsXML *node);
extern int ejsAppendAttributeToXML(Ejs *ejs, EjsXML *parent, EjsXML *node);
extern EjsXML *ejsCreateXMLList(Ejs *ejs, EjsXML *targetObject, EjsName *targetProperty);

#else
#define ejsIsXML(vp) 0
#endif /* BLD_FEATURE_EJS_E4X */


/**
 *  Type Helpers
 *  @description The type helpers interface defines the set of primitive operations a type must support to
 *      interact with the virtual machine.
 *  @ingroup EjsType
 */
typedef struct EjsTypeHelpers
{
    EjsVar  *(*castVar)(Ejs *ejs, EjsVar *vp, struct EjsType *type);
    EjsVar  *(*cloneVar)(Ejs *ejs, EjsVar *vp, bool deep);
    EjsVar  *(*createVar)(Ejs *ejs, struct EjsType *type, int size);
    int     (*defineProperty)(Ejs *ejs, EjsVar *vp, int slotNum, EjsName *qname, struct EjsType *propType, 
                int attributes, EjsVar *value);
    void    (*destroyVar)(Ejs *ejs, EjsVar *vp);
    int     (*deleteProperty)(Ejs *ejs, EjsVar *vp, int slotNum);
    int     (*deletePropertyByName)(Ejs *ejs, EjsVar *vp, EjsName *qname);
    int     (*finalizeVar)(Ejs *ejs, EjsVar *vp);
    EjsVar  *(*getProperty)(Ejs *ejs, EjsVar *vp, int slotNum);
    EjsVar  *(*getPropertyByName)(Ejs *ejs, EjsVar *vp, EjsName *qname);
    int     (*getPropertyCount)(Ejs *ejs, EjsVar *vp);
    EjsName (*getPropertyName)(Ejs *ejs, EjsVar *vp, int slotNum);
    struct EjsTrait *(*getPropertyTrait)(Ejs *ejs, EjsVar *vp, int slotNum);
    EjsVar  *(*invokeOperator)(Ejs *ejs, EjsVar *vp, int opCode, EjsVar *rhs);
    int     (*lookupProperty)(Ejs *ejs, EjsVar *vp, EjsName *qname);
    void    (*markVar)(Ejs *ejs, EjsVar *parent, EjsVar *vp);
    int     (*setProperty)(Ejs *ejs, EjsVar *vp, int slotNum, EjsVar *value);
    int     (*setPropertyByName)(Ejs *ejs, EjsVar *vp, EjsName *qname, EjsVar *value);
    int     (*setPropertyName)(Ejs *ejs, EjsVar *vp, int slotNum, EjsName *qname);
    int     (*setPropertyTrait)(Ejs *ejs, EjsVar *vp, int slotNum, struct EjsType *propType, int attributes);

} EjsTypeHelpers;


typedef EjsVar  *(*EjsCreateVarHelper)(Ejs *ejs, EjsType *type, int size);
typedef void    (*EjsDestroyVarHelper)(Ejs *ejs, EjsVar *vp);
typedef EjsVar  *(*EjsCastVarHelper)(Ejs *ejs, EjsVar *vp, EjsType *type);
typedef EjsVar  *(*EjsCloneVarHelper)(Ejs *ejs, EjsVar *vp, bool deep);
typedef int     (*EjsDefinePropertyHelper)(Ejs *ejs, EjsVar *vp, int slotNum, EjsName *qname, EjsType *propType, 
                    int attributes, EjsVar *value);
typedef int     (*EjsDeletePropertyHelper)(Ejs *ejs, EjsVar *vp, int slotNum);
typedef int     (*EjsDeletePropertyByNameHelper)(Ejs *ejs, EjsVar *vp, EjsName *qname);
typedef int     (*EjsFinalizeVarHelper)(Ejs *ejs, EjsVar *vp);
typedef EjsVar  *(*EjsGetPropertyHelper)(Ejs *ejs, EjsVar *vp, int slotNum);
typedef EjsVar  *(*EjsGetPropertyByNameHelper)(Ejs *ejs, EjsVar *vp, EjsName *qname);
typedef int     (*EjsGetPropertyCountHelper)(Ejs *ejs, EjsVar *vp);
typedef EjsName (*EjsGetPropertyNameHelper)(Ejs *ejs, EjsVar *vp, int slotNum);
typedef struct EjsTrait *(*EjsGetPropertyTraitHelper)(Ejs *ejs, EjsVar *vp, int slotNum);
typedef int     (*EjsLookupPropertyHelper)(Ejs *ejs, EjsVar *vp, EjsName *qname);
typedef EjsVar  *(*EjsInvokeOperatorHelper)(Ejs *ejs, EjsVar *vp, int opCode, EjsVar *rhs);
typedef void    (*EjsMarkVarHelper)(Ejs *ejs, EjsVar *parent, EjsVar *vp);
typedef int     (*EjsSetPropertyByNameHelper)(Ejs *ejs, EjsVar *vp, EjsName *qname, EjsVar *value);
typedef int     (*EjsSetPropertyHelper)(Ejs *ejs, EjsVar *vp, int slotNum, EjsVar *value);
typedef int     (*EjsSetPropertyNameHelper)(Ejs *ejs, EjsVar *vp, int slotNum, EjsName *qname);
typedef int     (*EjsSetPropertyTraitHelper)(Ejs *ejs, EjsVar *vp, int slotNum, EjsType *propType, int attributes);

/******************************** Private Prototypes **********************************/
/*
 *  Internal initialization routines
 */
extern void     ejsCreateAppType(Ejs *ejs);
extern void     ejsCreateArrayType(Ejs *ejs);
extern void     ejsCreateDebugType(Ejs *ejs);
extern void     ejsCreateBlockType(Ejs *ejs);
extern void     ejsCreateBooleanType(Ejs *ejs);
extern void     ejsCreateByteArrayType(Ejs *ejs);
extern void     ejsCreateConfigType(Ejs *ejs);
extern void     ejsCreateDateType(Ejs *ejs);
extern void     ejsCreateDbTypes(Ejs *ejs);
extern void     ejsCreateErrorType(Ejs *ejs);
extern void     ejsCreateFunctionType(Ejs *ejs);
extern void     ejsCreateGCType(Ejs *ejs);
extern void     ejsCreateGlobalBlock(Ejs *ejs);
extern int      ejsCreateFileType(Ejs *ejs);
extern void     ejsCreateFunctionType(Ejs *ejs);
extern void     ejsCreateIteratorType(Ejs *ejs);
extern void     ejsCreateLoggerType(Ejs *ejs);
extern int      ejsCreateHttpType(Ejs *ejs);
extern void     ejsCreateMemoryType(Ejs *ejs);
extern void     ejsCreateNamespaceType(Ejs *ejs);
extern void     ejsCreateNumberType(Ejs *ejs);
extern void     ejsCreateNullType(Ejs *ejs);
extern void     ejsCreateObjectType(Ejs *ejs);
extern void     ejsCreateReflectType(Ejs *ejs);
extern void     ejsCreateRegExpType(Ejs *ejs);
extern void     ejsCreateStringType(Ejs *ejs);
extern void     ejsCreateSystemType(Ejs *ejs);
extern void     ejsCreateTypeType(Ejs *ejs);
extern void     ejsCreateVoidType(Ejs *ejs);
extern void     ejsCreateTimerType(Ejs *ejs);
extern void     ejsCreateTypes(Ejs *ejs);
extern int      ejsCreateXMLType(Ejs *ejs);
extern int      ejsCreateXMLListType(Ejs *ejs);

/*
 *  Core type configuration
 */
extern void     ejsConfigureAppType(Ejs *ejs);
extern void     ejsConfigureArrayType(Ejs *ejs);
extern void     ejsConfigureBlockType(Ejs *ejs);
extern void     ejsConfigureBooleanType(Ejs *ejs);
extern void     ejsConfigureByteArrayType(Ejs *ejs);
extern void     ejsConfigureConfigType(Ejs *ejs);
extern void     ejsConfigureDateType(Ejs *ejs);
extern void     ejsConfigureDbTypes(Ejs *ejs);
extern void     ejsConfigureDebugType(Ejs *ejs);
extern void     ejsConfigureErrorType(Ejs *ejs);
extern void     ejsConfigureGCType(Ejs *ejs);
extern void     ejsConfigureGlobalBlock(Ejs *ejs);
extern int      ejsConfigureFileType(Ejs *ejs);
extern void     ejsConfigureFunctionType(Ejs *ejs);
extern int      ejsConfigureHttpType(Ejs *ejs);
extern void     ejsConfigureIteratorType(Ejs *ejs);
extern void     ejsConfigureLoggerType(Ejs *ejs);
extern void     ejsConfigureFunctionType(Ejs *ejs);
extern void     ejsConfigureNamespaceType(Ejs *ejs);
extern void     ejsConfigureMemoryType(Ejs *ejs);
extern void     ejsConfigureNumberType(Ejs *ejs);
extern void     ejsConfigureNullType(Ejs *ejs);
extern void     ejsConfigureObjectType(Ejs *ejs);
extern void     ejsConfigureReflectType(Ejs *ejs);
extern void     ejsConfigureRegExpType(Ejs *ejs);
extern void     ejsConfigureStringType(Ejs *ejs);
extern void     ejsConfigureSystemType(Ejs *ejs);
extern void     ejsConfigureTimerType(Ejs *ejs);
extern void     ejsConfigureTypeType(Ejs *ejs);
extern void     ejsConfigureTypes(Ejs *ejs);
extern void     ejsConfigureVoidType(Ejs *ejs);
extern int      ejsConfigureXMLType(Ejs *ejs);
extern int      ejsConfigureXMLListType(Ejs *ejs);

extern int      ejsAddNativeModule(Ejs *ejs, char *name, EjsNativeCallback callback);
extern void     ejsCreateCoreNamespaces(Ejs *ejs);
extern int      ejsCopyCoreTypes(Ejs *ejs);
extern int      ejsDefineCoreTypes(Ejs *ejs);
extern int      ejsDefineErrorTypes(Ejs *ejs);
extern int      ejsGetAvailableData(EjsByteArray *ap);
extern void     ejsInheritBaseClassNamespaces(Ejs *ejs, struct EjsType *type, struct EjsType *baseType);
extern void     ejsInitializeDefaultHelpers(struct EjsTypeHelpers *helpers);
extern void     ejsSetDbMemoryContext(MprThreadLocal *tls, MprCtx ctx);

#ifdef __cplusplus
}
#endif
#endif /* _h_EJS_CORE */

/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/*
 *  ejsModule.h - Module file format definition.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#ifndef _h_EJS_MODULE
#define _h_EJS_MODULE 1


#ifdef __cplusplus
extern "C" {
#endif

/********************************* Prototypes *********************************/
/*
 *  Module File Format and Layout:
 *
 *      Notes:
 *          - (N) Numbers are 1-3 little-endian encoded bytes using the 0x80
 *            as the continuation character
 *          - (S) Strings are pointers into the constant pool encoded as
 *            number offsets.
 *          - (T) Types are encoded and ored with the type encoding masks
 *            below. Types are either: untyped, unresolved or primitive (builtin).
 *            The relevant mask is ored into the lower 2 bits. Slot numbers and
 *            name tokens are shifted up 2 bits. Zero means untyped.
 *
 *      Notes:
 *          - Strings are UTF-8 and are always null terminated.
 */

/*
 *  Type encoding masks
 */
#define EJS_ENCODE_GLOBAL_NOREF         0x0
#define EJS_ENCODE_GLOBAL_NAME          0x1
#define EJS_ENCODE_GLOBAL_SLOT          0x2
#define EJS_ENCODE_GLOBAL_MASK          0x3

/*
 *  Fixup kinds
 */
#define EJS_FIXUP_BASE_TYPE             1
#define EJS_FIXUP_INTERFACE_TYPE        2
#define EJS_FIXUP_RETURN_TYPE           3
#define EJS_FIXUP_TYPE_PROPERTY         4
#define EJS_FIXUP_INSTANCE_PROPERTY     5
#define EJS_FIXUP_LOCAL                 6
#define EJS_FIXUP_EXCEPTION             7

typedef struct EjsTypeFixup
{
    int                 kind;                       /* Kind of fixup */
#if UNUSED
    union {
        EjsVar          *target;                    /* Target to fixup */
        EjsType         *targetType;
        EjsFunction     *targetFunction;
        EjsEx           *targetException;
    };
#else
    EjsVar              *target;                    /* Target to fixup */
#endif
    int                 slotNum;                    /* Slot of target */
    EjsName             typeName;                   /* Type name */
    int                 typeSlotNum;                /* Type slot number */
} EjsTypeFixup;


/*
 *  Module File Header
 *
 *      |---------------|---------------|---------------|---------------|
 *      | Magic Number (2)              | Major Version | Minor Version |
 *      |---------------|---------------|---------------|---------------|
 *      | Flags (1)     | filler (1)    |               |               |
 *      |---------------|---------------|---------------|---------------|
 *
 *  TODO - refactor and put slot first after Section(1)
 *
 *  Module Section (includes the string constant Pool)
 *
 *      |---------------|---------------|---------------|---------------|
 *      | Section (1)   | Name (S)      | Url (S)       | initSlot (N)  |
 *      |---------------|---------------|---------------|---------------|
 *      | Pool len (N)  | Pool Strings  |
 *      |---------------|---------------|---------------|---------------|
 *
 *      Notes:
 *      - Strings are null terminated
 *
 *      TODO
 *      - Java is optimized for string indexes
 *      - C is optimized for string pointers
 *      - Should we include string lengths in the constant pool
 *
 *
 *  Dependencies Section
 *      |---------------|---------------|---------------|
 *      | Section (1)   | Module Name String (S)        |
 *      |---------------|---------------|---------------|
 *      | Module Name URL (S)           |               |
 *      |---------------|---------------|---------------|
 *
 *
 *
 *  Type Definition Section
 *      |---------------|---------------|---------------|---------------|
 *      | Section (1)   | Type Name (S) | Namespace (S) | Attributes (N)|
 *      |---------------|---------------|---------------|---------------|
 *      | Prop slot (N) | Base type (T) | Num Static Props (N)          |
 *      |---------------|---------------|---------------|---------------|
 *      | Num Instance Props (N)        | Num Interface (N)             |
 *      |---------------|---------------|---------------|---------------|
 *      | Interface (T) ....                                            |
 *      |---------------|---------------|---------------|---------------|
 *
 *      Attributes:
 *          EJS_ATTR_DYNAMIC (TODO - more attributes are actually set)
 *
 *      Notes:
 *          - Prop slot not used for global types unless --out used
 *          - See below for attribute definitions
 *          - Type definition may appear where a property definition is
 *            expected. In that case, it is created as a property of the
 *            preceeding type. Otherwise, the type is global.
 *          - (S) token references point into the costant pool.
 *          - If base type slot is provided (non-zero), then base type name
 *            will be zero and must be ignored.
 *          - Expect to see up to N sections following this record where
 *              N == sum of num static, methods, inherited and instance
 *              property counts.
 *
 *  Property Definition Section
 *      |---------------|---------------|---------------|---------------|
 *      | Section (1)   | Name (S)      | Namespace (S) | Attributes (N)|
 *      |---------------|---------------|---------------|---------------|
 *      | Prop slot (N) | Type (T)      |               |               |
 *      |---------------|---------------|---------------|---------------|
 *
 *      Attributes:
 *          See property attributes below
 *
 *      Notes:
 *          - All types are always defined in the global object. Types must
 *              be predefined before properties that use them.
 *          - Only generate property definition sections if the object is
 *              "indexed", ie. accessed as obj["name"].
 *          - Name tokens refer to the type constant pool
 */

/*  TODO -- need to have default value for each arg...
 *      But that could be any time including a structural type
 *  TODO - the format of each section time is not very consistent (See prop slot). Refactor
 *  TODO - whan attributes are rationalized. Merge lang into attributes?
 */

/*
 *      Function Code Definition Section
 *      |---------------|---------------|---------------|---------------|
 *      | Section (1)   | Name (S)      | Namespace (S) | NextSlot (N)  |
 *      |---------------|---------------|---------------|---------------|
 *      | Attributes (N)| Lang (1)      | Ret Type (T)  | Prop Slot (N) | 
 *      |---------------|---------------|---------------|---------------|
 *      | Arg Count (N) | Local Count(N)| Excpt Count(N)| Code Len (N)  | 
 *      |---------------|---------------|---------------|---------------|
 *      | Code ...      |               |               |               |
 *      |---------------|---------------|---------------|---------------|
 *
 *      Properties, nested functions, blocks an exception sections follow.
 *
 *      Attributes:
 *          See property attributes below
 *
 *      Exception Section
 *      |---------------|---------------|---------------|---------------|
 *      | Section (1)   | Flags (1)     | Try block start offset (N)    |
 *      |---------------|---------------|---------------|---------------|
 *      | Try block end offset (N)      | Handler block start offset (N)|
 *      |---------------|---------------|---------------|---------------|
 *      | Handler block end offset (N)  | Catch type (N)                |
 *      |---------------|---------------|---------------|---------------|
 *
 *
 *  TODO CHECK
 *
 *  Property Attributes (bits) (low order bits first)
 *      |---------------|---------------|---------------|---------------|
 *      | Visibility (2)| Hidden (1)    | Final (1)     | Readonly (1)  |
 *      |---------------|---------------|---------------|---------------|
 *      | Permanent (1) | Const (1)     | Fixed type (1)| Virtual (1)   |
 *      |---------------|---------------|---------------|---------------|
 *
 *          - public (0), internal (1), protected (2), private (3)
 *          - dynamic
 *          - enumerable
 *          - const
 *          - (virtual)
 *
 *  Block Definition Section
 *      |---------------|---------------|---------------|---------------|
 *      | Section (1)   | Block Name (S)| Slot (N)      | Prop Count (N)|
 *      |---------------|---------------|---------------|---------------|
 *
 *
 *  Documentation Section (may preceed any Type, Function or Property section)
 *      |---------------|---------------|---------------|---------------|
 *      | Section (1)   | Text (S)      |               |               |
 *      |---------------|---------------|---------------|---------------|
 *
 *  Module file names: a.b.c.d, a.b.c.*;
 *  Search strategy:
 *      1. File named a.b.c
 *      2. File named a/b/c
 *      3. File named a.b.c in EJSPATH
 *      4. File named a/b/c in EJSPATH
 *      5. File named c in EJSPATH
 *
 *  Default EJSPath "{INSDIR}/classes:."
 */

#define EJS_MODULE_MAGIC            0xC7DA


/*
 *  Module file format version
 *  TODO - rename EJS_MODULE_MAJOR
 */
#define EJS_MAJOR               1
#define EJS_MINOR               4
#define EJS_MODULE_VERSION      (EJS_MAJOR << 8 | EJS_MINOR)


/*
 *  Section types
 */
#define EJS_SECT_MODULE         1           /* Module section */
#define EJS_SECT_MODULE_END     2           /* End of a module */
#define EJS_SECT_DEPENDENCY     3           /* Module dependency */
#define EJS_SECT_CLASS          4           /* Class definition */
#define EJS_SECT_CLASS_END      5           /* End of class definition */
#define EJS_SECT_FUNCTION       6           /* Function */
#define EJS_SECT_FUNCTION_END   7           /* End of function definition */
#define EJS_SECT_BLOCK          8           /* Nested block */
#define EJS_SECT_BLOCK_END      9           /* End of Nested block */
#define EJS_SECT_PROPERTY       10          /* Property (variable) definition */
#define EJS_SECT_EXCEPTION      11          /* Exception definition */
#define EJS_SECT_DOC            12          /* Documentation for an element */
#define EJS_SECT_MAX            13

/*
 *  Psudo section types for loader callback
 */
#define EJS_SECT_START          (EJS_SECT_MAX + 1)
#define EJS_SECT_END            (EJS_SECT_MAX + 2)


/*
 *  Align headers on a 4 byte boundary
 */
#define EJS_HDR_ALIGN           4


/*
 *  Module header flags
 */
#define EJS_MODULE_BOUND_GLOBALS 0x1        /* Module global slots are fully bound */
#define EJS_MODULE_INTERP_EMPTY 0x2         /* Module compiled with --empty. ie. not using native elements */


/*
 *  Module loader flags
 */
#define EJS_MODULE_DONT_INIT    0x1         /* Don't run initializers */
#define EJS_MODULE_BUILTIN      0x2         /* Loading builtins */


/*
 *  File format is little-endian. All headers are aligned on word boundaries
 */
typedef struct EjsModuleHdr {
    ushort      magic;                      /* Magic number for Ejscript modules */
    uchar       major;                      /* Module format major version number */
    uchar       minor;                      /* Module format minor version number */
    uchar       flags;                      /* Module flags */
    uchar       filler;                     /* Reserved */
#if UNUSED
    int         seq;                        /* Sequence number to match native code slots with module file */
#endif
    int         version;                    /* Reserved for an application module version number */
} EjsModuleHdr;


/*
 *  Structure for the string constant pool
 */
typedef struct EjsConst {
    char        *pool;                      /* Constant pool storage */
    int         size;                       /* Size of constant pool storage */
    int         len;                        /* Length of active constant pool */
    int         base;                       /* Base used during relocations */
    int         locked;                     /* No more additions allowed */
    MprHashTable *table;                    /* Hash table for fast lookup */
} EjsConst;



/*
 *  Type for globalProperties list which keeps a list of global properties for this module.
 */
//  TODO - get rid of this and just use EjsName
typedef struct EcModuleProp {
    EjsName     qname;
} EcModuleProp;


/*
 *  Module type
 */
typedef struct EjsModule {
    char            *name;                  /* Name of this module */
    char            *url;                   /* Loading URL */
    int             version;                /* Module file version number */
    char            *path;                  /* Module file path name */
    MprList         *dependencies;          /* Module file dependencies*/

    /*
     *  Used during code generation
     */
    struct EcCodeGen *code;                 /* Code generation buffer */
    MprFile         *file;                  /* File handle */
    MprList         *globalProperties;      /* List of global properties */
    EjsFunction     *initializer;           /* Initializer method */

    /*
     *  Used only while loading modules
     */
    EjsBlock        *scopeChain;            /* Scope of nested types/functions/blocks, being loaded */
    EjsConst        *constants;             /* Constant pool */
    int             nameToken;              /* TODO ?? what */
    int             firstGlobalSlot;        /* First global slot (if used) */
    struct EjsFunction  *currentMethod;     /* Current method being loaded */

    uint            boundGlobals    : 1;    /* Global slots are bound by compiler */
    uint            compiling       : 1;    /* Module currently being compiled from source */
    uint            loaded          : 1;    /* Module has been loaded from an external file */
    uint            nativeLoaded    : 1;    /* Backing shared library loaded */
    uint            configured      : 1;    /* Module is already configured */
    uint            hasNative       : 1;    /* Has native property definitions */
    uint            hasInitializer  : 1;    /* Has initializer function */
    uint            initialized     : 1;    /* Initializer has run */
    uint            hasError        : 1;    /* Module has a loader error */

    int             flags;                  /* Loading flags */
#if UNUSED
    int             seq;                    /* Generation sequence number to match native slots with mod file */
#endif

#if BLD_FEATURE_EJS_DOC
    char            *doc;                   /* Current doc string */
#endif

} EjsModule;


/*
 *  Documentation string information
 */
#if BLD_FEATURE_EJS_DOC

/*
 *  Element documentation string. The loader will create if required.
 */
typedef struct EjsDoc {
    char        *docString;                         /* Original doc string */
    char        *brief;
    char        *description;
    char        *example;
    char        *requires;
    char        *returns;
    char        *stability;                         /* prototype, evolving, stable, mature, deprecated */
    char        *spec;                              /* Where specified */
    bool        hide;                               /* Hide doc if true */

    MprList     *defaults;                          /* Parameter default values */
    MprList     *params;                            /* Function parameters */
    MprList     *options;                           /* Option parameter values */
    MprList     *see;
    MprList     *throws;

    EjsTrait    *trait;                             /* Back pointer to trait */
} EjsDoc;
#endif


typedef void (*EjsLoaderCallback)(struct Ejs *ejs, int kind, ...);


/******************************** Prototypes **********************************/
//DDD
extern EjsModule    *ejsCreateModule(struct Ejs *ejs, cchar *name, cchar *uri, cchar *pool, int poolSize);
//DDD
extern MprList      *ejsLoadModule(struct Ejs *ejs, cchar *name, cchar *uri, EjsLoaderCallback callback, int flags);
extern int          ejsSearch(Ejs *ejs, char **path, cchar *name);

extern int          ejsModuleReadName(struct Ejs *ejs, MprFile *file, char **name, int len);
extern int          ejsModuleReadNumber(struct Ejs *ejs, EjsModule *module, int *number);
extern int          ejsModuleReadByte(struct Ejs *ejs, EjsModule *module, int *number);
extern char         *ejsModuleReadString(struct Ejs *ejs, EjsModule *module);
extern int          ejsModuleReadType(struct Ejs *ejs, EjsModule *module, EjsType **typeRef, EjsTypeFixup **fixup, 
                        EjsName *typeName, int *slotNum);

#if BLD_FEATURE_EJS_DOC
extern char         *ejsGetDocKey(struct Ejs *ejs, EjsBlock *block, int slotNum, char *buf, int bufsize);
extern EjsDoc       *ejsCreateDoc(struct Ejs *ejs, EjsBlock *block, int slotNum, cchar *docString);
#endif

#ifdef __cplusplus
}
#endif
#endif /* _h_EJS_MODULE */

/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/*
 *  ejs.h - Ejscript top level header.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#ifndef _h_EJS_h
#define _h_EJS_h 1

#include    "mpr.h"

/********************************** Defines ***********************************/

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#endif /* _h_EJS_h */

/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/**
 *  ejsWeb.h -- Header for the Ejscript Web Framework
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#ifndef _h_EJS_WEB_h
#define _h_EJS_WEB_h 1

#if BLD_FEATURE_EJS_WEB
/*
 *  Indent to protect from genDepend first time.
 */

/*********************************** Defines **********************************/

#if BLD_APPWEB_PRODUCT
    #define EJS_EJSWEB          "ajsweb"    /* Ejscript framework generator */
#else
    #define EJS_EJSWEB          "ejsweb"    /* Ejscript framework generator */
#endif

#define EJS_EJSWEB_EXE          EJS_EJSWEB BLD_EXE
#define EJS_WEB_EXT             ".ejs"      /* Web page extension */
#define EJS_SESSION             "-ejs-session-"
#define EJS_SERVER_NAME         "Embedthis-Ejscript/" BLD_VERSION

#define EJS_MAX_HEADERS         2048        /* Max size of the headers */

/*
 *  Var collections
 */
#define EJS_WEB_HOST_VAR        3           /* Fields of the Host object */
#define EJS_WEB_REQUEST_VAR     1           /* Fields of the Request object */
#define EJS_WEB_RESPONSE_VAR    2           /* Fields of the Response object */

/*********************************** Types ************************************/
/*
 *  Service control block. This defines the function callbacks for a web server module to implement.
 *  Aall these functions as required to interact with the web server.
 */
typedef struct EjsWebControl {
    EjsService  *service;                   /* EJS service */
    Ejs         *master;                    /* Master interpreter */
    EjsVar      *applications;              /* Application cache */
    EjsObject   *sessions;                  /* Session cache */
    cchar       *modulePath;                /* Path to the ejs web server module and handler */
    int         sessionTimeout;             /* Default session timeout */
    int         nextSession;                /* Session ID counter */

    void        (*defineParams)(void *handle);
    void        (*discardOutput)(void *handle);
    void        (*error)(void *handle, int code, cchar *fmt, ...);
    cchar       *(*getHeader)(void *handle, cchar *key);
    EjsVar      *(*getVar)(void *handle, int collection, int field);
    void        (*redirect)(void *handle, int code, cchar *url);
    void        (*setCookie)(void *handle, cchar *name, cchar *value, int lifetime, cchar *path, bool secure);
    void        (*setHeader)(void *handle, bool allowMultiple, cchar *key, cchar *fmt, ...);
    void        (*setHttpCode)(void *handle, int code);
    void        (*setMimeType)(void *handle, cchar *mimeType);
    int         (*setVar)(void *handle, int collection, int field, EjsVar *value);
    int         (*write)(void *handle, cchar *buf, int size);

#if BLD_FEATURE_MULTITHREAD
    void        (*lock)(void *lockData);
    void        (*unlock)(void *lockData);
    void        *lockData;
#endif

} EjsWebControl;


/*
 *  Flags for EjsWeb.flags
 *  TODO - remove _FLAG
 */
#define EJS_WEB_FLAG_BROWSER_ERRORS      0x1    /* Send errors to the browser (typical dev use) */
#define EJS_WEB_FLAG_SESSION             0x2    /* Auto create sessions for each request */
#define EJS_WEB_FLAG_APP                 0x4    /* Request for content inside an Ejscript Application*/
#define EJS_WEB_FLAG_SOLO                0x8    /* Solo ejs file */

/*
 *  Per request control block
 */
typedef struct EjsWeb {
    Ejs             *ejs;           /* Ejscript interpreter handle */
    cchar           *appDir;        /* Directory containing the application */
    cchar           *appUrl;        /* Base url for the application. No trailing "/" */
    void            *handle;        /* Web server connection/request handle */
    EjsWebControl   *control;       /* Pointer to control block */
    cchar           *url;           /* App relative url: /controller/action. Starts with "/"  */
    int             flags;          /* Control flags */

    char            *controllerFile;/* Simple controller file name without path */
    EjsName         controllerName; /* Qualified Controller name (with "Controller" suffix) */
    EjsName         doActionName;   /* Qualified do action function name */
    char            *viewName;      /* Name of the view function */

    EjsVar          *params;        /* Form variables */
    EjsVar          *cookies;       /* Parsed cookies cache */
    struct EjsWebSession *session;  /* Current session */

    char            *error;         /* Error message */
    char            *cookie;        /* Cookie header */
    EjsType         *appType;       /* Application type instance */
    EjsName         appName;        /* Qualified Application name */

    EjsType         *controllerType;/* Controller type instance */
    EjsVar          *controller;    /* Controller instance to run */
    EjsVar          *doAction;      /* doAction() function to run. May be renderView() for Stand-Alone views. */

} EjsWeb;


/*
 *  Parse context
 */
typedef struct EjsWebParse {
    char    *inBuf;                 /* Input data to parse */
    char    *inp;                   /* Next character for input */
    char    *endp;                  /* End of storage (allow for null) */
    char    *tokp;                  /* Pointer to current parsed token */
    char    *token;                 /* Storage buffer for token */
    int     tokLen;                 /* Length of buffer */
} EjsWebParse;


/*
 *  Host class
 */
typedef struct EjsWebHost
{
    EjsVar      var;
    void        *handle;
} EjsWebHost;


/*
 *  Request class
 */
typedef struct EjsWebRequest
{
    EjsVar      var;
    void        *handle;
} EjsWebRequest;



/*
 *  Response class
 */
typedef struct EjsWebResponse
{
    EjsVar      var;
    void        *handle;
} EjsWebResponse;


typedef struct EjsWebSession
{
    EjsObject   obj;
    MprTime     expire;                     /* When the session should expire */
    cchar       *id;                        /* Session ID */
    int         timeout;                    /* Session inactivity lifespan */
} EjsWebSession;


/******************************* Internal APIs ********************************/

extern void         ejsConfigureWebTypes(Ejs *ejs);
extern int          ejsOpenWebFramework(EjsWebControl *control, bool useMaster);

//DDD
extern Ejs          *ejsGetMaster(MprCtx ctx);

extern void         ejsConfigureWebRequestType(Ejs *ejs);
extern void         ejsConfigureWebResponseType(Ejs *ejs);
extern void         ejsConfigureWebHostType(Ejs *ejs);
extern void         ejsConfigureWebControllerType(Ejs *ejs);
extern void         ejsConfigureWebSessionType(Ejs *ejs);

//DDD
extern EjsWeb       *ejsCreateWebRequest(MprCtx ctx, EjsWebControl *control, void *req, cchar *scriptName, cchar *uri,
                        cchar *dir, int flags);
extern int          ejsRunWebRequest(EjsWeb *web);
//DDD
extern EjsWebRequest *ejsCreateWebRequestObject(Ejs *ejs, void *handle);
//DDD
extern EjsWebHost   *ejsCreateWebHostObject(Ejs *ejs, void *handle);
//DDD
extern EjsWebResponse *ejsCreateWebResponseObject(Ejs *ejs, void *handle);
//DDD
extern EjsWebSession *ejsCreateWebSessionObject(Ejs *ejs, void *handle);

extern void         ejsDefineWebParam(Ejs *ejs, cchar *key, cchar *value);

//DDD
extern int          ejsLoadView(Ejs *ejs);
//DDD
extern void         ejsParseWebSessionCookie(EjsWeb *web);

/******************************** Published API *******************************/
#ifdef  __cplusplus
extern "C" {
#endif

//DDD -- All these
extern EjsWebSession *ejsCreateSession(Ejs *ejs, int timeout, bool secure);
extern bool         ejsDestroySession(Ejs *ejs);
extern void         ejsDefineParams(Ejs *ejs);
extern void         ejsDiscardOutput(Ejs *ejs);
extern EjsVar       *ejsCreateCookies(Ejs *ejs);
extern cchar        *ejsGetHeader(Ejs *ejs, cchar *key);
extern EjsVar       *ejsGetWebVar(Ejs *ejs, int collection, int field);
extern int          ejsMapToStorage(Ejs *ejs, char *path, int pathsize, cchar *uri);
extern int          ejsReadFile(Ejs *ejs, char **buf, int *len, cchar *path);
extern void         ejsRedirect(Ejs *ejs, int code, cchar *url);
extern void         ejsSetCookie(Ejs *ejs, cchar *name, cchar *value, int lifetime, cchar *path, bool secure);
extern void         ejsSetWebHeader(Ejs *ejs, bool allowMultiple, cchar *key, cchar *fmt, ...);
extern void         ejsSetHttpCode(Ejs *ejs, int code);
extern void         ejsSetMimeType(Ejs *ejs, int code);
extern int          ejsSetWebVar(Ejs *ejs, int collection, int field, EjsVar *value);
extern void         ejsWebError(Ejs *ejs, int code, cchar *fmt, ...);
extern int          ejsWriteBlock(Ejs *ejs, cchar *buf, int size);
extern int          ejsWriteString(Ejs *ejs, cchar *buf);
extern int          ejsWrite(Ejs *ejs, cchar *fmt, ...);

#ifdef  __cplusplus
}
#endif
#endif /* BLD_FEATURE_EJS_WEB */
#endif /* _h_EJS_WEB_h */

/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
#include "ejs.slots.h"
