%define api.pure full

%parse-param {struct xo ** xop} {const char ** sptr}
%lex-param {const char ** sptr}

%{
#include "../xo_describe.h"
#include "lex.h"
#include "impl.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
%}

%code requires {
#include "../xo.h"
#include <stdbool.h>
struct source {
    char * name[3];
    bool mix;
};
}

%union {
    struct token str;
    float_type flt;
    long num;

    int d;

    bool b;

/*
    struct {
        struct source * sources;
        size_t length;
    } source_list;
    struct source source;
*/

    struct filter filter;
    enum filter_type filter_type;
}

%{
enum yytokentype yylex(YYSTYPE *lvalp, const char ** sptr);
void yyerror(struct xo ** xop, const char ** _, const char * s);
%}

%token LEX_EMPTY

%token KWD_BYE KWD_GET KWD_COMMIT KWD_DESCRIBE
%token KWD_NEW KWD_CHAIN KWD_FILTER KWD_REPLACE
%token KWD_FP KWD_IN KWD_OUT KWD_LIMITER
%token KWD_LEFT KWD_RIGHT KWD_MONO
%token KWD_BQ KWD_A KWD_B
%token KWD_SV KWD_LP KWD_HP KWD_OVER KWD_ORDER KWD_F KWD_Q KWD_UNITY
%token KWD_SQRT1_2
%token KWD_IF KWD_NOT KWD_EQ KWD_LT KWD_GT

%token <str> LIT_STRING
%token <flt> LIT_FLOAT
%token <num> LIT_NUMERIC

%type <num> source over order
%type <d> get_thing
%type <flt> qval
%type <b> not get_thing_test

/*
%type <source_list> source_list
%type <source> source port_source mix_source
*/

%type <filter_type> sv_subtype
%type <filter> filter bq_filter sv_filter

%define parse.error verbose

%%

commands: %empty
        | command
        | commands ';'
        | commands ';' command
        ;

command: KWD_BYE
            { *sptr = NULL; YYACCEPT;                                   }
       | KWD_DESCRIBE
            { xo_describe(*xop, printf, 1, 0);                          }
       | KWD_COMMIT
            { commit(*xop);                                             }
       | KWD_GET get_thing
            { respond(-$2 - 1);                                         }
       | KWD_IF get_thing_test
            { respond(-$2 - 1); if (!$2) YYACCEPT;                      }
       | KWD_NEW
            { xo_free(*xop); *xop = xo_alloc();                         }
       | KWD_CHAIN source
            { xo_add_chain(*xop, $2);                                   }
       | KWD_FILTER filter
            { if ((*xop)->n_chains > 0)
                { *xo_add_filter_to_chain(*xop) = $2; }                 }
       | KWD_REPLACE LIT_NUMERIC LIT_NUMERIC KWD_FILTER filter
            { if ((*xop)->n_chains >= $2 &&                         //  }
                  ((*xop)->chains[$2].n_filters > $3))              //  }
                { (*xop)->chains[$2].filters[$3] = $5; }            //  }
              else                                                  //  }
                { yyerror(NULL, NULL, "index out of range"); YYABORT; } }
       ;

get_thing: KWD_FP
            { $$ = CAT(strfromf, FP)(NULL, 0, "%A", CAT(M_PIf, FP));    }
         | KWD_IN
            { $$ = XO_SOURCE_MAX;                                       }
         | KWD_OUT
            { $$ = (*xop) ? (*xop)->n_chains : 0;                       }
         | KWD_LIMITER
            { $$ = LIMITER;                                             }
         | source
            { $$ = ($1 <= XO_SOURCE_MAX);                               }
         ;

get_thing_test: get_thing not KWD_EQ LIT_NUMERIC
                { $$ = ($1 == $4) != $2;    }
              | get_thing not KWD_LT LIT_NUMERIC
                { $$ = ($1 < $4) != $2;     }
              | get_thing not KWD_GT LIT_NUMERIC
                { $$ = ($1 > $4) != $2;     }
              ;

not: %empty  { $$ = false; }
   | KWD_NOT { $$ = true; }
   ;

source: KWD_LEFT    { $$ = XO_LEFT;     }
      | KWD_RIGHT   { $$ = XO_RIGHT;    }
      | KWD_MONO    { $$ = XO_MONO;     }
      | LIT_NUMERIC { $$ = $1;          }
      ;


/*
chain_io: KWD_IN source KWD_OUT LIT_STRING { $$ = 
        ;
*/

filter: bq_filter { $$ = $1; }
      | sv_filter { $$ = $1; }
      ;

bq_filter: KWD_BQ
            { $$ = (struct filter) { .type = XO_FILTER_BQ };    }
         | KWD_BQ
           KWD_A LIT_FLOAT LIT_FLOAT LIT_FLOAT
           KWD_B LIT_FLOAT LIT_FLOAT
            { $$ = (struct filter) { .type = XO_FILTER_BQ,    //}
                    .a0 = $3, .a1 = $4, .a2 = $5,             //}
                    .b1 = $7, .b2 = $8 };                       }
         ;

sv_filter: KWD_SV KWD_UNITY
            { $$ = (struct filter) { .type = XO_FILTER_SV_LP }; }
         | KWD_SV sv_subtype order over
           KWD_F LIT_FLOAT KWD_Q qval
            { $$ = (struct filter) { .type = $2,              //}
                    .a0 = $6, .a1 = $8, .order = $3,          //}
                    .over = $4, };                              }
         ;

sv_subtype: KWD_LP { $$ = XO_FILTER_SV_LP; }
          | KWD_HP { $$ = XO_FILTER_SV_HP; }
          ;

order: %empty
        { $$ = 1;                                               }
     | KWD_ORDER LIT_NUMERIC
        { if ($2 < 2 || ($2 % 2) == 1) {                      //}
              yyerror(NULL, NULL,                             //}
                  "order must be positive, non-zero, even");  //}
              YYERROR; }                                      //}
          else { $$ = $2 / 2; }                                 }
     ;

over: %empty
        { $$ = 1;                                       }
    | KWD_OVER LIT_NUMERIC
        { if ($2 < 1) {                               //}
              yyerror(NULL, NULL,                     //}
                  "over must be positive, non-zero"); //}
              YYERROR; }                              //}
          else { $$ = $2; }                             }
    ;

qval: LIT_FLOAT   { $$ = $1;                    }
    | KWD_SQRT1_2 { $$ = CAT(M_SQRT1_2f, FP);   }
    ;



%%

#include <stdio.h>
void yyerror(struct xo ** xop, const char ** _, const char * s)
{
    (void)xop;
    fprintf(stderr, "%zu: %s\n", lineno, s);
}


