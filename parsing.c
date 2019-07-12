
#include "mpc.h"

#ifdef _WIN32

static char buffer[2048];

char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

void add_history(char* unused) {}

#else
#include <editline/readline.h>
#include <editline/history.h>
#endif
/*Declare New lval struct*/
typedef struct {
  int type;
  long num;
  int err;
}lval;
/*Create Enumeration of Possible lval Types*/
enum { LVAL_NUM, LVAL_ERR };
/*Create Enumeration of Possible Errors*/
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };
/*Create a new number type lval*/
lval lval_num(long x){
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}
/*Create a new error type lval*/
lval lval_err(int x){
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}
/*Print an "lval"*/
void lval_print(lval v){
  switch (v.type) {
    case LVAL_NUM: printf("%li", v.num); break;
    case LVAL_ERR:
      if(v.err==LERR_DIV_ZERO){ printf("Error: Invalid Operator!"); }
      if(v.err==LERR_BAD_OP){ printf("Error: Invalid Operator!"); }
      if(v.err==LERR_BAD_NUM){ printf("Error: Invalid Number!");}
      break;
  }
}
/*Print lval followed by new line*/
void lval_println(lval v){ lval_print(v); putchar('\n'); }
/* Use operator string to see which operation to perform */
lval eval_op(lval x, char* op, lval y) {
  /*If either value is an error return it*/
  if(x.type==LVAL_ERR){ return x; }
  if(y.type==LVAL_ERR){ return y; }
  /*Otherwise do maths*/
  if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
  if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
  if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
  if (strcmp(op, "/") == 0) {
    /*If the second operand is zero return error*/
    return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num); 
    }
  if (strcmp(op, "%") == 0) {
    /*Same as before*/
    return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num % y.num);
  }
  if (strcmp(op, "^") == 0) {
    return (x.num == 0 && y.num == 0) ? lval_err(LERR_BAD_NUM) : lval_num(pow(x.num,y.num)); 
    }
  if (strcmp(op, "min") == 0) { return lval_num((x.num<y.num)?x.num:y.num); }
  if (strcmp(op, "max") == 0) { return lval_num((x.num>y.num)?x.num:y.num); }
  
  return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {
  
  /*Check if there is some error in conversion*/ 
  if (strstr(t->tag, "number")) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);  //errno seems to be error indicator from the eerno.h library
  }
  /* The operator is always second child. */
  char* op = t->children[1]->contents;
  /* We store the third child in `x` */
  lval x = eval(t->children[2]);
  /* Iterate the remaining children and combining. */
  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }
  return x;  
}

int main(int argc, char** argv) {
  
  /* Create Some Parsers */
  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Lispy    = mpc_new("lispy");
  
  /* Define them with the following Language */
  /*| /((ab)+a?)/ | /((ba)+b?)/ for consecutive a and b*/
  /* /-?[0-9]+((.)([0-9]+))?/ - enabling decimal numbers */
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                     \
      number   : /-?[0-9]+((.)([0-9]+))?/ ;                             \
      operator : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\" ;  \
      expr     : <number> | '(' <operator> <expr>+ ')' ;  \
      lispy    : /^/ <operator> <expr>+ /$/ ;             \
    ",
    Number, Operator, Expr, Lispy);
/*Code for the operators written conventionally between two expressions */ 

/*mpca_lang(MPCA_LANG_DEFAULT,
    "                                                     \
      number   : /-?[0-9]+(([.])([0-9]+))?/ ;                             \
      operator : '+' | '-' | '*' | '/' | '%' | \"add\" | \"sub\" | \"mul\" | \"div\" | \"mod\" ;  \
      expr     : <number> | '(' <expr> <operator> <expr> ')' ;  \
      lispy    : /^/ ( <expr> <operator> <expr> )+ ( <operator> <expr> )* /$/ ;             \
    ",
    Number, Operator, Expr, Lispy); */
  
  puts("Lispy Version 0.0.0.0.2");
  puts("Press Ctrl+c to Exit\n");
  
  while (1) {
  
    char* input = readline("lispy> ");
    add_history(input);
    
    /* Attempt to parse the user input */
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      /* On success print and delete the AST */
      lval result = eval(r.output);   //output is mpc_val_t, declared typedef void mpc_val_t; why void stores all the data?
      lval_println(result);           //it actually have a sense, mpc_val_t is a alias to void; and it creates a pointer void*
      mpc_ast_delete(r.output);       //void pointers have a meaning - they just dont specify type of data they are pointing to - no void variables!
    } else {
      /* Otherwise print and delete the Error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
    
    free(input);
  }
  
  /* Undefine and delete our parsers */
  mpc_cleanup(4, Number, Operator, Expr, Lispy);
  
  return 0;
}

