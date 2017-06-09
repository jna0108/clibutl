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

#include "utl.h"

//<<<//
#ifndef UTL_NOPEG

int utl_peg_str(const char *pat, const char *str)
{ int k = 0;
  if (!(str && *str)) return -1;
  while (*str && *pat && (*str == *pat)) {str++; pat++;k++;}
  return (*pat == '\0'? k: -1);
}

int utl_peg_oneof(const char *pat, const char *str)
{
  if (!(str && *str)) return -1;
  return (strchr(pat,*str)?1:-1);
}

int utl_peg_eol(const char *str)
{
  int ret;
  switch (*str) {
    case '\0' : ret = 0; break;
    case '\n' : ret = 1; break;
    case '\r' : ret = (str[1] == '\n') ? 2 : 1;    
    default   : ret = -1;
  }
  return ret;
}

int utl_peg_wspace(const char *str)
{
  int ret = 0;
  while (1) {
    switch (*str) {
      case '\b' : case '\a':
      case ' ' : case '\t': case '\xA0' : ret++; str++; break;
      default: return ret;
    }
  }
  return ret;
}

int utl_peg_vspace(const char *str)
{
  int ret = 0;
  while (1) {
    switch (*str) {
      case '\r' : case '\n': case '\v' : case '\f': case '\b' : case '\a':
      case ' ' : case '\t': case '\xA0' : ret++; str++; break;
      default: return ret;
    }
  }
  return ret;
}


static peg_t utl_peg_init(peg_t p, const char *s)
{
  if (p) {
    p->start    = s;
    p->pos      = s;
    p->aux      = NULL;
    p->auxi     = 0;
    p->fail     = 0;
    p->errpos   = s;
    p->errrule  = NULL;
    p->errln    = 1;
    p->errcn    = 1;
    p->errmsg   = NULL;
    p->defer    = vecnew(pegdefer_t);
  }
  return p;
}

peg_t utl_peg_new()
{
  return utl_peg_init(malloc(sizeof(struct peg_s)),utl_emptystring); 
}

peg_t utl_peg_free(peg_t parser)
{
  if (parser) {
    if (parser->defer) parser->defer=vecfree(parser->defer);
    free(parser);
  }
  return NULL;
}

const char *utl_peg_defer(peg_t parser, pegaction_t func,const char *from, const char *to)
{
  pegdefer_t defer;
  
  if (!parser->fail) {
    defer.func = func;
    defer.from = from;
    defer.to   = to;
    vecpush(pegdefer_t, parser->defer, defer);
  }
  return NULL;
}

const char *utl_peg_back(peg_t parser,const char *rule_name, const char *pos,int32_t dcnt)
{
  int32_t cnt;
  if (parser->errpos <= pos) {
     parser->errpos  = pos;
     parser->errrule = rule_name;
     parser->errmsg = parser->errmsgtmp;
  }
  parser->pos = pos;
  cnt = veccount(parser->defer);
  if (dcnt < cnt) {
    cnt -= dcnt;
    vecdrop(parser->defer,cnt);
  }
  return NULL;  
}

/*
          peg_save.rpt++,(PEG_FAIL \
                          ? 0 \
                          :(peg_save.rlen = PEG_POS - peg_save.rpos,
                            peg_save.rpos=PEG_POS,
                            peg_save.rdcnt=PEG_DCNT)
                          ), \
          ((PEG_FAIL || peg_save.rpt >= peg_save.max || peg_save.rlen == 0) \
            ? (((PEG_FAIL=(peg_save.rpt<=peg_save.min)) \
                 ? PEG_BACK(peg_save.pos,peg_save.dcnt) \
                 : ((PEG_DCNT=peg_save.rdcnt),
                    (PEG_POS=peg_save.rpos))) \
              , peg_save.max=0) \
            : 0)

  */          

void utl_peg_repeat(peg_t peg_, const char *pegr_, pegsave_t *peg_save)
{
  peg_save->rpt++;
  if (!PEG_FAIL) {
    peg_save->rlen  = PEG_POS - peg_save->rpos;
    peg_save->rpos  = PEG_POS;
    peg_save->rdcnt = PEG_DCNT;
  }
  if (PEG_FAIL || peg_save->rpt >= peg_save->max || peg_save->rlen == 0) {
      if ((PEG_FAIL = (peg_save->rpt <= peg_save->min))) {
        PEG_BACK(peg_save->pos,peg_save->dcnt);
      }
      else {
        PEG_DCNT = peg_save->rdcnt;
        PEG_POS = peg_save->rpos;
      }
      peg_save->max = 0;
  }
}

void utl_peg_ref(peg_t parser, const char *rule_name, pegrule_t rule)
{
  const char *tmp = parser->pos;
  int32_t cnt = veccount(parser->defer);
  if (!parser->fail) {
    rule(parser,rule_name);
    if (parser->fail) 
      (void)utl_peg_back(parser,rule_name,tmp,cnt);
  }
}

static int peg_defer_func_NULL(const char *from, const char *to, void *aux)
{ return 1; }

static void utl_peg_seterrln(peg_t parser)
{
  const char *s;
  
  if (*parser->errpos) {
    s = parser->start;
    parser->errln = 1;
    parser->errcn = 1;
    parser->errptr = s;
    while (*s && s < parser->errpos) {
      if (s[0] == '\r' && s[1] == '\n') s++;
      if (*s == '\r' || *s == '\n') {
        parser->errptr=s+1;
        parser->errln++;
        parser->errcn=0;
      }
      s++;
      parser->errcn++;
    }  
  }
}

static void utl_peg_execdeferred(peg_t parser)
{
  pegdefer_t defer;
  pegdefer_t defer_NULL = {peg_defer_func_NULL, NULL, NULL};
  
  defer = vecfirst(pegdefer_t, parser->defer, defer_NULL);
  while (defer.func != peg_defer_func_NULL) {
    if (defer.func(defer.from, defer.to, parser->aux))
      break;
    defer = vecnext(pegdefer_t, parser->defer, defer_NULL);
  }
}

int utl_peg_parse(peg_t parser, pegrule_t start_rule, 
                const char *txt,const char *rule_name, void *aux)
{
  if (parser && start_rule && txt) {
    utl_peg_init(parser,txt);
    parser->aux = aux;
    utl_peg_ref(parser, rule_name, start_rule);

    if (!parser->fail) {
      if (parser->errpos <= parser->pos) {
        parser->errpos  = parser->pos;
        parser->errrule = rule_name;
        parser->errmsg = parser->errmsgtmp;
      }
      utl_peg_execdeferred(parser);
    }
    utl_peg_seterrln(parser);
    
    return !parser->fail;
  }
  return 0;
}

#endif
//>>>//
