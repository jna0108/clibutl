/* 
**  (C) by Remo Dentato (rdentato@gmail.com)
** 
** This software is distributed under the terms of the BSD license:
**   http://creativecommons.org/licenses/BSD/
**     
**                     ___   __
**                  __/  /_ /  )
**          ___  __(_   ___)  /
**         /  / /  )/  /  /  /  Minimalist
**        /  /_/  //  (__/  /  C utility 
**       (____,__/(_____(__/  Library
** 

## How to use utl

Include the `utl.h` header in your source files.
Compile and link `utl.c` with the other files.

Alternatively, you can do without `utl.c` simply by
compiling one of your source files with the 
symbol *`UTL_MAIN`* defined (either by definining it 
before including the header, or via a compiler switch
like "`-DUTL_MAIN`"). A good candidate is the
file where your `main()` function is.

*/

    
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

#define utl_NULLSTATEMENT do {} while (0)

#ifndef UTL_NOLOG

/* Use as if it was a printf */
#define logprintf(...)  (utl_log_time(),               \
                        fprintf(log_file,__VA_ARGS__), \
                        fputc('\n',log_file),          \
                        fflush(log_file))
                                                   
#define logclose()      utl_log_close("LOG STOP")
#define logopen(f,m)    utl_log_open(f,m)

#ifndef NDEBUG
#define logcheck(e)     utl_log_check(!!(e),#e,__FILE__,__LINE__)
#define logassert(e)    utl_log_assert(!!(e),#e,__FILE__,__LINE__)
#define logdebug        logprintf
#else
#define logcheck(e)     1
#define logassert(e)    utl_NULLSTATEMENT
#define logdebug(...)   utl_NULLSTATEMENT
#endif

/* To disable a message (without deleting the line). Useful for debugging  */                   
#define _logprintf(...) utl_NULLSTATEMENT
#define _logdebug(...)  utl_NULLSTATEMENT
#define _logcheck(...)  1
#define _logassert(...) utl_NULLSTATEMENT

int  utl_log_time(void);
void utl_log_close(char *msg);
void utl_log_open(char *fname, char *mode);
int  utl_log_check(int res, char *test, char *file, int line);
void utl_log_assert(int res, char *test, char *file, int line);

#ifdef UTL_MAIN

static FILE *log_file = NULL;

int utl_log_time()
{
  char     log_tstr[32];
  time_t   log_time;
  
  if (!log_file) log_file = stderr;
  time(&log_time);
  strftime(log_tstr,32,"%Y-%m-%d %X",localtime(&log_time));
  return fprintf(log_file,"%s ",log_tstr);
}

void utl_log_close(char *msg)
{
  if (msg) logprintf(msg);
  if (log_file && log_file != stderr) fclose(log_file);
  log_file = NULL;
}

void utl_log_open(char *fname, char *mode)
{
  char md[2];
  md[0] = (mode && *mode == 'w')? 'w' : 'a'; md[1] = '\0';
  log_file = fopen(fname,md);
  logprintf("LOG START");
}

int utl_log_check(int res, char *test, char *file, int line)
{
  logprintf("CHK %s (%s) %s:%d", (res?"PASS":"FAIL"), test, file, line);
  return res;
}

void utl_log_assert(int res, char *test, char *file, int line)
{
  if (!utl_log_check(res,test,file,line)) {
    logprintf("CHK EXITING ON FAIL");
    logclose();
    exit(1);
  }
}
#endif

#endif /* UTL_NOLOG */

#ifndef UTL_NOFSM
/* ---------------------------- */
/* You should have only one FSM per function to avoid clash in 
** state names. It's a limitation but can help keeping things tidy.
*/
#define fsm           
#define fsmGOTO(x)    goto fsm_state_##x
#define fsmSTATE(x)   fsm_state_##x :
#endif

/* ---------------------------- */

/*  .% Traced memory
**  ================
*/

#define memINVALID    -2
#define memOVERFLOW   -1
#define memVALID       0
#define memNULL        1

#ifdef UTL_MEMCHECK

void  *utl_malloc   (size_t size, char *file, int line );
void  *utl_calloc   (size_t num, size_t size, char *file, int line);
void  *utl_realloc  (void *ptr, size_t size, char *file, int line);
void   utl_free     (void *ptr, char *file, int line );
void  *utl_strdup   (void *ptr, char *file, int line);
                    
int    utl_check    (void *ptr,char *file, int line);
size_t utl_mem_used (void);


#ifdef UTL_MAIN
/*************************************/

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

