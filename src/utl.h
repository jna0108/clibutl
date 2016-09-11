/* 
**  (C) 2014 by Remo Dentato (rdentato@gmail.com)
** 
** This software is distributed under the terms of the MIT license:
**  https://opensource.org/licenses/MIT
**     
**                     ___   __
**                  __/  /_ /  )
**          ___  __(_   ___)  /
**         /  / /  )/  /  /  /  Minimalist
**        /  /_/  //  (__/  /  C utility 
**       (____,__/(_____(__/  Library
** 
** [[[

# utl

This library provides functions that are, hopefully, useful during
the developemnt phase. 

The functions are related to:

 - *Logging*. Mainly to be used for *testing* and *debugging* purpose 
 - *Memory check*
 - *Finite State Machines*
 - *Containers*. 
 - *Pattern matching* over string

The objective is to provide simple and reasonably
easy to use solutions to recurring problems that could be replaced
by more complex solutions if needed. For instance, the logging
functions are limited to write on files, for production environment
they can be replaced by some other logging (e.g. something with
multiple level of logging or using syslogd, ...).

The point is to avoid having to think about trivial things (again, like
logging) and directly jump into your problem. 
 
 
## How to use utl

 - Include the `utl.h` header in your source files.
 - Compile and link `utl.c` with the other files.

Alternatively, you can do without `utl.c` simply by
compiling one of your source files with the 
symbol *`UTL_MAIN`* defined (either by definining it 
before including the header, or via a compiler switch
like "`-DUTL_MAIN`"). A good candidate is the
file where your `main()` function is.

## The *selector* symbols

You may want to exclude entirely a portion of the library because
it is of no interest for your project. Leaving it would only 
pollute your namespace (and could possibly stop you using
`utl.h` altogether).

 - `NDEBUG` will cause:
   - `logdebug()` and `logassert()` to perform no action;
   - `logcheck()` to perform no action and always return `1`;
   - Memory checking functions will be disabled (even if `UTL_MEMCHECK`
     is defined).

 - `UTL_NOLOG` will make the logging functions `logxxx` unavailble.
 
 - `UTL_NOFSM` will make the finite state machines `fsmxxx` constructors unavailble.
 
 - `UTL_NOPMX` will make the pattern matching functions `pmxyyy` unavailble.
 
 - `UTL_NOVEC` will make the containers functions (`vecxxx`, `stkxxx`, `bufxxx`, ...) unavailble.

 - `UTL_MEMCHECK` will instrument the dynamic memory functions (`malloc()`, `free()`, ...) 
   to log their activities and perform additional checks. It also enables logging
   even if `UTL_NOLOG` is defined.

** ]]]
*/
   

#ifndef UTL_H
#define UTL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <time.h>
#include <ctype.h>

#ifdef NDEBUG

/* Give NDEBUG precedence over DEBUG */
#ifdef DEBUG
#undef DEBUG
#endif

/* Disable memory logging if NDEBUG is defined */
#ifdef UTL_MEMCHECK
#undef UTL_MEMCHECK
#endif

#endif /* NDEBUG */

#ifdef UTL_MEMCHECK
#ifdef UTL_NOLOG
#undef UTL_NOLOG
#endif
#endif

#ifdef UTL_MAIN
#define utl_extern(n,v) n = v
#else
#define utl_extern(n,v) extern n
#endif

#ifdef UTL_MAIN
int   utl_ret(int x)      {return x;}
void *utl_retptr(void *x) {return x;}
#else
int   utl_ret(int x);
void *utl_retptr(void *x);
#endif

utl_extern(char *utl_emptystring,"");


#ifndef UTL_NOLOG

/*

**          __
**         /  )
**        /  /______   ______
**       /  //  __  \ /  __  \
**      /  //  /_/  //  /_/  /
**     (__/ \______/ \___   /
**                    __/  /
**                   (____/
 

[[[
## Logging

Logging functions are just wrappers around the `fprintf()` function.
They print a message on a file (by default `stderr`) adding a timestamp
before the message and a newline `\n` at the end.
To make things simple, there is only one level of logging and only one log file
at the time.

### Log messages

  - `int logprintf(char *format, ...);`
     is the main function. On executing:  
     
     ```C  
       logprintf("Warp speed to %d.",count);
       logprintf("Moving on now.")
     ```
     
     the following lines will be printed (assuming `count` value is 4):
     
     ```
       2016-09-03 11:33:01 Warp speed to 4.
       2016-09-03 11:33:01 Moving on now. 
     ```
     On success it will return the number of characters written; on error
     it will return -1. 
     
  - `FILE *logopen(char *fname, char *mode);`
    sets the logging file (i.e. the 
    file the messages will be printed to). The `mode` parameters can be `"w"`,
    to start a new file, or `"a"` to append to an existing file. If anything
    else is passed as parameter, it will default to `"a"`.
    On success, the function returns the pointer to the log file. On failure
    it will return NULL and the log file will be reverted to `stderr`.
    To signal that the log file has been opened, the message `"LOG START"` is
    printed.
    
  - `void logclose(void);`
    Closes the current log files and revert back to `stderr`. Before closing it,
    the message `"LOG STOP"` is printed. Together with the `"LOG START"` message
    printed on opening the file, this is an easy way to determine how much time
    passed between 

### Testing

A simple way to create unit test (or applying
[Test Driven Development](https://en.wikipedia.org/wiki/Test-driven_development))
is to use the following functions:

  - `int logcheck(int test);`
    evaluates the test expression (whose result must be non-zero if the test passed
    and zero if it failed) and writes the result on the log file.

  - `void logassert(int test);`
    same as logcheck() but exits (with `abort()`)on failure. Should be used when
    the test could reveal that something went horribly wrong.

For eaxmple, the following code:

   ```C
     x = get_number_from(outer_space);
     y = get_number_from(inner_space);
     logcheck(x > 100.);
     logassert(y > 0.);
     logcheck(x/y > 0.33);
   ```
will print, assuming `x=35.2` and `y=3.0`:

   ```
     2016-09-03 11:33:01 CHK FAIL (x > 100.) spacing.c 34
     2016-09-03 11:33:01 CHK PASS (y > 0.) spacing.c 35
     2016-09-03 11:33:01 CHK PASS (x/y > 0.33) spacing.c 36
   ```

If, instead, we had `x=145.7` and `y=0.0`, we would have had:

   ```
     2016-09-03 11:33:01 CHK PASS (x > 100.) spacing.c 34
     2016-09-03 11:33:01 CHK FAIL (y > 0.) spacing.c 35
     2016-09-03 11:33:01 CHK EXITING ON FAIL
   ```
    
and the program would have been terminated with exit code 1 to  prevent the
division by zero to happen on the next line.

The fact that `logcheck()` returns the result of the test can be used to get more
information in case of a failure:

   ```C
     y = get_number_from(inner_space);
     if (!logcheck(x > 100.)) {
       logprintf("          x: %.1f",x);
     }
     logassert(y != 0.);
     logcheck(x/y > 0.33);
   ```

   will result in:
    
   ```
      2016-09-03 11:33:01 CHK FAIL (x > 100.) spacing.c 34
      2016-09-03 11:33:01           x: 35.2
   ```

### Debugging

While using a symbolic debugger like `gdb` is the *"right thing to do"*, there
are times when you need some more traces of the execution of your program to really
get what is going on (and catch that damned bug!). Insted of using `printf()` or
`logprintf()` (which you can always do, of course) you can use the 

  - `int logdebug(char *format, ...);`
  
function that behaves exactly as the `logprintf()` function but will do nothing if
the `NDEBUG` symbol is defined.

This way you can easily differentiate between normal log messages and messages that
are there just for debugging purposes.

### Temporary disabled functions

There are times when you don't want some log message to be generated or some check
to be performed. This is esepcially true for debugging messages that tend to fill
up the log file with a lot of information you may no longer need.

You could delete the call to the logging function altogether but you might need
them again afterward and it would be a waste to have to write them again.

For cases like this, the following functions are defined:

   - `int   _logprintf(char *format, ...);`
   - `int   _logdebug(char *format, ...);`
   - `int   _logcheck(int test);`
   - `void  _logassert(int test);`
   - `FILE *_logopen(char *fname, char *mode);`
   - `void  _logclose(void);`

that do nothing (`_logcheck()` will always return 1 as if the test passed).

Technically speaking I'm in violation of the C standard here (section 7.1.3):
identifiers starting with underscore are reserved for use as identifiers
with file scope.  I've tried many times to get rid of the initial underscore
(e.g. by using `X` or `X_`) but i still believe this is the best choice and
the worst that could happen is that they clash with some other library.
By the way, this is a risk that still must be taken into consideration for
any other identifier, so I'm not feeling particularly pressed on changing it.
  
** ]]] */


