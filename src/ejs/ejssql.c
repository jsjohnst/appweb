/*
 *  sqlite3.h -- SQLite header. Modified for embedding in Ejscript
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************** Includes **********************************/

#include "buildConfig.h"

#ifndef _h_SQLITE3_
#define _h_SQLITE3_ 1

/*********************************** Defines **********************************/
/*
 *  Sqlite configuration for use by Embedthis Ejscript. This must match the configuration in sqlite3.c.
 */
#define EJSCRIPT            1
#define PACKAGE             "sqlite" 
#define VERSION             "3.5.9"
#define PACKAGE_TARNAME     PACKAGE
#define PACKAGE_VERSION     VERSION
#define PACKAGE_STRING      "sqlite" VERSION
#define PACKAGE_BUGREPORT   "http://www.embedthis.com"

#define SQLITE_THREADSAFE   BLD_FEATURE_MULTITHREAD 
#define SQLITE_OMIT_BUILTIN_TEST        1
#define SQLITE_OMIT_BUILTIN_EXPLAIN     1
#define SQLITE_OMIT_LOAD_EXTENSION      1

#if MACOSX || LINUX || SOLARIS || VXWORKS
#define STDC_HEADERS        1
#define HAVE_SYS_TYPES_H    1
#define HAVE_SYS_STAT_H     1
#define HAVE_STDLIB_H       1
#define HAVE_STRING_H       1
#define HAVE_MEMORY_H       1
#define HAVE_STRINGS_H      1
#define HAVE_INTTYPES_H     1
#define HAVE_STDINT_H       1
#define HAVE_UNISTD_H       1
#define HAVE_DLFCN_H        1
#define HAVE_USLEEP         1
#define HAVE_LOCALTIME_R    1
#define HAVE_GMTIME_R       1
#define HAVE_READLINE       0 
#else
#define HAVE_STDLIB_H       1
#define HAVE_STRING_H       1
#define HAVE_STRINGS_H      1
#define HAVE_UNISTD_H       1
#endif

#if VXWORKS
#define NO_GETOD
#endif

#if WIN
/*
 *  Force winsock2 rather than winsock
 */
#include    <winsock2.h>
#endif


/*
 *  This is the start of the Official SQLite source code
 */
/*
** 2001 September 15
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This header file defines the interface that the SQLite library
** presents to client programs.  If a C-function, structure, datatype,
** or constant definition does not appear in this file, then it is
** not a published API of SQLite, is subject to change without
** notice, and should not be referenced by programs that use SQLite.
**
** Some of the definitions that are in this file are marked as
** "experimental".  Experimental interfaces are normally new
** features recently added to SQLite.  We do not anticipate changes 
** to experimental interfaces but reserve to make minor changes if
** experience from use "in the wild" suggest such changes are prudent.
**
** The official C-language API documentation for SQLite is derived
** from comments in this file.  This file is the authoritative source
** on how SQLite interfaces are suppose to operate.
**
** The name of this file under configuration management is "sqlite.h.in".
** The makefile makes some minor changes to this file (such as inserting
** the version number) and changes its name to "sqlite3.h" as
** part of the build process.
**
** @(#) $Id: sqlite.h.in,v 1.312 2008/05/12 12:39:56 drh Exp $
*/
#ifndef _SQLITE3_H_
#define _SQLITE3_H_
#include <stdarg.h>     /* Needed for the definition of va_list */

/*
** Make sure we can call this stuff from C++.
*/
#ifdef __cplusplus
extern "C" {
#endif

/*
** Add the ability to override 'extern'
*/
#ifndef SQLITE_EXTERN
# define SQLITE_EXTERN extern
#endif

/*
** Make sure these symbols where not defined by some previous header
** file.
*/
#ifdef SQLITE_VERSION
# undef SQLITE_VERSION
#endif
#ifdef SQLITE_VERSION_NUMBER
# undef SQLITE_VERSION_NUMBER
#endif

/*
** CAPI3REF: Compile-Time Library Version Numbers {F10010}
**
** The SQLITE_VERSION and SQLITE_VERSION_NUMBER #defines in
** the sqlite3.h file specify the version of SQLite with which
** that header file is associated.
**
** The "version" of SQLite is a string of the form "X.Y.Z".
** The phrase "alpha" or "beta" might be appended after the Z.
** The X value is major version number always 3 in SQLite3.
** The X value only changes when  backwards compatibility is
** broken and we intend to never break
** backwards compatibility.  The Y value is the minor version
** number and only changes when
** there are major feature enhancements that are forwards compatible
** but not backwards compatible.  The Z value is release number
** and is incremented with
** each release but resets back to 0 when Y is incremented.
**
** See also: [sqlite3_libversion()] and [sqlite3_libversion_number()].
**
** INVARIANTS:
**
** {F10011} The SQLITE_VERSION #define in the sqlite3.h header file
**          evaluates to a string literal that is the SQLite version
**          with which the header file is associated.
**
** {F10014} The SQLITE_VERSION_NUMBER #define resolves to an integer
**          with the value  (X*1000000 + Y*1000 + Z) where X, Y, and
**          Z are the major version, minor version, and release number.
*/
#define SQLITE_VERSION         "3.5.9"
#define SQLITE_VERSION_NUMBER  3005009

/*
** CAPI3REF: Run-Time Library Version Numbers {F10020}
** KEYWORDS: sqlite3_version
**
** These features provide the same information as the [SQLITE_VERSION]
** and [SQLITE_VERSION_NUMBER] #defines in the header, but are associated
** with the library instead of the header file.  Cautious programmers might
** include a check in their application to verify that 
** sqlite3_libversion_number() always returns the value 
** [SQLITE_VERSION_NUMBER].
**
** The sqlite3_libversion() function returns the same information as is
** in the sqlite3_version[] string constant.  The function is provided
** for use in DLLs since DLL users usually do not have direct access to string
** constants within the DLL.
**
** INVARIANTS:
**
** {F10021} The [sqlite3_libversion_number()] interface returns an integer
**          equal to [SQLITE_VERSION_NUMBER]. 
**
** {F10022} The [sqlite3_version] string constant contains the text of the
**          [SQLITE_VERSION] string. 
**
** {F10023} The [sqlite3_libversion()] function returns
**          a pointer to the [sqlite3_version] string constant.
*/
SQLITE_EXTERN const char sqlite3_version[];
const char *sqlite3_libversion(void);
int sqlite3_libversion_number(void);

/*
** CAPI3REF: Test To See If The Library Is Threadsafe {F10100}
**
** SQLite can be compiled with or without mutexes.  When
** the SQLITE_THREADSAFE C preprocessor macro is true, mutexes
** are enabled and SQLite is threadsafe.  When that macro is false,
** the mutexes are omitted.  Without the mutexes, it is not safe
** to use SQLite from more than one thread.
**
** There is a measurable performance penalty for enabling mutexes.
** So if speed is of utmost importance, it makes sense to disable
** the mutexes.  But for maximum safety, mutexes should be enabled.
** The default behavior is for mutexes to be enabled.
**
** This interface can be used by a program to make sure that the
** version of SQLite that it is linking against was compiled with
** the desired setting of the SQLITE_THREADSAFE macro.
**
** INVARIANTS:
**
** {F10101} The [sqlite3_threadsafe()] function returns nonzero if
**          SQLite was compiled with its mutexes enabled or zero
**          if SQLite was compiled with mutexes disabled.
*/
int sqlite3_threadsafe(void);

/*
** CAPI3REF: Database Connection Handle {F12000}
** KEYWORDS: {database connection} {database connections}
**
** Each open SQLite database is represented by pointer to an instance of the
** opaque structure named "sqlite3".  It is useful to think of an sqlite3
** pointer as an object.  The [sqlite3_open()], [sqlite3_open16()], and
** [sqlite3_open_v2()] interfaces are its constructors
** and [sqlite3_close()] is its destructor.  There are many other interfaces
** (such as [sqlite3_prepare_v2()], [sqlite3_create_function()], and
** [sqlite3_busy_timeout()] to name but three) that are methods on this
** object.
*/
typedef struct sqlite3 sqlite3;


/*
** CAPI3REF: 64-Bit Integer Types {F10200}
** KEYWORDS: sqlite_int64 sqlite_uint64
**
** Because there is no cross-platform way to specify 64-bit integer types
** SQLite includes typedefs for 64-bit signed and unsigned integers.
**
** The sqlite3_int64 and sqlite3_uint64 are the preferred type
** definitions.  The sqlite_int64 and sqlite_uint64 types are
** supported for backwards compatibility only.
**
** INVARIANTS:
**
** {F10201} The [sqlite_int64] and [sqlite3_int64] types specify a
**          64-bit signed integer.
**
** {F10202} The [sqlite_uint64] and [sqlite3_uint64] types specify
**          a 64-bit unsigned integer.
*/
#ifdef SQLITE_INT64_TYPE
  typedef SQLITE_INT64_TYPE sqlite_int64;
  typedef unsigned SQLITE_INT64_TYPE sqlite_uint64;
#elif defined(_MSC_VER) || defined(__BORLANDC__)
  typedef __int64 sqlite_int64;
  typedef unsigned __int64 sqlite_uint64;
#else
  typedef long long int sqlite_int64;
  typedef unsigned long long int sqlite_uint64;
#endif
typedef sqlite_int64 sqlite3_int64;
typedef sqlite_uint64 sqlite3_uint64;

/*
** If compiling for a processor that lacks floating point support,
** substitute integer for floating-point
*/
#ifdef SQLITE_OMIT_FLOATING_POINT
# define double sqlite3_int64
#endif

/*
** CAPI3REF: Closing A Database Connection {F12010}
**
** This routine is the destructor for the [sqlite3] object.  
**
** Applications should [sqlite3_finalize | finalize] all
** [prepared statements] and
** [sqlite3_blob_close | close] all [sqlite3_blob | BLOBs] 
** associated with the [sqlite3] object prior
** to attempting to close the [sqlite3] object.
**
** <todo>What happens to pending transactions?  Are they
** rolled back, or abandoned?</todo>
**
** INVARIANTS:
**
** {F12011} The [sqlite3_close()] interface destroys an [sqlite3] object
**          allocated by a prior call to [sqlite3_open()],
**          [sqlite3_open16()], or [sqlite3_open_v2()].
**
** {F12012} The [sqlite3_close()] function releases all memory used by the
**          connection and closes all open files.
**
** {F12013} If the database connection contains
**          [prepared statements] that have not been
**          finalized by [sqlite3_finalize()], then [sqlite3_close()]
**          returns [SQLITE_BUSY] and leaves the connection open.
**
** {F12014} Giving sqlite3_close() a NULL pointer is a harmless no-op.
**
** LIMITATIONS:
**
** {U12015} The parameter to [sqlite3_close()] must be an [sqlite3] object
**          pointer previously obtained from [sqlite3_open()] or the 
**          equivalent, or NULL.
**
** {U12016} The parameter to [sqlite3_close()] must not have been previously
**          closed.
*/
int sqlite3_close(sqlite3 *);

/*
** The type for a callback function.
** This is legacy and deprecated.  It is included for historical
** compatibility and is not documented.
*/
typedef int (*sqlite3_callback)(void*,int,char**, char**);

/*
** CAPI3REF: One-Step Query Execution Interface {F12100}
**
** The sqlite3_exec() interface is a convenient way of running
** one or more SQL statements without a lot of C code.  The
** SQL statements are passed in as the second parameter to
** sqlite3_exec().  The statements are evaluated one by one
** until either an error or an interrupt is encountered or
** until they are all done.  The 3rd parameter is an optional
** callback that is invoked once for each row of any query results
** produced by the SQL statements.  The 5th parameter tells where
** to write any error messages.
**
** The sqlite3_exec() interface is implemented in terms of
** [sqlite3_prepare_v2()], [sqlite3_step()], and [sqlite3_finalize()].
** The sqlite3_exec() routine does nothing that cannot be done
** by [sqlite3_prepare_v2()], [sqlite3_step()], and [sqlite3_finalize()].
** The sqlite3_exec() is just a convenient wrapper.
**
** INVARIANTS:
** 
** {F12101} The [sqlite3_exec()] interface evaluates zero or more UTF-8
**          encoded, semicolon-separated, SQL statements in the
**          zero-terminated string of its 2nd parameter within the
**          context of the [sqlite3] object given in the 1st parameter.
**
** {F12104} The return value of [sqlite3_exec()] is SQLITE_OK if all
**          SQL statements run successfully.
**
** {F12105} The return value of [sqlite3_exec()] is an appropriate 
**          non-zero error code if any SQL statement fails.
**
** {F12107} If one or more of the SQL statements handed to [sqlite3_exec()]
**          return results and the 3rd parameter is not NULL, then
**          the callback function specified by the 3rd parameter is
**          invoked once for each row of result.
**
** {F12110} If the callback returns a non-zero value then [sqlite3_exec()]
**          will aborted the SQL statement it is currently evaluating,
**          skip all subsequent SQL statements, and return [SQLITE_ABORT].
**          <todo>What happens to *errmsg here?  Does the result code for
**          sqlite3_errcode() get set?</todo>
**
** {F12113} The [sqlite3_exec()] routine will pass its 4th parameter through
**          as the 1st parameter of the callback.
**
** {F12116} The [sqlite3_exec()] routine sets the 2nd parameter of its
**          callback to be the number of columns in the current row of
**          result.
**
** {F12119} The [sqlite3_exec()] routine sets the 3rd parameter of its 
**          callback to be an array of pointers to strings holding the
**          values for each column in the current result set row as
**          obtained from [sqlite3_column_text()].
**
** {F12122} The [sqlite3_exec()] routine sets the 4th parameter of its
**          callback to be an array of pointers to strings holding the
**          names of result columns as obtained from [sqlite3_column_name()].
**
** {F12125} If the 3rd parameter to [sqlite3_exec()] is NULL then
**          [sqlite3_exec()] never invokes a callback.  All query
**          results are silently discarded.
**
** {F12128} If an error occurs while parsing or evaluating any of the SQL
**          statements handed to [sqlite3_exec()] then [sqlite3_exec()] will
**          return an [error code] other than [SQLITE_OK].
**
** {F12131} If an error occurs while parsing or evaluating any of the SQL
**          handed to [sqlite3_exec()] and if the 5th parameter (errmsg)
**          to [sqlite3_exec()] is not NULL, then an error message is
**          allocated using the equivalent of [sqlite3_mprintf()] and
**          *errmsg is made to point to that message.
**
** {F12134} The [sqlite3_exec()] routine does not change the value of
**          *errmsg if errmsg is NULL or if there are no errors.
**
** {F12137} The [sqlite3_exec()] function sets the error code and message
**          accessible via [sqlite3_errcode()], [sqlite3_errmsg()], and
**          [sqlite3_errmsg16()].
**
** LIMITATIONS:
**
** {U12141} The first parameter to [sqlite3_exec()] must be an valid and open
**          [database connection].
**
** {U12142} The database connection must not be closed while
**          [sqlite3_exec()] is running.
** 
** {U12143} The calling function is should use [sqlite3_free()] to free
**          the memory that *errmsg is left pointing at once the error
**          message is no longer needed.
**
** {U12145} The SQL statement text in the 2nd parameter to [sqlite3_exec()]
**          must remain unchanged while [sqlite3_exec()] is running.
*/
int sqlite3_exec(
  sqlite3*,                                  /* An open database */
  const char *sql,                           /* SQL to be evaluted */
  int (*callback)(void*,int,char**,char**),  /* Callback function */
  void *,                                    /* 1st argument to callback */
  char **errmsg                              /* Error msg written here */
);

/*
** CAPI3REF: Result Codes {F10210}
** KEYWORDS: SQLITE_OK {error code} {error codes}
**
** Many SQLite functions return an integer result code from the set shown
** here in order to indicates success or failure.
**
** See also: [SQLITE_IOERR_READ | extended result codes]
*/
#define SQLITE_OK           0   /* Successful result */
/* beginning-of-error-codes */
#define SQLITE_ERROR        1   /* SQL error or missing database */
#define SQLITE_INTERNAL     2   /* Internal logic error in SQLite */
#define SQLITE_PERM         3   /* Access permission denied */
#define SQLITE_ABORT        4   /* Callback routine requested an abort */
#define SQLITE_BUSY         5   /* The database file is locked */
#define SQLITE_LOCKED       6   /* A table in the database is locked */
#define SQLITE_NOMEM        7   /* A malloc() failed */
#define SQLITE_READONLY     8   /* Attempt to write a readonly database */
#define SQLITE_INTERRUPT    9   /* Operation terminated by sqlite3_interrupt()*/
#define SQLITE_IOERR       10   /* Some kind of disk I/O error occurred */
#define SQLITE_CORRUPT     11   /* The database disk image is malformed */
#define SQLITE_NOTFOUND    12   /* NOT USED. Table or record not found */
#define SQLITE_FULL        13   /* Insertion failed because database is full */
#define SQLITE_CANTOPEN    14   /* Unable to open the database file */
#define SQLITE_PROTOCOL    15   /* NOT USED. Database lock protocol error */
#define SQLITE_EMPTY       16   /* Database is empty */
#define SQLITE_SCHEMA      17   /* The database schema changed */
#define SQLITE_TOOBIG      18   /* String or BLOB exceeds size limit */
#define SQLITE_CONSTRAINT  19   /* Abort due to constraint violation */
#define SQLITE_MISMATCH    20   /* Data type mismatch */
#define SQLITE_MISUSE      21   /* Library used incorrectly */
#define SQLITE_NOLFS       22   /* Uses OS features not supported on host */
#define SQLITE_AUTH        23   /* Authorization denied */
#define SQLITE_FORMAT      24   /* Auxiliary database format error */
#define SQLITE_RANGE       25   /* 2nd parameter to sqlite3_bind out of range */
#define SQLITE_NOTADB      26   /* File opened that is not a database file */
#define SQLITE_ROW         100  /* sqlite3_step() has another row ready */
#define SQLITE_DONE        101  /* sqlite3_step() has finished executing */
/* end-of-error-codes */

/*
** CAPI3REF: Extended Result Codes {F10220}
** KEYWORDS: {extended error code} {extended error codes}
** KEYWORDS: {extended result codes}
**
** In its default configuration, SQLite API routines return one of 26 integer
** [SQLITE_OK | result codes].  However, experience has shown that
** many of these result codes are too course-grained.  They do not provide as
** much information about problems as programmers might like.  In an effort to
** address this, newer versions of SQLite (version 3.3.8 and later) include
** support for additional result codes that provide more detailed information
** about errors. The extended result codes are enabled or disabled
** for each database connection using the [sqlite3_extended_result_codes()]
** API.
** 
** Some of the available extended result codes are listed here.
** One may expect the number of extended result codes will be expand
** over time.  Software that uses extended result codes should expect
** to see new result codes in future releases of SQLite.
**
** The SQLITE_OK result code will never be extended.  It will always
** be exactly zero.
** 
** INVARIANTS:
**
** {F10223} The symbolic name for an extended result code always contains
**          a related primary result code as a prefix.
**
** {F10224} Primary result code names contain a single "_" character.
**
** {F10225} Extended result code names contain two or more "_" characters.
**
** {F10226} The numeric value of an extended result code contains the
**          numeric value of its corresponding primary result code in
**          its least significant 8 bits.
*/
#define SQLITE_IOERR_READ          (SQLITE_IOERR | (1<<8))
#define SQLITE_IOERR_SHORT_READ    (SQLITE_IOERR | (2<<8))
#define SQLITE_IOERR_WRITE         (SQLITE_IOERR | (3<<8))
#define SQLITE_IOERR_FSYNC         (SQLITE_IOERR | (4<<8))
#define SQLITE_IOERR_DIR_FSYNC     (SQLITE_IOERR | (5<<8))
#define SQLITE_IOERR_TRUNCATE      (SQLITE_IOERR | (6<<8))
#define SQLITE_IOERR_FSTAT         (SQLITE_IOERR | (7<<8))
#define SQLITE_IOERR_UNLOCK        (SQLITE_IOERR | (8<<8))
#define SQLITE_IOERR_RDLOCK        (SQLITE_IOERR | (9<<8))
#define SQLITE_IOERR_DELETE        (SQLITE_IOERR | (10<<8))
#define SQLITE_IOERR_BLOCKED       (SQLITE_IOERR | (11<<8))
#define SQLITE_IOERR_NOMEM         (SQLITE_IOERR | (12<<8))

/*
** CAPI3REF: Flags For File Open Operations {F10230}
**
** These bit values are intended for use in the
** 3rd parameter to the [sqlite3_open_v2()] interface and
** in the 4th parameter to the xOpen method of the
** [sqlite3_vfs] object.
*/
#define SQLITE_OPEN_READONLY         0x00000001
#define SQLITE_OPEN_READWRITE        0x00000002
#define SQLITE_OPEN_CREATE           0x00000004
#define SQLITE_OPEN_DELETEONCLOSE    0x00000008
#define SQLITE_OPEN_EXCLUSIVE        0x00000010
#define SQLITE_OPEN_MAIN_DB          0x00000100
#define SQLITE_OPEN_TEMP_DB          0x00000200
#define SQLITE_OPEN_TRANSIENT_DB     0x00000400
#define SQLITE_OPEN_MAIN_JOURNAL     0x00000800
#define SQLITE_OPEN_TEMP_JOURNAL     0x00001000
#define SQLITE_OPEN_SUBJOURNAL       0x00002000
#define SQLITE_OPEN_MASTER_JOURNAL   0x00004000

/*
** CAPI3REF: Device Characteristics {F10240}
**
** The xDeviceCapabilities method of the [sqlite3_io_methods]
** object returns an integer which is a vector of the these
** bit values expressing I/O characteristics of the mass storage
** device that holds the file that the [sqlite3_io_methods]
** refers to.
**
** The SQLITE_IOCAP_ATOMIC property means that all writes of
** any size are atomic.  The SQLITE_IOCAP_ATOMICnnn values
** mean that writes of blocks that are nnn bytes in size and
** are aligned to an address which is an integer multiple of
** nnn are atomic.  The SQLITE_IOCAP_SAFE_APPEND value means
** that when data is appended to a file, the data is appended
** first then the size of the file is extended, never the other
** way around.  The SQLITE_IOCAP_SEQUENTIAL property means that
** information is written to disk in the same order as calls
** to xWrite().
*/
#define SQLITE_IOCAP_ATOMIC          0x00000001
#define SQLITE_IOCAP_ATOMIC512       0x00000002
#define SQLITE_IOCAP_ATOMIC1K        0x00000004
#define SQLITE_IOCAP_ATOMIC2K        0x00000008
#define SQLITE_IOCAP_ATOMIC4K        0x00000010
#define SQLITE_IOCAP_ATOMIC8K        0x00000020
#define SQLITE_IOCAP_ATOMIC16K       0x00000040
#define SQLITE_IOCAP_ATOMIC32K       0x00000080
#define SQLITE_IOCAP_ATOMIC64K       0x00000100
#define SQLITE_IOCAP_SAFE_APPEND     0x00000200
#define SQLITE_IOCAP_SEQUENTIAL      0x00000400

/*
** CAPI3REF: File Locking Levels {F10250}
**
** SQLite uses one of these integer values as the second
** argument to calls it makes to the xLock() and xUnlock() methods
** of an [sqlite3_io_methods] object.
*/
#define SQLITE_LOCK_NONE          0
#define SQLITE_LOCK_SHARED        1
#define SQLITE_LOCK_RESERVED      2
#define SQLITE_LOCK_PENDING       3
#define SQLITE_LOCK_EXCLUSIVE     4

/*
** CAPI3REF: Synchronization Type Flags {F10260}
**
** When SQLite invokes the xSync() method of an
** [sqlite3_io_methods] object it uses a combination of
** these integer values as the second argument.
**
** When the SQLITE_SYNC_DATAONLY flag is used, it means that the
** sync operation only needs to flush data to mass storage.  Inode
** information need not be flushed. The SQLITE_SYNC_NORMAL flag means 
** to use normal fsync() semantics. The SQLITE_SYNC_FULL flag means 
** to use Mac OS-X style fullsync instead of fsync().
*/
#define SQLITE_SYNC_NORMAL        0x00002
#define SQLITE_SYNC_FULL          0x00003
#define SQLITE_SYNC_DATAONLY      0x00010


/*
** CAPI3REF: OS Interface Open File Handle {F11110}
**
** An [sqlite3_file] object represents an open file in the OS
** interface layer.  Individual OS interface implementations will
** want to subclass this object by appending additional fields
** for their own use.  The pMethods entry is a pointer to an
** [sqlite3_io_methods] object that defines methods for performing
** I/O operations on the open file.
*/
typedef struct sqlite3_file sqlite3_file;
struct sqlite3_file {
  const struct sqlite3_io_methods *pMethods;  /* Methods for an open file */
};

/*
** CAPI3REF: OS Interface File Virtual Methods Object {F11120}
**
** Every file opened by the [sqlite3_vfs] xOpen method contains a pointer to
** an instance of this object.  This object defines the
** methods used to perform various operations against the open file.
**
** The flags argument to xSync may be one of [SQLITE_SYNC_NORMAL] or
** [SQLITE_SYNC_FULL].  The first choice is the normal fsync().
*  The second choice is an
** OS-X style fullsync.  The SQLITE_SYNC_DATA flag may be ORed in to
** indicate that only the data of the file and not its inode needs to be
** synced.
** 
** The integer values to xLock() and xUnlock() are one of
** <ul>
** <li> [SQLITE_LOCK_NONE],
** <li> [SQLITE_LOCK_SHARED],
** <li> [SQLITE_LOCK_RESERVED],
** <li> [SQLITE_LOCK_PENDING], or
** <li> [SQLITE_LOCK_EXCLUSIVE].
** </ul>
** xLock() increases the lock. xUnlock() decreases the lock.  
** The xCheckReservedLock() method looks
** to see if any database connection, either in this
** process or in some other process, is holding an RESERVED,
** PENDING, or EXCLUSIVE lock on the file.  It returns true
** if such a lock exists and false if not.
** 
** The xFileControl() method is a generic interface that allows custom
** VFS implementations to directly control an open file using the
** [sqlite3_file_control()] interface.  The second "op" argument
** is an integer opcode.   The third
** argument is a generic pointer which is intended to be a pointer
** to a structure that may contain arguments or space in which to
** write return values.  Potential uses for xFileControl() might be
** functions to enable blocking locks with timeouts, to change the
** locking strategy (for example to use dot-file locks), to inquire
** about the status of a lock, or to break stale locks.  The SQLite
** core reserves opcodes less than 100 for its own use. 
** A [SQLITE_FCNTL_LOCKSTATE | list of opcodes] less than 100 is available.
** Applications that define a custom xFileControl method should use opcodes 
** greater than 100 to avoid conflicts.
**
** The xSectorSize() method returns the sector size of the
** device that underlies the file.  The sector size is the
** minimum write that can be performed without disturbing
** other bytes in the file.  The xDeviceCharacteristics()
** method returns a bit vector describing behaviors of the
** underlying device:
**
** <ul>
** <li> [SQLITE_IOCAP_ATOMIC]
** <li> [SQLITE_IOCAP_ATOMIC512]
** <li> [SQLITE_IOCAP_ATOMIC1K]
** <li> [SQLITE_IOCAP_ATOMIC2K]
** <li> [SQLITE_IOCAP_ATOMIC4K]
** <li> [SQLITE_IOCAP_ATOMIC8K]
** <li> [SQLITE_IOCAP_ATOMIC16K]
** <li> [SQLITE_IOCAP_ATOMIC32K]
** <li> [SQLITE_IOCAP_ATOMIC64K]
** <li> [SQLITE_IOCAP_SAFE_APPEND]
** <li> [SQLITE_IOCAP_SEQUENTIAL]
** </ul>
**
** The SQLITE_IOCAP_ATOMIC property means that all writes of
** any size are atomic.  The SQLITE_IOCAP_ATOMICnnn values
** mean that writes of blocks that are nnn bytes in size and
** are aligned to an address which is an integer multiple of
** nnn are atomic.  The SQLITE_IOCAP_SAFE_APPEND value means
** that when data is appended to a file, the data is appended
** first then the size of the file is extended, never the other
** way around.  The SQLITE_IOCAP_SEQUENTIAL property means that
** information is written to disk in the same order as calls
** to xWrite().
*/
typedef struct sqlite3_io_methods sqlite3_io_methods;
struct sqlite3_io_methods {
  int iVersion;
  int (*xClose)(sqlite3_file*);
  int (*xRead)(sqlite3_file*, void*, int iAmt, sqlite3_int64 iOfst);
  int (*xWrite)(sqlite3_file*, const void*, int iAmt, sqlite3_int64 iOfst);
  int (*xTruncate)(sqlite3_file*, sqlite3_int64 size);
  int (*xSync)(sqlite3_file*, int flags);
  int (*xFileSize)(sqlite3_file*, sqlite3_int64 *pSize);
  int (*xLock)(sqlite3_file*, int);
  int (*xUnlock)(sqlite3_file*, int);
  int (*xCheckReservedLock)(sqlite3_file*);
  int (*xFileControl)(sqlite3_file*, int op, void *pArg);
  int (*xSectorSize)(sqlite3_file*);
  int (*xDeviceCharacteristics)(sqlite3_file*);
  /* Additional methods may be added in future releases */
};

/*
** CAPI3REF: Standard File Control Opcodes {F11310}
**
** These integer constants are opcodes for the xFileControl method
** of the [sqlite3_io_methods] object and to the [sqlite3_file_control()]
** interface.
**
** The [SQLITE_FCNTL_LOCKSTATE] opcode is used for debugging.  This
** opcode causes the xFileControl method to write the current state of
** the lock (one of [SQLITE_LOCK_NONE], [SQLITE_LOCK_SHARED],
** [SQLITE_LOCK_RESERVED], [SQLITE_LOCK_PENDING], or [SQLITE_LOCK_EXCLUSIVE])
** into an integer that the pArg argument points to. This capability
** is used during testing and only needs to be supported when SQLITE_TEST
** is defined.
*/
#define SQLITE_FCNTL_LOCKSTATE        1

/*
** CAPI3REF: Mutex Handle {F17110}
**
** The mutex module within SQLite defines [sqlite3_mutex] to be an
** abstract type for a mutex object.  The SQLite core never looks
** at the internal representation of an [sqlite3_mutex].  It only
** deals with pointers to the [sqlite3_mutex] object.
**
** Mutexes are created using [sqlite3_mutex_alloc()].
*/
typedef struct sqlite3_mutex sqlite3_mutex;

/*
** CAPI3REF: OS Interface Object {F11140}
**
** An instance of this object defines the interface between the
** SQLite core and the underlying operating system.  The "vfs"
** in the name of the object stands for "virtual file system".
**
** The iVersion field is initially 1 but may be larger for future
** versions of SQLite.  Additional fields may be appended to this
** object when the iVersion value is increased.
**
** The szOsFile field is the size of the subclassed [sqlite3_file]
** structure used by this VFS.  mxPathname is the maximum length of
** a pathname in this VFS.
**
** Registered sqlite3_vfs objects are kept on a linked list formed by
** the pNext pointer.  The [sqlite3_vfs_register()]
** and [sqlite3_vfs_unregister()] interfaces manage this list
** in a thread-safe way.  The [sqlite3_vfs_find()] interface
** searches the list.
**
** The pNext field is the only field in the sqlite3_vfs 
** structure that SQLite will ever modify.  SQLite will only access
** or modify this field while holding a particular static mutex.
** The application should never modify anything within the sqlite3_vfs
** object once the object has been registered.
**
** The zName field holds the name of the VFS module.  The name must
** be unique across all VFS modules.
**
** {F11141} SQLite will guarantee that the zFilename string passed to
** xOpen() is a full pathname as generated by xFullPathname() and
** that the string will be valid and unchanged until xClose() is
** called.  {END} So the [sqlite3_file] can store a pointer to the
** filename if it needs to remember the filename for some reason.
**
** {F11142} The flags argument to xOpen() includes all bits set in
** the flags argument to [sqlite3_open_v2()].  Or if [sqlite3_open()]
** or [sqlite3_open16()] is used, then flags includes at least
** [SQLITE_OPEN_READWRITE] | [SQLITE_OPEN_CREATE]. {END}
** If xOpen() opens a file read-only then it sets *pOutFlags to
** include [SQLITE_OPEN_READONLY].  Other bits in *pOutFlags may be
** set.
** 
** {F11143} SQLite will also add one of the following flags to the xOpen()
** call, depending on the object being opened:
** 
** <ul>
** <li>  [SQLITE_OPEN_MAIN_DB]
** <li>  [SQLITE_OPEN_MAIN_JOURNAL]
** <li>  [SQLITE_OPEN_TEMP_DB]
** <li>  [SQLITE_OPEN_TEMP_JOURNAL]
** <li>  [SQLITE_OPEN_TRANSIENT_DB]
** <li>  [SQLITE_OPEN_SUBJOURNAL]
** <li>  [SQLITE_OPEN_MASTER_JOURNAL]
** </ul> {END}
**
** The file I/O implementation can use the object type flags to
** changes the way it deals with files.  For example, an application
** that does not care about crash recovery or rollback might make
** the open of a journal file a no-op.  Writes to this journal would
** also be no-ops, and any attempt to read the journal would return 
** SQLITE_IOERR.  Or the implementation might recognize that a database 
** file will be doing page-aligned sector reads and writes in a random 
** order and set up its I/O subsystem accordingly.
** 
** SQLite might also add one of the following flags to the xOpen
** method:
** 
** <ul>
** <li> [SQLITE_OPEN_DELETEONCLOSE]
** <li> [SQLITE_OPEN_EXCLUSIVE]
** </ul>
** 
** {F11145} The [SQLITE_OPEN_DELETEONCLOSE] flag means the file should be
** deleted when it is closed.  {F11146} The [SQLITE_OPEN_DELETEONCLOSE]
** will be set for TEMP  databases, journals and for subjournals. 
** {F11147} The [SQLITE_OPEN_EXCLUSIVE] flag means the file should be opened
** for exclusive access.  This flag is set for all files except
** for the main database file. {END}
** 
** {F11148} At least szOsFile bytes of memory are allocated by SQLite 
** to hold the  [sqlite3_file] structure passed as the third 
** argument to xOpen.  {END}  The xOpen method does not have to
** allocate the structure; it should just fill it in.
** 
** {F11149} The flags argument to xAccess() may be [SQLITE_ACCESS_EXISTS] 
** to test for the existance of a file,
** or [SQLITE_ACCESS_READWRITE] to test to see
** if a file is readable and writable, or [SQLITE_ACCESS_READ]
** to test to see if a file is at least readable.  {END} The file can be a 
** directory.
** 
** {F11150} SQLite will always allocate at least mxPathname+1 bytes for
** the output buffers for xGetTempname and xFullPathname. {F11151} The exact
** size of the output buffer is also passed as a parameter to both 
** methods. {END} If the output buffer is not large enough, SQLITE_CANTOPEN
** should be returned. As this is handled as a fatal error by SQLite,
** vfs implementations should endeavor to prevent this by setting 
** mxPathname to a sufficiently large value.
** 
** The xRandomness(), xSleep(), and xCurrentTime() interfaces
** are not strictly a part of the filesystem, but they are
** included in the VFS structure for completeness.
** The xRandomness() function attempts to return nBytes bytes
** of good-quality randomness into zOut.  The return value is
** the actual number of bytes of randomness obtained.  The
** xSleep() method causes the calling thread to sleep for at
** least the number of microseconds given.  The xCurrentTime()
** method returns a Julian Day Number for the current date and
** time.
*/
typedef struct sqlite3_vfs sqlite3_vfs;
struct sqlite3_vfs {
  int iVersion;            /* Structure version number */
  int szOsFile;            /* Size of subclassed sqlite3_file */
  int mxPathname;          /* Maximum file pathname length */
  sqlite3_vfs *pNext;      /* Next registered VFS */
  const char *zName;       /* Name of this virtual file system */
  void *pAppData;          /* Pointer to application-specific data */
  int (*xOpen)(sqlite3_vfs*, const char *zName, sqlite3_file*,
               int flags, int *pOutFlags);
  int (*xDelete)(sqlite3_vfs*, const char *zName, int syncDir);
  int (*xAccess)(sqlite3_vfs*, const char *zName, int flags);
  int (*xGetTempname)(sqlite3_vfs*, int nOut, char *zOut);
  int (*xFullPathname)(sqlite3_vfs*, const char *zName, int nOut, char *zOut);
  void *(*xDlOpen)(sqlite3_vfs*, const char *zFilename);
  void (*xDlError)(sqlite3_vfs*, int nByte, char *zErrMsg);
  void *(*xDlSym)(sqlite3_vfs*,void*, const char *zSymbol);
  void (*xDlClose)(sqlite3_vfs*, void*);
  int (*xRandomness)(sqlite3_vfs*, int nByte, char *zOut);
  int (*xSleep)(sqlite3_vfs*, int microseconds);
  int (*xCurrentTime)(sqlite3_vfs*, double*);
  /* New fields may be appended in figure versions.  The iVersion
  ** value will increment whenever this happens. */
};

/*
** CAPI3REF: Flags for the xAccess VFS method {F11190}
**
** {F11191} These integer constants can be used as the third parameter to
** the xAccess method of an [sqlite3_vfs] object. {END}  They determine
** what kind of permissions the xAccess method is
** looking for.  {F11192} With SQLITE_ACCESS_EXISTS, the xAccess method
** simply checks to see if the file exists. {F11193} With
** SQLITE_ACCESS_READWRITE, the xAccess method checks to see
** if the file is both readable and writable.  {F11194} With
** SQLITE_ACCESS_READ the xAccess method
** checks to see if the file is readable.
*/
#define SQLITE_ACCESS_EXISTS    0
#define SQLITE_ACCESS_READWRITE 1
#define SQLITE_ACCESS_READ      2

/*
** CAPI3REF: Enable Or Disable Extended Result Codes {F12200}
**
** The sqlite3_extended_result_codes() routine enables or disables the
** [SQLITE_IOERR_READ | extended result codes] feature of SQLite.
** The extended result codes are disabled by default for historical
** compatibility.
**
** INVARIANTS:
**
** {F12201} Each new [database connection] has the 
**          [extended result codes] feature
**          disabled by default.
**
** {F12202} The [sqlite3_extended_result_codes(D,F)] interface will enable
**          [extended result codes] for the 
**          [database connection] D if the F parameter
**          is true, or disable them if F is false.
*/
int sqlite3_extended_result_codes(sqlite3*, int onoff);

/*
** CAPI3REF: Last Insert Rowid {F12220}
**
** Each entry in an SQLite table has a unique 64-bit signed
** integer key called the "rowid". The rowid is always available
** as an undeclared column named ROWID, OID, or _ROWID_ as long as those
** names are not also used by explicitly declared columns. If
** the table has a column of type INTEGER PRIMARY KEY then that column
** is another alias for the rowid.
**
** This routine returns the rowid of the most recent
** successful INSERT into the database from the database connection
** shown in the first argument.  If no successful inserts
** have ever occurred on this database connection, zero is returned.
**
** If an INSERT occurs within a trigger, then the rowid of the
** inserted row is returned by this routine as long as the trigger
** is running.  But once the trigger terminates, the value returned
** by this routine reverts to the last value inserted before the
** trigger fired.
**
** An INSERT that fails due to a constraint violation is not a
** successful insert and does not change the value returned by this
** routine.  Thus INSERT OR FAIL, INSERT OR IGNORE, INSERT OR ROLLBACK,
** and INSERT OR ABORT make no changes to the return value of this
** routine when their insertion fails.  When INSERT OR REPLACE 
** encounters a constraint violation, it does not fail.  The
** INSERT continues to completion after deleting rows that caused
** the constraint problem so INSERT OR REPLACE will always change
** the return value of this interface. 
**
** For the purposes of this routine, an insert is considered to
** be successful even if it is subsequently rolled back.
**
** INVARIANTS:
**
** {F12221} The [sqlite3_last_insert_rowid()] function returns the
**          rowid of the most recent successful insert done
**          on the same database connection and within the same
**          trigger context, or zero if there have
**          been no qualifying inserts on that connection.
**
** {F12223} The [sqlite3_last_insert_rowid()] function returns
**          same value when called from the same trigger context
**          immediately before and after a ROLLBACK.
**
** LIMITATIONS:
**
** {U12232} If a separate thread does a new insert on the same
**          database connection while the [sqlite3_last_insert_rowid()]
**          function is running and thus changes the last insert rowid,
**          then the value returned by [sqlite3_last_insert_rowid()] is
**          unpredictable and might not equal either the old or the new
**          last insert rowid.
*/
sqlite3_int64 sqlite3_last_insert_rowid(sqlite3*);

/*
** CAPI3REF: Count The Number Of Rows Modified {F12240}
**
** This function returns the number of database rows that were changed
** or inserted or deleted by the most recently completed SQL statement
** on the connection specified by the first parameter.  Only
** changes that are directly specified by the INSERT, UPDATE, or
** DELETE statement are counted.  Auxiliary changes caused by
** triggers are not counted. Use the [sqlite3_total_changes()] function
** to find the total number of changes including changes caused by triggers.
**
** A "row change" is a change to a single row of a single table
** caused by an INSERT, DELETE, or UPDATE statement.  Rows that
** are changed as side effects of REPLACE constraint resolution,
** rollback, ABORT processing, DROP TABLE, or by any other
** mechanisms do not count as direct row changes.
**
** A "trigger context" is a scope of execution that begins and
** ends with the script of a trigger.  Most SQL statements are
** evaluated outside of any trigger.  This is the "top level"
** trigger context.  If a trigger fires from the top level, a
** new trigger context is entered for the duration of that one
** trigger.  Subtriggers create subcontexts for their duration.
**
** Calling [sqlite3_exec()] or [sqlite3_step()] recursively does
** not create a new trigger context.
**
** This function returns the number of direct row changes in the
** most recent INSERT, UPDATE, or DELETE statement within the same
** trigger context.
**
** So when called from the top level, this function returns the
** number of changes in the most recent INSERT, UPDATE, or DELETE
** that also occurred at the top level.
** Within the body of a trigger, the sqlite3_changes() interface
** can be called to find the number of
** changes in the most recently completed INSERT, UPDATE, or DELETE
** statement within the body of the same trigger.
** However, the number returned does not include in changes
** caused by subtriggers since they have their own context.
**
** SQLite implements the command "DELETE FROM table" without
** a WHERE clause by dropping and recreating the table.  (This is much
** faster than going through and deleting individual elements from the
** table.)  Because of this optimization, the deletions in
** "DELETE FROM table" are not row changes and will not be counted
** by the sqlite3_changes() or [sqlite3_total_changes()] functions.
** To get an accurate count of the number of rows deleted, use
** "DELETE FROM table WHERE 1" instead.
**
** INVARIANTS:
**
** {F12241} The [sqlite3_changes()] function returns the number of
**          row changes caused by the most recent INSERT, UPDATE,
**          or DELETE statement on the same database connection and
**          within the same trigger context, or zero if there have
**          not been any qualifying row changes.
**
** LIMITATIONS:
**
** {U12252} If a separate thread makes changes on the same database connection
**          while [sqlite3_changes()] is running then the value returned
**          is unpredictable and unmeaningful.
*/
int sqlite3_changes(sqlite3*);

/*
** CAPI3REF: Total Number Of Rows Modified {F12260}
***
** This function returns the number of row changes caused
** by INSERT, UPDATE or DELETE statements since the database handle
** was opened.  The count includes all changes from all trigger
** contexts.  But the count does not include changes used to
** implement REPLACE constraints, do rollbacks or ABORT processing,
** or DROP table processing.
** The changes
** are counted as soon as the statement that makes them is completed 
** (when the statement handle is passed to [sqlite3_reset()] or 
** [sqlite3_finalize()]).
**
** SQLite implements the command "DELETE FROM table" without
** a WHERE clause by dropping and recreating the table.  (This is much
** faster than going
** through and deleting individual elements from the table.)  Because of
** this optimization, the change count for "DELETE FROM table" will be
** zero regardless of the number of elements that were originally in the
** table. To get an accurate count of the number of rows deleted, use
** "DELETE FROM table WHERE 1" instead.
**
** See also the [sqlite3_changes()] interface.
**
** INVARIANTS:
** 
** {F12261} The [sqlite3_total_changes()] returns the total number
**          of row changes caused by INSERT, UPDATE, and/or DELETE
**          statements on the same [database connection], in any
**          trigger context, since the database connection was
**          created.
**
** LIMITATIONS:
**
** {U12264} If a separate thread makes changes on the same database connection
**          while [sqlite3_total_changes()] is running then the value 
**          returned is unpredictable and unmeaningful.
*/
int sqlite3_total_changes(sqlite3*);

/*
** CAPI3REF: Interrupt A Long-Running Query {F12270}
**
** This function causes any pending database operation to abort and
** return at its earliest opportunity. This routine is typically
** called in response to a user action such as pressing "Cancel"
** or Ctrl-C where the user wants a long query operation to halt
** immediately.
**
** It is safe to call this routine from a thread different from the
** thread that is currently running the database operation.  But it
** is not safe to call this routine with a database connection that
** is closed or might close before sqlite3_interrupt() returns.
**
** If an SQL is very nearly finished at the time when sqlite3_interrupt()
** is called, then it might not have an opportunity to be interrupted.
** It might continue to completion.
** An SQL operation that is interrupted will return
** [SQLITE_INTERRUPT].  If the interrupted SQL operation is an
** INSERT, UPDATE, or DELETE that is inside an explicit transaction, 
** then the entire transaction will be rolled back automatically.
** A call to sqlite3_interrupt() has no effect on SQL statements
** that are started after sqlite3_interrupt() returns.
**
** INVARIANTS:
**
** {F12271} The [sqlite3_interrupt()] interface will force all running
**          SQL statements associated with the same database connection
**          to halt after processing at most one additional row of
**          data.
**
** {F12272} Any SQL statement that is interrupted by [sqlite3_interrupt()]
**          will return [SQLITE_INTERRUPT].
**
** LIMITATIONS:
**
** {U12279} If the database connection closes while [sqlite3_interrupt()]
**          is running then bad things will likely happen.
*/
void sqlite3_interrupt(sqlite3*);

/*
** CAPI3REF: Determine If An SQL Statement Is Complete {F10510}
**
** These routines are useful for command-line input to determine if the
** currently entered text seems to form complete a SQL statement or
** if additional input is needed before sending the text into
** SQLite for parsing.  These routines return true if the input string
** appears to be a complete SQL statement.  A statement is judged to be
** complete if it ends with a semicolon token and is not a fragment of a
** CREATE TRIGGER statement.  Semicolons that are embedded within
** string literals or quoted identifier names or comments are not
** independent tokens (they are part of the token in which they are
** embedded) and thus do not count as a statement terminator.
**
** These routines do not parse the SQL and
** so will not detect syntactically incorrect SQL.
**
** INVARIANTS:
**
** {F10511} The sqlite3_complete() and sqlite3_complete16() functions
**          return true (non-zero) if and only if the last
**          non-whitespace token in their input is a semicolon that
**          is not in between the BEGIN and END of a CREATE TRIGGER
**          statement.
**
** LIMITATIONS:
**
** {U10512} The input to sqlite3_complete() must be a zero-terminated
**          UTF-8 string.
**
** {U10513} The input to sqlite3_complete16() must be a zero-terminated
**          UTF-16 string in native byte order.
*/
int sqlite3_complete(const char *sql);
int sqlite3_complete16(const void *sql);

/*
** CAPI3REF: Register A Callback To Handle SQLITE_BUSY Errors {F12310}
**
** This routine identifies a callback function that might be
** invoked whenever an attempt is made to open a database table 
** that another thread or process has locked.
** If the busy callback is NULL, then [SQLITE_BUSY]
** or [SQLITE_IOERR_BLOCKED]
** is returned immediately upon encountering the lock.
** If the busy callback is not NULL, then the
** callback will be invoked with two arguments.  The
** first argument to the handler is a copy of the void* pointer which
** is the third argument to this routine.  The second argument to
** the handler is the number of times that the busy handler has
** been invoked for this locking event.   If the
** busy callback returns 0, then no additional attempts are made to
** access the database and [SQLITE_BUSY] or [SQLITE_IOERR_BLOCKED] is returned.
** If the callback returns non-zero, then another attempt
** is made to open the database for reading and the cycle repeats.
**
** The presence of a busy handler does not guarantee that
** it will be invoked when there is lock contention.
** If SQLite determines that invoking the busy handler could result in
** a deadlock, it will go ahead and return [SQLITE_BUSY] or
** [SQLITE_IOERR_BLOCKED] instead of invoking the
** busy handler.
** Consider a scenario where one process is holding a read lock that
** it is trying to promote to a reserved lock and
** a second process is holding a reserved lock that it is trying
** to promote to an exclusive lock.  The first process cannot proceed
** because it is blocked by the second and the second process cannot
** proceed because it is blocked by the first.  If both processes
** invoke the busy handlers, neither will make any progress.  Therefore,
** SQLite returns [SQLITE_BUSY] for the first process, hoping that this
** will induce the first process to release its read lock and allow
** the second process to proceed.
**
** The default busy callback is NULL.
**
** The [SQLITE_BUSY] error is converted to [SQLITE_IOERR_BLOCKED]
** when SQLite is in the middle of a large transaction where all the
** changes will not fit into the in-memory cache.  SQLite will
** already hold a RESERVED lock on the database file, but it needs
** to promote this lock to EXCLUSIVE so that it can spill cache
** pages into the database file without harm to concurrent
** readers.  If it is unable to promote the lock, then the in-memory
** cache will be left in an inconsistent state and so the error
** code is promoted from the relatively benign [SQLITE_BUSY] to
** the more severe [SQLITE_IOERR_BLOCKED].  This error code promotion
** forces an automatic rollback of the changes.  See the
** <a href="http://www.sqlite.org/cvstrac/wiki?p=CorruptionFollowingBusyError">
** CorruptionFollowingBusyError</a> wiki page for a discussion of why
** this is important.
**  
** There can only be a single busy handler defined for each database
** connection.  Setting a new busy handler clears any previous one. 
** Note that calling [sqlite3_busy_timeout()] will also set or clear
** the busy handler.
**
** INVARIANTS:
**
** {F12311} The [sqlite3_busy_handler()] function replaces the busy handler
**          callback in the database connection identified by the 1st
**          parameter with a new busy handler identified by the 2nd and 3rd
**          parameters.
**
** {F12312} The default busy handler for new database connections is NULL.
**
** {F12314} When two or more database connection share a common cache,
**          the busy handler for the database connection currently using
**          the cache is invoked when the cache encounters a lock.
**
** {F12316} If a busy handler callback returns zero, then the SQLite
**          interface that provoked the locking event will return
**          [SQLITE_BUSY].
**
** {F12318} SQLite will invokes the busy handler with two argument which
**          are a copy of the pointer supplied by the 3rd parameter to
**          [sqlite3_busy_handler()] and a count of the number of prior
**          invocations of the busy handler for the same locking event.
**
** LIMITATIONS:
**
** {U12319} A busy handler should not call close the database connection
**          or prepared statement that invoked the busy handler.
*/
int sqlite3_busy_handler(sqlite3*, int(*)(void*,int), void*);

/*
** CAPI3REF: Set A Busy Timeout {F12340}
**
** This routine sets a [sqlite3_busy_handler | busy handler]
** that sleeps for a while when a
** table is locked.  The handler will sleep multiple times until 
** at least "ms" milliseconds of sleeping have been done. {F12343} After
** "ms" milliseconds of sleeping, the handler returns 0 which
** causes [sqlite3_step()] to return [SQLITE_BUSY] or [SQLITE_IOERR_BLOCKED].
**
** Calling this routine with an argument less than or equal to zero
** turns off all busy handlers.
**
** There can only be a single busy handler for a particular database
** connection.  If another busy handler was defined  
** (using [sqlite3_busy_handler()]) prior to calling
** this routine, that other busy handler is cleared.
**
** INVARIANTS:
**
** {F12341} The [sqlite3_busy_timeout()] function overrides any prior
**          [sqlite3_busy_timeout()] or [sqlite3_busy_handler()] setting
**          on the same database connection.
**
** {F12343} If the 2nd parameter to [sqlite3_busy_timeout()] is less than
**          or equal to zero, then the busy handler is cleared so that
**          all subsequent locking events immediately return [SQLITE_BUSY].
**
** {F12344} If the 2nd parameter to [sqlite3_busy_timeout()] is a positive
**          number N, then a busy handler is set that repeatedly calls
**          the xSleep() method in the VFS interface until either the
**          lock clears or until the cumulative sleep time reported back
**          by xSleep() exceeds N milliseconds.
*/
int sqlite3_busy_timeout(sqlite3*, int ms);

/*
** CAPI3REF: Convenience Routines For Running Queries {F12370}
**
** Definition: A <b>result table</b> is memory data structure created by the
** [sqlite3_get_table()] interface.  A result table records the
** complete query results from one or more queries.
**
** The table conceptually has a number of rows and columns.  But
** these numbers are not part of the result table itself.  These
** numbers are obtained separately.  Let N be the number of rows
** and M be the number of columns.
**
** A result table is an array of pointers to zero-terminated
** UTF-8 strings.  There are (N+1)*M elements in the array.  
** The first M pointers point to zero-terminated strings that 
** contain the names of the columns.
** The remaining entries all point to query results.  NULL
** values are give a NULL pointer.  All other values are in
** their UTF-8 zero-terminated string representation as returned by
** [sqlite3_column_text()].
**
** A result table might consists of one or more memory allocations.
** It is not safe to pass a result table directly to [sqlite3_free()].
** A result table should be deallocated using [sqlite3_free_table()].
**
** As an example of the result table format, suppose a query result
** is as follows:
**
** <blockquote><pre>
**        Name        | Age
**        -----------------------
**        Alice       | 43
**        Bob         | 28
**        Cindy       | 21
** </pre></blockquote>
**
** There are two column (M==2) and three rows (N==3).  Thus the
** result table has 8 entries.  Suppose the result table is stored
** in an array names azResult.  Then azResult holds this content:
**
** <blockquote><pre>
**        azResult&#91;0] = "Name";
**        azResult&#91;1] = "Age";
**        azResult&#91;2] = "Alice";
**        azResult&#91;3] = "43";
**        azResult&#91;4] = "Bob";
**        azResult&#91;5] = "28";
**        azResult&#91;6] = "Cindy";
**        azResult&#91;7] = "21";
** </pre></blockquote>
**
** The sqlite3_get_table() function evaluates one or more
** semicolon-separated SQL statements in the zero-terminated UTF-8
** string of its 2nd parameter.  It returns a result table to the
** pointer given in its 3rd parameter.
**
** After the calling function has finished using the result, it should 
** pass the pointer to the result table to sqlite3_free_table() in order to 
** release the memory that was malloc-ed.  Because of the way the 
** [sqlite3_malloc()] happens within sqlite3_get_table(), the calling
** function must not try to call [sqlite3_free()] directly.  Only 
** [sqlite3_free_table()] is able to release the memory properly and safely.
**
** The sqlite3_get_table() interface is implemented as a wrapper around
** [sqlite3_exec()].  The sqlite3_get_table() routine does not have access
** to any internal data structures of SQLite.  It uses only the public
** interface defined here.  As a consequence, errors that occur in the
** wrapper layer outside of the internal [sqlite3_exec()] call are not
** reflected in subsequent calls to [sqlite3_errcode()] or
** [sqlite3_errmsg()].
**
** INVARIANTS:
**
** {F12371} If a [sqlite3_get_table()] fails a memory allocation, then
**          it frees the result table under construction, aborts the
**          query in process, skips any subsequent queries, sets the
**          *resultp output pointer to NULL and returns [SQLITE_NOMEM].
**
** {F12373} If the ncolumn parameter to [sqlite3_get_table()] is not NULL
**          then [sqlite3_get_table()] write the number of columns in the
**          result set of the query into *ncolumn if the query is
**          successful (if the function returns SQLITE_OK).
**
** {F12374} If the nrow parameter to [sqlite3_get_table()] is not NULL
**          then [sqlite3_get_table()] write the number of rows in the
**          result set of the query into *nrow if the query is
**          successful (if the function returns SQLITE_OK).
**
** {F12376} The [sqlite3_get_table()] function sets its *ncolumn value
**          to the number of columns in the result set of the query in the
**          sql parameter, or to zero if the query in sql has an empty
**          result set.
*/
int sqlite3_get_table(
  sqlite3*,             /* An open database */
  const char *sql,      /* SQL to be evaluated */
  char ***pResult,      /* Results of the query */
  int *nrow,            /* Number of result rows written here */
  int *ncolumn,         /* Number of result columns written here */
  char **errmsg         /* Error msg written here */
);
void sqlite3_free_table(char **result);

/*
** CAPI3REF: Formatted String Printing Functions {F17400}
**
** These routines are workalikes of the "printf()" family of functions
** from the standard C library.
**
** The sqlite3_mprintf() and sqlite3_vmprintf() routines write their
** results into memory obtained from [sqlite3_malloc()].
** The strings returned by these two routines should be
** released by [sqlite3_free()].   Both routines return a
** NULL pointer if [sqlite3_malloc()] is unable to allocate enough
** memory to hold the resulting string.
**
** In sqlite3_snprintf() routine is similar to "snprintf()" from
** the standard C library.  The result is written into the
** buffer supplied as the second parameter whose size is given by
** the first parameter. Note that the order of the
** first two parameters is reversed from snprintf().  This is an
** historical accident that cannot be fixed without breaking
** backwards compatibility.  Note also that sqlite3_snprintf()
** returns a pointer to its buffer instead of the number of
** characters actually written into the buffer.  We admit that
** the number of characters written would be a more useful return
** value but we cannot change the implementation of sqlite3_snprintf()
** now without breaking compatibility.
**
** As long as the buffer size is greater than zero, sqlite3_snprintf()
** guarantees that the buffer is always zero-terminated.  The first
** parameter "n" is the total size of the buffer, including space for
** the zero terminator.  So the longest string that can be completely
** written will be n-1 characters.
**
** These routines all implement some additional formatting
** options that are useful for constructing SQL statements.
** All of the usual printf formatting options apply.  In addition, there
** is are "%q", "%Q", and "%z" options.
**
** The %q option works like %s in that it substitutes a null-terminated
** string from the argument list.  But %q also doubles every '\'' character.
** %q is designed for use inside a string literal.  By doubling each '\''
** character it escapes that character and allows it to be inserted into
** the string.
**
** For example, so some string variable contains text as follows:
**
** <blockquote><pre>
**  char *zText = "It's a happy day!";
** </pre></blockquote>
**
** One can use this text in an SQL statement as follows:
**
** <blockquote><pre>
**  char *zSQL = sqlite3_mprintf("INSERT INTO table VALUES('%q')", zText);
**  sqlite3_exec(db, zSQL, 0, 0, 0);
**  sqlite3_free(zSQL);
** </pre></blockquote>
**
** Because the %q format string is used, the '\'' character in zText
** is escaped and the SQL generated is as follows:
**
** <blockquote><pre>
**  INSERT INTO table1 VALUES('It''s a happy day!')
** </pre></blockquote>
**
** This is correct.  Had we used %s instead of %q, the generated SQL
** would have looked like this:
**
** <blockquote><pre>
**  INSERT INTO table1 VALUES('It's a happy day!');
** </pre></blockquote>
**
** This second example is an SQL syntax error.  As a general rule you
** should always use %q instead of %s when inserting text into a string 
** literal.
**
** The %Q option works like %q except it also adds single quotes around
** the outside of the total string.  Or if the parameter in the argument
** list is a NULL pointer, %Q substitutes the text "NULL" (without single
** quotes) in place of the %Q option. {END}  So, for example, one could say:
**
** <blockquote><pre>
**  char *zSQL = sqlite3_mprintf("INSERT INTO table VALUES(%Q)", zText);
**  sqlite3_exec(db, zSQL, 0, 0, 0);
**  sqlite3_free(zSQL);
** </pre></blockquote>
**
** The code above will render a correct SQL statement in the zSQL
** variable even if the zText variable is a NULL pointer.
**
** The "%z" formatting option works exactly like "%s" with the
** addition that after the string has been read and copied into
** the result, [sqlite3_free()] is called on the input string. {END}
**
** INVARIANTS:
**
** {F17403}  The [sqlite3_mprintf()] and [sqlite3_vmprintf()] interfaces
**           return either pointers to zero-terminated UTF-8 strings held in
**           memory obtained from [sqlite3_malloc()] or NULL pointers if
**           a call to [sqlite3_malloc()] fails.
**
** {F17406}  The [sqlite3_snprintf()] interface writes a zero-terminated
**           UTF-8 string into the buffer pointed to by the second parameter
**           provided that the first parameter is greater than zero.
**
** {F17407}  The [sqlite3_snprintf()] interface does not writes slots of
**           its output buffer (the second parameter) outside the range
**           of 0 through N-1 (where N is the first parameter)
**           regardless of the length of the string
**           requested by the format specification.
**   
*/
char *sqlite3_mprintf(const char*,...);
char *sqlite3_vmprintf(const char*, va_list);
char *sqlite3_snprintf(int,char*,const char*, ...);

/*
** CAPI3REF: Memory Allocation Subsystem {F17300}
**
** The SQLite core  uses these three routines for all of its own
** internal memory allocation needs. "Core" in the previous sentence
** does not include operating-system specific VFS implementation.  The
** windows VFS uses native malloc and free for some operations.
**
** The sqlite3_malloc() routine returns a pointer to a block
** of memory at least N bytes in length, where N is the parameter.
** If sqlite3_malloc() is unable to obtain sufficient free
** memory, it returns a NULL pointer.  If the parameter N to
** sqlite3_malloc() is zero or negative then sqlite3_malloc() returns
** a NULL pointer.
**
** Calling sqlite3_free() with a pointer previously returned
** by sqlite3_malloc() or sqlite3_realloc() releases that memory so
** that it might be reused.  The sqlite3_free() routine is
** a no-op if is called with a NULL pointer.  Passing a NULL pointer
** to sqlite3_free() is harmless.  After being freed, memory
** should neither be read nor written.  Even reading previously freed
** memory might result in a segmentation fault or other severe error.
** Memory corruption, a segmentation fault, or other severe error
** might result if sqlite3_free() is called with a non-NULL pointer that
** was not obtained from sqlite3_malloc() or sqlite3_free().
**
** The sqlite3_realloc() interface attempts to resize a
** prior memory allocation to be at least N bytes, where N is the
** second parameter.  The memory allocation to be resized is the first
** parameter.  If the first parameter to sqlite3_realloc()
** is a NULL pointer then its behavior is identical to calling
** sqlite3_malloc(N) where N is the second parameter to sqlite3_realloc().
** If the second parameter to sqlite3_realloc() is zero or
** negative then the behavior is exactly the same as calling
** sqlite3_free(P) where P is the first parameter to sqlite3_realloc().
** Sqlite3_realloc() returns a pointer to a memory allocation
** of at least N bytes in size or NULL if sufficient memory is unavailable.
** If M is the size of the prior allocation, then min(N,M) bytes
** of the prior allocation are copied into the beginning of buffer returned
** by sqlite3_realloc() and the prior allocation is freed.
** If sqlite3_realloc() returns NULL, then the prior allocation
** is not freed.
**
** The memory returned by sqlite3_malloc() and sqlite3_realloc()
** is always aligned to at least an 8 byte boundary. {END}
**
** The default implementation
** of the memory allocation subsystem uses the malloc(), realloc()
** and free() provided by the standard C library. {F17382} However, if 
** SQLite is compiled with the following C preprocessor macro
**
** <blockquote> SQLITE_MEMORY_SIZE=<i>NNN</i> </blockquote>
**
** where <i>NNN</i> is an integer, then SQLite create a static
** array of at least <i>NNN</i> bytes in size and use that array
** for all of its dynamic memory allocation needs. {END}  Additional
** memory allocator options may be added in future releases.
**
** In SQLite version 3.5.0 and 3.5.1, it was possible to define
** the SQLITE_OMIT_MEMORY_ALLOCATION which would cause the built-in
** implementation of these routines to be omitted.  That capability
** is no longer provided.  Only built-in memory allocators can be
** used.
**
** The windows OS interface layer calls
** the system malloc() and free() directly when converting
** filenames between the UTF-8 encoding used by SQLite
** and whatever filename encoding is used by the particular windows
** installation.  Memory allocation errors are detected, but
** they are reported back as [SQLITE_CANTOPEN] or
** [SQLITE_IOERR] rather than [SQLITE_NOMEM].
**
** INVARIANTS:
**
** {F17303}  The [sqlite3_malloc(N)] interface returns either a pointer to 
**           newly checked-out block of at least N bytes of memory
**           that is 8-byte aligned, 
**           or it returns NULL if it is unable to fulfill the request.
**
** {F17304}  The [sqlite3_malloc(N)] interface returns a NULL pointer if
**           N is less than or equal to zero.
**
** {F17305}  The [sqlite3_free(P)] interface releases memory previously
**           returned from [sqlite3_malloc()] or [sqlite3_realloc()],
**           making it available for reuse.
**
** {F17306}  A call to [sqlite3_free(NULL)] is a harmless no-op.
**
** {F17310}  A call to [sqlite3_realloc(0,N)] is equivalent to a call
**           to [sqlite3_malloc(N)].
**
** {F17312}  A call to [sqlite3_realloc(P,0)] is equivalent to a call
**           to [sqlite3_free(P)].
**
** {F17315}  The SQLite core uses [sqlite3_malloc()], [sqlite3_realloc()],
**           and [sqlite3_free()] for all of its memory allocation and
**           deallocation needs.
**
** {F17318}  The [sqlite3_realloc(P,N)] interface returns either a pointer
**           to a block of checked-out memory of at least N bytes in size
**           that is 8-byte aligned, or a NULL pointer.
**
** {F17321}  When [sqlite3_realloc(P,N)] returns a non-NULL pointer, it first
**           copies the first K bytes of content from P into the newly allocated
**           where K is the lessor of N and the size of the buffer P.
**
** {F17322}  When [sqlite3_realloc(P,N)] returns a non-NULL pointer, it first
**           releases the buffer P.
**
** {F17323}  When [sqlite3_realloc(P,N)] returns NULL, the buffer P is
**           not modified or released.
**
** LIMITATIONS:
**
** {U17350}  The pointer arguments to [sqlite3_free()] and [sqlite3_realloc()]
**           must be either NULL or else a pointer obtained from a prior
**           invocation of [sqlite3_malloc()] or [sqlite3_realloc()] that has
**           not been released.
**
** {U17351}  The application must not read or write any part of 
**           a block of memory after it has been released using
**           [sqlite3_free()] or [sqlite3_realloc()].
**
*/
void *sqlite3_malloc(int);
void *sqlite3_realloc(void*, int);
void sqlite3_free(void*);

/*
** CAPI3REF: Memory Allocator Statistics {F17370}
**
** SQLite provides these two interfaces for reporting on the status
** of the [sqlite3_malloc()], [sqlite3_free()], and [sqlite3_realloc()]
** the memory allocation subsystem included within the SQLite.
**
** INVARIANTS:
**
** {F17371} The [sqlite3_memory_used()] routine returns the
**          number of bytes of memory currently outstanding 
**          (malloced but not freed).
**
** {F17373} The [sqlite3_memory_highwater()] routine returns the maximum
**          value of [sqlite3_memory_used()] 
**          since the highwater mark was last reset.
**
** {F17374} The values returned by [sqlite3_memory_used()] and
**          [sqlite3_memory_highwater()] include any overhead
**          added by SQLite in its implementation of [sqlite3_malloc()],
**          but not overhead added by the any underlying system library
**          routines that [sqlite3_malloc()] may call.
** 
** {F17375} The memory highwater mark is reset to the current value of
**          [sqlite3_memory_used()] if and only if the parameter to
**          [sqlite3_memory_highwater()] is true.  The value returned
**          by [sqlite3_memory_highwater(1)] is the highwater mark
**          prior to the reset.
*/
sqlite3_int64 sqlite3_memory_used(void);
sqlite3_int64 sqlite3_memory_highwater(int resetFlag);

/*
** CAPI3REF: Pseudo-Random Number Generator {F17390}
**
** SQLite contains a high-quality pseudo-random number generator (PRNG) used to
** select random ROWIDs when inserting new records into a table that
** already uses the largest possible ROWID.  The PRNG is also used for
** the build-in random() and randomblob() SQL functions.  This interface allows
** appliations to access the same PRNG for other purposes.
**
** A call to this routine stores N bytes of randomness into buffer P.
**
** The first time this routine is invoked (either internally or by
** the application) the PRNG is seeded using randomness obtained
** from the xRandomness method of the default [sqlite3_vfs] object.
** On all subsequent invocations, the pseudo-randomness is generated
** internally and without recourse to the [sqlite3_vfs] xRandomness
** method.
**
** INVARIANTS:
**
** {F17392} The [sqlite3_randomness(N,P)] interface writes N bytes of
**          high-quality pseudo-randomness into buffer P.
*/
void sqlite3_randomness(int N, void *P);

/*
** CAPI3REF: Compile-Time Authorization Callbacks {F12500}
**
** This routine registers a authorizer callback with a particular
** [database connection], supplied in the first argument.
** The authorizer callback is invoked as SQL statements are being compiled
** by [sqlite3_prepare()] or its variants [sqlite3_prepare_v2()],
** [sqlite3_prepare16()] and [sqlite3_prepare16_v2()].  At various
** points during the compilation process, as logic is being created
** to perform various actions, the authorizer callback is invoked to
** see if those actions are allowed.  The authorizer callback should
** return [SQLITE_OK] to allow the action, [SQLITE_IGNORE] to disallow the
** specific action but allow the SQL statement to continue to be
** compiled, or [SQLITE_DENY] to cause the entire SQL statement to be
** rejected with an error.   If the authorizer callback returns
** any value other than [SQLITE_IGNORE], [SQLITE_OK], or [SQLITE_DENY]
** then [sqlite3_prepare_v2()] or equivalent call that triggered
** the authorizer will fail with an error message.
**
** When the callback returns [SQLITE_OK], that means the operation
** requested is ok.  When the callback returns [SQLITE_DENY], the
** [sqlite3_prepare_v2()] or equivalent call that triggered the
** authorizer will fail with an error message explaining that
** access is denied.  If the authorizer code is [SQLITE_READ]
** and the callback returns [SQLITE_IGNORE] then the
** [prepared statement] statement is constructed to substitute
** a NULL value in place of the table column that would have
** been read if [SQLITE_OK] had been returned.  The [SQLITE_IGNORE]
** return can be used to deny an untrusted user access to individual
** columns of a table.
**
** The first parameter to the authorizer callback is a copy of
** the third parameter to the sqlite3_set_authorizer() interface.
** The second parameter to the callback is an integer 
** [SQLITE_COPY | action code] that specifies the particular action
** to be authorized. The third through sixth
** parameters to the callback are zero-terminated strings that contain 
** additional details about the action to be authorized.
**
** An authorizer is used when [sqlite3_prepare | preparing]
** SQL statements from an untrusted
** source, to ensure that the SQL statements do not try to access data
** that they are not allowed to see, or that they do not try to
** execute malicious statements that damage the database.  For
** example, an application may allow a user to enter arbitrary
** SQL queries for evaluation by a database.  But the application does
** not want the user to be able to make arbitrary changes to the
** database.  An authorizer could then be put in place while the
** user-entered SQL is being [sqlite3_prepare | prepared] that
** disallows everything except [SELECT] statements.
**
** Applications that need to process SQL from untrusted sources
** might also consider lowering resource limits using [sqlite3_limit()]
** and limiting database size using the [max_page_count] [PRAGMA]
** in addition to using an authorizer.
**
** Only a single authorizer can be in place on a database connection
** at a time.  Each call to sqlite3_set_authorizer overrides the
** previous call.  Disable the authorizer by installing a NULL callback.
** The authorizer is disabled by default.
**
** Note that the authorizer callback is invoked only during 
** [sqlite3_prepare()] or its variants.  Authorization is not
** performed during statement evaluation in [sqlite3_step()].
**
** INVARIANTS:
**
** {F12501} The [sqlite3_set_authorizer(D,...)] interface registers a
**          authorizer callback with database connection D.
**
** {F12502} The authorizer callback is invoked as SQL statements are
**          being compiled
**
** {F12503} If the authorizer callback returns any value other than
**          [SQLITE_IGNORE], [SQLITE_OK], or [SQLITE_DENY] then
**          the [sqlite3_prepare_v2()] or equivalent call that caused
**          the authorizer callback to run shall fail with an
**          [SQLITE_ERROR] error code and an appropriate error message.
**
** {F12504} When the authorizer callback returns [SQLITE_OK], the operation
**          described is coded normally.
**
** {F12505} When the authorizer callback returns [SQLITE_DENY], the
**          [sqlite3_prepare_v2()] or equivalent call that caused the
**          authorizer callback to run shall fail
**          with an [SQLITE_ERROR] error code and an error message
**          explaining that access is denied.
**
** {F12506} If the authorizer code (the 2nd parameter to the authorizer
**          callback) is [SQLITE_READ] and the authorizer callback returns
**          [SQLITE_IGNORE] then the prepared statement is constructed to
**          insert a NULL value in place of the table column that would have
**          been read if [SQLITE_OK] had been returned.
**
** {F12507} If the authorizer code (the 2nd parameter to the authorizer
**          callback) is anything other than [SQLITE_READ], then
**          a return of [SQLITE_IGNORE] has the same effect as [SQLITE_DENY]. 
**
** {F12510} The first parameter to the authorizer callback is a copy of
**          the third parameter to the [sqlite3_set_authorizer()] interface.
**
** {F12511} The second parameter to the callback is an integer 
**          [SQLITE_COPY | action code] that specifies the particular action
**          to be authorized.
**
** {F12512} The third through sixth parameters to the callback are
**          zero-terminated strings that contain 
**          additional details about the action to be authorized.
**
** {F12520} Each call to [sqlite3_set_authorizer()] overrides the
**          any previously installed authorizer.
**
** {F12521} A NULL authorizer means that no authorization
**          callback is invoked.
**
** {F12522} The default authorizer is NULL.
*/
int sqlite3_set_authorizer(
  sqlite3*,
  int (*xAuth)(void*,int,const char*,const char*,const char*,const char*),
  void *pUserData
);

/*
** CAPI3REF: Authorizer Return Codes {F12590}
**
** The [sqlite3_set_authorizer | authorizer callback function] must
** return either [SQLITE_OK] or one of these two constants in order
** to signal SQLite whether or not the action is permitted.  See the
** [sqlite3_set_authorizer | authorizer documentation] for additional
** information.
*/
#define SQLITE_DENY   1   /* Abort the SQL statement with an error */
#define SQLITE_IGNORE 2   /* Don't allow access, but don't generate an error */

/*
** CAPI3REF: Authorizer Action Codes {F12550}
**
** The [sqlite3_set_authorizer()] interface registers a callback function
** that is invoked to authorizer certain SQL statement actions.  The
** second parameter to the callback is an integer code that specifies
** what action is being authorized.  These are the integer action codes that
** the authorizer callback may be passed.
**
** These action code values signify what kind of operation is to be 
** authorized.  The 3rd and 4th parameters to the authorization
** callback function will be parameters or NULL depending on which of these
** codes is used as the second parameter.  The 5th parameter to the
** authorizer callback is the name of the database ("main", "temp", 
** etc.) if applicable.  The 6th parameter to the authorizer callback
** is the name of the inner-most trigger or view that is responsible for
** the access attempt or NULL if this access attempt is directly from 
** top-level SQL code.
**
** INVARIANTS:
**
** {F12551} The second parameter to an 
**          [sqlite3_set_authorizer | authorizer callback is always an integer
**          [SQLITE_COPY | authorizer code] that specifies what action
**          is being authorized.
**
** {F12552} The 3rd and 4th parameters to the 
**          [sqlite3_set_authorizer | authorization callback function]
**          will be parameters or NULL depending on which 
**          [SQLITE_COPY | authorizer code] is used as the second parameter.
**
** {F12553} The 5th parameter to the
**          [sqlite3_set_authorizer | authorizer callback] is the name
**          of the database (example: "main", "temp", etc.) if applicable.
**
** {F12554} The 6th parameter to the
**          [sqlite3_set_authorizer | authorizer callback] is the name
**          of the inner-most trigger or view that is responsible for
**          the access attempt or NULL if this access attempt is directly from 
**          top-level SQL code.
*/
/******************************************* 3rd ************ 4th ***********/
#define SQLITE_CREATE_INDEX          1   /* Index Name      Table Name      */
#define SQLITE_CREATE_TABLE          2   /* Table Name      NULL            */
#define SQLITE_CREATE_TEMP_INDEX     3   /* Index Name      Table Name      */
#define SQLITE_CREATE_TEMP_TABLE     4   /* Table Name      NULL            */
#define SQLITE_CREATE_TEMP_TRIGGER   5   /* Trigger Name    Table Name      */
#define SQLITE_CREATE_TEMP_VIEW      6   /* View Name       NULL            */
#define SQLITE_CREATE_TRIGGER        7   /* Trigger Name    Table Name      */
#define SQLITE_CREATE_VIEW           8   /* View Name       NULL            */
#define SQLITE_DELETE                9   /* Table Name      NULL            */
#define SQLITE_DROP_INDEX           10   /* Index Name      Table Name      */
#define SQLITE_DROP_TABLE           11   /* Table Name      NULL            */
#define SQLITE_DROP_TEMP_INDEX      12   /* Index Name      Table Name      */
#define SQLITE_DROP_TEMP_TABLE      13   /* Table Name      NULL            */
#define SQLITE_DROP_TEMP_TRIGGER    14   /* Trigger Name    Table Name      */
#define SQLITE_DROP_TEMP_VIEW       15   /* View Name       NULL            */
#define SQLITE_DROP_TRIGGER         16   /* Trigger Name    Table Name      */
#define SQLITE_DROP_VIEW            17   /* View Name       NULL            */
#define SQLITE_INSERT               18   /* Table Name      NULL            */
#define SQLITE_PRAGMA               19   /* Pragma Name     1st arg or NULL */
#define SQLITE_READ                 20   /* Table Name      Column Name     */
#define SQLITE_SELECT               21   /* NULL            NULL            */
#define SQLITE_TRANSACTION          22   /* NULL            NULL            */
#define SQLITE_UPDATE               23   /* Table Name      Column Name     */
#define SQLITE_ATTACH               24   /* Filename        NULL            */
#define SQLITE_DETACH               25   /* Database Name   NULL            */
#define SQLITE_ALTER_TABLE          26   /* Database Name   Table Name      */
#define SQLITE_REINDEX              27   /* Index Name      NULL            */
#define SQLITE_ANALYZE              28   /* Table Name      NULL            */
#define SQLITE_CREATE_VTABLE        29   /* Table Name      Module Name     */
#define SQLITE_DROP_VTABLE          30   /* Table Name      Module Name     */
#define SQLITE_FUNCTION             31   /* Function Name   NULL            */
#define SQLITE_COPY                  0   /* No longer used */

/*
** CAPI3REF: Tracing And Profiling Functions {F12280}
**
** These routines register callback functions that can be used for
** tracing and profiling the execution of SQL statements.
**
** The callback function registered by sqlite3_trace() is invoked at
** various times when an SQL statement is being run by [sqlite3_step()].
** The callback returns a UTF-8 rendering of the SQL statement text
** as the statement first begins executing.  Additional callbacks occur
** as each triggersubprogram is entered.  The callbacks for triggers
** contain a UTF-8 SQL comment that identifies the trigger.
** 
** The callback function registered by sqlite3_profile() is invoked
** as each SQL statement finishes.  The profile callback contains
** the original statement text and an estimate of wall-clock time
** of how long that statement took to run.
**
** The sqlite3_profile() API is currently considered experimental and
** is subject to change or removal in a future release.
**
** The trigger reporting feature of the trace callback is considered
** experimental and is subject to change or removal in future releases.
** Future versions of SQLite might also add new trace callback 
** invocations.
**
** INVARIANTS:
**
** {F12281} The callback function registered by [sqlite3_trace()] is
**          whenever an SQL statement first begins to execute and
**          whenever a trigger subprogram first begins to run.
**
** {F12282} Each call to [sqlite3_trace()] overrides the previously
**          registered trace callback.
**
** {F12283} A NULL trace callback disables tracing.
**
** {F12284} The first argument to the trace callback is a copy of
**          the pointer which was the 3rd argument to [sqlite3_trace()].
**
** {F12285} The second argument to the trace callback is a
**          zero-terminated UTF8 string containing the original text
**          of the SQL statement as it was passed into [sqlite3_prepare_v2()]
**          or the equivalent, or an SQL comment indicating the beginning
**          of a trigger subprogram.
**
** {F12287} The callback function registered by [sqlite3_profile()] is invoked
**          as each SQL statement finishes.
**
** {F12288} The first parameter to the profile callback is a copy of
**          the 3rd parameter to [sqlite3_profile()].
**
** {F12289} The second parameter to the profile callback is a
**          zero-terminated UTF-8 string that contains the complete text of
**          the SQL statement as it was processed by [sqlite3_prepare_v2()]
**          or the equivalent.
**
** {F12290} The third parameter to the profile  callback is an estimate
**          of the number of nanoseconds of wall-clock time required to
**          run the SQL statement from start to finish.
*/
void *sqlite3_trace(sqlite3*, void(*xTrace)(void*,const char*), void*);
void *sqlite3_profile(sqlite3*,
   void(*xProfile)(void*,const char*,sqlite3_uint64), void*);

/*
** CAPI3REF: Query Progress Callbacks {F12910}
**
** This routine configures a callback function - the
** progress callback - that is invoked periodically during long
** running calls to [sqlite3_exec()], [sqlite3_step()] and
** [sqlite3_get_table()].   An example use for this 
** interface is to keep a GUI updated during a large query.
**
** If the progress callback returns non-zero, the opertion is
** interrupted.  This feature can be used to implement a
** "Cancel" button on a GUI dialog box.
**
** INVARIANTS:
**
** {F12911} The callback function registered by [sqlite3_progress_handler()]
**          is invoked periodically during long running calls to
**          [sqlite3_step()].
**
** {F12912} The progress callback is invoked once for every N virtual
**          machine opcodes, where N is the second argument to 
**          the [sqlite3_progress_handler()] call that registered
**          the callback.  <todo>What if N is less than 1?</todo>
**
** {F12913} The progress callback itself is identified by the third
**          argument to [sqlite3_progress_handler()].
**
** {F12914} The fourth argument [sqlite3_progress_handler()] is a
***         void pointer passed to the progress callback
**          function each time it is invoked.
**
** {F12915} If a call to [sqlite3_step()] results in fewer than
**          N opcodes being executed,
**          then the progress callback is never invoked. {END}
** 
** {F12916} Every call to [sqlite3_progress_handler()]
**          overwrites any previously registere progress handler.
**
** {F12917} If the progress handler callback is NULL then no progress
**          handler is invoked.
**
** {F12918} If the progress callback returns a result other than 0, then
**          the behavior is a if [sqlite3_interrupt()] had been called.
*/
void sqlite3_progress_handler(sqlite3*, int, int(*)(void*), void*);

/*
** CAPI3REF: Opening A New Database Connection {F12700}
**
** These routines open an SQLite database file whose name
** is given by the filename argument.
** The filename argument is interpreted as UTF-8
** for [sqlite3_open()] and [sqlite3_open_v2()] and as UTF-16
** in the native byte order for [sqlite3_open16()].
** An [sqlite3*] handle is usually returned in *ppDb, even
** if an error occurs.  The only exception is if SQLite is unable
** to allocate memory to hold the [sqlite3] object, a NULL will
** be written into *ppDb instead of a pointer to the [sqlite3] object.
** If the database is opened (and/or created)
** successfully, then [SQLITE_OK] is returned.  Otherwise an
** error code is returned.  The
** [sqlite3_errmsg()] or [sqlite3_errmsg16()]  routines can be used to obtain
** an English language description of the error.
**
** The default encoding for the database will be UTF-8 if
** [sqlite3_open()] or [sqlite3_open_v2()] is called and
** UTF-16 in the native byte order if [sqlite3_open16()] is used.
**
** Whether or not an error occurs when it is opened, resources
** associated with the [sqlite3*] handle should be released by passing it
** to [sqlite3_close()] when it is no longer required.
**
** The [sqlite3_open_v2()] interface works like [sqlite3_open()] 
** except that it acccepts two additional parameters for additional control
** over the new database connection.  The flags parameter can be
** one of:
**
** <ol>
** <li>  [SQLITE_OPEN_READONLY]
** <li>  [SQLITE_OPEN_READWRITE]
** <li>  [SQLITE_OPEN_READWRITE] | [SQLITE_OPEN_CREATE]
** </ol>
**
** The first value opens the database read-only. 
** If the database does not previously exist, an error is returned.
** The second option opens
** the database for reading and writing if possible, or reading only if
** if the file is write protected.  In either case the database
** must already exist or an error is returned.  The third option
** opens the database for reading and writing and creates it if it does
** not already exist.
** The third options is behavior that is always used for [sqlite3_open()]
** and [sqlite3_open16()].
**
** If the 3rd parameter to [sqlite3_open_v2()] is not one of the
** combinations shown above then the behavior is undefined.
**
** If the filename is ":memory:", then an private
** in-memory database is created for the connection.  This in-memory
** database will vanish when the database connection is closed.  Future
** version of SQLite might make use of additional special filenames
** that begin with the ":" character.  It is recommended that 
** when a database filename really does begin with
** ":" that you prefix the filename with a pathname like "./" to
** avoid ambiguity.
**
** If the filename is an empty string, then a private temporary
** on-disk database will be created.  This private database will be
** automatically deleted as soon as the database connection is closed.
**
** The fourth parameter to sqlite3_open_v2() is the name of the
** [sqlite3_vfs] object that defines the operating system 
** interface that the new database connection should use.  If the
** fourth parameter is a NULL pointer then the default [sqlite3_vfs]
** object is used.
**
** <b>Note to windows users:</b>  The encoding used for the filename argument
** of [sqlite3_open()] and [sqlite3_open_v2()] must be UTF-8, not whatever
** codepage is currently defined.  Filenames containing international
** characters must be converted to UTF-8 prior to passing them into
** [sqlite3_open()] or [sqlite3_open_v2()].
**
** INVARIANTS:
**
** {F12701} The [sqlite3_open()], [sqlite3_open16()], and
**          [sqlite3_open_v2()] interfaces create a new
**          [database connection] associated with
**          the database file given in their first parameter.
**
** {F12702} The filename argument is interpreted as UTF-8
**          for [sqlite3_open()] and [sqlite3_open_v2()] and as UTF-16
**          in the native byte order for [sqlite3_open16()].
**
** {F12703} A successful invocation of [sqlite3_open()], [sqlite3_open16()], 
**          or [sqlite3_open_v2()] writes a pointer to a new
**          [database connection] into *ppDb.
**
** {F12704} The [sqlite3_open()], [sqlite3_open16()], and
**          [sqlite3_open_v2()] interfaces return [SQLITE_OK] upon success,
**          or an appropriate [error code] on failure.
**
** {F12706} The default text encoding for a new database created using
**          [sqlite3_open()] or [sqlite3_open_v2()] will be UTF-8.
**
** {F12707} The default text encoding for a new database created using
**          [sqlite3_open16()] will be UTF-16.
**
** {F12709} The [sqlite3_open(F,D)] interface is equivalent to
**          [sqlite3_open_v2(F,D,G,0)] where the G parameter is
**          [SQLITE_OPEN_READWRITE]|[SQLITE_OPEN_CREATE].
**
** {F12711} If the G parameter to [sqlite3_open_v2(F,D,G,V)] contains the
**          bit value [SQLITE_OPEN_READONLY] then the database is opened
**          for reading only.
**
** {F12712} If the G parameter to [sqlite3_open_v2(F,D,G,V)] contains the
**          bit value [SQLITE_OPEN_READWRITE] then the database is opened
**          reading and writing if possible, or for reading only if the
**          file is write protected by the operating system.
**
** {F12713} If the G parameter to [sqlite3_open(v2(F,D,G,V)] omits the
**          bit value [SQLITE_OPEN_CREATE] and the database does not
**          previously exist, an error is returned.
**
** {F12714} If the G parameter to [sqlite3_open(v2(F,D,G,V)] contains the
**          bit value [SQLITE_OPEN_CREATE] and the database does not
**          previously exist, then an attempt is made to create and
**          initialize the database.
**
** {F12717} If the filename argument to [sqlite3_open()], [sqlite3_open16()],
**          or [sqlite3_open_v2()] is ":memory:", then an private,
**          ephemeral, in-memory database is created for the connection.
**          <todo>Is SQLITE_OPEN_CREATE|SQLITE_OPEN_READWRITE required
**          in sqlite3_open_v2()?</todo>
**
** {F12719} If the filename is NULL or an empty string, then a private,
**          ephermeral on-disk database will be created.
**          <todo>Is SQLITE_OPEN_CREATE|SQLITE_OPEN_READWRITE required
**          in sqlite3_open_v2()?</todo>
**
** {F12721} The [database connection] created by 
**          [sqlite3_open_v2(F,D,G,V)] will use the
**          [sqlite3_vfs] object identified by the V parameter, or
**          the default [sqlite3_vfs] object is V is a NULL pointer.
*/
int sqlite3_open(
  const char *filename,   /* Database filename (UTF-8) */
  sqlite3 **ppDb          /* OUT: SQLite db handle */
);
int sqlite3_open16(
  const void *filename,   /* Database filename (UTF-16) */
  sqlite3 **ppDb          /* OUT: SQLite db handle */
);
int sqlite3_open_v2(
  const char *filename,   /* Database filename (UTF-8) */
  sqlite3 **ppDb,         /* OUT: SQLite db handle */
  int flags,              /* Flags */
  const char *zVfs        /* Name of VFS module to use */
);

/*
** CAPI3REF: Error Codes And Messages {F12800}
**
** The sqlite3_errcode() interface returns the numeric
** [SQLITE_OK | result code] or [SQLITE_IOERR_READ | extended result code]
** for the most recent failed sqlite3_* API call associated
** with [sqlite3] handle 'db'. If a prior API call failed but the
** most recent API call succeeded, the return value from sqlite3_errcode()
** is undefined.
**
** The sqlite3_errmsg() and sqlite3_errmsg16() return English-language
** text that describes the error, as either UTF8 or UTF16 respectively.
** Memory to hold the error message string is managed internally.
** The application does not need to worry with freeing the result.
** However, the error string might be overwritten or deallocated by
** subsequent calls to other SQLite interface functions.
**
** INVARIANTS:
**
** {F12801} The [sqlite3_errcode(D)] interface returns the numeric
**          [SQLITE_OK | result code] or
**          [SQLITE_IOERR_READ | extended result code]
**          for the most recently failed interface call associated
**          with [database connection] D.
**
** {F12803} The [sqlite3_errmsg(D)] and [sqlite3_errmsg16(D)]
**          interfaces return English-language text that describes
**          the error in the mostly recently failed interface call,
**          encoded as either UTF8 or UTF16 respectively.
**
** {F12807} The strings returned by [sqlite3_errmsg()] and [sqlite3_errmsg16()]
**          are valid until the next SQLite interface call.
**
** {F12808} Calls to API routines that do not return an error code
**          (example: [sqlite3_data_count()]) do not
**          change the error code or message returned by
**          [sqlite3_errcode()], [sqlite3_errmsg()], or [sqlite3_errmsg16()].
**
** {F12809} Interfaces that are not associated with a specific
**          [database connection] (examples:
**          [sqlite3_mprintf()] or [sqlite3_enable_shared_cache()]
**          do not change the values returned by
**          [sqlite3_errcode()], [sqlite3_errmsg()], or [sqlite3_errmsg16()].
*/
int sqlite3_errcode(sqlite3 *db);
const char *sqlite3_errmsg(sqlite3*);
const void *sqlite3_errmsg16(sqlite3*);

/*
** CAPI3REF: SQL Statement Object {F13000}
** KEYWORDS: {prepared statement} {prepared statements}
**
** An instance of this object represent single SQL statements.  This
** object is variously known as a "prepared statement" or a 
** "compiled SQL statement" or simply as a "statement".
** 
** The life of a statement object goes something like this:
**
** <ol>
** <li> Create the object using [sqlite3_prepare_v2()] or a related
**      function.
** <li> Bind values to host parameters using
**      [sqlite3_bind_blob | sqlite3_bind_* interfaces].
** <li> Run the SQL by calling [sqlite3_step()] one or more times.
** <li> Reset the statement using [sqlite3_reset()] then go back
**      to step 2.  Do this zero or more times.
** <li> Destroy the object using [sqlite3_finalize()].
** </ol>
**
** Refer to documentation on individual methods above for additional
** information.
*/
typedef struct sqlite3_stmt sqlite3_stmt;

/*
** CAPI3REF: Run-time Limits {F12760}
**
** This interface allows the size of various constructs to be limited
** on a connection by connection basis.  The first parameter is the
** [database connection] whose limit is to be set or queried.  The
** second parameter is one of the [limit categories] that define a
** class of constructs to be size limited.  The third parameter is the
** new limit for that construct.  The function returns the old limit.
**
** If the new limit is a negative number, the limit is unchanged.
** For the limit category of SQLITE_LIMIT_XYZ there is a hard upper
** bound set by a compile-time C-preprocess macro named SQLITE_MAX_XYZ.
** (The "_LIMIT_" in the name is changed to "_MAX_".)
** Attempts to increase a limit above its hard upper bound are
** silently truncated to the hard upper limit.
**
** Run time limits are intended for use in applications that manage
** both their own internal database and also databases that are controlled
** by untrusted external sources.  An example application might be a
** webbrowser that has its own databases for storing history and
** separate databases controlled by javascript applications downloaded
** off the internet.  The internal databases can be given the
** large, default limits.  Databases managed by external sources can
** be given much smaller limits designed to prevent a denial of service
** attach.  Developers might also want to use the [sqlite3_set_authorizer()]
** interface to further control untrusted SQL.  The size of the database
** created by an untrusted script can be contained using the
** [max_page_count] [PRAGMA].
**
** This interface is currently considered experimental and is subject
** to change or removal without prior notice.
**
** INVARIANTS:
**
** {F12762} A successful call to [sqlite3_limit(D,C,V)] where V is
**          positive changes the
**          limit on the size of construct C in [database connection] D
**          to the lessor of V and the hard upper bound on the size
**          of C that is set at compile-time.
**
** {F12766} A successful call to [sqlite3_limit(D,C,V)] where V is negative
**          leaves the state of [database connection] D unchanged.
**
** {F12769} A successful call to [sqlite3_limit(D,C,V)] returns the
**          value of the limit on the size of construct C in
**          in [database connection] D as it was prior to the call.
*/
int sqlite3_limit(sqlite3*, int id, int newVal);

/*
** CAPI3REF: Run-Time Limit Categories {F12790}
** KEYWORDS: {limit category} {limit categories}
** 
** These constants define various aspects of a [database connection]
** that can be limited in size by calls to [sqlite3_limit()].
** The meanings of the various limits are as follows:
**
** <dl>
** <dt>SQLITE_LIMIT_LENGTH</dt>
** <dd>The maximum size of any
** string or blob or table row.<dd>
**
** <dt>SQLITE_LIMIT_SQL_LENGTH</dt>
** <dd>The maximum length of an SQL statement.</dd>
**
** <dt>SQLITE_LIMIT_COLUMN</dt>
** <dd>The maximum number of columns in a table definition or in the
** result set of a SELECT or the maximum number of columns in an index
** or in an ORDER BY or GROUP BY clause.</dd>
**
** <dt>SQLITE_LIMIT_EXPR_DEPTH</dt>
** <dd>The maximum depth of the parse tree on any expression.</dd>
**
** <dt>SQLITE_LIMIT_COMPOUND_SELECT</dt>
** <dd>The maximum number of terms in a compound SELECT statement.</dd>
**
** <dt>SQLITE_LIMIT_VDBE_OP</dt>
** <dd>The maximum number of instructions in a virtual machine program
** used to implement an SQL statement.</dd>
**
** <dt>SQLITE_LIMIT_FUNCTION_ARG</dt>
** <dd>The maximum number of arguments on a function.</dd>
**
** <dt>SQLITE_LIMIT_ATTACHED</dt>
** <dd>The maximum number of attached databases.</dd>
**
** <dt>SQLITE_LIMIT_LIKE_PATTERN_LENGTH</dt>
** <dd>The maximum length of the pattern argument to the LIKE or
** GLOB operators.</dd>
**
** <dt>SQLITE_LIMIT_VARIABLE_NUMBER</dt>
** <dd>The maximum number of variables in an SQL statement that can
** be bound.</dd>
** </dl>
*/
#define SQLITE_LIMIT_LENGTH                    0
#define SQLITE_LIMIT_SQL_LENGTH                1
#define SQLITE_LIMIT_COLUMN                    2
#define SQLITE_LIMIT_EXPR_DEPTH                3
#define SQLITE_LIMIT_COMPOUND_SELECT           4
#define SQLITE_LIMIT_VDBE_OP                   5
#define SQLITE_LIMIT_FUNCTION_ARG              6
#define SQLITE_LIMIT_ATTACHED                  7
#define SQLITE_LIMIT_LIKE_PATTERN_LENGTH       8
#define SQLITE_LIMIT_VARIABLE_NUMBER           9

/*
** CAPI3REF: Compiling An SQL Statement {F13010}
**
** To execute an SQL query, it must first be compiled into a byte-code
** program using one of these routines. 
**
** The first argument "db" is an [database connection] 
** obtained from a prior call to [sqlite3_open()], [sqlite3_open_v2()]
** or [sqlite3_open16()]. 
** The second argument "zSql" is the statement to be compiled, encoded
** as either UTF-8 or UTF-16.  The sqlite3_prepare() and sqlite3_prepare_v2()
** interfaces uses UTF-8 and sqlite3_prepare16() and sqlite3_prepare16_v2()
** use UTF-16. {END}
**
** If the nByte argument is less
** than zero, then zSql is read up to the first zero terminator.
** If nByte is non-negative, then it is the maximum number of 
** bytes read from zSql.  When nByte is non-negative, the
** zSql string ends at either the first '\000' or '\u0000' character or 
** the nByte-th byte, whichever comes first. If the caller knows
** that the supplied string is nul-terminated, then there is a small
** performance advantage to be had by passing an nByte parameter that 
** is equal to the number of bytes in the input string <i>including</i> 
** the nul-terminator bytes.{END}
**
** *pzTail is made to point to the first byte past the end of the
** first SQL statement in zSql.  These routines only compiles the first
** statement in zSql, so *pzTail is left pointing to what remains
** uncompiled.
**
** *ppStmt is left pointing to a compiled [prepared statement] that can be
** executed using [sqlite3_step()].  Or if there is an error, *ppStmt is
** set to NULL.  If the input text contains no SQL (if the input
** is and empty string or a comment) then *ppStmt is set to NULL.
** {U13018} The calling procedure is responsible for deleting the
** compiled SQL statement
** using [sqlite3_finalize()] after it has finished with it.
**
** On success, [SQLITE_OK] is returned.  Otherwise an 
** [error code] is returned.
**
** The sqlite3_prepare_v2() and sqlite3_prepare16_v2() interfaces are
** recommended for all new programs. The two older interfaces are retained
** for backwards compatibility, but their use is discouraged.
** In the "v2" interfaces, the prepared statement
** that is returned (the [sqlite3_stmt] object) contains a copy of the 
** original SQL text. {END} This causes the [sqlite3_step()] interface to
** behave a differently in two ways:
**
** <ol>
** <li>
** If the database schema changes, instead of returning [SQLITE_SCHEMA] as it
** always used to do, [sqlite3_step()] will automatically recompile the SQL
** statement and try to run it again.  If the schema has changed in
** a way that makes the statement no longer valid, [sqlite3_step()] will still
** return [SQLITE_SCHEMA].  But unlike the legacy behavior, 
** [SQLITE_SCHEMA] is now a fatal error.  Calling
** [sqlite3_prepare_v2()] again will not make the
** error go away.  Note: use [sqlite3_errmsg()] to find the text
** of the parsing error that results in an [SQLITE_SCHEMA] return. {END}
** </li>
**
** <li>
** When an error occurs, 
** [sqlite3_step()] will return one of the detailed 
** [error codes] or [extended error codes]. 
** The legacy behavior was that [sqlite3_step()] would only return a generic
** [SQLITE_ERROR] result code and you would have to make a second call to
** [sqlite3_reset()] in order to find the underlying cause of the problem.
** With the "v2" prepare interfaces, the underlying reason for the error is
** returned immediately.
** </li>
** </ol>
**
** INVARIANTS:
**
** {F13011} The [sqlite3_prepare(db,zSql,...)] and
**          [sqlite3_prepare_v2(db,zSql,...)] interfaces interpret the
**          text in their zSql parameter as UTF-8.
**
** {F13012} The [sqlite3_prepare16(db,zSql,...)] and
**          [sqlite3_prepare16_v2(db,zSql,...)] interfaces interpret the
**          text in their zSql parameter as UTF-16 in the native byte order.
**
** {F13013} If the nByte argument to [sqlite3_prepare_v2(db,zSql,nByte,...)]
**          and its variants is less than zero, then SQL text is
**          read from zSql is read up to the first zero terminator.
**
** {F13014} If the nByte argument to [sqlite3_prepare_v2(db,zSql,nByte,...)]
**          and its variants is non-negative, then at most nBytes bytes
**          SQL text is read from zSql.
**
** {F13015} In [sqlite3_prepare_v2(db,zSql,N,P,pzTail)] and its variants
**          if the zSql input text contains more than one SQL statement
**          and pzTail is not NULL, then *pzTail is made to point to the
**          first byte past the end of the first SQL statement in zSql.
**          <todo>What does *pzTail point to if there is one statement?</todo>
**
** {F13016} A successful call to [sqlite3_prepare_v2(db,zSql,N,ppStmt,...)]
**          or one of its variants writes into *ppStmt a pointer to a new
**          [prepared statement] or a pointer to NULL
**          if zSql contains nothing other than whitespace or comments. 
**
** {F13019} The [sqlite3_prepare_v2()] interface and its variants return
**          [SQLITE_OK] or an appropriate [error code] upon failure.
**
** {F13021} Before [sqlite3_prepare(db,zSql,nByte,ppStmt,pzTail)] or its
**          variants returns an error (any value other than [SQLITE_OK])
**          it first sets *ppStmt to NULL.
*/
int sqlite3_prepare(
  sqlite3 *db,            /* Database handle */
  const char *zSql,       /* SQL statement, UTF-8 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
  const char **pzTail     /* OUT: Pointer to unused portion of zSql */
);
int sqlite3_prepare_v2(
  sqlite3 *db,            /* Database handle */
  const char *zSql,       /* SQL statement, UTF-8 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
  const char **pzTail     /* OUT: Pointer to unused portion of zSql */
);
int sqlite3_prepare16(
  sqlite3 *db,            /* Database handle */
  const void *zSql,       /* SQL statement, UTF-16 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
  const void **pzTail     /* OUT: Pointer to unused portion of zSql */
);
int sqlite3_prepare16_v2(
  sqlite3 *db,            /* Database handle */
  const void *zSql,       /* SQL statement, UTF-16 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
  const void **pzTail     /* OUT: Pointer to unused portion of zSql */
);

/*
** CAPIREF: Retrieving Statement SQL {F13100}
**
** This intereface can be used to retrieve a saved copy of the original
** SQL text used to create a [prepared statement].
**
** INVARIANTS:
**
** {F13101} If the [prepared statement] passed as 
**          the an argument to [sqlite3_sql()] was compiled
**          compiled using either [sqlite3_prepare_v2()] or
**          [sqlite3_prepare16_v2()],
**          then [sqlite3_sql()] function returns a pointer to a
**          zero-terminated string containing a UTF-8 rendering
**          of the original SQL statement.
**
** {F13102} If the [prepared statement] passed as 
**          the an argument to [sqlite3_sql()] was compiled
**          compiled using either [sqlite3_prepare()] or
**          [sqlite3_prepare16()],
**          then [sqlite3_sql()] function returns a NULL pointer.
**
** {F13103} The string returned by [sqlite3_sql(S)] is valid until the
**          [prepared statement] S is deleted using [sqlite3_finalize(S)].
*/
const char *sqlite3_sql(sqlite3_stmt *pStmt);

/*
** CAPI3REF:  Dynamically Typed Value Object  {F15000}
** KEYWORDS: {protected sqlite3_value} {unprotected sqlite3_value}
**
** SQLite uses the sqlite3_value object to represent all values
** that can be stored in a database table.
** SQLite uses dynamic typing for the values it stores.  
** Values stored in sqlite3_value objects can be
** be integers, floating point values, strings, BLOBs, or NULL.
**
** An sqlite3_value object may be either "protected" or "unprotected".
** Some interfaces require a protected sqlite3_value.  Other interfaces
** will accept either a protected or an unprotected sqlite3_value.
** Every interface that accepts sqlite3_value arguments specifies 
** whether or not it requires a protected sqlite3_value.
**
** The terms "protected" and "unprotected" refer to whether or not
** a mutex is held.  A internal mutex is held for a protected
** sqlite3_value object but no mutex is held for an unprotected
** sqlite3_value object.  If SQLite is compiled to be single-threaded
** (with SQLITE_THREADSAFE=0 and with [sqlite3_threadsafe()] returning 0)
** then there is no distinction between
** protected and unprotected sqlite3_value objects and they can be
** used interchangable.  However, for maximum code portability it
** is recommended that applications make the distinction between
** between protected and unprotected sqlite3_value objects even if
** they are single threaded.
**
** The sqlite3_value objects that are passed as parameters into the
** implementation of application-defined SQL functions are protected.
** The sqlite3_value object returned by
** [sqlite3_column_value()] is unprotected.
** Unprotected sqlite3_value objects may only be used with
** [sqlite3_result_value()] and [sqlite3_bind_value()].  All other
** interfaces that use sqlite3_value require protected sqlite3_value objects.
*/
typedef struct Mem sqlite3_value;

/*
** CAPI3REF:  SQL Function Context Object {F16001}
**
** The context in which an SQL function executes is stored in an
** sqlite3_context object.  A pointer to an sqlite3_context
** object is always first parameter to application-defined SQL functions.
*/
typedef struct sqlite3_context sqlite3_context;

/*
** CAPI3REF:  Binding Values To Prepared Statements {F13500}
**
** In the SQL strings input to [sqlite3_prepare_v2()] and its
** variants, literals may be replace by a parameter in one
** of these forms:
**
** <ul>
** <li>  ?
** <li>  ?NNN
** <li>  :VVV
** <li>  @VVV
** <li>  $VVV
** </ul>
**
** In the parameter forms shown above NNN is an integer literal,
** VVV alpha-numeric parameter name.
** The values of these parameters (also called "host parameter names"
** or "SQL parameters")
** can be set using the sqlite3_bind_*() routines defined here.
**
** The first argument to the sqlite3_bind_*() routines always
** is a pointer to the [sqlite3_stmt] object returned from
** [sqlite3_prepare_v2()] or its variants. The second
** argument is the index of the parameter to be set. The
** first parameter has an index of 1.  When the same named
** parameter is used more than once, second and subsequent
** occurrences have the same index as the first occurrence. 
** The index for named parameters can be looked up using the
** [sqlite3_bind_parameter_name()] API if desired.  The index
** for "?NNN" parameters is the value of NNN.
** The NNN value must be between 1 and the compile-time
** parameter SQLITE_MAX_VARIABLE_NUMBER (default value: 999).
**
** The third argument is the value to bind to the parameter.
**
** In those
** routines that have a fourth argument, its value is the number of bytes
** in the parameter.  To be clear: the value is the number of <u>bytes</u>
** in the value, not the number of characters. 
** If the fourth parameter is negative, the length of the string is
** number of bytes up to the first zero terminator.
**
** The fifth argument to sqlite3_bind_blob(), sqlite3_bind_text(), and
** sqlite3_bind_text16() is a destructor used to dispose of the BLOB or
** string after SQLite has finished with it. If the fifth argument is
** the special value [SQLITE_STATIC], then SQLite assumes that the
** information is in static, unmanaged space and does not need to be freed.
** If the fifth argument has the value [SQLITE_TRANSIENT], then
** SQLite makes its own private copy of the data immediately, before
** the sqlite3_bind_*() routine returns.
**
** The sqlite3_bind_zeroblob() routine binds a BLOB of length N that
** is filled with zeros.  A zeroblob uses a fixed amount of memory
** (just an integer to hold it size) while it is being processed.
** Zeroblobs are intended to serve as place-holders for BLOBs whose
** content is later written using 
** [sqlite3_blob_open | increment BLOB I/O] routines. A negative
** value for the zeroblob results in a zero-length BLOB.
**
** The sqlite3_bind_*() routines must be called after
** [sqlite3_prepare_v2()] (and its variants) or [sqlite3_reset()] and
** before [sqlite3_step()].
** Bindings are not cleared by the [sqlite3_reset()] routine.
** Unbound parameters are interpreted as NULL.
**
** These routines return [SQLITE_OK] on success or an error code if
** anything goes wrong.  [SQLITE_RANGE] is returned if the parameter
** index is out of range.  [SQLITE_NOMEM] is returned if malloc fails.
** [SQLITE_MISUSE] might be returned if these routines are called on a
** virtual machine that is the wrong state or which has already been finalized.
** Detection of misuse is unreliable.  Applications should not depend
** on SQLITE_MISUSE returns.  SQLITE_MISUSE is intended to indicate a
** a logic error in the application.  Future versions of SQLite might
** panic rather than return SQLITE_MISUSE.
**
** See also: [sqlite3_bind_parameter_count()],
** [sqlite3_bind_parameter_name()], and
** [sqlite3_bind_parameter_index()].
**
** INVARIANTS:
**
** {F13506} The [sqlite3_prepare | SQL statement compiler] recognizes
**          tokens of the forms "?", "?NNN", "$VVV", ":VVV", and "@VVV"
**          as SQL parameters, where NNN is any sequence of one or more
**          digits and where VVV is any sequence of one or more 
**          alphanumeric characters or "::" optionally followed by
**          a string containing no spaces and contained within parentheses.
**
** {F13509} The initial value of an SQL parameter is NULL.
**
** {F13512} The index of an "?" SQL parameter is one larger than the
**          largest index of SQL parameter to the left, or 1 if
**          the "?" is the leftmost SQL parameter.
**
** {F13515} The index of an "?NNN" SQL parameter is the integer NNN.
**
** {F13518} The index of an ":VVV", "$VVV", or "@VVV" SQL parameter is
**          the same as the index of leftmost occurances of the same
**          parameter, or one more than the largest index over all
**          parameters to the left if this is the first occurrance
**          of this parameter, or 1 if this is the leftmost parameter.
**
** {F13521} The [sqlite3_prepare | SQL statement compiler] fail with
**          an [SQLITE_RANGE] error if the index of an SQL parameter
**          is less than 1 or greater than SQLITE_MAX_VARIABLE_NUMBER.
**
** {F13524} Calls to [sqlite3_bind_text | sqlite3_bind(S,N,V,...)]
**          associate the value V with all SQL parameters having an
**          index of N in the [prepared statement] S.
**
** {F13527} Calls to [sqlite3_bind_text | sqlite3_bind(S,N,...)]
**          override prior calls with the same values of S and N.
**
** {F13530} Bindings established by [sqlite3_bind_text | sqlite3_bind(S,...)]
**          persist across calls to [sqlite3_reset(S)].
**
** {F13533} In calls to [sqlite3_bind_blob(S,N,V,L,D)],
**          [sqlite3_bind_text(S,N,V,L,D)], or
**          [sqlite3_bind_text16(S,N,V,L,D)] SQLite binds the first L
**          bytes of the blob or string pointed to by V, when L
**          is non-negative.
**
** {F13536} In calls to [sqlite3_bind_text(S,N,V,L,D)] or
**          [sqlite3_bind_text16(S,N,V,L,D)] SQLite binds characters
**          from V through the first zero character when L is negative.
**
** {F13539} In calls to [sqlite3_bind_blob(S,N,V,L,D)],
**          [sqlite3_bind_text(S,N,V,L,D)], or
**          [sqlite3_bind_text16(S,N,V,L,D)] when D is the special
**          constant [SQLITE_STATIC], SQLite assumes that the value V
**          is held in static unmanaged space that will not change
**          during the lifetime of the binding.
**
** {F13542} In calls to [sqlite3_bind_blob(S,N,V,L,D)],
**          [sqlite3_bind_text(S,N,V,L,D)], or
**          [sqlite3_bind_text16(S,N,V,L,D)] when D is the special
**          constant [SQLITE_TRANSIENT], the routine makes a 
**          private copy of V value before it returns.
**
** {F13545} In calls to [sqlite3_bind_blob(S,N,V,L,D)],
**          [sqlite3_bind_text(S,N,V,L,D)], or
**          [sqlite3_bind_text16(S,N,V,L,D)] when D is a pointer to
**          a function, SQLite invokes that function to destroy the
**          V value after it has finished using the V value.
**
** {F13548} In calls to [sqlite3_bind_zeroblob(S,N,V,L)] the value bound
**          is a blob of L bytes, or a zero-length blob if L is negative.
**
** {F13551} In calls to [sqlite3_bind_value(S,N,V)] the V argument may
**          be either a [protected sqlite3_value] object or an
**          [unprotected sqlite3_value] object.
*/
int sqlite3_bind_blob(sqlite3_stmt*, int, const void*, int n, void(*)(void*));
int sqlite3_bind_double(sqlite3_stmt*, int, double);
int sqlite3_bind_int(sqlite3_stmt*, int, int);
int sqlite3_bind_int64(sqlite3_stmt*, int, sqlite3_int64);
int sqlite3_bind_null(sqlite3_stmt*, int);
int sqlite3_bind_text(sqlite3_stmt*, int, const char*, int n, void(*)(void*));
int sqlite3_bind_text16(sqlite3_stmt*, int, const void*, int, void(*)(void*));
int sqlite3_bind_value(sqlite3_stmt*, int, const sqlite3_value*);
int sqlite3_bind_zeroblob(sqlite3_stmt*, int, int n);

/*
** CAPI3REF: Number Of SQL Parameters {F13600}
**
** This routine can be used to find the number of SQL parameters
** in a prepared statement.  SQL parameters are tokens of the
** form "?", "?NNN", ":AAA", "$AAA", or "@AAA" that serve as
** place-holders for values that are [sqlite3_bind_blob | bound]
** to the parameters at a later time.
**
** This routine actually returns the index of the largest parameter.
** For all forms except ?NNN, this will correspond to the number of
** unique parameters.  If parameters of the ?NNN are used, there may
** be gaps in the list.
**
** See also: [sqlite3_bind_blob|sqlite3_bind()],
** [sqlite3_bind_parameter_name()], and
** [sqlite3_bind_parameter_index()].
**
** INVARIANTS:
**
** {F13601} The [sqlite3_bind_parameter_count(S)] interface returns
**          the largest index of all SQL parameters in the
**          [prepared statement] S, or 0 if S
**          contains no SQL parameters.
*/
int sqlite3_bind_parameter_count(sqlite3_stmt*);

/*
** CAPI3REF: Name Of A Host Parameter {F13620}
**
** This routine returns a pointer to the name of the n-th
** SQL parameter in a [prepared statement].
** SQL parameters of the form "?NNN" or ":AAA" or "@AAA" or "$AAA"
** have a name which is the string "?NNN" or ":AAA" or "@AAA" or "$AAA"
** respectively.
** In other words, the initial ":" or "$" or "@" or "?"
** is included as part of the name.
** Parameters of the form "?" without a following integer have no name.
**
** The first host parameter has an index of 1, not 0.
**
** If the value n is out of range or if the n-th parameter is
** nameless, then NULL is returned.  The returned string is
** always in the UTF-8 encoding even if the named parameter was
** originally specified as UTF-16 in [sqlite3_prepare16()] or
** [sqlite3_prepare16_v2()].
**
** See also: [sqlite3_bind_blob|sqlite3_bind()],
** [sqlite3_bind_parameter_count()], and
** [sqlite3_bind_parameter_index()].
**
** INVARIANTS:
**
** {F13621} The [sqlite3_bind_parameter_name(S,N)] interface returns
**          a UTF-8 rendering of the name of the SQL parameter in
**          [prepared statement] S having index N, or
**          NULL if there is no SQL parameter with index N or if the
**          parameter with index N is an anonymous parameter "?".
*/
const char *sqlite3_bind_parameter_name(sqlite3_stmt*, int);

/*
** CAPI3REF: Index Of A Parameter With A Given Name {F13640}
**
** Return the index of an SQL parameter given its name.  The
** index value returned is suitable for use as the second
** parameter to [sqlite3_bind_blob|sqlite3_bind()].  A zero
** is returned if no matching parameter is found.  The parameter
** name must be given in UTF-8 even if the original statement
** was prepared from UTF-16 text using [sqlite3_prepare16_v2()].
**
** See also: [sqlite3_bind_blob|sqlite3_bind()],
** [sqlite3_bind_parameter_count()], and
** [sqlite3_bind_parameter_index()].
**
** INVARIANTS:
**
** {F13641} The [sqlite3_bind_parameter_index(S,N)] interface returns
**          the index of SQL parameter in [prepared statement]
**          S whose name matches the UTF-8 string N, or 0 if there is
**          no match.
*/
int sqlite3_bind_parameter_index(sqlite3_stmt*, const char *zName);

/*
** CAPI3REF: Reset All Bindings On A Prepared Statement {F13660}
**
** Contrary to the intuition of many, [sqlite3_reset()] does not
** reset the [sqlite3_bind_blob | bindings] on a 
** [prepared statement].  Use this routine to
** reset all host parameters to NULL.
**
** INVARIANTS:
**
** {F13661} The [sqlite3_clear_bindings(S)] interface resets all
**          SQL parameter bindings in [prepared statement] S
**          back to NULL.
*/
int sqlite3_clear_bindings(sqlite3_stmt*);

/*
** CAPI3REF: Number Of Columns In A Result Set {F13710}
**
** Return the number of columns in the result set returned by the 
** [prepared statement]. This routine returns 0
** if pStmt is an SQL statement that does not return data (for 
** example an UPDATE).
**
** INVARIANTS:
**
** {F13711} The [sqlite3_column_count(S)] interface returns the number of
**          columns in the result set generated by the
**          [prepared statement] S, or 0 if S does not generate
**          a result set.
*/
int sqlite3_column_count(sqlite3_stmt *pStmt);

/*
** CAPI3REF: Column Names In A Result Set {F13720}
**
** These routines return the name assigned to a particular column
** in the result set of a SELECT statement.  The sqlite3_column_name()
** interface returns a pointer to a zero-terminated UTF8 string
** and sqlite3_column_name16() returns a pointer to a zero-terminated
** UTF16 string.  The first parameter is the
** [prepared statement] that implements the SELECT statement.
** The second parameter is the column number.  The left-most column is
** number 0.
**
** The returned string pointer is valid until either the 
** [prepared statement] is destroyed by [sqlite3_finalize()]
** or until the next call sqlite3_column_name() or sqlite3_column_name16()
** on the same column.
**
** If sqlite3_malloc() fails during the processing of either routine
** (for example during a conversion from UTF-8 to UTF-16) then a
** NULL pointer is returned.
**
** The name of a result column is the value of the "AS" clause for
** that column, if there is an AS clause.  If there is no AS clause
** then the name of the column is unspecified and may change from
** one release of SQLite to the next.
**
** INVARIANTS:
**
** {F13721} A successful invocation of the [sqlite3_column_name(S,N)]
**          interface returns the name
**          of the Nth column (where 0 is the left-most column) for the
**          result set of [prepared statement] S as a
**          zero-terminated UTF-8 string.
**
** {F13723} A successful invocation of the [sqlite3_column_name16(S,N)]
**          interface returns the name
**          of the Nth column (where 0 is the left-most column) for the
**          result set of [prepared statement] S as a
**          zero-terminated UTF-16 string in the native byte order.
**
** {F13724} The [sqlite3_column_name()] and [sqlite3_column_name16()]
**          interfaces return a NULL pointer if they are unable to
**          allocate memory memory to hold there normal return strings.
**
** {F13725} If the N parameter to [sqlite3_column_name(S,N)] or
**          [sqlite3_column_name16(S,N)] is out of range, then the
**          interfaces returns a NULL pointer.
** 
** {F13726} The strings returned by [sqlite3_column_name(S,N)] and
**          [sqlite3_column_name16(S,N)] are valid until the next
**          call to either routine with the same S and N parameters
**          or until [sqlite3_finalize(S)] is called.
**
** {F13727} When a result column of a [SELECT] statement contains
**          an AS clause, the name of that column is the indentifier
**          to the right of the AS keyword.
*/
const char *sqlite3_column_name(sqlite3_stmt*, int N);
const void *sqlite3_column_name16(sqlite3_stmt*, int N);

/*
** CAPI3REF: Source Of Data In A Query Result {F13740}
**
** These routines provide a means to determine what column of what
** table in which database a result of a SELECT statement comes from.
** The name of the database or table or column can be returned as
** either a UTF8 or UTF16 string.  The _database_ routines return
** the database name, the _table_ routines return the table name, and
** the origin_ routines return the column name.
** The returned string is valid until
** the [prepared statement] is destroyed using
** [sqlite3_finalize()] or until the same information is requested
** again in a different encoding.
**
** The names returned are the original un-aliased names of the
** database, table, and column.
**
** The first argument to the following calls is a [prepared statement].
** These functions return information about the Nth column returned by 
** the statement, where N is the second function argument.
**
** If the Nth column returned by the statement is an expression
** or subquery and is not a column value, then all of these functions
** return NULL.  These routine might also return NULL if a memory
** allocation error occurs.  Otherwise, they return the 
** name of the attached database, table and column that query result
** column was extracted from.
**
** As with all other SQLite APIs, those postfixed with "16" return
** UTF-16 encoded strings, the other functions return UTF-8. {END}
**
** These APIs are only available if the library was compiled with the 
** SQLITE_ENABLE_COLUMN_METADATA preprocessor symbol defined.
**
** {U13751}
** If two or more threads call one or more of these routines against the same
** prepared statement and column at the same time then the results are
** undefined.
**
** INVARIANTS:
**
** {F13741} The [sqlite3_column_database_name(S,N)] interface returns either
**          the UTF-8 zero-terminated name of the database from which the 
**          Nth result column of [prepared statement] S 
**          is extracted, or NULL if the the Nth column of S is a
**          general expression or if unable to allocate memory
**          to store the name.
**          
** {F13742} The [sqlite3_column_database_name16(S,N)] interface returns either
**          the UTF-16 native byte order
**          zero-terminated name of the database from which the 
**          Nth result column of [prepared statement] S 
**          is extracted, or NULL if the the Nth column of S is a
**          general expression or if unable to allocate memory
**          to store the name.
**          
** {F13743} The [sqlite3_column_table_name(S,N)] interface returns either
**          the UTF-8 zero-terminated name of the table from which the 
**          Nth result column of [prepared statement] S 
**          is extracted, or NULL if the the Nth column of S is a
**          general expression or if unable to allocate memory
**          to store the name.
**          
** {F13744} The [sqlite3_column_table_name16(S,N)] interface returns either
**          the UTF-16 native byte order
**          zero-terminated name of the table from which the 
**          Nth result column of [prepared statement] S 
**          is extracted, or NULL if the the Nth column of S is a
**          general expression or if unable to allocate memory
**          to store the name.
**          
** {F13745} The [sqlite3_column_origin_name(S,N)] interface returns either
**          the UTF-8 zero-terminated name of the table column from which the 
**          Nth result column of [prepared statement] S 
**          is extracted, or NULL if the the Nth column of S is a
**          general expression or if unable to allocate memory
**          to store the name.
**          
** {F13746} The [sqlite3_column_origin_name16(S,N)] interface returns either
**          the UTF-16 native byte order
**          zero-terminated name of the table column from which the 
**          Nth result column of [prepared statement] S 
**          is extracted, or NULL if the the Nth column of S is a
**          general expression or if unable to allocate memory
**          to store the name.
**          
** {F13748} The return values from
**          [sqlite3_column_database_name|column metadata interfaces]
**          are valid
**          for the lifetime of the [prepared statement]
**          or until the encoding is changed by another metadata
**          interface call for the same prepared statement and column.
**
** LIMITATIONS:
**
** {U13751} If two or more threads call one or more
**          [sqlite3_column_database_name|column metadata interfaces]
**          the same [prepared statement] and result column
**          at the same time then the results are undefined.
*/
const char *sqlite3_column_database_name(sqlite3_stmt*,int);
const void *sqlite3_column_database_name16(sqlite3_stmt*,int);
const char *sqlite3_column_table_name(sqlite3_stmt*,int);
const void *sqlite3_column_table_name16(sqlite3_stmt*,int);
const char *sqlite3_column_origin_name(sqlite3_stmt*,int);
const void *sqlite3_column_origin_name16(sqlite3_stmt*,int);

/*
** CAPI3REF: Declared Datatype Of A Query Result {F13760}
**
** The first parameter is a [prepared statement]. 
** If this statement is a SELECT statement and the Nth column of the 
** returned result set of that SELECT is a table column (not an
** expression or subquery) then the declared type of the table
** column is returned.  If the Nth column of the result set is an
** expression or subquery, then a NULL pointer is returned.
** The returned string is always UTF-8 encoded.  {END} 
** For example, in the database schema:
**
** CREATE TABLE t1(c1 VARIANT);
**
** And the following statement compiled:
**
** SELECT c1 + 1, c1 FROM t1;
**
** Then this routine would return the string "VARIANT" for the second
** result column (i==1), and a NULL pointer for the first result column
** (i==0).
**
** SQLite uses dynamic run-time typing.  So just because a column
** is declared to contain a particular type does not mean that the
** data stored in that column is of the declared type.  SQLite is
** strongly typed, but the typing is dynamic not static.  Type
** is associated with individual values, not with the containers
** used to hold those values.
**
** INVARIANTS:
**
** {F13761}  A successful call to [sqlite3_column_decltype(S,N)]
**           returns a zero-terminated UTF-8 string containing the
**           the declared datatype of the table column that appears
**           as the Nth column (numbered from 0) of the result set to the
**           [prepared statement] S.
**
** {F13762}  A successful call to [sqlite3_column_decltype16(S,N)]
**           returns a zero-terminated UTF-16 native byte order string
**           containing the declared datatype of the table column that appears
**           as the Nth column (numbered from 0) of the result set to the
**           [prepared statement] S.
**
** {F13763}  If N is less than 0 or N is greater than or equal to
**           the number of columns in [prepared statement] S
**           or if the Nth column of S is an expression or subquery rather
**           than a table column or if a memory allocation failure
**           occurs during encoding conversions, then
**           calls to [sqlite3_column_decltype(S,N)] or
**           [sqlite3_column_decltype16(S,N)] return NULL.
*/
const char *sqlite3_column_decltype(sqlite3_stmt*,int);
const void *sqlite3_column_decltype16(sqlite3_stmt*,int);

/* 
** CAPI3REF:  Evaluate An SQL Statement {F13200}
**
** After an [prepared statement] has been prepared with a call
** to either [sqlite3_prepare_v2()] or [sqlite3_prepare16_v2()] or to one of
** the legacy interfaces [sqlite3_prepare()] or [sqlite3_prepare16()],
** then this function must be called one or more times to evaluate the 
** statement.
**
** The details of the behavior of this sqlite3_step() interface depend
** on whether the statement was prepared using the newer "v2" interface
** [sqlite3_prepare_v2()] and [sqlite3_prepare16_v2()] or the older legacy
** interface [sqlite3_prepare()] and [sqlite3_prepare16()].  The use of the
** new "v2" interface is recommended for new applications but the legacy
** interface will continue to be supported.
**
** In the legacy interface, the return value will be either [SQLITE_BUSY], 
** [SQLITE_DONE], [SQLITE_ROW], [SQLITE_ERROR], or [SQLITE_MISUSE].
** With the "v2" interface, any of the other [SQLITE_OK | result code]
** or [SQLITE_IOERR_READ | extended result code] might be returned as
** well.
**
** [SQLITE_BUSY] means that the database engine was unable to acquire the
** database locks it needs to do its job.  If the statement is a COMMIT
** or occurs outside of an explicit transaction, then you can retry the
** statement.  If the statement is not a COMMIT and occurs within a
** explicit transaction then you should rollback the transaction before
** continuing.
**
** [SQLITE_DONE] means that the statement has finished executing
** successfully.  sqlite3_step() should not be called again on this virtual
** machine without first calling [sqlite3_reset()] to reset the virtual
** machine back to its initial state.
**
** If the SQL statement being executed returns any data, then 
** [SQLITE_ROW] is returned each time a new row of data is ready
** for processing by the caller. The values may be accessed using
** the [sqlite3_column_int | column access functions].
** sqlite3_step() is called again to retrieve the next row of data.
** 
** [SQLITE_ERROR] means that a run-time error (such as a constraint
** violation) has occurred.  sqlite3_step() should not be called again on
** the VM. More information may be found by calling [sqlite3_errmsg()].
** With the legacy interface, a more specific error code (example:
** [SQLITE_INTERRUPT], [SQLITE_SCHEMA], [SQLITE_CORRUPT], and so forth)
** can be obtained by calling [sqlite3_reset()] on the
** [prepared statement].  In the "v2" interface,
** the more specific error code is returned directly by sqlite3_step().
**
** [SQLITE_MISUSE] means that the this routine was called inappropriately.
** Perhaps it was called on a [prepared statement] that has
** already been [sqlite3_finalize | finalized] or on one that had 
** previously returned [SQLITE_ERROR] or [SQLITE_DONE].  Or it could
** be the case that the same database connection is being used by two or
** more threads at the same moment in time.
**
** <b>Goofy Interface Alert:</b>
** In the legacy interface, 
** the sqlite3_step() API always returns a generic error code,
** [SQLITE_ERROR], following any error other than [SQLITE_BUSY]
** and [SQLITE_MISUSE].  You must call [sqlite3_reset()] or
** [sqlite3_finalize()] in order to find one of the specific
** [error codes] that better describes the error.
** We admit that this is a goofy design.  The problem has been fixed
** with the "v2" interface.  If you prepare all of your SQL statements
** using either [sqlite3_prepare_v2()] or [sqlite3_prepare16_v2()] instead
** of the legacy [sqlite3_prepare()] and [sqlite3_prepare16()], then the 
** more specific [error codes] are returned directly
** by sqlite3_step().  The use of the "v2" interface is recommended.
**
** INVARIANTS:
**
** {F13202}  If [prepared statement] S is ready to be
**           run, then [sqlite3_step(S)] advances that prepared statement
**           until to completion or until it is ready to return another
**           row of the result set or an interrupt or run-time error occurs.
**
** {F15304}  When a call to [sqlite3_step(S)] causes the 
**           [prepared statement] S to run to completion,
**           the function returns [SQLITE_DONE].
**
** {F15306}  When a call to [sqlite3_step(S)] stops because it is ready
**           to return another row of the result set, it returns
**           [SQLITE_ROW].
**
** {F15308}  If a call to [sqlite3_step(S)] encounters an
**           [sqlite3_interrupt|interrupt] or a run-time error,
**           it returns an appropraite error code that is not one of
**           [SQLITE_OK], [SQLITE_ROW], or [SQLITE_DONE].
**
** {F15310}  If an [sqlite3_interrupt|interrupt] or run-time error
**           occurs during a call to [sqlite3_step(S)]
**           for a [prepared statement] S created using
**           legacy interfaces [sqlite3_prepare()] or
**           [sqlite3_prepare16()] then the function returns either
**           [SQLITE_ERROR], [SQLITE_BUSY], or [SQLITE_MISUSE].
*/
int sqlite3_step(sqlite3_stmt*);

/*
** CAPI3REF: Number of columns in a result set {F13770}
**
** Return the number of values in the current row of the result set.
**
** INVARIANTS:
**
** {F13771}  After a call to [sqlite3_step(S)] that returns
**           [SQLITE_ROW], the [sqlite3_data_count(S)] routine
**           will return the same value as the
**           [sqlite3_column_count(S)] function.
**
** {F13772}  After [sqlite3_step(S)] has returned any value other than
**           [SQLITE_ROW] or before [sqlite3_step(S)] has been 
**           called on the [prepared statement] for
**           the first time since it was [sqlite3_prepare|prepared]
**           or [sqlite3_reset|reset], the [sqlite3_data_count(S)]
**           routine returns zero.
*/
int sqlite3_data_count(sqlite3_stmt *pStmt);

/*
** CAPI3REF: Fundamental Datatypes {F10265}
** KEYWORDS: SQLITE_TEXT
**
** {F10266}Every value in SQLite has one of five fundamental datatypes:
**
** <ul>
** <li> 64-bit signed integer
** <li> 64-bit IEEE floating point number
** <li> string
** <li> BLOB
** <li> NULL
** </ul> {END}
**
** These constants are codes for each of those types.
**
** Note that the SQLITE_TEXT constant was also used in SQLite version 2
** for a completely different meaning.  Software that links against both
** SQLite version 2 and SQLite version 3 should use SQLITE3_TEXT not
** SQLITE_TEXT.
*/
#define SQLITE_INTEGER  1
#define SQLITE_FLOAT    2
#define SQLITE_BLOB     4
#define SQLITE_NULL     5
#ifdef SQLITE_TEXT
# undef SQLITE_TEXT
#else
# define SQLITE_TEXT     3
#endif
#define SQLITE3_TEXT     3

/*
** CAPI3REF: Results Values From A Query {F13800}
**
** These routines form the "result set query" interface.
**
** These routines return information about
** a single column of the current result row of a query.  In every
** case the first argument is a pointer to the 
** [prepared statement] that is being
** evaluated (the [sqlite3_stmt*] that was returned from 
** [sqlite3_prepare_v2()] or one of its variants) and
** the second argument is the index of the column for which information 
** should be returned.  The left-most column of the result set
** has an index of 0.
**
** If the SQL statement is not currently point to a valid row, or if the
** the column index is out of range, the result is undefined. 
** These routines may only be called when the most recent call to
** [sqlite3_step()] has returned [SQLITE_ROW] and neither
** [sqlite3_reset()] nor [sqlite3_finalize()] has been call subsequently.
** If any of these routines are called after [sqlite3_reset()] or
** [sqlite3_finalize()] or after [sqlite3_step()] has returned
** something other than [SQLITE_ROW], the results are undefined.
** If [sqlite3_step()] or [sqlite3_reset()] or [sqlite3_finalize()]
** are called from a different thread while any of these routines
** are pending, then the results are undefined.  
**
** The sqlite3_column_type() routine returns 
** [SQLITE_INTEGER | datatype code] for the initial data type
** of the result column.  The returned value is one of [SQLITE_INTEGER],
** [SQLITE_FLOAT], [SQLITE_TEXT], [SQLITE_BLOB], or [SQLITE_NULL].  The value
** returned by sqlite3_column_type() is only meaningful if no type
** conversions have occurred as described below.  After a type conversion,
** the value returned by sqlite3_column_type() is undefined.  Future
** versions of SQLite may change the behavior of sqlite3_column_type()
** following a type conversion.
**
** If the result is a BLOB or UTF-8 string then the sqlite3_column_bytes() 
** routine returns the number of bytes in that BLOB or string.
** If the result is a UTF-16 string, then sqlite3_column_bytes() converts
** the string to UTF-8 and then returns the number of bytes.
** If the result is a numeric value then sqlite3_column_bytes() uses
** [sqlite3_snprintf()] to convert that value to a UTF-8 string and returns
** the number of bytes in that string.
** The value returned does not include the zero terminator at the end
** of the string.  For clarity: the value returned is the number of
** bytes in the string, not the number of characters.
**
** Strings returned by sqlite3_column_text() and sqlite3_column_text16(),
** even empty strings, are always zero terminated.  The return
** value from sqlite3_column_blob() for a zero-length blob is an arbitrary
** pointer, possibly even a NULL pointer.
**
** The sqlite3_column_bytes16() routine is similar to sqlite3_column_bytes()
** but leaves the result in UTF-16 in native byte order instead of UTF-8.  
** The zero terminator is not included in this count.
**
** The object returned by [sqlite3_column_value()] is an
** [unprotected sqlite3_value] object.  An unprotected sqlite3_value object
** may only be used with [sqlite3_bind_value()] and [sqlite3_result_value()].
** If the [unprotected sqlite3_value] object returned by
** [sqlite3_column_value()] is used in any other way, including calls
** to routines like 
** [sqlite3_value_int()], [sqlite3_value_text()], or [sqlite3_value_bytes()],
** then the behavior is undefined.
**
** These routines attempt to convert the value where appropriate.  For
** example, if the internal representation is FLOAT and a text result
** is requested, [sqlite3_snprintf()] is used internally to do the conversion
** automatically.  The following table details the conversions that
** are applied:
**
** <blockquote>
** <table border="1">
** <tr><th> Internal<br>Type <th> Requested<br>Type <th>  Conversion
**
** <tr><td>  NULL    <td> INTEGER   <td> Result is 0
** <tr><td>  NULL    <td>  FLOAT    <td> Result is 0.0
** <tr><td>  NULL    <td>   TEXT    <td> Result is NULL pointer
** <tr><td>  NULL    <td>   BLOB    <td> Result is NULL pointer
** <tr><td> INTEGER  <td>  FLOAT    <td> Convert from integer to float
** <tr><td> INTEGER  <td>   TEXT    <td> ASCII rendering of the integer
** <tr><td> INTEGER  <td>   BLOB    <td> Same as for INTEGER->TEXT
** <tr><td>  FLOAT   <td> INTEGER   <td> Convert from float to integer
** <tr><td>  FLOAT   <td>   TEXT    <td> ASCII rendering of the float
** <tr><td>  FLOAT   <td>   BLOB    <td> Same as FLOAT->TEXT
** <tr><td>  TEXT    <td> INTEGER   <td> Use atoi()
** <tr><td>  TEXT    <td>  FLOAT    <td> Use atof()
** <tr><td>  TEXT    <td>   BLOB    <td> No change
** <tr><td>  BLOB    <td> INTEGER   <td> Convert to TEXT then use atoi()
** <tr><td>  BLOB    <td>  FLOAT    <td> Convert to TEXT then use atof()
** <tr><td>  BLOB    <td>   TEXT    <td> Add a zero terminator if needed
** </table>
** </blockquote>
**
** The table above makes reference to standard C library functions atoi()
** and atof().  SQLite does not really use these functions.  It has its
** on equavalent internal routines.  The atoi() and atof() names are
** used in the table for brevity and because they are familiar to most
** C programmers.
**
** Note that when type conversions occur, pointers returned by prior
** calls to sqlite3_column_blob(), sqlite3_column_text(), and/or
** sqlite3_column_text16() may be invalidated. 
** Type conversions and pointer invalidations might occur
** in the following cases:
**
** <ul>
** <li><p>  The initial content is a BLOB and sqlite3_column_text() 
**          or sqlite3_column_text16() is called.  A zero-terminator might
**          need to be added to the string.</p></li>
**
** <li><p>  The initial content is UTF-8 text and sqlite3_column_bytes16() or
**          sqlite3_column_text16() is called.  The content must be converted
**          to UTF-16.</p></li>
**
** <li><p>  The initial content is UTF-16 text and sqlite3_column_bytes() or
**          sqlite3_column_text() is called.  The content must be converted
**          to UTF-8.</p></li>
** </ul>
**
** Conversions between UTF-16be and UTF-16le are always done in place and do
** not invalidate a prior pointer, though of course the content of the buffer
** that the prior pointer points to will have been modified.  Other kinds
** of conversion are done in place when it is possible, but sometime it is
** not possible and in those cases prior pointers are invalidated.  
**
** The safest and easiest to remember policy is to invoke these routines
** in one of the following ways:
**
**  <ul>
**  <li>sqlite3_column_text() followed by sqlite3_column_bytes()</li>
**  <li>sqlite3_column_blob() followed by sqlite3_column_bytes()</li>
**  <li>sqlite3_column_text16() followed by sqlite3_column_bytes16()</li>
**  </ul>
**
** In other words, you should call sqlite3_column_text(), sqlite3_column_blob(),
** or sqlite3_column_text16() first to force the result into the desired
** format, then invoke sqlite3_column_bytes() or sqlite3_column_bytes16() to
** find the size of the result.  Do not mix call to sqlite3_column_text() or
** sqlite3_column_blob() with calls to sqlite3_column_bytes16().  And do not
** mix calls to sqlite3_column_text16() with calls to sqlite3_column_bytes().
**
** The pointers returned are valid until a type conversion occurs as
** described above, or until [sqlite3_step()] or [sqlite3_reset()] or
** [sqlite3_finalize()] is called.  The memory space used to hold strings
** and blobs is freed automatically.  Do <b>not</b> pass the pointers returned
** [sqlite3_column_blob()], [sqlite3_column_text()], etc. into 
** [sqlite3_free()].
**
** If a memory allocation error occurs during the evaluation of any
** of these routines, a default value is returned.  The default value
** is either the integer 0, the floating point number 0.0, or a NULL
** pointer.  Subsequent calls to [sqlite3_errcode()] will return
** [SQLITE_NOMEM].
**
** INVARIANTS:
**
** {F13803} The [sqlite3_column_blob(S,N)] interface converts the
**          Nth column in the current row of the result set for
**          [prepared statement] S into a blob and then returns a
**          pointer to the converted value.
**
** {F13806} The [sqlite3_column_bytes(S,N)] interface returns the
**          number of bytes in the blob or string (exclusive of the
**          zero terminator on the string) that was returned by the
**          most recent call to [sqlite3_column_blob(S,N)] or
**          [sqlite3_column_text(S,N)].
**
** {F13809} The [sqlite3_column_bytes16(S,N)] interface returns the
**          number of bytes in the string (exclusive of the
**          zero terminator on the string) that was returned by the
**          most recent call to [sqlite3_column_text16(S,N)].
**
** {F13812} The [sqlite3_column_double(S,N)] interface converts the
**          Nth column in the current row of the result set for
**          [prepared statement] S into a floating point value and
**          returns a copy of that value.
**
** {F13815} The [sqlite3_column_int(S,N)] interface converts the
**          Nth column in the current row of the result set for
**          [prepared statement] S into a 64-bit signed integer and
**          returns the lower 32 bits of that integer.
**
** {F13818} The [sqlite3_column_int64(S,N)] interface converts the
**          Nth column in the current row of the result set for
**          [prepared statement] S into a 64-bit signed integer and
**          returns a copy of that integer.
**
** {F13821} The [sqlite3_column_text(S,N)] interface converts the
**          Nth column in the current row of the result set for
**          [prepared statement] S into a zero-terminated UTF-8 
**          string and returns a pointer to that string.
**
** {F13824} The [sqlite3_column_text16(S,N)] interface converts the
**          Nth column in the current row of the result set for
**          [prepared statement] S into a zero-terminated 2-byte
**          aligned UTF-16 native byte order
**          string and returns a pointer to that string.
**
** {F13827} The [sqlite3_column_type(S,N)] interface returns
**          one of [SQLITE_NULL], [SQLITE_INTEGER], [SQLITE_FLOAT],
**          [SQLITE_TEXT], or [SQLITE_BLOB] as appropriate for
**          the Nth column in the current row of the result set for
**          [prepared statement] S.
**
** {F13830} The [sqlite3_column_value(S,N)] interface returns a
**          pointer to an [unprotected sqlite3_value] object for the
**          Nth column in the current row of the result set for
**          [prepared statement] S.
*/
const void *sqlite3_column_blob(sqlite3_stmt*, int iCol);
int sqlite3_column_bytes(sqlite3_stmt*, int iCol);
int sqlite3_column_bytes16(sqlite3_stmt*, int iCol);
double sqlite3_column_double(sqlite3_stmt*, int iCol);
int sqlite3_column_int(sqlite3_stmt*, int iCol);
sqlite3_int64 sqlite3_column_int64(sqlite3_stmt*, int iCol);
const unsigned char *sqlite3_column_text(sqlite3_stmt*, int iCol);
const void *sqlite3_column_text16(sqlite3_stmt*, int iCol);
int sqlite3_column_type(sqlite3_stmt*, int iCol);
sqlite3_value *sqlite3_column_value(sqlite3_stmt*, int iCol);

/*
** CAPI3REF: Destroy A Prepared Statement Object {F13300}
**
** The sqlite3_finalize() function is called to delete a 
** [prepared statement]. If the statement was
** executed successfully, or not executed at all, then SQLITE_OK is returned.
** If execution of the statement failed then an 
** [error code] or [extended error code]
** is returned. 
**
** This routine can be called at any point during the execution of the
** [prepared statement].  If the virtual machine has not 
** completed execution when this routine is called, that is like
** encountering an error or an interrupt.  (See [sqlite3_interrupt()].) 
** Incomplete updates may be rolled back and transactions cancelled,  
** depending on the circumstances, and the 
** [error code] returned will be [SQLITE_ABORT].
**
** INVARIANTS:
**
** {F11302} The [sqlite3_finalize(S)] interface destroys the
**          [prepared statement] S and releases all
**          memory and file resources held by that object.
**
** {F11304} If the most recent call to [sqlite3_step(S)] for the
**          [prepared statement] S returned an error,
**          then [sqlite3_finalize(S)] returns that same error.
*/
int sqlite3_finalize(sqlite3_stmt *pStmt);

/*
** CAPI3REF: Reset A Prepared Statement Object {F13330}
**
** The sqlite3_reset() function is called to reset a 
** [prepared statement] object.
** back to its initial state, ready to be re-executed.
** Any SQL statement variables that had values bound to them using
** the [sqlite3_bind_blob | sqlite3_bind_*() API] retain their values.
** Use [sqlite3_clear_bindings()] to reset the bindings.
**
** {F11332} The [sqlite3_reset(S)] interface resets the [prepared statement] S
**          back to the beginning of its program.
**
** {F11334} If the most recent call to [sqlite3_step(S)] for 
**          [prepared statement] S returned [SQLITE_ROW] or [SQLITE_DONE],
**          or if [sqlite3_step(S)] has never before been called on S,
**          then [sqlite3_reset(S)] returns [SQLITE_OK].
**
** {F11336} If the most recent call to [sqlite3_step(S)] for
**          [prepared statement] S indicated an error, then
**          [sqlite3_reset(S)] returns an appropriate [error code].
**
** {F11338} The [sqlite3_reset(S)] interface does not change the values
**          of any [sqlite3_bind_blob|bindings] on [prepared statement] S.
*/
int sqlite3_reset(sqlite3_stmt *pStmt);

/*
** CAPI3REF: Create Or Redefine SQL Functions {F16100}
** KEYWORDS: {function creation routines} 
**
** These two functions (collectively known as
** "function creation routines") are used to add SQL functions or aggregates
** or to redefine the behavior of existing SQL functions or aggregates.  The
** difference only between the two is that the second parameter, the
** name of the (scalar) function or aggregate, is encoded in UTF-8 for
** sqlite3_create_function() and UTF-16 for sqlite3_create_function16().
**
** The first parameter is the [database connection] to which the SQL
** function is to be added.  If a single
** program uses more than one [database connection] internally, then SQL
** functions must be added individually to each [database connection].
**
** The second parameter is the name of the SQL function to be created
** or redefined.
** The length of the name is limited to 255 bytes, exclusive of the 
** zero-terminator.  Note that the name length limit is in bytes, not
** characters.  Any attempt to create a function with a longer name
** will result in an SQLITE_ERROR error.
**
** The third parameter is the number of arguments that the SQL function or
** aggregate takes. If this parameter is negative, then the SQL function or
** aggregate may take any number of arguments.
**
** The fourth parameter, eTextRep, specifies what 
** [SQLITE_UTF8 | text encoding] this SQL function prefers for
** its parameters.  Any SQL function implementation should be able to work
** work with UTF-8, UTF-16le, or UTF-16be.  But some implementations may be
** more efficient with one encoding than another.  It is allowed to
** invoke sqlite3_create_function() or sqlite3_create_function16() multiple
** times with the same function but with different values of eTextRep.
** When multiple implementations of the same function are available, SQLite
** will pick the one that involves the least amount of data conversion.
** If there is only a single implementation which does not care what
** text encoding is used, then the fourth argument should be
** [SQLITE_ANY].
**
** The fifth parameter is an arbitrary pointer.  The implementation
** of the function can gain access to this pointer using
** [sqlite3_user_data()].
**
** The seventh, eighth and ninth parameters, xFunc, xStep and xFinal, are
** pointers to C-language functions that implement the SQL
** function or aggregate. A scalar SQL function requires an implementation of
** the xFunc callback only, NULL pointers should be passed as the xStep
** and xFinal parameters. An aggregate SQL function requires an implementation
** of xStep and xFinal and NULL should be passed for xFunc. To delete an
** existing SQL function or aggregate, pass NULL for all three function
** callback.
**
** It is permitted to register multiple implementations of the same
** functions with the same name but with either differing numbers of
** arguments or differing perferred text encodings.  SQLite will use
** the implementation most closely matches the way in which the
** SQL function is used.
**
** INVARIANTS:
**
** {F16103} The [sqlite3_create_function16()] interface behaves exactly
**          like [sqlite3_create_function()] in every way except that it
**          interprets the zFunctionName argument as
**          zero-terminated UTF-16 native byte order instead of as a
**          zero-terminated UTF-8.
**
** {F16106} A successful invocation of
**          the [sqlite3_create_function(D,X,N,E,...)] interface registers
**          or replaces callback functions in [database connection] D
**          used to implement the SQL function named X with N parameters
**          and having a perferred text encoding of E.
**
** {F16109} A successful call to [sqlite3_create_function(D,X,N,E,P,F,S,L)]
**          replaces the P, F, S, and L values from any prior calls with
**          the same D, X, N, and E values.
**
** {F16112} The [sqlite3_create_function(D,X,...)] interface fails with
**          a return code of [SQLITE_ERROR] if the SQL function name X is
**          longer than 255 bytes exclusive of the zero terminator.
**
** {F16118} Either F must be NULL and S and L are non-NULL or else F
**          is non-NULL and S and L are NULL, otherwise
**          [sqlite3_create_function(D,X,N,E,P,F,S,L)] returns [SQLITE_ERROR].
**
** {F16121} The [sqlite3_create_function(D,...)] interface fails with an
**          error code of [SQLITE_BUSY] if there exist [prepared statements]
**          associated with the [database connection] D.
**
** {F16124} The [sqlite3_create_function(D,X,N,...)] interface fails with an
**          error code of [SQLITE_ERROR] if parameter N (specifying the number
**          of arguments to the SQL function being registered) is less
**          than -1 or greater than 127.
**
** {F16127} When N is non-negative, the [sqlite3_create_function(D,X,N,...)]
**          interface causes callbacks to be invoked for the SQL function
**          named X when the number of arguments to the SQL function is
**          exactly N.
**
** {F16130} When N is -1, the [sqlite3_create_function(D,X,N,...)]
**          interface causes callbacks to be invoked for the SQL function
**          named X with any number of arguments.
**
** {F16133} When calls to [sqlite3_create_function(D,X,N,...)]
**          specify multiple implementations of the same function X
**          and when one implementation has N>=0 and the other has N=(-1)
**          the implementation with a non-zero N is preferred.
**
** {F16136} When calls to [sqlite3_create_function(D,X,N,E,...)]
**          specify multiple implementations of the same function X with
**          the same number of arguments N but with different
**          encodings E, then the implementation where E matches the
**          database encoding is preferred.
**
** {F16139} For an aggregate SQL function created using
**          [sqlite3_create_function(D,X,N,E,P,0,S,L)] the finializer
**          function L will always be invoked exactly once if the
**          step function S is called one or more times.
**
** {F16142} When SQLite invokes either the xFunc or xStep function of
**          an application-defined SQL function or aggregate created
**          by [sqlite3_create_function()] or [sqlite3_create_function16()],
**          then the array of [sqlite3_value] objects passed as the
**          third parameter are always [protected sqlite3_value] objects.
*/
int sqlite3_create_function(
  sqlite3 *db,
  const char *zFunctionName,
  int nArg,
  int eTextRep,
  void *pApp,
  void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
  void (*xStep)(sqlite3_context*,int,sqlite3_value**),
  void (*xFinal)(sqlite3_context*)
);
int sqlite3_create_function16(
  sqlite3 *db,
  const void *zFunctionName,
  int nArg,
  int eTextRep,
  void *pApp,
  void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
  void (*xStep)(sqlite3_context*,int,sqlite3_value**),
  void (*xFinal)(sqlite3_context*)
);

/*
** CAPI3REF: Text Encodings {F10267}
**
** These constant define integer codes that represent the various
** text encodings supported by SQLite.
*/
#define SQLITE_UTF8           1
#define SQLITE_UTF16LE        2
#define SQLITE_UTF16BE        3
#define SQLITE_UTF16          4    /* Use native byte order */
#define SQLITE_ANY            5    /* sqlite3_create_function only */
#define SQLITE_UTF16_ALIGNED  8    /* sqlite3_create_collation only */

/*
** CAPI3REF: Obsolete Functions
**
** These functions are all now obsolete.  In order to maintain
** backwards compatibility with older code, we continue to support
** these functions.  However, new development projects should avoid
** the use of these functions.  To help encourage people to avoid
** using these functions, we are not going to tell you want they do.
*/
int sqlite3_aggregate_count(sqlite3_context*);
int sqlite3_expired(sqlite3_stmt*);
int sqlite3_transfer_bindings(sqlite3_stmt*, sqlite3_stmt*);
int sqlite3_global_recover(void);
void sqlite3_thread_cleanup(void);
int sqlite3_memory_alarm(void(*)(void*,sqlite3_int64,int),void*,sqlite3_int64);

/*
** CAPI3REF: Obtaining SQL Function Parameter Values {F15100}
**
** The C-language implementation of SQL functions and aggregates uses
** this set of interface routines to access the parameter values on
** the function or aggregate.
**
** The xFunc (for scalar functions) or xStep (for aggregates) parameters
** to [sqlite3_create_function()] and [sqlite3_create_function16()]
** define callbacks that implement the SQL functions and aggregates.
** The 4th parameter to these callbacks is an array of pointers to
** [protected sqlite3_value] objects.  There is one [sqlite3_value] object for
** each parameter to the SQL function.  These routines are used to
** extract values from the [sqlite3_value] objects.
**
** These routines work only with [protected sqlite3_value] objects.
** Any attempt to use these routines on an [unprotected sqlite3_value]
** object results in undefined behavior.
**
** These routines work just like the corresponding 
** [sqlite3_column_blob | sqlite3_column_* routines] except that 
** these routines take a single [protected sqlite3_value] object pointer
** instead of an [sqlite3_stmt*] pointer and an integer column number.
**
** The sqlite3_value_text16() interface extracts a UTF16 string
** in the native byte-order of the host machine.  The
** sqlite3_value_text16be() and sqlite3_value_text16le() interfaces
** extract UTF16 strings as big-endian and little-endian respectively.
**
** The sqlite3_value_numeric_type() interface attempts to apply
** numeric affinity to the value.  This means that an attempt is
** made to convert the value to an integer or floating point.  If
** such a conversion is possible without loss of information (in other
** words if the value is a string that looks like a number)
** then the conversion is done.  Otherwise no conversion occurs.  The 
** [SQLITE_INTEGER | datatype] after conversion is returned.
**
** Please pay particular attention to the fact that the pointer that
** is returned from [sqlite3_value_blob()], [sqlite3_value_text()], or
** [sqlite3_value_text16()] can be invalidated by a subsequent call to
** [sqlite3_value_bytes()], [sqlite3_value_bytes16()], [sqlite3_value_text()],
** or [sqlite3_value_text16()].  
**
** These routines must be called from the same thread as
** the SQL function that supplied the [sqlite3_value*] parameters.
**
**
** INVARIANTS:
**
** {F15103} The [sqlite3_value_blob(V)] interface converts the
**          [protected sqlite3_value] object V into a blob and then returns a
**          pointer to the converted value.
**
** {F15106} The [sqlite3_value_bytes(V)] interface returns the
**          number of bytes in the blob or string (exclusive of the
**          zero terminator on the string) that was returned by the
**          most recent call to [sqlite3_value_blob(V)] or
**          [sqlite3_value_text(V)].
**
** {F15109} The [sqlite3_value_bytes16(V)] interface returns the
**          number of bytes in the string (exclusive of the
**          zero terminator on the string) that was returned by the
**          most recent call to [sqlite3_value_text16(V)],
**          [sqlite3_value_text16be(V)], or [sqlite3_value_text16le(V)].
**
** {F15112} The [sqlite3_value_double(V)] interface converts the
**          [protected sqlite3_value] object V into a floating point value and
**          returns a copy of that value.
**
** {F15115} The [sqlite3_value_int(V)] interface converts the
**          [protected sqlite3_value] object V into a 64-bit signed integer and
**          returns the lower 32 bits of that integer.
**
** {F15118} The [sqlite3_value_int64(V)] interface converts the
**          [protected sqlite3_value] object V into a 64-bit signed integer and
**          returns a copy of that integer.
**
** {F15121} The [sqlite3_value_text(V)] interface converts the
**          [protected sqlite3_value] object V into a zero-terminated UTF-8 
**          string and returns a pointer to that string.
**
** {F15124} The [sqlite3_value_text16(V)] interface converts the
**          [protected sqlite3_value] object V into a zero-terminated 2-byte
**          aligned UTF-16 native byte order
**          string and returns a pointer to that string.
**
** {F15127} The [sqlite3_value_text16be(V)] interface converts the
**          [protected sqlite3_value] object V into a zero-terminated 2-byte
**          aligned UTF-16 big-endian
**          string and returns a pointer to that string.
**
** {F15130} The [sqlite3_value_text16le(V)] interface converts the
**          [protected sqlite3_value] object V into a zero-terminated 2-byte
**          aligned UTF-16 little-endian
**          string and returns a pointer to that string.
**
** {F15133} The [sqlite3_value_type(V)] interface returns
**          one of [SQLITE_NULL], [SQLITE_INTEGER], [SQLITE_FLOAT],
**          [SQLITE_TEXT], or [SQLITE_BLOB] as appropriate for
**          the [sqlite3_value] object V.
**
** {F15136} The [sqlite3_value_numeric_type(V)] interface converts
**          the [protected sqlite3_value] object V into either an integer or
**          a floating point value if it can do so without loss of
**          information, and returns one of [SQLITE_NULL],
**          [SQLITE_INTEGER], [SQLITE_FLOAT], [SQLITE_TEXT], or
**          [SQLITE_BLOB] as appropriate for
**          the [protected sqlite3_value] object V after the conversion attempt.
*/
const void *sqlite3_value_blob(sqlite3_value*);
int sqlite3_value_bytes(sqlite3_value*);
int sqlite3_value_bytes16(sqlite3_value*);
double sqlite3_value_double(sqlite3_value*);
int sqlite3_value_int(sqlite3_value*);
sqlite3_int64 sqlite3_value_int64(sqlite3_value*);
const unsigned char *sqlite3_value_text(sqlite3_value*);
const void *sqlite3_value_text16(sqlite3_value*);
const void *sqlite3_value_text16le(sqlite3_value*);
const void *sqlite3_value_text16be(sqlite3_value*);
int sqlite3_value_type(sqlite3_value*);
int sqlite3_value_numeric_type(sqlite3_value*);

/*
** CAPI3REF: Obtain Aggregate Function Context {F16210}
**
** The implementation of aggregate SQL functions use this routine to allocate
** a structure for storing their state.  
** The first time the sqlite3_aggregate_context() routine is
** is called for a particular aggregate, SQLite allocates nBytes of memory
** zeros that memory, and returns a pointer to it.
** On second and subsequent calls to sqlite3_aggregate_context()
** for the same aggregate function index, the same buffer is returned.
** The implementation
** of the aggregate can use the returned buffer to accumulate data.
**
** SQLite automatically frees the allocated buffer when the aggregate
** query concludes.
**
** The first parameter should be a copy of the 
** [sqlite3_context | SQL function context] that is the first
** parameter to the callback routine that implements the aggregate
** function.
**
** This routine must be called from the same thread in which
** the aggregate SQL function is running.
**
** INVARIANTS:
**
** {F16211} The first invocation of [sqlite3_aggregate_context(C,N)] for
**          a particular instance of an aggregate function (for a particular
**          context C) causes SQLite to allocation N bytes of memory,
**          zero that memory, and return a pointer to the allocationed
**          memory.
**
** {F16213} If a memory allocation error occurs during
**          [sqlite3_aggregate_context(C,N)] then the function returns 0.
**
** {F16215} Second and subsequent invocations of
**          [sqlite3_aggregate_context(C,N)] for the same context pointer C
**          ignore the N parameter and return a pointer to the same
**          block of memory returned by the first invocation.
**
** {F16217} The memory allocated by [sqlite3_aggregate_context(C,N)] is
**          automatically freed on the next call to [sqlite3_reset()]
**          or [sqlite3_finalize()] for the [prepared statement] containing
**          the aggregate function associated with context C.
*/
void *sqlite3_aggregate_context(sqlite3_context*, int nBytes);

/*
** CAPI3REF: User Data For Functions {F16240}
**
** The sqlite3_user_data() interface returns a copy of
** the pointer that was the pUserData parameter (the 5th parameter)
** of the the [sqlite3_create_function()]
** and [sqlite3_create_function16()] routines that originally
** registered the application defined function. {END}
**
** This routine must be called from the same thread in which
** the application-defined function is running.
**
** INVARIANTS:
**
** {F16243} The [sqlite3_user_data(C)] interface returns a copy of the
**          P pointer from the [sqlite3_create_function(D,X,N,E,P,F,S,L)]
**          or [sqlite3_create_function16(D,X,N,E,P,F,S,L)] call that
**          registered the SQL function associated with 
**          [sqlite3_context] C.
*/
void *sqlite3_user_data(sqlite3_context*);

/*
** CAPI3REF: Database Connection For Functions {F16250}
**
** The sqlite3_context_db_handle() interface returns a copy of
** the pointer to the [database connection] (the 1st parameter)
** of the the [sqlite3_create_function()]
** and [sqlite3_create_function16()] routines that originally
** registered the application defined function.
**
** INVARIANTS:
**
** {F16253} The [sqlite3_context_db_handle(C)] interface returns a copy of the
**          D pointer from the [sqlite3_create_function(D,X,N,E,P,F,S,L)]
**          or [sqlite3_create_function16(D,X,N,E,P,F,S,L)] call that
**          registered the SQL function associated with 
**          [sqlite3_context] C.
*/
sqlite3 *sqlite3_context_db_handle(sqlite3_context*);

/*
** CAPI3REF: Function Auxiliary Data {F16270}
**
** The following two functions may be used by scalar SQL functions to
** associate meta-data with argument values. If the same value is passed to
** multiple invocations of the same SQL function during query execution, under
** some circumstances the associated meta-data may be preserved. This may
** be used, for example, to add a regular-expression matching scalar
** function. The compiled version of the regular expression is stored as
** meta-data associated with the SQL value passed as the regular expression
** pattern.  The compiled regular expression can be reused on multiple
** invocations of the same function so that the original pattern string
** does not need to be recompiled on each invocation.
**
** The sqlite3_get_auxdata() interface returns a pointer to the meta-data
** associated by the sqlite3_set_auxdata() function with the Nth argument
** value to the application-defined function.
** If no meta-data has been ever been set for the Nth
** argument of the function, or if the cooresponding function parameter
** has changed since the meta-data was set, then sqlite3_get_auxdata()
** returns a NULL pointer.
**
** The sqlite3_set_auxdata() interface saves the meta-data
** pointed to by its 3rd parameter as the meta-data for the N-th
** argument of the application-defined function.  Subsequent
** calls to sqlite3_get_auxdata() might return this data, if it has
** not been destroyed. 
** If it is not NULL, SQLite will invoke the destructor 
** function given by the 4th parameter to sqlite3_set_auxdata() on
** the meta-data when the corresponding function parameter changes
** or when the SQL statement completes, whichever comes first.
**
** SQLite is free to call the destructor and drop meta-data on
** any parameter of any function at any time.  The only guarantee
** is that the destructor will be called before the metadata is
** dropped.
**
** In practice, meta-data is preserved between function calls for
** expressions that are constant at compile time. This includes literal
** values and SQL variables.
**
** These routines must be called from the same thread in which
** the SQL function is running.
**
** INVARIANTS:
**
** {F16272} The [sqlite3_get_auxdata(C,N)] interface returns a pointer
**          to metadata associated with the Nth parameter of the SQL function
**          whose context is C, or NULL if there is no metadata associated
**          with that parameter.
**
** {F16274} The [sqlite3_set_auxdata(C,N,P,D)] interface assigns a metadata
**          pointer P to the Nth parameter of the SQL function with context
**          C.
**
** {F16276} SQLite will invoke the destructor D with a single argument
**          which is the metadata pointer P following a call to
**          [sqlite3_set_auxdata(C,N,P,D)] when SQLite ceases to hold
**          the metadata.
**
** {F16277} SQLite ceases to hold metadata for an SQL function parameter
**          when the value of that parameter changes.
**
** {F16278} When [sqlite3_set_auxdata(C,N,P,D)] is invoked, the destructor
**          is called for any prior metadata associated with the same function
**          context C and parameter N.
**
** {F16279} SQLite will call destructors for any metadata it is holding
**          in a particular [prepared statement] S when either
**          [sqlite3_reset(S)] or [sqlite3_finalize(S)] is called.
*/
void *sqlite3_get_auxdata(sqlite3_context*, int N);
void sqlite3_set_auxdata(sqlite3_context*, int N, void*, void (*)(void*));


/*
** CAPI3REF: Constants Defining Special Destructor Behavior {F10280}
**
** These are special value for the destructor that is passed in as the
** final argument to routines like [sqlite3_result_blob()].  If the destructor
** argument is SQLITE_STATIC, it means that the content pointer is constant
** and will never change.  It does not need to be destroyed.  The 
** SQLITE_TRANSIENT value means that the content will likely change in
** the near future and that SQLite should make its own private copy of
** the content before returning.
**
** The typedef is necessary to work around problems in certain
** C++ compilers.  See ticket #2191.
*/
typedef void (*sqlite3_destructor_type)(void*);
#define SQLITE_STATIC      ((sqlite3_destructor_type)0)
#define SQLITE_TRANSIENT   ((sqlite3_destructor_type)-1)

/*
** CAPI3REF: Setting The Result Of An SQL Function {F16400}
**
** These routines are used by the xFunc or xFinal callbacks that
** implement SQL functions and aggregates.  See
** [sqlite3_create_function()] and [sqlite3_create_function16()]
** for additional information.
**
** These functions work very much like the 
** [sqlite3_bind_blob | sqlite3_bind_*] family of functions used
** to bind values to host parameters in prepared statements.
** Refer to the
** [sqlite3_bind_blob | sqlite3_bind_* documentation] for
** additional information.
**
** The sqlite3_result_blob() interface sets the result from
** an application defined function to be the BLOB whose content is pointed
** to by the second parameter and which is N bytes long where N is the
** third parameter. 
** The sqlite3_result_zeroblob() inerfaces set the result of
** the application defined function to be a BLOB containing all zero
** bytes and N bytes in size, where N is the value of the 2nd parameter.
**
** The sqlite3_result_double() interface sets the result from
** an application defined function to be a floating point value specified
** by its 2nd argument.
**
** The sqlite3_result_error() and sqlite3_result_error16() functions
** cause the implemented SQL function to throw an exception.
** SQLite uses the string pointed to by the
** 2nd parameter of sqlite3_result_error() or sqlite3_result_error16()
** as the text of an error message.  SQLite interprets the error
** message string from sqlite3_result_error() as UTF8. SQLite
** interprets the string from sqlite3_result_error16() as UTF16 in native
** byte order.  If the third parameter to sqlite3_result_error()
** or sqlite3_result_error16() is negative then SQLite takes as the error
** message all text up through the first zero character.
** If the third parameter to sqlite3_result_error() or
** sqlite3_result_error16() is non-negative then SQLite takes that many
** bytes (not characters) from the 2nd parameter as the error message.
** The sqlite3_result_error() and sqlite3_result_error16()
** routines make a copy private copy of the error message text before
** they return.  Hence, the calling function can deallocate or
** modify the text after they return without harm.
** The sqlite3_result_error_code() function changes the error code
** returned by SQLite as a result of an error in a function.  By default,
** the error code is SQLITE_ERROR.  A subsequent call to sqlite3_result_error()
** or sqlite3_result_error16() resets the error code to SQLITE_ERROR.
**
** The sqlite3_result_toobig() interface causes SQLite
** to throw an error indicating that a string or BLOB is to long
** to represent.  The sqlite3_result_nomem() interface
** causes SQLite to throw an exception indicating that the a
** memory allocation failed.
**
** The sqlite3_result_int() interface sets the return value
** of the application-defined function to be the 32-bit signed integer
** value given in the 2nd argument.
** The sqlite3_result_int64() interface sets the return value
** of the application-defined function to be the 64-bit signed integer
** value given in the 2nd argument.
**
** The sqlite3_result_null() interface sets the return value
** of the application-defined function to be NULL.
**
** The sqlite3_result_text(), sqlite3_result_text16(), 
** sqlite3_result_text16le(), and sqlite3_result_text16be() interfaces
** set the return value of the application-defined function to be
** a text string which is represented as UTF-8, UTF-16 native byte order,
** UTF-16 little endian, or UTF-16 big endian, respectively.
** SQLite takes the text result from the application from
** the 2nd parameter of the sqlite3_result_text* interfaces.
** If the 3rd parameter to the sqlite3_result_text* interfaces
** is negative, then SQLite takes result text from the 2nd parameter 
** through the first zero character.
** If the 3rd parameter to the sqlite3_result_text* interfaces
** is non-negative, then as many bytes (not characters) of the text
** pointed to by the 2nd parameter are taken as the application-defined
** function result.
** If the 4th parameter to the sqlite3_result_text* interfaces
** or sqlite3_result_blob is a non-NULL pointer, then SQLite calls that
** function as the destructor on the text or blob result when it has
** finished using that result.
** If the 4th parameter to the sqlite3_result_text* interfaces
** or sqlite3_result_blob is the special constant SQLITE_STATIC, then
** SQLite assumes that the text or blob result is constant space and
** does not copy the space or call a destructor when it has
** finished using that result.
** If the 4th parameter to the sqlite3_result_text* interfaces
** or sqlite3_result_blob is the special constant SQLITE_TRANSIENT
** then SQLite makes a copy of the result into space obtained from
** from [sqlite3_malloc()] before it returns.
**
** The sqlite3_result_value() interface sets the result of
** the application-defined function to be a copy the
** [unprotected sqlite3_value] object specified by the 2nd parameter.  The
** sqlite3_result_value() interface makes a copy of the [sqlite3_value]
** so that [sqlite3_value] specified in the parameter may change or
** be deallocated after sqlite3_result_value() returns without harm.
** A [protected sqlite3_value] object may always be used where an
** [unprotected sqlite3_value] object is required, so either
** kind of [sqlite3_value] object can be used with this interface.
**
** If these routines are called from within the different thread 
** than the one containing the application-defined function that recieved
** the [sqlite3_context] pointer, the results are undefined.
**
** INVARIANTS:
**
** {F16403} The default return value from any SQL function is NULL.
**
** {F16406} The [sqlite3_result_blob(C,V,N,D)] interface changes the
**          return value of function C to be a blob that is N bytes
**          in length and with content pointed to by V.
**
** {F16409} The [sqlite3_result_double(C,V)] interface changes the
**          return value of function C to be the floating point value V.
**
** {F16412} The [sqlite3_result_error(C,V,N)] interface changes the return
**          value of function C to be an exception with error code
**          [SQLITE_ERROR] and a UTF8 error message copied from V up to the
**          first zero byte or until N bytes are read if N is positive.
**
** {F16415} The [sqlite3_result_error16(C,V,N)] interface changes the return
**          value of function C to be an exception with error code
**          [SQLITE_ERROR] and a UTF16 native byte order error message
**          copied from V up to the first zero terminator or until N bytes
**          are read if N is positive.
**
** {F16418} The [sqlite3_result_error_toobig(C)] interface changes the return
**          value of the function C to be an exception with error code
**          [SQLITE_TOOBIG] and an appropriate error message.
**
** {F16421} The [sqlite3_result_error_nomem(C)] interface changes the return
**          value of the function C to be an exception with error code
**          [SQLITE_NOMEM] and an appropriate error message.
**
** {F16424} The [sqlite3_result_error_code(C,E)] interface changes the return
**          value of the function C to be an exception with error code E.
**          The error message text is unchanged.
**
** {F16427} The [sqlite3_result_int(C,V)] interface changes the
**          return value of function C to be the 32-bit integer value V.
**
** {F16430} The [sqlite3_result_int64(C,V)] interface changes the
**          return value of function C to be the 64-bit integer value V.
**
** {F16433} The [sqlite3_result_null(C)] interface changes the
**          return value of function C to be NULL.
**
** {F16436} The [sqlite3_result_text(C,V,N,D)] interface changes the
**          return value of function C to be the UTF8 string
**          V up to the first zero if N is negative
**          or the first N bytes of V if N is non-negative.
**
** {F16439} The [sqlite3_result_text16(C,V,N,D)] interface changes the
**          return value of function C to be the UTF16 native byte order
**          string V up to the first zero if N is
**          negative or the first N bytes of V if N is non-negative.
**
** {F16442} The [sqlite3_result_text16be(C,V,N,D)] interface changes the
**          return value of function C to be the UTF16 big-endian
**          string V up to the first zero if N is
**          is negative or the first N bytes or V if N is non-negative.
**
** {F16445} The [sqlite3_result_text16le(C,V,N,D)] interface changes the
**          return value of function C to be the UTF16 little-endian
**          string V up to the first zero if N is
**          negative or the first N bytes of V if N is non-negative.
**
** {F16448} The [sqlite3_result_value(C,V)] interface changes the
**          return value of function C to be [unprotected sqlite3_value]
**          object V.
**
** {F16451} The [sqlite3_result_zeroblob(C,N)] interface changes the
**          return value of function C to be an N-byte blob of all zeros.
**
** {F16454} The [sqlite3_result_error()] and [sqlite3_result_error16()]
**          interfaces make a copy of their error message strings before
**          returning.
**
** {F16457} If the D destructor parameter to [sqlite3_result_blob(C,V,N,D)],
**          [sqlite3_result_text(C,V,N,D)], [sqlite3_result_text16(C,V,N,D)],
**          [sqlite3_result_text16be(C,V,N,D)], or
**          [sqlite3_result_text16le(C,V,N,D)] is the constant [SQLITE_STATIC]
**          then no destructor is ever called on the pointer V and SQLite
**          assumes that V is immutable.
**
** {F16460} If the D destructor parameter to [sqlite3_result_blob(C,V,N,D)],
**          [sqlite3_result_text(C,V,N,D)], [sqlite3_result_text16(C,V,N,D)],
**          [sqlite3_result_text16be(C,V,N,D)], or
**          [sqlite3_result_text16le(C,V,N,D)] is the constant
**          [SQLITE_TRANSIENT] then the interfaces makes a copy of the
**          content of V and retains the copy.
**
** {F16463} If the D destructor parameter to [sqlite3_result_blob(C,V,N,D)],
**          [sqlite3_result_text(C,V,N,D)], [sqlite3_result_text16(C,V,N,D)],
**          [sqlite3_result_text16be(C,V,N,D)], or
**          [sqlite3_result_text16le(C,V,N,D)] is some value other than
**          the constants [SQLITE_STATIC] and [SQLITE_TRANSIENT] then 
**          SQLite will invoke the destructor D with V as its only argument
**          when it has finished with the V value.
*/
void sqlite3_result_blob(sqlite3_context*, const void*, int, void(*)(void*));
void sqlite3_result_double(sqlite3_context*, double);
void sqlite3_result_error(sqlite3_context*, const char*, int);
void sqlite3_result_error16(sqlite3_context*, const void*, int);
void sqlite3_result_error_toobig(sqlite3_context*);
void sqlite3_result_error_nomem(sqlite3_context*);
void sqlite3_result_error_code(sqlite3_context*, int);
void sqlite3_result_int(sqlite3_context*, int);
void sqlite3_result_int64(sqlite3_context*, sqlite3_int64);
void sqlite3_result_null(sqlite3_context*);
void sqlite3_result_text(sqlite3_context*, const char*, int, void(*)(void*));
void sqlite3_result_text16(sqlite3_context*, const void*, int, void(*)(void*));
void sqlite3_result_text16le(sqlite3_context*, const void*, int,void(*)(void*));
void sqlite3_result_text16be(sqlite3_context*, const void*, int,void(*)(void*));
void sqlite3_result_value(sqlite3_context*, sqlite3_value*);
void sqlite3_result_zeroblob(sqlite3_context*, int n);

/*
** CAPI3REF: Define New Collating Sequences {F16600}
**
** These functions are used to add new collation sequences to the
** [sqlite3*] handle specified as the first argument. 
**
** The name of the new collation sequence is specified as a UTF-8 string
** for sqlite3_create_collation() and sqlite3_create_collation_v2()
** and a UTF-16 string for sqlite3_create_collation16(). In all cases
** the name is passed as the second function argument.
**
** The third argument may be one of the constants [SQLITE_UTF8],
** [SQLITE_UTF16LE] or [SQLITE_UTF16BE], indicating that the user-supplied
** routine expects to be passed pointers to strings encoded using UTF-8,
** UTF-16 little-endian or UTF-16 big-endian respectively. The
** third argument might also be [SQLITE_UTF16_ALIGNED] to indicate that
** the routine expects pointers to 16-bit word aligned strings
** of UTF16 in the native byte order of the host computer.
**
** A pointer to the user supplied routine must be passed as the fifth
** argument.  If it is NULL, this is the same as deleting the collation
** sequence (so that SQLite cannot call it anymore).
** Each time the application
** supplied function is invoked, it is passed a copy of the void* passed as
** the fourth argument to sqlite3_create_collation() or
** sqlite3_create_collation16() as its first parameter.
**
** The remaining arguments to the application-supplied routine are two strings,
** each represented by a (length, data) pair and encoded in the encoding
** that was passed as the third argument when the collation sequence was
** registered. {END} The application defined collation routine should
** return negative, zero or positive if
** the first string is less than, equal to, or greater than the second
** string. i.e. (STRING1 - STRING2).
**
** The sqlite3_create_collation_v2() works like sqlite3_create_collation()
** excapt that it takes an extra argument which is a destructor for
** the collation.  The destructor is called when the collation is
** destroyed and is passed a copy of the fourth parameter void* pointer
** of the sqlite3_create_collation_v2().
** Collations are destroyed when
** they are overridden by later calls to the collation creation functions
** or when the [sqlite3*] database handle is closed using [sqlite3_close()].
**
** INVARIANTS:
**
** {F16603} A successful call to the
**          [sqlite3_create_collation_v2(B,X,E,P,F,D)] interface
**          registers function F as the comparison function used to
**          implement collation X on [database connection] B for
**          databases having encoding E.
**
** {F16604} SQLite understands the X parameter to
**          [sqlite3_create_collation_v2(B,X,E,P,F,D)] as a zero-terminated
**          UTF-8 string in which case is ignored for ASCII characters and
**          is significant for non-ASCII characters.
**
** {F16606} Successive calls to [sqlite3_create_collation_v2(B,X,E,P,F,D)]
**          with the same values for B, X, and E, override prior values
**          of P, F, and D.
**
** {F16609} The destructor D in [sqlite3_create_collation_v2(B,X,E,P,F,D)]
**          is not NULL then it is called with argument P when the
**          collating function is dropped by SQLite.
**
** {F16612} A collating function is dropped when it is overloaded.
**
** {F16615} A collating function is dropped when the database connection
**          is closed using [sqlite3_close()].
**
** {F16618} The pointer P in [sqlite3_create_collation_v2(B,X,E,P,F,D)]
**          is passed through as the first parameter to the comparison
**          function F for all subsequent invocations of F.
**
** {F16621} A call to [sqlite3_create_collation(B,X,E,P,F)] is exactly
**          the same as a call to [sqlite3_create_collation_v2()] with
**          the same parameters and a NULL destructor.
**
** {F16624} Following a [sqlite3_create_collation_v2(B,X,E,P,F,D)],
**          SQLite uses the comparison function F for all text comparison
**          operations on [database connection] B on text values that
**          use the collating sequence name X.
**
** {F16627} The [sqlite3_create_collation16(B,X,E,P,F)] works the same
**          as [sqlite3_create_collation(B,X,E,P,F)] except that the
**          collation name X is understood as UTF-16 in native byte order
**          instead of UTF-8.
**
** {F16630} When multiple comparison functions are available for the same
**          collating sequence, SQLite chooses the one whose text encoding
**          requires the least amount of conversion from the default
**          text encoding of the database.
*/
int sqlite3_create_collation(
  sqlite3*, 
  const char *zName, 
  int eTextRep, 
  void*,
  int(*xCompare)(void*,int,const void*,int,const void*)
);
int sqlite3_create_collation_v2(
  sqlite3*, 
  const char *zName, 
  int eTextRep, 
  void*,
  int(*xCompare)(void*,int,const void*,int,const void*),
  void(*xDestroy)(void*)
);
int sqlite3_create_collation16(
  sqlite3*, 
  const char *zName, 
  int eTextRep, 
  void*,
  int(*xCompare)(void*,int,const void*,int,const void*)
);

/*
** CAPI3REF: Collation Needed Callbacks {F16700}
**
** To avoid having to register all collation sequences before a database
** can be used, a single callback function may be registered with the
** database handle to be called whenever an undefined collation sequence is
** required.
**
** If the function is registered using the sqlite3_collation_needed() API,
** then it is passed the names of undefined collation sequences as strings
** encoded in UTF-8. {F16703} If sqlite3_collation_needed16() is used, the names
** are passed as UTF-16 in machine native byte order. A call to either
** function replaces any existing callback.
**
** When the callback is invoked, the first argument passed is a copy
** of the second argument to sqlite3_collation_needed() or
** sqlite3_collation_needed16().  The second argument is the database
** handle.  The third argument is one of [SQLITE_UTF8],
** [SQLITE_UTF16BE], or [SQLITE_UTF16LE], indicating the most
** desirable form of the collation sequence function required.
** The fourth parameter is the name of the
** required collation sequence.
**
** The callback function should register the desired collation using
** [sqlite3_create_collation()], [sqlite3_create_collation16()], or
** [sqlite3_create_collation_v2()].
**
** INVARIANTS:
**
** {F16702} A successful call to [sqlite3_collation_needed(D,P,F)]
**          or [sqlite3_collation_needed16(D,P,F)] causes
**          the [database connection] D to invoke callback F with first
**          parameter P whenever it needs a comparison function for a
**          collating sequence that it does not know about.
**
** {F16704} Each successful call to [sqlite3_collation_needed()] or
**          [sqlite3_collation_needed16()] overrides the callback registered
**          on the same [database connection] by prior calls to either
**          interface.
**
** {F16706} The name of the requested collating function passed in the
**          4th parameter to the callback is in UTF-8 if the callback
**          was registered using [sqlite3_collation_needed()] and
**          is in UTF-16 native byte order if the callback was
**          registered using [sqlite3_collation_needed16()].
**
** 
*/
int sqlite3_collation_needed(
  sqlite3*, 
  void*, 
  void(*)(void*,sqlite3*,int eTextRep,const char*)
);
int sqlite3_collation_needed16(
  sqlite3*, 
  void*,
  void(*)(void*,sqlite3*,int eTextRep,const void*)
);

/*
** Specify the key for an encrypted database.  This routine should be
** called right after sqlite3_open().
**
** The code to implement this API is not available in the public release
** of SQLite.
*/
int sqlite3_key(
  sqlite3 *db,                   /* Database to be rekeyed */
  const void *pKey, int nKey     /* The key */
);

/*
** Change the key on an open database.  If the current database is not
** encrypted, this routine will encrypt it.  If pNew==0 or nNew==0, the
** database is decrypted.
**
** The code to implement this API is not available in the public release
** of SQLite.
*/
int sqlite3_rekey(
  sqlite3 *db,                   /* Database to be rekeyed */
  const void *pKey, int nKey     /* The new key */
);

/*
** CAPI3REF:  Suspend Execution For A Short Time {F10530}
**
** The sqlite3_sleep() function
** causes the current thread to suspend execution
** for at least a number of milliseconds specified in its parameter.
**
** If the operating system does not support sleep requests with 
** millisecond time resolution, then the time will be rounded up to 
** the nearest second. The number of milliseconds of sleep actually 
** requested from the operating system is returned.
**
** SQLite implements this interface by calling the xSleep()
** method of the default [sqlite3_vfs] object.
**
** INVARIANTS:
**
** {F10533} The [sqlite3_sleep(M)] interface invokes the xSleep
**          method of the default [sqlite3_vfs|VFS] in order to
**          suspend execution of the current thread for at least
**          M milliseconds.
**
** {F10536} The [sqlite3_sleep(M)] interface returns the number of
**          milliseconds of sleep actually requested of the operating
**          system, which might be larger than the parameter M.
*/
int sqlite3_sleep(int);

/*
** CAPI3REF:  Name Of The Folder Holding Temporary Files {F10310}
**
** If this global variable is made to point to a string which is
** the name of a folder (a.ka. directory), then all temporary files
** created by SQLite will be placed in that directory.  If this variable
** is NULL pointer, then SQLite does a search for an appropriate temporary
** file directory.
**
** It is not safe to modify this variable once a database connection
** has been opened.  It is intended that this variable be set once
** as part of process initialization and before any SQLite interface
** routines have been call and remain unchanged thereafter.
*/
SQLITE_EXTERN char *sqlite3_temp_directory;

/*
** CAPI3REF:  Test To See If The Database Is In Auto-Commit Mode {F12930}
**
** The sqlite3_get_autocommit() interfaces returns non-zero or
** zero if the given database connection is or is not in autocommit mode,
** respectively.   Autocommit mode is on
** by default.  Autocommit mode is disabled by a [BEGIN] statement.
** Autocommit mode is reenabled by a [COMMIT] or [ROLLBACK].
**
** If certain kinds of errors occur on a statement within a multi-statement
** transactions (errors including [SQLITE_FULL], [SQLITE_IOERR], 
** [SQLITE_NOMEM], [SQLITE_BUSY], and [SQLITE_INTERRUPT]) then the
** transaction might be rolled back automatically.  The only way to
** find out if SQLite automatically rolled back the transaction after
** an error is to use this function.
**
** INVARIANTS:
**
** {F12931} The [sqlite3_get_autocommit(D)] interface returns non-zero or
**          zero if the [database connection] D is or is not in autocommit
**          mode, respectively.
**
** {F12932} Autocommit mode is on by default.
**
** {F12933} Autocommit mode is disabled by a successful [BEGIN] statement.
**
** {F12934} Autocommit mode is enabled by a successful [COMMIT] or [ROLLBACK]
**          statement.
** 
**
** LIMITATIONS:
***
** {U12936} If another thread changes the autocommit status of the database
**          connection while this routine is running, then the return value
**          is undefined.
*/
int sqlite3_get_autocommit(sqlite3*);

/*
** CAPI3REF:  Find The Database Handle Of A Prepared Statement {F13120}
**
** The sqlite3_db_handle interface
** returns the [sqlite3*] database handle to which a
** [prepared statement] belongs.
** The database handle returned by sqlite3_db_handle
** is the same database handle that was
** the first argument to the [sqlite3_prepare_v2()] or its variants
** that was used to create the statement in the first place.
**
** INVARIANTS:
**
** {F13123} The [sqlite3_db_handle(S)] interface returns a pointer
**          to the [database connection] associated with
**          [prepared statement] S.
*/
sqlite3 *sqlite3_db_handle(sqlite3_stmt*);


/*
** CAPI3REF: Commit And Rollback Notification Callbacks {F12950}
**
** The sqlite3_commit_hook() interface registers a callback
** function to be invoked whenever a transaction is committed.
** Any callback set by a previous call to sqlite3_commit_hook()
** for the same database connection is overridden.
** The sqlite3_rollback_hook() interface registers a callback
** function to be invoked whenever a transaction is committed.
** Any callback set by a previous call to sqlite3_commit_hook()
** for the same database connection is overridden.
** The pArg argument is passed through
** to the callback.  If the callback on a commit hook function 
** returns non-zero, then the commit is converted into a rollback.
**
** If another function was previously registered, its
** pArg value is returned.  Otherwise NULL is returned.
**
** Registering a NULL function disables the callback.
**
** For the purposes of this API, a transaction is said to have been 
** rolled back if an explicit "ROLLBACK" statement is executed, or
** an error or constraint causes an implicit rollback to occur.
** The rollback callback is not invoked if a transaction is
** automatically rolled back because the database connection is closed.
** The rollback callback is not invoked if a transaction is
** rolled back because a commit callback returned non-zero.
** <todo> Check on this </todo>
**
** These are experimental interfaces and are subject to change.
**
** INVARIANTS:
**
** {F12951} The [sqlite3_commit_hook(D,F,P)] interface registers the
**          callback function F to be invoked with argument P whenever
**          a transaction commits on [database connection] D.
**
** {F12952} The [sqlite3_commit_hook(D,F,P)] interface returns the P
**          argument from the previous call with the same 
**          [database connection ] D , or NULL on the first call
**          for a particular [database connection] D.
**
** {F12953} Each call to [sqlite3_commit_hook()] overwrites the callback
**          registered by prior calls.
**
** {F12954} If the F argument to [sqlite3_commit_hook(D,F,P)] is NULL
**          then the commit hook callback is cancelled and no callback
**          is invoked when a transaction commits.
**
** {F12955} If the commit callback returns non-zero then the commit is
**          converted into a rollback.
**
** {F12961} The [sqlite3_rollback_hook(D,F,P)] interface registers the
**          callback function F to be invoked with argument P whenever
**          a transaction rolls back on [database connection] D.
**
** {F12962} The [sqlite3_rollback_hook(D,F,P)] interface returns the P
**          argument from the previous call with the same 
**          [database connection ] D , or NULL on the first call
**          for a particular [database connection] D.
**
** {F12963} Each call to [sqlite3_rollback_hook()] overwrites the callback
**          registered by prior calls.
**
** {F12964} If the F argument to [sqlite3_rollback_hook(D,F,P)] is NULL
**          then the rollback hook callback is cancelled and no callback
**          is invoked when a transaction rolls back.
*/
void *sqlite3_commit_hook(sqlite3*, int(*)(void*), void*);
void *sqlite3_rollback_hook(sqlite3*, void(*)(void *), void*);

/*
** CAPI3REF: Data Change Notification Callbacks {F12970}
**
** The sqlite3_update_hook() interface
** registers a callback function with the database connection identified by the 
** first argument to be invoked whenever a row is updated, inserted or deleted.
** Any callback set by a previous call to this function for the same 
** database connection is overridden.
**
** The second argument is a pointer to the function to invoke when a 
** row is updated, inserted or deleted. 
** The first argument to the callback is
** a copy of the third argument to sqlite3_update_hook().
** The second callback 
** argument is one of [SQLITE_INSERT], [SQLITE_DELETE] or [SQLITE_UPDATE],
** depending on the operation that caused the callback to be invoked.
** The third and 
** fourth arguments to the callback contain pointers to the database and 
** table name containing the affected row.
** The final callback parameter is 
** the rowid of the row.
** In the case of an update, this is the rowid after 
** the update takes place.
**
** The update hook is not invoked when internal system tables are
** modified (i.e. sqlite_master and sqlite_sequence).
**
** If another function was previously registered, its pArg value
** is returned.  Otherwise NULL is returned.
**
** INVARIANTS:
**
** {F12971} The [sqlite3_update_hook(D,F,P)] interface causes callback
**          function F to be invoked with first parameter P whenever
**          a table row is modified, inserted, or deleted on
**          [database connection] D.
**
** {F12973} The [sqlite3_update_hook(D,F,P)] interface returns the value
**          of P for the previous call on the same [database connection] D,
**          or NULL for the first call.
**
** {F12975} If the update hook callback F in [sqlite3_update_hook(D,F,P)]
**          is NULL then the no update callbacks are made.
**
** {F12977} Each call to [sqlite3_update_hook(D,F,P)] overrides prior calls
**          to the same interface on the same [database connection] D.
**
** {F12979} The update hook callback is not invoked when internal system
**          tables such as sqlite_master and sqlite_sequence are modified.
**
** {F12981} The second parameter to the update callback 
**          is one of [SQLITE_INSERT], [SQLITE_DELETE] or [SQLITE_UPDATE],
**          depending on the operation that caused the callback to be invoked.
**
** {F12983} The third and fourth arguments to the callback contain pointers
**          to zero-terminated UTF-8 strings which are the names of the
**          database and table that is being updated.

** {F12985} The final callback parameter is the rowid of the row after
**          the change occurs.
*/
void *sqlite3_update_hook(
  sqlite3*, 
  void(*)(void *,int ,char const *,char const *,sqlite3_int64),
  void*
);

/*
** CAPI3REF:  Enable Or Disable Shared Pager Cache {F10330}
**
** This routine enables or disables the sharing of the database cache
** and schema data structures between connections to the same database.
** Sharing is enabled if the argument is true and disabled if the argument
** is false.
**
** Cache sharing is enabled and disabled
** for an entire process. {END} This is a change as of SQLite version 3.5.0.
** In prior versions of SQLite, sharing was
** enabled or disabled for each thread separately.
**
** The cache sharing mode set by this interface effects all subsequent
** calls to [sqlite3_open()], [sqlite3_open_v2()], and [sqlite3_open16()].
** Existing database connections continue use the sharing mode
** that was in effect at the time they were opened.
**
** Virtual tables cannot be used with a shared cache.   When shared
** cache is enabled, the [sqlite3_create_module()] API used to register
** virtual tables will always return an error.
**
** This routine returns [SQLITE_OK] if shared cache was
** enabled or disabled successfully.  An [error code]
** is returned otherwise.
**
** Shared cache is disabled by default. But this might change in
** future releases of SQLite.  Applications that care about shared
** cache setting should set it explicitly.
**
** INVARIANTS:
** 
** {F10331} A successful invocation of [sqlite3_enable_shared_cache(B)]
**          will enable or disable shared cache mode for any subsequently
**          created [database connection] in the same process.
**
** {F10336} When shared cache is enabled, the [sqlite3_create_module()]
**          interface will always return an error.
**
** {F10337} The [sqlite3_enable_shared_cache(B)] interface returns
**          [SQLITE_OK] if shared cache was enabled or disabled successfully.
**
** {F10339} Shared cache is disabled by default.
*/
int sqlite3_enable_shared_cache(int);

/*
** CAPI3REF:  Attempt To Free Heap Memory {F17340}
**
** The sqlite3_release_memory() interface attempts to
** free N bytes of heap memory by deallocating non-essential memory
** allocations held by the database labrary. {END}  Memory used
** to cache database pages to improve performance is an example of
** non-essential memory.  Sqlite3_release_memory() returns
** the number of bytes actually freed, which might be more or less
** than the amount requested.
**
** INVARIANTS:
**
** {F17341} The [sqlite3_release_memory(N)] interface attempts to
**          free N bytes of heap memory by deallocating non-essential
**          memory allocations held by the database labrary.
**
** {F16342} The [sqlite3_release_memory(N)] returns the number
**          of bytes actually freed, which might be more or less
**          than the amount requested.
*/
int sqlite3_release_memory(int);

/*
** CAPI3REF:  Impose A Limit On Heap Size {F17350}
**
** The sqlite3_soft_heap_limit() interface
** places a "soft" limit on the amount of heap memory that may be allocated
** by SQLite. If an internal allocation is requested 
** that would exceed the soft heap limit, [sqlite3_release_memory()] is
** invoked one or more times to free up some space before the allocation
** is made.
**
** The limit is called "soft", because if
** [sqlite3_release_memory()] cannot
** free sufficient memory to prevent the limit from being exceeded,
** the memory is allocated anyway and the current operation proceeds.
**
** A negative or zero value for N means that there is no soft heap limit and
** [sqlite3_release_memory()] will only be called when memory is exhausted.
** The default value for the soft heap limit is zero.
**
** SQLite makes a best effort to honor the soft heap limit.  
** But if the soft heap limit cannot honored, execution will
** continue without error or notification.  This is why the limit is 
** called a "soft" limit.  It is advisory only.
**
** Prior to SQLite version 3.5.0, this routine only constrained the memory
** allocated by a single thread - the same thread in which this routine
** runs.  Beginning with SQLite version 3.5.0, the soft heap limit is
** applied to all threads. The value specified for the soft heap limit
** is an upper bound on the total memory allocation for all threads. In
** version 3.5.0 there is no mechanism for limiting the heap usage for
** individual threads.
**
** INVARIANTS:
**
** {F16351} The [sqlite3_soft_heap_limit(N)] interface places a soft limit
**          of N bytes on the amount of heap memory that may be allocated
**          using [sqlite3_malloc()] or [sqlite3_realloc()] at any point
**          in time.
**
** {F16352} If a call to [sqlite3_malloc()] or [sqlite3_realloc()] would
**          cause the total amount of allocated memory to exceed the
**          soft heap limit, then [sqlite3_release_memory()] is invoked
**          in an attempt to reduce the memory usage prior to proceeding
**          with the memory allocation attempt.
**
** {F16353} Calls to [sqlite3_malloc()] or [sqlite3_realloc()] that trigger
**          attempts to reduce memory usage through the soft heap limit
**          mechanism continue even if the attempt to reduce memory
**          usage is unsuccessful.
**
** {F16354} A negative or zero value for N in a call to
**          [sqlite3_soft_heap_limit(N)] means that there is no soft
**          heap limit and [sqlite3_release_memory()] will only be
**          called when memory is completely exhausted.
**
** {F16355} The default value for the soft heap limit is zero.
**
** {F16358} Each call to [sqlite3_soft_heap_limit(N)] overrides the
**          values set by all prior calls.
*/
void sqlite3_soft_heap_limit(int);

/*
** CAPI3REF:  Extract Metadata About A Column Of A Table {F12850}
**
** This routine
** returns meta-data about a specific column of a specific database
** table accessible using the connection handle passed as the first function 
** argument.
**
** The column is identified by the second, third and fourth parameters to 
** this function. The second parameter is either the name of the database
** (i.e. "main", "temp" or an attached database) containing the specified
** table or NULL. If it is NULL, then all attached databases are searched
** for the table using the same algorithm as the database engine uses to 
** resolve unqualified table references.
**
** The third and fourth parameters to this function are the table and column 
** name of the desired column, respectively. Neither of these parameters 
** may be NULL.
**
** Meta information is returned by writing to the memory locations passed as
** the 5th and subsequent parameters to this function. Any of these 
** arguments may be NULL, in which case the corresponding element of meta 
** information is ommitted.
**
** <pre>
** Parameter     Output Type      Description
** -----------------------------------
**
**   5th         const char*      Data type
**   6th         const char*      Name of the default collation sequence 
**   7th         int              True if the column has a NOT NULL constraint
**   8th         int              True if the column is part of the PRIMARY KEY
**   9th         int              True if the column is AUTOINCREMENT
** </pre>
**
**
** The memory pointed to by the character pointers returned for the 
** declaration type and collation sequence is valid only until the next 
** call to any sqlite API function.
**
** If the specified table is actually a view, then an error is returned.
**
** If the specified column is "rowid", "oid" or "_rowid_" and an 
** INTEGER PRIMARY KEY column has been explicitly declared, then the output 
** parameters are set for the explicitly declared column. If there is no
** explicitly declared IPK column, then the output parameters are set as 
** follows:
**
** <pre>
**     data type: "INTEGER"
**     collation sequence: "BINARY"
**     not null: 0
**     primary key: 1
**     auto increment: 0
** </pre>
**
** This function may load one or more schemas from database files. If an
** error occurs during this process, or if the requested table or column
** cannot be found, an SQLITE error code is returned and an error message
** left in the database handle (to be retrieved using sqlite3_errmsg()).
**
** This API is only available if the library was compiled with the
** SQLITE_ENABLE_COLUMN_METADATA preprocessor symbol defined.
*/
int sqlite3_table_column_metadata(
  sqlite3 *db,                /* Connection handle */
  const char *zDbName,        /* Database name or NULL */
  const char *zTableName,     /* Table name */
  const char *zColumnName,    /* Column name */
  char const **pzDataType,    /* OUTPUT: Declared data type */
  char const **pzCollSeq,     /* OUTPUT: Collation sequence name */
  int *pNotNull,              /* OUTPUT: True if NOT NULL constraint exists */
  int *pPrimaryKey,           /* OUTPUT: True if column part of PK */
  int *pAutoinc               /* OUTPUT: True if column is auto-increment */
);

/*
** CAPI3REF: Load An Extension {F12600}
**
** {F12601} The sqlite3_load_extension() interface
** attempts to load an SQLite extension library contained in the file
** zFile. {F12602} The entry point is zProc. {F12603} zProc may be 0
** in which case the name of the entry point defaults
** to "sqlite3_extension_init".
**
** {F12604} The sqlite3_load_extension() interface shall
** return [SQLITE_OK] on success and [SQLITE_ERROR] if something goes wrong.
**
** {F12605}
** If an error occurs and pzErrMsg is not 0, then the
** sqlite3_load_extension() interface shall attempt to fill *pzErrMsg with 
** error message text stored in memory obtained from [sqlite3_malloc()].
** {END}  The calling function should free this memory
** by calling [sqlite3_free()].
**
** {F12606}
** Extension loading must be enabled using [sqlite3_enable_load_extension()]
** prior to calling this API or an error will be returned.
*/
int sqlite3_load_extension(
  sqlite3 *db,          /* Load the extension into this database connection */
  const char *zFile,    /* Name of the shared library containing extension */
  const char *zProc,    /* Entry point.  Derived from zFile if 0 */
  char **pzErrMsg       /* Put error message here if not 0 */
);

/*
** CAPI3REF:  Enable Or Disable Extension Loading {F12620}
**
** So as not to open security holes in older applications that are
** unprepared to deal with extension loading, and as a means of disabling
** extension loading while evaluating user-entered SQL, the following
** API is provided to turn the [sqlite3_load_extension()] mechanism on and
** off.  {F12622} It is off by default. {END} See ticket #1863.
**
** {F12621} Call the sqlite3_enable_load_extension() routine
** with onoff==1 to turn extension loading on
** and call it with onoff==0 to turn it back off again. {END}
*/
int sqlite3_enable_load_extension(sqlite3 *db, int onoff);

/*
** CAPI3REF: Make Arrangements To Automatically Load An Extension {F12640}
**
** {F12641} This function
** registers an extension entry point that is automatically invoked
** whenever a new database connection is opened using
** [sqlite3_open()], [sqlite3_open16()], or [sqlite3_open_v2()]. {END}
**
** This API can be invoked at program startup in order to register
** one or more statically linked extensions that will be available
** to all new database connections.
**
** {F12642} Duplicate extensions are detected so calling this routine multiple
** times with the same extension is harmless.
**
** {F12643} This routine stores a pointer to the extension in an array
** that is obtained from sqlite_malloc(). {END} If you run a memory leak
** checker on your program and it reports a leak because of this
** array, then invoke [sqlite3_reset_auto_extension()] prior
** to shutdown to free the memory.
**
** {F12644} Automatic extensions apply across all threads. {END}
**
** This interface is experimental and is subject to change or
** removal in future releases of SQLite.
*/
int sqlite3_auto_extension(void *xEntryPoint);


/*
** CAPI3REF: Reset Automatic Extension Loading {F12660}
**
** {F12661} This function disables all previously registered
** automatic extensions. {END}  This
** routine undoes the effect of all prior [sqlite3_auto_extension()]
** calls.
**
** {F12662} This call disabled automatic extensions in all threads. {END}
**
** This interface is experimental and is subject to change or
** removal in future releases of SQLite.
*/
void sqlite3_reset_auto_extension(void);


/*
****** EXPERIMENTAL - subject to change without notice **************
**
** The interface to the virtual-table mechanism is currently considered
** to be experimental.  The interface might change in incompatible ways.
** If this is a problem for you, do not use the interface at this time.
**
** When the virtual-table mechanism stablizes, we will declare the
** interface fixed, support it indefinitely, and remove this comment.
*/

/*
** Structures used by the virtual table interface
*/
typedef struct sqlite3_vtab sqlite3_vtab;
typedef struct sqlite3_index_info sqlite3_index_info;
typedef struct sqlite3_vtab_cursor sqlite3_vtab_cursor;
typedef struct sqlite3_module sqlite3_module;

/*
** CAPI3REF: Virtual Table Object {F18000}
** KEYWORDS: sqlite3_module
**
** A module is a class of virtual tables.  Each module is defined
** by an instance of the following structure.  This structure consists
** mostly of methods for the module.
*/
struct sqlite3_module {
  int iVersion;
  int (*xCreate)(sqlite3*, void *pAux,
               int argc, const char *const*argv,
               sqlite3_vtab **ppVTab, char**);
  int (*xConnect)(sqlite3*, void *pAux,
               int argc, const char *const*argv,
               sqlite3_vtab **ppVTab, char**);
  int (*xBestIndex)(sqlite3_vtab *pVTab, sqlite3_index_info*);
  int (*xDisconnect)(sqlite3_vtab *pVTab);
  int (*xDestroy)(sqlite3_vtab *pVTab);
  int (*xOpen)(sqlite3_vtab *pVTab, sqlite3_vtab_cursor **ppCursor);
  int (*xClose)(sqlite3_vtab_cursor*);
  int (*xFilter)(sqlite3_vtab_cursor*, int idxNum, const char *idxStr,
                int argc, sqlite3_value **argv);
  int (*xNext)(sqlite3_vtab_cursor*);
  int (*xEof)(sqlite3_vtab_cursor*);
  int (*xColumn)(sqlite3_vtab_cursor*, sqlite3_context*, int);
  int (*xRowid)(sqlite3_vtab_cursor*, sqlite3_int64 *pRowid);
  int (*xUpdate)(sqlite3_vtab *, int, sqlite3_value **, sqlite3_int64 *);
  int (*xBegin)(sqlite3_vtab *pVTab);
  int (*xSync)(sqlite3_vtab *pVTab);
  int (*xCommit)(sqlite3_vtab *pVTab);
  int (*xRollback)(sqlite3_vtab *pVTab);
  int (*xFindFunction)(sqlite3_vtab *pVtab, int nArg, const char *zName,
                       void (**pxFunc)(sqlite3_context*,int,sqlite3_value**),
                       void **ppArg);

  int (*xRename)(sqlite3_vtab *pVtab, const char *zNew);
};

/*
** CAPI3REF: Virtual Table Indexing Information {F18100}
** KEYWORDS: sqlite3_index_info
**
** The sqlite3_index_info structure and its substructures is used to
** pass information into and receive the reply from the xBestIndex
** method of an sqlite3_module.  The fields under **Inputs** are the
** inputs to xBestIndex and are read-only.  xBestIndex inserts its
** results into the **Outputs** fields.
**
** The aConstraint[] array records WHERE clause constraints of the
** form:
**
**         column OP expr
**
** Where OP is =, &lt;, &lt;=, &gt;, or &gt;=.  
** The particular operator is stored
** in aConstraint[].op.  The index of the column is stored in 
** aConstraint[].iColumn.  aConstraint[].usable is TRUE if the
** expr on the right-hand side can be evaluated (and thus the constraint
** is usable) and false if it cannot.
**
** The optimizer automatically inverts terms of the form "expr OP column"
** and makes other simplifications to the WHERE clause in an attempt to
** get as many WHERE clause terms into the form shown above as possible.
** The aConstraint[] array only reports WHERE clause terms in the correct
** form that refer to the particular virtual table being queried.
**
** Information about the ORDER BY clause is stored in aOrderBy[].
** Each term of aOrderBy records a column of the ORDER BY clause.
**
** The xBestIndex method must fill aConstraintUsage[] with information
** about what parameters to pass to xFilter.  If argvIndex>0 then
** the right-hand side of the corresponding aConstraint[] is evaluated
** and becomes the argvIndex-th entry in argv.  If aConstraintUsage[].omit
** is true, then the constraint is assumed to be fully handled by the
** virtual table and is not checked again by SQLite.
**
** The idxNum and idxPtr values are recorded and passed into xFilter.
** sqlite3_free() is used to free idxPtr if needToFreeIdxPtr is true.
**
** The orderByConsumed means that output from xFilter will occur in
** the correct order to satisfy the ORDER BY clause so that no separate
** sorting step is required.
**
** The estimatedCost value is an estimate of the cost of doing the
** particular lookup.  A full scan of a table with N entries should have
** a cost of N.  A binary search of a table of N entries should have a
** cost of approximately log(N).
*/
struct sqlite3_index_info {
  /* Inputs */
  int nConstraint;           /* Number of entries in aConstraint */
  struct sqlite3_index_constraint {
     int iColumn;              /* Column on left-hand side of constraint */
     unsigned char op;         /* Constraint operator */
     unsigned char usable;     /* True if this constraint is usable */
     int iTermOffset;          /* Used internally - xBestIndex should ignore */
  } *aConstraint;            /* Table of WHERE clause constraints */
  int nOrderBy;              /* Number of terms in the ORDER BY clause */
  struct sqlite3_index_orderby {
     int iColumn;              /* Column number */
     unsigned char desc;       /* True for DESC.  False for ASC. */
  } *aOrderBy;               /* The ORDER BY clause */

  /* Outputs */
  struct sqlite3_index_constraint_usage {
    int argvIndex;           /* if >0, constraint is part of argv to xFilter */
    unsigned char omit;      /* Do not code a test for this constraint */
  } *aConstraintUsage;
  int idxNum;                /* Number used to identify the index */
  char *idxStr;              /* String, possibly obtained from sqlite3_malloc */
  int needToFreeIdxStr;      /* Free idxStr using sqlite3_free() if true */
  int orderByConsumed;       /* True if output is already ordered */
  double estimatedCost;      /* Estimated cost of using this index */
};
#define SQLITE_INDEX_CONSTRAINT_EQ    2
#define SQLITE_INDEX_CONSTRAINT_GT    4
#define SQLITE_INDEX_CONSTRAINT_LE    8
#define SQLITE_INDEX_CONSTRAINT_LT    16
#define SQLITE_INDEX_CONSTRAINT_GE    32
#define SQLITE_INDEX_CONSTRAINT_MATCH 64

/*
** CAPI3REF: Register A Virtual Table Implementation {F18200}
**
** This routine is used to register a new module name with an SQLite
** connection.  Module names must be registered before creating new
** virtual tables on the module, or before using preexisting virtual
** tables of the module.
*/
int sqlite3_create_module(
  sqlite3 *db,               /* SQLite connection to register module with */
  const char *zName,         /* Name of the module */
  const sqlite3_module *,    /* Methods for the module */
  void *                     /* Client data for xCreate/xConnect */
);

/*
** CAPI3REF: Register A Virtual Table Implementation {F18210}
**
** This routine is identical to the sqlite3_create_module() method above,
** except that it allows a destructor function to be specified. It is
** even more experimental than the rest of the virtual tables API.
*/
int sqlite3_create_module_v2(
  sqlite3 *db,               /* SQLite connection to register module with */
  const char *zName,         /* Name of the module */
  const sqlite3_module *,    /* Methods for the module */
  void *,                    /* Client data for xCreate/xConnect */
  void(*xDestroy)(void*)     /* Module destructor function */
);

/*
** CAPI3REF: Virtual Table Instance Object {F18010}
** KEYWORDS: sqlite3_vtab
**
** Every module implementation uses a subclass of the following structure
** to describe a particular instance of the module.  Each subclass will
** be tailored to the specific needs of the module implementation.   The
** purpose of this superclass is to define certain fields that are common
** to all module implementations.
**
** Virtual tables methods can set an error message by assigning a
** string obtained from sqlite3_mprintf() to zErrMsg.  The method should
** take care that any prior string is freed by a call to sqlite3_free()
** prior to assigning a new string to zErrMsg.  After the error message
** is delivered up to the client application, the string will be automatically
** freed by sqlite3_free() and the zErrMsg field will be zeroed.  Note
** that sqlite3_mprintf() and sqlite3_free() are used on the zErrMsg field
** since virtual tables are commonly implemented in loadable extensions which
** do not have access to sqlite3MPrintf() or sqlite3Free().
*/
struct sqlite3_vtab {
  const sqlite3_module *pModule;  /* The module for this virtual table */
  int nRef;                       /* Used internally */
  char *zErrMsg;                  /* Error message from sqlite3_mprintf() */
  /* Virtual table implementations will typically add additional fields */
};

/*
** CAPI3REF: Virtual Table Cursor Object  {F18020}
** KEYWORDS: sqlite3_vtab_cursor
**
** Every module implementation uses a subclass of the following structure
** to describe cursors that point into the virtual table and are used
** to loop through the virtual table.  Cursors are created using the
** xOpen method of the module.  Each module implementation will define
** the content of a cursor structure to suit its own needs.
**
** This superclass exists in order to define fields of the cursor that
** are common to all implementations.
*/
struct sqlite3_vtab_cursor {
  sqlite3_vtab *pVtab;      /* Virtual table of this cursor */
  /* Virtual table implementations will typically add additional fields */
};

/*
** CAPI3REF: Declare The Schema Of A Virtual Table {F18280}
**
** The xCreate and xConnect methods of a module use the following API
** to declare the format (the names and datatypes of the columns) of
** the virtual tables they implement.
*/
int sqlite3_declare_vtab(sqlite3*, const char *zCreateTable);

/*
** CAPI3REF: Overload A Function For A Virtual Table {F18300}
**
** Virtual tables can provide alternative implementations of functions
** using the xFindFunction method.  But global versions of those functions
** must exist in order to be overloaded.
**
** This API makes sure a global version of a function with a particular
** name and number of parameters exists.  If no such function exists
** before this API is called, a new function is created.  The implementation
** of the new function always causes an exception to be thrown.  So
** the new function is not good for anything by itself.  Its only
** purpose is to be a place-holder function that can be overloaded
** by virtual tables.
**
** This API should be considered part of the virtual table interface,
** which is experimental and subject to change.
*/
int sqlite3_overload_function(sqlite3*, const char *zFuncName, int nArg);

/*
** The interface to the virtual-table mechanism defined above (back up
** to a comment remarkably similar to this one) is currently considered
** to be experimental.  The interface might change in incompatible ways.
** If this is a problem for you, do not use the interface at this time.
**
** When the virtual-table mechanism stabilizes, we will declare the
** interface fixed, support it indefinitely, and remove this comment.
**
****** EXPERIMENTAL - subject to change without notice **************
*/

/*
** CAPI3REF: A Handle To An Open BLOB {F17800}
**
** An instance of this object represents an open BLOB on which
** incremental I/O can be preformed.
** Objects of this type are created by
** [sqlite3_blob_open()] and destroyed by [sqlite3_blob_close()].
** The [sqlite3_blob_read()] and [sqlite3_blob_write()] interfaces
** can be used to read or write small subsections of the blob.
** The [sqlite3_blob_bytes()] interface returns the size of the
** blob in bytes.
*/
typedef struct sqlite3_blob sqlite3_blob;

/*
** CAPI3REF: Open A BLOB For Incremental I/O {F17810}
**
** This interfaces opens a handle to the blob located
** in row iRow, column zColumn, table zTable in database zDb;
** in other words,  the same blob that would be selected by:
**
** <pre>
**     SELECT zColumn FROM zDb.zTable WHERE rowid = iRow;
** </pre> {END}
**
** If the flags parameter is non-zero, the blob is opened for 
** read and write access. If it is zero, the blob is opened for read 
** access.
**
** Note that the database name is not the filename that contains
** the database but rather the symbolic name of the database that
** is assigned when the database is connected using [ATTACH].
** For the main database file, the database name is "main".  For
** TEMP tables, the database name is "temp".
**
** On success, [SQLITE_OK] is returned and the new 
** [sqlite3_blob | blob handle] is written to *ppBlob. 
** Otherwise an error code is returned and 
** any value written to *ppBlob should not be used by the caller.
** This function sets the database-handle error code and message
** accessible via [sqlite3_errcode()] and [sqlite3_errmsg()].
** 
** INVARIANTS:
**
** {F17813} A successful invocation of the [sqlite3_blob_open(D,B,T,C,R,F,P)]
**          interface opens an [sqlite3_blob] object P on the blob
**          in column C of table T in database B on [database connection] D.
**
** {F17814} A successful invocation of [sqlite3_blob_open(D,...)] starts
**          a new transaction on [database connection] D if that connection
**          is not already in a transaction.
**
** {F17816} The [sqlite3_blob_open(D,B,T,C,R,F,P)] interface opens the blob
**          for read and write access if and only if the F parameter
**          is non-zero.
**
** {F17819} The [sqlite3_blob_open()] interface returns [SQLITE_OK] on 
**          success and an appropriate [error code] on failure.
**
** {F17821} If an error occurs during evaluation of [sqlite3_blob_open(D,...)]
**          then subsequent calls to [sqlite3_errcode(D)],
**          [sqlite3_errmsg(D)], and [sqlite3_errmsg16(D)] will return
**          information approprate for that error.
*/
int sqlite3_blob_open(
  sqlite3*,
  const char *zDb,
  const char *zTable,
  const char *zColumn,
  sqlite3_int64 iRow,
  int flags,
  sqlite3_blob **ppBlob
);

/*
** CAPI3REF:  Close A BLOB Handle {F17830}
**
** Close an open [sqlite3_blob | blob handle].
**
** Closing a BLOB shall cause the current transaction to commit
** if there are no other BLOBs, no pending prepared statements, and the
** database connection is in autocommit mode.
** If any writes were made to the BLOB, they might be held in cache
** until the close operation if they will fit. {END}
** Closing the BLOB often forces the changes
** out to disk and so if any I/O errors occur, they will likely occur
** at the time when the BLOB is closed.  {F17833} Any errors that occur during
** closing are reported as a non-zero return value.
**
** The BLOB is closed unconditionally.  Even if this routine returns
** an error code, the BLOB is still closed.
**
** INVARIANTS:
**
** {F17833} The [sqlite3_blob_close(P)] interface closes an
**          [sqlite3_blob] object P previously opened using
**          [sqlite3_blob_open()].
**
** {F17836} Closing an [sqlite3_blob] object using
**          [sqlite3_blob_close()] shall cause the current transaction to
**          commit if there are no other open [sqlite3_blob] objects
**          or [prepared statements] on the same [database connection] and
**          the [database connection] is in
**          [sqlite3_get_autocommit | autocommit mode].
**
** {F17839} The [sqlite3_blob_close(P)] interfaces closes the 
**          [sqlite3_blob] object P unconditionally, even if
**          [sqlite3_blob_close(P)] returns something other than [SQLITE_OK].
**          
*/
int sqlite3_blob_close(sqlite3_blob *);

/*
** CAPI3REF:  Return The Size Of An Open BLOB {F17840}
**
** Return the size in bytes of the blob accessible via the open 
** [sqlite3_blob] object in its only argument.
**
** INVARIANTS:
**
** {F17843} The [sqlite3_blob_bytes(P)] interface returns the size
**          in bytes of the BLOB that the [sqlite3_blob] object P
**          refers to.
*/
int sqlite3_blob_bytes(sqlite3_blob *);

/*
** CAPI3REF:  Read Data From A BLOB Incrementally {F17850}
**
** This function is used to read data from an open 
** [sqlite3_blob | blob-handle] into a caller supplied buffer.
** N bytes of data are copied into buffer
** Z from the open blob, starting at offset iOffset.
**
** If offset iOffset is less than N bytes from the end of the blob, 
** [SQLITE_ERROR] is returned and no data is read.  If N or iOffset is
** less than zero [SQLITE_ERROR] is returned and no data is read.
**
** On success, SQLITE_OK is returned. Otherwise, an 
** [error code] or an [extended error code] is returned.
**
** INVARIANTS:
**
** {F17853} The [sqlite3_blob_read(P,Z,N,X)] interface reads N bytes
**          beginning at offset X from
**          the blob that [sqlite3_blob] object P refers to
**          and writes those N bytes into buffer Z.
**
** {F17856} In [sqlite3_blob_read(P,Z,N,X)] if the size of the blob
**          is less than N+X bytes, then the function returns [SQLITE_ERROR]
**          and nothing is read from the blob.
**
** {F17859} In [sqlite3_blob_read(P,Z,N,X)] if X or N is less than zero
**          then the function returns [SQLITE_ERROR]
**          and nothing is read from the blob.
**
** {F17862} The [sqlite3_blob_read(P,Z,N,X)] interface returns [SQLITE_OK]
**          if N bytes where successfully read into buffer Z.
**
** {F17865} If the requested read could not be completed,
**          the [sqlite3_blob_read(P,Z,N,X)] interface returns an
**          appropriate [error code] or [extended error code].
**
** {F17868} If an error occurs during evaluation of [sqlite3_blob_read(P,...)]
**          then subsequent calls to [sqlite3_errcode(D)],
**          [sqlite3_errmsg(D)], and [sqlite3_errmsg16(D)] will return
**          information approprate for that error, where D is the
**          database handle that was used to open blob handle P.
*/
int sqlite3_blob_read(sqlite3_blob *, void *Z, int N, int iOffset);

/*
** CAPI3REF:  Write Data Into A BLOB Incrementally {F17870}
**
** This function is used to write data into an open 
** [sqlite3_blob | blob-handle] from a user supplied buffer.
** n bytes of data are copied from the buffer
** pointed to by z into the open blob, starting at offset iOffset.
**
** If the [sqlite3_blob | blob-handle] passed as the first argument
** was not opened for writing (the flags parameter to [sqlite3_blob_open()]
*** was zero), this function returns [SQLITE_READONLY].
**
** This function may only modify the contents of the blob; it is
** not possible to increase the size of a blob using this API.
** If offset iOffset is less than n bytes from the end of the blob, 
** [SQLITE_ERROR] is returned and no data is written.  If n is
** less than zero [SQLITE_ERROR] is returned and no data is written.
**
** On success, SQLITE_OK is returned. Otherwise, an 
** [error code] or an [extended error code] is returned.
**
** INVARIANTS:
**
** {F17873} The [sqlite3_blob_write(P,Z,N,X)] interface writes N bytes
**          from buffer Z into
**          the blob that [sqlite3_blob] object P refers to
**          beginning at an offset of X into the blob.
**
** {F17875} The [sqlite3_blob_write(P,Z,N,X)] interface returns
**          [SQLITE_READONLY] if the [sqlite3_blob] object P was
**          [sqlite3_blob_open | opened] for reading only.
**
** {F17876} In [sqlite3_blob_write(P,Z,N,X)] if the size of the blob
**          is less than N+X bytes, then the function returns [SQLITE_ERROR]
**          and nothing is written into the blob.
**
** {F17879} In [sqlite3_blob_write(P,Z,N,X)] if X or N is less than zero
**          then the function returns [SQLITE_ERROR]
**          and nothing is written into the blob.
**
** {F17882} The [sqlite3_blob_write(P,Z,N,X)] interface returns [SQLITE_OK]
**          if N bytes where successfully written into blob.
**
** {F17885} If the requested write could not be completed,
**          the [sqlite3_blob_write(P,Z,N,X)] interface returns an
**          appropriate [error code] or [extended error code].
**
** {F17888} If an error occurs during evaluation of [sqlite3_blob_write(D,...)]
**          then subsequent calls to [sqlite3_errcode(D)],
**          [sqlite3_errmsg(D)], and [sqlite3_errmsg16(D)] will return
**          information approprate for that error.
*/
int sqlite3_blob_write(sqlite3_blob *, const void *z, int n, int iOffset);

/*
** CAPI3REF:  Virtual File System Objects {F11200}
**
** A virtual filesystem (VFS) is an [sqlite3_vfs] object
** that SQLite uses to interact
** with the underlying operating system.  Most SQLite builds come with a
** single default VFS that is appropriate for the host computer.
** New VFSes can be registered and existing VFSes can be unregistered.
** The following interfaces are provided.
**
** The sqlite3_vfs_find() interface returns a pointer to 
** a VFS given its name.  Names are case sensitive.
** Names are zero-terminated UTF-8 strings.
** If there is no match, a NULL
** pointer is returned.  If zVfsName is NULL then the default 
** VFS is returned. 
**
** New VFSes are registered with sqlite3_vfs_register().
** Each new VFS becomes the default VFS if the makeDflt flag is set.
** The same VFS can be registered multiple times without injury.
** To make an existing VFS into the default VFS, register it again
** with the makeDflt flag set.  If two different VFSes with the
** same name are registered, the behavior is undefined.  If a
** VFS is registered with a name that is NULL or an empty string,
** then the behavior is undefined.
** 
** Unregister a VFS with the sqlite3_vfs_unregister() interface.
** If the default VFS is unregistered, another VFS is chosen as
** the default.  The choice for the new VFS is arbitrary.
**
** INVARIANTS:
**
** {F11203} The [sqlite3_vfs_find(N)] interface returns a pointer to the
**          registered [sqlite3_vfs] object whose name exactly matches
**          the zero-terminated UTF-8 string N, or it returns NULL if
**          there is no match.
**
** {F11206} If the N parameter to [sqlite3_vfs_find(N)] is NULL then
**          the function returns a pointer to the default [sqlite3_vfs]
**          object if there is one, or NULL if there is no default 
**          [sqlite3_vfs] object.
**
** {F11209} The [sqlite3_vfs_register(P,F)] interface registers the
**          well-formed [sqlite3_vfs] object P using the name given
**          by the zName field of the object.
**
** {F11212} Using the [sqlite3_vfs_register(P,F)] interface to register
**          the same [sqlite3_vfs] object multiple times is a harmless no-op.
**
** {F11215} The [sqlite3_vfs_register(P,F)] interface makes the
**          the [sqlite3_vfs] object P the default [sqlite3_vfs] object
**          if F is non-zero.
**
** {F11218} The [sqlite3_vfs_unregister(P)] interface unregisters the
**          [sqlite3_vfs] object P so that it is no longer returned by
**          subsequent calls to [sqlite3_vfs_find()].
*/
sqlite3_vfs *sqlite3_vfs_find(const char *zVfsName);
int sqlite3_vfs_register(sqlite3_vfs*, int makeDflt);
int sqlite3_vfs_unregister(sqlite3_vfs*);

/*
** CAPI3REF: Mutexes {F17000}
**
** The SQLite core uses these routines for thread
** synchronization.  Though they are intended for internal
** use by SQLite, code that links against SQLite is
** permitted to use any of these routines.
**
** The SQLite source code contains multiple implementations 
** of these mutex routines.  An appropriate implementation
** is selected automatically at compile-time.  The following
** implementations are available in the SQLite core:
**
** <ul>
** <li>   SQLITE_MUTEX_OS2
** <li>   SQLITE_MUTEX_PTHREAD
** <li>   SQLITE_MUTEX_W32
** <li>   SQLITE_MUTEX_NOOP
** </ul>
**
** The SQLITE_MUTEX_NOOP implementation is a set of routines 
** that does no real locking and is appropriate for use in 
** a single-threaded application.  The SQLITE_MUTEX_OS2,
** SQLITE_MUTEX_PTHREAD, and SQLITE_MUTEX_W32 implementations
** are appropriate for use on os/2, unix, and windows.
** 
** If SQLite is compiled with the SQLITE_MUTEX_APPDEF preprocessor
** macro defined (with "-DSQLITE_MUTEX_APPDEF=1"), then no mutex
** implementation is included with the library.  The
** mutex interface routines defined here become external
** references in the SQLite library for which implementations
** must be provided by the application.  This facility allows an
** application that links against SQLite to provide its own mutex
** implementation without having to modify the SQLite core.
**
** {F17011} The sqlite3_mutex_alloc() routine allocates a new
** mutex and returns a pointer to it. {F17012} If it returns NULL
** that means that a mutex could not be allocated. {F17013} SQLite
** will unwind its stack and return an error. {F17014} The argument
** to sqlite3_mutex_alloc() is one of these integer constants:
**
** <ul>
** <li>  SQLITE_MUTEX_FAST
** <li>  SQLITE_MUTEX_RECURSIVE
** <li>  SQLITE_MUTEX_STATIC_MASTER
** <li>  SQLITE_MUTEX_STATIC_MEM
** <li>  SQLITE_MUTEX_STATIC_MEM2
** <li>  SQLITE_MUTEX_STATIC_PRNG
** <li>  SQLITE_MUTEX_STATIC_LRU
** <li>  SQLITE_MUTEX_STATIC_LRU2
** </ul> {END}
**
** {F17015} The first two constants cause sqlite3_mutex_alloc() to create
** a new mutex.  The new mutex is recursive when SQLITE_MUTEX_RECURSIVE
** is used but not necessarily so when SQLITE_MUTEX_FAST is used. {END}
** The mutex implementation does not need to make a distinction
** between SQLITE_MUTEX_RECURSIVE and SQLITE_MUTEX_FAST if it does
** not want to.  {F17016} But SQLite will only request a recursive mutex in
** cases where it really needs one.  {END} If a faster non-recursive mutex
** implementation is available on the host platform, the mutex subsystem
** might return such a mutex in response to SQLITE_MUTEX_FAST.
**
** {F17017} The other allowed parameters to sqlite3_mutex_alloc() each return
** a pointer to a static preexisting mutex. {END}  Four static mutexes are
** used by the current version of SQLite.  Future versions of SQLite
** may add additional static mutexes.  Static mutexes are for internal
** use by SQLite only.  Applications that use SQLite mutexes should
** use only the dynamic mutexes returned by SQLITE_MUTEX_FAST or
** SQLITE_MUTEX_RECURSIVE.
**
** {F17018} Note that if one of the dynamic mutex parameters (SQLITE_MUTEX_FAST
** or SQLITE_MUTEX_RECURSIVE) is used then sqlite3_mutex_alloc()
** returns a different mutex on every call.  {F17034} But for the static 
** mutex types, the same mutex is returned on every call that has
** the same type number. {END}
**
** {F17019} The sqlite3_mutex_free() routine deallocates a previously
** allocated dynamic mutex. {F17020} SQLite is careful to deallocate every
** dynamic mutex that it allocates. {U17021} The dynamic mutexes must not be in 
** use when they are deallocated. {U17022} Attempting to deallocate a static
** mutex results in undefined behavior. {F17023} SQLite never deallocates
** a static mutex. {END}
**
** The sqlite3_mutex_enter() and sqlite3_mutex_try() routines attempt
** to enter a mutex. {F17024} If another thread is already within the mutex,
** sqlite3_mutex_enter() will block and sqlite3_mutex_try() will return
** SQLITE_BUSY. {F17025}  The sqlite3_mutex_try() interface returns SQLITE_OK
** upon successful entry.  {F17026} Mutexes created using
** SQLITE_MUTEX_RECURSIVE can be entered multiple times by the same thread.
** {F17027} In such cases the,
** mutex must be exited an equal number of times before another thread
** can enter.  {U17028} If the same thread tries to enter any other
** kind of mutex more than once, the behavior is undefined.
** {F17029} SQLite will never exhibit
** such behavior in its own use of mutexes. {END}
**
** Some systems (ex: windows95) do not the operation implemented by
** sqlite3_mutex_try().  On those systems, sqlite3_mutex_try() will
** always return SQLITE_BUSY.  {F17030} The SQLite core only ever uses
** sqlite3_mutex_try() as an optimization so this is acceptable behavior. {END}
**
** {F17031} The sqlite3_mutex_leave() routine exits a mutex that was
** previously entered by the same thread.  {U17032} The behavior
** is undefined if the mutex is not currently entered by the
** calling thread or is not currently allocated.  {F17033} SQLite will
** never do either. {END}
**
** See also: [sqlite3_mutex_held()] and [sqlite3_mutex_notheld()].
*/
sqlite3_mutex *sqlite3_mutex_alloc(int);
void sqlite3_mutex_free(sqlite3_mutex*);
void sqlite3_mutex_enter(sqlite3_mutex*);
int sqlite3_mutex_try(sqlite3_mutex*);
void sqlite3_mutex_leave(sqlite3_mutex*);

/*
** CAPI3REF: Mutex Verifcation Routines {F17080}
**
** The sqlite3_mutex_held() and sqlite3_mutex_notheld() routines
** are intended for use inside assert() statements. {F17081} The SQLite core
** never uses these routines except inside an assert() and applications
** are advised to follow the lead of the core.  {F17082} The core only
** provides implementations for these routines when it is compiled
** with the SQLITE_DEBUG flag.  {U17087} External mutex implementations
** are only required to provide these routines if SQLITE_DEBUG is
** defined and if NDEBUG is not defined.
**
** {F17083} These routines should return true if the mutex in their argument
** is held or not held, respectively, by the calling thread. {END}
**
** {X17084} The implementation is not required to provided versions of these
** routines that actually work.
** If the implementation does not provide working
** versions of these routines, it should at least provide stubs
** that always return true so that one does not get spurious
** assertion failures. {END}
**
** {F17085} If the argument to sqlite3_mutex_held() is a NULL pointer then
** the routine should return 1.  {END} This seems counter-intuitive since
** clearly the mutex cannot be held if it does not exist.  But the
** the reason the mutex does not exist is because the build is not
** using mutexes.  And we do not want the assert() containing the
** call to sqlite3_mutex_held() to fail, so a non-zero return is
** the appropriate thing to do.  {F17086} The sqlite3_mutex_notheld() 
** interface should also return 1 when given a NULL pointer.
*/
int sqlite3_mutex_held(sqlite3_mutex*);
int sqlite3_mutex_notheld(sqlite3_mutex*);

/*
** CAPI3REF: Mutex Types {F17001}
**
** {F17002} The [sqlite3_mutex_alloc()] interface takes a single argument
** which is one of these integer constants. {END}
*/
#define SQLITE_MUTEX_FAST             0
#define SQLITE_MUTEX_RECURSIVE        1
#define SQLITE_MUTEX_STATIC_MASTER    2
#define SQLITE_MUTEX_STATIC_MEM       3  /* sqlite3_malloc() */
#define SQLITE_MUTEX_STATIC_MEM2      4  /* sqlite3_release_memory() */
#define SQLITE_MUTEX_STATIC_PRNG      5  /* sqlite3_random() */
#define SQLITE_MUTEX_STATIC_LRU       6  /* lru page list */
#define SQLITE_MUTEX_STATIC_LRU2      7  /* lru page list */

/*
** CAPI3REF: Low-Level Control Of Database Files {F11300}
**
** {F11301} The [sqlite3_file_control()] interface makes a direct call to the
** xFileControl method for the [sqlite3_io_methods] object associated
** with a particular database identified by the second argument. {F11302} The
** name of the database is the name assigned to the database by the
** <a href="lang_attach.html">ATTACH</a> SQL command that opened the
** database. {F11303} To control the main database file, use the name "main"
** or a NULL pointer. {F11304} The third and fourth parameters to this routine
** are passed directly through to the second and third parameters of
** the xFileControl method.  {F11305} The return value of the xFileControl
** method becomes the return value of this routine.
**
** {F11306} If the second parameter (zDbName) does not match the name of any
** open database file, then SQLITE_ERROR is returned. {F11307} This error
** code is not remembered and will not be recalled by [sqlite3_errcode()]
** or [sqlite3_errmsg()]. {U11308} The underlying xFileControl method might
** also return SQLITE_ERROR.  {U11309} There is no way to distinguish between
** an incorrect zDbName and an SQLITE_ERROR return from the underlying
** xFileControl method. {END}
**
** See also: [SQLITE_FCNTL_LOCKSTATE]
*/
int sqlite3_file_control(sqlite3*, const char *zDbName, int op, void*);

/*
** CAPI3REF: Testing Interface {F11400}
**
** The sqlite3_test_control() interface is used to read out internal
** state of SQLite and to inject faults into SQLite for testing
** purposes.  The first parameter a operation code that determines
** the number, meaning, and operation of all subsequent parameters.
**
** This interface is not for use by applications.  It exists solely
** for verifying the correct operation of the SQLite library.  Depending
** on how the SQLite library is compiled, this interface might not exist.
**
** The details of the operation codes, their meanings, the parameters
** they take, and what they do are all subject to change without notice.
** Unlike most of the SQLite API, this function is not guaranteed to
** operate consistently from one release to the next.
*/
int sqlite3_test_control(int op, ...);

/*
** CAPI3REF: Testing Interface Operation Codes {F11410}
**
** These constants are the valid operation code parameters used
** as the first argument to [sqlite3_test_control()].
**
** These parameters and their meansing are subject to change
** without notice.  These values are for testing purposes only.
** Applications should not use any of these parameters or the
** [sqlite3_test_control()] interface.
*/
#define SQLITE_TESTCTRL_FAULT_CONFIG             1
#define SQLITE_TESTCTRL_FAULT_FAILURES           2
#define SQLITE_TESTCTRL_FAULT_BENIGN_FAILURES    3
#define SQLITE_TESTCTRL_FAULT_PENDING            4
#define SQLITE_TESTCTRL_PRNG_SAVE                5
#define SQLITE_TESTCTRL_PRNG_RESTORE             6
#define SQLITE_TESTCTRL_PRNG_RESET               7
#define SQLITE_TESTCTRL_BITVEC_TEST              8


/*
** Undo the hack that converts floating point types to integer for
** builds on processors without floating point support.
*/
#ifdef SQLITE_OMIT_FLOATING_POINT
# undef double
#endif

#ifdef __cplusplus
}  /* End of the 'extern "C"' block */
#endif
#endif

#endif /* _h_SQLITE3_ */
/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
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


/******************************************************************************/
/* 
 *  This file is an amalgamation of all the individual source code files for
 *  Michaels Portable Runtime 2.1.0.
 *
 *  Catenating all the source into a single file makes embedding simpler and
 *  the resulting application faster, as many compilers can do whole file
 *  optimization.
 *
 *  If you want to modify mpr, you can still get the whole source
 *  as individual files if you need.
 */


/************************************************************************/
/*
 *  Start of file "../include/mprOs.h"
 */
/************************************************************************/

/*
 *  mprOs.h -- Include O/S headers and smooth out per-O/S differences
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */


/*
 *  This header is part of the Michael's Portable Runtime and aims to include
 *  all necessary O/S headers and to unify the constants and declarations 
 *  required by Embedthis products. It can be included by C or C++ programs.
 */


#ifndef _h_MPR_OS_HDRS
#define _h_MPR_OS_HDRS 1

#include    "buildConfig.h"

/*
 *  Porters, add your CPU families here and update configure code. 
 */
#define MPR_CPU_UNKNOWN     0
#define MPR_CPU_IX86        1
#define MPR_CPU_PPC         2
#define MPR_CPU_SPARC       3
#define MPR_CPU_XSCALE      4
#define MPR_CPU_ARM         5
#define MPR_CPU_MIPS        6
#define MPR_CPU_68K         7
#define MPR_CPU_SIMNT       8           /* VxWorks NT simulator */
#define MPR_CPU_SIMSPARC    9           /* VxWorks sparc simulator */
#define MPR_CPU_IX64        10          /* AMD64 or EMT64 */
#define MPR_CPU_UNIVERSAL   11          /* MAC OS X universal binaries */
#define MPR_CPU_SH4         12


/* TODO merge in freebsd, VXWORKS & MACOSX */
 
#if BLD_UNIX_LIKE && !VXWORKS && !MACOSX
    #include    <sys/types.h>
    #include    <time.h>
    #include    <arpa/inet.h>
    #include    <ctype.h>
    #include    <dirent.h>
    #include    <dlfcn.h>
    #include    <fcntl.h>
    #include    <grp.h> 
    #include    <errno.h>
    #include    <libgen.h>
    #include    <limits.h>
    #include    <netdb.h>
    #include    <net/if.h>
    #include    <netinet/in.h>
    #include    <netinet/tcp.h>
    #include    <netinet/ip.h>
    #include    <pthread.h> 
    #include    <pwd.h> 
    #include    <sys/poll.h>
#if !CYGWIN
    #include    <resolv.h>
#endif
    #include    <setjmp.h>
    #include    <signal.h>
    #include    <stdarg.h>
    #include    <stdio.h>
    #include    <stdlib.h>
    #include    <string.h>
    #include    <syslog.h>
    #include    <sys/ioctl.h>
    #include    <sys/mman.h>
    #include    <sys/stat.h>
    #include    <sys/param.h>
    #include    <sys/resource.h>
    #include    <sys/sem.h>
    #include    <sys/shm.h>
    #include    <sys/socket.h>
    #include    <sys/select.h>
    #include    <sys/time.h>
    #include    <sys/times.h>
    #include    <sys/utsname.h>
    #include    <sys/uio.h>
    #include    <sys/wait.h>
    #include    <unistd.h>

#if LINUX && !__UCLIBC__
    #include    <sys/sendfile.h>
#endif

#if CYGWIN || LINUX
    #include    <stdint.h>
#else
    #include    <netinet/in_systm.h>
#endif

#if BLD_FEATURE_FLOATING_POINT
    #define __USE_ISOC99 1
    #include    <math.h>
#if !CYGWIN
    #include    <values.h>
#endif
#endif

#endif /* BLD_UNIX_LIKE */


#if VXWORKS
    #include    <vxWorks.h>
    #include    <envLib.h>
    #include    <sys/types.h>
    #include    <time.h>
    #include    <arpa/inet.h>
    #include    <ctype.h>
    #include    <dirent.h>
    #include    <fcntl.h>
    #include    <errno.h>
    #include    <limits.h>
    #include    <loadLib.h>
    #include    <netdb.h>
    #include    <net/if.h>
    #include    <netinet/tcp.h>
    #include    <netinet/in.h>
    #include    <netinet/ip.h>
    #include    <setjmp.h>
    #include    <signal.h>
    #include    <stdarg.h>
    #include    <stdio.h>
    #include    <stdlib.h>
    #include    <string.h>
    #include    <sysSymTbl.h>
    #include    <sys/fcntlcom.h>
    #include    <sys/ioctl.h>
    #include    <sys/stat.h>
    #include    <sys/socket.h>
    #include    <sys/times.h>
    #include    <unistd.h>
    #include    <unldLib.h>

#if _WRS_VXWORKS_MAJOR >= 6
    #include    <wait.h>
#endif

    #if BLD_FEATURE_FLOATING_POINT
    #include    <float.h>
    #define __USE_ISOC99 1
    #include    <math.h>
    #endif

    #include    <sockLib.h>
    #include    <inetLib.h>
    #include    <ioLib.h>
    #include    <pipeDrv.h>
    #include    <hostLib.h>
    #include    <netdb.h>
    #include    <tickLib.h>
    #include    <taskHookLib.h>
#endif /* VXWORKS */



#if MACOSX
    #include    <time.h>
    #include    <arpa/inet.h>
    #include    <ctype.h>
    #include    <dirent.h>
    #include    <dlfcn.h>
    #include    <fcntl.h>
    #include    <grp.h> 
    #include    <errno.h>
    #include    <libgen.h>
    #include    <limits.h>
    #include    <mach-o/dyld.h>
    #include    <netdb.h>
    #include    <net/if.h>
    #include    <netinet/in.h>
    #include    <netinet/tcp.h>
    #include    <sys/poll.h>
    #include    <pthread.h> 
    #include    <pwd.h> 
    #include    <resolv.h>
    #include    <setjmp.h>
    #include    <signal.h>
    #include    <stdarg.h>
    #include    <stdio.h>
    #include    <stdlib.h>
    #include    <stdint.h>
    #include    <string.h>
    #include    <syslog.h>
    #include    <sys/ioctl.h>
    #include    <sys/mman.h>
    #include    <sys/types.h>
    #include    <sys/stat.h>
    #include    <sys/param.h>
    #include    <sys/resource.h>
    #include    <sys/sem.h>
    #include    <sys/shm.h>
    #include    <sys/socket.h>
    #include    <sys/select.h>
    #include    <sys/time.h>
    #include    <sys/times.h>
    #include    <sys/types.h>
    #include    <sys/uio.h>
    #include    <sys/utsname.h>
    #include    <sys/wait.h>
    #include    <unistd.h>
    #include    <libkern/OSAtomic.h>

    #if BLD_FEATURE_FLOATING_POINT
    #include    <float.h>
    #define __USE_ISOC99 1
    #include    <math.h>
    #endif
#endif /* MACOSX */


#if FREEBSD
    #include    <time.h>
    #include    <arpa/inet.h>
    #include    <ctype.h>
    #include    <dirent.h>
    #include    <dlfcn.h>
    #include    <fcntl.h>
    #include    <grp.h> 
    #include    <errno.h>
    #include    <libgen.h>
    #include    <limits.h>
    #include    <netdb.h>
    #include    <sys/socket.h>
    #include    <net/if.h>
    #include    <netinet/in_systm.h>
    #include    <netinet/in.h>
    #include    <netinet/tcp.h>
    #include    <netinet/ip.h>
    #include    <pthread.h> 
    #include    <pwd.h> 
    #include    <resolv.h>
    #include    <signal.h>
    #include    <stdarg.h>
    #include    <stdio.h>
    #include    <stdlib.h>
    #include    <stdint.h>
    #include    <string.h>
    #include    <syslog.h>
    #include    <sys/ioctl.h>
    #include    <sys/types.h>
    #include    <sys/stat.h>
    #include    <sys/param.h>
    #include    <sys/resource.h>
    #include    <sys/sem.h>
    #include    <sys/shm.h>
    #include    <sys/select.h>
    #include    <sys/time.h>
    #include    <sys/times.h>
    #include    <sys/types.h>
    #include    <sys/utsname.h>
    #include    <sys/wait.h>
    #include    <unistd.h>

#if BLD_FEATURE_FLOATING_POINT
    #include    <float.h>
    #define __USE_ISOC99 1
    #include    <math.h>
#endif

    #define CLD_EXITED 1
    #define CLD_KILLED 2
#endif // FREEBSD

#if BLD_WIN_LIKE
    /*
     *  We replace insecure functions with Embedthis replacements
     */
    #define     _CRT_SECURE_NO_DEPRECATE 1

    /*
     *  Need this for the latest winsock APIs
     */
    #ifndef     _WIN32_WINNT
    #define     _WIN32_WINNT 0x501
    #endif

    #include    <winsock2.h>
    #include    <ws2tcpip.h>

    #include    <ctype.h>
    #include    <conio.h>
    #include    <direct.h>
    #include    <errno.h>
    #include    <fcntl.h>
    #include    <io.h>
    #include    <limits.h>
    #include    <malloc.h>
    #include    <process.h>
    #include    <sys/stat.h>
    #include    <sys/types.h>
    #include    <setjmp.h>
    #include    <stddef.h>
    #include    <stdio.h>
    #include    <stdlib.h>
    #include    <string.h>
    #include    <stdarg.h>
    #include    <time.h>
    #include    <windows.h>
    #if BLD_FEATURE_FLOATING_POINT
    #include    <math.h>
    #include    <float.h>
    #endif
    #include    <shlobj.h>
    #include    <shellapi.h>
    #include    <wincrypt.h>

#if BLD_DEBUG
    #include    <crtdbg.h>
#endif
    /*
     *  Make windows a bit more unix like
     */

    #undef     _WIN32_WINNT
#endif /* WIN */



#if BREW

    #if BLD_FEATURE_FLOATING_POINT
    #warning "Floating point is not supported on Brew"
    #endif

    #if BLD_FEATURE_MULTITHREAD
    #warning "Multithreading is not supported on Brew"
    #endif

    #include    <AEEModGen.h>
    #include    <AEEAppGen.h>
    #include    <BREWVersion.h>

    #if BREW_MAJ_VER == 2
        /*
         *  Fix for BREW 2.X
         */
        #ifdef __GNUC__
        #define __inline extern __inline__
        #endif
        #include    <AEENet.h>
        #undef __inline
    #endif

    #include    <AEE.h>
    #include    <AEEBitmap.h>
    #include    <AEEDisp.h>
    #include    <AEEFile.h>
    #include    <AEEHeap.h>
    #include    <AEEImageCtl.h>
    #include    <AEEMedia.h>
    #include    <AEEMediaUtil.h>
    #include    <AEEMimeTypes.h>
    #include    <AEEStdLib.h>
    #include    <AEEShell.h>
    #include    <AEESoundPlayer.h>
    #include    <AEEText.h>
    #include    <AEETransform.h>
    #include    <AEEWeb.h>

    #if BREW_MAJ_VER >= 3
    #include    <AEESMS.h>
    #endif

    #include    <AEETAPI.h>
#endif /* BREW */


#ifndef BITSPERBYTE
#define BITSPERBYTE     (8 * sizeof(char))
#endif

#define BITS(type)      (BITSPERBYTE * (int) sizeof(type))

#ifndef MAXINT
#if INT_MAX
    #define MAXINT      INT_MAX
#else
    #define MAXINT      0x7fffffff
#endif
    #define MAXINT64    INT64(0x7fffffffffffffff)
#endif

/*
 *  Byte orderings
 */
#define MPR_LITTLE_ENDIAN   1
#define MPR_BIG_ENDIAN      2

/*
 *  Current endian ordering
 */
#define MPR_ENDIAN          BLD_ENDIAN

/*
 *  Conversions between integer and pointer. Caller must ensure data is not lost
 */
#if __WORDSIZE == 64 || BLD_CPU_ARCH == MPR_CPU_IX64
    #define MPR_64_BIT 1
    #define ITOP(i)         ((void*) ((int64) i))
    #define PTOI(i)         ((int) ((int64) i))
    #define LTOP(i)         ((void*) ((int64) i))
    #define PTOL(i)         ((int64) i)
#else
    #define MPR_64_BIT 0
    #define ITOP(i)         ((void*) ((int) i))
    #define PTOI(i)         ((int) i)
    #define LTOP(i)         ((void*) ((int) i))
    #define PTOL(i)         ((int64) (int) i)
#endif

#ifndef max
    #define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
    #define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif

#define MPR_ARRAY_SIZE(type)    (sizeof(type) / sizeof(type[0]))

#ifndef PRINTF_ATTRIBUTE
#if (__GNUC__ >= 3) && !DOXYGEN && BLD_DEBUG
/** 
 *  Use gcc attribute to check printf fns.  a1 is the 1-based index of the parameter containing the format, 
 *  and a2 the index of the first argument. Note that some gcc 2.x versions don't handle this properly 
 */     
#define PRINTF_ATTRIBUTE(a1, a2) __attribute__ ((format (__printf__, a1, a2)))
#else
#define PRINTF_ATTRIBUTE(a1, a2)
#endif
#endif


#undef likely
#undef unlikely
#if (__GNUC__ >= 3)
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

/*
 *  Abbreviation for const types
 */
typedef const char cchar;
typedef const unsigned char cuchar;
typedef const void cvoid;

#ifdef __cplusplus
extern "C" {
#else

#if !MACOSX
typedef int bool;
#endif
#endif

//  TODO
#define MPR_INLINE


#if CYGWIN || LINUX

#if CYGWIN
    typedef unsigned long ulong;
#endif

    typedef unsigned char uchar;

    __extension__ typedef long long int int64;
    __extension__ typedef unsigned long long int uint64;

    #define INT64(x) (x##LL)
    #define UINT64(x) (x##ULL)
    #define closesocket(x)  close(x)
    #define MPR_BINARY      ""
    #define MPR_TEXT        ""
    #define SOCKET_ERROR    -1
    #define SET_SOCKOPT_CAST void*

#if BLD_FEATURE_FLOATING_POINT
    #define MAX_FLOAT       MAXFLOAT
#endif

#define isNan(f) (f == FP_NAN)

#if CYGWIN
    #ifndef PTHREAD_MUTEX_RECURSIVE_NP
    #define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
    #endif
    #define __WALL          0

#else
    #define O_BINARY        0
    #define O_TEXT          0
    /*
     *  For some reason it is removed from fedora 6 pthreads.h and only
     *  comes in for UNIX96
     */
    extern int pthread_mutexattr_gettype (__const pthread_mutexattr_t *__restrict
                          __attr, int *__restrict __kind) __THROW;
    /* 
     *  Set the mutex kind attribute in *ATTR to KIND (either PTHREAD_MUTEX_NORMAL,
     *  PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ERRORCHECK, or PTHREAD_MUTEX_DEFAULT).  
     */
    extern int pthread_mutexattr_settype (pthread_mutexattr_t *__attr, int __kind) __THROW;
#endif

#if LINUX
/* To avoid XOPEN define */
extern char *strptime(__const char *__restrict __s, __const char *__restrict __fmt, struct tm *__tp) __THROW;
extern char **environ;
#endif

    #define true 1
    #define false 0

#endif  /* CYGWIN || LINUX  */


#if VXWORKS

    typedef unsigned char uchar;
    typedef unsigned int uint;
    typedef unsigned long ulong;
    typedef long long int int64;
    typedef unsigned long long int uint64;

    #define HAVE_SOCKLEN_T
    #define INT64(x) (x##LL)
    #define UINT64(x) (x##ULL)

    #define closesocket(x)  close(x)
    #define getpid()        taskIdSelf()

    #define MPR_BINARY      ""
    #define MPR_TEXT        ""
    #define O_BINARY        0
    #define O_TEXT          0
    #define SOCKET_ERROR    -1
    #define MSG_NOSIGNAL    0
    #define __WALL          0

    //  TODO - refactor - rename
    #define SET_SOCKOPT_CAST char*

#if BLD_FEATURE_FLOATING_POINT
    #define MAX_FLOAT       FLT_MAX
#endif

    #undef R_OK
    #define R_OK    4
    #undef W_OK
    #define W_OK    2
    #undef X_OK
    #define X_OK    1
    #undef F_OK
    #define F_OK    0

    extern int sysClkRateGet();

    #ifndef SHUT_RDWR
    #define SHUT_RDWR       2
    #endif

#if _WRS_VXWORKS_MAJOR < 6
    #define NI_MAXHOST      128
    extern STATUS access(const char *path, int mode);
    typedef int     socklen_t;
    struct sockaddr_storage {
        char        pad[1024];
    };
#else
    #if BLD_HOST_CPU_ARCH == MPR_CPU_PPC
        #define __va_copy(dest, src) *(dest) = *(src)
    #endif
    #define HAVE_SOCKLEN_T
#endif

#endif  /* VXWORKS */



#if MACOSX
    typedef unsigned long ulong;
    typedef unsigned char uchar;

    __extension__ typedef long long int int64;
    __extension__ typedef unsigned long long int uint64;

    #define INT64(x) (x##LL)
    #define UINT64(x) (x##ULL)

    #define closesocket(x)  close(x)
    #define MPR_BINARY      ""
    #define MPR_TEXT        ""
    #define O_BINARY        0
    #define O_TEXT          0
    #define SOCKET_ERROR    -1
    #define MSG_NOSIGNAL    0
    #define __WALL          0           /* 0x40000000 */
    #define SET_SOCKOPT_CAST void*
    #define PTHREAD_MUTEX_RECURSIVE_NP  PTHREAD_MUTEX_RECURSIVE

#if BLD_FEATURE_FLOATING_POINT
    #define MAX_FLOAT       MAXFLOAT
#endif
    
    /*
     *  Fix for MAC OS X - getenv
     */
    #if !HAVE_DECL_ENVIRON
    #ifdef __APPLE__
        #include <crt_externs.h>
        #define environ (*_NSGetEnviron())
    #else
        extern char **environ;
    #endif
    #endif
#endif /* MACOSX */


#if FREEBSD
    typedef unsigned long ulong;
    typedef unsigned char uchar;

    __extension__ typedef long long int int64;
    __extension__ typedef unsigned long long int uint64;
    #define INT64(x) (x##LL)

    typedef socklen_t       MprSocklen;
    #define SocketLenPtr    MprSocklen*
    #define closesocket(x)  close(x)
    #define MPR_BINARY      ""
    #define MPR_TEXT        ""
    #define O_BINARY        0
    #define O_TEXT          0
    #define SOCKET_ERROR    -1
    #define MPR_DLL_EXT     ".dylib"
    #define __WALL          0x40000000
    #define PTHREAD_MUTEX_RECURSIVE_NP  PTHREAD_MUTEX_RECURSIVE

#if BLD_FEATURE_FLOATING_POINT
    #define MAX_FLOAT       MAXFLOAT
#endif

#endif /* FREEBSD */


#if WIN
    typedef unsigned char uchar;
    typedef unsigned int uint;
    typedef unsigned long ulong;
    typedef unsigned short ushort;
    typedef __int64 int64;
    typedef unsigned __int64 uint64;

    typedef int     uid_t;
    typedef void    *handle;
    typedef char    *caddr_t;
    typedef long    pid_t;
    typedef int     gid_t;
    typedef ushort  mode_t;
    typedef void    *siginfo_t;
    typedef int     socklen_t;

    #define HAVE_SOCKLEN_T
    #define INT64(x) (x##i64)
    #define UINT64(x) (x##Ui64)

    #undef R_OK
    #define R_OK    4
    #undef W_OK
    #define W_OK    2

    /*
     *  On windows map X_OK to R_OK
     */
    #undef X_OK
    #define X_OK    4
    #undef F_OK
    #define F_OK    0

    #undef SHUT_RDWR
    #define SHUT_RDWR       2
    
    #ifndef EADDRINUSE
    #define EADDRINUSE      46
    #endif

    #ifndef EWOULDBLOCK
    #define EWOULDBLOCK     EAGAIN
    #endif

    #ifndef ENETDOWN
    #define ENETDOWN        43
    #endif

    #ifndef ECONNRESET
    #define ECONNRESET      44
    #endif

    #ifndef ECONNREFUSED
    #define ECONNREFUSED    45
    #endif

    #define MSG_NOSIGNAL    0
    #define MPR_BINARY      "b"
    #define MPR_TEXT        "t"

#if BLD_FEATURE_FLOATING_POINT
    #define MAX_FLOAT       DBL_MAX
#endif

#ifndef FILE_FLAG_FIRST_PIPE_INSTANCE
#define FILE_FLAG_FIRST_PIPE_INSTANCE   0x00080000
#endif

    #define SET_SOCKOPT_CAST const char*

    #define inline __inline
    #define chmod _chmod

    #define isNan(f) (_isnan(f))

    /*
     *  PHP can't handle this
     */
    #if !BUILDING_PHP
    #define popen _popen
    #define pclose _pclose
    #endif

/*
 *  When time began
 */
#define GENESIS UINT64(11644473600000000)

struct timezone {
  int  tz_minuteswest;      /* minutes W of Greenwich */
  int  tz_dsttime;          /* type of dst correction */
};

static int gettimeofday(struct timeval *tv, struct timezone *tz);
static char *strptime(cchar *buf, cchar *fmt, struct tm *tm);

    #define true 1
    #define false 0
#endif /* WIN */



#if SOLARIS
    typedef unsigned char uchar;
    typedef long long int int64;
    typedef unsigned long long int uint64;

    #define INT64(x) (x##LL)
    #define UINT64(x) (x##ULL)

    #define closesocket(x)  close(x)
    #define MPR_BINARY      ""
    #define MPR_TEXT        ""
    #define O_BINARY        0
    #define O_TEXT          0
    #define SOCKET_ERROR    -1
    #define MSG_NOSIGNAL    0
    #define INADDR_NONE     ((in_addr_t) 0xffffffff)
    #define __WALL  0
    #define PTHREAD_MUTEX_RECURSIVE_NP  PTHREAD_MUTEX_RECURSIVE

#if BLD_FEATURE_FLOATING_POINT
    #define MAX_FLOAT       MAXFLOAT
#endif

#endif /* SOLARIS */




#if BREW
    typedef unsigned char uchar;
    typedef unsigned int uint;
    typedef unsigned long ulong;
    typedef unsigned short ushort;
    typedef uint    off_t;
    typedef long    pid_t;

    #define O_RDONLY        0
    #define O_WRONLY        1
    #define O_RDWR          2
    #define O_CREAT         0x200
    #define O_TRUNC         0x400
    #define O_BINARY        0
    #define O_TEXT          0x20000
    #define O_EXCL          0x40000
    #define O_APPEND        0x80000

    #define R_OK    4
    #define W_OK    2
    #define X_OK    1
    #define F_OK    0

    #define SEEK_SET    0
    #define SEEK_CUR    1
    #define SEEK_END    2

    extern int  getpid();
    extern int  isalnum(int c);
    extern int  isalpha(int c);
    extern int  isdigit(int c);
    extern int  islower(int c);
    extern int  isupper(int c);
    extern int  isspace(int c);
    extern int  isxdigit(int c);

    extern uint strlen(const char *str);
    extern char *strstr(const char *string, const char *strSet);
    extern void *memset(const void *dest, int c, uint count);
    extern void exit(int status);
    extern char *strpbrk(const char *str, const char *set);
    extern uint strspn(const char *str, const char *set);
    extern int  tolower(int c);
    extern int  toupper(int c);
    extern void *memcpy(void *dest, const void *src, uint count);
    extern void *memmove(void *dest, const void *src, uint count);

    extern int  atoi(const char *str);
    extern void free(void *ptr);
    extern void *malloc(uint size);
    extern void *realloc(void *ptr, uint size);
    extern char *strcat(char *dest, const char *src);
    extern char *strchr(const char *str, int c);
    extern int  strcmp(const char *s1, const char *s2);
    extern int  strncmp(const char *s1, const char *s2, uint count);
    extern char *strcpy(char *dest, const char *src);
    extern char *strncpy(char *dest, const char *src, uint count);
    extern char *strrchr(const char *str, int c);

    #undef  printf
    #define printf DBGPRINTF

    #if BREWSIM && BLD_DEBUG
        extern _CRTIMP int __cdecl _CrtCheckMemory(void);
        extern _CRTIMP int __cdecl _CrtSetReportHook();
    #endif

    #ifndef _TIME_T_DEFINED
        typedef int64 time_t;
    #endif

    /*
     *  When time began
     */
    #define GENESIS UINT64(11644473600000000)

    struct timezone {
      int  tz_minuteswest;      /* minutes W of Greenwich */
      int  tz_dsttime;          /* type of dst correction */
    };

    static int gettimeofday(struct timeval *tv, struct timezone *tz);
    static char *strptime(cchar *buf, cchar *fmt, struct tm *tm);

#endif /* BREW */


typedef off_t MprOffset;

//  TODO fix. Use intptr_t
#if BLD_UNIX_LIKE
    typedef pthread_t   MprOsThread;
#elif BLD_CPU_ARCH == MPR_CPU_IX64
    typedef int64       MprOsThread;
#else
    typedef int         MprOsThread;
#endif


#ifdef __cplusplus
}
#endif

#endif /* _h_MPR_OS_HDRS */

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
/************************************************************************/
/*
 *  End of file "../include/mprOs.h"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../include/mprTune.h"
 */
/************************************************************************/

/*
 *  mprTune.h - Header for the Michael's Portable Runtime (MPR) Base.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/*
 *  See mpr.dox for additional documentation.
 */


#ifndef _h_MPR_TUNE
#define _h_MPR_TUNE 1


#include    "buildConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Build tuning
 */
#define MPR_TUNE_SIZE       1       /* Tune for size */
#define MPR_TUNE_BALANCED   2       /* Tune balancing speed and size */
#define MPR_TUNE_SPEED      3       /* Tune for speed */

#ifndef BLD_TUNE
#define BLD_TUNE MPR_TUNE_BALANCED
#endif


#if BLD_TUNE == MPR_TUNE_SIZE || DOXYGEN
    /*
     *  Squeeze mode optimizes to reduce memory usage
     */
    #define MPR_MAX_FNAME           128         /**< Reasonable filename size */
    #define MPR_MAX_PATH            256         /**< Reasonable path name size */
    #define MPR_DEFAULT_STACK       32768       /**< Default stack size (32K) */
    #define MPR_MAX_STRING          1024        /**< Maximum (stack) string size */
    
    #define MPR_DEFAULT_ALLOC       64          /**< Default small alloc size */
    #define MPR_DEFAULT_HASH_SIZE   23          /**< Default size of hash table */ 
    #define MPR_MAX_ARGC            128         /**< Reasonable max of args */
    #define MPR_MAX_LOG_STRING      512         /**< Maximum log message */
    #define MPR_BUFSIZE             1024        /**< Reasonable size for buffers */
    #define MPR_XML_BUFSIZE         512         /**< XML read buffer size */
    #define MPR_LIST_INCR           8           /**< Default list growth inc */
    #define MPR_BUF_INCR            1024        /**< Default buffer growth inc */
    #define MPR_MAX_BUF             4194304     /**< Max buffer size */
    #define MPR_HTTP_BUFSIZE        2048        /**< HTTP buffer size. Must fit complete HTTP headers */
    #define MPR_SSL_BUFSIZE         2048        /**< SSL has 16K max*/
    #define MPR_FILES_HASH_SIZE     29          /** Hash size for rom file system */
    #define MPR_TIME_HASH_SIZE      67          /** Hash size for time token lookup */
    #define MPR_HTTP_MAX_PASS       64          /**< Size of password */
    #define MPR_HTTP_MAX_USER       64          /**< Size of user name */
    #define MPR_HTTP_MAX_SECRET     32          /**< Random bytes to use */
    
#elif BLD_TUNE == MPR_TUNE_BALANCED
    
    /*
     *  Tune balancing speed and size
     */
    #define MPR_MAX_FNAME           256
    #define MPR_MAX_PATH            1024
    #define MPR_DEFAULT_STACK       65536
    #define MPR_MAX_STRING          2048
    #define MPR_DEFAULT_ALLOC       256
    #define MPR_DEFAULT_HASH_SIZE   43
    #define MPR_MAX_ARGC            256
    #define MPR_MAX_LOG_STRING      8192
    #define MPR_BUFSIZE             1024
    #define MPR_XML_BUFSIZE         1024
    #define MPR_LIST_INCR           16
    #define MPR_BUF_INCR            1024
    #define MPR_MAX_BUF             -1
    #define MPR_HTTP_BUFSIZE        4096
    #define MPR_SSL_BUFSIZE         4096
    #define MPR_FILES_HASH_SIZE     61
    #define MPR_TIME_HASH_SIZE      89
    
    #define MPR_HTTP_MAX_PASS       128
    #define MPR_HTTP_MAX_USER       64
    #define MPR_HTTP_MAX_SECRET     32
    
#else
    /*
     *  Tune for speed
     */
    #define MPR_MAX_FNAME           1024
    #define MPR_MAX_PATH            2048
    #define MPR_DEFAULT_STACK       131072
    #define MPR_MAX_STRING          4096
    #define MPR_DEFAULT_ALLOC       512
    #define MPR_DEFAULT_HASH_SIZE   97
    #define MPR_MAX_ARGC            512
    #define MPR_MAX_LOG_STRING      8192
    #define MPR_BUFSIZE             1024
    #define MPR_XML_BUFSIZE         1024
    #define MPR_LIST_INCR           16
    #define MPR_BUF_INCR            1024
    #define MPR_MAX_BUF             -1
    #define MPR_HTTP_BUFSIZE        8192
    #define MPR_SSL_BUFSIZE         4096
    #define MPR_FILES_HASH_SIZE     61
    #define MPR_TIME_HASH_SIZE      97
    
    #define MPR_HTTP_MAX_PASS       128
    #define MPR_HTTP_MAX_USER       64
    #define MPR_HTTP_MAX_SECRET     32
#endif


#if BLD_FEATURE_IPV6
#define MPR_MAX_IP_NAME         NI_MAXHOST      /**< Maximum size of a host name string */
#define MPR_MAX_IP_ADDR         128             /**< Maximum size of an IP address */
#define MPR_MAX_IP_PORT         6               /**< MMaximum size of a port number */

#define MPR_MAX_IP_ADDR_PORT    (MPR_MAX_IP_ADDR + NI_MAXSERV)  /**< Maximum size of an IP address with port number */

#else
/*
 *  IPv4 support only
 */
#define MPR_MAX_IP_NAME         128
#define MPR_MAX_IP_ADDR         16
#define MPR_MAX_IP_PORT         6
#define MPR_MAX_IP_ADDR_PORT    32
#endif

/*
 *  Signal sent on Unix to break out of a select call.
 */
#define MPR_WAIT_SIGNAL         (SIGUSR2)

/*
 *  Socket event message
 */
#define MPR_SOCKET_MESSAGE      (WM_USER + 32)

/*
 *  Priorities
 */
#define MPR_BACKGROUND_PRIORITY 15          /**< May only get CPU if idle */
#define MPR_LOW_PRIORITY        25
#define MPR_NORMAL_PRIORITY     50          /**< Normal (default) priority */
#define MPR_HIGH_PRIORITY       75
#define MPR_CRITICAL_PRIORITY   99          /**< May not yield */

#define MPR_EVENT_PRIORITY      75          /**< Run service event thread at higher priority */
#define MPR_POOL_PRIORITY       60          /**< Slightly elevated priority */
#define MPR_REQUEST_PRIORITY    50          /**< Normal priority */

/* 
 *  Timeouts
 */
#define MPR_TICKS_PER_SEC       1000        /**< Time ticks per second */
#define MPR_HTTP_TIMEOUT        60000       /**< HTTP Request timeout (60 sec) */
#define MPR_TIMEOUT_LOG_STAMP   3600000     /**< Time between log time stamps (1 hr) */
#define MPR_TIMEOUT_PRUNER      600000      /**< Time between pruner runs (10 min) */
#define MPR_TIMEOUT_START_TASK  2000        /**< Time to start tasks running */
#define MPR_TIMEOUT_STOP_TASK   10000       /**< Time to stop running tasks */
#define MPR_TIMEOUT_STOP_THREAD 10000       /**< Time to stop running threads */
#define MPR_TIMEOUT_STOP        5000        /**< Wait when stopping resources */
#define MPR_TIMEOUT_LINGER      2000        /**< Close socket linger timeout */


/*
 *  Default thread counts
 */
#if BLD_FEATURE_MULTITHREAD || DOXYGEN
#define MPR_DEFAULT_MIN_THREADS 0           /**< Default min threads (0) */
#define MPR_DEFAULT_MAX_THREADS 10          /**< Default max threads (10) */
#else
#define MPR_DEFAULT_MIN_THREADS 0
#define MPR_DEFAULT_MAX_THREADS 0
#endif

/*
 *  Debug control
 */
#define MPR_MAX_BLOCKED_LOCKS   100         /* Max threads blocked on lock */
#define MPR_MAX_RECURSION       15          /* Max recursion with one thread */
#define MPR_MAX_LOCKS           512         /* Total lock count max */
#define MPR_MAX_LOCK_TIME       (60 * 1000) /* Time in msec to hold a lock */

#define MPR_TIMER_TOLERANCE     2           /* Used in timer calculations */

/*
 *  Events
 */
#define MPR_EVENT_TIME_SLICE    20          /* 20 msec */


/*
 *  HTTP
 */
#define MPR_HTTP_RETRIES        (2)

#ifdef __cplusplus
}
#endif

#endif /* _h_MPR_TUNE */


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
/************************************************************************/
/*
 *  End of file "../include/mprTune.h"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../include/mpr.h"
 */
/************************************************************************/

/*
 *  mpr.h -- Header for the Michael's Portable Runtime (MPR).
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/**
 *  @file mpr.h
 *  Michael's Portable Runtime (MPR) is a portable runtime core for embedded applications.
 *  The MPR provides management for logging, error handling, events, files, http, memory, ssl, sockets, strings, 
 *  xml parsing, and date/time functions. It also provides a foundation of safe routines for secure programming, 
 *  that help to prevent buffer overflows and other security threats. It is correctly handles null arguments without
 *  crashing. The MPR is a library and a C API that can be used in both C and C++ programs.
 *  \n\n
 *  The MPR uses by convention a set extended typedefs for common types. These include: bool, cchar, cvoid, uchar, 
 *  short, ushort, int, uint, long, ulong, int64, uint64, float, and double. The cchar type is a const char, 
 *  cvoid is const void, and several types have "u" prefixes to denote unsigned qualifiers.
 *  \n\n
 *  The MPR includes a memory manager to minimize memory leaks and maximize allocation efficiency. It utilizes 
 *  a heap and slab allocators with tree links. All memory allocated is connected to a parent memory block thus forming a
 *  tree. When any block is freed, all child blocks are also freed. Most MPR APIs take a memory parent context 
 *  as the first parameter.
 *  \n\n
 *  Many of these APIs are not thread-safe. If utilizing multithreaded programming on a supporting 
 *      operating system, be careful if you modify or delete the underlying data while accessing the resource 
 *      from another thread.
 */

#ifndef _h_MPR
#define _h_MPR 1




#ifdef __cplusplus
extern "C" {
#endif


struct  Mpr;
struct  MprBlk;
struct  MprBuf;
struct  MprEvent;
struct  MprEventService;
struct  MprFile;
struct  MprFileInfo;
struct  MprFileService;
struct  MprHeap;
struct  MprHttp;
struct  MprModule;
struct  MprOsService;
struct  MprSocket;
struct  MprSocketService;
struct  MprSsl;
struct  MprWaitService;
struct  MprWaitHandler;
struct  MprXml;

#if BLD_FEATURE_MULTITHREAD
struct  MprCond;
struct  MprMutex;
struct  MprThreadService;
struct  MprThread;
struct  MprPoolService;
struct  MprPoolThread;
#endif


//  TODO - make these consistent: CANT vs NOT or NO

/**
 *  Standard MPR return and error codes
 */
#define MPR_ERR_OK                      0       /**< Success */
#define MPR_ERR_BASE                    -1      /*   Base error code */
#define MPR_ERR                         -1      /**< Default error code */
#define MPR_ERR_GENERAL                 -1      /**< General error */
#define MPR_ERR_ABORTED                 -2      /**< Action aborted */
#define MPR_ERR_ALREADY_EXISTS          -3      /**< Item already exists */
#define MPR_ERR_BAD_ARGS                -4      /**< Bad arguments or paramaeters */
#define MPR_ERR_BAD_FORMAT              -5      /**< Bad input format */
#define MPR_ERR_BAD_HANDLE              -6
#define MPR_ERR_BAD_STATE               -7      /**< Module is in a bad state */
#define MPR_ERR_BAD_SYNTAX              -8      /**< Input has bad syntax */
#define MPR_ERR_BAD_TYPE                -9
#define MPR_ERR_BAD_VALUE               -10
#define MPR_ERR_BUSY                    -11
#define MPR_ERR_CANT_ACCESS             -12     /**< Can't access the file or resource */
#define MPR_ERR_CANT_COMPLETE           -13
#define MPR_ERR_CANT_CREATE             -14     /**< Can't create the file or resource */
#define MPR_ERR_CANT_INITIALIZE         -15
#define MPR_ERR_CANT_OPEN               -16     /**< Can't open the file or resource */
#define MPR_ERR_CANT_READ               -17     /**< Can't read from the file or resource */
#define MPR_ERR_CANT_WRITE              -18     /**< Can't write to the file or resource */
#define MPR_ERR_DELETED                 -19
#define MPR_ERR_NETWORK                 -20
#define MPR_ERR_NOT_FOUND               -21
#define MPR_ERR_NOT_INITIALIZED         -22     /**< Module or resource is not initialized */
#define MPR_ERR_NOT_READY               -23
#define MPR_ERR_READ_ONLY               -24     /**< The operation timed out */
#define MPR_ERR_TIMEOUT                 -25
#define MPR_ERR_TOO_MANY                -26
#define MPR_ERR_WONT_FIT                -27
#define MPR_ERR_WOULD_BLOCK             -28
#define MPR_ERR_CANT_ALLOCATE           -29
#define MPR_ERR_NO_MEMORY               -30     /**< Memory allocation error */
#define MPR_ERR_CANT_DELETE             -31
#define MPR_ERR_CANT_CONNECT            -32
#define MPR_ERR_MAX                     -33

/**
 *  Standard logging trace levels are 0 to 9 with 0 being the most verbose. These are ored with the error source
 *  and type flags. The MPR_LOG_MASK is used to extract the trace level from a flags word. We expect most apps
 *  to run with level 2 trace enabled.
 */
#define MPR_ERROR       1       /* Hard error trace level */
#define MPR_WARN        2       /* Soft warning trace level */
#define MPR_CONFIG      2       /* Configuration settings trace level. */
#define MPR_INFO        3       /* Informational trace only */
#define MPR_DEBUG       4       /* Debug information trace level */
#define MPR_VERBOSE     9       /* Highest level of trace */
#define MPR_LEVEL_MASK  0xf     /* Level mask */

/*
 *  Error source flags
 */
#define MPR_ERROR_SRC   0x10    /* Originated from mprError */
#define MPR_LOG_SRC     0x20    /* Originated from mprLog */
#define MPR_ASSERT_SRC  0x40    /* Originated from mprAssert */
#define MPR_FATAL_SRC   0x80    /* Fatal error. Log and exit */

/*
 *  Log message type flags. Specify what kind of log / error message it is. Listener handlers examine this flag
 *  to determine if they should process the message.Assert messages are trapped when in DEV mode. Otherwise ignored.
 */
#define MPR_LOG_MSG     0x100   /* Log trace message - not an error */
#define MPR_ERROR_MSG   0x200   /* General error */
#define MPR_ASSERT_MSG  0x400   /* Assert flags -- trap in debugger */
#define MPR_USER_MSG    0x800   /* User message */

/*
 *  Log output modifiers
 */
#define MPR_RAW         0x1000  /* Raw trace output */

/*
 *  Error line number information.
 */
#define MPR_LINE(s)     #s
#define MPR_LINE2(s)    MPR_LINE(s)
#define MPR_LINE3       MPR_LINE2(__LINE__)
#define MPR_LOC        __FILE__ ":" MPR_LINE3

#define MPR_STRINGIFY(s) #s

/**
 *  Trigger a breakpoint.
 *  @description Triggers a breakpoint and traps to the debugger. 
 *  @ingroup Mpr
 */
extern void mprBreakpoint();

#if BLD_FEATURE_ASSERT
    #define mprAssert(C)    if (C) ; else mprStaticAssert(MPR_LOC, #C)
#else
    #define mprAssert(C)    if (1) ; else
#endif

/*
 *  Parameter values for serviceEvents(loopOnce)
 */
#define MPR_LOOP_ONCE           1
#define MPR_LOOP_FOREVER        0

#define MPR_TEST_TIMEOUT        10000       /* Ten seconds */
#define MPR_TEST_LONG_TIMEOUT   300000      /* 5 minutes */
#define MPR_TEST_SHORT_TIMEOUT  200         /* 1/5 sec */
#define MPR_TEST_NAP            50          /* Short timeout to prevent busy waiting */

/**
 *  Memory Allocation Service.
 *  @description The MPR provides a memory manager that sits above malloc. This layer provides arena and slab 
 *  based allocations with a tree structured allocation mechanism. The goal of the layer is to provide 
 *  a fast, secure, scalable memory allocator suited for embedded applications in multithreaded environments. 
 *  \n\n
 *  By using a tree structured network of memory contexts, error recovery in applications and memory freeing becomes
 *  much easier and more reliable. When a memory block is allocated a parent memory block must be specified. When
 *  the parent block is freed, all its children are automatically freed. 
 *  \n\n
 *  The MPR handles memory allocation errors globally. The application can configure a memory limits and redline
 *  so that memory depletion can be proactively detected and handled. This relieves most cost from detecting and
 *  handling allocation errors. 
 *  @stability Evolving
 *  @defgroup MprMem MprMem
 *  @see MprCtx, mprFree, mprRealloc, mprAlloc, mprAllocObject, mprAllocObjectZeroed, mprAllocZeroed, mprGetParent, 
 *      mprCreate, mprSetAllocLimits, mprAllocObjWithDestructor, mprAllocObjWithDestructorZeroed,
 *      mprHasAllocError mprResetAllocError, mprMemdup, mprStrndup, mprMemcpy, 
 */
typedef struct MprMem { int dummy; } MprMem;

/**
 *  Memory context type.
 *  @description Blocks of memory are allocated using a memory context as the parent. Any allocated memory block
 *      may serve as the memory context for subsequent memory allocations. Freeing a block via \ref mprFree
 *      will release the allocated block and all child blocks.
 *  @ingroup MprMem
 */
typedef cvoid *MprCtx;

/**
 *  Safe String Module
 *  @description The MPR provides a suite of safe string manipulation routines to help prevent buffer overflows
 *      and other potential security traps.
 *  @see MprString, mprAllocSprintf, mprAllocStrcat, mprAllocStrcpy, mprAllocVsprintf, mprAtoi, mprItoa, mprMemcpy,
 *      mprPrintf, mprReallocStrcat, mprSprintf, mprStaticPrintf, mprStrLower, mprStrTok, mprStrTrim, mprStrUpper,
 *      mprStrcmpAnyCase, mprStrcmpAnyCaseCount, mprStrcpy, mprStrlen, mprVsprintf, mprErrorPrintf,
 *      mprAllocStrcat, mprAllocStrcpy, mprReallocStrcat, mprAllocSprintf, mprAllocVsprintf, mprAllocSprintf
 */
typedef struct MprString { int dummy; } MprString;

//  TODO - rename
/**
 *  Print a formatted message to the standard error channel
 *  @description This is a secure replacement for fprintf(stderr. 
 *  @param ctx Any memory context allocated by the MPR.
 *  @param fmt Printf style format string
 *  @return Returns the number of bytes written
 *  @ingroup MprString
 */
extern int mprErrorPrintf(MprCtx ctx, cchar *fmt, ...);

/**
 *  Print a message to the applications standard output without allocating memory.
 *  @description This is a secure replacement for printf that will not allocate memory.
 *  @param ctx Any memory context allocated by the MPR. This is used to locate the standard output channel and not
 *      to allocate memory.
 *  @param fmt Printf style format string
 *  @return Returns the number of bytes written
 *  @remarks The maximum output is MPR_MAX_STRING - 1.
 *  @ingroup MprString
 */
extern int mprStaticPrintf(MprCtx ctx, cchar *fmt, ...);

/**
 *  Print a message to the standard error channel without allocating memory.
 *  @description This is a secure replacement for fprintf(stderr that will not allocate memory.
 *  @param ctx Any memory context allocated by the MPR. This is used to locate the standard output channel and not
 *      to allocate memory.
 *  @param fmt Printf style format string
 *  @return Returns the number of bytes written
 *  @remarks The maximum output is MPR_MAX_STRING - 1.
 *  @ingroup MprString
 */
extern int mprStaticErrorPrintf(MprCtx ctx, cchar *fmt, ...);

/**
 *  Formatted print. This is a secure verion of printf that can handle null args.
 *  @description This is a secure replacement for printf. It can handle null arguments without crashes.
 *      minimal footprint. The MPR can be build without using any printf routines.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param fmt Printf style format string
 *  @return Returns the number of bytes written
 *  @ingroup MprString
 */
extern int mprPrintf(MprCtx ctx, cchar *fmt, ...);

/**
 *  Print a formatted message to a file descriptor
 *  @description This is a replacement for fprintf as part of the safe string MPR library. It minimizes 
 *      memory use and uses a file descriptor instead of a File pointer.
 *  @param file MprFile object returned via mprOpen.
 *  @param fmt Printf style format string
 *  @return Returns the number of bytes written
 *  @ingroup MprString
 */
extern int mprFprintf(struct MprFile *file, cchar *fmt, ...);

/**
 *  Format a string into a statically allocated buffer.
 *  @description This call format a string using printf style formatting arguments. A trailing null will 
 *      always be appended. The call returns the size of the allocated string excluding the null.
 *  @param buf Pointer to the buffer.
 *  @param maxSize Size of the buffer.
 *  @param fmt Printf style format string
 *  @return Returns the number of characters in the string.
 *  @ingroup MprString
 */
extern int mprSprintf(char *buf, int maxSize, cchar *fmt, ...);

/**
 *  Format a string into a statically allocated buffer.
 *  @description This call format a string using printf style formatting arguments. A trailing null will 
 *      always be appended. The call returns the size of the allocated string excluding the null.
 *  @param buf Pointer to the buffer.
 *  @param maxSize Size of the buffer.
 *  @param fmt Printf style format string
 *  @param args Varargs argument obtained from va_start.
 *  @return Returns the number of characters in the string.
 *  @ingroup MprString
 */
extern int mprVsprintf(char *buf, int maxSize, cchar *fmt, va_list args);

/**
 *  Convert an integer to a string.
 *  @description This call converts the supplied integer into a string formatted into the supplied buffer.
 *  @param buf Pointer to the buffer that will hold the string.
 *  @param size Size of the buffer.
 *  @param value Integer value to convert
 *  @param radix The base radix to use when encoding the number
 *  @return Returns the number of characters in the string.
 *  @ingroup MprString
 */
extern char *mprItoa(char *buf, int size, int value, int radix);

/**
 *  Convert a string to an integer.
 *  @description This call converts the supplied string to an integer using the specified radix (base).
 *  @param str Pointer to the string to parse.
 *  @param radix Base to use when parsing the string
 *  @return Returns the integer equivalent value of the string. 
 *  @ingroup MprString
 */
extern int mprAtoi(cchar *str, int radix);

/**
 *  Get the next word token.
 *  @description Split a string into word tokens using the supplied delimiter.
 *  @param buf Buffer to use to hold the word token
 *  @param bufsize Size of the buffer
 *  @param str Input string to tokenize. Note this cannot be a const string. It will be written.
 *  @param delim String of delimiter characters to use when tokenizing
 *  @param tok Pointer to a word to hold a pointer to the next token in the original string.
 *  @return Returns the number of bytes in the allocated block.
 *  @ingroup MprString
 */
extern char *mprGetWordTok(char *buf, int bufsize, cchar *str, cchar *delim, cchar **tok);

/**
 *  Safe copy for a block of data.
 *  @description Safely copy a block of data into an existing memory block. The call ensures the destination 
 *      block is not overflowed and returns the size of the block actually copied. This is similar to memcpy, but 
 *      is a safer alternative.
 *  @param dest Pointer to the destination block.
 *  @param destMax Maximum size of the destination block.
 *  @param src Block to copy
 *  @param nbytes Size of the source block
 *  @return Returns the number of characters in the allocated block.
 *  @ingroup MprString
 */
extern int mprMemcpy(void *dest, int destMax, cvoid *src, int nbytes);

/**
 *  Compare two byte strings.
 *  @description Safely compare two byte strings. This is a safe replacement for memcmp.
 *  @param b1 Pointer to the first byte string.
 *  @param b1Len Length of the first byte string.
 *  @param b2 Pointer to the second byte string.
 *  @param b2Len Length of the second byte string.
 *  @return Returns zero if the byte strings are identical. Otherwise returns -1 if the first string is less than the 
 *      second. Returns 1 if the first is greater than the first.
 *  @ingroup MprString
 */
extern int mprMemcmp(cvoid *b1, int b1Len, cvoid *b2, int b2Len);

// TODO - should remove delimiter
/**
 *  Catenate strings.
 *  @description Safe replacement for strcat. Catenates a string onto an existing string. This call accepts 
 *      a variable list of strings to append. The list of strings is terminated by a null argument. The call
 *      returns the length of the resulting string. This call is similar to strcat, but it will enforce a 
 *      maximum size for the resulting string and will ensure it is terminated with a null.
 *  @param dest Pointer to a pointer that will hold the address of the allocated block.
 *  @param max Maximum size of the new block.
 *  @param delim String of delimiter characters to insert between appended strings.
 *  @param src Variable list of strings to append. The final string argument must be null.
 *  @return Returns the number of characters in the resulting string.
 *  @ingroup MprString
 */
extern int mprStrcat(char *dest, int max, cchar *delim, cchar *src, ...);

/**
 *  Copy a string.
 *  @description Safe replacement for strcpy. Copy a string and ensure the target string is not overflowed. 
 *      The call returns the length of the resultant string or an error code if it will not fit into the target
 *      string. This is similar to strcpy, but it will enforce a maximum size for the copied string and will 
 *      ensure it is terminated with a null.
 *  @param dest Pointer to a pointer that will hold the address of the allocated block.
 *  @param destMax Maximum size of the target string.
 *  @param src String to copy
 *  @return Returns the number of characters in the target string.
 *  @ingroup MprString
 */
extern int mprStrcpy(char *dest, int destMax, cchar *src);

/**
 *  Find a substring.
 *  @description Locate the first occurrence of pattern in a string, but do not search more than the given length. 
 *  @param str Pointer to the string to search.
 *  @param pattern String pattern to search for.
 *  @param len Count of characters in the pattern to actually search for.
 *  @return Returns the number of characters in the target string.
 *  @ingroup MprString
 */
extern char *mprStrnstr(cchar *str, cchar *pattern, int len);

/**
 *  Compare strings.
 *  @description Compare two strings. This is a safe replacement for strcmp. It can handle null args.
 *  @param str1 First string to compare.
 *  @param str2 Second string to compare.
 *  @return Returns zero if the strings are identical. Return -1 if the first string is less than the second. Return 1
 *      if the first string is greater than the second.
 *  @ingroup MprString
 */
extern int mprStrcmp(cchar *str1, cchar *str2);

/**
 *  Compare strings ignoring case.
 *  @description Compare two strings ignoring case differences. This call operates similarly to strcmp.
 *  @param str1 First string to compare.
 *  @param str2 Second string to compare. 
 *  @return Returns zero if the strings are equivalent, < 0 if s1 sorts lower than s2 in the collating sequence 
 *      or > 0 if it sorts higher.
 *  @ingroup MprString
 */
extern int mprStrcmpAnyCase(cchar *str1, cchar *str2);

/**
 *  Compare strings ignoring case.
 *  @description Compare two strings ignoring case differences for a given string length. This call operates 
 *      similarly to strncmp.
 *  @param str1 First string to compare.
 *  @param str2 Second string to compare.
 *  @param len Length of characters to compare.
 *  @return Returns zero if the strings are equivalent, < 0 if s1 sorts lower than s2 in the collating sequence 
 *      or > 0 if it sorts higher.
 *  @ingroup MprString
 */
extern int mprStrcmpAnyCaseCount(cchar *str1, cchar *str2, int len);

/**
 *  Return the length of a string.
 *  @description Safe replacement for strlen. This call returns the length of a string and tests if the length is 
 *      less than a given maximum.
 *  @param src String to measure.
 *  @param max Maximum length for the string
 *  @return Returns the length of the string or MPR_ERR_WONT_FIT if the length is greater than \a max.
 *  @ingroup MprString
 */
extern int mprStrlen(cchar *src, int max);

/**
 *  Convert a string to lower case.
 *  @description Convert a string to its lower case equivalent.
 *  @param str String to convert.
 *  @return Returns a pointer to the converted string. Will always equal str.
 *  @ingroup MprString
 */
extern char *mprStrLower(char *str);


/**
 *  Convert a string to upper case.
 *  @description Convert a string to its upper case equivalent.
 *  @param str String to convert.
 *  @return Returns a pointer to the converted string. Will always equal str.
 *  @ingroup MprString
 */
extern char *mprStrUpper(char *str);

/**
 *  Trim a string.
 *  @description Trim leading and trailing characters off a string.
 *  @param str String to trim.
 *  @param set String of characters to remove.
 *  @return Returns a pointer to the trimmed string. May not equal \a str. If \a str was dynamically allocated, 
 *      do not call mprFree on the returned trimmed pointer. You must use \a str when calling mprFree.
 *  @ingroup MprString
 */
extern char *mprStrTrim(char *str, cchar *set);

/**
 *  Tokenize a string
 *  @description Split a string into tokens.
 *  @param str String to tokenize.
 *  @param delim String of characters to use as token separators.
 *  @param last Last token pointer.
 *  @return Returns a pointer to the next token.
 *  @ingroup MprString
 */
extern char *mprStrTok(char *str, cchar *delim, char **last);

/**
 *  Duplicate a string.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @description Copy a string into a newly allocated block. The new block will be sized to the maximum of the 
 *      length of the existing string (plus a null) and the requested size.
 *  @param str Pointer to the block to duplicate.
 *  @param size Requested minimum size of the allocated block holding the duplicated string.
 *  @return Returns an allocated block. Caller must free via #mprFree.
 *  @ingroup MprMem
 */
extern char *mprStrndup(MprCtx ctx, cchar *str, uint size);

/**
 *  Safe replacement for strdup
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @description mprStrdup() should be used as a replacement for \b strdup wherever possible. It allows the 
 *      strdup to be copied to be NULL, in which case it will allocate an empty string. 
 *  @param str Pointer to string to duplicate. If \b str is NULL, allocate a new string containing only a 
 *      trailing NULL character.
 *  @return Returns an allocated string including trailing null.
 *  @remarks Memory allocated via mprStrdup() must be freed via mprFree().
 *  @ingroup MprMem
 */
extern char *mprStrdup(MprCtx ctx, cchar *str);

/**
 *  Allocate a buffer of sufficient length to hold the formatted string.
 *  @description This call will dynamically allocate a buffer up to the specified maximum size and will format the 
 *      supplied arguments into the buffer.  A trailing null will always be appended. The call returns
 *      the size of the allocated string excluding the null.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param buf Pointer to a location to hold the buffer pointer.
 *  @param maxSize Maximum size to allocate for the buffer including the trailing null.
 *  @param fmt Printf style format string
 *  @return Returns the number of characters in the string.
 *  @ingroup MprString
 */
extern int mprAllocSprintf(MprCtx ctx, char **buf, int maxSize, cchar *fmt, ...);

/**
 *  Allocate a buffer of sufficient length to hold the formatted string.
 *  @description This call will dynamically allocate a buffer up to the specified maximum size and will format 
 *      the supplied arguments into the buffer. A trailing null will always be appended. The call returns
 *      the size of the allocated string excluding the null.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param buf Pointer to a location to hold the buffer pointer.
 *  @param maxSize Maximum size to allocate for the buffer including the trailing null.
 *  @param fmt Printf style format string
 *  @param arg Varargs argument obtained from va_start.
 *  @return Returns the number of characters in the string.
 *  @ingroup MprString
 */
extern int mprAllocVsprintf(MprCtx ctx, char **buf, int maxSize, cchar *fmt, va_list arg);

/**
 *  Allocate a new block to hold catenated strings.
 *  @description Allocate a new memory block of the required size and catenate
 *      the supplied strings delimited by the supplied delimiter. A variable
 *      number of strings may be supplied. The last argument must be a null.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param dest Pointer to a pointer that will hold the address of the allocated block.
 *  @param max Maximum size of the new block.
 *  @param delim Delimiter string to insert between the input strings
 *  @param src First string to catenate. There may be a variable number of of strings supplied.
 *  @return Returns the number of characters in the allocated block.
 *  @ingroup MprString
 */
extern int mprAllocStrcat(MprCtx ctx, char **dest, int max, cchar *delim, cchar *src, ...);

/**
 *  Copy a string into a newly allocated block.
 *  @description Allocate a new memory block of the required size and copy
 *      a string into it. The call returns the size of the allocated 
 *      block. This is similar to mprStrdup, but it will enforce a maximum
 *      size for the copied string and will ensure it is terminated with a null.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param dest Pointer to a pointer that will hold the address of the allocated block.
 *  @param max Maximum size of the new block.
 *  @param src Block to copy
 *  @return Returns the number of characters in the allocated block.
 *  @ingroup MprString
 */
extern int mprAllocStrcpy(MprCtx ctx, char **dest, int max, cchar *src);

/**
 *  Append strings to an existing string and reallocate as required.
 *  @description Append a list of strings to an existing string. The list of strings is terminated by a 
 *      null argument. The call returns the size of the allocated block. 
 *  @param ctx Any memory context allocated by the MPR.
 *  @param dest Pointer to a pointer that will hold the address of the allocated block.
 *  @param max Maximum size of the new block.
 *  @param existingLen Length of any existing string.
 *  @param delim String of delimiter characters to insert between appended strings.
 *  @param src Variable list of strings to append. The final string argument must be null.
 *  @return Returns the number of characters in the resulting string.
 *  @ingroup MprString
 */
extern int mprReallocStrcat(MprCtx ctx, char **dest, int max, int existingLen, cchar *delim, cchar *src, ...);

/**
 *  Buffer refill callback function
 *  @description Function to call when the buffer is depleted and needs more data.
 *  @param buf Instance of an MprBuf
 *  @param arg Data argument supplied to #mprSetBufRefillProc
 *  @returns The callback should return 0 if successful, otherwise a negative error code.
 *  @ingroup MprBuf
 */
typedef int (*MprBufProc)(struct MprBuf* bp, void *arg);

/**
 *  Dynamic Buffer Module
 *  @description MprBuf is a flexible, dynamic growable buffer structure. It has start and end pointers to the
 *      data buffer which act as read/write pointers. Routines are provided to get and put data into and out of the
 *      buffer and automatically advance the appropriate start/end pointer. By definition, the buffer is empty when
 *      the start pointer == the end pointer. Buffers can be created with a fixed size or can grow dynamically as 
 *      more data is added to the buffer. 
 *  \n\n
 *  For performance, the specification of MprBuf is deliberately exposed. All members of MprBuf are implicitly public.
 *  However, it is still recommended that wherever possible, you use the accessor routines provided.
 *  @stability Evolving.
 *  @see MprBuf, mprCreateBuf, mprSetBufMax, mprStealBuf, mprAdjustBufStart, mprAdjustBufEnd, mprCopyBufDown,
 *      mprFlushBuf, mprGetCharFromBuf, mprGetBlockFromBuf, mprGetBufLength, mprGetBufOrigin, mprGetBufSize,
 *      mprGetBufEnd, mprGetBufSpace, mprGetGrowBuf, mprGrowBuf, mprInsertCharToBuf,
 *      mprLookAtNextCharInBuf, mprLookAtLastCharInBuf, mprPutCharToBuf, mprPutBlockToBuf, mprPutIntToBuf,
 *      mprPutStringToBuf, mprPutFmtToBuf, mprRefillBuf, mprResetBufIfEmpty, mprSetBufSize, mprGetBufRefillProc,
 *      mprSetBufRefillProc, mprFree, MprBufProc
 *  @defgroup MprBuf MprBuf
 */
typedef struct MprBuf {
    uchar           *data;              /**< Actual buffer for data */
    uchar           *endbuf;            /**< Pointer one past the end of buffer */
    uchar           *start;             /**< Pointer to next data char */
    uchar           *end;               /**< Pointer one past the last data chr */
    int             buflen;             /**< Current size of buffer */
    int             maxsize;            /**< Max size the buffer can ever grow */
    int             growBy;             /**< Next growth increment to use */
    MprBufProc      refillProc;         /**< Auto-refill procedure */
    void            *refillArg;         /**< Refill arg */
} MprBuf;


/**
 *  Create a new buffer
 *  @description Create a new buffer. Use mprFree to free the buffer
 *  @param ctx Any memory context allocated by the MPR
 *  @param initialSize Initial size of the buffer
 *  @param maxSize Maximum size the buffer can grow to
 *  @return a new buffer
 *  @ingroup MprBuf
 */
extern MprBuf *mprCreateBuf(MprCtx ctx, int initialSize, int maxSize);

/**
 *  Set the maximum buffer size
 *  @description Update the maximum buffer size set when the buffer was created
 *  @param buf Buffer created via mprCreateBuf
 *  @param maxSize New maximum size the buffer can grow to
 *  @ingroup MprBuf
 */
extern void mprSetBufMax(MprBuf *buf, int maxSize);

/**
 *  Steal the buffer memory from a buffer
 *  @description Steal ownership of the buffer memory from the buffer structure. All MPR memory is owned by a 
 *      memory context and the contents of the buffer is owned by the MprBuf object. Stealing the buffer content 
 *      memory is useful to preserve the buffer contents after the buffer is freed
 *  @param ctx Memory context to won the memory for the buffer
 *  @param buf Buffer created via mprCreateBuf
 *  @return pointer to the buffer contents. Use mprGetBufLength before calling mprStealBuf to determine the resulting
 *      size of the contents.
 *  @ingroup MprBuf
 */
extern char *mprStealBuf(MprCtx ctx, MprBuf *buf);

/**
 *  Add a null character to the buffer contents.
 *  @description Add a null byte but do not change the buffer content lengths. The null is added outside the
 *      "official" content length. This is useful when calling #mprGetBufStart and using the returned pointer 
 *      as a "C" string pointer.
 *  @param buf Buffer created via mprCreateBuf
 *  @ingroup MprBuf
 */
extern void mprAddNullToBuf(MprBuf *buf);

/**
 *  Adjust the buffer start position
 *  @description Adjust the buffer start position by the specified amount. This is typically used to advance the
 *      start position as content is consumed. Adjusting the start or end position will change the value returned
 *      by #mprGetBufLength. If using the mprGetBlock or mprGetChar routines, adjusting the start position is
 *      done automatically.
 *  @param buf Buffer created via mprCreateBuf
 *  @param count Positive or negative count of bytes to adjust the start position.
 *  @ingroup MprBuf
 */
extern void mprAdjustBufStart(MprBuf *buf, int count);

/**
 *  Adjust the buffer end position
 *  @description Adjust the buffer start end position by the specified amount. This is typically used to advance the
 *      end position as content is appended to the buffer. Adjusting the start or end position will change the value 
 *      returned by #mprGetBufLength. If using the mprPutBlock or mprPutChar routines, adjusting the end position is
 *      done automatically.
 *  @param buf Buffer created via mprCreateBuf
 *  @param count Positive or negative count of bytes to adjust the start position.
 *  @ingroup MprBuf
 */
extern void mprAdjustBufEnd(MprBuf *buf, int count);

/**
 *  Compact the buffer contents
 *  @description Compact the buffer contents by copying the contents down to start the the buffer origin.
 *  @param buf Buffer created via mprCreateBuf
 *  @ingroup MprBuf
 */
extern void mprCompactBuf(MprBuf *buf);

/**
 *  Flush the buffer contents
 *  @description Discard the buffer contents and reset the start end content pointers.
 *  @param buf Buffer created via mprCreateBuf
 *  @ingroup MprBuf
 */
extern void mprFlushBuf(MprBuf *buf);

/**
 *  Get a character from the buffer
 *  @description Get the next byte from the buffer start and advance the start position.
 *  @param buf Buffer created via mprCreateBuf
 *  @return The character or -1 if the buffer is empty.
 *  @ingroup MprBuf
 */
extern int mprGetCharFromBuf(MprBuf *buf);

/**
 *  Get a block of data from the buffer
 *  @description Get a block of data from the buffer start and advance the start position. If the requested
 *      length is greater than the available buffer content, then return whatever data is available.
 *  @param buf Buffer created via mprCreateBuf
 *  @param blk Destination block for the read data. 
 *  @param count Count of bytes to read from the buffer.
 *  @return The count of bytes rread into the block or -1 if the buffer is empty.
 *  @ingroup MprBuf
 */
extern int mprGetBlockFromBuf(MprBuf *buf, uchar *blk, int count);

/**
 *  Get the buffer content length.
 *  @description Get the length of the buffer contents. This is not the same as the buffer size which may be larger.
 *  @param buf Buffer created via mprCreateBuf
 *  @returns The length of the content stored in the buffer.
 *  @ingroup MprBuf
 */
extern int mprGetBufLength(MprBuf *buf);

/**
 *  Get the origin of the buffer content storage.
 *  @description Get a pointer to the start of the buffer content storage. This may not be equal to the start of
 *      the buffer content if #mprAdjustBufStart has been called. Use #mprGetBufSize to determine the length
 *      of the buffer content storage array. 
 *  @param buf Buffer created via mprCreateBuf
 *  @returns A pointer to the buffer content storage.
 *  @ingroup MprBuf
 */
extern char *mprGetBufOrigin(MprBuf *buf);

/**
 *  Get the current size of the buffer content storage.
 *  @description This returns the size of the memory block allocated for storing the buffer contents.
 *  @param buf Buffer created via mprCreateBuf
 *  @returns The size of the buffer content storage.
 *  @ingroup MprBuf
 */
extern int mprGetBufSize(MprBuf *buf);

/**
 *  Get the space available to store content
 *  @description Get the number of bytes available to store content in the buffer
 *  @param buf Buffer created via mprCreateBuf
 *  @returns The number of bytes available
 *  @ingroup MprBuf
 */
extern int mprGetBufSpace(MprBuf *buf);

/**
 *  Get the start of the buffer contents
 *  @description Get a pointer to the start of the buffer contents. Use #mprGetBufLength to determine the length
 *      of the content. Use #mprGetBufEnd to get a pointer to the location after the end of the content.
 *  @param buf Buffer created via mprCreateBuf
 *  @returns Pointer to the start of the buffer data contents
 *  @ingroup MprBuf
 */
extern char *mprGetBufStart(MprBuf *buf);

/**
 *  Get a reference to the end of the buffer contents
 *  @description Get a pointer to the location immediately after the end of the buffer contents.
 *  @param buf Buffer created via mprCreateBuf
 *  @returns Pointer to the end of the buffer data contents. Points to the location one after the last data byte.
 *  @ingroup MprBuf
 */
extern char *mprGetBufEnd(MprBuf *buf);

/**
 *  Grow the buffer
 *  @description Grow the storage allocated for content for the buffer. The new size must be less than the maximum
 *      limit specified via #mprCreateBuf or #mprSetBufSize.
 *  @param buf Buffer created via mprCreateBuf
 *  @param count Count of bytes by which to grow the buffer content size. 
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprGrowBuf(MprBuf *buf, int count);

/**
 *  Insert a character into the buffer
 *  @description Insert a character into to the buffer prior to the current buffer start point.
 *  @param buf Buffer created via mprCreateBuf
 *  @param c Character to append.
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprInsertCharToBuf(MprBuf *buf, int c);

/**
 *  Peek at the next character in the buffer
 *  @description Non-destructively return the next character from the start position in the buffer. 
 *      The character is returned and the start position is not altered.
 *  @param buf Buffer created via mprCreateBuf
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprLookAtNextCharInBuf(MprBuf *buf);

/**
 *  Peek at the last character in the buffer
 *  @description Non-destructively return the last character from just prior to the end position in the buffer. 
 *      The character is returned and the end position is not altered.
 *  @param buf Buffer created via mprCreateBuf
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprLookAtLastCharInBuf(MprBuf *buf);

/**
 *  Put a character to the buffer.
 *  @description Append a character to the buffer at the end position and increment the end pointer.
 *  @param buf Buffer created via mprCreateBuf
 *  @param c Character to append
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprPutCharToBuf(MprBuf *buf, int c);

/**
 *  Put a block to the buffer.
 *  @description Append a block of data  to the buffer at the end position and increment the end pointer.
 *  @param buf Buffer created via mprCreateBuf
 *  @param ptr Block to append
 *  @param size Size of block to append
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprPutBlockToBuf(MprBuf *buf, cchar *ptr, int size);

/**
 *  Put an integer to the buffer.
 *  @description Append a integer to the buffer at the end position and increment the end pointer.
 *  @param buf Buffer created via mprCreateBuf
 *  @param i Integer to append to the buffer
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprPutIntToBuf(MprBuf *buf, int i);

/**
 *  Put a string to the buffer.
 *  @description Append a null terminated string to the buffer at the end position and increment the end pointer.
 *  @param buf Buffer created via mprCreateBuf
 *  @param str String to append
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprPutStringToBuf(MprBuf *buf, cchar *str);

/**
 *  Put a formatted string to the buffer.
 *  @description Format a string and Append to the buffer at the end position and increment the end pointer.
 *  @param buf Buffer created via mprCreateBuf
 *  @param fmt Printf style format string
 *  @param ... Variable arguments for the format string
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprPutFmtToBuf(MprBuf *buf, cchar *fmt, ...);

/**
 *  Refill the buffer with data
 *  @description Refill the buffer by calling the refill procedure specified via #mprSetBufRefillProc
 *  @param buf Buffer created via mprCreateBuf
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprRefillBuf(MprBuf *buf);

/**
 *  Reset the buffer
 *  @description If the buffer is empty, reset the buffer start and end pointers to the beginning of the buffer.
 *  @param buf Buffer created via mprCreateBuf
 *  @ingroup MprBuf
 */
extern void mprResetBufIfEmpty(MprBuf *buf);

/**
 *  Set the buffer size
 *  @description Set the current buffer content size and maximum size limit. Setting a current size will
 *      immediately grow the buffer to be this size. If the size is less than the current buffer size, 
 *      the requested size will be ignored. ie. this call will not shrink the buffer. Setting a maxSize 
 *      will define a maximum limit for how big the buffer contents can grow. Set either argument to 
 *      -1 to be ignored.
 *  @param buf Buffer created via mprCreateBuf
 *  @param size Size to immediately make the buffer. If size is less than the current buffer size, it will be ignored.
 *      Set to -1 to ignore this parameter.
 *  @param maxSize Maximum size the buffer contents can grow to.
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprSetBufSize(MprBuf *buf, int size, int maxSize);

/**
 *  Get the buffer refill procedure
 *  @description Return the buffer refill callback function.
 *  @param buf Buffer created via mprCreateBuf
 *  @returns The refill call back function if defined.
 *  @ingroup MprBuf
 */
extern MprBufProc mprGetBufRefillProc(MprBuf *buf);

/**
 *  Set the buffer refill procedure
 *  @description Define a buffer refill procedure. The MprBuf module will not invoke or manage this refill procedure.
 *      It is simply stored to allow upper layers to use and provide their own auto-refill mechanism.
 *  @param buf Buffer created via mprCreateBuf
 *  @param fn Callback function to store.
 *  @param arg Callback data argument.
 *  @ingroup MprBuf
 */
extern void mprSetBufRefillProc(MprBuf *buf, MprBufProc fn, void *arg);

/**
 *  Date and Time Service
 *  @stability Evolving
 *  @see MprTime, mprCtime, mprAsctime, mprGetTime, mprLocaltime, mprRfcTime
 *  @defgroup MprDate MprDate
 */
typedef struct MprDate { int dummy; } MprDate;

/**
 *  Mpr time structure.
 *  @description MprTime is the cross platform time abstraction structure. Time is stored as milliseconds
 *      since the epoch: 00:00:00 UTC Jan 1 1970. MprTime is typically a 64 bit quantity.
 *  @ingroup MprDate
 */
typedef int64 MprTime;
struct tm;

extern int mprCreateTimeService(MprCtx ctx);

/**
 *  Get the system time.
 *  @description Get the system time in milliseconds.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @return Returns the time in milliseconds since boot.
 *  @ingroup MprDate
 */
extern MprTime  mprGetTime(MprCtx ctx);

extern MprTime  mprGetTimeRemaining(MprCtx ctx, MprTime mark, uint timeout);
extern MprTime  mprGetElapsedTime(MprCtx ctx, MprTime mark);
extern int      mprCompareTime(MprTime t1, MprTime t2);

extern MprTime  mprMakeLocalTime(MprCtx ctx, struct tm *timep);
extern int      mprParseTime(MprCtx ctx, MprTime *time, cchar *str);


/**
 *  Format time as a string
 *  @description Safe replacement for asctime. This call formats the time value supplied via \a timeptr into the 
 *      supplied buffer. The supplied buffer size will be observed and the formatted time string will always
 *      have a terminating null.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param timeptr Time to format
 *  @param buf Buffer to hold the formatted string
 *  @param bufsize Maximum length for the string
 *  @return Returns the length of the string or MPR_ERR_WONT_FIT if the length is greater than \a max.
 *  @ingroup MprDate
 */
extern int mprAsctime(MprCtx ctx, char *buf, int bufsize, const struct tm *timeptr);

/**
 *  Convert time to local time and format as a string.
 *  @description Safe replacement for ctime. This call formats the time value supplied via \a timer into 
 *      the supplied buffer. The supplied buffer size will be observed and the formatted time string will always
 *      have a terminating null.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param buf Buffer to hold the formatted string
 *  @param bufsize Maximum length for the string
 *  @param time Time to format. Use mprGetTime to retrieve the current time.
 *  @return Returns the length of the string or MPR_ERR_WONT_FIT if the length is greater than \a max.
 *  @ingroup MprDate
 */
extern int mprCtime(MprCtx ctx, char *buf, int bufsize, MprTime time);

/**
 *  Convert time to local representation.
 *  @description Safe replacement for localtime. This call converts the time value to local time and formats 
 *      the as a struct tm.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param timep Pointer to a tm structure to hold the result
 *  @param time Time to format
 *  @return Returns a pointer to the tmBuf.
 *  @ingroup MprDate
 */
extern struct tm *mprLocaltime(MprCtx ctx, struct tm *timep, MprTime time);

/**
 *  Convert time to UTC and parse into a time structure
 *  @description Safe replacement for gmtime. This call converts the supplied time value
 *      to UTC time and parses the result into a tm structure.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param timep Pointer to a tm structure to hold the result.
 *  @param time The time to format
 *  @return Returns the tm structure reference
 *  @ingroup MprDate
 */
extern struct tm *mprGmtime(MprCtx ctx, struct tm *timep, MprTime time);

/**
 *  Format time according to RFC822.
 *  @description Thread-safe formatting of time dates according to RFC822.
 *      For example: "Fri, 07 Jan 2003 12:12:21 GMT"
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param buf Time to format
 *  @param bufsize Size of \a buf.
 *  @param timep Input pointer to a tm structure holding the time to format.
 *  @return Returns zero on success. Returns MPR_ERR_WONT_FIT if the bufsize is too small.
 */
extern int      mprRfctime(MprCtx ctx, char *buf, int bufsize, const struct tm *timep);

extern int      mprStrftime(MprCtx ctx, char *buf, int bufsize, cchar *fmt, const struct tm *timep);

/**
 *  List Module.
 *  @description The MprList is a dynamic growable list suitable for storing pointers to arbitrary objects.
 *  @stability Evolving.
 *  @see MprList, mprAddItem, mprGetItem, mprCreateList, mprClearList, mprLookupItem, mprFree, 
 *      mprGetFirstItem, mprGetListCapacity, mprGetListCount, mprGetNextItem, mprGetPrevItem, 
 *      mprRemoveItem, mprRemoveItemByIndex, mprRemoveRangeOfItems, mprAppendList, mprSortList, 
 *      mprDupList, MprListCompareProc, mprFree, mprCreateKeyPair
 *  @defgroup MprList MprList
 */
typedef struct MprList {
    void    **items;                    /**< List item data */
    int     length;                     /**< Current length of the list contents */
    int     capacity;                   /**< Current list size */ 
    int     maxSize;                    /**< Maximum capacity */
} MprList;

/**
 *  List comparison procedure for sorting
 *  @description Callback function signature used by #mprSortList
 *  @param arg1 First list item to compare
 *  @param arg2 Second list item to compare
 *  @returns Return zero if the items are equal. Return -1 if the first arg is less than the second. Otherwise return 1.
 *  @ingroup MprList
 */
typedef int (*MprListCompareProc)(cvoid *arg1, cvoid *arg2);


/**
 *  Add an item to a list
 *  @description Add the specified item to the list. The list must have been previously created via 
 *      mprCreateList. The list will grow as required to store the item
 *  @param list List pointer returned from #mprCreateList
 *  @param item Pointer to item to store
 *  @return Returns a positive integer list index for the inserted item. If the item cannot be inserted due 
 *      to a memory allocation failure, -1 is returned
 *  @ingroup MprList
 */
extern int mprAddItem(MprList *list, cvoid *item);


/**
 *  Append a list
 *  @description Append the contents of one list to another. The list will grow as required to store the item
 *  @param list List pointer returned from #mprCreateList
 *  @param add List whose contents are added
 *  @return Returns a pointer to the original list if successful. Returns NULL on memory allocation errors.
 *  @ingroup MprList
 */
extern MprList *mprAppendList(MprList *list, MprList *add);


/**
 *  Create a list.
 *  @description Creates an empty list. MprList's can store generic pointers. They automatically grow as 
 *      required when items are added to the list. Callers should invoke mprFree when finished with the
 *      list to release allocated storage.
 *  @param ctx Any memory context allocated by the MPR.
 *  @return Returns a pointer to the list. 
 *  @ingroup MprList
 */
extern MprList *mprCreateList(MprCtx ctx);

/**
 *  Copy a list
 *  @description Copy the contents of a list into an existing list. The destination list is cleared first and 
 *      has its dimensions set to that of the source clist.
 *  @param dest Destination list for the copy
 *  @param src Source list
 *  @return Returns zero if successful, otherwise a negative MPR error code.
 *  @ingroup MprList
 */
extern int mprCopyList(MprList *dest, MprList *src);

/**
 *  Duplicate a list
 *  @description Copy the contents of a list into a new list. 
 *  @param ctx Memory context from which to allocate the list. See #mprAlloc
 *  @param src Source list to copy
 *  @return Returns a new list reference
 *  @ingroup MprList
 */
extern MprList *mprDupList(MprCtx ctx, MprList *src);

/**
 *  Clears the list of all items.
 *  @description Resets the list length to zero and clears all items. Existing items are not freed, they 
 *      are only removed from the list.
 *  @param list List pointer returned from mprCreateList.
 *  @ingroup MprList
 */
extern void mprClearList(MprList *list);

/**
 *  Find an item and return its index.
 *  @description Search for an item in the list and return its index.
 *  @param list List pointer returned from mprCreateList.
 *  @param item Pointer to value stored in the list.
 *  @ingroup MprList
 */
extern int mprLookupItem(MprList *list, cvoid *item);

/**
 *  Get the first item in the list.
 *  @description Returns the value of the first item in the list. After calling this routine, the remaining 
 *      list items can be walked using mprGetNextItem.
 *  @param list List pointer returned from mprCreateList.
 *  @ingroup MprList
 */
extern void *mprGetFirstItem(MprList *list);

/**
 *  Get the last item in the list.
 *  @description Returns the value of the last item in the list. After calling this routine, the remaining 
 *      list items can be walked using mprGetPrevItem.
 *  @param list List pointer returned from mprCreateList.
 *  @ingroup MprList
 */
extern void *mprGetLastItem(MprList *list);

/**
 *  Get an list item.
 *  @description Get an list item specified by its index.
 *  @param list List pointer returned from mprCreateList.
 *  @param index Item index into the list. Indexes have a range from zero to the lenghth of the list - 1.
 *  @ingroup MprList
 */
extern void *mprGetItem(MprList *list, int index);

/**
 *  Get the current capacity of the list.
 *  @description Returns the capacity of the list. This will always be equal to or greater than the list length.
 *  @param list List pointer returned from mprCreateList.
 *  @ingroup MprList
 */
extern int mprGetListCapacity(MprList *list);

/**
 *  Get the number of items in the list.
 *  @description Returns the number of items in the list. This will always be less than or equal to the list capacity.
 *  @param list List pointer returned from mprCreateList.
 *  @ingroup MprList
 */
extern int mprGetListCount(MprList *list);

/**
 *  Get the next item in the list.
 *  @description Returns the value of the next item in the list. Before calling
 *      this routine, mprGetFirstItem must be called to initialize the traversal of the list.
 *  @param list List pointer returned from mprCreateList.
 *  @param lastIndex Pointer to an integer that will hold the last index retrieved.
 *  @ingroup MprList
 */
extern void *mprGetNextItem(MprList *list, int *lastIndex);

/**
 *  Get the previous item in the list.
 *  @description Returns the value of the previous item in the list. Before 
 *      calling this routine, mprGetFirstItem and/or mprGetNextItem must be
 *      called to initialize the traversal of the list.
 *  @param list List pointer returned from mprCreateList.
 *  @param lastIndex Pointer to an integer that will hold the last index retrieved.
 *  @ingroup MprList
 */
extern void *mprGetPrevItem(MprList *list, int *lastIndex);

/**
 *  Initialize a list structure
 *  @description If a list is statically declared inside another structure, mprInitList can be used to 
 *      initialize it before use.
 *  @param list Reference to the MprList struct.
 *  @ingroup MprList
 */
extern void mprInitList(MprList *list);

/**
 *  Insert an item into a list at a specific position
 *  @description Insert the item into the list before the specified position. The list will grow as required 
 *      to store the item
 *  @param list List pointer returned from #mprCreateList
 *  @param index Location at which to store the item. The previous item at this index is moved up to make room.
 *  @param item Pointer to item to store
 *  @return Returns the position index (positive integer) if successful. If the item cannot be inserted due 
 *      to a memory allocation failure, -1 is returned
 *  @ingroup MprList
 */
extern int mprInsertItemAtPos(MprList *list, int index, cvoid *item);

/**
 *  Remove an item from the list
 *  @description Search for a specified item and then remove it from the list.
 *      Existing items are not freed, they are only removed from the list.
 *  @param list List pointer returned from mprCreateList.
 *  @param item Item pointer to remove. 
 *  @return Returns zero if successful, otherwise a negative MPR error code.
 *  @ingroup MprList
 */
extern int mprRemoveItem(MprList *list, void *item);

/**
 *  Remove an item from the list
 *  @description Removes the element specified by \a index, from the list. The
 *      list index is provided by mprInsertItem.
 *  @return Returns zero if successful, otherwise a negative MPR error code.
 *  @ingroup MprList
 */
extern int mprRemoveItemAtPos(MprList *list, int index);

/**
 *  Remove the last item from the list
 *  @description Remove the item at the highest index position.
 *      Existing items are not freed, they are only removed from the list.
 *  @param list List pointer returned from mprCreateList.
 *  @return Returns zero if successful, otherwise a negative MPR error code.
 *  @ingroup MprList
 */
extern int mprRemoveLastItem(MprList *list);

/**
 *  Remove a range of items from the list.
 *  @description Remove a range of items from the list. The range is specified
 *      from the \a start index up to and including the \a end index.
 *  @param list List pointer returned from mprCreateList.
 *  @param start Starting item index to remove (inclusive)
 *  @param end Ending item index to remove (inclusive)
 *  @return Returns zero if successful, otherwise a negative MPR error code.
 *  @ingroup MprList
 */
extern int mprRemoveRangeOfItems(MprList *list, int start, int end);

/**
 *  Set a list item
 *  @description Update the list item stored at the specified index
 *  @param list List pointer returned from mprCreateList.
 *  @param index Location to update
 *  @param item Pointer to item to store
 *  @return Returns the old item previously at that location index
 *  @ingroup MprList
 */
extern void *mprSetItem(MprList *list, int index, cvoid *item);

/**
 *  Define the list size limits
 *  @description Define the list initial size and maximum size it can grow to.
 *  @param list List pointer returned from mprCreateList.
 *  @param initialSize Initial size for the list. This call will allocate space for at least this number of items.
 *  @param maxSize Set the maximum limit the list can grow to become.
 *  @return Returns zero if successful, otherwise a negative MPR error code.
 *  @ingroup MprList
 */
extern int mprSetListLimits(MprList *list, int initialSize, int maxSize);

/**
 *  Sort a list
 *  @description Sort a list using the sort ordering dictated by the supplied compare function.
 *  @param list List pointer returned from mprCreateList.
 *  @param compare Comparison function. If null, then a default string comparison is used.
 *  @ingroup MprList
 */
extern void mprSortList(MprList *list, MprListCompareProc compare);

/**
 *  Key value pairs for use with MprList or MprHash
 *  @ingroup MprList
 */
typedef struct MprKeyValue {
    char        *key;               /**< Key string */
    char        *value;             /**< Associated value for the key */
} MprKeyValue;


/**
 *  Create a key / value pair
 *  @description Allocate and initialize a key value pair for use by the MprList or MprHash modules.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param key Key string
 *  @param value Key value string
 *  @returns An initialized MprKeyValue
 *  @ingroup MprList
 */
extern MprKeyValue *mprCreateKeyPair(MprCtx ctx, cchar *key, cchar *value);

/**
 *  Logging Services
 *  @stability Evolving
 *  @defgroup MprLog MprLog
 *  @see mprError, mprLog, mprSetLogHandler, mprSetLogLevel, mprUserError, mprRawLog, mprFatalError, MprLogHandler
 *      mprGetLogHandler, mprMemoryError, mprStaticAssert, mprStaticError
 */
typedef struct MprLog { int dummy; } MprLog;

/**
 *  Log handler callback type.
 *  @description Callback prototype for the log handler. Used by mprSetLogHandler to define 
 *      a message logging handler to process log and error messages. 
 *  @param file Source filename. Derived by using __FILE__.
 *  @param line Source line number. Derived by using __LINE__.
 *  @param flags Error flags.
 *  @param level Message logging level. Levels are 0-9 with zero being the most verbose.
 *  @param msg Message being logged.
 *  @ingroup MprLog
 */
typedef void (*MprLogHandler)(MprCtx ctx, int flags, int level, cchar *msg);

/**
 *  Set an MPR debug log handler.
 *  @description Defines a callback handler for MPR debug and error log messages. When output is sent to 
 *      the debug channel, the log handler will be invoked to accept the output message.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param handler Callback handler
 *  @param handlerData Callback handler data
 *  @ingroup MprLog
 */
extern void mprSetLogHandler(MprCtx ctx, MprLogHandler handler, void *handlerData);

/**
 *  Get the current MPR debug log handler.
 *  @description Get the log handler defined via #mprSetLogHandler
 *  @param ctx Any memory context allocated by the MPR.
 *  @returns A function of the signature #MprLogHandler
 *  @ingroup MprLog
 */
extern MprLogHandler mprGetLogHandler(MprCtx ctx);

/**
 *  Log an error message.
 *  @description Send an error message to the MPR debug logging subsystem. The 
 *      message will be to the log handler defined by #mprSetLogHandler. It 
 *      is up to the log handler to respond appropriately and log the message.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param fmt Printf style format string. Variable number of arguments to 
 *  @param ... Variable number of arguments for printf data
 *  @ingroup MprLog
 */
extern void mprError(MprCtx ctx, cchar *fmt, ...);

/**
 *  Log a fatal error message and exit.
 *  @description Send a fatal error message to the MPR debug logging subsystem and then exit the application by
 *      calling exit(). The message will be to the log handler defined by #mprSetLogHandler. It 
 *      is up to the log handler to respond appropriately and log the message.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param fmt Printf style format string. Variable number of arguments to 
 *  @param ... Variable number of arguments for printf data
 *  @ingroup MprLog
 */
extern void mprFatalError(MprCtx ctx, cchar *fmt, ...);

/**
 *  Log a memory error message.
 *  @description Send a memory error message to the MPR debug logging subsystem. The message will be 
 *      passed to the log handler defined by #mprSetLogHandler. It is up to the log handler to respond appropriately
 *      to the fatal message, the MPR takes no other action other than logging the message. Typically, a memory 
 *      message will be logged and the application will be shutdown. The preferred method of operation is to define
 *      a memory depletion callback via #mprCreate. This will be invoked whenever a memory allocation error occurs.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param fmt Printf style format string. Variable number of arguments to 
 *  @param ... Variable number of arguments for printf data
 *  @ingroup MprLog
 */
extern void mprMemoryError(MprCtx ctx, cchar *fmt, ...);

/**
 *  Write a message to the diagnostic log file.
 *  @description Send a message to the MPR logging subsystem.
 *  @param level Logging level for this message. The level is 0-9 with zero being the most verbose.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param fmt Printf style format string. Variable number of arguments to 
 *  @param ... Variable number of arguments for printf data
 *  @remarks mprLog is highly useful as a debugging aid when integrating or when developing new modules. 
 *  @ingroup MprLog
 */
extern void mprLog(MprCtx ctx, int level, cchar *fmt, ...);

/**
 *  Write a raw log message to the diagnostic log file.
 *  @description Send a raw message to the MPR logging subsystem. Raw messages do not have any application prefix
 *      attached to the message and do not append a newline to the message.
 *  @param level Logging level for this message. The level is 0-9 with zero being the most verbose.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param fmt Printf style format string. Variable number of arguments to 
 *  @param ... Variable number of arguments for printf data
 *  @remarks mprLog is highly useful as a debugging aid when integrating or when developing new modules. 
 *  @ingroup MprLog
 */
extern void mprRawLog(MprCtx ctx, int level, cchar *fmt, ...);

/**
 *  Output an assertion failed message.
 *  @description This will emit an assertion failed message to the standard error output. It will bypass the logging
 *      system.
 *  @param loc Source code location string. Use MPR_LOC to define a file name and line number string suitable for this
 *      parameter.
 *  @param msg Simple string message to output
 *  @ingroup MprLog
 */
extern void mprStaticAssert(cchar *loc, cchar *msg);

/**
 *  Write a message to the diagnostic log file without allocating any memory. Useful for log messages from within the
 *      memory allocator.
 *  @description Send a message to the MPR logging subsystem. This will not allocate any memory while formatting the 
 *      message. The formatted message string will be truncated in size to #MPR_MAX_STRING bytes. 
 *  @param ctx Any memory context allocated by the MPR.
 *  @param fmt Printf style format string. Variable number of arguments to 
 *  @param ... Variable number of arguments for printf data
 *  @ingroup MprLog
 */
extern void mprStaticError(MprCtx ctx, cchar *fmt, ...);


/**
 *  Display an error message to the user.
 *  @description Display an error message to the user and then send it to the 
 *      MPR debug logging subsystem. The message will be passed to the log 
 *      handler defined by mprSetLogHandler. It is up to the log handler to 
 *      respond appropriately and display the message to the user.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param fmt Printf style format string. Variable number of arguments to 
 *  @param ... Variable number of arguments for printf data
 *  @ingroup MprLog
 */
extern void mprUserError(MprCtx ctx, cchar *fmt, ...);

/**
 *  Hash table entry structure.
 *  @description Each hash entry has a descriptor entry. This is used to manage the hash table link chains.
 *  @see MprHash, mprAddHash, mprAddDuplicateHash, mprCopyHash, mprCreateHash, mprGetFirstHash, mprGetNextHash,
 *      mprGethashCount, mprLookupHash, mprLookupHashEntry, mprRemoveHash, mprFree, mprCreateKeyPair
 *  @stability Evolving.
 *  @defgroup MprHash MprHash
 */
typedef struct MprHash {
    struct MprHash *next;               /**< Next symbol in hash chain */
    char            *key;               /**< Hash key */
    cvoid           *data;              /**< Pointer to symbol data */
    int             bucket;             /**< Hash bucket index */
} MprHash;


/**
 *  Hash table control structure
 */
typedef struct MprHashTable {
    MprHash         **buckets;          /**< Hash collision bucket table */
    int             hashSize;           /**< Size of the buckets array */
    int             count;              /**< Number of symbols in the table */
} MprHashTable;


/**
 *  Add a symbol value into the hash table
 *  @description Associate an arbitrary value with a string symbol key and insert into the symbol table.
 *  @param table Symbol table returned via mprCreateSymbolTable.
 *  @param key String key of the symbole entry to delete.
 *  @param ptr Arbitrary pointer to associate with the key in the table.
 *  @return Integer count of the number of entries.
 *  @ingroup MprHash
 */
extern MprHash *mprAddHash(MprHashTable *table, cchar *key, cvoid *ptr);

/**
 *  Add a duplicate symbol value into the hash table
 *  @description Add a symbol to the hash which may clash with an existing entry. Duplicate symbols can be added to
 *      the hash, but only one may be retrieved via #mprLookupHash. To recover duplicate entries walk the hash using
 *      #mprGetNextHash.
 *  @param table Symbol table returned via mprCreateSymbolTable.
 *  @param key String key of the symbole entry to delete.
 *  @param ptr Arbitrary pointer to associate with the key in the table.
 *  @return Integer count of the number of entries.
 *  @ingroup MprHash
 */
extern MprHash *mprAddDuplicateHash(MprHashTable *table, cchar *key, cvoid *ptr);

/**
 *  Copy a hash table
 *  @description Create a new hash table and copy all the entries from an existing table.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param table Symbol table returned via mprCreateSymbolTable.
 *  @return A new hash table initialized with the contents of the original hash table.
 *  @ingroup MprHash
 */
extern MprHashTable *mprCopyHash(MprCtx ctx, MprHashTable *table);

/**
 *  Create a hash table
 *  @description Creates a hash table that can store arbitrary objects associated with string key values.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param hashSize Size of the hash table for the symbol table. Should be a prime number.
 *  @return Returns a pointer to the allocated symbol table. Caller should use mprFree to dispose of the table 
 *      when complete.
 *  @ingroup MprHash
 */
extern MprHashTable *mprCreateHash(MprCtx ctx, int hashSize);

/**
 *  Return the first symbol in a symbol entry
 *  @description Prepares for walking the contents of a symbol table by returning the first entry in the symbol table.
 *  @param table Symbol table returned via mprCreateSymbolTable.
 *  @return Pointer to the first entry in the symbol table.
 *  @ingroup MprHash
 */
extern MprHash *mprGetFirstHash(MprHashTable *table);

/**
 *  Return the next symbol in a symbol entry
 *  @description Continues walking the contents of a symbol table by returning
 *      the next entry in the symbol table. A previous call to mprGetFirstSymbol
 *      or mprGetNextSymbol is required to supply the value of the \a last
 *      argument.
 *  @param table Symbol table returned via mprCreateSymbolTable.
 *  @param last Symbol table entry returned via mprGetFirstSymbol or mprGetNextSymbol.
 *  @return Pointer to the first entry in the symbol table.
 *  @ingroup MprHash
 */
extern MprHash *mprGetNextHash(MprHashTable *table, MprHash *last);

/**
 *  Return the count of symbols in a symbol entry
 *  @description Returns the number of symbols currently existing in a symbol table.
 *  @param table Symbol table returned via mprCreateSymbolTable.
 *  @return Integer count of the number of entries.
 *  @ingroup MprHash
 */
extern int mprGetHashCount(MprHashTable *table);

/**
 *  Lookup a symbol in the hash table.
 *  @description Lookup a symbol key and return the value associated with that key.
 *  @param table Symbol table returned via mprCreateSymbolTable.
 *  @param key String key of the symbole entry to delete.
 *  @return Value associated with the key when the entry was inserted via mprInsertSymbol.
 *  @ingroup MprHash
 */
extern cvoid *mprLookupHash(MprHashTable *table, cchar *key);

/**
 *  Lookup a symbol in the hash table and return the hash entry
 *  @description Lookup a symbol key and return the hash table descriptor associated with that key.
 *  @param table Symbol table returned via mprCreateSymbolTable.
 *  @param key String key of the symbole entry to delete.
 *  @return MprHash table structure for the entry
 *  @ingroup MprHash
 */
extern MprHash *mprLookupHashEntry(MprHashTable *table, cchar *key);

/**
 *  Remove a symbol entry from the hash table.
 *  @description Removes a symbol entry from the symbol table. The entry is looked up via the supplied \a key.
 *  @param table Symbol table returned via mprCreateSymbolTable.
 *  @param key String key of the symbole entry to delete.
 *  @return Returns zero if successful, otherwise a negative MPR error code is returned.
 *  @ingroup MprHash
 */
extern int mprRemoveHash(MprHashTable *table, cchar *key);



typedef bool            (*MprAccessFileProc)(struct MprFileService *fs, cchar *path, int omode);
typedef int             (*MprDeleteFileProc)(struct MprFileService *fs, cchar *path);
typedef int             (*MprDeleteDirProc)(struct MprFileService *fs, cchar *path);
typedef int             (*MprGetFileInfoProc)(struct MprFileService *fs, cchar *path, struct MprFileInfo *info);
typedef int             (*MprMakeDirProc)(struct MprFileService *fs, cchar *path, int perms);
typedef struct MprFile* (*MprOpenFileProc)(MprCtx ctx, struct MprFileService *fs, cchar *path, int omode, int perms);
typedef void            (*MprCloseFileProc)(struct MprFile *file);
typedef int             (*MprReadFileProc)(struct MprFile *file, void *buf, uint size);
typedef long            (*MprSeekFileProc)(struct MprFile *file, int seekType, long distance);
typedef int             (*MprSetBufferedProc)(struct MprFile *file, int initialSize, int maxSize);
typedef int             (*MprWriteFileProc)(struct MprFile *file, cvoid *buf, uint count);

/**
 *  File system service
 *  @description The MPR provides a file system abstraction to support non-disk based file access such as flash or 
 *      other ROM based file systems. The MprFileService structure defines a virtual file system interface that
 *      will be invoked by the various MPR file routines.
 */
typedef struct MprFileService {
    MprAccessFileProc   accessFile;     /**< Virtual access file routine */
    MprDeleteFileProc   deleteFile;     /**< Virtual delete file routine */
    MprDeleteDirProc    deleteDir;      /**< Virtual delete directory routine */
    MprGetFileInfoProc  getFileInfo;    /**< Virtual get file information routine */
    MprMakeDirProc      makeDir;        /**< Virtual make directory routine */
    MprOpenFileProc     openFile;       /**< Virtual open file routine */
    MprCloseFileProc    closeFile;      /**< Virtual close file routine */
    MprReadFileProc     readFile;       /**< Virtual read file routine */
    MprSeekFileProc     seekFile;       /**< Virtual seek file routine */
    MprSetBufferedProc  setBuffered;    /**< Virtual set buffered I/O routine */
    MprWriteFileProc    writeFile;      /**< Virtual write file routine */

    struct MprFile  *console;           /**< Standard output file */
    struct MprFile  *error;             /**< Standard error file */

} MprFileService;


#if BLD_FEATURE_ROMFS
/*
 *  A RomInode is created for each file in the Rom file system.
 */
typedef struct  MprRomInode {
    char            *path;              /* File path */
    uchar           *data;              /* Pointer to file data */
    int             size;               /* Size of file */
    int             num;                /* Inode number */
} MprRomInode;


typedef struct MprRomFileService {
    MprFileService  fileService;
    MprHashTable    *fileIndex;
    MprRomInode     *romInodes;
    char            *root;
    int             rootLen;
} MprRomFileService;
#elif BREW


typedef struct MprBrewFileService {
    MprFileService  fileService;
    IFileMgr        *fileMgr;           /* File manager */
} MprBrewFileService;
#else

typedef MprFileService MprDiskFileService;
#endif


/**
 *  File information structure
 *  @description MprFileInfo is the cross platform File information structure.
 *  @ingroup MprFile
 */
typedef struct MprFileInfo {
    int64           size;               /**< File length */
    MprTime         atime;              /**< Access time */
    MprTime         ctime;              /**< Create time */
    MprTime         mtime;              /**< Modified time */
    uint            inode;              /**< Inode number */
    bool            isDir;              /**< Set if directory */
    bool            isReg;              /**< Set if a regular file */
    bool            caseMatters;        /**< Case comparisons matter */
    int             perms;              /**< Permission mask */
    int             valid;              /**< Valid data bit */
} MprFileInfo;


/*
 *  TODO - need a printf routine for MprFile
 */
/**
 *  File Services Module
 *  @description MprFile is the cross platform File I/O abstraction control structure. An instance will be
 *       created when a file is created or opened via #mprOpen.
 *  @stability Evolving.
 *  @see MprFile mprClose mprGetFileInfo mprGets mprMakeDir mprOpen mprPuts mprRead mprDelete mprDeleteDir 
 *      mprSeek mprWrite mprFlush MprFile mprGetBaseName mprGetDirName mprGetExtension mprGetc
 *      mprGetParentDir mprMakeDir mprMakeDirPath mprMakeTempFileName mprGetRelFilename mprCleanFilename
 *      mprGetAbsFilename mprGetFileNewline mprGetFileDelimiter mprGetUnixFilename mprGetWinFilename 
 *      mprMapDelimiters MprFileInfo mprAccess mprCompareFilename mprCopyFile mprDisableFileBuffering
 *      mprEnableFileBuffering mprGetDirList
 *
 *  @defgroup MprFile MprFile
 */
typedef struct MprFile {
#if BLD_DEBUG
    cchar           *path;              /**< Filename */
#endif
    MprBuf          *buf;               /**< Buffer for I/O if buffered */
    MprOffset       pos;                /**< Current read position  */
    MprOffset       size;               /**< Current file size */
    int             mode;               /**< File open mode */
    int             perms;              /**< File permissions */
#if BLD_FEATURE_ROMFS
    MprRomInode     *inode;             /**< Reference to ROM file */
#else
#if BREW
    IFile           *fd;                /**< File handle */
#else
    int             fd;
#endif /* BREW */
#endif /* BLD_FEATURE_ROMFS */
    MprFileService  *fileService;       /**< File system implementaion */
} MprFile;


/**
 *  Directory entry description
 *  @description The MprGetDirList will create a list of directory entries.
 */
typedef struct MprDirEntry {
    char            *name;              /**< Name of the file */
    MprTime         lastModified;       /**< Time the file was last modified */
    MprOffset       size;               /**< Size of the file */
    bool            isDir;              /**< True if the file is a directory */
} MprDirEntry;


/*
 *  File system initialization routines
 */
extern MprFileService *mprCreateFileService(MprCtx ctx);
#if BLD_FEATURE_ROMFS
    extern MprRomFileService *mprCreateRomFileService(MprCtx ctx);
    extern int      mprSetRomFileSystem(MprCtx ctx, MprRomInode *inodeList);
#elif BREW
    extern MprBrewFileService *mprCreateBrewFileService(MprCtx ctx);
#else
    extern MprDiskFileService *mprCreateDiskFileService(MprCtx ctx);
#endif

/**
 *  Determine if a file can be accessed
 *  @description Test if a file can be accessed for a given mode
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename File name to test
 *  @param omode Posix style file open mode mask. See #mprOpen for the various modes.
 *  @returns True if the file exists and can be accessed
 *  @ingroup MprFile
 */
extern bool mprAccess(MprCtx ctx, cchar *filename, int omode);

/**
 *  Copy a file
 *  @description Create a new copy of a file with the specified open permissions mode.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param from Filename of the existing file to copy
 *  @param to Name of the new file copy
 *  @param omode Posix style file open mode mask. See #mprOpen for the various modes.
 *  @returns True if the file exists and can be accessed
 *  @ingroup MprFile
 */
extern int mprCopyFile(MprCtx ctx, cchar *from, cchar *to, int omode);

/**
 *  Delete a file.
 *  @description Delete a file.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename String containing the filename to delete.
 *  @return Returns zero if successful otherwise a negative MPR error code is returned.
 *  @ingroup MprFile
 */
extern int mprDelete(MprCtx ctx, cchar *filename);

/**
 *  Delete a directory.
 *  @description Delete a directory.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename String containing the directory name to delete.
 *  @return Returns zero if successful otherwise a negative MPR error code is returned.
 *  @ingroup MprFile
 */
extern int mprDeleteDir(MprCtx ctx, cchar *filename);

/**
 *  Disable file buffering
 *  @description Disable any buffering of data when using the buffer.
 *  @param file File instance returned from #mprOpen
 *  @ingroup MprFile
 */
extern void mprDisableFileBuffering(MprFile *file);

/**
 *  Enable file buffering
 *  @description Enable data buffering when using the buffer.
 *  @param file File instance returned from #mprOpen
 *  @param size Size to allocate for the buffer.
 *  @param maxSize Maximum size the data buffer can grow to
 *  @ingroup MprFile
 */
extern int mprEnableFileBuffering(MprFile *file, int size, int maxSize);

/**
 *  Get the base portion of a filename
 *  @description Get the base portion of a filename by stripping off all directory components
 *  @param filename File name to examine
 *  @returns A filename without any directory portion. The filename is a reference into the original file string and 
 *      should not be freed. 
 *  @ingroup MprFile
 */
extern cchar *mprGetBaseName(cchar *filename);

/**
 *  Get the directory portion of a filename
 *  @description Get the directory portion of a filename by stripping off the base name.
 *  @param buf Buffer to hold the directory name
 *  @param bufsize Size of buf
 *  @param filename File name to examine
 *  @returns A reference to the buffer passed in the buf parameter.
 *  @ingroup MprFile
 */
extern char *mprGetDirName(char *buf, int bufsize, cchar *filename);

/**
 *  Get the file extension portion of a filename
 *  @description Get the file extension portion of a filename. The file extension is the portion after the last "."
 *      in the filename.
 *  @param filename File name to examine
 *  @returns A filename extension. The extension is a reference into the original file string and should not be freed.
 *  @ingroup MprFile
 */
extern cchar *mprGetExtension(cchar *filename);

/**
 *  Create a directory list of files.
 *  @description Get the list of files in a directory and return a list.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename Directory to list.
 *  @param enumDirs Set to true to enumerate directory entries as well as regular filenames. 
 *  @returns A list (MprList) of directory filenames. Each filename is a regular string owned by the list object.
 *      Use #mprFree to free the memory for the list and directory filenames.
 *  @ingroup MprFile
 */
extern MprList *mprGetDirList(MprCtx ctx, cchar *filename, bool enumDirs);

/**
 *  Return information about a file.
 *  @description Returns file status information regarding the \a filename.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename String containing the filename to query.
 *  @param info Pointer to a pre-allocated MprFileInfo structure.
 *  @return Returns zero if successful, otherwise a negative MPR error code is returned.
 *  @ingroup MprFile
 */
extern int mprGetFileInfo(MprCtx ctx, cchar *filename, MprFileInfo *info);

/**
 *  Return the current file position
 *  @description Return the current read/write file position.
 *  @param file A file object returned from #mprOpen
 *  @returns The current file offset position if successful. Returns a negative MPR error code on errors.
 *  @ingroup MprFile
 */
extern MprOffset mprGetFilePosition(MprFile *file);

/**
 *  Get the size of the file
 *  @description Return the current file size
 *  @param file A file object returned from #mprOpen
 *  @returns The current file size if successful. Returns a negative MPR error code on errors.
 *  @ingroup MprFile
 */
extern MprOffset mprGetFileSize(MprFile *file);

/**
 *  Read a line from the file.
 *  @description Read a single line from the file and advance the read position. Lines are delimited by the 
 *      newline character. The newline is not included in the returned buffer.
 *  @param file Pointer to an MprFile object returned via MprOpen.
 *  @param buf Pre-allocated buffer to contain the line of data.
 *  @param size Size of \a buf.
 *  @return The number of characters read into \a buf.
 *  @ingroup MprFile
 */
extern char *mprGets(MprFile *file, char *buf, uint size);

/**
 *  Read a character from the file.
 *  @description Read a single character from the file and advance the read position.
 *  @param file Pointer to an MprFile object returned via MprOpen.
 *  @return If successful, return the character just read. Otherwise return a negative MPR error code.
 *      End of file is signified by reading 0.
 *  @ingroup MprFile
 */
extern int mprGetc(MprFile *file);

/**
 *  Make a directory
 *  @description Make a directory using the supplied path. Intermediate directories are created as required.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param path String containing the directory pathname to create.
 *  @param perms Posix style file permissions mask.
 *  @return Returns zero if successful, otherwise a negative MPR error code is returned.
 *  @ingroup MprFile
 */
extern int mprMakeDir(MprCtx ctx, cchar *path, int perms);

/**
 *  Make a directory and all required intervening directories.
 *  @description Make a directory with all the necessary intervening directories.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param path Directory path name.
 *  @param perms Posix style file permissions mask.
 *  @return Returns zero if successful, otherwise a negative MPR error code.
 *  @ingroup MprFile
 */
extern int mprMakeDirPath(MprCtx ctx, cchar *path, int perms);

/**
 *  Make a temporary filename.
 *  @description Thread-safe way to make a unique temporary filename. 
 *  @param ctx Any memory context allocated by the MPR.
 *  @param buf Buffer to hold the name of the filename.
 *  @param bufsize Maximum length for the generated filename.
 *  @param tmpDir Base directory in which the temp file will be allocated.
 *  @return Returns zero if successful. Otherwise it returns MPR_ERR_CANT_CREATE.
 *  @ingroup MprFile
 */
extern int mprMakeTempFileName(MprCtx ctx, char *buf, int bufsize, cchar *tmpDir);

/**
 *  Open a file
 *  @description Open a file and return a file object.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename String containing the filename to open or create.
 *  @param omode Posix style file open mode mask. The open mode may contain 
 *      the following mask values ored together:
 *      @li O_RDONLY Open read only
 *      @li O_WRONLY Open write only
 *      @li O_RDWR Open for read and write
 *      @li O_CREAT Create or re-create
 *      @li O_TRUNC Truncate
 *      @li O_BINARY Open for binary data
 *      @li O_TEXT Open for text data
 *      @li O_EXCL Open with an exclusive lock
 *      @li O_APPEND Open to append
 *  @param perms Posix style file permissions mask.
 *  @return Returns an MprFile object to use in other file operations.
 *  @ingroup MprFile
 */
extern MprFile *mprOpen(MprCtx ctx, cchar *filename, int omode, int perms);

/**
 *  Non-destructively read a character from the file.
 *  @description Read a single character from the file without advancing the read position.
 *  @param file Pointer to an MprFile object returned via MprOpen.
 *  @return If successful, return the character just read. Otherwise return a negative MPR error code.
 *      End of file is signified by reading 0.
 *  @ingroup MprFile
 */
extern int mprPeekc(MprFile *file);

/**
 *  Write a character to the file.
 *  @description Writes a single character to the file. Output is buffered and is
 *      flushed as required or when mprClose is called.
 *  @param file Pointer to an MprFile object returned via MprOpen.
 *  @param c Character to write
 *  @return One if successful, otherwise returns a negative MPR error code on errors.
 *  @ingroup MprFile
 */
extern int mprPutc(MprFile *file, int c);

/**
 *  Write a line to the file.
 *  @description Writes a single line to the file. Output is buffered and is
 *      flushed as required or when mprClose is called.
 *  @param file Pointer to an MprFile object returned via MprOpen.
 *  @param buf Buffer containing the line to write
 *  @param size Size of \a buf in characters to write
 *  @return The number of characters written to the file. Returns a negative MPR error code on errors.
 *  @ingroup MprFile
 */
extern int mprPuts(MprFile *file, cchar *buf, uint size);

/**
 *  Read data from a file.
 *  @description Reads data from a file. 
 *  @param file Pointer to an MprFile object returned via MprOpen.
 *  @param buf Buffer to contain the read data.
 *  @param size Size of \a buf in characters.
 *  @return The number of characters read from the file. Returns a negative MPR error code on errors.
 *  @ingroup MprFile
 */
extern int mprRead(MprFile *file, void *buf, uint size);

/**
 *  Seek the I/O pointer to a new location in the file.
 *  @description Move the position in the file to/from which I/O will be performed in the file. Seeking prior 
 *      to a read or write will cause the next I/O to occur at that location.
 *  @param file Pointer to an MprFile object returned via MprOpen.
 *  @param seekType Seek type may be one of the following three values:
 *      @li SEEK_SET    Seek to a position relative to the start of the file
 *      @li SEEK_CUR    Seek relative to the current position
 *      @li SEEK_END    Seek relative to the end of the file
 *  @param distance A positive or negative byte offset.
 *  @return Returns zero if successful otherwise a negative MPR error code is returned.
 *  @ingroup MprFile
 */
extern long mprSeek(MprFile *file, int seekType, long distance);

/**
 *  Write data to a file.
 *  @description Writes data to a file. 
 *  @param file Pointer to an MprFile object returned via MprOpen.
 *  @param buf Buffer containing the data to write.
 *  @param count Cound of characters in \a buf to write
 *  @return The number of characters actually written to the file. Returns a negative MPR error code on errors.
 *  @ingroup MprFile
 */
extern int mprWrite(MprFile *file, cvoid *buf, uint count);

/**
 *  Flush any buffered write data
 *  @description Write buffered write data and then reset the internal buffers.
 *  @param file Pointer to an MprFile object returned via MprOpen.
 *  @return Zero if successful, otherwise a negative MPR error code.
 *  @ingroup MprFile
 */
extern int mprFlush(MprFile *file);

/**
 *  Compare two filenames
 *  @description Compare two filenames to see if they are equal. This does not convert filenames to absolute form first,
 *      that is the callers responsibility. It does handle case sensitivity appropriately. The len parameter 
 *      if non-zero, specifies how many characters of the filenames to compare.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param path1 First filename to compare
 *  @param path2 Second filename to compare
 *  @param len How many characters to compare.
 *  @returns True if the file exists and can be accessed
 *  @ingroup MprFile
 */
extern int mprCompareFilename(MprCtx ctx, cchar *path1, cchar *path2, int len);

/**
 *  Clean a filename
 *  @description Clean a filename by removing redundant segments. This will remove "./", "../dir" and duplicate 
 *      filename delimiters.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename First filename to compare
 *  @returns A newly allocated, clean filename. Caller should free via #mprFree
 *  @ingroup MprFile
 */
extern char *mprCleanFilename(MprCtx ctx, cchar *filename);

/**
 *  Convert a filename to an absolute filename
 *  @description Get an absolute (canonical) equivalent representation of a filename. 
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename Filename to examine
 *  @returns An absolute filename. Caller should free via #mprFree
 *  @ingroup MprFile
 */
extern char *mprGetAbsFilename(MprCtx ctx, cchar *filename);

/**
 *  Get the filename directory delimiter.
 *  Return the delimiter character used to separate directories on a given file system. Typically "/" or "\"
 *  @param ctx Any memory context allocated by the MPR.
 *  @param path Use this path to specify either the root of the file system or a file on the file system.
 *  @returns The character used as the filename directory delimiter.
 *  @ingroup MprFile
 */
extern int mprGetFileDelimiter(MprCtx ctx, cchar *path);

/**
 *  Get the file newline character string
 *  Return the character string used to delimit new lines in text files.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param path Use this path to specify either the root of the file system or a file on the file system.
 *  @returns A string used to delimit new lines. This is typically "\n" or "\r\n"
 *  @ingroup MprFile
 */
extern cchar *mprGetFileNewline(MprCtx ctx, cchar *path);

//  TODO - inconsistent vs mprGetDirName. I prefer this form.
/**
 *  Get the parent directory of a filename
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename File name to examine
 *  @returns An allocated string containing the parent directory. Caller must free using #mprFree.
 *  @ingroup MprFile
 */
extern char *mprGetParentDir(MprCtx ctx, cchar *filename);

/**
 *  Get a relative filename
 *  @description Get an equivalent filename that is relative to the application's current working directory.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename File name to examine
 *  @returns An allocated string containing the relative directory. Caller must free using #mprFree.
 *  @ingroup MprFile
 */
extern char *mprGetRelFilename(MprCtx ctx, cchar *filename);

/**
 *  Get a filename formatted like a Unix path name
 *  @description Get an equivalent filename that is relative to the application's current working directory and is 
 *      formatted like a Unix path name. This means it will use forward slashes ("/") as the directory delimiter and will
 *      not contain any drive specifications.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename File name to examine
 *  @returns An allocated string containing the new filename. Caller must free using #mprFree.
 *  @ingroup MprFile
 */
extern char *mprGetUnixFilename(MprCtx ctx, cchar *filename);

/**
 *  Get a filename formatted like a Windows path name
 *  @description Get an equivalent filename that is relative to the application's current working directory and is 
 *      formatted like a Windows path name. This means it will use backward slashes ("\") as the directory delimiter 
 *      and will contain a drive specification.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename File name to examine
 *  @returns An allocated string containing the new filename. Caller must free using #mprFree.
 *  @ingroup MprFile
 */
extern char *mprGetWinFilename(MprCtx ctx, cchar *filename);

/**
 *  Map the delimiters in a filename.
 *  @description Map the directory delimiters in a filename to the specified delimiter. This is useful to change from
 *      backward to forward slashes when dealing with Windows filenames.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename File name to examine
 *  @param delimiter Character string to use as a directory delimiter. Typically "/" or "\".
 *  @returns An allocated string containing the parent directory. Caller must free using #mprFree.
 *  @ingroup MprFile
 */
extern void mprMapDelimiters(MprCtx ctx, char *filename, int delimiter);

/*
 *  These are internal APIs
 */
extern void mprSetFileNewline(MprCtx ctx, cchar *filename, cchar *newline);
extern void mprSetFileDelimiter(MprCtx ctx, cchar *filename, int delimiter);


typedef struct MprOsService {
    int             dummy;
} MprOsService;

extern MprOsService *mprCreateOsService(MprCtx ctx);
extern int          mprStartOsService(MprOsService *os);
extern void         mprStopOsService(MprOsService *os);

/*
 *  Search path delimiter
 */
#if BLD_WIN_LIKE
#define MPR_SEARCH_DELIM ";"
#else
#define MPR_SEARCH_DELIM ":"
#endif


typedef struct MprModuleService {
    MprList         *modules;
    char            *searchPath;
#if BLD_FEATURE_MULTITHREAD
    struct MprMutex *mutex;
#endif
} MprModuleService;


extern MprModuleService *mprCreateModuleService(MprCtx ctx);
extern int              mprStartModuleService(MprModuleService *os);
extern void             mprStopModuleService(MprModuleService *os);

/**
 *  Module start/stop point function signature
 *  @param mp Module object reference returned from #mprCreateModule
 *  @returns zero if successful, otherwise return a negative MPR error code.
 */ 
typedef int (*MprModuleProc)(struct MprModule *mp);

/**
 *  Loadable Module Service
 *  @description The MPR provides services to load and unload shared libraries.
 *  @see MprModule, mprGetModuleSearchPath, mprSetModuleSearchPath, mprLoadModule, mprUnloadModule, 
 *      mprCreateModule, mprLookupModule, MprModuleProc
 *  @stability Evolving.
 *  @defgroup MprModule MprModule
 */
typedef struct MprModule {
    char            *name;              /**< Unique module name */
    char            *version;           /**< Module version */
    void            *moduleData;        /**< Module specific data */
    void            *handle;            /**< O/S shared library load handle */
    MprModuleProc   start;              /**< Start the module service */
    MprModuleProc   stop;               /**< Stop the module service */
} MprModule;


/**
 *  Loadable module entry point signature. 
 *  @description Loadable modules can have an entry point that is invoked automatically when a module is loaded. 
 *  @ingroup MprModule
 */
typedef MprModule *(*MprModuleEntry)(MprCtx ctx);


/**
 *  Get the module search path
 *  @description Get the directory search path used by the MPR when loading dynamic modules. This is a colon separated (or 
 *      semicolon on Windows) set of directories.
 *  @param ctx Any memory context allocated by the MPR.
 *  @returns The module search path. Caller must not free.
 *  @ingroup MprModule
 */
extern cchar *mprGetModuleSearchPath(MprCtx ctx);

/**
 *  Set the module search path
 *  @description Set the directory search path used by the MPR when loading dynamic modules. This path string must
 *      should be a colon separated (or semicolon on Windows) set of directories. 
 *  @param ctx Any memory context allocated by the MPR.
 *  @param searchPath Colon separated set of directories
 *  @returns The module search path. Caller must not free.
 *  @ingroup MprModule
 */
extern void mprSetModuleSearchPath(MprCtx ctx, char *searchPath);


/**
 *  Create a module
 *  @description This call will create a module object for a loadable module. This should be invoked by the 
 *      module itself in its module entry point to register itself with the MPR.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param name Name of the module
 *  @param version Version string of the form: Major.Minor.patch
 *  @param moduleData to associate with this module
 *  @param start Start function to invoke to start module services
 *  @param stop Stop function to invoke to stop module services
 *  @returns A module object for this module
 *  @ingroup MprModule
 */
extern MprModule *mprCreateModule(MprCtx ctx, cchar *name, cchar *version, void *moduleData, MprModuleProc start,
        MprModuleProc stop);

/**
 *  Load a module
 *  @description Load a module into the MPR. This will load a dynamic shared object (shared library) and call the
 *      modules entry point. If the module has already been loaded, it this call will do nothing and will just
 *      return the already defined module object. 
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename Name of the module to load. The module will be searched using the defined module search path 
 *      (see #mprSetModuleSearchPath). The filename may or may not include a platform specific shared library extension such
 *      as .dll, .so or .dylib. By omitting the library extension, code can portably load shared libraries.
 *  @param entryPoint Name of function to invoke after loading the module.
 *  @returns A module object for this module created in the module entry point by calling #mprCreateModule
 *  @ingroup MprModule
 */
extern MprModule *mprLoadModule(MprCtx ctx, cchar *filename, cchar *entryPoint);

/**
 *  Lookup a module
 *  @description Lookup a module by name and return the module object.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param name Name of the module specified to #mprCreateModule.
 *  @returns A module object for this module created in the module entry point by calling #mprCreateModule
 *  @ingroup MprModule
 */
extern MprModule *mprLookupModule(MprCtx ctx, cchar *name);

/**
 *  Unload a module
 *  @description Unload a module from the MPR. This will unload a dynamic shared object (shared library). This routine
 *      is not fully supported by the MPR and is often fraught with issues. A module must usually be completely inactive 
 *      with no allocated memory when it is unloaded.
 *  @param mp Module object returned via #mprLookupModule
 *  @ingroup MprModule
 */
extern void mprUnloadModule(MprModule *mp);

/*
 *  Event flags
 */
#define MPR_EVENT_CONTINUOUS    0x1     /**< Auto reschedule the event */
#define MPR_EVENT_THREAD        0x2     /**< Run proc using pool thread */
#define MPR_EVENT_RUNNING       0x4     /**< Event currently executing */

/**
 *  Event callback function
 *  @ingroup MprEvent
 */
typedef void (*MprEventProc)(void *data, struct MprEvent *event);

/**
 *  Event object
 *  @description The MPR provides a powerful priority based eventing mechanism. Events are described by MprEvent objects
 *      which are created and queued via #mprCreateEvent. Each event may have a priority and may be one-shot or 
 *      be continuously rescheduled according to a specified period. The event subsystem provides the basis for 
 *      callback timers. 
 *  @see MprEvent, mprCreateEvent, mprCreateTimerEvent, mprRescheduleEvent, mprStopContinuousEvent, 
 *      mprRestartContinuousEvent, MprEventProc
 *  @defgroup MprEvent MprEvent
 */
typedef struct MprEvent {
    MprEventProc        proc;           /**< Callback procedure */
    MprTime             timestamp;      /**< When was the event created */
    int                 priority;       /**< Priority 0-99. 99 is highest */
    int                 period;         /**< Reschedule period */
    int                 flags;          /**< Event flags */
    MprTime             due;            /**< When is the event due */
    void                *data;          /**< Event private data */
    struct MprEvent     *next;          /**< Next event linkage */
    struct MprEvent     *prev;          /**< Previous event linkage */
    struct MprEventService *service;    /* Event service */
} MprEvent;


/*
 *  Event service
 */
typedef struct MprEventService {
    MprEvent        eventQ;             /* Event queue */
    MprEvent        timerQ;             /* Queue of future events */
    MprEvent        taskQ;              /* Task queue */
    MprTime         lastEventDue;       /* When the last event is due */
    MprTime         lastRan;            /* When last checked queues */
    MprTime         now;                /* Current notion of time */
    int             eventCounter;       /* Incremented for each event (wraps) */

#if BLD_FEATURE_MULTITHREAD
    struct MprSpin  *spin;              /* Multi-thread sync */
#endif

} MprEventService;


extern MprEventService *mprCreateEventService(MprCtx ctx);
extern int      mprStartEventService(MprEventService *ts);
extern int      mprStopEventService(MprEventService *ts);
extern int      mprGetEventCounter(MprEventService *es);

/*
 *  ServiceEvents parameters
 */
#define MPR_SERVICE_ONE_THING   0x1     /**< Wait for one event or one I/O */

/**
 *  Service events
 *  @description Service the event queue. This call will block for the given delay until an event is ready to be
 *      serviced. Flags may modify the calls behavior.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param delay Time in milliseconds to block until an event occurs.
 *  @param flags If set to MPR_SERVICE_ONE_THING, this call will service at most one event. Otherwise set to zero.
 *  @returns A count of the number of events serviced
 *  @ingroup MprEvent
 */
extern int mprServiceEvents(MprCtx ctx, int delay, int flags);

extern void     mprDoEvent(MprEvent *event, void *poolThread);
extern MprEvent *mprGetNextEvent(MprEventService *es);
extern int      mprGetIdleTime(MprEventService *es);

/**
 *  Create a new event
 *  @description Create and queue a new event for service
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param proc Function to invoke when the event is run
 *  @param period Time in milliseconds used by continuous events between firing of the event.
 *  @param priority Priority to associate with the event. Priorities are integer values between 0 and 100 inclusive with
 *      50 being a normal priority. Useful constants are: 
 *      @li MPR_LOW_PRIORITY
 *      @li MPR_NORMAL_PRIORITY
 *      @li MPR_HIGH_PRIORITY
 *  @param data Data to associate with the event and stored in event->data.
 *  @param flags Flags to modify the behavior of the event. Valid values are: MPR_EVENT_CONTINUOUS to create an 
 *      event which will be automatically rescheduled accoring to the specified period.
 *  @ingroup MprEvent
 */
extern MprEvent *mprCreateEvent(MprCtx ctx, MprEventProc proc, int period, int priority, void *data, int flags);

/**
 *  Remove an event
 *  @description Remove a queued event. This is useful to remove continuous events from the event queue.
 *  @param event Event object returned from #mprCreateEvent
 *  @ingroup MprEvent
 */
extern void mprRemoveEvent(MprEvent *event);

/**
 *  Stop an event
 *  @description Stop a continuous event and remove from the queue. The event object is not freed, but simply removed
 *      from the event queue.
 *  @param event Event object returned from #mprCreateEvent
 *  @ingroup MprEvent
 */
extern void mprStopContinuousEvent(MprEvent *event);

/**
 *  Restart an event
 *  @description Restart a continuous event after it has been stopped via #mprStopContinuousEvent. This call will 
 *      add the event to the event queue and it will run after the configured event period has expired.
 *  @param event Event object returned from #mprCreateEvent
 *  @ingroup MprEvent
 */
extern void mprRestartContinuousEvent(MprEvent *event);

/**
 *  Create a timer event
 *  @description Create and queue a timer event for service. This is a convenience wrapper to create continuous
 *      events over the #mprCreateEvent call.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param proc Function to invoke when the event is run
 *  @param period Time in milliseconds used by continuous events between firing of the event.
 *  @param priority Priority to associate with the event. Priorities are integer values between 0 and 100 inclusive with
 *      50 being a normal priority. Useful constants are: 
 *      @li MPR_LOW_PRIORITY
 *      @li MPR_NORMAL_PRIORITY
 *      @li MPR_HIGH_PRIORITY
 *  @param data Data to associate with the event and stored in event->data.
 *  @param flags Not used.
 *  @ingroup MprEvent
 */
extern MprEvent *mprCreateTimerEvent(MprCtx ctx, MprEventProc proc, int period, int priority, void *data, int flags);

/**
 *  Reschedule an event
 *  @description Reschedule a continuous event by modifying its period.
 *  @param event Event object returned from #mprCreateEvent
 *  @param period Time in milliseconds used by continuous events between firing of the event.
 *  @ingroup MprEvent
 */
extern void mprRescheduleEvent(MprEvent *event, int period);

#if BLD_FEATURE_XML
/*
 *  XML parser states. The states that are passed to the user handler have "U" appended to the comment.
 *  The error states (ERR and EOF) must be negative.
 */
#define MPR_XML_ERR                 -1      /* Error */
#define MPR_XML_EOF                 -2      /* End of input */
#define MPR_XML_BEGIN               1       /* Before next tag               */
#define MPR_XML_AFTER_LS            2       /* Seen "<"                      */
#define MPR_XML_COMMENT             3       /* Seen "<!--" (usr)        U    */
#define MPR_XML_NEW_ELT             4       /* Seen "<tag" (usr)        U    */
#define MPR_XML_ATT_NAME            5       /* Seen "<tag att"               */
#define MPR_XML_ATT_EQ              6       /* Seen "<tag att" =             */
#define MPR_XML_NEW_ATT             7       /* Seen "<tag att = "val"   U    */
#define MPR_XML_SOLO_ELT_DEFINED    8       /* Seen "<tag../>"          U    */
#define MPR_XML_ELT_DEFINED         9       /* Seen "<tag...>"          U    */
#define MPR_XML_ELT_DATA            10      /* Seen "<tag>....<"        U    */
#define MPR_XML_END_ELT             11      /* Seen "<tag>....</tag>"   U    */
#define MPR_XML_PI                  12      /* Seen "<?processingInst"  U    */
#define MPR_XML_CDATA               13      /* Seen "<![CDATA["         U    */

/*
 *  Lex tokens
 */
typedef enum MprXmlToken {
    MPR_XMLTOK_ERR,
    MPR_XMLTOK_TOO_BIG,                     /* Token is too big */
    MPR_XMLTOK_CDATA,
    MPR_XMLTOK_COMMENT,
    MPR_XMLTOK_INSTRUCTIONS,
    MPR_XMLTOK_LS,                          /* "<" -- Opening a tag */
    MPR_XMLTOK_LS_SLASH,                    /* "</" -- Closing a tag */
    MPR_XMLTOK_GR,                          /* ">" -- End of an open tag */
    MPR_XMLTOK_SLASH_GR,                    /* "/>" -- End of a solo tag */
    MPR_XMLTOK_TEXT,
    MPR_XMLTOK_EQ,
    MPR_XMLTOK_EOF,
    MPR_XMLTOK_SPACE,
} MprXmlToken;

typedef int (*MprXmlHandler)(struct MprXml *xp, int state, cchar *tagName, cchar* attName, cchar* value);
typedef int (*MprXmlInputStream)(struct MprXml *xp, void *arg, char *buf, int size);

/*
 *  Per XML session structure
 */
typedef struct MprXml {
    MprXmlHandler       handler;            /* Callback function */
    MprXmlInputStream   readFn;             /* Read data function */
    MprBuf              *inBuf;             /* Input data queue */
    MprBuf              *tokBuf;            /* Parsed token buffer */
    int                 quoteChar;          /* XdbAtt quote char */
    int                 lineNumber;         /* Current line no for debug */
    void                *parseArg;          /* Arg passed to mprXmlParse() */
    void                *inputArg;          /* Arg for mprXmlSetInputStream() */
    char                *errMsg;            /* Error message text */
} MprXml;

extern MprXml   *mprXmlOpen(MprCtx ctx, int initialSize, int maxSize);
extern void     mprXmlSetParserHandler(MprXml *xp, MprXmlHandler h);
extern void     mprXmlSetInputStream(MprXml *xp, MprXmlInputStream s, void *arg);
extern int      mprXmlParse(MprXml *xp);
extern void     mprXmlSetParseArg(MprXml *xp, void *parseArg);
extern void     *mprXmlGetParseArg(MprXml *xp);
extern cchar    *mprXmlGetErrorMsg(MprXml *xp);
extern int      mprXmlGetLineNumber(MprXml *xp);

#endif /* BLD_FEATURE_XML */


#if BLD_FEATURE_MULTITHREAD
/**
 *  Multithreaded Synchronization Services
 *  @see MprMutex, mprCreateStaticLock, mprFree, mprLock, mprTryLock, mprUnlock, mprGlobalLock, mprGlobalUnlock, 
 *      MprSpin, mprCreateSpinLock, MprCond, mprCreateCond, mprWaitForCond, mprSignalCond, mprFree
 *  @stability Evolving.
 *  @defgroup MprSynch MprSynch
 */
typedef struct MprSynch { int dummy; } MprSynch;

/**
 *  Multithreading lock control structure
 *  @description MprMutex is used for multithread locking in multithreaded applications.
 *  @ingroup MprSynch
 */
typedef struct MprMutex {
    #if BLD_WIN_LIKE
        CRITICAL_SECTION cs;            /**< Internal mutex critical section */
    #elif VXWORKS
        SEM_ID      cs;
    #elif BLD_UNIX_LIKE
        pthread_mutex_t  cs;
    #else
        error("Unsupported OS");
    #endif
} MprMutex;


#if !__UCLIBC__
#define BLD_HAS_SPINLOCK    1
#endif

/**
 *  Multithreading spin lock control structure
 *  @description    MprSpin is used for multithread locking in multithreaded applications.
 *  @ingroup MprSynch
 */
typedef struct MprSpin {
    #if USE_MPR_LOCK
        MprMutex            cs;
    #elif BLD_WIN_LIKE
        CRITICAL_SECTION    cs;            /**< Internal mutex critical section */
    #elif VXWORKS
        SEM_ID              cs;
    #elif MACOSX
        OSSpinLock          cs;
    #elif BLD_UNIX_LIKE && BLD_HAS_SPINLOCK
        pthread_spinlock_t  cs;
    #elif BLD_UNIX_LIKE
        pthread_mutex_t     cs;
    #else
        error("Unsupported OS");
    #endif
    #if BLD_DEBUG
        MprOsThread         owner;
    #endif
} MprSpin;


/**
 *  Create a Mutex lock object.
 *  @description This call creates a Mutex lock object that can be used in #mprLock, #mprTryLock and #mprUnlock calls. 
 *      Use #mprFree to destroy the lock.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @ingroup MprSynch
 */
extern MprMutex *mprCreateLock(MprCtx ctx);

//  TODO - rename mprInitLock
/**
 *  Initialize a statically allocated Mutex lock object.
 *  @description This call initialized a Mutex lock object without allocation. The object can then be used used 
 *      in #mprLock, #mprTryLock and #mprUnlock calls.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param mutex Reference to an MprMutex structure to initialize
 *  @returns A reference to the supplied mutex. Returns null on errors.
 *  @ingroup MprSynch
 */
extern MprMutex *mprCreateStaticLock(MprCtx ctx, MprMutex *mutex);

/**
 *  Attempt to lock access.
 *  @description This call attempts to assert a lock on the given \a lock mutex so that other threads calling 
 *      mprLock or mprTryLock will block until the current thread calls mprUnlock.
 *  @returns Returns zero if the successful in locking the mutex. Returns a negative MPR error code if unsuccessful.
 *  @ingroup MprSynch
 */
extern bool mprTryLock(MprMutex *lock);

/**
 *  Create a spin lock lock object.
 *  @description This call creates a spinlock object that can be used in #mprSpinLock, and #mprSpinUnlock calls. Spin locks
 *      using MprSpin are much faster than MprMutex based locks on some systems.
 *      Use #mprFree to destroy the lock.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @ingroup MprSynch
 */
extern MprSpin *mprCreateSpinLock(MprCtx ctx);


//  TODO - rename mprInitSpinLock
/**
 *  Initialize a statically allocated spinlock object.
 *  @description This call initialized a spinlock lock object without allocation. The object can then be used used 
 *      in #mprSpinLock and #mprSpinUnlock calls.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param lock Reference to a static #MprSpin  object.
 *  @returns A reference to the MprSpin object. Returns null on errors.
 *  @ingroup MprSynch
 */
extern MprSpin *mprCreateStaticSpinLock(MprCtx ctx, MprSpin *lock);


/**
 *  Attempt to lock access on a spin lock
 *  @description This call attempts to assert a lock on the given \a spin lock so that other threads calling 
 *      mprSpinLock or mprTrySpinLock will block until the current thread calls mprSpinUnlock.
 *  @returns Returns zero if the successful in locking the spinlock. Returns a negative MPR error code if unsuccessful.
 *  @ingroup MprSynch
 */
extern bool mprTrySpinLock(MprSpin *lock);

/*
 *  For maximum performance, use the spin lock/unlock routines macros
 */
#if BLD_USE_LOCK_MACROS && !DOXYGEN
    /*
     *  Spin lock macros
     */
    #if MACOSX
        #define mprSpinLock(lock)   OSSpinLockLock(&((lock)->cs))
        #define mprSpinUnlock(lock) OSSpinLockUnlock(&((lock)->cs))
    #elif BLD_UNIX_LIKE && BLD_HAS_SPINLOCK
        #define mprSpinLock(lock)   pthread_spin_lock(&((lock)->cs))
        #define mprSpinUnlock(lock) pthread_spin_unlock(&((lock)->cs))
    #elif BLD_UNIX_LIKE
        #define mprSpinLock(lock)   pthread_mutex_lock(&((lock)->cs))
        #define mprSpinUnlock(lock) pthread_mutex_unlock(&((lock)->cs))
    #elif BLD_WIN_LIKE
        #define mprSpinLock(lock)   EnterCriticalSection(&((lock)->cs))
        #define mprSpinUnlock(lock) LeaveCriticalSection(&((lock)->cs))
    #elif VXWORKS
        #define mprSpinLock(lock)   semTake((lock)->cs, WAIT_FOREVER)
        #define mprSpinUnlock(lock) semGive((lock)->cs)
    #endif

    /*
     *  Lock macros
     */
    #if BLD_UNIX_LIKE
        #define mprLock(lock)       pthread_mutex_lock(&((lock)->cs))
        #define mprUnlock(lock)     pthread_mutex_unlock(&((lock)->cs))
    #elif BLD_WIN_LIKE
        #define mprUnlock(lock)     LeaveCriticalSection(&((lock)->cs))
        #define mprLock(lock)       EnterCriticalSection(&((lock)->cs))
    #elif VXWORKS
        #define mprUnlock(lock)     semGive((lock)->cs)
        #define mprLock(lock)       semTake((lock)->cs, WAIT_FOREVER)
    #endif
#else

    /**
     *  Lock access.
     *  @description This call asserts a lock on the given \a lock mutex so that other threads calling mprLock will 
     *      block until the current thread calls mprUnlock.
     *  @ingroup MprSynch
     */
    extern void mprLock(MprMutex *lock);

    /**
     *  Unlock a mutex.
     *  @description This call unlocks a mutex previously locked via mprLock or mprTryLock.
     *  @ingroup MprSynch
     */
    extern void mprUnlock(MprMutex *lock);

    /**
     *  Lock a spinlock.
     *  @description This call asserts a lock on the given \a spinlock so that other threads calling mprSpinLock will
     *      block until the curren thread calls mprSpinUnlock.
     *  @ingroup MprSynch
     */
    extern void mprSpinLock(MprSpin *lock);

    /**
     *  Unlock a spinlock.
     *  @description This call unlocks a spinlock previously locked via mprSpinLock or mprTrySpinLock.
     *  @ingroup MprSynch
     */
    extern void mprSpinUnlock(MprSpin *lock);
#endif

/**
 *  Globally lock the application.
 *  @description This call asserts the application global lock so that other threads calling mprGlobalLock will 
 *      block until the current thread calls mprGlobalUnlock.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @ingroup MprSynch
 */
extern void mprGlobalLock(MprCtx ctx);

/**
 *  Unlock the global mutex.
 *  @description This call unlocks the global mutex previously locked via mprGlobalLock.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @ingroup MprSynch
 */
extern void mprGlobalUnlock(MprCtx ctx);

/**
 *  Condition variable for multi-thread synchronization. Condition variables can be used to coordinate threads 
 *  when running in a multi-threaded mode. These variables are level triggered in that a condition can be 
 *  signalled prior to another thread waiting. That thread will then not block if it calls waitForCond().
 *  @ingroup MprSynch
 */
typedef struct MprCond {
    #if BLD_UNIX_LIKE
        pthread_cond_t cv;              /**< Unix pthreads condition variable */
    #elif BLD_WIN_LIKE
        HANDLE      cv;                 /* Windows event handle */
        int         numWaiting;         /* Number waiting to be signalled */
    #elif VXWORKS
        SEM_ID      cv;                 /* Condition variable */
    #else
        error("Unsupported OS");
    #endif
        MprMutex    *mutex;             /**< Thread synchronization mutex */
        int         triggered;          /**< Value of the condition */
} MprCond;


/**
 *  Create a condition lock variable.
 *  @description This call creates a condition variable object that can be used in #mprWaitForCond and #mprSignalCond calls. 
 *      Use #mprFree to destroy the condition variable.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @ingroup MprSynch
 */
extern MprCond *mprCreateCond(MprCtx ctx);

/**
 *  Wait for a condition lock variable.
 *  @description Wait for a condition lock variable to be signaled. If the condition is signaled before the timeout
 *      expires this call will reset the condition variable and return. This way, it automatically resets the variable
 *      for future waiters.
 *  @param cond Condition variable object created via #mprCreateCond
 *  @param timeout Time in milliseconds to wait for the condition variable to be signaled.
 *  @ingroup MprSynch
 */
extern int mprWaitForCond(MprCond *cond, int timeout);

/**
 *  Signal a condition lock variable.
*   @description Signal a condition variable and set it to the \a triggered status. Existing or future callers of
*       #mprWaitForCond will be awakened.
 *  @param cond Condition variable object created via #mprCreateCond
 *  @ingroup MprSynch
 */
extern void mprSignalCond(MprCond *cond);


/*
 *  Thread service
 */
typedef struct MprThreadService {
    MprList         *threads;           /* List of all threads */
    struct MprThread *mainThread;       /* Main application Mpr thread id */
    MprMutex        *mutex;             /* Multi-thread sync */
    int             stackSize;          /* Default thread stack size */
} MprThreadService;


typedef void (*MprThreadProc)(void *arg, struct MprThread *tp);

extern MprThreadService *mprCreateThreadService(struct Mpr *mpr);
extern int mprStartThreadService(MprThreadService *ts);
extern int mprStopThreadService(MprThreadService *ts, int timeout);

/**
 *  Thread Service. 
 *  @description The MPR provides a cross-platform thread abstraction above O/S native threads. It supports 
 *      arbitrary thread creation, thread priorities, thread management and thread local storage. By using these
 *      thread primitives with the locking and synchronization primitives offered by #MprMutex, #MprSpin and 
 *      #MprCond - you can create cross platform multi-threaded applications.
 *  @stability Evolving
 *  @see MprThread, mprCreateThread, mprStartThread, mprGetThreadName, mprGetThreadPriority, 
 *      mprSetThreadPriority, mprGetCurrentThread, mprGetCurrentOsThread, mprSetThreadPriority, 
 *      mprSetThreadData, mprGetThreadData, mprCreateThreadLocal
 *  @defgroup MprThread MprThread
 */
typedef struct MprThread {
    MprOsThread     osThreadID;         /**< O/S thread id */

#if BLD_WIN_LIKE
    handle          threadHandle;       /**< Threads OS handle for WIN */
#endif
    void            *data;              /**< Data argument */
    MprThreadProc   entry;              /**< Users thread entry point */
    char            *name;              /**< Name of thead for trace */
    MprMutex        *mutex;             /**< Multi-thread synchronization */
    ulong           pid;                /**< Owning process id */
    int             priority;           /**< Current priority */
    int             stackSize;          /**< Only VxWorks implements */
} MprThread;


/**
 *  Thread local data storage
 */
typedef struct MprThreadLocal {
#if BLD_UNIX_LIKE
    pthread_key_t   key;                /**< Data key */
#elif BLD_WIN_LIKE
    DWORD           key;
#endif
} MprThreadLocal;


/**
 *  Create a new thread
 *  @description MPR threads are usually real O/S threads and can be used with the various locking services (#MprMutex,
 *      #MprCond, #MprSpin) to enable scalable multithreaded applications.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param name Unique name to give the thread
 *  @param proc Entry point function for the thread. #mprStartThread will invoke this function to start the thread
 *  @param data Thread private data stored in MprThread.data
 *  @param priority Priority to associate with the thread. Mpr thread priorities are are integer values between 0 
 *      and 100 inclusive with 50 being a normal priority. The MPR maps these priorities in a linear fashion onto 
 *      native O/S priorites. Useful constants are: 
 *      @li MPR_LOW_PRIORITY
 *      @li MPR_NORMAL_PRIORITY
 *      @li MPR_HIGH_PRIORITY
 *  @param stackSize Stack size to use for the thread. On VM based systems, increasing this value, does not 
 *      necessarily incurr a real memory (working-set) increase. Set to zero for a default stack size.
 *  @returns A MprThread object
 *  @ingroup MprThread
 */
extern MprThread *mprCreateThread(MprCtx ctx, cchar *name, MprThreadProc proc, void *data, int priority, int stackSize);

/**
 *  Start a thread
 *  @description Start a thread previously created via #mprCreateThread. The thread will begin at the entry function 
 *      defined in #mprCreateThread.
 *  @param thread Thread object returned from #mprCreateThread
 *  @return Returns zero if successful, otherwise a negative MPR error code.
 *  @ingroup MprThread
 */
extern int mprStartThread(MprThread *thread);

/**
 *  Get the thread name.
 *  @description MPR threads are usually real O/S threads and can be used with the various locking services (#MprMutex,
 *      #MprCond, #MprSpin) to enable scalable multithreaded applications.
 *  @param thread Thread object returned from #mprCreateThread
 *  @return Returns a string name for the thread. Caller must not free.
 *  @ingroup MprThread
 */
extern cchar *mprGetThreadName(MprThread *thread);

/**
 *  Get the thread priroity
 *  @description Get the current priority for the specified thread.
 *  @param thread Thread object returned by #mprCreateThread
 *  @returns An integer MPR thread priority between 0 and 100 inclusive.
 *  @ingroup MprThread
 */
extern int mprGetThreadPriority(MprThread *thread);

/**
 *  Set the thread priroity
 *  @description Set the current priority for the specified thread.
 *  @param thread Thread object returned by #mprCreateThread
 *  @param priority Priority to associate with the thread. Mpr thread priorities are are integer values between 0 
 *      and 100 inclusive with 50 being a normal priority. The MPR maps these priorities in a linear fashion onto 
 *      native O/S priorites. Useful constants are: 
 *      @li MPR_LOW_PRIORITY
 *      @li MPR_NORMAL_PRIORITY
 *      @li MPR_HIGH_PRIORITY
 *  @ingroup MprThread
 */
extern void mprSetThreadPriority(MprThread *thread, int priority);

/**
 *  Get the currently executing thread.
 *  @description Get the thread object for the currently executing O/S thread.
 *  @param ctx Any memory context allocated by the MPR.
 *  @return Returns a thread object representing the current O/S thread.
 *  @ingroup MprThread
 */
extern MprThread *mprGetCurrentThread(MprCtx ctx);

/**
 *  Get the O/S thread
 *  @description Get the O/S thread ID for the currently executing thread.
 *  @return Returns a platform specific O/S thread ID. On Unix, this is a pthread reference. On other systems it is
 *      a thread integer value.
 *  @ingroup MprThread
 */
extern MprOsThread mprGetCurrentOsThread();

/**
 *  Set the thread priroity for the current thread.
 *  @param ctx Any memory context allocated by the MPR.
 *  @description Set the current priority for the specified thread.
 *  @param priority Priority to associate with the thread. Mpr thread priorities are are integer values between 0 
 *      and 100 inclusive with 50 being a normal priority. The MPR maps these priorities in a linear fashion onto 
 *      native O/S priorites. Useful constants are: 
 *      @li MPR_LOW_PRIORITY
 *      @li MPR_NORMAL_PRIORITY
 *      @li MPR_HIGH_PRIORITY
 *  @ingroup MprThread
 */
extern void mprSetCurrentThreadPriority(MprCtx ctx, int priority);

/*
 *  Somewhat internal APIs
 */
extern int mprMapMprPriorityToOs(int mprPriority);
extern int mprMapOsPriorityToMpr(int nativePriority);
extern void mprSetThreadStackSize(MprCtx ctx, int size);
extern int mprSetThreadData(MprThreadLocal *tls, void *value);
extern void *mprGetThreadData(MprThreadLocal *tls);
extern MprThreadLocal *mprCreateThreadLocal();

#else /* !BLD_FEATURE_MULTITHREAD */

typedef struct MprThreadLocal {
    int             dummy;
} MprThreadLocal;
typedef void *MprThread;

#define mprInitThreads(ctx, mpr)
#define mprTermThreads(mpr)
#define mprCreateLock(ctx)
#define mprLock(lock)
#define mprTryLock(lock)
#define mprUnlock(lock)
#define mprCreateSpinLock(ctx)
#define mprSpinLock(lock)
#define mprTrySpinLock(lock)
#define mprSpinUnlock(lock)
#define mprGlobalLock(mpr)
#define mprGlobalUnlock(mpr)
#define mprSetThreadData(tls, value)
#define mprGetThreadData(tls) NULL
#define mprCreateThreadLocal(ejs) ((void*) 1)
#endif /* BLD_FEATURE_MULTITHREAD */

extern cchar        *mprGetCurrentThreadName(MprCtx ctx);

/*
 *  Magic number to identify blocks. Only used in debug mode.
 */
#define MPR_ALLOC_MAGIC     0xe814ecab

/**
 *  Memory allocation error callback. Notifiers are called if mprSetNotifier has been called on a context and a 
 *  memory allocation fails. All notifiers up the parent context chain are called in order.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param size Size of memory allocation request that failed
 *  @param total Total memory allocations so far
 *  @param granted Set to true if the request was actually granted, but the application is now exceeding its redline
 *      memory limit.
 *  @ingroup MprMem
 */
typedef void (*MprAllocNotifier)(MprCtx ctx, uint size, uint total, bool granted);

/**
 *  Mpr memory block destructors prototype
 *  @param ctx Any memory context allocated by the MPR.
 *  @return Return zero if the memory was actually freed. Return non-zero to prevent the memory being freed.
 *  @ingroup MprMem
 */
typedef int (*MprDestructor)(MprCtx ctx);

/*
 *  Align blocks on 8 byte boundaries.
 */
#define MPR_ALLOC_ALIGN(x)  (((x) + 7 ) & ~7)
#define MPR_PAGE_ALIGN(x, pagesize) (((x) + (pagesize) - 1) & ~(pagesize - 1))

#if BLD_DEBUG
#define BLD_FEATURE_MEMORY_DEBUG    1       /* Enable memory debug assist. Fill blocks, verifies block integrity. */
#define BLD_FEATURE_MEMORY_STATS    1       /* Include memory stats routines */
#else
#define BLD_FEATURE_MEMORY_STATS    1
#endif

/*
 *  MprBlk flags
 */
#define MPR_ALLOC_HAS_DESTRUCTOR    0x1     /* Block has a destructor to be called when freed */
#define MPR_ALLOC_HAS_ERROR         0x2     /* Memory context has had allocation errors */
#define MPR_ALLOC_IS_HEAP           0x4     /* Block is a heap context */
#define MPR_ALLOC_FROM_MALLOC       0x8     /* Block allocated from a malloc heap */

/**
 *  Memory Allocation Block Header.
 *  @ingroup MprMem
 */
typedef struct MprBlk {
    struct MprBlk   *parent;                /* Parent block */
    struct MprBlk   *children;              /* First child block. Flags stored in low order bits. */
    struct MprBlk   *next;                  /* Next sibling. Flags stored in low order bits. */

    uint            size: 28;               /* Size of the block (not counting header) */
    uint            flags: 4;               /* Flags */

#if BLD_FEATURE_MEMORY_DEBUG
    /*
     *  For debug, we don't worry about this bloating the MprBlk and messing up alignment.
     */
    uint            magic;                  /* Unique signature. Messes up alignment for debug */
#endif
} MprBlk;


//  TODO - refactor. Since we now use bit fields in MprBlk - dont need some of these
#define MPR_ALLOC_HDR_SIZE      (MPR_ALLOC_ALIGN(sizeof(struct MprBlk)))
#define MPR_GET_BLK(ptr)        ((MprBlk*) (((char*) (ptr)) - MPR_ALLOC_HDR_SIZE))
#define MPR_GET_PTR(bp)         ((char*) (((char*) (bp)) + MPR_ALLOC_HDR_SIZE))
#define MPR_GET_BLK_SIZE(bp)    ((bp)->size)
#define MPR_SET_SIZE(bp, len)   ((bp)->size = len)
#define mprGetBlockSize(ptr)    ((ptr) ? (MPR_GET_BLK_SIZE(MPR_GET_BLK(ptr)) - MPR_ALLOC_HDR_SIZE): 0)

/*
 *  Region of memory. Regions are used to describe chunks of memory used by Heaps.
 */
typedef struct MprRegion {
    struct MprRegion *next;                 /* Next region in chain */
    char            *memory;                /* Region memory data */
    char            *nextMem;               /* Pointer to next free byte in memory */
    int             vmSize;                 /* Size of virtual memory containing the region struct plus region memory */
    int             size;                   /* Original size of region */
    int             remaining;              /* Remaining bytes in the region */
} MprRegion;


/*
 *  Heap flags
 */
#define MPR_ALLOC_PAGE_HEAP     0x1         /* Page based heap. Used for allocating arenas and slabs */
#define MPR_ALLOC_ARENA_HEAP    0x2         /* Heap is an arena. All allocations are done from one or more regions */
#define MPR_ALLOC_SLAB_HEAP     0x4         /* Heap is a slab. Constant sized objects use slab heaps */
#define MPR_ALLOC_FREE_CHILDREN 0x8         /* Heap must be accessed in a thread safe fashion */
#define MPR_ALLOC_THREAD_SAFE   0x10        /* Heap must be accessed in a thread safe fashion */

/*
 *  The heap context supports arena and slab based allocations. Layout of allocated heap blocks:
 *      HDR
 *      MprHeap
 *      MprRegion
 *      Heap Data
 *      Destructor
 */
typedef struct MprHeap {
    cchar           *name;                  /* Debugging name of the heap */
    MprDestructor   destructor;             /* Heap destructor routine */
    MprRegion       *region;                /* Current region of memory for allocation */
    MprRegion       *depleted;              /* Depleted regions. All useful memory has been allocated */
    int             flags;                  /* Heap flags */

    /*
     *  Slab allocation object information and free list
     */
    int             objSize;                /* Size of each heap object */
    MprBlk          *freeList;              /* Linked list of free objects */

    /*
     *  Heap stats
     */
    int            allocBytes;             /* Number of bytes allocated for this heap */
    int            peakAllocBytes;         /* Peak allocated (max allocBytes) */
    int            allocBlocks;            /* Number of alloced blocks for this heap */
    int            peakAllocBlocks;        /* Peak allocated blocks */
    int            totalAllocCalls;        /* Total count of allocation calls */
    int            freeListCount;          /* Count of objects on freeList */
    int            peakFreeListCount;      /* Peak count of blocks on the free list */
    int            reuseCount;             /* Count of allocations from the freelist */
    int            reservedBytes;          /* Virtual allocations for page heaps */

    MprAllocNotifier notifier;              /* Memory allocation failure callback */

#if BLD_FEATURE_MULTITHREAD
    MprSpin         spin;
#endif
} MprHeap;


/*
 *  Memory allocation control
 */
typedef struct MprAlloc {
    MprHeap         pageHeap;               /* Page based heap for Arena allocations */
    uint            pageSize;               /* System page size */
    int             inAllocException;       /* Recursive protect */
    uint            bytesAllocated;         /* Bytes currently allocated */
    uint            peakAllocated;          /* Peak bytes allocated */
    uint            errors;                 /* Allocation errors */
    uint            peakStack;              /* Peak stack usage */
    uint            numCpu;                 /* Number of CPUs */
    uint            redLine;                /* Warn if allocation exceeds this level */
    uint            maxMemory;              /* Max memory to allocate */
    void            *stackStart;            /* Start of app stack */
} MprAlloc;


#if BLD_WIN_LIKE || VXWORKS
#define MPR_MAP_READ        0x1
#define MPR_MAP_WRITE       0x2
#define MPR_MAP_EXECUTE     0x4
#else
#define MPR_MAP_READ        PROT_READ
#define MPR_MAP_WRITE       PROT_WRITE
#define MPR_MAP_EXECUTE     PROT_EXECUTE
#endif

extern struct Mpr *mprCreateAllocService(MprAllocNotifier cback, MprDestructor destructor);
extern MprHeap  *mprAllocArena(MprCtx ctx, cchar *name, uint arenaSize, bool threadSafe, MprDestructor destructor);
extern MprHeap  *mprAllocSlab(MprCtx ctx, cchar *name, uint objSize, uint count, bool threadSafe, MprDestructor destructor);
extern void     mprSetAllocNotifier(MprCtx ctx, MprAllocNotifier cback);
extern void     mprInitBlock(MprCtx ctx, void *ptr, uint size);

/**
 *  Allocate a block of memory
 *  @description Allocates a block of memory using the supplied memory context \a ctx as the parent. #mprAlloc 
 *      manages a tree structure of memory blocks. Freeing a block via mprFree will release the allocated block
 *      and all child blocks.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param size Size of the memory block to allocate.
 *  @return Returns a pointer to the allocated block. If memory is not available the memory exhaustion handler 
 *      specified via mprCreate will be called to allow global recovery.
 *  @remarks Do not mix calls to malloc and mprAlloc.
 *  @ingroup MprMem
 */
extern void *mprAlloc(MprCtx ctx, uint size);

//  TODO - Refactor all this naming. the naming with respect to mprAllocObj
/**
 *  Allocate an object block of memory
 *  @description Allocates a block of memory using the supplied memory context \a ctx as the parent. #mprAllocObject
 *      associates a destructor function with an object. This function will be invoked when the object is freed. 
 *      Freeing a block will first call the destructor and if that returns zero, mprFree will release the allocated 
 *      block and all child blocks.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param size Size of the memory block to allocate.
 *  @param destructor Destructor function to invoke when the allocation is freed via #mprFree.
 *  @return Returns a pointer to the allocated block. If memory is not available the memory exhaustion handler 
 *      specified via mprCreate will be called to allow global recovery.
 *  @remarks Do not mix calls to malloc and mprAlloc.
 *  @stability Prototype. This function names are highly likely to be refactored.
 *  @ingroup MprMem
 */
extern void *mprAllocObject(MprCtx ctx, uint size, MprDestructor destructor);

/**
 *  Allocate an object block of memory and zero it.
 *  @description Allocates a zeroed block of memory using the supplied memory context \a ctx as the parent. #mprAllocObject
 *      associates a destructor function with an object. This function will be invoked when the object is freed. 
 *      Freeing a block will first call the destructor and if that returns zero, mprFree will release the allocated 
 *      block and all child blocks.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param size Size of the memory block to allocate.
 *  @param destructor Destructor function to invoke when the allocation is freed via #mprFree.
 *  @return Returns a pointer to the allocated block. If memory is not available the memory exhaustion handler 
 *      specified via mprCreate will be called to allow global recovery.
 *  @remarks Do not mix calls to malloc and mprAlloc.
 *  @stability Prototype. This function names are highly likely to be refactored.
 *  @ingroup MprMem
 */
extern void *mprAllocObjectZeroed(MprCtx ctx, uint size, MprDestructor destructor);

/**
 *  Allocate a zeroed block of memory
 *  @description Allocates a zeroed block of memory using the supplied memory context \a ctx as the parent. #mprAlloc 
 *      manages a tree structure of memory blocks. Freeing a block via mprFree will release the allocated block
 *      and all child blocks.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param size Size of the memory block to allocate.
 *  @return Returns a pointer to the allocated block. If memory is not available the memory exhaustion handler 
 *      specified via mprCreate will be called to allow global recovery.
 *  @remarks Do not mix calls to malloc and mprAlloc.
 *  @ingroup MprMem
 */
extern void *mprAllocZeroed(MprCtx ctx, uint size);

/**
 *  Reallocate a block
 *  @description Reallocates a block increasing its size. If the specified size is less than the current block size,
 *      the call will ignore the request and simply return the existing block.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param ptr Memory to reallocate. If NULL, call malloc.
 *  @param size New size of the required memory block.
 *  @return Returns a pointer to the allocated block. If memory is not available the memory exhaustion handler 
 *      specified via mprCreate will be called to allow global recovery.
 *  @remarks Do not mix calls to realloc and mprRealloc.
 *  @ingroup MprMem
 */
extern void *mprRealloc(MprCtx ctx, void *ptr, uint size);

/**
 *  Free a block of memory.
 *  @description mprFree should be used to free memory allocated by mprAlloc, or mprRealloc. This call will ignore
 *      calls to free a null pointer, thus it is an acceptable idiom to free a pointer without testing its value for null.
 *      When mprFree is called it will first invoke any object destructor function for the allocated block. If this
 *      destructor returns zero, it will then proceed and free all allocated children before finally releasing the block.
 *  @param ptr Memory to free. If NULL, take no action.
 *  @return Returns zero if the block was actually freed. If the destructor prevented the freeing, a non-zero value will
 *      be returned. 
 *  @ingroup MprMem
 */
extern int mprFree(void *ptr);

/**
 *  Update the destructor for a block of memory.
 *  @description This call updates the destructor for a block of memory allocated via mprAllocObject.
 *  @param ptr Memory to free. If NULL, take no action.
 *  @param destructor Destructor function to invoke when #mprFree is called.
 *  @ingroup MprMem
 */
extern void     mprSetDestructor(void *ptr, MprDestructor destructor);
extern void     mprFreeChildren(void *ptr);
extern int      mprStealBlock(MprCtx ctx, cvoid *ptr);

#if BLD_FEATURE_MEMORY_DEBUG
extern void     mprValidateBlock(MprCtx ctx);
#endif
#if BLD_FEATURE_MEMORY_STATS
extern void     mprPrintAllocReport(MprCtx ctx, cchar *msg);
#endif

#if DOXYGEN
typedef void *Type;
//  TODO - refactor these names
/**
 *  Allocate an object of a given type
 *  @description Allocates a block of memory large enough to hold an instance of the specified type. This uses the 
 *      supplied memory context \a ctx as the parent. This is implemented as a macro
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param type Type of the object to allocate
 *  @return Returns a pointer to the allocated block. If memory is not available the memory exhaustion handler 
 *      specified via mprCreate will be called to allow global recovery.
 *  @remarks Do not mix calls to malloc and mprAlloc.
 *  @stability Prototype. This function names are highly likely to be refactored.
 *  @ingroup MprMem
 */
void *mprAllocObj(MprCtx ctx, Type type) { return 0; }

/**
 *  Allocate a zeroed object of a given type
 *  @description Allocates a zeroed block of memory large enough to hold an instance of the specified type. This uses the 
 *      supplied memory context \a ctx as the parent. This is implemented as a macro
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param type Type of the object to allocate
 *  @return Returns a pointer to the allocated block. If memory is not available the memory exhaustion handler 
 *      specified via mprCreate will be called to allow global recovery.
 *  @remarks Do not mix calls to malloc and mprAlloc.
 *  @stability Prototype. This function names are highly likely to be refactored.
 *  @ingroup MprMem
 */
void *mprAllocObjZeroed(MprCtx ctx, Type type) { return 0; }

/**
 *  Allocate an object of a given type with a destructor
 *  @description Allocates a block of memory large enough to hold an instance of the specified type with a destructor. 
 *      This uses the supplied memory context \a ctx as the parent. This is implemented as a macro.
 *      this call associates a destructor function with an object. This function will be invoked when the object is freed. 
 *      Freeing a block will first call the destructor and if that returns zero, mprFree will release the allocated 
 *      block and all child blocks.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param type Type of the object to allocate
 *  @param destructor Destructor function to invoke when the allocation is freed via #mprFree.
 *  @return Returns a pointer to the allocated block. If memory is not available the memory exhaustion handler 
 *      specified via mprCreate will be called to allow global recovery.
 *  @remarks Do not mix calls to malloc and mprAlloc.
 *  @stability Prototype. This function names are highly likely to be refactored.
 *  @ingroup MprMem
 */
extern void *mprAllocObjWithDestructor(MprCtx ctx, Type type, MprDestructor destructor)

/**
 *  Allocate a zeroed object of a given type with a destructor
 *  @description Allocates a zeroed block of memory large enough to hold an instance of the specified type with a 
 *      destructor. This uses the supplied memory context \a ctx as the parent. This is implemented as a macro.
 *      this call associates a destructor function with an object. This function will be invoked when the object is freed. 
 *      Freeing a block will first call the destructor and if that returns zero, mprFree will release the allocated 
 *      block and all child blocks.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param type Type of the object to allocate
 *  @param destructor Destructor function to invoke when the allocation is freed via #mprFree.
 *  @return Returns a pointer to the allocated block. If memory is not available the memory exhaustion handler 
 *      specified via mprCreate will be called to allow global recovery.
 *  @remarks Do not mix calls to malloc and mprAlloc.
 *  @stability Prototype. This function names are highly likely to be refactored.
 *  @ingroup MprMem
 */
void *mprAllocObjWithDestructorZeroed(MprCtx ctx, Type type, MprDestructor destructor) { return 0;}

#else
/*
 *  Macros for typed based allocations
 */
#define mprAllocObj(ctx, type) \
    ((type*) mprAlloc(ctx, sizeof(type)))
#define mprAllocObjZeroed(ctx, type) \
    ((type*) mprAllocZeroed(ctx, sizeof(type)))
#define mprAllocObjWithDestructor(ctx, type, destructor) \
    ((type*) mprAllocObject(ctx, sizeof(type), (MprDestructor) destructor))
#define mprAllocObjWithDestructorZeroed(ctx, type, destructor) \
    ((type*) mprAllocObjectZeroed(ctx, sizeof(type), (MprDestructor) destructor))
#endif

/**
 *  Determine if the MPR has encountered memory allocation errors.
 *  @description Returns true if the MPR has had a memory allocation error. Allocation errors occur if any
 *      memory allocation would cause the application to exceed the configured redline limit, or if any O/S memory
 *      allocation request fails.
 *  @param ctx Any memory context allocated by the MPR.
 *  @return TRUE if a memory allocation error has occurred. Otherwise returns FALSE.
 *  @ingroup MprMem
 */
extern bool mprHasAllocError(MprCtx ctx);

/**
 *  Reset the memory allocation error flag
 *  @description Reset the alloc error flag triggered.
 *  @param ctx Any memory context allocated by the MPR.
 *  @ingroup MprMem
 */
extern void mprResetAllocError(MprCtx ctx);

extern void mprSetAllocError(MprCtx ctx);

/**
 *  Get the memory parent of a block.
 *  @description Return the parent memory context for a block
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @return Return the memory owning this block
 *  @ingroup MprMem
 */
extern void *mprGetParent(MprCtx ctx);

extern int      mprIsValid(MprCtx ctx);
extern bool     mprStackCheck(MprCtx ctx);

/**
 *  Configure the application memory limits
 *  @description Configure memory limits to constrain memory usage by the application. The memory allocation subsystem
 *      will check these limits before granting memory allocation requrests. The redLine is a soft limit that if exceeded
 *      will invoke the memory allocation callback, but will still honor the request. The maxMemory limit is a hard limit.
 *      The MPR will prevent allocations which exceed this maximum. The memory callback handler is defined via 
 *      the #mprCreate call.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param redline Soft memory limit. If exceeded, the request will be granted, but the memory handler will be invoked.
 *  @param maxMemory Hard memory limit. If exceeded, the request will not be granted, and the memory handler will be invoked.
 *  @ingroup MprMem
 */
extern void mprSetAllocLimits(MprCtx ctx, uint redline, uint maxMemory);

extern MprAlloc *mprGetAllocStats(MprCtx ctx);
extern int      mprGetUsedMemory(MprCtx ctx);

/**
 *  Duplicate a block of memory.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @description Copy a block of memory into a newly allocated block.
 *  @param ptr Pointer to the block to duplicate.
 *  @param size Size of the block to copy.
 *  @return Returns an allocated block. Caller must free via #mprFree.
 *  @ingroup MprMem
 */
extern void *mprMemdup(MprCtx ctx, cvoid *ptr, uint size);

/**
 *  Copy a memory block into an allocated block.
 *  @description Allocate a new memory block of the required size and copy a source block into it. 
 *      The call returns the size of the allocated block.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param dest Pointer to a pointer that will hold the address of the allocated block.
 *  @param destMax Maximum size of the new block.
 *  @param src Block to copy
 *  @param nbytes Size of the \a src block to copy
 *  @return Returns the number of bytes in the allocated block.
 *  @ingroup MprString
 */
extern int mprAllocMemcpy(MprCtx ctx, char **dest, int destMax, cvoid *src, int nbytes);

/*
 *  Memory mapping. Used for stack memory
 */
extern void *mprMapAlloc(uint size, int mode);
extern void mprMapFree(void *ptr, uint size);
extern int mprGetPageSize(MprCtx ctx);

/*
 *  Wait service.
 */
/*
 *  Standard wait for IO options
 */
#define MPR_READABLE            0x2
#define MPR_WRITEABLE           0x4

/*
 *  Wait service flags
 */
#define MPR_BREAK_REQUESTED     0x1         /* Pending wakeup on service thread */
#define MPR_NEED_RECALL         0x2         /* A handler needs to be recalled */

#define MPR_READ_PIPE           0
#define MPR_WRITE_PIPE          1

typedef void    (*MprWaitProc)(void *data, int mask, int isMprPoolThread);

#if BLD_WIN_LIKE
typedef long    (*MprMsgCallback)(HWND hwnd, uint msg, uint wp, long lp);
#endif

typedef struct MprWaitService {
    MprList         *list;                  /* List of handlers */
    int             flags;                  /* State flags */
    int             listGeneration;         /* Generation number for list changes */
    int             maskGeneration;         /* Generation number for mask changes */
    int             lastMaskGeneration;     /* Last generation number for mask changes */
    int             rebuildMasks;           /* IO mask rebuild required */

#if BLD_UNIX_LIKE
    struct pollfd   *fds;                   /* File descriptors to select on */
    int             fdsCount;               /* Count of active fds in array */
    int             fdsSize;                /* Size of fds array */
    int             breakPipe[2];           /* Pipe to wakeup select when multithreaded */
#endif
#if BLD_WIN_LIKE
#if USE_EVENTS
    WSAEVENT        *events;                /* Array of events to select on */
    int             eventsCount;            /* Count of active events in array */
    int             eventsSize;             /* Size of the events array */
    WSAEVENT        breakEvent;             /* Event to wakeup select when multithreaded */
#else
    HWND            hwnd;                   /* Window handle */
    int             socketMessage;          /* Message id for socket events */
    MprMsgCallback  msgCallback;            /* Message handler callback */
#endif
#endif

#if BLD_FEATURE_MULTITHREAD
    MprThread       *serviceThread;         /* Dedicated service thread */
    MprMutex        *mutex;                 /* General multi-thread sync */
#endif

} MprWaitService;


extern MprWaitService *mprCreateWaitService(struct Mpr *mpr);
extern int  mprInitSelectWait(MprWaitService *ws);
extern int  mprStartWaitService(MprWaitService *ws);
extern int  mprStopWaitService(MprWaitService *ws);

#if BLD_WIN_LIKE
extern int  mprInitWindow(MprWaitService *ws);
extern void mprSetWinMsgCallback(MprWaitService *ws, MprMsgCallback callback);
extern void mprServiceWinIO(MprWaitService *ws, int sockFd, int winMask);
#endif

#if BLD_FEATURE_MULTITHREAD
    extern void mprSetWaitServiceThread(MprWaitService *ws, MprThread *thread);
    extern void mprAwakenWaitService(MprWaitService *ws);
#else
    #define mprAwakenWaitService(ws)
#endif

extern int mprWaitForIO(MprWaitService *ws, int timeout);


/*
 *  Handler Flags
 */
#define MPR_WAIT_CLIENT_CLOSED  0x2     /* Client disconnection received */
#define MPR_WAIT_RECALL_HANDLER 0x4     /* Must recall the handler asap */
#define MPR_WAIT_THREAD         0x8     /* Run callback via thread pool */

/**
 *  Wait Handler Service
 *  @description Wait handlers provide callbacks for when I/O events occur. They provide a wait to service many
 *      I/O file descriptors without requiring a thread per descriptor.
 *  @see mprSetWaitInterest, mprWaitForSingleIO, mprSetWaitCallback, mprDisableWaitEvents, mprEnableWaitEvents,
 *      mprRecallWaitHandler, MprWaitHandler, mprCreateEvent, mprServiceEvents, MprEvent
 *  @defgroup MprWaitHandler MprWaitHandler
 */
typedef struct MprWaitHandler {
    int             desiredMask;        /**< Mask of desired events */
    int             disableMask;        /**< Mask of disabled events */
    int             presentMask;        /**< Mask of current events */
    int             fd;                 /**< O/S File descriptor (sp->sock) */
    int             flags;              /**< Control flags */
    void            *handlerData;       /**< Argument to pass to proc */
#if BLD_WIN_LIKE
#if USE_EVENTS
    WSAEVENT        *event;             /**< Wait event handle */
#endif
#endif

#if BLD_FEATURE_MULTITHREAD
    int             priority;           /**< Thread priority */
    struct MprEvent *threadEvent;       /**< Event reference */
#endif

    MprWaitService  *waitService;       /**< Wait service pointer */
    MprWaitProc     proc;               /**< Wait handler procedure */

    struct MprWaitHandler *next;        /**< List linkage */
    struct MprWaitHandler *prev;

} MprWaitHandler;


/**
 *  Create a wait handler
 *  @description Create a wait handler that will be invoked when I/O of interest occurs on the specified file handle
 *      The wait handler is registered with the MPR event I/O mechanism.
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @param fd File descriptor
 *  @param mask Mask of events of interest. This is made by oring MPR_READABLE and MPR_WRITEABLE
 *  @param proc Callback function to invoke when an I/O event of interest has occurred.
 *  @param data Data item to pass to the callback
 *  @param priority MPR priority to associate with the callback. This is only used if the MPR_WAIT_THREAD is specified
 *      in the flags and the MPR is build multithreaded.
 *  @param flags Flags may be set to MPR_WAIT_THREAD if the callback function should be invoked using a thread from
 *      the thread pool.
 *  @returns A new wait handler registered with the MPR event mechanism
 *  @ingroup MprWaitHandler
 */
extern MprWaitHandler *mprCreateWaitHandler(MprCtx ctx, int fd, int mask, MprWaitProc proc, void *data,
        int priority, int flags);

/**
 *  Disable wait events
 *  @description Disable wait events for a given file descriptor.
 *  @param wp Wait handler created vai #mprCreateWaitHandler
 *  @param wakeup Set to true if it should wakeup the MPR service thread.
 *  @ingroup MprWaitHandler
 */
extern void mprDisableWaitEvents(MprWaitHandler *wp, bool wakeup);

/**
 *  Enable wait events
 *  @description Enable wait events for a given file descriptor.
 *  @param wp Wait handler created vai #mprCreateWaitHandler
 *  @param wakeup Set to true if it should wakeup the MPR service thread. This should normally be set to true.
 *  @ingroup MprWaitHandler
 */
extern void mprEnableWaitEvents(MprWaitHandler *wp, bool wakeup);
extern int  mprInsertWaitHandler(MprWaitService *ws, struct MprWaitHandler *wp);
extern int  mprModifyWaitHandler(MprWaitService *ws, struct MprWaitHandler *wp, bool wakeup);
extern void mprInvokeWaitCallback(MprWaitHandler *wp, void *poolThread);

/**
 *  Recall a wait handler
 *  @description Signal that a wait handler should be recalled a the earliest opportunity. This is useful
 *      when a protocol stack has buffered data that must be processed regardless of whether more I/O occurs. 
 *  @param wp Wait handler created vai #mprCreateWaitHandler
 *  @ingroup MprWaitHandler
 */
extern void mprRecallWaitHandler(MprWaitHandler *wp);
extern void mprRemoveWaitHandler(MprWaitService *ws, struct MprWaitHandler *wp);
extern int  mprRunWaitHandler(MprWaitHandler *wp);

/**
 *  Define the events of interest for a wait handler
 *  @description Define the events of interest for a wait handler. The mask describes whether readable or writable
 *      events should be signalled to the wait handler. Disconnection events are passed via read events.
 *  @param wp Wait handler created vai #mprCreateWaitHandler
 *  @param mask Mask of MPR_READABLE and MPR_WRITEABLE
 *  @ingroup MprWaitHandler
 */
extern void mprSetWaitInterest(MprWaitHandler *wp, int mask);

/**
 *  Define the wait handler callback
 *  @description This updates the callback function for the wait handler. Callback functions are originally specified
 *      via #mprCreateWaitHandler.
 *  @param wp Wait handler created vai #mprCreateWaitHandler
 *  @param proc Callback function to invoke when an I/O event of interest has occurred.
 *  @param mask Mask of MPR_READABLE and MPR_WRITEABLE
 *  @ingroup MprWaitHandler
 */
extern void mprSetWaitCallback(MprWaitHandler *wp, MprWaitProc proc, int mask);

//  TODO - remove either the wp or fd argument
/**
 *  Wait for I/O on an event handler
 *  @description This call will block for a given timeout until I/O of interest occurs on the given 
 *      serviced. Flags may modify the calls behavior.
 *  @param wp Wait handler created vai #mprCreateWaitHandler
 *  @param fd File descriptor to wait on.
 *  @param mask Mask of MPR_READABLE and MPR_WRITEABLE
 *  @param timeout Time in milliseconds to block
 *  @returns Zero on success, otherwise a negative MPR error code.
 *  @ingroup MprWaitHandler
 */
extern int mprWaitForSingleIO(MprWaitHandler *wp, int fd, int mask, int timeout);

/**
 *  Socket I/O callback procedure
 */
typedef void (*MprSocketProc)(void *data, struct MprSocket *sp, int mask, bool isPoolThread);

/**
 *  Socket connection acceptance callback procedure
 */
typedef void (*MprSocketAcceptProc)(void *data, struct MprSocket *sp, cchar *ip, int port);

/*
 *  Socket service provider interface.
 */
typedef struct MprSocketProvider {
    cchar           *name;

#if BLD_FEATURE_SSL
    struct MprSsl   *defaultSsl;
#endif

    struct MprSocket *(*acceptSocket)(struct MprSocket *sp, bool invokeCallback);
    void            (*closeSocket)(struct MprSocket *socket, bool gracefully);
    int             (*configureSsl)(struct MprSsl *ssl);
    int             (*connectSocket)(struct MprSocket *socket, cchar *host, int port, int flags);
    struct MprSocket *(*createSocket)(MprCtx ctx, struct MprSsl *ssl);
    int             (*flushSocket)(struct MprSocket *socket);
    int             (*listenSocket)(struct MprSocket *socket, cchar *host, int port, MprSocketAcceptProc acceptFn, 
                        void *data, int flags);
    int             (*readSocket)(struct MprSocket *socket, void *buf, int len);
    int             (*writeSocket)(struct MprSocket *socket, void *buf, int len);
} MprSocketProvider;


/*
 *  Mpr socket service class
 */
typedef struct MprSocketService {
    int             maxClients;
    int             numClients;

    MprSocketProvider *standardProvider;
    MprSocketProvider *secureProvider;

#if BLD_FEATURE_MULTITHREAD
    MprMutex        *mutex;
#endif
} MprSocketService;


extern MprSocketService *mprCreateSocketService(MprCtx ctx);
extern int  mprStartSocketService(MprSocketService *ss);
extern void mprStopSocketService(MprSocketService *ss);
extern int  mprSetMaxSocketClients(MprCtx ctx, int max);
extern void mprSetSecureProvider(MprCtx ctx, MprSocketProvider *provider);
extern bool mprHasSecureSockets(MprCtx ctx);

/*
 *  Socket close flags
 */
#define MPR_SOCKET_GRACEFUL     1           /* Do a graceful shutdown */

/*
 *  Socket event types
 */
#define MPR_SOCKET_READABLE     0x2
#define MPR_SOCKET_WRITABLE     0x4
#define MPR_SOCKET_EXCEPTION    0x8

/*
 *  Socket Flags
 */
#define MPR_SOCKET_BLOCK        0x1         /**< Use blocking I/O */
#define MPR_SOCKET_BROADCAST    0x2         /**< Broadcast mode */
#define MPR_SOCKET_CLOSED       0x4         /**< MprSocket has been closed */
#define MPR_SOCKET_CONNECTING   0x8         /**< MprSocket has been closed */
#define MPR_SOCKET_DATAGRAM     0x10        /**< Use datagrams */
#define MPR_SOCKET_EOF          0x20        /**< Seen end of file */
#define MPR_SOCKET_LISTENER     0x40        /**< MprSocket is server listener */
#define MPR_SOCKET_NOREUSE      0x80        /**< Dont set SO_REUSEADDR option */
#define MPR_SOCKET_NODELAY      0x100       /**< Disable Nagle algorithm */
#define MPR_SOCKET_THREAD       0x400       /**< Process callbacks on a pool thread */
#define MPR_SOCKET_CLIENT       0x800       /**< Socket is a client */


/**
 *  Socket Service
 *  @description The MPR Socket service provides IPv4 and IPv6 capabilities for both client and server endpoints.
 *  Datagrams, Broadcast and point to point services are supported. The APIs can be used in both blocking and
 *  non-blocking modes.
 *  \n\n
 *  The socket service integrates with the MPR thread pool and eventing services. Socket connections can be handled
 *  by threads from the thread pool for scalable, multithreaded applications.
 *
 *  @stability Evolving
 *  @see MprSocket, mprCreateSocket, mprOpenClientSocket, mprOpenServerSocket, mprCloseSocket, mprFree, mprFlushSocket,
 *      mprWriteSocket, mprWriteSocketString, mprReadSocket, mprSetSocketCallback, mprSetSocketEventMask, 
 *      mprGetSocketBlockingMode, mprGetSocketEof, mprGetSocketFd, mprGetSocketPort, mprGetSocketBlockingMode, 
 *      mprSetSocketNoDelay, mprGetSocketError, mprParseIp, mprSendFileToSocket, mprSetSocketEof, mprSocketIsSecure
 *      mprWriteSocketVector
 *  @defgroup MprSocket MprSocket
 */
typedef struct MprSocket {
    MprSocketService *service;          /**< Socket service */
    MprSocketAcceptProc
                    acceptCallback;     /**< Accept callback */
    void            *acceptData;        /**< User accept callback data */
    int             currentEvents;      /**< Mask of ready events (FD_x) */
    int             error;              /**< Last error */
    int             handlerMask;        /**< Handler events of interest */
    int             handlerPriority;    /**< Handler priority */
    int             interestEvents;     /**< Mask of events to watch for */
    MprSocketProc   ioCallback;         /**< User I/O callback */
    void            *ioData;            /**< User io callback data */
    void            *ioData2;           /**< Secondary user io callback data */
    char            *ipAddr;            /**< Server side ip address */
    char            *clientIpAddr;      /**< Client side ip address */
    int             port;               /**< Port to listen on */
    int             waitForEvents;      /**< Events being waited on */

#if BLD_FEATURE_MULTITHREAD
    MprMutex        *mutex;             /**< Multi-thread sync */
#endif

    MprWaitHandler  *handler;           /**< Wait handler */
    int             fd;                 /**< Actual socket file handle */
    int             flags;              /**< Current state flags */

    MprSocketProvider *provider;        /**< Socket implementation provider */
    struct MprSocket *listenSock;       /**< Listening socket */

    struct MprSslSocket *sslSocket;     /**< Extended ssl socket state. If set, then using ssl */
    struct MprSsl   *ssl;               /**< SSL configuration */
} MprSocket;


/*
 *  Vectored write array
 */
typedef struct MprIOVec {
    char            *start;
    size_t          len;
} MprIOVec;


/**
 *  Flag for mprCreateSocket to use the default SSL provider
 */ 
#define MPR_SECURE_CLIENT ((struct MprSsl*) 1)

//  TODO - some of these names are not very consistent SocketIsSecure
/**
 *  Create a socket
 *  @description Create a new socket
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @param ssl An optional SSL context if the socket is to support SSL. Use the #MPR_SECURE_CLIENT define to specify
 *      that mprCreateSocket should use the default SSL provider.
 *  @return A new socket object
 *  @ingroup MprSocket
 */
extern MprSocket *mprCreateSocket(MprCtx ctx, struct MprSsl *ssl);

/**
 *  Open a client socket
 *  @description Open a client connection
 *  @param sp Socket object returned via #mprCreateSocket
 *  @param hostName Host or IP address to connect to.
 *  @param port TCP/IP port number to connect to.
 *  @param flags Socket flags may use the following flags ored together:
 *      @li MPR_SOCKET_BLOCK - to use blocking I/O. The default is non-blocking.
 *      @li MPR_SOCKET_BROADCAST - Use IPv4 broadcast
 *      @li MPR_SOCKET_DATAGRAM - Use IPv4 datagrams
 *      @li MPR_SOCKET_NOREUSE - Set NOREUSE flag on the socket
 *      @li MPR_SOCKET_NODELAY - Set NODELAY on the socket
 *      @li MPR_SOCKET_THREAD - Process callbacks on a separate thread.
 *  @return Zero if the connection is successful. Otherwise a negative MPR error code.
 *  @ingroup MprSocket
 */
extern int mprOpenClientSocket(MprSocket *sp, cchar *hostName, int port, int flags);

/**
 *  Open a server socket
 *  @description Open a server socket and listen for client connections.
 *  @param sp Socket object returned via #mprCreateSocket
 *  @param ipAddr IP address to bind to. Set to 0.0.0.0 to bind to all possible addresses on a given port.
 *  @param port TCP/IP port number to connect to. 
 *  @param acceptFn Callback function to invoke to accept incoming client connections.
 *  @param data Opaque data reference to pass to the accept function.
 *  @param flags Socket flags may use the following flags ored together:
 *      @li MPR_SOCKET_BLOCK - to use blocking I/O. The default is non-blocking.
 *      @li MPR_SOCKET_BROADCAST - Use IPv4 broadcast
 *      @li MPR_SOCKET_DATAGRAM - Use IPv4 datagrams
 *      @li MPR_SOCKET_NOREUSE - Set NOREUSE flag on the socket
 *      @li MPR_SOCKET_NODELAY - Set NODELAY on the socket
 *      @li MPR_SOCKET_THREAD - Process callbacks on a separate thread.
 *  @return Zero if the connection is successful. Otherwise a negative MPR error code.
 *  @ingroup MprSocket
 */
extern int mprOpenServerSocket(MprSocket *sp, cchar *ipAddr, int port, MprSocketAcceptProc acceptFn, void *data, int flags);

/**
 *  Close a socket
 *  @description Close a socket. If the \a graceful option is true, the socket will first wait for written data to drain
 *      before doing a graceful close.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @param graceful Set to true to do a graceful close. Otherwise, an abortive close will be performed.
 *  @ingroup MprSocket
 */
extern void mprCloseSocket(MprSocket *sp, bool graceful);

/**
 *  Flush a socket
 *  @description Flush any buffered data in a socket. Standard sockets do not use buffering and this call will do nothing.
 *      SSL sockets do buffer and calling mprFlushSocket will write pending written data.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @return A count of bytes actually written. Return a negative MPR error code on errors.
 *  @ingroup MprSocket
 */
extern int mprFlushSocket(MprSocket *sp);


/**
 *  Write to a socket
 *  @description Write a block of data to a socket. If the socket is in non-blocking mode (the default), the write
 *      may return having written less than the required bytes. 
 *  @param sp Socket object returned from #mprCreateSocket
 *  @param buf Reference to a block to write to the socket
 *  @param len Length of data to write. This may be less than the requested write length if the socket is in non-blocking
 *      mode. Will return a negative MPR error code on errors.
 *  @return A count of bytes actually written. Return a negative MPR error code on errors.
 *  @ingroup MprSocket
 */
extern int mprWriteSocket(MprSocket *sp, void *buf, int len);

/**
 *  Write to a string to a socket
 *  @description Write a string  to a socket. If the socket is in non-blocking mode (the default), the write
 *      may return having written less than the required bytes. 
 *  @param sp Socket object returned from #mprCreateSocket
 *  @param str Null terminated string to write.
 *  @return A count of bytes actually written. Return a negative MPR error code on errors.
 *  @ingroup MprSocket
 */
extern int mprWriteSocketString(MprSocket *sp, cchar *str);

/**
 *  Read from a socket
 *  @description Read data from a socket. The read will return with whatever bytes are available. If none and the socket
 *      is in blocking mode, it will block untill there is some data available or the socket is disconnected.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @param buf Pointer to a buffer to hold the read data. 
 *  @param size Size of the buffer.
 *  @return A count of bytes actually read. Return a negative MPR error code on errors.
 *  @ingroup MprSocket
 */
extern int mprReadSocket(MprSocket *sp, void *buf, int size);

/**
 *  Set the socket callback.
 *  @description Define a socket callback function to invoke in response to socket I/O events.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @param fn Callback function.
 *  @param data Data to pass with the callback.
 *  @param data2 More data to pass with the callback.
 *  @param mask Bit mask of events of interest. Set to MPR_READABLE and/or MPR_WRITABLE.
 *  @param priority Priority to associate with the event. Priorities are integer values between 0 and 100 inclusive with
 *      50 being a normal priority. (See #MPR_NORMAL_PRIORITY).
 *  @ingroup MprSocket
 */
extern void mprSetSocketCallback(MprSocket *sp, MprSocketProc fn, void *data, void *data2, int mask, int priority);

/**
 *  Define the events of interest for a socket
 *  @description Define an event mask of interest for a socket. The mask is made by oring the MPR_READABLE and MPR_WRITEABLE
 *      flags as requried
 *  @param sp Socket object returned from #mprCreateSocket
 *  @param mask Set to true to do a graceful close. Otherwise, an abortive close will be performed.
 *  @ingroup MprSocket
 */
extern void mprSetSocketEventMask(MprSocket *sp, int mask);

/**
 *  Get the socket blocking mode.
 *  @description Return the current blocking mode setting.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @return True if the socket is in blocking mode. Otherwise false.
 *  @ingroup MprSocket
 */
extern bool mprGetSocketBlockingMode(MprSocket *sp);

/**
 *  Test if the other end of the socket has been closed.
 *  @description Determine if the other end of the socket has been closed and the socket is at end-of-file.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @return True if the socket is at end-of-file.
 *  @ingroup MprSocket
 */
extern bool mprGetSocketEof(MprSocket *sp);

/**
 *  Get the socket file descriptor.
 *  @description Get the file descriptor associated with a socket.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @return The integer file descriptor used by the O/S for the socket.
 *  @ingroup MprSocket
 */
extern int mprGetSocketFd(MprSocket *sp);

/**
 *  Get the port used by a socket
 *  @description Get the TCP/IP port number used by the socket.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @return The integer TCP/IP port number used by the socket.
 *  @ingroup MprSocket
 */
extern int mprGetSocketPort(MprSocket *sp);

/**
 *  Set the socket blocking mode.
 *  @description Set the blocking mode for a socket. By default a socket is in non-blocking mode where read / write
 *      calls will not block.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @param on Set to zero to put the socket into non-blocking mode. Set to non-zero to enable blocking mode.
 *  @return The old blocking mode if successful or a negative MPR error code.
 *  @ingroup MprSocket
 */
extern int mprSetSocketBlockingMode(MprSocket *sp, bool on);

/**
 *  Set the socket delay mode.
 *  @description Set the socket delay behavior (nagle algorithm). By default a socket will partial packet writes
 *      a little to try to accumulate data and coalesce TCP/IP packages. Setting the delay mode to false may
 *      result in higher performance for interactive applications.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @param on Set to non-zero to put the socket into no delay mode. Set to zero to enable the nagle algorithm.
 *  @return The old delay mode if successful or a negative MPR error code.
 *  @ingroup MprSocket
 */
extern int mprSetSocketNoDelay(MprSocket *sp, bool on);

/**
 *  Get a socket error code
 *  @description This will map a Windows socket error code into a posix error code.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @return A posix error code. 
 *  @ingroup MprSocket
 */
extern int mprGetSocketError(MprSocket *sp);

//  TODO reverse file and sock args
/**
 *  Send a file to a socket
 *  @description Write the contents of a file to a socket. If the socket is in non-blocking mode (the default), the write
 *      may return having written less than the required bytes. This API permits the writing of data before and after
 *      the file contents. 
 *  @param file File to write to the socket
 *  @param sock Socket object returned from #mprCreateSocket
 *  @param offset offset within the file from which to read data
 *  @param bytes Length of file data to write
 *  @param beforeVec Vector of data to write before the file contents
 *  @param beforeCount Count of entries in beforeVect
 *  @param afterVec Vector of data to write after the file contents
 *  @param afterCount Count of entries in afterCount
 *  @return A count of bytes actually written. Return a negative MPR error code on errors.
 *  @ingroup MprSocket
 */
extern MprOffset mprSendFileToSocket(MprFile *file, MprSocket *sock, MprOffset offset, int bytes, MprIOVec *beforeVec, 
    int beforeCount, MprIOVec *afterVec, int afterCount);

extern void mprSetSocketEof(MprSocket *sp, bool eof);

/**
 *  Determine if the socket is secure
 *  @description Determine if the socket is using SSL to provide enhanced security.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @return True if the socket is using SSL, otherwise zero.
 *  @ingroup MprSocket
 */
extern bool mprSocketIsSecure(MprSocket *sp);

/**
 *  Write a vector to a socket
 *  @description Do scatter/gather I/O by writing a vector of buffers to a socket.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @param iovec Vector of data to write before the file contents
 *  @param count Count of entries in beforeVect
 *  @return A count of bytes actually written. Return a negative MPR error code on errors.
 *  @ingroup MprSocket
 */
extern int mprWriteSocketVector(MprSocket *sp, MprIOVec *iovec, int count);

extern int mprParseIp(MprCtx ctx, cchar *ipSpec, char **ipAddrRef, int *port, int defaultPort);

#if BLD_FEATURE_SSL
/*
 *  Put these here to reduce namespace clutter, so users who want SSL don't have to include mprSsl.h and thus 
 *  pull in ssl headers.
 */
extern MprModule   *mprLoadSsl(MprCtx ctx, bool lazy);
extern void         mprConfigureSsl(struct MprSsl *ssl);
#endif

#if BLD_FEATURE_MULTITHREAD

#if BLD_DEBUG
//  TODO - rename MprThreadPoolStats
typedef struct MprPoolStats {
    int             maxThreads;         /* Configured max number of threads */
    int             minThreads;         /* Configured minimum */
    int             numThreads;         /* Configured minimum */
    int             maxUse;             /* Max used */
    int             pruneHighWater;     /* Peak thread use in last minute */
    int             idleThreads;        /* Current idle */
    int             busyThreads;        /* Current busy */
} MprPoolStats;
#endif


//  TODO - rename MprThreadPoolService
/**
 *  Thread Pool Service
 *  @description The MPR provides a thread pool for rapid starting and assignment of threads to tasks.
 *  @stability Evolving
 *  @see MprPoolService, mprAvailablePoolThreads, mprSetMaxPoolThreads, mprSetMinPoolThreads
 *  @defgroup MprPoolService MprPoolService
 */
typedef struct MprPoolService {
    int             nextTaskNum;        /* Unique next task number */
    MprList         *runningTasks;      /* List of executing tasks */
    int             stackSize;          /* Stack size for worker threads */
    MprList         *tasks;             /* Prioritized list of pending tasks */

    MprList         *busyThreads;       /* List of threads to service tasks */
    MprList         *idleThreads;       /* List of threads to service tasks */
    int             maxThreads;         /* Max # threads in pool */
    int             maxUseThreads;      /* Max threads ever used */
    int             minThreads;         /* Max # threads in pool */
    MprMutex        *mutex;             /* Per task synchronization */
    int             nextThreadNum;      /* Unique next thread number */
    int             numThreads;         /* Current number of threads in pool */
    int             pruneHighWater;     /* Peak thread use in last minute */
    struct MprEvent *pruneTimer;        /* Timer for excess threads pruner */
} MprPoolService;


extern MprPoolService *mprCreatePoolService(MprCtx ctx);
extern int mprStartPoolService(MprPoolService *ps);
extern void mprStopPoolService(MprPoolService *ps, int timeout);

//  TODO - rename mprGetAvailablePoolThreads
/**
 *  Get the count of available pool threads
 *  Return the count of free threads in the thread pool.
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @returns An integer count of pool threads.
 *  @ingroup MprPoolService
 */
extern int  mprAvailablePoolThreads(MprCtx ctx);

extern void mprSetPoolThreadStackSize(MprCtx ctx, int n);

/**
 *  Set the minimum count of pool threads
 *  Set the count of threads the pool will have. This will cause the pool to pre-create at least this many threads.
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @param count Minimum count of threads to use.
 *  @ingroup MprPoolService
 */
extern void mprSetMinPoolThreads(MprCtx ctx, int count);

/**
 *  Set the maximum count of pool threads
 *  Set the maximum number of pool threads for the MPR. If this number if less than the current number of threads,
 *      excess threads will be gracefully pruned as they exit.
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @param count Maximum limit of threads to define.
 *  @ingroup MprPoolService
 */
extern void mprSetMaxPoolThreads(MprCtx ctx, int count);

/**
 *  Get the maximum count of pool threads
 *  Get the maximum limit of pool threads. 
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @return The maximum count of pool threads.
 *  @ingroup MprPoolService
 */
extern int mprGetMaxPoolThreads(MprCtx ctx);

#if BLD_DEBUG
extern void mprGetPoolServiceStats(MprPoolService *ps, MprPoolStats *stats);
#endif

/*
 *  State
 */
#define MPR_POOL_THREAD_SLEEPING    0x1
#define MPR_POOL_THREAD_IDLE        0x2
#define MPR_POOL_THREAD_BUSY        0x4
#define MPR_POOL_THREAD_PRUNED      0x8

typedef void        (*MprPoolProc)(void *data, struct MprPoolThread *tp);

/*
 *  Threads in the thread pool
 */
typedef struct MprPoolThread {
    MprPoolProc     proc;               /* Procedure to run */
    void            *data;
    int             priority;
    int             state;
    MprPoolService  *pool;              /* Pool service */

    struct MprThread *thread;           /* Associated thread */
    MprCond         *idleCond;          /* Used to wait for work */
} MprPoolThread;


extern int mprStartPoolThread(MprCtx ctx, MprPoolProc proc, void *data, int priority);

#endif /* BLD_FEATURE_MULTITHREAD */



extern int  mprDecode64(char *buffer, int bufsize, cchar *str);
extern void mprEncode64(char *buffer, int bufsize, cchar *str);
extern char *mprGetMD5Hash(MprCtx ctx, uchar *buf, int len, cchar *prefix);
extern int  mprCalcDigestNonce(MprCtx ctx, char **nonce, cchar *secret, cchar *etag, cchar *realm);
extern int  mprCalcDigest(MprCtx ctx, char **digest, cchar *userName, cchar *password, cchar *realm,
                cchar *uri, cchar *nonce, cchar *qop, cchar *nc, cchar *cnonce, cchar *method);

/**
 *  URI management
 *  @description The MPR provides routines for formatting and parsing URIs. Routines are also provided
 *      to escape dangerous characters for URIs as well as HTML content and shell commands.
 *  @stability Evolving
 *  @see MprHttp, mprFormatUri, mprEscapeCmd, mprEscapeHtml, mprUrlEncode, mprUrlDecode, mprValidateUrl
 *  @defgroup MprUri MprUri
 */
typedef struct MprUri {
    char        *originalUri;           /**< Original URI */
    char        *parsedUriBuf;          /**< Allocated storage for parsed uri */

    /*
     *  These are just pointers into the parsedUriBuf. None of these fields are Url decoded.
     */
    char        *scheme;                /**< URI scheme (http|https|...) */
    char        *host;                  /**< Url host name */
    int         port;                   /**< Port number */
    char        *url;                   /**< Url path name (without scheme, host, query or fragements) */
    char        *ext;                   /**< Document extension */
    char        *query;                 /**< Query string */
    bool        secure;                 /**< Using https */
} MprUri;


/*
 *  Character escaping masks
 */
#define MPR_HTTP_ESCAPE_HTML            0x1
#define MPR_HTTP_ESCAPE_SHELL           0x2
#define MPR_HTTP_ESCAPE_URL             0x4


/**
 *  Parse a URI
 *  @description Parse a uri and return a tokenized MprUri structure.
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @param uri Uri string to parse
 *  @return A newly allocated MprUri structure. Caller must free using #mprFree.
 *  @ingroup MprUri
 */
extern MprUri *mprParseUri(MprCtx ctx, cchar *uri);

/**
 *  Format a URI
 *  @description Format a URI string using the input components.
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @param protocol Protocol string for the uri. Example: "http"
 *  @param host Host or IP address
 *  @param port TCP/IP port number
 *  @param path URL path
 *  @param query Additiona query parameters.
 *  @return A newly allocated uri string. Caller must free using #mprFree.
 *  @ingroup MprUri
 */
extern char *mprFormatUri(MprCtx ctx, cchar *protocol, cchar *host, int port, cchar *path, cchar *query);

/**
 *  Encode a string escaping typical command (shell) characters
 *  @description Encode a string escaping all dangerous characters that have meaning for the unix or MS-DOS command shells.
 *  @param buf Buffer to hold the encoded string.
 *  @param len Length of the buffer to hold the encoded string.
 *  @param cmd Command string to encode
 *  @param escChar Escape character to use when encoding the command.
 *  @return A reference to the buf argument.
 *  @ingroup MprUri
 */
extern char *mprEscapeCmd(char *buf, int len, cchar *cmd, int escChar);

/**
 *  Encode a string by escaping typical HTML characters
 *  @description Encode a string escaping all dangerous characters that have meaning in HTML documents
 *  @param buf Buffer to hold the encoded string.
 *  @param len Length of the buffer to hold the encoded string.
 *  @param html HTML content to encode
 *  @return A reference to the buf argument.
 *  @ingroup MprUri
 */
extern char *mprEscapeHtml(char *buf, int len, cchar *html);

/**
 *  Encode a string by escaping URL characters
 *  @description Encode a string escaping all characters that have meaning for URLs.
 *  @param buf Buffer to hold the encoded string.
 *  @param len Length of the buffer to hold the encoded string.
 *  @param url URL to encode
 *  @return A reference to the buf argument.
 *  @ingroup MprUri
 */
extern char *mprUrlEncode(char *buf, int len, cchar *url);

/**
 *  Decode a URL string by de-scaping URL characters
 *  @description Decode a string with www-encoded characters that have meaning for URLs.
 *  @param url Buffer to hold the decoded string.
 *  @param urlSize Length of the buffer to hold the decoded string.
 *  @param buf URL to decode
 *  @return A reference to the buf argument.
 *  @ingroup MprUri
 */
extern char *mprUrlDecode(char *url, int urlSize, cchar *buf);

//  TODO - should not operate in-situ.
/**
 *  Validate a URL
 *  @description Validate and canonicalize a URL. This removes redundant "./" sequences and simplifies "../dir" 
 *      references. This operates in-situ and modifies the existing string.
 *  @param url Url string to validate
 *  @return A reference to the original url.
 *  @ingroup MprUri
 */
extern char *mprValidateUrl(char *url);

#if BLD_FEATURE_HTTP

#define MPR_HTTP_NAME                   "Embedthis-http/" BLD_VERSION

/*
 *  Standard HTTP/1.1 response codes. See url.c for the actual strings used for each code.
 */
#define MPR_HTTP_CODE_CONTINUE                  100
#define MPR_HTTP_CODE_OK                        200
#define MPR_HTTP_CODE_CREATED                   201
#define MPR_HTTP_CODE_ACCEPTED                  202
#define MPR_HTTP_CODE_NOT_AUTHORITATIVE         203
#define MPR_HTTP_CODE_NO_CONTENT                204
#define MPR_HTTP_CODE_RESET                     205
#define MPR_HTTP_CODE_PARTIAL                   206
#define MPR_HTTP_CODE_MOVED_PERMANENTLY         301
#define MPR_HTTP_CODE_MOVED_TEMPORARILY         302
#define MPR_HTTP_CODE_NOT_MODIFIED              304
#define MPR_HTTP_CODE_USE_PROXY                 305
#define MPR_HTTP_CODE_TEMPORARY_REDIRECT        307
#define MPR_HTTP_CODE_BAD_REQUEST               400
#define MPR_HTTP_CODE_UNAUTHORIZED              401
#define MPR_HTTP_CODE_PAYMENT_REQUIRED          402
#define MPR_HTTP_CODE_FORBIDDEN                 403
#define MPR_HTTP_CODE_NOT_FOUND                 404
#define MPR_HTTP_CODE_BAD_METHOD                405
#define MPR_HTTP_CODE_NOT_ACCEPTABLE            406
#define MPR_HTTP_CODE_REQUEST_TIME_OUT          408
#define MPR_HTTP_CODE_CONFLICT                  409
#define MPR_HTTP_CODE_GONE                      410
#define MPR_HTTP_CODE_LENGTH_REQUIRED           411
#define MPR_HTTP_CODE_REQUEST_TOO_LARGE         413
#define MPR_HTTP_CODE_REQUEST_URL_TOO_LARGE     414
#define MPR_HTTP_CODE_UNSUPPORTED_MEDIA_TYPE    415
#define MPR_HTTP_CODE_RANGE_NOT_SATISFIABLE     416
#define MPR_HTTP_CODE_EXPECTATION_FAILED        417
#define MPR_HTTP_CODE_INTERNAL_SERVER_ERROR     500
#define MPR_HTTP_CODE_NOT_IMPLEMENTED           501
#define MPR_HTTP_CODE_BAD_GATEWAY               502
#define MPR_HTTP_CODE_SERVICE_UNAVAILABLE       503
#define MPR_HTTP_CODE_GATEWAY_TIME_OUT          504
#define MPR_HTTP_CODE_BAD_VERSION               505
#define MPR_HTTP_CODE_INSUFFICIENT_STORAGE      507

/*
 *  Proprietary HTTP codes.
 */
#define MPR_HTTP_CODE_COMMS_ERROR               550
#define MPR_HTTP_CODE_CLIENT_ERROR              551

/*
 *  Overall HTTP service
 */
typedef struct MprHttpService {
    MprHashTable    *codes;                                 /* Http response code hash */
    char            *secret;                                /* Random bytes to use in authentication */
#if BLD_FEATURE_MULTITHREAD
    MprMutex        *mutex;                                 /* Mutli-thread sync */
#endif
} MprHttpService;

extern MprHttpService *mprCreateHttpService(MprCtx ctx);
extern int      mprStartHttpService(MprHttpService *hs);
extern int      mprStopHttpService(MprHttpService *hs);

/*
 *  Request states
 */
#define MPR_HTTP_STATE_BEGIN            1                   /* Ready for a new request */
#define MPR_HTTP_STATE_WAIT             2                   /* Waiting for the response */
#define MPR_HTTP_STATE_CONTENT          3                   /* Reading posted content */
#define MPR_HTTP_STATE_CHUNK            4                   /* Reading chunk length */
#define MPR_HTTP_STATE_PROCESSING       5                   /* Reading posted content */
#define MPR_HTTP_STATE_COMPLETE         6                   /* Processing complete */

/*
 *  HTTP protocol versions
 */
#define MPR_HTTP_1_0                    0                   /* HTTP/1.0 */
#define MPR_HTTP_1_1                    1                   /* HTTP/1.1 */

/**
 *  Get the Http reponse code as a string
 *  @description Get the Http response code as a string.
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @param code Http status code
 *  @return A reference to the response code string. Callers must not free this reference.
 *  @ingroup MprHttp
 */
extern cchar *mprGetHttpCodeString(MprCtx ctx, int code);

#endif /* BLD_FEATURE_HTTP */



#if BLD_FEATURE_HTTP_CLIENT
/*
 *  Callback flags
 */
#define MPR_HTTP_CALL_POST_DATA         1                   /* Time for caller to supply post data */
#define MPR_HTTP_CALL_RESPONSE_DATA     2                   /* Progressive reading of response data */
#define MPR_HTTP_CALL_COMPLETE          3                   /* Request is complete */

/*
 *  mprHttpRequst flags
 */
#define MPR_HTTP_DONT_BLOCK             0x1                 /**< Don't wait for a response */

/**
 *  Http callback procedure
 *  @param http Http object created via #mprCreateHttp
 *  @param nbytes Number of bytes read
 */
typedef void        (*MprHttpProc)(struct MprHttp *http, int nbytes);


/**
 *  HTTP Per-request structure
 */
typedef struct MprHttpRequest {
    struct MprHttp  *http;              /**< Reference to Http service object */
    char            *method;            /**< Request method GET, HEAD, POST, DELETE, OPTIONS, PUT, TRACE */
    MprUri          *uri;               /**< Request uri */
    MprHashTable    *headers;           /**< Headers keyword values */
    MprBuf          *outBuf;            /**< Request output buffer */
    char            *uploadFilename;    /**< Upload filename */
    char            *bodyData;          /**< Form post data */
    int             bodyLen;            /**< Length of bodyData */
    char            *formData;          /**< Form post data */
    int             formLen;            /**< Length of formData */
    int             sentCredentials;    /**< Credentials sent with request */
} MprHttpRequest;


/*
 *  Response flags
 */
#define MPR_HTTP_RESP_COMPLETE          0x4     /* Request complete */
#define MPR_HTTP_RESP_INPUT_CHUNKED     0x8     /* Using HTTP/1.1 chunked */
#define MPR_HTTP_RESP_BUFFER_SIZED      0x10    /* Input buffer has been resized */

/**
 *  HTTP Per-response structure
 */
typedef struct MprHttpResponse {
    struct MprHttp  *http;              /**< Reference to Http service object */

    int             length;             /**< Actual length of content received */
    int             contentLength;      /**< Content length header */
    int             contentRemaining;   /**< Remaining content data to read */
    int             chunkRemaining;     /**< Remaining content data to read in this chunk */

    MprHashTable    *headers;           /**< Headers keyword values */
    MprBuf          *content;           /**< Response content data */

    int             code;               /**< Http response status code */
    char            *message;           /**< Http response status message */
    char            *protocol;          /**< Response protocol "HTTP/1.0" or "HTTP/1.1" */
    char            *location;          /**< Redirect location */
    char            *authAlgorithm;     /**< Authentication algorithm */
    char            *authStale;         /**< Stale handshake value */
    int             flags;              /**< Control flags */
} MprHttpResponse;


/**
 *  Http per-connection structure. 
 *  @description The HTTP service provides a Http client with optional SSL capabilities. It supports 
 *      response chunking and ranged requests.
 *  @stability Prototype.
 *  @see mprCreateHttp mprCreateHttpSecret mprDisconnectHttp mprGetHttpFlags mprGetHttpState mprGetHttpDefaultHost
 *      mprGetHttpDefaultPort mprGetHttpCodeString mprGetHttpCode mprGetHttpMessage mprGetHttpContentLength
 *      mprGetHttpContent mprGetHttpError mprGetHttpHeader mprGetHttpHeaders mprGetHttpHeadersHash mprHttpRequest
 *      mprResetHttpCredentials mprSetHttpBody mprSetHttpCallback mprSetHttpCredentials mprSetHttpBufferSize
 *      mprSetHttpDefaultHost mprSetHttpDefaultPort mprSetHttpFollowRedirects mprSetHttpForm mprAddHttpFormItem
 *      mprSetHttpHeader mprSetHttpKeepAlive mprSetHttpProtocol mprSetHttpProxy mprSetHttpRetries mprSetHttpTimeout
 *      mprSetHttpUpload mprWriteHttpBody
 *  @defgroup MprHttp MprHttp
 */
typedef struct MprHttp {
    MprHttpService  *httpService;

    MprHttpRequest  *request;           /**< Request object */
    MprHttpResponse *response;          /**< Response object */
    MprSocket       *sock;              /**< Underlying socket handle */
    MprBuf          *headerBuf;         /**< Header buffer */

    int             state;              /**< Connection state  */
    int             userFlags;          /**< User flags to mprHttpRequest */

    char            *currentHost;       /**< Current connection host */
    int             currentPort;        /**< Current connection port */

    char            *protocol;          /**< HTTP protocol to use */
    char            *defaultHost;       /**< Default target host (if unspecified) */
    char            *proxyHost;         /**< Proxy host to connect via */
    int             defaultPort;        /**< Default target port (if unspecified) */
    int             proxyPort;          /**< Proxy port to connect via */

    MprTime         timestamp;          /**< Timeout timestamp for last I/O  */
    MprEvent        *timer;             /**< Timeout event handle  */
    int             timeoutPeriod;      /**< Timeout value */
    int             retries;            /**< Max number of retry attempts */

    MprHttpProc     callback;           /**< Response callback structure  */
    void            *callbackArg;       /**< Argument to callback  */

    /*
     *  Auth details
     */
    char            *authCnonce;        /**< Digest authentication cnonce value */
    char            *authDomain;        /**< Authentication domain */
    char            *authNonce;         /**< Nonce value used in digest authentication */
    int             authNc;             /**< Digest authentication nc value */
    char            *authOpaque;        /**< Opaque value used to calculate digest session */
    char            *authRealm;         /**< Authentication realm */
    char            *authQop;           /**< Digest authentication qop value */
    char            *authType;          /**< Basic or Digest */
    char            *password;          /**< As the name says */
    char            *user;              /**< User account name */

    char            *error;             /**< Error message if failure  */
    char            *userHeaders;       /**< User headers */

    bool            useKeepAlive;       /**< Use connection keep-alive for all connections */
    bool            keepAlive;          /**< Use keep-alive for this connection */
    bool            followRedirects;    /**< Follow redirects */

    int             bufsize;            /**< Initial buffer size */
    int             bufmax;             /**< Maximum buffer size. -1 is no max */
    int             protocolVersion;    /**< HTTP protocol version to request */

#if BLD_FEATURE_MULTITHREAD
    MprCond         *completeCond;      /**< Signalled when request is complete */
    MprMutex        *mutex;             /**< Mutli-thread sync */
#endif
} MprHttp;



/**
 *  Create a Http connection object
 *  @description Create a new http connection object. This creates an object that can be initialized and then
 *      used with mprHttpRequest
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @return A newly allocated MprHttp structure. Caller must free using #mprFree.
 *  @ingroup MprHttp
 */
extern MprHttp *mprCreateHttp(MprCtx ctx);

/**
 *  Create a Http secret
 *  @description Create http secret data that is used to seed SSL based communications.
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @return Zero if successful, otherwise a negative MPR error code
 *  @ingroup MprHttp
 */
extern int mprCreateHttpSecret(MprCtx ctx);

/**
 *  Disconnect a Http connection
 *  @description Disconned any keep-alive connection associated with this http object.
 *  @param http Http object created via #mprCreateHttp
 *  @ingroup MprHttp
 */
extern void mprDisconnectHttp(MprHttp *http);

/**
 *  Return the http flags
 *  @description Get the current http flags. The valid flags are:
 *      @li MPR_HTTP_DONT_BLOCK  - For non-blocking connections
 *  @param http Http object created via #mprCreateHttp
 *  @return Zero if successful, otherwise a negative MPR error code
 *  @ingroup MprHttp
 */
extern int mprGetHttpFlags(MprHttp *http);

/**
 *  Get the http state
 *  @description Get the current http state. The valid state are:
 *      @li  MPR_HTTP_STATE_BEGIN            - Ready for a new request
 *      @li  MPR_HTTP_STATE_WAIT             - Waiting for the response
 *      @li  MPR_HTTP_STATE_CONTENT          - Reading posted content
 *      @li  MPR_HTTP_STATE_CHUNK            - Reading chunk length
 *      @li  MPR_HTTP_STATE_PROCESSING       - Reading posted content
 *      @li  MPR_HTTP_STATE_COMPLETE         - Processing complete
 *  @param http Http object created via #mprCreateHttp
 *  @return Zero if successful, otherwise a negative MPR error code
 *  @ingroup MprHttp
 */
extern int mprGetHttpState(MprHttp *http);

/**
 *  Get the default host
 *  @description A default host can be defined which will be used for URIs that omit a host specification.
 *  @param http Http object created via #mprCreateHttp
 *  @return A reference to the default host string. Callers must not free this reference.
 *  @ingroup MprHttp
 */
extern cchar *mprGetHttpDefaultHost(MprHttp *http);

/**
 *  Get the default port
 *  @description A default port can be defined which will be used for URIs that omit a host:port specification.
 *  @param http Http object created via #mprCreateHttp
 *  @return The default port number
 *  @ingroup MprHttp
 */
extern int mprGetHttpDefaultPort(MprHttp *http);

/**
 *  Get the Http response code
 *  @description Get the http response code for the last request.
 *  @param http Http object created via #mprCreateHttp
 *  @return An integer Http response code. Typically 200 is success.
 *  @ingroup MprHttp
 */
extern int mprGetHttpCode(MprHttp *http);


/**
 *  Get the Http response message
 *  @description Get the Http response message supplied on the first line of the Http response.
 *  @param http Http object created via #mprCreateHttp
 *  @return A reference to the response message string. Callers must not free this reference.
 *  @ingroup MprHttp
 */
extern cchar *mprGetHttpMessage(MprHttp *http);

/**
 *  Get the response content length
 *  @description Get the length of the response content (if any)
 *  @param http Http object created via #mprCreateHttp
 *  @return A count of the response content data in bytes.
 *  @ingroup MprHttp
 */
extern int mprGetHttpContentLength(MprHttp *http);

/**
 *  Get the response contenat
 *  @description Get the response content as a string
 *  @param http Http object created via #mprCreateHttp
 *  @return A reference to the response content. This is a reference into the Http content buffering. This call cannot
 *      be used if a callback has been configured on the Http response object. In that case, it is the callers
 *      responsibility to save the response content during each callback invocation.
 *  @ingroup MprHttp
 */
extern cchar *mprGetHttpContent(MprHttp *http);

/**
 *  Get the Http error message
 *  @description Get a Http error message. Error messages may be generated for internal or client side errors.
 *  @param http Http object created via #mprCreateHttp
 *  @return A error string. The caller must not free this reference.
 *  @ingroup MprHttp
 */
extern cchar *mprGetHttpError(MprHttp *http);

/**
 *  Get a http response header.
 *  @description Get a http response header for a given header key.
 *  @param http Http object created via #mprCreateHttp
 *  @param key Name of the header to retrieve. This should be a lower case header name. For example: "Connection"
 *  @return Value associated with the header key or null if the key did not exist in the response.
 *  @ingroup MprHttp
 */
extern cchar *mprGetHttpHeader(MprHttp *http, cchar *key);

/**
 *  Get all the http response headers.
 *  @description Get all the response headers. The returned string formats all the headers in the form:
 *      key: value\\nkey2: value2\\n...
 *  @param http Http object created via #mprCreateHttp
 *  @return String containing all the headers. The caller must free this returned string.
 *  @ingroup MprHttp
 */
extern char *mprGetHttpHeaders(MprHttp *http);

/**
 *  Get the hash table of response Http headers
 *  @description Get the internal hash table of response headers
 *  @param http Http object created via #mprCreateHttp
 *  @return Hash table. See MprHash for how to access the hash table.
 *  @ingroup MprHttp
 */
extern MprHashTable *mprGetHttpHeadersHash(MprHttp *http);

/**
 *  Issue a new Http request
 *  @description Issue a new Http request on the http object. 
 *  @param http Http object created via #mprCreateHttp
 *  @param method Http method to use. Valid methods include: "GET", "POST", "PUT", "DELETE", "OPTIONS" and "TRACE" 
 *  @param uri URI to fetch
 *  @param flags Request modification flags. Valid flags are: #MPR_HTTP_DONT_BLOCK for non-blocking I/O
 *  @return Zero if successful, otherwise a negative MPR error code
 *  @ingroup MprHttp
 */
extern int mprHttpRequest(MprHttp *http, cchar *method, cchar *uri, int flags);

/**
 *  Reset Http credential.
 *  @description Reset any previously defined Http credentials on this http object.
 *  @param http Http object created via #mprCreateHttp
 *  @ingroup MprHttp
 */
extern void mprResetHttpCredentials(MprHttp *http);

/**
 *  Set the http request body content
 *  @description Define content to be sent with the Http request. 
 *  @param http Http object created via #mprCreateHttp
 *  @param body Pointer to the body content.
 *  @param len Length of the body content
 *  @return Zero if successful, otherwise a negative MPR error code
 *  @ingroup MprHttp
 */
extern int mprSetHttpBody(MprHttp *http, cchar *body, int len);

/**
 *  Define a Http callback.
 *  @description If define, the http callabck will be invoked as response data or state is received.
 *  @param http Http object created via #mprCreateHttp
 *  @param fn Callback function. 
 *  @param arg Data argument to provide to the callback function.
 *  @ingroup MprHttp
 */
extern void mprSetHttpCallback(MprHttp *http, MprHttpProc fn, void *arg);

/**
 *  Set the Http credentials
 *  @description Define a username and password to use with Http authentication for sites that require it.
 *  @param http Http object created via #mprCreateHttp
 *  @param username String username
 *  @param password Un-encrypted password string
 *  @ingroup MprHttp
 */
extern void mprSetHttpCredentials(MprHttp *http, cchar *username, cchar *password);

/**
 *  Set the Http buffer size.
 *  @description Define an initial and maximum limit for the response content buffering. By default, the 
 *      buffer will grow to accomodate all response data.
 *  @param http Http object created via #mprCreateHttp
 *  @param initialSize Starting size of the response content buffer
 *  @param maxSize Maximum size of the response content buffer.
 *  @ingroup MprHttp
 */
extern void mprSetHttpBufferSize(MprHttp *http, int initialSize, int maxSize);

/**
 *  Define a default host
 *  @description Define a default host to use for connections if the URI does not specify a host
 *  @param http Http object created via #mprCreateHttp
 *  @param host Host or IP address
 *  @ingroup MprHttp
 */
extern void mprSetHttpDefaultHost(MprHttp *http, cchar *host);

/**
 *  Define a default port
 *  @description Define a default port to use for connections if the URI does not define a port
 *  @param http Http object created via #mprCreateHttp
 *  @param port Integer port number
 *  @ingroup MprHttp
 */
extern void mprSetHttpDefaultPort(MprHttp *http, int port);

/**
 *  Follow redirctions
 *  @description Enabling follow redirects enables the Http service to transparently follow 301 and 302 redirections
 *      and fetch the redirected URI.
 *  @param http Http object created via #mprCreateHttp
 *  @param follow Set to true to enable transparent redirections
 *  @ingroup MprHttp
 */
extern void mprSetHttpFollowRedirects(MprHttp *http, bool follow);

/**
 *  Set Http request form content
 *  @description Define request content that is formatted using www-form-urlencoded formatting. This is the 
 *      traditional way to post form data. For example: "name=John+Smith&City=Seattle". Multiple calls may be 
 *      made to this routine and the form data will be catenated.
 *  @param http Http object created via #mprCreateHttp
 *  @param form String containing the encoded form data.
 *  @param len Length of the form data
 *  @return Zero if successful, otherwise a negative MPR error code
 *  @ingroup MprHttp
 */
extern int mprSetHttpForm(MprHttp *http, cchar *form, int len);

/**
 *  Add a form item
 *  @description Add a key/value pair to the request form data.
 *  @param http Http object created via #mprCreateHttp
 *  @param key Name of the form entity to define.
 *  @param value Value of the form entity.
 *  @return Zero if successful, otherwise a negative MPR error code
 *  @ingroup MprHttp
 */
extern int mprAddHttpFormItem(MprHttp *http, cchar *key, cchar *value);

/**
 *  Add a request header
 *  @description Add a Http header to send with the request
 *  @param http Http object created via #mprCreateHttp
 *  @param key Http header key
 *  @param value Http header value
 *  @param overwrite Set to true to overwrite any previous header of the same key. Set to false to allow duplicate
 *      headers of the same key value.
 *  @return Zero if successful, otherwise a negative MPR error code
 *  @ingroup MprHttp
 */
extern int mprSetHttpHeader(MprHttp *http, cchar *key, cchar *value, bool overwrite);

/**
 *  Control Http Keep-Alive
 *  @description Http keep alive means that the TCP/IP connection is preserved accross multiple requests. This
 *      typically means much higher performance and better response. Http keep alive is enabled by default 
 *      for Http/1.1 (the default). Disable keep alive when talking
 *      to old, broken HTTP servers.
 *  @param http Http object created via #mprCreateHttp
 *  @param on Set to true to enable http keep-alive
 *  @return Zero if successful, otherwise a negative MPR error code
 *  @ingroup MprHttp
 */
extern void mprSetHttpKeepAlive(MprHttp *http, bool on);

/**
 *  Set the Http protocol variant.
 *  @description Set the Http protocol variant to use. 
 *  @param http Http object created via #mprCreateHttp
 *  @param protocol  String representing the protocol variant. Valid values are:
 *      @li "HTTP/1.0"
 *      @li "HTTP/1.1"
 *  Use HTTP/1.1 wherever possible.
 *  @ingroup MprHttp
 */
extern void mprSetHttpProtocol(MprHttp *http, cchar *protocol);

/**
 *  Define a Http proxy host
 *  @description Define a http proxy host to communicate via when accessing the net.
 *  @param http Http object created via #mprCreateHttp
 *  @param host Proxy host name or IP address
 *  @param port Proxy host port number.
 *  @ingroup MprHttp
 */
extern void mprSetHttpProxy(MprHttp *http, cchar *host, int port);

/**
 *  Set the Http retry count
 *  @description Define the number of retries before failing a request. It is normative for network errors
 *      to require that requests be sometimes retried. The default retries is set to (2).
 *  @param http Http object created via #mprCreateHttp
 *  @param retries Count of retries
 *  @ingroup MprHttp
 */
extern void mprSetHttpRetries(MprHttp *http, int retries);

/**
 *  Set the Http inactivity timeout
 *  @description Define an inactivity timeout after which the Http connection will be closed. 
 *  @param http Http object created via #mprCreateHttp
 *  @param timeout Timeout in milliseconds
 *  @ingroup MprHttp
 */
extern void mprSetHttpTimeout(MprHttp *http, int timeout);

//  TODO - not implemented
extern int mprSetHttpUpload(MprHttp *http, cchar *key, cchar *value);

/**
 *  Write Http request content data
 *  @description Write request content data. This routine is used when a callback has been defined for the 
 *      Http object.
 *  @param http Http object created via #mprCreateHttp
 *  @param buf Pointer to buffer containing the data to write
 *  @param len Length of data to write
 *  @param block Set to true to block while the write is completed. If set to false, the call may return with fewer
 *      bytes written. It is then the callers responsibility to retry the write.
 *  @return Number of bytes successfully written.
 *  @ingroup MprHttp
 */
extern int mprWriteHttpBody(MprHttp *http, cchar *buf, int len, bool block);

#endif /* BLD_FEATURE_HTTP_CLIENT */

/* ********************************* MprCmd ************************************/
#if BLD_FEATURE_CMD

/*
 *  Child status structure. Designed to be async-thread safe.
 */
typedef struct MprCmdChild {
    ulong           pid;                /*  Process ID */
    int             exitStatus;         /*  Exit status */
} MprCmdChild;


#define MPR_CMD_EOF_COUNT       2

/*
 *  Channels for clientFd and serverFd
 */
#define MPR_CMD_STDIN           0       /* Stdout for the client side */
#define MPR_CMD_STDOUT          1       /* Stdin for the client side */
#define MPR_CMD_STDERR          2       /* Stderr for the client side */
#define MPR_CMD_MAX_PIPE        3

/*
 *  Cmd procs must return the number of bytes read or -1 for errors.
 */
struct MprCmd;
typedef void    (*MprCmdProc)(struct MprCmd *cmd, int fd, int channel, void *data);

/*
 *  Flags
 */
#define MPR_CMD_NEW_SESSION     0x1     /* Create a new session on unix */
#define MPR_CMD_SHOW            0x2     /* Show the window of the created process on windows */
#define MPR_CMD_DETACHED        0x4     /* Detach the child process and don't wait */
#define MPR_CMD_IN              0x1000  /* Connect to stdin */
#define MPR_CMD_OUT             0x2000  /* Capture stdout */
#define MPR_CMD_ERR             0x4000  /* Capture stdout */

typedef struct MprCmdFile {
    char            *name;
    int             fd;
    int             clientFd;
#if BLD_WIN_LIKE
    HANDLE          handle;
#endif
} MprCmdFile;


typedef struct MprCmd {
    char            *program;           /* Program path name */
    char            **argv;             /* List of args. Null terminated */
    char            **env;              /* List of environment variables. Null terminated */
    char            *dir;               /* Current working dir for the process */
    int             argc;               /* Count of args in argv */
    int             status;             /* Command exit status */
    int             flags;              /* Control flags (userFlags not here) */
    int             eofCount;           /* Count of end-of-files */
    int             requiredEof;        /* Number of EOFs required for an exit */
    bool            completed;          /* Command is complete */
    MprCmdFile      files[MPR_CMD_MAX_PIPE]; /* Stdin, stdout for the command */
    MprWaitHandler  *handlers[MPR_CMD_MAX_PIPE];
    MprCmdProc      callback;           /* Handler for client output and completion */
    void            *callbackData;
    MprBuf          *stdoutBuf;         /* Standard output from the client */
    MprBuf          *stderrBuf;         /* Standard error output from the client */

    uint64          process;            /* Id/handle of the created process */

    void            *userData;          /* User data storage */
    int             userFlags;          /* User flags storage */

#if WIN
    HANDLE          thread;             /* Handle of the primary thread for the created process */
    char            *command;           /* Windows command line */          
#endif
#if VXWORKS && UNUSED
    MprSelectHandler *handler;
    int             waitFd;             /* Pipe to await child exit */
#endif

#if VXWORKS
    /*
     *  Don't use MprCond so we can build single-threaded and still use MprCmd
     */
    SEM_ID          startCond;          /* Synchronization semaphore for task start */
    SEM_ID          exitCond;           /* Synchronization semaphore for task exit */
#endif

#if BLD_FEATURE_MULTITHREAD
    MprMutex        *mutex;
#endif
} MprCmd;


extern void mprCloseCmdFd(MprCmd *cmd, int channel);
extern MprCmd *mprCreateCmd(MprCtx ctx);
extern void mprDisableCmdEvents(MprCmd *cmd, int channel);
extern void mprEnableCmdEvents(MprCmd *cmd, int channel);
extern int mprGetCmdExitStatus(MprCmd *cmd, int *statusp);
extern int mprGetCmdFd(MprCmd *cmd, int channel);
extern MprBuf *mprGetCmdBuf(MprCmd *cmd, int channel);
extern bool mprIsCmdRunning(MprCmd *cmd);
extern int mprMakeCmdIO(MprCmd *cmd);
extern void mprPollCmd(MprCmd *cmd);
extern int mprReadCmdPipe(MprCmd *cmd, int channel, char *buf, int bufsize);
extern int mprReapCmd(MprCmd *cmd, int timeout);
extern int mprRunCmd(MprCmd *cmd, cchar *command, char **out, char **err, int flags);
extern int mprRunCmdV(MprCmd *cmd, int argc, char **argv, char **out, char **err, int flags);
extern void mprSetCmdCallback(MprCmd *cmd, MprCmdProc callback, void *data);
extern void mprSetCmdDir(MprCmd *cmd, cchar *dir);
extern void mprSetCmdEnv(MprCmd *cmd, cchar **env);
extern int  mprStartCmd(MprCmd *cmd, int argc, char **argv, char **envp, int flags);
extern void mprStopCmd(MprCmd *cmd);
extern int mprWaitForCmd(MprCmd *cmd, int timeout);
extern int mprWriteCmdPipe(MprCmd *cmd, int channel, char *buf, int bufsize);

#endif /* BLD_FEATURE_CMD */

/* *********************************** Mpr ************************************/
/*
 *  Mpr flags
 */
#define MPR_EXITING                 0x1     /* App is exiting */
#define MPR_SERVICE_THREAD          0x4     /* Using a service thread for events */
#define MPR_STOPPED                 0x8     /* Mpr services stopped */
#define MPR_STARTED                 0x10    /* Mpr services started */
#define MPR_SSL_PROVIDER_LOADED     0x20    /* SSL provider loaded */

#define MPR_USER_START_FLAGS        (MPR_SERVICE_THREAD)

/**
 *  Primary MPR application control structure
 *  @description The Mpr structure stores critical application state information and is the root memory allocation
 *      context block. It is used as the MprCtx context for other memory allocations and is thus
 *      the ultimate parent of all allocated memory.
 *  @stability Evolving.
 *  @see mprGetApp, mprCreateEx, mprIsExiting, mprSignalExit, mprTerminate, mprGetKeyValue, mprRemoveKeyValue,
 *      mprSetDebugMode, mprGetErrorMsg, mprGetOsError, mprBreakpoint
 *  @defgroup Mpr Mpr
 */
typedef struct Mpr {
    MprHeap         heap;               /**< Top level memory pool */
    MprHeap         pageHeap;           /**< Heap for arenas and slabs. Always page oriented */

    bool            debugMode;          /**< Run in debug mode (no timers) */
    int             logLevel;           /**< Log trace level */
    MprLogHandler   logHandler;         /**< Current log handler callback */
    void            *logHandlerData;    /**< Handle data for log handler */
    MprHashTable    *table;             /**< TODO - what is this for ? */
    MprHashTable    *timeTokens;        /**< Date/Time parsing tokens */
    char            *name;              /**< Product name */
    char            *title;             /**< Product title */
    char            *version;           /**< Product version */

    int             argc;               /**< Count of command line args */
    char            **argv;             /**< Application command line args */
    char            *domainName;        /**< Domain portion */
    char            *hostName;          /**< Host name (fully qualified name) */
    char            *serverName;        /**< Server name portion (no domain) */

    int             flags;              /**< Processing state */
    int             timezone;           /**< Minutes west of Greenwich */

    bool            caseSensitive;      /**< File name comparisons are case sensitive */
    int             hasEventsThread;    /**< Running an events thread */
    int             pathDelimiter;      /**< Filename path delimiter */
    cchar           *newline;           /**< Newline delimiter for text files */

#if WIN
    char            *cygdrive;          /**< Cygwin drive root */
#endif

    /**<
     *  Service pointers
     */
    struct MprFileService   *fileService;   /**< File system service object */
    struct MprOsService     *osService;     /**< O/S service object */

    struct MprEventService  *eventService;  /**< Event service object */
    struct MprPoolService   *poolService;   /**< Pool service object */
    struct MprWaitService   *waitService;   /**< IO Waiting service object */
    struct MprSocketService *socketService; /**< Socket service object */
#if BLD_FEATURE_HTTP
    struct MprHttpService   *httpService;   /**< HTTP service object */
#endif
#if BLD_FEATURE_CMD
    struct MprCmdService    *cmdService;    /**< Command service object */
#endif

    struct MprModuleService *moduleService; /**< Module service object */

#if BLD_FEATURE_MULTITHREAD
    struct MprThreadService *threadService; /**< Thread service object */

    MprMutex        *mutex;             /**< Thread synchronization */
    MprSpin         *spin;              /**< Quick thread synchronization */
#endif

#if BLD_WIN_LIKE
    long            appInstance;        /**< Application instance (windows) */
#endif

#if BREW
    uint            classId;            /**< Brew class ID */
    IShell          *shell;             /**< Brew shell object */
    IDisplay        *display;           /**< Brew display object */
    ITAPI           *tapi;              /**< TAPI object */
    int             displayHeight;      /**< Display height */
    int             displayWidth;       /**< Display width */
    char            *args;              /**< Command line args */
#endif

} Mpr;


#if !BLD_WIN_LIKE || DOXYGEN
/* TODO OPT - fix to allow _globalMpr on windows */
extern Mpr  *_globalMpr;                /* Mpr singleton */
#define mprGetMpr(ctx) _globalMpr
#else

/**
 *  Return the MPR control instance.
 *  @description Return the MPR singleton control object. 
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @return Returns the MPR control object.
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern struct Mpr *mprGetMpr(MprCtx ctx);
#endif


/**
 *  Create an instance of the MPR.
 *  @description Initializes the MPR and creates an Mpr control object. The Mpr Object manages Mpr facilities 
 *      and is the top level memory context. It may be used wherever a MprCtx parameter is required. This 
 *      function must be called prior to calling any other Mpr API.
 *  @param argc Count of command line args
 *  @param argv Command line arguments for the application. Arguments may be passed into the Mpr for retrieval
 *      by the unit test framework.
 *  @param cback Memory allocation failure notification callback.
 *  @return Returns a pointer to the Mpr object. 
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern Mpr *mprCreate(int argc, char **argv, MprAllocNotifier cback);


/**
 *  Create an instance of the MPR.
 *  @description Alternate API to create and initialize the MPR. The Mpr object manages Mpr facilities and 
 *      is the top level memory context. It may be used wherever a MprCtx parameter is required. This 
 *      function, or #mprCreate must be called prior to calling any other Mpr API.
 *  @param argc Count of arguments supplied in argv
 *  @param argv Program arguments. The MPR can store the program arguments for retrieval by other parts of the program.
 *  @param cback Callback function to be invoked on memory allocation errors. Set to null if not required.
 *  @param shell Optional reference to an O/S implementation dependent shell object. Used by Brew.
 *  @return Returns a pointer to the Mpr object. 
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern Mpr *mprCreateEx(int argc, char **argv, MprAllocNotifier cback, void *shell);

extern int mprStart(Mpr *mpr, int startFlags);
extern void mprStop(Mpr *mpr);

/**
 *  Signal the MPR to exit gracefully.
 *  @description Set the must exit flag for the MPR.
 *  @param ctx Any memory context allocated by the MPR.
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern void mprSignalExit(MprCtx ctx);

/**
 *  Determine if the MPR should exit
 *  @description Returns true if the MPR should exit gracefully.
 *  @param ctx Any memory context allocated by the MPR.
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern bool mprIsExiting(MprCtx ctx);

//  TODO - do we need these?
/**
 *  Set the value of a key.
 *  @description Sets the value of a key. Keys are stored by the MPR and may be
 *      retrieved using mprGetKeyValue. This routine is a solution for systems
 *      that do not allow global or static variables.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param key String key name.
 *  @param ptr Value to associate with the key.
 *  @return Returns zero if successful, otherwise a negative MPR error code.
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern int mprSetKeyValue(MprCtx ctx, cchar *key, void *ptr);


/* TODO -- should this be delete or remove or unset */
/**
 *  Remove a key
 *  @description Removes a key and any associated value.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param key String key name.
 *  @return Returns zero if successful, otherwise a negative MPR error code.
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern int mprRemoveKeyValue(MprCtx ctx, cchar *key);

/**
 *  Get the value of a key.
 *  @description Gets the value of a key. Keys are stored by the MPR and may be set using mprSetKeyValue. 
 *      This routine is a solution for systems that do not allow global or static variables.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param key String key name.
 *  @return Returns the value associated with the key via mprSetKeyValue.
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern cvoid *mprGetKeyValue(MprCtx ctx, cchar *key);

extern int      mprSetAppName(MprCtx ctx, cchar *name, cchar *title, cchar *version);
extern cchar    *mprGetAppName(MprCtx ctx);
extern char     *mprGetAppPath(MprCtx ctx, char *path, int pathsize);
extern char     *mprGetAppDir(MprCtx ctx, char *path, int pathsize);
extern cchar    *mprGetAppTitle(MprCtx ctx);
extern cchar    *mprGetAppVersion(MprCtx ctx);
extern void     mprSetHostName(MprCtx ctx, cchar *s);
extern cchar    *mprGetHostName(MprCtx ctx);
extern void     mprSetServerName(MprCtx ctx, cchar *s);
extern cchar    *mprGetServerName(MprCtx ctx);
extern void     mprSetDomainName(MprCtx ctx, cchar *s);
extern cchar    *mprGetDomainName(MprCtx ctx);

/**
 *  Get the debug mode.
 *  @description Returns whether the debug mode is enabled. Some modules
 *      observe debug mode and disable timeouts and timers so that single-step
 *      debugging can be used.
 *  @param ctx Any memory context allocated by the MPR.
 *  @return Returns true if debug mode is enabled, otherwise returns false.
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern bool     mprGetDebugMode(MprCtx ctx);

extern int      mprGetLogLevel(MprCtx ctx);

/**
 *  Return the O/S error code.
 *  @description Returns an O/S error code from the most recent system call. 
 *      This returns errno on Unix systems or GetLastError() on Windows..
 *  @return The O/S error code.
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern int      mprGetOsError();

extern int      mprMakeArgv(MprCtx ctx, cchar *prog, cchar *cmd, int *argc, char ***argv);

/** 
 *  Turn on debug mode.
 *  @description Debug mode disables timeouts and timers. This makes debugging
 *      much easier.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param on Set to true to enable debugging mode.
 *  @stability Evolving.
 */
extern void     mprSetDebugMode(MprCtx ctx, bool on);

/**
 *  Set the current logging level.
 *  @description This call defines the maximum level of messages that will be
 *      logged. Calls to mprLog specify a message level. If the message level
 *      is greater than the defined logging level, the message is ignored.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param level New logging level. Must be 0-9 inclusive.
 *  @return Returns the previous logging level.
 *  @stability Evolving.
 *  @ingroup MprLog
 */
extern void     mprSetLogLevel(MprCtx ctx, int level);

extern void     mprSleep(MprCtx ctx, int msec);
extern void     mprSetShell(MprCtx ctx, void *shell);
extern void     *mprGetShell(MprCtx ctx);
extern void     mprSetClassId(MprCtx ctx, uint classId);
extern uint     mprGetClassId(MprCtx ctx);

#if BREW
extern void     mprSetDisplay(MprCtx ctx, void *display);
extern void     *mprGetDisplay(MprCtx ctx);
#endif

#if BLD_WIN_LIKE
extern int      mprReadRegistry(MprCtx ctx, char **buf, int max, cchar *key, cchar *val);
extern int      mprWriteRegistry(MprCtx ctx, cchar *key, cchar *name, cchar *value);
#endif

extern int      mprStartEventsThread(Mpr *mpr);

/**
 *  Terminate the MPR.
 *  @description Terminates the MPR and disposes of all allocated resources. The mprTerminate
 *      function will recursively free all memory allocated by the MPR.
 *  @param ctx Any memory context object returned by #mprAlloc.
 *  @param graceful Shutdown gracefully waiting for all events to drain. Otherise exit immediately
 *      without waiting for any threads or events to complete.
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern void     mprTerminate(MprCtx ctx, bool graceful);

extern bool     mprIsRunningEventsThread(MprCtx ctx);
extern bool     mprIsService(Mpr *mpr);

extern void     mprSetPriority(Mpr *mpr, int pri);
extern void     mprWriteToOsLog(MprCtx ctx, cchar *msg, int flags, int level);

#if BLD_WIN_LIKE
extern HWND     mprGetHwnd(MprCtx ctx);
extern void     mprSetHwnd(MprCtx ctx, HWND h);
extern long     mprGetInst(MprCtx ctx);
extern void     mprSetInst(MprCtx ctx, long inst);
extern void     mprSetSocketMessage(MprCtx ctx, int message);
#endif

extern int      mprGetRandomBytes(MprCtx ctx, uchar *buf, int size, int block);

extern int      mprGetEndian(MprCtx ctx);

/* ******************************** Unicode ***********************************/

#define BLD_FEATURE_UTF16  1
#if BLD_FEATURE_UTF16
    typedef short MprUsData;
#else
    typedef char MprUsData;
#endif

typedef struct MprUs {
    MprUsData   *str;
    int         length;
} MprUs;


extern MprUs    *mprAllocUs(MprCtx ctx);

extern int      mprCopyUs(MprUs *dest, MprUs *src);
extern int      mprCatUs(MprUs *dest, MprUs *src);
extern int      mprCatUsArgs(MprUs *dest, MprUs *src, ...);
extern MprUs    *mprDupUs(MprUs *us);

extern int      mprCopyStrToUs(MprUs *dest, cchar *str);

extern int      mprGetUsLength(MprUs *us);

extern int      mprContainsChar(MprUs *us, int charPat);
extern int      mprContainsUs(MprUs *us, MprUs *pat);
extern int      mprContainsCaselessUs(MprUs *us, MprUs *pat);
extern int      mprContainsStr(MprUs *us, cchar *pat);

#if FUTURE
extern int      mprContainsPattern(MprUs *us, MprRegex *pat);
#endif

extern MprUs    *mprTrimUs(MprUs *dest, MprUs *pat);
extern int      mprTruncateUs(MprUs *dest, int len);
extern MprUs    *mprSubUs(MprUs *dest, int start, int len);

extern MprUs    *mprMemToUs(MprCtx ctx, const uchar *buf, int len);
extern MprUs    *mprStrToUs(MprCtx ctx, cchar *str);
extern char     *mprUsToStr(MprUs *us);

extern void     mprUsToLower(MprUs *us);
extern void     mprUsToUpper(MprUs *us);

extern MprUs    *mprTokenizeUs(MprUs *us, MprUs *delim, int *last);

extern int      mprFormatUs(MprUs *us, int maxSize, cchar *fmt, ...);
extern int      mprScanUs(MprUs *us, cchar *fmt, ...);

/*
 *  What about:
 *      isdigit, isalpha, isalnum, isupper, islower, isspace
 *      replace
 *      reverse
 *      split
 *          extern MprList *mprSplit(MprUs *us, MprUs *delim)
 */
/* MprFree */


/* ****************************************************************************/

#ifdef __cplusplus
}
#endif
#endif /* _h_MPR */

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
/************************************************************************/
/*
 *  End of file "../include/mpr.h"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../include/mprTest.h"
 */
/************************************************************************/

/*
 *  mprTest.h - Header for the Embedthis Unit Test Framework
 *  
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */


#ifndef _h_MPR_TEST
#define _h_MPR_TEST 1



#if BLD_FEATURE_TEST
/*
 *  Tunable constants
 */
#define MPR_TEST_POLL_NAP   25
#define MPR_TEST_SLEEP      (60 * 1000)
#define MPR_TEST_MAX_STACK  (16)

/*
 *  Unit test definition structures
 */
struct MprTestGroup;
typedef void        (*MprTestProc)(struct MprTestGroup *tp);

typedef struct MprTestCase {
    char            *name;
    int             level;
    MprTestProc     proc;
    int             (*init)(struct MprTestGroup *gp);
    int             (*term)(struct MprTestGroup *gp);
} MprTestCase;

typedef struct MprTestDef {
    char                *name;
    struct MprTestDef   **groupDefs;
    int                 (*init)(struct MprTestGroup *gp);
    int                 (*term)(struct MprTestGroup *gp);
    MprTestCase         caseDefs[32];
} MprTestDef;


/*
 *  Assert macros for use by unit tests
 */
#undef  assert
#define assert(C)   assertTrue(gp, MPR_LOC, C, #C)

#define MPR_TEST(level, functionName) { #functionName, level, functionName, 0, 0 }

typedef struct MprTestService {
    int             argc;                   /* Count of arguments */
    char            **argv;                 /* Arguments for test */
    int             activeThreadCount;      /* Currently active test threads */
    char            commandLine[MPR_MAX_STRING];
    bool            continueOnFailures;     /* Keep testing on failures */
    bool            debugOnFailures;        /* Break to the debugger */
    int             echoCmdLine;            /* Echo the command line */
    int             firstArg;               /* Count of arguments */
    MprList         *groups;                /* Master list of test groups */
    int             iterations;             /* Times to run the test */
    bool            singleStep;             /* Pause between tests */
    cchar           *name;                  /* Name for entire test */
    int             numThreads;             /* Number of test threads */
    int             poolThreads;            /* Count of pool threads */
    cchar           *testLog;               /* Log file for test results */
    MprTime         start;                  /* When testing began */
    int             testDepth;              /* Depth of entire test */
    MprList         *perThreadGroups;       /* Per thread copy of groups */
    int             totalFailedCount;       /* Total count of failing tests */
    int             totalTestCount;         /* Total count of all tests */
    MprList         *testFilter;            /* Test groups to run */
    int             verbose;                /* Output activity trace */

#if BLD_FEATURE_MULTITHREAD
    MprMutex        *mutex;                 /* Multi-thread sync */
#endif
} MprTestService;

extern MprTestService *mprCreateTestService(MprCtx ctx);
extern int          mprParseTestArgs(MprTestService *ts, int argc, char **argv);
extern int          mprRunTests(MprTestService *sp);
extern void         mprReportTestResults(MprTestService *sp);


/*
 *  A test group is a group of tests to cover a unit of functionality. A test group may contain other test groups.
 */
typedef struct MprTestGroup {
    char            *name;                  /* Name of test */
    char            *fullName;              /* Fully qualified name of test */
    int             testDepth;              /* Depth at which test should run */
    bool            skip;                   /* Skip this test */

    bool            success;                /* Result of last run */
    int             failedCount;            /* Total failures of this test */
    int             testCount;              /* Count of tests */
    MprList         *failures;              /* List of all failures */

    MprTestService  *service;               /* Reference to the service */

    volatile bool   condWakeFlag;           /* Used when single-threaded */
    volatile bool   cond2WakeFlag;          /* Used when single-threaded */

    struct MprTestGroup *parent;            /* Parent test group */
    MprList         *groups;                /* List of groups */
    MprList         *cases;                 /* List of tests in this group */
    MprTestDef      *def;                   /* Test definition ref */

#if BLD_FEATURE_MULTITHREAD
    MprCond         *cond;                  /* Multi-thread sync */
    MprCond         *cond2;                 /* Second multi-thread sync */
    MprMutex        *mutex;                 /* Multi-thread sync */
#endif

    void            *data;                  /* Test specific data */
    MprCtx          ctx;                    /* Memory context for unit tests to use */
    
} MprTestGroup;


extern MprTestGroup *mprAddTestGroup(MprTestService *ts, MprTestDef *def);
extern void         mprResetTestGroup(MprTestGroup *gp);
extern bool         assertTrue(MprTestGroup *gp, cchar *loc, bool success, cchar *msg);
extern void         mprSignalTestComplete(MprTestGroup *gp);
extern void         mprSignalTest2Complete(MprTestGroup *gp);
extern bool         mprWaitForTestToComplete(MprTestGroup *gp, int timeout);
extern bool         mprWaitForTest2ToComplete(MprTestGroup *gp, int timeout);


typedef struct MprTestFailure {
    char            *loc;
    char            *message;
} MprTestFailure;


#endif /* BLD_FEATURE_TEST */
#endif /* _h_MPR_TEST */


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
/************************************************************************/
/*
 *  End of file "../include/mprTest.h"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../include/mprUnix.h"
 */
/************************************************************************/

/*
 *  mprUnix.h - Make windows a bit more unix like
 *  
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

#ifndef _h_MPR_UNIX
#define _h_MPR_UNIX 1

#if BLD_WIN_LIKE
#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Define BLD_NO_POSIX_REMAP if these defines mess with your app
 */
#if BLD_WIN_LIKE && !BLD_NO_POSIX_REMAP
#define access      _access
#define close       _close
#define fileno      _fileno
#define fstat       _fstat
#define getpid      _getpid
#define open        _open
#define putenv      _putenv
#define read        _read
#define stat        _stat
#define umask       _umask
#define unlink      _unlink
#define write       _write
#define strdup      _strdup
#define lseek       _lseek
#define getcwd      _getcwd
#define chdir       _chdir

#define mkdir(a,b)  _mkdir(a)
#define rmdir(a)    _rmdir(a)

#define R_OK        4
#define W_OK        2
#define MPR_TEXT    "t"

extern void         srand48(long);
extern long         lrand48(void);
extern long         ulimit(int, ...);
extern long         nap(long);
extern int          getuid(void);
extern int          geteuid(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* BLD_WIN_LIKE */
#endif /* _h_MPR_UNIX */

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
/************************************************************************/
/*
 *  End of file "../include/mprUnix.h"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../include/pcre.h"
 */
/************************************************************************/

/*************************************************
*       Perl-Compatible Regular Expressions      *
*************************************************/

/* This is the public header file for the PCRE library, to be #included by
applications that call the PCRE functions.

           Copyright (c) 1997-2008 University of Cambridge

-----------------------------------------------------------------------------
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of the University of Cambridge nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------------
*/

#ifndef _PCRE_H
#define _PCRE_H

/* The current PCRE version information. */

#define PCRE_MAJOR          7
#define PCRE_MINOR          7
#define PCRE_PRERELEASE     
#define PCRE_DATE           2008-05-07

/* When an application links to a PCRE DLL in Windows, the symbols that are
imported have to be identified as such. When building PCRE, the appropriate
export setting is defined in pcre_internal.h, which includes this file. So we
don't change existing definitions of PCRE_EXP_DECL and PCRECPP_EXP_DECL. */

#if BLD_FEATURE_STATIC
#define PCRE_STATIC
#endif

#if BLD_ALL_IN_ONE
    /*
     *  When building all-in-one, we must use internal definitions
     */
    #ifndef PCRE_EXP_DECL
    #  ifdef _WIN32
    #    ifndef PCRE_STATIC
    #      define PCRE_EXP_DECL       extern __declspec(dllexport)
    #      define PCRE_EXP_DEFN       __declspec(dllexport)
    #      define PCRE_EXP_DATA_DEFN  __declspec(dllexport)
    #    else
    #      define PCRE_EXP_DECL       extern
    #      define PCRE_EXP_DEFN
    #      define PCRE_EXP_DATA_DEFN
    #    endif
    #  else
    #    ifdef __cplusplus
    #      define PCRE_EXP_DECL       extern "C"
    #    else
    #      define PCRE_EXP_DECL       extern
    #    endif
    #    ifndef PCRE_EXP_DEFN
    #      define PCRE_EXP_DEFN       PCRE_EXP_DECL
    #    endif
    #    ifndef PCRE_EXP_DATA_DEFN
    #      define PCRE_EXP_DATA_DEFN
    #    endif
    #  endif
    #endif
    
    /* EMBEDTHIS */
    #    ifndef PCRE_EXP_DEFN
    #      define PCRE_EXP_DEFN       PCRE_EXP_DECL
    #    endif
    #    ifndef PCRE_EXP_DATA_DEFN
    #      define PCRE_EXP_DATA_DEFN
    #    endif
#else
    #if defined(_WIN32) && !defined(PCRE_STATIC)
    #  ifndef PCRE_EXP_DECL
    #    define PCRE_EXP_DECL  extern __declspec(dllimport)
    #  endif
    #  ifdef __cplusplus
    #    ifndef PCRECPP_EXP_DECL
    #      define PCRECPP_EXP_DECL  extern __declspec(dllimport)
    #    endif
    #    ifndef PCRECPP_EXP_DEFN
    #      define PCRECPP_EXP_DEFN  __declspec(dllimport)
    #    endif
    #  endif
    #endif
    
    /* By default, we use the standard "extern" declarations. */
    
    #ifndef PCRE_EXP_DECL
    #  ifdef __cplusplus
    #    define PCRE_EXP_DECL  extern "C"
    #  else
    #    define PCRE_EXP_DECL  extern
    #  endif
    #endif
    
    #ifdef __cplusplus
    #  ifndef PCRECPP_EXP_DECL
    #    define PCRECPP_EXP_DECL  extern
    #  endif
    #  ifndef PCRECPP_EXP_DEFN
    #    define PCRECPP_EXP_DEFN
    #  endif
    #endif
#endif

/* Have to include stdlib.h in order to ensure that size_t is defined;
it is needed here for malloc. */

#include <stdlib.h>

/* Allow for C++ users */

#ifdef __cplusplus
extern "C" {
#endif

/* Options */

#define PCRE_CASELESS           0x00000001
#define PCRE_MULTILINE          0x00000002
#define PCRE_DOTALL             0x00000004
#define PCRE_EXTENDED           0x00000008
#define PCRE_ANCHORED           0x00000010
#define PCRE_DOLLAR_ENDONLY     0x00000020
#define PCRE_EXTRA              0x00000040
#define PCRE_NOTBOL             0x00000080
#define PCRE_NOTEOL             0x00000100
#define PCRE_UNGREEDY           0x00000200
#define PCRE_NOTEMPTY           0x00000400
#define PCRE_UTF8               0x00000800
#define PCRE_NO_AUTO_CAPTURE    0x00001000
#define PCRE_NO_UTF8_CHECK      0x00002000
#define PCRE_AUTO_CALLOUT       0x00004000
#define PCRE_PARTIAL            0x00008000
#define PCRE_DFA_SHORTEST       0x00010000
#define PCRE_DFA_RESTART        0x00020000
#define PCRE_FIRSTLINE          0x00040000
#define PCRE_DUPNAMES           0x00080000
#define PCRE_NEWLINE_CR         0x00100000
#define PCRE_NEWLINE_LF         0x00200000
#define PCRE_NEWLINE_CRLF       0x00300000
#define PCRE_NEWLINE_ANY        0x00400000
#define PCRE_NEWLINE_ANYCRLF    0x00500000
#define PCRE_BSR_ANYCRLF        0x00800000
#define PCRE_BSR_UNICODE        0x01000000
#define PCRE_JAVASCRIPT_COMPAT  0x02000000

/* Exec-time and get/set-time error codes */

#define PCRE_ERROR_NOMATCH         (-1)
#define PCRE_ERROR_NULL            (-2)
#define PCRE_ERROR_BADOPTION       (-3)
#define PCRE_ERROR_BADMAGIC        (-4)
#define PCRE_ERROR_UNKNOWN_OPCODE  (-5)
#define PCRE_ERROR_UNKNOWN_NODE    (-5)  /* For backward compatibility */
#define PCRE_ERROR_NOMEMORY        (-6)
#define PCRE_ERROR_NOSUBSTRING     (-7)
#define PCRE_ERROR_MATCHLIMIT      (-8)
#define PCRE_ERROR_CALLOUT         (-9)  /* Never used by PCRE itself */
#define PCRE_ERROR_BADUTF8        (-10)
#define PCRE_ERROR_BADUTF8_OFFSET (-11)
#define PCRE_ERROR_PARTIAL        (-12)
#define PCRE_ERROR_BADPARTIAL     (-13)
#define PCRE_ERROR_INTERNAL       (-14)
#define PCRE_ERROR_BADCOUNT       (-15)
#define PCRE_ERROR_DFA_UITEM      (-16)
#define PCRE_ERROR_DFA_UCOND      (-17)
#define PCRE_ERROR_DFA_UMLIMIT    (-18)
#define PCRE_ERROR_DFA_WSSIZE     (-19)
#define PCRE_ERROR_DFA_RECURSE    (-20)
#define PCRE_ERROR_RECURSIONLIMIT (-21)
#define PCRE_ERROR_NULLWSLIMIT    (-22)  /* No longer actually used */
#define PCRE_ERROR_BADNEWLINE     (-23)

/* Request types for pcre_fullinfo() */

#define PCRE_INFO_OPTIONS            0
#define PCRE_INFO_SIZE               1
#define PCRE_INFO_CAPTURECOUNT       2
#define PCRE_INFO_BACKREFMAX         3
#define PCRE_INFO_FIRSTBYTE          4
#define PCRE_INFO_FIRSTCHAR          4  /* For backwards compatibility */
#define PCRE_INFO_FIRSTTABLE         5
#define PCRE_INFO_LASTLITERAL        6
#define PCRE_INFO_NAMEENTRYSIZE      7
#define PCRE_INFO_NAMECOUNT          8
#define PCRE_INFO_NAMETABLE          9
#define PCRE_INFO_STUDYSIZE         10
#define PCRE_INFO_DEFAULT_TABLES    11
#define PCRE_INFO_OKPARTIAL         12
#define PCRE_INFO_JCHANGED          13
#define PCRE_INFO_HASCRORLF         14

/* Request types for pcre_config(). Do not re-arrange, in order to remain
compatible. */

#define PCRE_CONFIG_UTF8                    0
#define PCRE_CONFIG_NEWLINE                 1
#define PCRE_CONFIG_LINK_SIZE               2
#define PCRE_CONFIG_POSIX_MALLOC_THRESHOLD  3
#define PCRE_CONFIG_MATCH_LIMIT             4
#define PCRE_CONFIG_STACKRECURSE            5
#define PCRE_CONFIG_UNICODE_PROPERTIES      6
#define PCRE_CONFIG_MATCH_LIMIT_RECURSION   7
#define PCRE_CONFIG_BSR                     8

/* Bit flags for the pcre_extra structure. Do not re-arrange or redefine
these bits, just add new ones on the end, in order to remain compatible. */

#define PCRE_EXTRA_STUDY_DATA             0x0001
#define PCRE_EXTRA_MATCH_LIMIT            0x0002
#define PCRE_EXTRA_CALLOUT_DATA           0x0004
#define PCRE_EXTRA_TABLES                 0x0008
#define PCRE_EXTRA_MATCH_LIMIT_RECURSION  0x0010

/* Types */

struct real_pcre;                 /* declaration; the definition is private  */
typedef struct real_pcre pcre;

/* When PCRE is compiled as a C++ library, the subject pointer type can be
replaced with a custom type. For conventional use, the public interface is a
const char *. */

#ifndef PCRE_SPTR
#define PCRE_SPTR const char *
#endif

/* The structure for passing additional data to pcre_exec(). This is defined in
such as way as to be extensible. Always add new fields at the end, in order to
remain compatible. */

typedef struct pcre_extra {
  unsigned long int flags;        /* Bits for which fields are set */
  void *study_data;               /* Opaque data from pcre_study() */
  unsigned long int match_limit;  /* Maximum number of calls to match() */
  void *callout_data;             /* Data passed back in callouts */
  const unsigned char *tables;    /* Pointer to character tables */
  unsigned long int match_limit_recursion; /* Max recursive calls to match() */
} pcre_extra;

/* The structure for passing out data via the pcre_callout_function. We use a
structure so that new fields can be added on the end in future versions,
without changing the API of the function, thereby allowing old clients to work
without modification. */

typedef struct pcre_callout_block {
  int          version;           /* Identifies version of block */
  /* ------------------------ Version 0 ------------------------------- */
  int          callout_number;    /* Number compiled into pattern */
  int         *offset_vector;     /* The offset vector */
  PCRE_SPTR    subject;           /* The subject being matched */
  int          subject_length;    /* The length of the subject */
  int          start_match;       /* Offset to start of this match attempt */
  int          current_position;  /* Where we currently are in the subject */
  int          capture_top;       /* Max current capture */
  int          capture_last;      /* Most recently closed capture */
  void        *callout_data;      /* Data passed in with the call */
  /* ------------------- Added for Version 1 -------------------------- */
  int          pattern_position;  /* Offset to next item in the pattern */
  int          next_item_length;  /* Length of next item in the pattern */
  /* ------------------------------------------------------------------ */
} pcre_callout_block;

/* Indirection for store get and free functions. These can be set to
alternative malloc/free functions if required. Special ones are used in the
non-recursive case for "frames". There is also an optional callout function
that is triggered by the (?) regex item. For Virtual Pascal, these definitions
have to take another form. */

#ifndef VPCOMPAT
PCRE_EXP_DECL void *(*pcre_malloc)(size_t);
PCRE_EXP_DECL void  (*pcre_free)(void *);
PCRE_EXP_DECL void *(*pcre_stack_malloc)(size_t);
PCRE_EXP_DECL void  (*pcre_stack_free)(void *);
PCRE_EXP_DECL int   (*pcre_callout)(pcre_callout_block *);
#else   /* VPCOMPAT */
PCRE_EXP_DECL void *pcre_malloc(size_t);
PCRE_EXP_DECL void  pcre_free(void *);
PCRE_EXP_DECL void *pcre_stack_malloc(size_t);
PCRE_EXP_DECL void  pcre_stack_free(void *);
PCRE_EXP_DECL int   pcre_callout(pcre_callout_block *);
#endif  /* VPCOMPAT */

/* Exported PCRE functions */

PCRE_EXP_DECL pcre *pcre_compile(const char *, int, const char **, int *,
                  const unsigned char *);
PCRE_EXP_DECL pcre *pcre_compile2(const char *, int, int *, const char **,
                  int *, const unsigned char *);
PCRE_EXP_DECL int  pcre_config(int, void *);
PCRE_EXP_DECL int  pcre_copy_named_substring(const pcre *, const char *,
                  int *, int, const char *, char *, int);
PCRE_EXP_DECL int  pcre_copy_substring(const char *, int *, int, int, char *,
                  int);
PCRE_EXP_DECL int  pcre_dfa_exec(const pcre *, const pcre_extra *,
                  const char *, int, int, int, int *, int , int *, int);
PCRE_EXP_DECL int  pcre_exec(const pcre *, const pcre_extra *, PCRE_SPTR,
                   int, int, int, int *, int);
PCRE_EXP_DECL void pcre_free_substring(const char *);
PCRE_EXP_DECL void pcre_free_substring_list(const char **);
PCRE_EXP_DECL int  pcre_fullinfo(const pcre *, const pcre_extra *, int,
                  void *);
PCRE_EXP_DECL int  pcre_get_named_substring(const pcre *, const char *,
                  int *, int, const char *, const char **);
PCRE_EXP_DECL int  pcre_get_stringnumber(const pcre *, const char *);
PCRE_EXP_DECL int  pcre_get_stringtable_entries(const pcre *, const char *,
                  char **, char **);
PCRE_EXP_DECL int  pcre_get_substring(const char *, int *, int, int,
                  const char **);
PCRE_EXP_DECL int  pcre_get_substring_list(const char *, int *, int,
                  const char ***);
PCRE_EXP_DECL int  pcre_info(const pcre *, int *, int *);
PCRE_EXP_DECL const unsigned char *pcre_maketables(void);
PCRE_EXP_DECL int  pcre_refcount(pcre *, int);
PCRE_EXP_DECL pcre_extra *pcre_study(const pcre *, int, const char **);
PCRE_EXP_DECL const char *pcre_version(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* End of pcre.h */
/************************************************************************/
/*
 *  End of file "../include/pcre.h"
 */
/************************************************************************/


#define EJSCRIPT 1

#include "buildConfig.h"

#if BLD_FEATURE_SQLITE
#ifndef BUILDING_CROSS

/*
** 2001 September 15
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This file contains code to implement the "sqlite" command line
** utility for accessing SQLite databases.
**
** $Id: shell.c,v 1.178 2008/05/05 16:27:24 drh Exp $
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>

#if !defined(_WIN32) && !defined(WIN32) && !defined(__OS2__)
# include <signal.h>
# include <pwd.h>
# include <unistd.h>
# include <sys/types.h>
#endif

#ifdef __OS2__
# include <unistd.h>
#endif

#if defined(HAVE_READLINE) && HAVE_READLINE==1
# include <readline/readline.h>
# include <readline/history.h>
#else
# define readline(p) local_getline(p,stdin)
# define add_history(X)
# define read_history(X)
# define write_history(X)
# define stifle_history(X)
#endif

#if defined(_WIN32) || defined(WIN32)
# include <io.h>
#else
/* Make sure isatty() has a prototype.
*/
extern int isatty();
#endif

#if defined(_WIN32_WCE)
/* Windows CE (arm-wince-mingw32ce-gcc) does not provide isatty()
 * thus we always assume that we have a console. That can be
 * overridden with the -batch command line option.
 */
#define isatty(x) 1
#endif

#if !defined(_WIN32) && !defined(WIN32) && !defined(__OS2__)
#include <sys/time.h>
#include <sys/resource.h>

/* Saved resource information for the beginning of an operation */
static struct rusage sBegin;

/* True if the timer is enabled */
static int enableTimer = 0;

/*
** Begin timing an operation
*/
static void beginTimer(void){
  if( enableTimer ){
    getrusage(RUSAGE_SELF, &sBegin);
  }
}

/* Return the difference of two time_structs in microseconds */
static int timeDiff(struct timeval *pStart, struct timeval *pEnd){
  return (pEnd->tv_usec - pStart->tv_usec) + 
         1000000*(pEnd->tv_sec - pStart->tv_sec);
}

/*
** Print the timing results.
*/
static void endTimer(void){
  if( enableTimer ){
    struct rusage sEnd;
    getrusage(RUSAGE_SELF, &sEnd);
    printf("CPU Time: user %f sys %f\n",
       0.000001*timeDiff(&sBegin.ru_utime, &sEnd.ru_utime),
       0.000001*timeDiff(&sBegin.ru_stime, &sEnd.ru_stime));
  }
}
#define BEGIN_TIMER beginTimer()
#define END_TIMER endTimer()
#define HAS_TIMER 1
#else
#define BEGIN_TIMER 
#define END_TIMER
#define HAS_TIMER 0
#endif


/*
** If the following flag is set, then command execution stops
** at an error if we are not interactive.
*/
static int bail_on_error = 0;

/*
** Threat stdin as an interactive input if the following variable
** is true.  Otherwise, assume stdin is connected to a file or pipe.
*/
static int stdin_is_interactive = 1;

/*
** The following is the open SQLite database.  We make a pointer
** to this database a static variable so that it can be accessed
** by the SIGINT handler to interrupt database processing.
*/
static sqlite3 *db = 0;

/*
** True if an interrupt (Control-C) has been received.
*/
static volatile int seenInterrupt = 0;

/*
** This is the name of our program. It is set in main(), used
** in a number of other places, mostly for error messages.
*/
static char *Argv0;

/*
** Prompt strings. Initialized in main. Settable with
**   .prompt main continue
*/
static char mainPrompt[20];     /* First line prompt. default: "sqlite> "*/
static char continuePrompt[20]; /* Continuation prompt. default: "   ...> " */

/*
** Write I/O traces to the following stream.
*/
#ifdef SQLITE_ENABLE_IOTRACE
static FILE *iotrace = 0;
#endif

/*
** This routine works like printf in that its first argument is a
** format string and subsequent arguments are values to be substituted
** in place of % fields.  The result of formatting this string
** is written to iotrace.
*/
#ifdef SQLITE_ENABLE_IOTRACE
static void iotracePrintf(const char *zFormat, ...){
  va_list ap;
  char *z;
  if( iotrace==0 ) return;
  va_start(ap, zFormat);
  z = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  fprintf(iotrace, "%s", z);
  sqlite3_free(z);
}
#endif


/*
** Determines if a string is a number of not.
*/
static int isNumber(const char *z, int *realnum){
  if( *z=='-' || *z=='+' ) z++;
  if( !isdigit(*z) ){
    return 0;
  }
  z++;
  if( realnum ) *realnum = 0;
  while( isdigit(*z) ){ z++; }
  if( *z=='.' ){
    z++;
    if( !isdigit(*z) ) return 0;
    while( isdigit(*z) ){ z++; }
    if( realnum ) *realnum = 1;
  }
  if( *z=='e' || *z=='E' ){
    z++;
    if( *z=='+' || *z=='-' ) z++;
    if( !isdigit(*z) ) return 0;
    while( isdigit(*z) ){ z++; }
    if( realnum ) *realnum = 1;
  }
  return *z==0;
}

/*
** A global char* and an SQL function to access its current value 
** from within an SQL statement. This program used to use the 
** sqlite_exec_printf() API to substitue a string into an SQL statement.
** The correct way to do this with sqlite3 is to use the bind API, but
** since the shell is built around the callback paradigm it would be a lot
** of work. Instead just use this hack, which is quite harmless.
*/
static const char *zShellStatic = 0;
static void shellstaticFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  assert( 0==argc );
  assert( zShellStatic );
  sqlite3_result_text(context, zShellStatic, -1, SQLITE_STATIC);
}


/*
** This routine reads a line of text from FILE in, stores
** the text in memory obtained from malloc() and returns a pointer
** to the text.  NULL is returned at end of file, or if malloc()
** fails.
**
** The interface is like "readline" but no command-line editing
** is done.
*/
static char *local_getline(char *zPrompt, FILE *in){
  char *zLine;
  int nLine;
  int n;
  int eol;

  if( zPrompt && *zPrompt ){
    printf("%s",zPrompt);
    fflush(stdout);
  }
  nLine = 100;
  zLine = malloc( nLine );
  if( zLine==0 ) return 0;
  n = 0;
  eol = 0;
  while( !eol ){
    if( n+100>nLine ){
      nLine = nLine*2 + 100;
      zLine = realloc(zLine, nLine);
      if( zLine==0 ) return 0;
    }
    if( fgets(&zLine[n], nLine - n, in)==0 ){
      if( n==0 ){
        free(zLine);
        return 0;
      }
      zLine[n] = 0;
      eol = 1;
      break;
    }
    while( zLine[n] ){ n++; }
    if( n>0 && zLine[n-1]=='\n' ){
      n--;
      zLine[n] = 0;
      eol = 1;
    }
  }
  zLine = realloc( zLine, n+1 );
  return zLine;
}

/*
** Retrieve a single line of input text.
**
** zPrior is a string of prior text retrieved.  If not the empty
** string, then issue a continuation prompt.
*/
static char *one_input_line(const char *zPrior, FILE *in){
  char *zPrompt;
  char *zResult;
  if( in!=0 ){
    return local_getline(0, in);
  }
  if( zPrior && zPrior[0] ){
    zPrompt = continuePrompt;
  }else{
    zPrompt = mainPrompt;
  }
  zResult = readline(zPrompt);
#if defined(HAVE_READLINE) && HAVE_READLINE==1
  if( zResult && *zResult ) add_history(zResult);
#endif
  return zResult;
}

struct previous_mode_data {
  int valid;        /* Is there legit data in here? */
  int mode;
  int showHeader;
  int colWidth[100];
};

/*
** An pointer to an instance of this structure is passed from
** the main program to the callback.  This is used to communicate
** state and mode information.
*/
struct callback_data {
  sqlite3 *db;            /* The database */
  int echoOn;            /* True to echo input commands */
  int cnt;               /* Number of records displayed so far */
  FILE *out;             /* Write results here */
  int mode;              /* An output mode setting */
  int writableSchema;    /* True if PRAGMA writable_schema=ON */
  int showHeader;        /* True to show column names in List or Column mode */
  char *zDestTable;      /* Name of destination table when MODE_Insert */
  char separator[20];    /* Separator character for MODE_List */
  int colWidth[100];     /* Requested width of each column when in column mode*/
  int actualWidth[100];  /* Actual width of each column */
  char nullvalue[20];    /* The text to print when a NULL comes back from
                         ** the database */
  struct previous_mode_data explainPrev;
                         /* Holds the mode information just before
                         ** .explain ON */
  char outfile[FILENAME_MAX]; /* Filename for *out */
  const char *zDbFilename;    /* name of the database file */
};

/*
** These are the allowed modes.
*/
#define MODE_Line     0  /* One column per line.  Blank line between records */
#define MODE_Column   1  /* One record per line in neat columns */
#define MODE_List     2  /* One record per line with a separator */
#define MODE_Semi     3  /* Same as MODE_List but append ";" to each line */
#define MODE_Html     4  /* Generate an XHTML table */
#define MODE_Insert   5  /* Generate SQL "insert" statements */
#define MODE_Tcl      6  /* Generate ANSI-C or TCL quoted elements */
#define MODE_Csv      7  /* Quote strings, numbers are plain */
#define MODE_Explain  8  /* Like MODE_Column, but do not truncate data */

static const char *modeDescr[] = {
  "line",
  "column",
  "list",
  "semi",
  "html",
  "insert",
  "tcl",
  "csv",
  "explain",
};

/*
** Number of elements in an array
*/
#define ArraySize(X)  (sizeof(X)/sizeof(X[0]))

/*
** Output the given string as a quoted string using SQL quoting conventions.
*/
static void output_quoted_string(FILE *out, const char *z){
  int i;
  int nSingle = 0;
  for(i=0; z[i]; i++){
    if( z[i]=='\'' ) nSingle++;
  }
  if( nSingle==0 ){
    fprintf(out,"'%s'",z);
  }else{
    fprintf(out,"'");
    while( *z ){
      for(i=0; z[i] && z[i]!='\''; i++){}
      if( i==0 ){
        fprintf(out,"''");
        z++;
      }else if( z[i]=='\'' ){
        fprintf(out,"%.*s''",i,z);
        z += i+1;
      }else{
        fprintf(out,"%s",z);
        break;
      }
    }
    fprintf(out,"'");
  }
}

/*
** Output the given string as a quoted according to C or TCL quoting rules.
*/
static void output_c_string(FILE *out, const char *z){
  unsigned int c;
  fputc('"', out);
  while( (c = *(z++))!=0 ){
    if( c=='\\' ){
      fputc(c, out);
      fputc(c, out);
    }else if( c=='\t' ){
      fputc('\\', out);
      fputc('t', out);
    }else if( c=='\n' ){
      fputc('\\', out);
      fputc('n', out);
    }else if( c=='\r' ){
      fputc('\\', out);
      fputc('r', out);
    }else if( !isprint(c) ){
      fprintf(out, "\\%03o", c&0xff);
    }else{
      fputc(c, out);
    }
  }
  fputc('"', out);
}

/*
** Output the given string with characters that are special to
** HTML escaped.
*/
static void output_html_string(FILE *out, const char *z){
  int i;
  while( *z ){
    for(i=0; z[i] && z[i]!='<' && z[i]!='&'; i++){}
    if( i>0 ){
      fprintf(out,"%.*s",i,z);
    }
    if( z[i]=='<' ){
      fprintf(out,"&lt;");
    }else if( z[i]=='&' ){
      fprintf(out,"&amp;");
    }else{
      break;
    }
    z += i + 1;
  }
}

/*
** If a field contains any character identified by a 1 in the following
** array, then the string must be quoted for CSV.
*/
static const char needCsvQuote[] = {
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   
  1, 0, 1, 0, 0, 0, 0, 1,   0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 1, 
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   
};

/*
** Output a single term of CSV.  Actually, p->separator is used for
** the separator, which may or may not be a comma.  p->nullvalue is
** the null value.  Strings are quoted using ANSI-C rules.  Numbers
** appear outside of quotes.
*/
static void output_csv(struct callback_data *p, const char *z, int bSep){
  FILE *out = p->out;
  if( z==0 ){
    fprintf(out,"%s",p->nullvalue);
  }else{
    int i;
    int nSep = strlen(p->separator);
    for(i=0; z[i]; i++){
      if( needCsvQuote[((unsigned char*)z)[i]] 
         || (z[i]==p->separator[0] && 
             (nSep==1 || memcmp(z, p->separator, nSep)==0)) ){
        i = 0;
        break;
      }
    }
    if( i==0 ){
      putc('"', out);
      for(i=0; z[i]; i++){
        if( z[i]=='"' ) putc('"', out);
        putc(z[i], out);
      }
      putc('"', out);
    }else{
      fprintf(out, "%s", z);
    }
  }
  if( bSep ){
    fprintf(p->out, "%s", p->separator);
  }
}

#ifdef SIGINT
/*
** This routine runs when the user presses Ctrl-C
*/
static void interrupt_handler(int NotUsed){
  seenInterrupt = 1;
  if( db ) sqlite3_interrupt(db);
}
#endif

/*
** This is the callback routine that the SQLite library
** invokes for each row of a query result.
*/
static int callback(void *pArg, int nArg, char **azArg, char **azCol){
  int i;
  struct callback_data *p = (struct callback_data*)pArg;
  switch( p->mode ){
    case MODE_Line: {
      int w = 5;
      if( azArg==0 ) break;
      for(i=0; i<nArg; i++){
        int len = strlen(azCol[i] ? azCol[i] : "");
        if( len>w ) w = len;
      }
      if( p->cnt++>0 ) fprintf(p->out,"\n");
      for(i=0; i<nArg; i++){
        fprintf(p->out,"%*s = %s\n", w, azCol[i],
                azArg[i] ? azArg[i] : p->nullvalue);
      }
      break;
    }
    case MODE_Explain:
    case MODE_Column: {
      if( p->cnt++==0 ){
        for(i=0; i<nArg; i++){
          int w, n;
          if( i<ArraySize(p->colWidth) ){
            w = p->colWidth[i];
          }else{
            w = 0;
          }
          if( w<=0 ){
            w = strlen(azCol[i] ? azCol[i] : "");
            if( w<10 ) w = 10;
            n = strlen(azArg && azArg[i] ? azArg[i] : p->nullvalue);
            if( w<n ) w = n;
          }
          if( i<ArraySize(p->actualWidth) ){
            p->actualWidth[i] = w;
          }
          if( p->showHeader ){
            fprintf(p->out,"%-*.*s%s",w,w,azCol[i], i==nArg-1 ? "\n": "  ");
          }
        }
        if( p->showHeader ){
          for(i=0; i<nArg; i++){
            int w;
            if( i<ArraySize(p->actualWidth) ){
               w = p->actualWidth[i];
            }else{
               w = 10;
            }
            fprintf(p->out,"%-*.*s%s",w,w,"-----------------------------------"
                   "----------------------------------------------------------",
                    i==nArg-1 ? "\n": "  ");
          }
        }
      }
      if( azArg==0 ) break;
      for(i=0; i<nArg; i++){
        int w;
        if( i<ArraySize(p->actualWidth) ){
           w = p->actualWidth[i];
        }else{
           w = 10;
        }
        if( p->mode==MODE_Explain && azArg[i] && strlen(azArg[i])>w ){
          w = strlen(azArg[i]);
        }
        fprintf(p->out,"%-*.*s%s",w,w,
            azArg[i] ? azArg[i] : p->nullvalue, i==nArg-1 ? "\n": "  ");
      }
      break;
    }
    case MODE_Semi:
    case MODE_List: {
      if( p->cnt++==0 && p->showHeader ){
        for(i=0; i<nArg; i++){
          fprintf(p->out,"%s%s",azCol[i], i==nArg-1 ? "\n" : p->separator);
        }
      }
      if( azArg==0 ) break;
      for(i=0; i<nArg; i++){
        char *z = azArg[i];
        if( z==0 ) z = p->nullvalue;
        fprintf(p->out, "%s", z);
        if( i<nArg-1 ){
          fprintf(p->out, "%s", p->separator);
        }else if( p->mode==MODE_Semi ){
          fprintf(p->out, ";\n");
        }else{
          fprintf(p->out, "\n");
        }
      }
      break;
    }
    case MODE_Html: {
      if( p->cnt++==0 && p->showHeader ){
        fprintf(p->out,"<TR>");
        for(i=0; i<nArg; i++){
          fprintf(p->out,"<TH>%s</TH>",azCol[i]);
        }
        fprintf(p->out,"</TR>\n");
      }
      if( azArg==0 ) break;
      fprintf(p->out,"<TR>");
      for(i=0; i<nArg; i++){
        fprintf(p->out,"<TD>");
        output_html_string(p->out, azArg[i] ? azArg[i] : p->nullvalue);
        fprintf(p->out,"</TD>\n");
      }
      fprintf(p->out,"</TR>\n");
      break;
    }
    case MODE_Tcl: {
      if( p->cnt++==0 && p->showHeader ){
        for(i=0; i<nArg; i++){
          output_c_string(p->out,azCol[i] ? azCol[i] : "");
          fprintf(p->out, "%s", p->separator);
        }
        fprintf(p->out,"\n");
      }
      if( azArg==0 ) break;
      for(i=0; i<nArg; i++){
        output_c_string(p->out, azArg[i] ? azArg[i] : p->nullvalue);
        fprintf(p->out, "%s", p->separator);
      }
      fprintf(p->out,"\n");
      break;
    }
    case MODE_Csv: {
      if( p->cnt++==0 && p->showHeader ){
        for(i=0; i<nArg; i++){
          output_csv(p, azCol[i] ? azCol[i] : "", i<nArg-1);
        }
        fprintf(p->out,"\n");
      }
      if( azArg==0 ) break;
      for(i=0; i<nArg; i++){
        output_csv(p, azArg[i], i<nArg-1);
      }
      fprintf(p->out,"\n");
      break;
    }
    case MODE_Insert: {
      if( azArg==0 ) break;
      fprintf(p->out,"INSERT INTO %s VALUES(",p->zDestTable);
      for(i=0; i<nArg; i++){
        char *zSep = i>0 ? ",": "";
        if( azArg[i]==0 ){
          fprintf(p->out,"%sNULL",zSep);
        }else if( isNumber(azArg[i], 0) ){
          fprintf(p->out,"%s%s",zSep, azArg[i]);
        }else{
          if( zSep[0] ) fprintf(p->out,"%s",zSep);
          output_quoted_string(p->out, azArg[i]);
        }
      }
      fprintf(p->out,");\n");
      break;
    }
  }
  return 0;
}

/*
** Set the destination table field of the callback_data structure to
** the name of the table given.  Escape any quote characters in the
** table name.
*/
static void set_table_name(struct callback_data *p, const char *zName){
  int i, n;
  int needQuote;
  char *z;

  if( p->zDestTable ){
    free(p->zDestTable);
    p->zDestTable = 0;
  }
  if( zName==0 ) return;
  needQuote = !isalpha((unsigned char)*zName) && *zName!='_';
  for(i=n=0; zName[i]; i++, n++){
    if( !isalnum((unsigned char)zName[i]) && zName[i]!='_' ){
      needQuote = 1;
      if( zName[i]=='\'' ) n++;
    }
  }
  if( needQuote ) n += 2;
  z = p->zDestTable = malloc( n+1 );
  if( z==0 ){
    fprintf(stderr,"Out of memory!\n");
    exit(1);
  }
  n = 0;
  if( needQuote ) z[n++] = '\'';
  for(i=0; zName[i]; i++){
    z[n++] = zName[i];
    if( zName[i]=='\'' ) z[n++] = '\'';
  }
  if( needQuote ) z[n++] = '\'';
  z[n] = 0;
}

/* zIn is either a pointer to a NULL-terminated string in memory obtained
** from malloc(), or a NULL pointer. The string pointed to by zAppend is
** added to zIn, and the result returned in memory obtained from malloc().
** zIn, if it was not NULL, is freed.
**
** If the third argument, quote, is not '\0', then it is used as a 
** quote character for zAppend.
*/
static char *appendText(char *zIn, char const *zAppend, char quote){
  int len;
  int i;
  int nAppend = strlen(zAppend);
  int nIn = (zIn?strlen(zIn):0);

  len = nAppend+nIn+1;
  if( quote ){
    len += 2;
    for(i=0; i<nAppend; i++){
      if( zAppend[i]==quote ) len++;
    }
  }

  zIn = (char *)realloc(zIn, len);
  if( !zIn ){
    return 0;
  }

  if( quote ){
    char *zCsr = &zIn[nIn];
    *zCsr++ = quote;
    for(i=0; i<nAppend; i++){
      *zCsr++ = zAppend[i];
      if( zAppend[i]==quote ) *zCsr++ = quote;
    }
    *zCsr++ = quote;
    *zCsr++ = '\0';
    assert( (zCsr-zIn)==len );
  }else{
    memcpy(&zIn[nIn], zAppend, nAppend);
    zIn[len-1] = '\0';
  }

  return zIn;
}


/*
** Execute a query statement that has a single result column.  Print
** that result column on a line by itself with a semicolon terminator.
**
** This is used, for example, to show the schema of the database by
** querying the SQLITE_MASTER table.
*/
static int run_table_dump_query(FILE *out, sqlite3 *db, const char *zSelect){
  sqlite3_stmt *pSelect;
  int rc;
  rc = sqlite3_prepare(db, zSelect, -1, &pSelect, 0);
  if( rc!=SQLITE_OK || !pSelect ){
    return rc;
  }
  rc = sqlite3_step(pSelect);
  while( rc==SQLITE_ROW ){
    fprintf(out, "%s;\n", sqlite3_column_text(pSelect, 0));
    rc = sqlite3_step(pSelect);
  }
  return sqlite3_finalize(pSelect);
}


/*
** This is a different callback routine used for dumping the database.
** Each row received by this callback consists of a table name,
** the table type ("index" or "table") and SQL to create the table.
** This routine should print text sufficient to recreate the table.
*/
static int dump_callback(void *pArg, int nArg, char **azArg, char **azCol){
  int rc;
  const char *zTable;
  const char *zType;
  const char *zSql;
  struct callback_data *p = (struct callback_data *)pArg;

  if( nArg!=3 ) return 1;
  zTable = azArg[0];
  zType = azArg[1];
  zSql = azArg[2];
  
  if( strcmp(zTable, "sqlite_sequence")==0 ){
    fprintf(p->out, "DELETE FROM sqlite_sequence;\n");
  }else if( strcmp(zTable, "sqlite_stat1")==0 ){
    fprintf(p->out, "ANALYZE sqlite_master;\n");
  }else if( strncmp(zTable, "sqlite_", 7)==0 ){
    return 0;
  }else if( strncmp(zSql, "CREATE VIRTUAL TABLE", 20)==0 ){
    char *zIns;
    if( !p->writableSchema ){
      fprintf(p->out, "PRAGMA writable_schema=ON;\n");
      p->writableSchema = 1;
    }
    zIns = sqlite3_mprintf(
       "INSERT INTO sqlite_master(type,name,tbl_name,rootpage,sql)"
       "VALUES('table','%q','%q',0,'%q');",
       zTable, zTable, zSql);
    fprintf(p->out, "%s\n", zIns);
    sqlite3_free(zIns);
    return 0;
  }else{
    fprintf(p->out, "%s;\n", zSql);
  }

  if( strcmp(zType, "table")==0 ){
    sqlite3_stmt *pTableInfo = 0;
    char *zSelect = 0;
    char *zTableInfo = 0;
    char *zTmp = 0;
   
    zTableInfo = appendText(zTableInfo, "PRAGMA table_info(", 0);
    zTableInfo = appendText(zTableInfo, zTable, '"');
    zTableInfo = appendText(zTableInfo, ");", 0);

    rc = sqlite3_prepare(p->db, zTableInfo, -1, &pTableInfo, 0);
    if( zTableInfo ) free(zTableInfo);
    if( rc!=SQLITE_OK || !pTableInfo ){
      return 1;
    }

    zSelect = appendText(zSelect, "SELECT 'INSERT INTO ' || ", 0);
    zTmp = appendText(zTmp, zTable, '"');
    if( zTmp ){
      zSelect = appendText(zSelect, zTmp, '\'');
    }
    zSelect = appendText(zSelect, " || ' VALUES(' || ", 0);
    rc = sqlite3_step(pTableInfo);
    while( rc==SQLITE_ROW ){
      const char *zText = (const char *)sqlite3_column_text(pTableInfo, 1);
      zSelect = appendText(zSelect, "quote(", 0);
      zSelect = appendText(zSelect, zText, '"');
      rc = sqlite3_step(pTableInfo);
      if( rc==SQLITE_ROW ){
        zSelect = appendText(zSelect, ") || ',' || ", 0);
      }else{
        zSelect = appendText(zSelect, ") ", 0);
      }
    }
    rc = sqlite3_finalize(pTableInfo);
    if( rc!=SQLITE_OK ){
      if( zSelect ) free(zSelect);
      return 1;
    }
    zSelect = appendText(zSelect, "|| ')' FROM  ", 0);
    zSelect = appendText(zSelect, zTable, '"');

    rc = run_table_dump_query(p->out, p->db, zSelect);
    if( rc==SQLITE_CORRUPT ){
      zSelect = appendText(zSelect, " ORDER BY rowid DESC", 0);
      rc = run_table_dump_query(p->out, p->db, zSelect);
    }
    if( zSelect ) free(zSelect);
  }
  return 0;
}

/*
** Run zQuery.  Use dump_callback() as the callback routine so that
** the contents of the query are output as SQL statements.
**
** If we get a SQLITE_CORRUPT error, rerun the query after appending
** "ORDER BY rowid DESC" to the end.
*/
static int run_schema_dump_query(
  struct callback_data *p, 
  const char *zQuery,
  char **pzErrMsg
){
  int rc;
  rc = sqlite3_exec(p->db, zQuery, dump_callback, p, pzErrMsg);
  if( rc==SQLITE_CORRUPT ){
    char *zQ2;
    int len = strlen(zQuery);
    if( pzErrMsg ) sqlite3_free(*pzErrMsg);
    zQ2 = malloc( len+100 );
    if( zQ2==0 ) return rc;
    sqlite3_snprintf(sizeof(zQ2), zQ2, "%s ORDER BY rowid DESC", zQuery);
    rc = sqlite3_exec(p->db, zQ2, dump_callback, p, pzErrMsg);
    free(zQ2);
  }
  return rc;
}

/*
** Text of a help message
*/
static char zHelp[] =
  ".bail ON|OFF           Stop after hitting an error.  Default OFF\n"
  ".databases             List names and files of attached databases\n"
  ".dump ?TABLE? ...      Dump the database in an SQL text format\n"
  ".echo ON|OFF           Turn command echo on or off\n"
  ".exit                  Exit this program\n"
  ".explain ON|OFF        Turn output mode suitable for EXPLAIN on or off.\n"
  ".header(s) ON|OFF      Turn display of headers on or off\n"
  ".help                  Show this message\n"
  ".import FILE TABLE     Import data from FILE into TABLE\n"
  ".indices TABLE         Show names of all indices on TABLE\n"
#ifdef SQLITE_ENABLE_IOTRACE
  ".iotrace FILE          Enable I/O diagnostic logging to FILE\n"
#endif
#ifndef SQLITE_OMIT_LOAD_EXTENSION
  ".load FILE ?ENTRY?     Load an extension library\n"
#endif
  ".mode MODE ?TABLE?     Set output mode where MODE is one of:\n"
  "                         csv      Comma-separated values\n"
  "                         column   Left-aligned columns.  (See .width)\n"
  "                         html     HTML <table> code\n"
  "                         insert   SQL insert statements for TABLE\n"
  "                         line     One value per line\n"
  "                         list     Values delimited by .separator string\n"
  "                         tabs     Tab-separated values\n"
  "                         tcl      TCL list elements\n"
  ".nullvalue STRING      Print STRING in place of NULL values\n"
  ".output FILENAME       Send output to FILENAME\n"
  ".output stdout         Send output to the screen\n"
  ".prompt MAIN CONTINUE  Replace the standard prompts\n"
  ".quit                  Exit this program\n"
  ".read FILENAME         Execute SQL in FILENAME\n"
  ".schema ?TABLE?        Show the CREATE statements\n"
  ".separator STRING      Change separator used by output mode and .import\n"
  ".show                  Show the current values for various settings\n"
  ".tables ?PATTERN?      List names of tables matching a LIKE pattern\n"
  ".timeout MS            Try opening locked tables for MS milliseconds\n"
#if HAS_TIMER
  ".timer ON|OFF          Turn the CPU timer measurement on or off\n"
#endif
  ".width NUM NUM ...     Set column widths for \"column\" mode\n"
;

/* Forward reference */
static int process_input(struct callback_data *p, FILE *in);

/*
** Make sure the database is open.  If it is not, then open it.  If
** the database fails to open, print an error message and exit.
*/
static void open_db(struct callback_data *p){
  if( p->db==0 ){
    sqlite3_open(p->zDbFilename, &p->db);
    db = p->db;
    if( db && sqlite3_errcode(db)==SQLITE_OK ){
      sqlite3_create_function(db, "shellstatic", 0, SQLITE_UTF8, 0,
          shellstaticFunc, 0, 0);
    }
    if( db==0 || SQLITE_OK!=sqlite3_errcode(db) ){
      fprintf(stderr,"Unable to open database \"%s\": %s\n", 
          p->zDbFilename, sqlite3_errmsg(db));
      exit(1);
    }
#ifndef SQLITE_OMIT_LOAD_EXTENSION
    sqlite3_enable_load_extension(p->db, 1);
#endif
  }
}

/*
** Do C-language style dequoting.
**
**    \t    -> tab
**    \n    -> newline
**    \r    -> carriage return
**    \NNN  -> ascii character NNN in octal
**    \\    -> backslash
*/
static void resolve_backslashes(char *z){
  int i, j, c;
  for(i=j=0; (c = z[i])!=0; i++, j++){
    if( c=='\\' ){
      c = z[++i];
      if( c=='n' ){
        c = '\n';
      }else if( c=='t' ){
        c = '\t';
      }else if( c=='r' ){
        c = '\r';
      }else if( c>='0' && c<='7' ){
        c -= '0';
        if( z[i+1]>='0' && z[i+1]<='7' ){
          i++;
          c = (c<<3) + z[i] - '0';
          if( z[i+1]>='0' && z[i+1]<='7' ){
            i++;
            c = (c<<3) + z[i] - '0';
          }
        }
      }
    }
    z[j] = c;
  }
  z[j] = 0;
}

/*
** Interpret zArg as a boolean value.  Return either 0 or 1.
*/
static int booleanValue(char *zArg){
  int val = atoi(zArg);
  int j;
  for(j=0; zArg[j]; j++){
    zArg[j] = tolower(zArg[j]);
  }
  if( strcmp(zArg,"on")==0 ){
    val = 1;
  }else if( strcmp(zArg,"yes")==0 ){
    val = 1;
  }
  return val;
}

/*
** If an input line begins with "." then invoke this routine to
** process that line.
**
** Return 1 on error, 2 to exit, and 0 otherwise.
*/
static int do_meta_command(char *zLine, struct callback_data *p){
  int i = 1;
  int nArg = 0;
  int n, c;
  int rc = 0;
  char *azArg[50];

  /* Parse the input line into tokens.
  */
  while( zLine[i] && nArg<ArraySize(azArg) ){
    while( isspace((unsigned char)zLine[i]) ){ i++; }
    if( zLine[i]==0 ) break;
    if( zLine[i]=='\'' || zLine[i]=='"' ){
      int delim = zLine[i++];
      azArg[nArg++] = &zLine[i];
      while( zLine[i] && zLine[i]!=delim ){ i++; }
      if( zLine[i]==delim ){
        zLine[i++] = 0;
      }
      if( delim=='"' ) resolve_backslashes(azArg[nArg-1]);
    }else{
      azArg[nArg++] = &zLine[i];
      while( zLine[i] && !isspace((unsigned char)zLine[i]) ){ i++; }
      if( zLine[i] ) zLine[i++] = 0;
      resolve_backslashes(azArg[nArg-1]);
    }
  }

  /* Process the input line.
  */
  if( nArg==0 ) return rc;
  n = strlen(azArg[0]);
  c = azArg[0][0];
  if( c=='b' && n>1 && strncmp(azArg[0], "bail", n)==0 && nArg>1 ){
    bail_on_error = booleanValue(azArg[1]);
  }else

  if( c=='d' && n>1 && strncmp(azArg[0], "databases", n)==0 ){
    struct callback_data data;
    char *zErrMsg = 0;
    open_db(p);
    memcpy(&data, p, sizeof(data));
    data.showHeader = 1;
    data.mode = MODE_Column;
    data.colWidth[0] = 3;
    data.colWidth[1] = 15;
    data.colWidth[2] = 58;
    data.cnt = 0;
    sqlite3_exec(p->db, "PRAGMA database_list; ", callback, &data, &zErrMsg);
    if( zErrMsg ){
      fprintf(stderr,"Error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
    }
  }else

  if( c=='d' && strncmp(azArg[0], "dump", n)==0 ){
    char *zErrMsg = 0;
    open_db(p);
    fprintf(p->out, "BEGIN TRANSACTION;\n");
    p->writableSchema = 0;
    if( nArg==1 ){
      run_schema_dump_query(p, 
        "SELECT name, type, sql FROM sqlite_master "
        "WHERE sql NOT NULL AND type=='table'", 0
      );
      run_table_dump_query(p->out, p->db,
        "SELECT sql FROM sqlite_master "
        "WHERE sql NOT NULL AND type IN ('index','trigger','view')"
      );
    }else{
      int i;
      for(i=1; i<nArg; i++){
        zShellStatic = azArg[i];
        run_schema_dump_query(p,
          "SELECT name, type, sql FROM sqlite_master "
          "WHERE tbl_name LIKE shellstatic() AND type=='table'"
          "  AND sql NOT NULL", 0);
        run_table_dump_query(p->out, p->db,
          "SELECT sql FROM sqlite_master "
          "WHERE sql NOT NULL"
          "  AND type IN ('index','trigger','view')"
          "  AND tbl_name LIKE shellstatic()"
        );
        zShellStatic = 0;
      }
    }
    if( p->writableSchema ){
      fprintf(p->out, "PRAGMA writable_schema=OFF;\n");
      p->writableSchema = 0;
    }
    if( zErrMsg ){
      fprintf(stderr,"Error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
    }else{
      fprintf(p->out, "COMMIT;\n");
    }
  }else

  if( c=='e' && strncmp(azArg[0], "echo", n)==0 && nArg>1 ){
    p->echoOn = booleanValue(azArg[1]);
  }else

  if( c=='e' && strncmp(azArg[0], "exit", n)==0 ){
    rc = 2;
  }else

  if( c=='e' && strncmp(azArg[0], "explain", n)==0 ){
    int val = nArg>=2 ? booleanValue(azArg[1]) : 1;
    if(val == 1) {
      if(!p->explainPrev.valid) {
        p->explainPrev.valid = 1;
        p->explainPrev.mode = p->mode;
        p->explainPrev.showHeader = p->showHeader;
        memcpy(p->explainPrev.colWidth,p->colWidth,sizeof(p->colWidth));
      }
      /* We could put this code under the !p->explainValid
      ** condition so that it does not execute if we are already in
      ** explain mode. However, always executing it allows us an easy
      ** was to reset to explain mode in case the user previously
      ** did an .explain followed by a .width, .mode or .header
      ** command.
      */
      p->mode = MODE_Explain;
      p->showHeader = 1;
      memset(p->colWidth,0,ArraySize(p->colWidth));
      p->colWidth[0] = 4;                  /* addr */
      p->colWidth[1] = 13;                 /* opcode */
      p->colWidth[2] = 4;                  /* P1 */
      p->colWidth[3] = 4;                  /* P2 */
      p->colWidth[4] = 4;                  /* P3 */
      p->colWidth[5] = 13;                 /* P4 */
      p->colWidth[6] = 2;                  /* P5 */
      p->colWidth[7] = 13;                  /* Comment */
    }else if (p->explainPrev.valid) {
      p->explainPrev.valid = 0;
      p->mode = p->explainPrev.mode;
      p->showHeader = p->explainPrev.showHeader;
      memcpy(p->colWidth,p->explainPrev.colWidth,sizeof(p->colWidth));
    }
  }else

  if( c=='h' && (strncmp(azArg[0], "header", n)==0 ||
                 strncmp(azArg[0], "headers", n)==0 )&& nArg>1 ){
    p->showHeader = booleanValue(azArg[1]);
  }else

  if( c=='h' && strncmp(azArg[0], "help", n)==0 ){
    fprintf(stderr,zHelp);
  }else

  if( c=='i' && strncmp(azArg[0], "import", n)==0 && nArg>=3 ){
    char *zTable = azArg[2];    /* Insert data into this table */
    char *zFile = azArg[1];     /* The file from which to extract data */
    sqlite3_stmt *pStmt;        /* A statement */
    int rc;                     /* Result code */
    int nCol;                   /* Number of columns in the table */
    int nByte;                  /* Number of bytes in an SQL string */
    int i, j;                   /* Loop counters */
    int nSep;                   /* Number of bytes in p->separator[] */
    char *zSql;                 /* An SQL statement */
    char *zLine;                /* A single line of input from the file */
    char **azCol;               /* zLine[] broken up into columns */
    char *zCommit;              /* How to commit changes */   
    FILE *in;                   /* The input file */
    int lineno = 0;             /* Line number of input file */

    open_db(p);
    nSep = strlen(p->separator);
    if( nSep==0 ){
      fprintf(stderr, "non-null separator required for import\n");
      return 0;
    }
    zSql = sqlite3_mprintf("SELECT * FROM '%q'", zTable);
    if( zSql==0 ) return 0;
    nByte = strlen(zSql);
    rc = sqlite3_prepare(p->db, zSql, -1, &pStmt, 0);
    sqlite3_free(zSql);
    if( rc ){
      fprintf(stderr,"Error: %s\n", sqlite3_errmsg(db));
      nCol = 0;
      rc = 1;
    }else{
      nCol = sqlite3_column_count(pStmt);
    }
    sqlite3_finalize(pStmt);
    if( nCol==0 ) return 0;
    zSql = malloc( nByte + 20 + nCol*2 );
    if( zSql==0 ) return 0;
    sqlite3_snprintf(nByte+20, zSql, "INSERT INTO '%q' VALUES(?", zTable);
    j = strlen(zSql);
    for(i=1; i<nCol; i++){
      zSql[j++] = ',';
      zSql[j++] = '?';
    }
    zSql[j++] = ')';
    zSql[j] = 0;
    rc = sqlite3_prepare(p->db, zSql, -1, &pStmt, 0);
    free(zSql);
    if( rc ){
      fprintf(stderr, "Error: %s\n", sqlite3_errmsg(db));
      sqlite3_finalize(pStmt);
      return 1;
    }
    in = fopen(zFile, "rb");
    if( in==0 ){
      fprintf(stderr, "cannot open file: %s\n", zFile);
      sqlite3_finalize(pStmt);
      return 0;
    }
    azCol = malloc( sizeof(azCol[0])*(nCol+1) );
    if( azCol==0 ){
      fclose(in);
      return 0;
    }
    sqlite3_exec(p->db, "BEGIN", 0, 0, 0);
    zCommit = "COMMIT";
    while( (zLine = local_getline(0, in))!=0 ){
      char *z;
      i = 0;
      lineno++;
      azCol[0] = zLine;
      for(i=0, z=zLine; *z && *z!='\n' && *z!='\r'; z++){
        if( *z==p->separator[0] && strncmp(z, p->separator, nSep)==0 ){
          *z = 0;
          i++;
          if( i<nCol ){
            azCol[i] = &z[nSep];
            z += nSep-1;
          }
        }
      }
      *z = 0;
      if( i+1!=nCol ){
        fprintf(stderr,"%s line %d: expected %d columns of data but found %d\n",
           zFile, lineno, nCol, i+1);
        zCommit = "ROLLBACK";
        break;
      }
      for(i=0; i<nCol; i++){
        sqlite3_bind_text(pStmt, i+1, azCol[i], -1, SQLITE_STATIC);
      }
      sqlite3_step(pStmt);
      rc = sqlite3_reset(pStmt);
      free(zLine);
      if( rc!=SQLITE_OK ){
        fprintf(stderr,"Error: %s\n", sqlite3_errmsg(db));
        zCommit = "ROLLBACK";
        rc = 1;
        break;
      }
    }
    free(azCol);
    fclose(in);
    sqlite3_finalize(pStmt);
    sqlite3_exec(p->db, zCommit, 0, 0, 0);
  }else

  if( c=='i' && strncmp(azArg[0], "indices", n)==0 && nArg>1 ){
    struct callback_data data;
    char *zErrMsg = 0;
    open_db(p);
    memcpy(&data, p, sizeof(data));
    data.showHeader = 0;
    data.mode = MODE_List;
    zShellStatic = azArg[1];
    sqlite3_exec(p->db,
      "SELECT name FROM sqlite_master "
      "WHERE type='index' AND tbl_name LIKE shellstatic() "
      "UNION ALL "
      "SELECT name FROM sqlite_temp_master "
      "WHERE type='index' AND tbl_name LIKE shellstatic() "
      "ORDER BY 1",
      callback, &data, &zErrMsg
    );
    zShellStatic = 0;
    if( zErrMsg ){
      fprintf(stderr,"Error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
    }
  }else

#ifdef SQLITE_ENABLE_IOTRACE
  if( c=='i' && strncmp(azArg[0], "iotrace", n)==0 ){
    extern void (*sqlite3IoTrace)(const char*, ...);
    if( iotrace && iotrace!=stdout ) fclose(iotrace);
    iotrace = 0;
    if( nArg<2 ){
      sqlite3IoTrace = 0;
    }else if( strcmp(azArg[1], "-")==0 ){
      sqlite3IoTrace = iotracePrintf;
      iotrace = stdout;
    }else{
      iotrace = fopen(azArg[1], "w");
      if( iotrace==0 ){
        fprintf(stderr, "cannot open \"%s\"\n", azArg[1]);
        sqlite3IoTrace = 0;
      }else{
        sqlite3IoTrace = iotracePrintf;
      }
    }
  }else
#endif

#ifndef SQLITE_OMIT_LOAD_EXTENSION
  if( c=='l' && strncmp(azArg[0], "load", n)==0 && nArg>=2 ){
    const char *zFile, *zProc;
    char *zErrMsg = 0;
    int rc;
    zFile = azArg[1];
    zProc = nArg>=3 ? azArg[2] : 0;
    open_db(p);
    rc = sqlite3_load_extension(p->db, zFile, zProc, &zErrMsg);
    if( rc!=SQLITE_OK ){
      fprintf(stderr, "%s\n", zErrMsg);
      sqlite3_free(zErrMsg);
      rc = 1;
    }
  }else
#endif

  if( c=='m' && strncmp(azArg[0], "mode", n)==0 && nArg>=2 ){
    int n2 = strlen(azArg[1]);
    if( strncmp(azArg[1],"line",n2)==0
        ||
        strncmp(azArg[1],"lines",n2)==0 ){
      p->mode = MODE_Line;
    }else if( strncmp(azArg[1],"column",n2)==0
              ||
              strncmp(azArg[1],"columns",n2)==0 ){
      p->mode = MODE_Column;
    }else if( strncmp(azArg[1],"list",n2)==0 ){
      p->mode = MODE_List;
    }else if( strncmp(azArg[1],"html",n2)==0 ){
      p->mode = MODE_Html;
    }else if( strncmp(azArg[1],"tcl",n2)==0 ){
      p->mode = MODE_Tcl;
    }else if( strncmp(azArg[1],"csv",n2)==0 ){
      p->mode = MODE_Csv;
      sqlite3_snprintf(sizeof(p->separator), p->separator, ",");
    }else if( strncmp(azArg[1],"tabs",n2)==0 ){
      p->mode = MODE_List;
      sqlite3_snprintf(sizeof(p->separator), p->separator, "\t");
    }else if( strncmp(azArg[1],"insert",n2)==0 ){
      p->mode = MODE_Insert;
      if( nArg>=3 ){
        set_table_name(p, azArg[2]);
      }else{
        set_table_name(p, "table");
      }
    }else {
      fprintf(stderr,"mode should be one of: "
         "column csv html insert line list tabs tcl\n");
    }
  }else

  if( c=='n' && strncmp(azArg[0], "nullvalue", n)==0 && nArg==2 ) {
    sqlite3_snprintf(sizeof(p->nullvalue), p->nullvalue,
                     "%.*s", (int)ArraySize(p->nullvalue)-1, azArg[1]);
  }else

  if( c=='o' && strncmp(azArg[0], "output", n)==0 && nArg==2 ){
    if( p->out!=stdout ){
      fclose(p->out);
    }
    if( strcmp(azArg[1],"stdout")==0 ){
      p->out = stdout;
      sqlite3_snprintf(sizeof(p->outfile), p->outfile, "stdout");
    }else{
      p->out = fopen(azArg[1], "wb");
      if( p->out==0 ){
        fprintf(stderr,"can't write to \"%s\"\n", azArg[1]);
        p->out = stdout;
      } else {
         sqlite3_snprintf(sizeof(p->outfile), p->outfile, "%s", azArg[1]);
      }
    }
  }else

  if( c=='p' && strncmp(azArg[0], "prompt", n)==0 && (nArg==2 || nArg==3)){
    if( nArg >= 2) {
      strncpy(mainPrompt,azArg[1],(int)ArraySize(mainPrompt)-1);
    }
    if( nArg >= 3) {
      strncpy(continuePrompt,azArg[2],(int)ArraySize(continuePrompt)-1);
    }
  }else

  if( c=='q' && strncmp(azArg[0], "quit", n)==0 ){
    rc = 2;
  }else

  if( c=='r' && strncmp(azArg[0], "read", n)==0 && nArg==2 ){
    FILE *alt = fopen(azArg[1], "rb");
    if( alt==0 ){
      fprintf(stderr,"can't open \"%s\"\n", azArg[1]);
    }else{
      process_input(p, alt);
      fclose(alt);
    }
  }else

  if( c=='s' && strncmp(azArg[0], "schema", n)==0 ){
    struct callback_data data;
    char *zErrMsg = 0;
    open_db(p);
    memcpy(&data, p, sizeof(data));
    data.showHeader = 0;
    data.mode = MODE_Semi;
    if( nArg>1 ){
      int i;
      for(i=0; azArg[1][i]; i++) azArg[1][i] = tolower(azArg[1][i]);
      if( strcmp(azArg[1],"sqlite_master")==0 ){
        char *new_argv[2], *new_colv[2];
        new_argv[0] = "CREATE TABLE sqlite_master (\n"
                      "  type text,\n"
                      "  name text,\n"
                      "  tbl_name text,\n"
                      "  rootpage integer,\n"
                      "  sql text\n"
                      ")";
        new_argv[1] = 0;
        new_colv[0] = "sql";
        new_colv[1] = 0;
        callback(&data, 1, new_argv, new_colv);
      }else if( strcmp(azArg[1],"sqlite_temp_master")==0 ){
        char *new_argv[2], *new_colv[2];
        new_argv[0] = "CREATE TEMP TABLE sqlite_temp_master (\n"
                      "  type text,\n"
                      "  name text,\n"
                      "  tbl_name text,\n"
                      "  rootpage integer,\n"
                      "  sql text\n"
                      ")";
        new_argv[1] = 0;
        new_colv[0] = "sql";
        new_colv[1] = 0;
        callback(&data, 1, new_argv, new_colv);
      }else{
        zShellStatic = azArg[1];
        sqlite3_exec(p->db,
          "SELECT sql FROM "
          "  (SELECT * FROM sqlite_master UNION ALL"
          "   SELECT * FROM sqlite_temp_master) "
          "WHERE tbl_name LIKE shellstatic() AND type!='meta' AND sql NOTNULL "
          "ORDER BY substr(type,2,1), name",
          callback, &data, &zErrMsg);
        zShellStatic = 0;
      }
    }else{
      sqlite3_exec(p->db,
         "SELECT sql FROM "
         "  (SELECT * FROM sqlite_master UNION ALL"
         "   SELECT * FROM sqlite_temp_master) "
         "WHERE type!='meta' AND sql NOTNULL AND name NOT LIKE 'sqlite_%'"
         "ORDER BY substr(type,2,1), name",
         callback, &data, &zErrMsg
      );
    }
    if( zErrMsg ){
      fprintf(stderr,"Error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
    }
  }else

  if( c=='s' && strncmp(azArg[0], "separator", n)==0 && nArg==2 ){
    sqlite3_snprintf(sizeof(p->separator), p->separator,
                     "%.*s", (int)sizeof(p->separator)-1, azArg[1]);
  }else

  if( c=='s' && strncmp(azArg[0], "show", n)==0){
    int i;
    fprintf(p->out,"%9.9s: %s\n","echo", p->echoOn ? "on" : "off");
    fprintf(p->out,"%9.9s: %s\n","explain", p->explainPrev.valid ? "on" :"off");
    fprintf(p->out,"%9.9s: %s\n","headers", p->showHeader ? "on" : "off");
    fprintf(p->out,"%9.9s: %s\n","mode", modeDescr[p->mode]);
    fprintf(p->out,"%9.9s: ", "nullvalue");
      output_c_string(p->out, p->nullvalue);
      fprintf(p->out, "\n");
    fprintf(p->out,"%9.9s: %s\n","output",
                                 strlen(p->outfile) ? p->outfile : "stdout");
    fprintf(p->out,"%9.9s: ", "separator");
      output_c_string(p->out, p->separator);
      fprintf(p->out, "\n");
    fprintf(p->out,"%9.9s: ","width");
    for (i=0;i<(int)ArraySize(p->colWidth) && p->colWidth[i] != 0;i++) {
      fprintf(p->out,"%d ",p->colWidth[i]);
    }
    fprintf(p->out,"\n");
  }else

  if( c=='t' && n>1 && strncmp(azArg[0], "tables", n)==0 ){
    char **azResult;
    int nRow, rc;
    char *zErrMsg;
    open_db(p);
    if( nArg==1 ){
      rc = sqlite3_get_table(p->db,
        "SELECT name FROM sqlite_master "
        "WHERE type IN ('table','view') AND name NOT LIKE 'sqlite_%'"
        "UNION ALL "
        "SELECT name FROM sqlite_temp_master "
        "WHERE type IN ('table','view') "
        "ORDER BY 1",
        &azResult, &nRow, 0, &zErrMsg
      );
    }else{
      zShellStatic = azArg[1];
      rc = sqlite3_get_table(p->db,
        "SELECT name FROM sqlite_master "
        "WHERE type IN ('table','view') AND name LIKE '%'||shellstatic()||'%' "
        "UNION ALL "
        "SELECT name FROM sqlite_temp_master "
        "WHERE type IN ('table','view') AND name LIKE '%'||shellstatic()||'%' "
        "ORDER BY 1",
        &azResult, &nRow, 0, &zErrMsg
      );
      zShellStatic = 0;
    }
    if( zErrMsg ){
      fprintf(stderr,"Error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
    }
    if( rc==SQLITE_OK ){
      int len, maxlen = 0;
      int i, j;
      int nPrintCol, nPrintRow;
      for(i=1; i<=nRow; i++){
        if( azResult[i]==0 ) continue;
        len = strlen(azResult[i]);
        if( len>maxlen ) maxlen = len;
      }
      nPrintCol = 80/(maxlen+2);
      if( nPrintCol<1 ) nPrintCol = 1;
      nPrintRow = (nRow + nPrintCol - 1)/nPrintCol;
      for(i=0; i<nPrintRow; i++){
        for(j=i+1; j<=nRow; j+=nPrintRow){
          char *zSp = j<=nPrintRow ? "" : "  ";
          printf("%s%-*s", zSp, maxlen, azResult[j] ? azResult[j] : "");
        }
        printf("\n");
      }
    }else{
      rc = 1;
    }
    sqlite3_free_table(azResult);
  }else

  if( c=='t' && n>4 && strncmp(azArg[0], "timeout", n)==0 && nArg>=2 ){
    open_db(p);
    sqlite3_busy_timeout(p->db, atoi(azArg[1]));
  }else
  
#if HAS_TIMER  
  if( c=='t' && n>=5 && strncmp(azArg[0], "timer", n)==0 && nArg>1 ){
    enableTimer = booleanValue(azArg[1]);
  }else
#endif

  if( c=='w' && strncmp(azArg[0], "width", n)==0 ){
    int j;
    assert( nArg<=ArraySize(azArg) );
    for(j=1; j<nArg && j<ArraySize(p->colWidth); j++){
      p->colWidth[j-1] = atoi(azArg[j]);
    }
  }else


  {
    fprintf(stderr, "unknown command or invalid arguments: "
      " \"%s\". Enter \".help\" for help\n", azArg[0]);
  }

  return rc;
}

/*
** Return TRUE if a semicolon occurs anywhere in the first N characters
** of string z[].
*/
static int _contains_semicolon(const char *z, int N){
  int i;
  for(i=0; i<N; i++){  if( z[i]==';' ) return 1; }
  return 0;
}

/*
** Test to see if a line consists entirely of whitespace.
*/
static int _all_whitespace(const char *z){
  for(; *z; z++){
    if( isspace(*(unsigned char*)z) ) continue;
    if( *z=='/' && z[1]=='*' ){
      z += 2;
      while( *z && (*z!='*' || z[1]!='/') ){ z++; }
      if( *z==0 ) return 0;
      z++;
      continue;
    }
    if( *z=='-' && z[1]=='-' ){
      z += 2;
      while( *z && *z!='\n' ){ z++; }
      if( *z==0 ) return 1;
      continue;
    }
    return 0;
  }
  return 1;
}

/*
** Return TRUE if the line typed in is an SQL command terminator other
** than a semi-colon.  The SQL Server style "go" command is understood
** as is the Oracle "/".
*/
static int _is_command_terminator(const char *zLine){
  while( isspace(*(unsigned char*)zLine) ){ zLine++; };
  if( zLine[0]=='/' && _all_whitespace(&zLine[1]) ) return 1;  /* Oracle */
  if( tolower(zLine[0])=='g' && tolower(zLine[1])=='o'
         && _all_whitespace(&zLine[2]) ){
    return 1;  /* SQL Server */
  }
  return 0;
}

/*
** Read input from *in and process it.  If *in==0 then input
** is interactive - the user is typing it it.  Otherwise, input
** is coming from a file or device.  A prompt is issued and history
** is saved only if input is interactive.  An interrupt signal will
** cause this routine to exit immediately, unless input is interactive.
**
** Return the number of errors.
*/
static int process_input(struct callback_data *p, FILE *in){
  char *zLine = 0;
  char *zSql = 0;
  int nSql = 0;
  int nSqlPrior = 0;
  char *zErrMsg;
  int rc;
  int errCnt = 0;
  int lineno = 0;
  int startline = 0;

  while( errCnt==0 || !bail_on_error || (in==0 && stdin_is_interactive) ){
    fflush(p->out);
    free(zLine);
    zLine = one_input_line(zSql, in);
    if( zLine==0 ){
      break;  /* We have reached EOF */
    }
    if( seenInterrupt ){
      if( in!=0 ) break;
      seenInterrupt = 0;
    }
    lineno++;
    if( p->echoOn ) printf("%s\n", zLine);
    if( (zSql==0 || zSql[0]==0) && _all_whitespace(zLine) ) continue;
    if( zLine && zLine[0]=='.' && nSql==0 ){
      rc = do_meta_command(zLine, p);
      if( rc==2 ){
        break;
      }else if( rc ){
        errCnt++;
      }
      continue;
    }
    if( _is_command_terminator(zLine) ){
      memcpy(zLine,";",2);
    }
    nSqlPrior = nSql;
    if( zSql==0 ){
      int i;
      for(i=0; zLine[i] && isspace((unsigned char)zLine[i]); i++){}
      if( zLine[i]!=0 ){
        nSql = strlen(zLine);
        zSql = malloc( nSql+1 );
        if( zSql==0 ){
          fprintf(stderr, "out of memory\n");
          exit(1);
        }
        memcpy(zSql, zLine, nSql+1);
        startline = lineno;
      }
    }else{
      int len = strlen(zLine);
      zSql = realloc( zSql, nSql + len + 2 );
      if( zSql==0 ){
        fprintf(stderr,"%s: out of memory!\n", Argv0);
        exit(1);
      }
      zSql[nSql++] = '\n';
      memcpy(&zSql[nSql], zLine, len+1);
      nSql += len;
    }
    if( zSql && _contains_semicolon(&zSql[nSqlPrior], nSql-nSqlPrior)
                && sqlite3_complete(zSql) ){
      p->cnt = 0;
      open_db(p);
      BEGIN_TIMER;
      rc = sqlite3_exec(p->db, zSql, callback, p, &zErrMsg);
      END_TIMER;
      if( rc || zErrMsg ){
        char zPrefix[100];
        if( in!=0 || !stdin_is_interactive ){
          sqlite3_snprintf(sizeof(zPrefix), zPrefix, 
                           "SQL error near line %d:", startline);
        }else{
          sqlite3_snprintf(sizeof(zPrefix), zPrefix, "SQL error:");
        }
        if( zErrMsg!=0 ){
          printf("%s %s\n", zPrefix, zErrMsg);
          sqlite3_free(zErrMsg);
          zErrMsg = 0;
        }else{
          printf("%s %s\n", zPrefix, sqlite3_errmsg(p->db));
        }
        errCnt++;
      }
      free(zSql);
      zSql = 0;
      nSql = 0;
    }
  }
  if( zSql ){
    if( !_all_whitespace(zSql) ) printf("Incomplete SQL: %s\n", zSql);
    free(zSql);
  }
  free(zLine);
  return errCnt;
}

/*
** Return a pathname which is the user's home directory.  A
** 0 return indicates an error of some kind.  Space to hold the
** resulting string is obtained from malloc().  The calling
** function should free the result.
*/
static char *find_home_dir(void){
  char *home_dir = NULL;

#if !defined(_WIN32) && !defined(WIN32) && !defined(__OS2__) && !defined(_WIN32_WCE)
  struct passwd *pwent;
  uid_t uid = getuid();
  if( (pwent=getpwuid(uid)) != NULL) {
    home_dir = pwent->pw_dir;
  }
#endif

#if defined(_WIN32_WCE)
  /* Windows CE (arm-wince-mingw32ce-gcc) does not provide getenv()
   */
  home_dir = strdup("/");
#else

#if defined(_WIN32) || defined(WIN32) || defined(__OS2__)
  if (!home_dir) {
    home_dir = getenv("USERPROFILE");
  }
#endif

  if (!home_dir) {
    home_dir = getenv("HOME");
  }

#if defined(_WIN32) || defined(WIN32) || defined(__OS2__)
  if (!home_dir) {
    char *zDrive, *zPath;
    int n;
    zDrive = getenv("HOMEDRIVE");
    zPath = getenv("HOMEPATH");
    if( zDrive && zPath ){
      n = strlen(zDrive) + strlen(zPath) + 1;
      home_dir = malloc( n );
      if( home_dir==0 ) return 0;
      sqlite3_snprintf(n, home_dir, "%s%s", zDrive, zPath);
      return home_dir;
    }
    home_dir = "c:\\";
  }
#endif

#endif /* !_WIN32_WCE */

  if( home_dir ){
    int n = strlen(home_dir) + 1;
    char *z = malloc( n );
    if( z ) memcpy(z, home_dir, n);
    home_dir = z;
  }

  return home_dir;
}

/*
** Read input from the file given by sqliterc_override.  Or if that
** parameter is NULL, take input from ~/.sqliterc
*/
static void process_sqliterc(
  struct callback_data *p,        /* Configuration data */
  const char *sqliterc_override   /* Name of config file. NULL to use default */
){
  char *home_dir = NULL;
  const char *sqliterc = sqliterc_override;
  char *zBuf = 0;
  FILE *in = NULL;
  int nBuf;

  if (sqliterc == NULL) {
    home_dir = find_home_dir();
    if( home_dir==0 ){
      fprintf(stderr,"%s: cannot locate your home directory!\n", Argv0);
      return;
    }
    nBuf = strlen(home_dir) + 16;
    zBuf = malloc( nBuf );
    if( zBuf==0 ){
      fprintf(stderr,"%s: out of memory!\n", Argv0);
      exit(1);
    }
    sqlite3_snprintf(nBuf, zBuf,"%s/.sqliterc",home_dir);
    free(home_dir);
    sqliterc = (const char*)zBuf;
  }
  in = fopen(sqliterc,"rb");
  if( in ){
    if( stdin_is_interactive ){
      printf("-- Loading resources from %s\n",sqliterc);
    }
    process_input(p,in);
    fclose(in);
  }
  free(zBuf);
  return;
}

/*
** Show available command line options
*/
static const char zOptions[] = 
  "   -init filename       read/process named file\n"
  "   -echo                print commands before execution\n"
  "   -[no]header          turn headers on or off\n"
  "   -bail                stop after hitting an error\n"
  "   -interactive         force interactive I/O\n"
  "   -batch               force batch I/O\n"
  "   -column              set output mode to 'column'\n"
  "   -csv                 set output mode to 'csv'\n"
  "   -html                set output mode to HTML\n"
  "   -line                set output mode to 'line'\n"
  "   -list                set output mode to 'list'\n"
  "   -separator 'x'       set output field separator (|)\n"
  "   -nullvalue 'text'    set text string for NULL values\n"
  "   -version             show SQLite version\n"
;
static void usage(int showDetail){
  fprintf(stderr,
      "Usage: %s [OPTIONS] FILENAME [SQL]\n"  
      "FILENAME is the name of an SQLite database. A new database is created\n"
      "if the file does not previously exist.\n", Argv0);
  if( showDetail ){
    fprintf(stderr, "OPTIONS include:\n%s", zOptions);
  }else{
    fprintf(stderr, "Use the -help option for additional information\n");
  }
  exit(1);
}

/*
** Initialize the state information in data
*/
static void main_init(struct callback_data *data) {
  memset(data, 0, sizeof(*data));
  data->mode = MODE_List;
  memcpy(data->separator,"|", 2);
  data->showHeader = 0;
  sqlite3_snprintf(sizeof(mainPrompt), mainPrompt,"sqlite> ");
  sqlite3_snprintf(sizeof(continuePrompt), continuePrompt,"   ...> ");
}

int main(int argc, char **argv){
  char *zErrMsg = 0;
  struct callback_data data;
  const char *zInitFile = 0;
  char *zFirstCmd = 0;
  int i;
  int rc = 0;

  Argv0 = argv[0];
  main_init(&data);
  stdin_is_interactive = isatty(0);

  /* Make sure we have a valid signal handler early, before anything
  ** else is done.
  */
#ifdef SIGINT
  signal(SIGINT, interrupt_handler);
#endif

  /* Do an initial pass through the command-line argument to locate
  ** the name of the database file, the name of the initialization file,
  ** and the first command to execute.
  */
  for(i=1; i<argc-1; i++){
    char *z;
    if( argv[i][0]!='-' ) break;
    z = argv[i];
    if( z[0]=='-' && z[1]=='-' ) z++;
    if( strcmp(argv[i],"-separator")==0 || strcmp(argv[i],"-nullvalue")==0 ){
      i++;
    }else if( strcmp(argv[i],"-init")==0 ){
      i++;
      zInitFile = argv[i];
    }
  }
  if( i<argc ){
#ifdef OS_OS2
    data.zDbFilename = (const char *)convertCpPathToUtf8( argv[i++] );
#else
    data.zDbFilename = argv[i++];
#endif
  }else{
#ifndef SQLITE_OMIT_MEMORYDB
    data.zDbFilename = ":memory:";
#else
    data.zDbFilename = 0;
#endif
  }
  if( i<argc ){
    zFirstCmd = argv[i++];
  }
  data.out = stdout;

#ifdef SQLITE_OMIT_MEMORYDB
  if( data.zDbFilename==0 ){
    fprintf(stderr,"%s: no database filename specified\n", argv[0]);
    exit(1);
  }
#endif

  /* Go ahead and open the database file if it already exists.  If the
  ** file does not exist, delay opening it.  This prevents empty database
  ** files from being created if a user mistypes the database name argument
  ** to the sqlite command-line tool.
  */
  if( access(data.zDbFilename, 0)==0 ){
    open_db(&data);
  }

  /* Process the initialization file if there is one.  If no -init option
  ** is given on the command line, look for a file named ~/.sqliterc and
  ** try to process it.
  */
  process_sqliterc(&data,zInitFile);

  /* Make a second pass through the command-line argument and set
  ** options.  This second pass is delayed until after the initialization
  ** file is processed so that the command-line arguments will override
  ** settings in the initialization file.
  */
  for(i=1; i<argc && argv[i][0]=='-'; i++){
    char *z = argv[i];
    if( z[1]=='-' ){ z++; }
    if( strcmp(z,"-init")==0 ){
      i++;
    }else if( strcmp(z,"-html")==0 ){
      data.mode = MODE_Html;
    }else if( strcmp(z,"-list")==0 ){
      data.mode = MODE_List;
    }else if( strcmp(z,"-line")==0 ){
      data.mode = MODE_Line;
    }else if( strcmp(z,"-column")==0 ){
      data.mode = MODE_Column;
    }else if( strcmp(z,"-csv")==0 ){
      data.mode = MODE_Csv;
      memcpy(data.separator,",",2);
    }else if( strcmp(z,"-separator")==0 ){
      i++;
      sqlite3_snprintf(sizeof(data.separator), data.separator,
                       "%.*s",(int)sizeof(data.separator)-1,argv[i]);
    }else if( strcmp(z,"-nullvalue")==0 ){
      i++;
      sqlite3_snprintf(sizeof(data.nullvalue), data.nullvalue,
                       "%.*s",(int)sizeof(data.nullvalue)-1,argv[i]);
    }else if( strcmp(z,"-header")==0 ){
      data.showHeader = 1;
    }else if( strcmp(z,"-noheader")==0 ){
      data.showHeader = 0;
    }else if( strcmp(z,"-echo")==0 ){
      data.echoOn = 1;
    }else if( strcmp(z,"-bail")==0 ){
      bail_on_error = 1;
    }else if( strcmp(z,"-version")==0 ){
      printf("%s\n", sqlite3_libversion());
      return 0;
    }else if( strcmp(z,"-interactive")==0 ){
      stdin_is_interactive = 1;
    }else if( strcmp(z,"-batch")==0 ){
      stdin_is_interactive = 0;
    }else if( strcmp(z,"-help")==0 || strcmp(z, "--help")==0 ){
      usage(1);
    }else{
      fprintf(stderr,"%s: unknown option: %s\n", Argv0, z);
      fprintf(stderr,"Use -help for a list of options.\n");
      return 1;
    }
  }

  if( zFirstCmd ){
    /* Run just the command that follows the database name
    */
    if( zFirstCmd[0]=='.' ){
      do_meta_command(zFirstCmd, &data);
      exit(0);
    }else{
      int rc;
      open_db(&data);
      rc = sqlite3_exec(data.db, zFirstCmd, callback, &data, &zErrMsg);
      if( rc!=0 && zErrMsg!=0 ){
        fprintf(stderr,"SQL error: %s\n", zErrMsg);
        exit(1);
      }
    }
  }else{
    /* Run commands received from standard input
    */
    if( stdin_is_interactive ){
      char *zHome;
      char *zHistory = 0;
      int nHistory;
      printf(
        "SQLite version %s\n"
        "Enter \".help\" for instructions\n",
        sqlite3_libversion()
      );
      zHome = find_home_dir();
      if( zHome && (zHistory = malloc(nHistory = strlen(zHome)+20))!=0 ){
        sqlite3_snprintf(nHistory, zHistory,"%s/.sqlite_history", zHome);
      }
#if defined(HAVE_READLINE) && HAVE_READLINE==1
      if( zHistory ) read_history(zHistory);
#endif
      rc = process_input(&data, 0);
      if( zHistory ){
        stifle_history(100);
        write_history(zHistory);
        free(zHistory);
      }
      free(zHome);
    }else{
      rc = process_input(&data, stdin);
    }
  }
  set_table_name(&data, 0);
  if( db ){
    if( sqlite3_close(db)!=SQLITE_OK ){
      fprintf(stderr,"error closing database: %s\n", sqlite3_errmsg(db));
    }
  }
  return rc;
}
#endif /* !BUILDING_CROSS */
#endif /* BLD_FEATURE_SQLITE */
