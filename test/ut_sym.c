/*
**  (C) by Remo Dentato (rdentato@gmail.com)
** 
** This software is distributed under the terms of the MIT license:
**  https://opensource.org/licenses/MIT
**     
**                        ___   __
**                     __/  /_ /  )
**             ___  __(_   ___)  /
**            /  / /  )/  /  /  /
**           /  (_/  //  (__/  / 
**          (____,__/(_____(__/
**    https://github.com/rdentato/clibutl
**
*/
#define UTL_MEMCHECK
#include "utl.h"

const char *pippo = "pippo";

int main(int argc, char *argv[])
{
  sym_t tbl;
  uint32_t k,j;
  int32_t  i;
  char *s;
  buf_t b;
  
  logopen("l_sym.log","w");
  
  tbl=symnew();
  logcheck(tbl && symcount(tbl) == 0);
  
  b = tbl->aux;
  logcheck(b);

  j=symadd(tbl,pippo);
  logexpect(symcount(tbl) == 1,"Count:%d",symcount(tbl));
  logexpect(j==4,"j: %d",j);
  
  k = symsearch(tbl, pippo);
  logexpect(k == 4,"k:%d",k);
  
  s = symget(tbl,k);
  logcheck(s && s != pippo && strcmp(s,pippo)==0);
  
  j=symadd(tbl,"pluto");
  j=symadd(tbl,"paperino");
  logcheck(symcount(tbl)==3);
  
  k = symsearch(tbl,"topolino");
  logcheck(k == symNULL);
  
  s = symget(tbl,k);
  logcheck(s==NULL);
  
  k = symsearch(tbl,"pluto");
  logcheck(k!=symNULL);
  logtrace("%s: %d",symget(tbl,k),k);
  
  i = symgetdata(tbl,k);
  logcheck(i == 0);
 
  i = symsetdata(tbl,k,-213);
  logcheck(i);
  i = symgetdata(tbl,k);
  logcheck(i == -213);
  
  k = symfirst(tbl);
  while (k != symNULL) {
    logtrace("%d <-> %s (%d)",k,symget(tbl,k),symgetdata(tbl,k));
    k = symnext(tbl);
  }
  logtrace("count:%d max:%d",tbl->cnt,tbl->max);
  
  k = symfirst(tbl);
  while (k != symNULL) {
    logtrace("%d <-> %s (%d)",k,symget(tbl,k),symgetdata(tbl,k));
    k = symnext(tbl);
  }
  logtrace("count:%d max:%d esz:%d",tbl->cnt,tbl->max,tbl->esz);
  /* START OF FREEZE TEST */
  logcheck(symfreeze("l_sym.frz",tbl));
  #if 1
  tbl=symfree(tbl);
  logassert(tbl == NULL);

  tbl=symunfreeze("l_sym.frz");
  logassert(tbl);
  logassert(tbl->hsh);
  logassert(tbl->cmp);
  b = tbl->aux;
#endif
  /* END OF FREEZE TEST */
  
  k = symfirst(tbl);
  logcheck(symfreeze("l_sym2.frz",tbl));
  while (k != symNULL) {
    logtrace("%d <-> %s (%d)",k,symget(tbl,k),symgetdata(tbl,k));
    k = symnext(tbl);
  }
  logtrace("count:%d max:%d esz:%d",tbl->cnt,tbl->max,tbl->esz);
  
  loginfo("searching");
  k = symsearch(tbl,"pluto");
  logcheck(k != symNULL);
  logtrace("%s: %d",symget(tbl,k),k);
  logcheck(buf(b)[k] == 'p');
  
  j = symdel(tbl,"pluto");
  logcheck(j);
  
  logcheck(buf(b)[k] == '\0');
    
  k = symsearch(tbl,"pluto");
  logcheck(k == symNULL);
   
  tbl=symfree(tbl);
  logcheck(!tbl);
  
  logclose();
  exit(0);
}