#define logprintf(...)  utl_log_printf(__VA_ARGS__)

#define logopen(f,m)    utl_log_open(f,m)
#define logclose()      utl_log_close("LOG STOP")
                        
#ifndef NDEBUG          
#define logcheck(e)     utl_log_check(!!(e),#e,__FILE__,__LINE__)
#define logassert(e)    utl_log_assert(!!(e),#e,__FILE__,__LINE__)
#define logdebug        logprintf
#else                   
#define logcheck(e)     utl_ret(1)
#define logassert(e)    ((void)0)
#define logdebug(...)   utl_ret(0)
#endif                  
                        
#define _logprintf(...) utl_ret(0)
#define _logdebug(...)  utl_ret(0)
#define _logcheck(...)  utl_ret(1)
#define _logassert(...) ((void)0)
#define _logopen(f,m)   utl_retptr(NULL)
#define _logclose()     utl_ret(0)

int   utl_log_printf(char *format, ...);
FILE *utl_log_open(char *fname, char *mode);
int   utl_log_close(char *msg);
int   utl_log_check(int res, char *test, char *file, int32_t line);
void  utl_log_assert(int res, char *test, char *file, int32_t line);

#ifdef UTL_MAIN

static FILE *utl_log_file = NULL;

int utl_log_close(char *msg)
{
  int ret = 0;
  if (msg) logprintf(msg);
  if (utl_log_file && utl_log_file != stderr) ret = fclose(utl_log_file);
  utl_log_file = NULL;
  return ret;
}

FILE *utl_log_open(char *fname, char *mode)
{
  char md[2];
  md[0] = (mode && *mode == 'w')? 'w' : 'a';
  md[1] = '\0';
  utl_log_close(NULL);
  utl_log_file = fopen(fname,md);
  logprintf("LOG START");
  return utl_log_file;
}

int utl_log_printf(char *format, ...)
{
  va_list    args;
  char       log_tstr[32];
  time_t     log_time;
  int        ret = 0;
  struct tm *log_time_tm;
  
  if (!utl_log_file) utl_log_file = stderr;
  if (time(&log_time) < 0) ret = -1;
  if (ret >= 0 && !(log_time_tm = localtime(&log_time))) ret = -1;
  if (ret >= 0 && !strftime(log_tstr,32,"%Y-%m-%d %H:%M:%S",log_time_tm)) ret =-1;
  if (ret >= 0) ret = fprintf(utl_log_file,"%s ",log_tstr);
  if (ret >= 0 && fflush(utl_log_file)) ret = -1;
  va_start(args, format);
  if (ret >= 0) ret = vfprintf(utl_log_file, format, args);
  va_end(args);
  if (ret >= 0 && (fputc('\n',utl_log_file) == EOF)) ret = -1;
  if (ret >= 0 && fflush(utl_log_file)) ret = -1;
  return ret;
}

int utl_log_check(int res, char *test, char *file, int32_t line)
{
  logprintf("CHK %s (%s) %s:%d", (res?"PASS":"FAIL"), test, file, line);
  return res;
}

void utl_log_assert(int res, char *test, char *file, int32_t line)
{
  if (!utl_log_check(res,test,file,line)) {
    logprintf("CHK EXITING ON FAIL");
    logclose();
    abort();
  }
}

#endif

#endif /* UTL_NOLOG */