int utl_check(void *ptr,char *file, int line)
{
  utl_mem_t *p;
  
  if (ptr == NULL) return memNULL;
  p = utl_mem(ptr);
  if (memcmp(p->chk,utl_BEG_CHK,4)) { 
    logprintf("MEM Invalid or double freed %p (%lu %s:%d)",p->blk, \
                                               utl_mem_allocated, file, line);     
    return memINVALID; 
  }
  if (memcmp(p->blk+p->size,utl_END_CHK,4)) {
    logprintf("MEM Boundary overflow detected %p [%lu] (%lu %s:%d)", \
                              p->blk, p->size, utl_mem_allocated, file, line); 
    return memOVERFLOW;
  }
  logprintf("MEM Valid pointer %p (%lu %s:%d)",ptr, utl_mem_allocated, file, line); 
  return memVALID; 
}

void *utl_malloc(size_t size, char *file, int line )
{
  utl_mem_t *p;
  
  if (size == 0) logprintf("MEM Shouldn't allocate 0 bytes (%lu %s:%d)", \
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
  logprintf("MEM alloc %p [%lu] (%lu %s:%d)",p->blk,size,utl_mem_allocated,file,line);
  return p->blk;
};

void *utl_calloc(size_t num, size_t size, char *file, int line)
{
  void *ptr;
  
  size = num * size;
  ptr = utl_malloc(size,file,line);
  if (ptr)  memset(ptr,0x00,size);
  return ptr;
};

void utl_free(void *ptr, char *file, int line)
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
                          
    case memINVALID :  logprintf("MEM free an invalid pointer! (%lu %s:%d)", \
                                                utl_mem_allocated, file, line);
                          break;
  }
}

