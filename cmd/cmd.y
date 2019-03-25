%define api.pure full

%parse-param {struct xo ** xop} {const char ** sptr}
%lex-param {const char ** sptr}

%{
#include "../xo_describe.h"
#include "lex.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
void respond(int response);
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
    double flt;
    long num;

    int d;

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

%token KWD_BYE KWD_COMMIT KWD_GET KWD_SOURCES KWD_NEW KWD_CHAIN KWD_FILTER
%token KWD_FP KWD_IN KWD_OUT KWD_LIMITER
%token KWD_LEFT KWD_RIGHT KWD_MONO
%token KWD_SV KWD_LP KWD_HP KWD_BQ KWD_A KWD_B

%token <str> LIT_STRING
%token <flt> LIT_FLOAT
%token <num> LIT_NUMERIC

%type <num> source
%type <d> get_thing

/*
%type <source_list> source_list
%type <source> source port_source mix_source
*/

%type <filter_type> sv_subtype
%type <filter> filter

%define parse.error verbose

%%

commands: %empty
        | command
        | commands ';'
        | commands ';' command
        ;

command: KWD_BYE { printf("bye\n"); *sptr = NULL; YYABORT; }
       | KWD_COMMIT { xo_describe(*xop, printf, 1, 0); }
       | KWD_GET get_thing { respond(-$2 - 1); }
/*
       | KWD_SOURCES source_list
        { printf("%zu\n", $2.length); }
*/
       | KWD_NEW { if (*xop) { xo_free(*xop); } *xop = xo_alloc(); }
       | KWD_CHAIN chain_io { xo_add_chain(*xop, XO_MONO); }
       | KWD_FILTER filter { *xo_add_filter_to_chain(*xop) = $2; }
       ;

get_thing: KWD_FP { $$ = CAT(strfromf, FP)(NULL, 0, "%A", CAT(M_PIf, FP)); }
         | KWD_IN { $$ = XO_SOURCE_MAX; }
         | KWD_OUT { $$ = (*xop) ? (*xop)->n_chains : 0; }
         | KWD_LIMITER { $$ = LIMITER; }
         | source { $$ = $1; }
         ;

 /*
source_list: source
            {
                $$.sources = malloc(sizeof($1));
                $$.length = 1;
                $$.sources[0] = $1;
            }
           | source_list source
            {
                $$.sources = realloc($$.sources,
                    ($$.length + 1) * sizeof($2));
                $$.sources[$$.length++] = $2;
            }
           ;
source: port_source { $$ = $1; }
      | mix_source { $$ = $1; }
      ;

port_source: KWD_PORT LIT_STRING
            {
                $$ = (struct source) {
                    .name = { strndup($2.start, $2.length) }
                };
            }
           ;
mix_source: KWD_MIX LIT_STRING LIT_STRING LIT_STRING
            {
                $$ = (struct source) {
                    .name = {
                        strndup($2.start, $2.length),
                        strndup($3.start, $3.length),
                        strndup($4.start, $4.length)
                    },
                    .mix = true
                };
            }
          ;
 */

source: KWD_LEFT { $$ = XO_LEFT; }
      | KWD_RIGHT { $$ = XO_RIGHT; }
      | KWD_MONO { $$ = XO_MONO; }
      | LIT_NUMERIC { $$ = $1; }
      ;


chain_io: KWD_IN source KWD_OUT LIT_STRING
        ;

filter: KWD_SV sv_subtype
        KWD_A LIT_FLOAT LIT_FLOAT LIT_FLOAT
        {
            $$ = (struct filter) {
                .a0 = $4, .a1 = $5, .a2 = $6,
                .type = $2
            };
        }
      | KWD_BQ
        KWD_A LIT_FLOAT LIT_FLOAT LIT_FLOAT
        KWD_B LIT_FLOAT LIT_FLOAT
        {
            $$ = (struct filter) {
                .a0 = $3, .a1 = $4, .a2 = $5,
                .b1 = $7, .b2 = $8,
                .type = XO_FILTER_BQ
            };
        }
      ;

sv_subtype: KWD_LP { $$ = XO_FILTER_SV_LP; }
          | KWD_HP { $$ = XO_FILTER_SV_HP; }
          ;

%%

#include <stdio.h>
void yyerror(struct xo ** xop, const char ** _, const char * s)
{
    (void)xop;
    fprintf(stderr, "%s\n", s);
}