#ifndef UTL_NOFSM
/*

**                     ____
**                    /  __)
**                  _/  /_ _____   ______ 
**                 |   __//  ___) /      \
**                 /  /   \___ \ /  / /  /
**                /  /   (_____//__/_/__/ 
**               /  /
**               \_/   


 [[[

## Finite State Machines

Many piece of software are better understood (and desigend) as
[Finite State Machines](https://en.wikipedia.org/wiki/Finite-state_machine).

There are many ways to represent a FSM in C. Most common methods are:
  - Using a `state` variable and a `switch{...}` within a loop. 
  - Using a table of pointers to functions that will be invoked to
    perform the actions and move from a state to another.
    
These methods, however, hide the structure of the FSM itself which is better
understood as a graph with nodes representing states and arcs representing 
transitions. 

The following macros implement a technique explained by Tim Cooper in the
the article [*Goto? Yes Goto!*](http://ftp.math.utah.edu/pub/tex/bib/complang.html#Cooper:1991:GYG)
published on the May 1991 issue of the *Computer Language* magazine.

The main advantage is to directly implement the FSM transition diagram
to the point that is extremely easy to draw the diagram from the code (which 
is not an easy task with the other methods).

The code for each state (which performs the actions for that state,
read the next input, etc. ) is enclosed in a `fsmSTATE(statename)` macro;
to move from a state to another the `fsmGOTO(statename)` is used.

The following is an example to show how easy is to relate the code to 
the transition diagram.

   ```C
     fsm {
       fsmSTATE(start_of_the_day) {
         num_cust = 0;
         fsmGOTO(gate_closed);
       }
       
       fsmSTATE(gate_closed) {
         if (ticket valid(getticket())) fsmGOTO(gate_opened);
         if (time_to_close() fsmGOTO(closed_for_the_day);
         fsmGOTO(gate_closed);          
       }
       
       fsmSTATE(gate_opened) {
         if (customer_passed()) {
           num_cust++;
           if (num_cust == 100) fsmGOTO(closed_for_the day);
           if (time_to_close()) fsmGOTO(closed_for_the day);
           fsmGOTO(gate_closed);
         }
         fsmGOTO(gate_opened);
       }
       
       fsmSTATE(closed_for_the_day) {
         printf("%d customer served", num_cust);
       }
     }
   ```

   ``` 
                               ,--------------.      ,----,
                              V                \    V    /
     start_of_the_day --->  gate_closed  ---> gate_opened ---,
                               \   \            ^           /
                                \   `-----------'          /
                                 \                        V
                                  `------> closed_for_the_day
   ```

The original article had many suggestions on how to extended this
basic idea. However, after so many years, I still think that this
is the best way to represent FSM in C code.

** ]]] */

#define fsm           
#define fsmGOTO(x)    goto fsm_state_##x
#define fsmSTATE(x)   fsm_state_##x :

#endif

/* ---------------------------- */

/*  .% Traced memory
**  ================

**      ______   _____   ______ 
**     /      \ / ___ \ /      \
**    /  / /  //   ___//  / /  /
**   /__/_/__/ \_____//__/_/__/ 
**   
**   
*/

#define memINVALID    -2
#define memOVERFLOW   -1
#define memVALID       0
#define memNULL        1

#ifdef UTL_MEMCHECK

void  *utl_malloc   (size_t size, char *file, int32_t line );
void  *utl_calloc   (size_t num, size_t size, char *file, int32_t line);
void  *utl_realloc  (void *ptr, size_t size, char *file, int32_t line);
void   utl_free     (void *ptr, char *file, int32_t line );
void  *utl_strdup   (void *ptr, char *file, int32_t line);
                    
int    utl_check    (void *ptr,char *file, int32_t line);
size_t utl_mem_used (void);


#ifdef UTL_MAIN

static char  *utl_BEG_CHK = "\xBE\xEF\xF0\x0D";
static char  *utl_END_CHK = "\xDE\xAD\xC0\xDA";
static char  *utl_CLR_CHK = "\xDE\xFA\xCE\xD0";
static size_t utl_mem_allocated;

typedef struct {
   size_t size;
   char   chk[4];
   char   blk[4];
} utl_mem_t;

#define utl_mem(x) ((utl_mem_t *)((char *)(x) -  offsetof(utl_mem_t, blk)))

int utl_check(void *ptr,char *file, int32_t line)
{
  utl_mem_t *p;
  
  if (ptr == NULL) return memNULL;
  p = utl_mem(ptr);
  if (memcmp(p->chk,utl_BEG_CHK,4)) { 
    logprintf("MEM Invalid or double freed %p (%lu %s:%d)",p->blk,
                                               utl_mem_allocated, file, line);     
    return memINVALID; 
  }
  if (memcmp(p->blk+p->size,utl_END_CHK,4)) {
    logprintf("MEM Boundary overflow %p [%lu] (%lu %s:%d)",
                              p->blk, p->size, utl_mem_allocated, file, line); 
    return memOVERFLOW;
  }
  logprintf("MEM Valid pointer %p (%lu %s:%d)",ptr, utl_mem_allocated, file, line); 
  return memVALID; 
}

void *utl_malloc(size_t size, char *file, int32_t line )
{
  utl_mem_t *p;
  
  if (size == 0) logprintf("MEM Requesto for 0 bytes (%lu %s:%d)",
                                                utl_mem_allocated, file, line);
  p = malloc(sizeof(utl_mem_t) +size);
  if (p == NULL) {
    logprintf("MEM Out of Memory (%lu %s:%d)",utl_mem_allocated, file, line);
    return NULL;
  }
  p->size = size;
  memcpy(p->chk,utl_BEG_CHK,4);
  memcpy(p->blk+p->size,utl_END_CHK,4);
  utl_mem_allocated += size;
  logprintf("MEM Allocated %p [%lu] (%lu %s:%d)",p->blk,size,utl_mem_allocated,file,line);
  return p->blk;
}

void *utl_calloc(size_t num, size_t size, char *file, int32_t line)
{
  void *ptr;
  
  size = num * size;
  ptr = utl_malloc(size,file,line);
  if (ptr)  memset(ptr,0x00,size);
  return ptr;
}

void utl_free(void *ptr, char *file, int32_t line)
{
  utl_mem_t *p=NULL;
  
  switch (utl_check(ptr,file,line)) {
    case memNULL  :    logprintf("MEM free NULL (%lu %s:%d)", 
                                                utl_mem_allocated, file, line);
                       break;
                          
    case memOVERFLOW : logprintf( "MEM Freeing an overflown block  (%lu %s:%d)", 
                                                           utl_mem_allocated, file, line);
    case memVALID :    p = utl_mem(ptr); 
                       memcpy(p->chk,utl_CLR_CHK,4);
                       utl_mem_allocated -= p->size;
                       if (p->size == 0)
                         logprintf("MEM Freeing a block of 0 bytes (%lu %s:%d)", 
                                             utl_mem_allocated, file, line);

                       logprintf("MEM free %p [%lu] (%lu %s %d)", ptr, 
                                 p?p->size:0,utl_mem_allocated, file, line);
                       free(p);
                       break;
                          
    case memINVALID :  logprintf("MEM free an invalid pointer! (%lu %s:%d)", 
                                                utl_mem_allocated, file, line);
                       break;
  }
}