void *utl_realloc(void *ptr, size_t size, char *file, int line)
{
  utl_mem_t *p;
  
  if (size == 0) {
    logprintf("MEM realloc() used as free() %p -> [0] (%lu %s:%d)",ptr,utl_mem_allocated, file, line);
    utl_free(ptr,file,line); 
  } 
  else {
    switch (utl_check(ptr,file,line)) {
      case memNULL   : logprintf("MEM realloc() used as malloc() (%lu %s:%d)", \
                                             utl_mem_allocated, file, line);
                          return utl_malloc(size,file,line);
                        
      case memVALID  : p = utl_mem(ptr); 
                          p = realloc(p,sizeof(utl_mem_t) + size); 
                          if (p == NULL) {
                            logprintf("MEM Out of Memory (%lu %s:%d)", \
                                             utl_mem_allocated, file, line);
                            return NULL;
                          }
                          utl_mem_allocated -= p->size;
                          utl_mem_allocated += size; 
                          logprintf("MEM realloc %p [%lu] -> %p [%lu] (%lu %s:%d)", \
                                          ptr, p->size, p->blk, size, \
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

void *utl_strdup(void *ptr, char *file, int line)
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
  logprintf("MEM strdup %p [%lu] -> %p (%lu %s:%d)", ptr, size, dest, \
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

utl_extern(int(*utl_pmx_ext)(char *r, char *t) , NULL);

utl_extern(char     *utl_pmx_capt[20]   , {0} );
utl_extern(uint8_t   utl_pmx_capnum     ,  0  );
utl_extern(uint8_t   utl_pmx_capstk_num ,  0  );
utl_extern(uint8_t   utl_pmx_capstk[10] , {0} );

#define pmxstart(n) utl_pmx_capt[2*(n)]
#define pmxend(n)   utl_pmx_capt[2*(n)+1]
#define pmxlen(n)   ((int)(pmxend(n)-pmxstart(n)))
#define pmxcount()  utl_pmx_capnum

char *utl_pmx_search(char *r, char *t);
#define pmxsearch(r,t) utl_pmx_search(r,t)

#ifdef UTL_MAIN

static char *utl_pmx_alt(char *r)
{
  int paren=0;
  
  while (*r) {
    switch (*r++) {
      case '%' : if (*r)  r++; break;
      case '(' : paren++; break;
      case ')' : if (paren == 0) return NULL;
                 paren--; break;
      case '<' : while (*r && *r != '>') r++; while (*r == '>') r++;
                 break;
      case '|' : if (paren == 0) return r;
    }
  }
  return NULL;
}

static char *utl_pmx_skip(char *r)
{
  int paren=0;
  
  while (*r) {
    switch (*r++) {
      case '%' : if (*r)  r++; break;
      case '(' : paren++; break;
      case ')' : if (paren == 0) return (r-1);
                 paren--; break;
      case '<' : while (*r && *r != '>') r++; while (*r == '>') r++;
                 break;
    }
  }
  return r;
}

static int utl_pmx_is_in(char *r, char c)
{
  char *s=r;
  while (*s && *s != '>') s++;
  if (*s && s[1] == '>') s++; /* allow '>' as last element of the set */
  
  if (c == *r)   return 1; /* just in case of a - */
  while (r<s) {
    if (r[1] == '-' && (s-r) >= 3 ) { /* its a range */
      if (r[0] <= c && c <= r[2]) return 1;
      r+=3;
    }
    else if (c == *r) return 1;
    r++;
  }
  return 0;
}

static int utl_pmx_class(char *r, char *t)
{
  int inv = 0;
  int rc = *r;
  int c = *t;
  if (isupper(rc)) {inv = 1; rc = tolower(rc); }
  switch (rc) {
    case 'a' : return inv ^ !!isalpha(c) ;
    case 's' : return inv ^ !!isspace(c) ;
    case 'u' : return inv ^ !!isupper(c) ;
    case 'l' : return inv ^ !!islower(c) ;
    case 'd' : return inv ^ !!isdigit(c) ;
    case 'x' : return inv ^ !!isxdigit(c);
    case 'w' : return inv ^ !!isalnum(c) ;
  
    case 'g' : return inv ^ (c == '+'  || c == '-');
    case 'n' : return inv ^ (c == '\r' || c == '\n');
    case '!' : inv = 1;
    case '=' : return inv ^ utl_pmx_is_in(r+1,c);
    
    case '$' : return c == '\0';
    case '.' : return 1;
    
    default  : if (utl_pmx_ext) return utl_pmx_ext(r,t);
  }
  return 0;
}

#define utl_pmx_cappush(n) (utl_pmx_capstk[utl_pmx_capstk_num++] = n)
#define utl_pmx_cappop()   (utl_pmx_capstk[--utl_pmx_capstk_num])
#define utl_pmx_captop()   (utl_pmx_capstk[utl_pmx_capstk_num-1])
#define utl_pmx_fail()      goto fail

static char *utl_pmx_match(char *r, char *t)
{
  uint32_t n;
  uint32_t min_n;
  uint32_t max_n;
  uint32_t l;
  char c;
    
  for (l=0;l<20;l++) utl_pmx_capt[l] = NULL;
  
  utl_pmx_capt[0] = t; utl_pmx_capt[1] = t;
  utl_pmx_capnum=0;
  utl_pmx_capstk_num = 0;   
  utl_pmx_cappush(utl_pmx_capnum++);
  
  while (*r) {
    logdebug("match %d [%s] [%s]",pmxcount(),r,t);
    c ='\0'; 
    switch (*r) {
      case '<' : r++; n = 0; min_n = 1; max_n = 1;
                 switch (*r) {
                   case '?' : min_n = 0; max_n = 1; r++; break;
                   case '*' : min_n = 0; max_n = 0xFFFFFFFF; r++; break;
                   case '+' : min_n = 1; max_n = 0xFFFFFFFF; r++; break;
                 }
                 while ((l=utl_pmx_class(r,t)) && (n < max_n)) {
                   n++;
                   while (*t && l>0) {t++; l--;}
                 }
                 if ( n < min_n ) utl_pmx_fail();
                 while (*r && *r != '>') r++;
                 while (*r == '>') r++; 
                 break;

      case '(' : if (utl_pmx_capnum<10) {
                   pmxstart(utl_pmx_capnum) = pmxend(utl_pmx_capnum) = t;
                   utl_pmx_cappush(utl_pmx_capnum++);
                 }
                 r++;
                 break;
                 
      case '|' : /* matched! skip the rest */
                 r = utl_pmx_skip(r);
                 break;
      
      case ')' : if (utl_pmx_capstk_num > 0) {
                   pmxend(utl_pmx_cappop()) = t;
                 }
                 r++;
                 break;
      
      case '%' : if (r[1]) {c = r[1]; r++; }
      default  : if (c == '\0') c = *r;
                 if (*t != c) utl_pmx_fail();
                 t++; r++;
                 break;
    }
    continue;
    
    fail: t=pmxstart(utl_pmx_captop()); /* reset text */
          /* search for an alternative pattern to match */
          r=utl_pmx_alt(r);
          /*pmxcount() = 0;*/
          if (!r) return NULL;
  }
  utl_pmx_capt[1] = t;
  return utl_pmx_capt[0];
}

char *utl_pmx_search(char *r, char *t)
{
  char *ret=NULL;
  
  if (*r == '^') ret = utl_pmx_match(r+1,t);
  else while (!(ret = utl_pmx_match(r,t)) && *t) t++;
  
  return ret;
}

#endif
#endif /* UTL_NOPMX */


#endif /* UTL_H */