void *utl_realloc(void *ptr, size_t size, char *file, int32_t line)
{
  utl_mem_t *p;
  
  if (size == 0) {
    logprintf("MEM realloc() used as free() %p -> [0] (%lu %s:%d)",
                                                      ptr,utl_mem_allocated, file, line);
    utl_free(ptr,file,line); 
  } 
  else {
    switch (utl_check(ptr,file,line)) {
      case memNULL   : logprintf("MEM realloc() used as malloc() (%lu %s:%d)", 
                                             utl_mem_allocated, file, line);
                          return utl_malloc(size,file,line);
                        
      case memVALID  : p = utl_mem(ptr); 
                       p = realloc(p,sizeof(utl_mem_t) + size); 
                       if (p == NULL) {
                         logprintf("MEM Out of Memory (%lu %s:%d)", 
                                          utl_mem_allocated, file, line);
                         return NULL;
                       }
                       utl_mem_allocated -= p->size;
                       utl_mem_allocated += size; 
                       logprintf("MEM realloc %p [%lu] -> %p [%lu] (%lu %s:%d)", 
                                       ptr, p->size, p->blk, size, 
                                       utl_mem_allocated, file, line);
                       p->size = size;
                       memcpy(p->chk,utl_BEG_CHK,4);
                       memcpy(p->blk+p->size,utl_END_CHK,4);
                       ptr = p->blk;
                       break;
    }
  }
  return ptr;
}

void *utl_strdup(void *ptr, char *file, int32_t line)
{
  char *dest;
  size_t size;
  
  if (ptr == NULL) {
    logprintf("MEM strdup NULL (%lu %s:%d)", utl_mem_allocated, file, line);
    return NULL;
  }
  size = strlen(ptr)+1;

  dest = utl_malloc(size,file,line);
  if (dest) memcpy(dest,ptr,size);
  logprintf("MEM strdup %p [%lu] -> %p (%lu %s:%d)", ptr, size, dest, 
                                                utl_mem_allocated, file, line);
  return dest;
}
#undef utl_mem

size_t utl_mem_used() {return utl_mem_allocated;}

#endif

/*************************************/

#define malloc(n)     utl_malloc(n,__FILE__,__LINE__)
#define calloc(n,s)   utl_calloc(n,s,__FILE__,__LINE__)
#define realloc(p,n)  utl_realloc(p,n,__FILE__,__LINE__)
#define free(p)       utl_free(p,__FILE__,__LINE__)

#ifdef strdup
#undef strdup
#endif
#define strdup(p)     utl_strdup(p,__FILE__,__LINE__)

#define memcheck(p)   utl_check(p,__FILE__, __LINE__)
#define memused()     utl_mem_used()

#else /* UTL_MEMCHECK */

#define memcheck(p)     memVALID
#define memused()       0

#endif /* UTL_MEMCHECK */

/*************************************/

#ifndef UTL_NOVEC

#define vec_MIN_ELEM 16
#define vec_MAX_ELEM (1<<24)

typedef struct vec_s {
  uint32_t  fst; 
  uint32_t  lst; 
  uint32_t  max;
  uint32_t  cnt;
  uint16_t  esz;  /* each element max size: 64K */
  uint16_t  flg;
  int     (*cmp)(void *, void *);
  uint8_t  *vec; 
  uint32_t  elm;
} vec_s, *vec_t;

#define vecset(type,v,i,e)   (*((type *)(&(v->elm))) = (e), (type *)utl_vec_set(v,i))
#define vecins(type,v,i,e)   (*((type *)(&(v->elm))) = (e), (type *)utl_vec_ins(v,i))
#define vecadd(type,v,e)     (*((type *)(&(v->elm))) = (e), (type *)utl_vec_ins(v,(v)->cnt))

#define vecget(type,v,i)     (type *)utl_vec_get(v,i)
#define vec(type,v)          ((type *)(v->vec))

#define vecnew(type)         utl_vec_new(sizeof(type))
#define vecfree(v)           utl_vec_free(v)
#define vecdel(v,i)          utl_vec_del(v,i)

#define veccount(v) ((v)->cnt)
#define vecmax(v)   ((v)->max)

#define vecread(v,i,n,f)  utl_vec_read(v,i,n,f)
#define vecwrite(v,i,n,f) utl_vec_write(v,i,n,f)

#define vecsort(v,c)           utl_vec_sort(v,c)

#define vecsearch(type,v,e)      (*((type *)(&(v->elm))) = (e), (type *)utl_vec_search(v))

#define vecSORTED 0x0001

#define vecissorted(v) ((v)->flg &   vecSORTED)
#define vecsorted(v)   ((v)->flg |=  vecSORTED)
#define vecunsorted(v) ((v)->flg &= ~vecSORTED)

void *utl_vec_set(vec_t v, uint32_t i);
void *utl_vec_ins(vec_t v, uint32_t i);
void *utl_vec_get(vec_t v, uint32_t i);

int16_t utl_vec_del(vec_t v,  uint32_t i);
int16_t utl_vec_delrange(vec_t v, uint32_t i,  uint32_t j);

vec_t utl_vec_new(uint16_t esz);
vec_t utl_vec_free(vec_t v);

size_t utl_vec_read(vec_t v,uint32_t i, size_t n,FILE *f);
size_t utl_vec_write(vec_t v, uint32_t i, size_t n, FILE *f);

void utl_vec_sort(vec_t v, int (*cmp)(void *, void *));
void *utl_vec_search(vec_t v);

#define stk_t                 vec_t
#define stknew(type)          vecnew(type)
#define stkfree(v)            vecfree(v)
#define stkpush(type,v,e)     vecset(type,v,(v)->cnt,e)
#define stktop(type,v,d)      ((v)->cnt? vecget(type,v,(v)->cnt  ,d) : d)
#define stkisempty(v)         ((v)->cnt == 0)
#define stkcount(v)           ((v)->cnt)

#define stkdrop(v)            ((v)->cnt? (v)->cnt-- : 0)
#define stkdup(v)             utl_stk_dup(v)
#define stkrot(v)             utl_stk_rot(v)

#define que_t                 vec_t

#define buf_t                 vec_t
#define bufnew()              vecnew(char)
#define bufaddc(b,c)           vecadd(char,b,c)
#define bufsetc(b,i,c)        vecset(char,b,i,c)
#define bufinsc(b,i,c)        utl_buf_insc(b,i,c)
#define bufinss(b,i,s)        utl_buf_inss(b,i,s)
#define bufsets(b,i,s)        utl_buf_sets(b,i,s)

#define bufgetc(b,i)          utl_buf_get(b,i)
#define bufdel(b,i,j)         utl_buf_del(b,i,j)
#define bufread(b,i,n,f)      utl_buf_read(b,i,n,f)
#define bufreadln(b,i,f)      utl_buf_readln(b,i,f)
#define bufreadall(b,i,f)     utl_buf_readall(b,i,f)
#define bufwrite(b,i,n,f)     vecwrite(b,i,n,f)
#define buf(b)                vec(char, b)
#define buflen(b)             veccount(b)

char utl_buf_get(buf_t b, uint32_t n);
size_t utl_buf_readall(buf_t b, uint32_t i, FILE *f);
size_t utl_buf_read(buf_t b, uint32_t i, uint32_t n, FILE *f) ;
char *utl_buf_readln(buf_t b, uint32_t i, FILE *f);
char *utl_buf_sets(buf_t b, uint32_t i, char *s);
char *utl_buf_inss(buf_t b, uint32_t i, char *s);
char *utl_buf_insc(buf_t b, uint32_t i, char c);
int16_t utl_buf_del(buf_t b, uint32_t i,  uint32_t j);

#ifdef UTL_MAIN

static int16_t utl_vec_makeroom(vec_t v,uint32_t n)
{
  uint32_t new_max = 1;
  uint8_t *new_vec = NULL;

  if (n < v->max) return 1;
  new_max = v->max;
  while (new_max <= n) new_max += (new_max / 2);  /*  (new_max *= 1.5) instead of (new_max *= 2) */
  new_vec = realloc(v->vec, new_max * v->esz);
  if (!new_vec) return 0;
  v->vec = new_vec;  v->max = new_max;
  return 1;
}

static int16_t utl_vec_makegap(vec_t v, uint32_t i, uint32_t l)
{
 
  if (!utl_vec_makeroom(v,i+l+1)) return 0;
  
  /*
  **                    __l__
  **  ABCDEFGH       ABC-----DEFGH
  **  |  |    |      |  |    |    |
  **  0  i    cnt    0  i   i+l   cnt+l
  */
  
  if (i < v->cnt) {
    memmove(v->vec + (i+l)*v->esz,  v->vec + i*v->esz,  (v->cnt-i)*v->esz);
    v->cnt += l;
  }
  return 1;
}


static int16_t utl_vec_delgap(vec_t v, uint32_t i, uint32_t l)
{
  
  /*
  **       __l__                                
  **    ABCdefghIJKLM              ABCIJKLM     
  **    |  |    |    |             |  |    |    
  **    0  i   i+l   cnt           0  i    cnt-l  
  */
  _logdebug("DELGAP: %d %d",i,l);
  if (i < v->cnt) {
    if (i+l >= v->cnt) v->cnt = i; /* Just drop last elements */
    else {
      memmove(v->vec + i*v->esz,  v->vec + (i+l)*v->esz,  (v->cnt-(i+l))*v->esz);
      v->cnt -= l;
    }
  }
  return 1;
}


vec_t utl_vec_new(uint16_t esz)
{
  vec_t v = NULL;
  uint32_t sz = sizeof(vec_s)+(esz-sizeof(uint32_t));
  
  v = malloc(sz);
  if (v) {
    memset(v,0,sz);
    v->esz = esz;
    v->max = vec_MIN_ELEM;
    v->vec = malloc(v->max * esz);
    vecunsorted(v);
    if (!v->vec) { free(v); v = NULL;}
  }
  return v;
}

vec_t utl_vec_free(vec_t v)
{
  if (v) { 
    if (v->vec) free(v->vec);
    v->vec = NULL;
    free (v);
  }
  return NULL;
}

void *utl_vec_get(vec_t v, uint32_t i)
{
  uint8_t *elm=NULL;
  
  if (i < v->cnt) {
    elm = v->vec + (i*v->esz);
    memcpy(&(v->elm),elm,v->esz);
  }
  return elm;
}

void *utl_vec_set(vec_t v, uint32_t i)
{
  uint8_t *elm=NULL;

  if (utl_vec_makeroom(v,i)) {
    elm = v->vec + (i*v->esz);
    memcpy(elm, &(v->elm), v->esz);
    if (i>=v->cnt) v->cnt = i+1;
    vecunsorted(v);
  }
  return elm;
}

void *utl_vec_ins(vec_t v, uint32_t i)
{
  uint8_t *elm=NULL;

  if (utl_vec_makegap(v,i,1)) elm = utl_vec_set(v,i);
  vecunsorted(v);
  return elm;
}

int16_t utl_vec_del(vec_t v, uint32_t i)
{
  return utl_vec_delgap(v,i,1);
}

int16_t utl_vec_delrange(vec_t v, uint32_t i,  uint32_t j)
{
  if (j<i) return 0;
  return utl_vec_delgap(v,i,j-i+1);  
}

size_t utl_vec_read(vec_t v,uint32_t i, size_t n,FILE *f)
{
  if (utl_vec_makeroom(v,i+n+1)) {
    n = fread(v->vec + (i*v->esz), v->esz, n, f);
    if (v->cnt < i+n) v->cnt = i+n;
    vecunsorted(v);
  }
  return 0;
}

size_t utl_vec_write(vec_t v, uint32_t i, size_t n, FILE *f)
{
  if (i+n> v->max) n = v->max - i;
  return fwrite(v->vec+(i*v->esz),v->esz,n,f);
}

void utl_vec_sort(vec_t v, int (*cmp)(void *, void *))
{
  if (vecissorted(v)) return;
  if (cmp) v->cmp = cmp;
  if (v->cmp) {
    if (v->cnt > 1) qsort(v->vec,v->cnt,v->esz,(int (*)(const void *, const void *))(v->cmp));  
    vecsorted(v);
  }
}

void *utl_vec_search(vec_t v)
{
  if (v->cmp) {
    utl_vec_sort(v,NULL);
    return bsearch(&(v->elm),v->vec,v->cnt,v->esz,(int (*)(const void *, const void *))(v->cmp));
  }
  return NULL;
}

char utl_buf_get(buf_t b, uint32_t n)
{
  char *s = vecget(char,b,n);
  return s?*s:'\0';
}

size_t utl_buf_read(buf_t b, uint32_t i, uint32_t n, FILE *f)
{
  size_t r = vecread(b,i,n,f);
  bufsetc(b,i+n,'\0');
  b->cnt = i+n;
  return r;
}

size_t utl_buf_readall(buf_t b, uint32_t i, FILE *f)
{
  uint32_t n,pos;
  size_t ret;
  pos = ftell(f);
  fseek(f,0,SEEK_END);
  n = ftell(f);
  fseek(f,pos,SEEK_SET);
  ret = bufread(b,i,n-pos,f);
  bufsetc(b,i+(n-pos),'\0');
  b->cnt = i+(n-pos);
  return ret;
}

char *utl_buf_readln(buf_t b, uint32_t i, FILE *f)
{
  int c;
  uint32_t n = i;
  
  if (!b || !f || feof(f)) return NULL;
  while ((c=fgetc(f)) != EOF) {
    if (c == '\r') {
      c = fgetc(f);
      if (c != '\n') ungetc(c,f);
      c = '\n';
    }
    bufsetc(b,i++,c);
    if (c=='\n') break;
  }
  if (i==n) return NULL;
  if (bufgetc(b,i-1) != '\n') bufsetc(b,i++,'\n');
  bufsetc(b,i,'\0');
  b->cnt = i;
  return buf(b)+n;
}

char *utl_buf_sets(buf_t b, uint32_t i, char *s)
{
  char *r = buf(b)+i;
  while (*s) bufsetc(b,i++,*s++);
  bufsetc(b,i,'\0');
  b->cnt=i;
  return r;
}

char *utl_buf_inss(buf_t b, uint32_t i, char *s)
{
  uint32_t n;
  char *p;
  if (!s || !b) return NULL;
  n = strlen(s);
  
  if (n == 0 || !utl_vec_makegap(b, i, n)) return NULL;
    
  p = buf(b)+i;
  while (*s) *p++ = *s++;

  bufsetc(b,b->cnt,'\0');  b->cnt--;
  return buf(b)+i;
}

char *utl_buf_insc(buf_t b, uint32_t i, char c)
{
  if (!utl_vec_makegap(b, i, 1)) return NULL;
    
  buf(b)[i] = c;
  bufsetc(b,b->cnt,'\0');  b->cnt--;
  return buf(b)+i;
}

int16_t utl_buf_del(buf_t b, uint32_t i,  uint32_t j)
{
  int16_t r;
  _logdebug("len:%d",b->cnt);
  r = utl_vec_delgap(b, i, j-i+1);
  _logdebug("len:%d",b->cnt);
  if (r) {
    bufsetc(b,b->cnt,'\0');
    b->cnt--;
  }
  return r;
}

#endif /* UTL_MAIN */

#endif  /* UTL_NOVEC */

#ifndef UTL_NOPMX

/*
**        ______   ______ ___  ___ 
**       /  __  \ /      \\  \/  /  
**      /  /_/  //  / /  / \    \  
**     /  _____//__/_/__/ /__/\__\ 
**    /  /
**   (__/
*/


#define utl_pmx_MAXCAPT 16

utl_extern(char     *utl_pmx_capt[utl_pmx_MAXCAPT][2]   , {{0}} );
utl_extern(uint8_t   utl_pmx_capnum                     ,  0  );
utl_extern(char     *utl_pmx_error                      ,  NULL );

#define pmxsearch(r,t)  utl_pmx_search(r,t)
#define pmxstart(n)    (utl_pmx_capt[n][0])
#define pmxend(n)      (utl_pmx_capt[n][1])
#define pmxcount()     (utl_pmx_capnum)
#define pmxlen(n)       utl_pmx_len(n)
#define pmxerror()     (utl_pmx_error)

char *utl_pmx_search(char *pat, char *txt);
size_t utl_pmx_len(uint8_t n);


#ifdef UTL_MAIN
#if 0
static int(*utl_pmx_ext)(char *r, char *t) = NULL;
#endif

typedef struct {
  char *txt;
  char *pat;
  int32_t min_n;
  int32_t max_n;
  int16_t inv;
} utl_pmx_state_s;

utl_pmx_state_s utl_pmx_stack[utl_pmx_MAXCAPT];
uint8_t utl_pmx_stack_cnt = 0;

#define utl_pmx_set_paterror(t) do {if (utl_pmx_error == NULL) {utl_pmx_error = t;}} while (0)

static int utl_pmx_utf8 = 0;

#define utl_pmx_FAIL       goto fail

#define utl_pmx_newcap(t) do {                                       \
                            if (utl_pmx_capnum < utl_pmx_MAXCAPT) {  \
                              utl_pmx_capt[utl_pmx_capnum][0] =      \
                              utl_pmx_capt[utl_pmx_capnum][1] = (t); \
                              utl_pmx_capnum++;                      \
                            }                                        \
                          } while(0)


static uint32_t utl_pmx_neg = 0;
#define utl_pmx_is_negated()   (utl_pmx_neg &   (1<<utl_pmx_captop()))
#define utl_pmx_clr_negated()  (utl_pmx_neg &= ~(1<<utl_pmx_captop()))
#define utl_pmx_set_negated()  (utl_pmx_neg |=  (1<<utl_pmx_capnum))

static uint8_t utl_pmx_capstk_num = 0;
static uint8_t utl_pmx_capstk[utl_pmx_MAXCAPT];

#define utl_pmx_cappush(n) (utl_pmx_capstk[utl_pmx_capstk_num++] = n)
#define utl_pmx_captop()   (utl_pmx_capstk[utl_pmx_capstk_num-1])

static uint8_t utl_pmx_cappop()  {return utl_pmx_capstk[--utl_pmx_capstk_num];}

size_t utl_pmx_len(uint8_t n) {return pmxend(n)-pmxstart(n);}

static int utl_pmx_get_utf8(char*t, int32_t *ch)
{
  int len = 0;
  int32_t val;
  val=0;
  
       if ((*t & 0x80) == 0x00) { val =  *t        ; len = 1; }
  else if ((*t & 0xE0) == 0xC0) { val = (*t & 0x1F); len = 2; }
  else if ((*t & 0xF0) == 0xE0) { val = (*t & 0x0F); len = 3; }
  else if ((*t & 0xF8) == 0xF0) { val = (*t & 0x07); len = 4; }
  
  switch (len) {  /* WARNING: falls through! */
    case 4: if ((*++t & 0xC0) != 0x80) {len=0; break;}
            val = (val << 6) | (*t & 0x3F);
    case 3: if ((*++t & 0xC0) != 0x80) {len=0; break;}
            val = (val << 6) | (*t & 0x3F);
    case 2: if ((*++t & 0xC0) != 0x80) {len=0; break;}
            val = (val << 6) | (*t & 0x3F);
  }
  if (len && ch) *ch = val;
  return len;
}

// Returns the length in bytes of the character or 0 if it is '\0'
static int32_t utl_pmx_nextch(char *t, int32_t *c_ptr)
{
  int32_t len = 0;
  
  if (utl_pmx_utf8) len = utl_pmx_get_utf8(t, c_ptr);
  else if ((*c_ptr = (uint8_t)(*t))) len = 1;
  
  return len;
}

static int32_t utl_pmx_gethex(char *pat, int32_t *c_ptr)
{
  int32_t ch =0;
  int32_t len =0;
  
  while (*pat && (*pat != '>') && !isxdigit(*pat)) {len++; pat++;}
  while (*pat) {
         if ('0'<= *pat && *pat <= '9') ch = (ch << 4) + (*pat -'0');
    else if ('A'<= *pat && *pat <= 'F') ch = (ch << 4) + (*pat -'A'+10);
    else if ('a'<= *pat && *pat <= 'f') ch = (ch << 4) + (*pat -'a'+10);
    else break;
    len++; pat++;
  }
  *c_ptr = ch;
  return len;  
}

static int utl_pmx_isin(char *pat, char *pat_end, int32_t ch, int32_t (*nxt)(char *, int32_t *))
{
  int32_t c1,c2;
  int32_t len;
  
  while (pat<pat_end) {
    c2=0; c1=0;
    len = nxt(pat, &c1);
    if (ch == c1) return 1;
    pat +=len; 
    if (*pat == '-') {
      len = nxt(++pat, &c2);
      if (c1 <= ch && ch <= c2) return 1;
      pat += len; 
    }
  }
  return 0;
}

#define utl_pmx_isin_chars(p,e,c) utl_pmx_isin(p,e,c,utl_pmx_nextch)
#define utl_pmx_isin_codes(p,e,c) utl_pmx_isin(p,e,c,utl_pmx_gethex)

#if 0
static int utl_pmx_quoted(char *r, char *t, char *s)
{
  int ret = 0;
  int n;
  
  n=s-r;
  int32_t c_esc='\\';
  int32_t c_beg='"';
  int32_t c_end='"';
  int32_t ch;
  
  ch= *t;
  if (ch == '\0') return 0;
  _logdebug("QUOTED: [%s] [%s] [%s] (%d)",r,t,s,s-r);
  n=s-r;
  if (n <= 1) { /* just <q> or <q\\> */
         if (ch == '"')  {c_esc='\\';  c_beg=ch;  c_end='"';}
    else if (ch == '\'') {c_esc='\\';  c_beg=ch;  c_end='\'';}
    else if (ch == '(')  {c_esc='\0';  c_beg=ch;  c_end=')';}
    else if (ch == '[')  {c_esc='\0';  c_beg=ch;  c_end=']';}
    else if (ch == '{')  {c_esc='\0';  c_beg=ch;  c_end='}';}
    else if (ch == '<')  {c_esc='\0';  c_beg=ch;  c_end='>';}
  }
  if (n == 1) { /* escape character provided */
    c_esc = r[0];
  }
  else if (n == 2) { /* start/end but no escape character */
    c_esc = '\0'; c_beg = r[0]; c_end = r[1];
  }
  else if (n >= 3) { /* escape/start/end */
    c_esc = r[0]; c_beg = r[1]; c_end = r[2];    
  }
  _logdebug("QUOTED: '%ch' '%c' '%c'",c_esc?c_esc:' ',c_beg,c_end);
  if (*t == c_beg) {
    for (t++, n=0, ret=2; *t;  t++, ret++) {
      if (*t == c_esc && t[1] != '\0') {
        t++;
        ret++;
      }
      else if (*t == c_end) {
        if (n > 0) n--;
        else break;
      }
      else if (*t == c_beg) n++;
    }
    if (*t == '\0') ret = 0;
  }
  return ret;
}
#endif 

static int utl_pmx_class(char **pat_ptr, char **txt_ptr)
{
  int inv = 0;
  
  char *pat = *pat_ptr;
  char *txt = *txt_ptr;
  char *pat_end;
  
  int32_t len = 0;
  int32_t n = 0;
  int32_t min_n=0;
  int32_t max_n=0;
  int32_t ch;
  
  _logdebug("class:[%s][%s]",pat,txt);
                
  pat++;  /* skip the '<' */
  
  // {{ Find the end of the pattern
  pat_end=pat;
  while (*pat_end && *pat_end != '>') pat_end++;
  if (pat_end[1] == '>') pat_end++; /* allow just one '>' at the end of a pattern */
  //}}
  
  // {{ Get how many times the match has to occur.
  //      Examples: <2-4l> matches 2,3 or 4 lower case letters
  //                <-4d>  matches from 0 to 4 decimal digits
  //                <2-s>  matches at 2 spaces or more
  //                <3u>   matches exactly 3 upper case letters
  while ('0' <= *pat && *pat <= '9') 
    min_n = (min_n*10) + (*pat++ - '0');
  
  if (*pat == '-') {
    pat++;
    while ('0'<=*pat && *pat <= '9') 
      max_n = (max_n*10) + (*pat++ - '0'); 
    
    if (max_n == 0) max_n = INT32_MAX;
  }
  if (max_n < min_n) max_n = min_n;
  
  if (max_n == 0) {
    switch (*pat) {
      case '*' : min_n = 0; max_n = INT32_MAX; pat++; break;
      case '+' : min_n = 1; max_n = INT32_MAX; pat++; break;
      case '?' : min_n = 0; max_n = 1;         pat++; break;
      default  : min_n = 1; max_n = 1;                break;
    }
  }
  // }}
  
  if (*pat == '!') {inv = 1; pat++;}
  
  // {{ Matches a pattern n times
  #define utl_W(tst) while ((len = utl_pmx_nextch(txt,&ch)) && ((!tst) == inv) && (n<max_n)) {n++; txt+=len;}
  switch (*pat) {
    case 'a' : utl_W(isalpha(ch))               ; break;
    case 's' : utl_W(isspace(ch))               ; break;
    case 'u' : utl_W(isupper(ch))               ; break;
    case 'l' : utl_W(islower(ch))               ; break;
    case 'd' : utl_W(isdigit(ch))               ; break;
    case 'x' : utl_W(isxdigit(ch))              ; break;
    case 'w' : utl_W(isalnum(ch))               ; break;
    case 'c' : utl_W(iscntrl(ch))               ; break;
    case 'g' : utl_W(isgraph(ch))               ; break;
    case 'i' : utl_W((ch < 0x80))               ; break;
    case 'k' : utl_W((ch == ' '  || ch =='\t')) ; break;
    case 'n' : utl_W((ch == '\r' || ch =='\n')) ; break;
    case 'p' : utl_W(ispunct(ch))               ; break;
    case 'q' : utl_W(isalnum(ch))               ; break;
    case 'r' : utl_W(isprint(ch))               ; break;
    
    case '.' : utl_W(ch)                        ; break;

    case '=' : utl_W(utl_pmx_isin_chars(pat+1,pat_end,ch)); break;
    case '#' : utl_W(utl_pmx_isin_codes(pat+1,pat_end,ch)); break;
    
    case 'N' : utl_W((txt[0]=='\r'
                            ? (txt[1] == '\n'? (len++) : 1)
                            : (txt[0] == '\n'?  1 : 0)) ) ; break;
    
    case '$' : if (*txt == '\0') n=min_n; break;
    case '>' : return 0;
    
  }
  #undef utl_W
  // }}
  
  if (n < min_n) return 0;
  
  // {{ Advance pattern and matched text
  while (*pat_end == '>') pat_end++;
  *pat_ptr=pat_end;
  *txt_ptr=txt;
  // }}
  
  return 1;
}

static char *utl_pmx_alt_skip(char *pat)
{
  int paren=0;
  
  while (*pat) {
    switch (*pat++) {
      case '%': if (*pat) pat++; /* works for utf8 as well */
                break;
                
      case '(': paren++;
                utl_pmx_newcap(NULL);
                break;
                
      case ')': if (paren == 0) return (pat-1);
                paren--;
                break;
                
      case '<': while (*pat && *pat != '>') pat++;
                while (*pat == '>') pat++;
                break;
    }
  }
  return pat;
}

static char *utl_pmx_alt(char *pat)
{
  int paren=0;
  
  while (*pat) {
    switch (*pat++) {
      case '%': if (*pat) pat++; /* works for utf8 as well */
                break;
                
      case '(': paren++;
                utl_pmx_newcap(NULL);
                break;
                
      case ')': if (paren > 0) {
                  paren--;
                }
                else if (utl_pmx_capstk_num > 0) {
                  if (utl_pmx_is_negated()) {
                    utl_pmx_clr_negated();
                    return pat - 1;
                  }
                  else utl_pmx_cappop();
                } 
                else return utl_emptystring;
                break;

      case '<': while (*pat && *pat != '>') pat++;
                while (*pat == '>') pat++;
                break;
                 
      case '|': if (paren == 0) return pat;
    }
  }
  return utl_emptystring;
}

static char *utl_pmx_match(char *pat, char *txt)
{
  int32_t len;
  int32_t ch;
  int32_t c1;

  utl_pmx_neg = 0;
  utl_pmx_capnum = 0;
  utl_pmx_capstk_num = 0;   
  
  utl_pmx_cappush(utl_pmx_capnum);
  utl_pmx_newcap(txt);
  
  while (*pat) {
    logdebug("match %d [%s] [%s]",pmxcount(),pat,txt);
    c1 = 0; 
    switch (*pat) {
      case '<' : if (!utl_pmx_class(&pat,&txt)) utl_pmx_FAIL;
                 break;

      case '(' : pat++;
                 if (*pat == '|') {
                   pat++;
                   utl_pmx_set_negated();
                 }
                 if (utl_pmx_capnum < utl_pmx_MAXCAPT) {
                   utl_pmx_cappush(utl_pmx_capnum);
                   utl_pmx_newcap(txt);
                 }
                 else utl_pmx_set_paterror(pat);
                 break;
                 
      case '|' : if (utl_pmx_is_negated()) {
                   pat = utl_pmx_alt(pat);
                   txt = pmxstart(utl_pmx_captop());
                   utl_pmx_clr_negated();
                 }
                 else
                   pat = utl_pmx_alt_skip(pat);
                 break;
      
      case ')' : if (utl_pmx_is_negated()) {
                   utl_pmx_clr_negated();
                   utl_pmx_FAIL;
                 } 
                 if (utl_pmx_capstk_num > 0) {
                   utl_pmx_capt[utl_pmx_cappop()][1] = txt;
                 }
                 else utl_pmx_set_paterror(pat);
                 pat++;
                 break;
                 
      case '%' : if (pat[1]) len = utl_pmx_nextch(++pat, &c1);

      default  : if (c1 == 0) len = utl_pmx_nextch(pat, &c1);
                 len = utl_pmx_nextch(txt, &ch);
                 if (ch != c1) utl_pmx_FAIL;
                 txt+=len; pat+=len;
                 break;
                 
      fail     : pat = utl_pmx_alt(pat) ; /* search for an alternative */
                 if (*pat == '\0') utl_pmx_capnum = 0;
                 txt = pmxstart(utl_pmx_captop()); /* reset text */
                 break;
    }
  }
  utl_pmx_capt[0][1] = txt;
  
  if (utl_pmx_capstk_num != 0)  utl_pmx_set_paterror(pat);
  
  for (len = utl_pmx_capnum; len < utl_pmx_MAXCAPT; len++) {
    utl_pmx_capt[len][0] = utl_pmx_capt[len][1] = NULL;
  }
  _logdebug("res: %p - %p",utl_pmx_capt[0][0],utl_pmx_capt[0][1]);
  return utl_pmx_capt[0][0];
}

char *utl_pmx_search(char *pat, char *txt)
{
  char *ret=NULL;
  
  utl_pmx_error = NULL;
  
       if (strncmp(pat,"<utf>",5) == 0) {pat+=5; utl_pmx_utf8=1;}
  else if (strncmp(pat,"<iso>",5) == 0) {pat+=5; utl_pmx_utf8=0;}
    
  if (*pat == '^')  ret = utl_pmx_match(pat+1,txt);
  else while (!(ret = utl_pmx_match(pat,txt)) && *txt) {
         txt += utl_pmx_utf8 ? utl_pmx_get_utf8(txt, NULL) : 1;
       }
  _logdebug("ret: %p",ret);
  return ret;
}

#endif
#endif /* UTL_NOPMX */


#endif /* UTL_H */

