#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


// for lex
#define MAXLEN 256

// Token types
typedef enum {
    UNKNOWN, END, ENDFILE, 
    INT, ID,
    ADDSUB, MULDIV,
    INCDEC, // ++, --
    AND, OR, XOR, // &|^
    ADDSUB_ASSIGN, // +=, -=
    ASSIGN, 
    LPAREN, RPAREN
} TokenSet;

// Test if a token matches the current token 
extern int match(TokenSet token);

// Get the next token
extern void advance(void);

// Get the lexeme of the current token
extern char *getLexeme(void);

// for parser

#define TBLSIZE 256

// Set PRINTERR to 1 to print error message while calling error()
// Make sure you set PRINTERR to 0 before you submit your code
#define PRINTERR 0

#define DEBUG 0

// Call this macro to print error message and exit the program
// This will also print where you called it in your program
#define error(errorNum) { \
    if (PRINTERR) \
        fprintf(stderr, "error() called at %s:%d: ", __FILE__, __LINE__); \
    err(errorNum); \
}

// Error types
typedef enum {
    UNDEFINED, MISPAREN, NOTNUMID, NOTFOUND, RUNOUT, NOTLVAL, 
    DIVZERO, SYNTAXERR, NOTASSIGN
} ErrorType;

// Structure of the symbol table
typedef struct {
    int val;
    char name[MAXLEN];
    int address; // memory address
} Symbol;

// Structure of a tree node
typedef struct _Node {
    TokenSet data;
    int val;
    int is_lvalue;
    char lexeme[MAXLEN];
    struct _Node *left; 
    struct _Node *right;
} BTNode;

// The symbol table
extern Symbol table[TBLSIZE];

// Initialize the symbol table with builtin variables
extern void initTable(void);

// Get the value of a variable
extern int getval(char *str);
extern int getaddress(char* str);

// Set the value of a variable
extern int setval(char *str, int val);
extern int setaddress(char* str);

// Make a new node according to token type and lexeme
extern BTNode *makeNode(TokenSet tok, const char *lexe);

// Free the syntax tree
extern void freeTree(BTNode *root);

extern BTNode *factor(void);
extern BTNode *unary_expr(void);
extern BTNode *muldiv_expr_tail(BTNode *left);
extern BTNode *muldiv_expr(void);
extern BTNode *addsub_expr_tail(BTNode *left);
extern BTNode *muldiv_expr(void);
extern BTNode *addsub_expr_tail(BTNode *left);
extern BTNode *addsub_expr(void);
extern BTNode *and_expr_tail(BTNode *left);
extern BTNode *and_expr(void);
extern BTNode *xor_expr_tail(BTNode *left);
extern BTNode *xor_expr(void);
extern BTNode *or_expr_tail(BTNode *left);
extern BTNode *or_expr(void);
extern BTNode *assign_expr(void);
extern int statement(void);

// Print error message and exit the program
extern void err(ErrorType errorNum);


// for codeGen
// Evaluate the syntax tree
extern int evaluateTree(BTNode *root);


// Generate Assembly code
extern int genCode(BTNode* root);

// Print the syntax tree in prefix
extern void printPrefix(BTNode *root);


/*============================================================================================
lex implementation
============================================================================================*/

static TokenSet getToken(void);
static TokenSet curToken = UNKNOWN;
static char lexeme[MAXLEN];

TokenSet getToken(void)
{
    int i = 0;
    char c = '\0';

    while ((c = fgetc(stdin)) == ' ' || c == '\t');

    if (isdigit(c)) {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while (isdigit(c) && i < MAXLEN) {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return INT;
    } else if ( c == '&' ) {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return AND;
    } else if ( c == '|' ) {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return OR;
    } else if ( c == '^' ) {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return XOR;
    } else if (c == '+' || c == '-') {
        lexeme[0] = c;
        c = fgetc(stdin);
        if (c == lexeme[0]) {
            // ++ or --
            lexeme[1] = c;
            lexeme[2] = '\0';
            return INCDEC;
        } else if ( c == '=' ) {
            lexeme[1] = c;
            lexeme[2] = '\0';
            return ADDSUB_ASSIGN;
        }
        ungetc(c, stdin);
        lexeme[1] = '\0';
        return ADDSUB;
    } else if (c == '*' || c == '/') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return MULDIV;
    } else if (c == '\n') {
        lexeme[0] = '\0';
        return END;
    } else if (c == '=') {
        strcpy(lexeme, "=");
        return ASSIGN;
    } else if (c == '(') {
        strcpy(lexeme, "(");
        return LPAREN;
    } else if (c == ')') {
        strcpy(lexeme, ")");
        return RPAREN;
    } else if (isalpha(c)) {
        lexeme[0] = c;
        // long variable name
        // accept char, int, underscore
        c = fgetc(stdin);
        i = 1;
        while ( (isalpha(c) || isdigit(c) || c == '_' ) && i < MAXLEN) {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return ID;
    } else if (c == EOF) {
        return ENDFILE;
    } else {
        return UNKNOWN;
    }
}

void advance(void) {
    curToken = getToken();
}

int match(TokenSet token) {
    if (curToken == UNKNOWN)
        advance();
    return token == curToken;
}

char *getLexeme(void) {
    return lexeme;
}


/*============================================================================================
parser implementation
============================================================================================*/

int sbcount = 0, mem_count = 0;
Symbol table[TBLSIZE];

void initTable(void) {
    strcpy(table[0].name, "x");
    table[0].address = 0;
    strcpy(table[1].name, "y");
    table[1].address = 4;
    strcpy(table[2].name, "z");
    table[2].address = 8;
    sbcount = 3;
    mem_count = 3;
}

int getval(char *str) {
    int i = 0;

    for (i = 0; i < sbcount; i++)
        if (strcmp(str, table[i].name) == 0)
            return table[i].val;

    if (sbcount >= TBLSIZE)
        error(RUNOUT);
    
    // Undefined variable
    error(NOTFOUND);
    return 0;
}

int getaddress(char* str) {
    int i = 0;
    for (i = 0; i < mem_count; i++) {
        if (strcmp(str, table[i].name) == 0) {
            return table[i].address;
        }
    }
    // error(NOTFOUND);
    return -1;
}

int setaddress(char *str) {
    int i = 0;

    for (i = 0; i < mem_count; i++) {
        if (strcmp(str, table[i].name) == 0) {
            return table[i].address;
        }
    }

    if (mem_count >= TBLSIZE)
        error(RUNOUT);
    
    strcpy(table[mem_count].name, str);
    table[mem_count].address = (mem_count) * 4;
    return table[mem_count++].address;
}

int setval(char *str, int val) {
    int i = 0;

    for (i = 0; i < sbcount; i++) {
        if (strcmp(str, table[i].name) == 0) {
            table[i].val = val;
            return val;
        }
    }

    if (sbcount >= TBLSIZE)
        error(RUNOUT);


    strcpy(table[sbcount].name, str);
    table[sbcount].val = val;
    sbcount++;
    return val;
}

BTNode *makeNode(TokenSet tok, const char *lexe) {
    BTNode *node = (BTNode*)malloc(sizeof(BTNode));
    strcpy(node->lexeme, lexe);
    node->data = tok;
    node->val = 0;
    node->left = NULL;
    node->right = NULL;
    return node;
}

void freeTree(BTNode *root) {
    if (root != NULL) {
        freeTree(root->left);
        freeTree(root->right);
        free(root);
    }
}



// assign_expr := ID ASSIGN assign_expr | ID ADDSUB_ASSIGN assign_expr | or_expr
BTNode *assign_expr(void) {
    BTNode *left = or_expr(), *retp;
    
    if ( match(ASSIGN) ) {
        if (!left->is_lvalue || left->data != ID) {
            error(NOTLVAL);
        }
        retp = makeNode(ASSIGN, getLexeme());
        advance();
        retp->left = left; // ID
        retp->right = assign_expr(); // assign_expr
    } else if ( match(ADDSUB_ASSIGN) ) {
        if(!left->is_lvalue  || left->data != ID) {
            error(NOTLVAL);
        }
        retp = makeNode(ADDSUB_ASSIGN, getLexeme());
        advance();
        retp->left = left; // ID
        retp->right = assign_expr(); // assign_expr
    } else {
        retp = left;
    }

    return retp;
    
}
// or_expr := xor_expr or_expr_tail
BTNode *or_expr(void) {
    BTNode *node = xor_expr();
    return or_expr_tail(node);
}

// or_expr_tail := OR xor_expr or_expr_tail | NiL
BTNode *or_expr_tail(BTNode *left) {
    BTNode *node = NULL;

    if (match(OR)) {
        node = makeNode(OR, getLexeme());
        advance();
        node->left = left;
        node->right = xor_expr();
        return or_expr_tail(node);
    } else {
        return left;
    }
}
// xor_expr := and_expr xor_expr_tail
BTNode *xor_expr(void) {
    BTNode *node = and_expr();
    return xor_expr_tail(node);
}
// xor_expr_tail := XOR and_expr xor_expr_tail | NiL
BTNode *xor_expr_tail(BTNode *left) {
    BTNode *node = NULL;

    if (match(XOR)) {
        node = makeNode(XOR, getLexeme());
        advance();
        node->left = left;
        node->right = and_expr();
        return xor_expr_tail(node);
    } else {
        return left;
    }
}
// and_expr := addsub_expr and_expr_tail
BTNode *and_expr(void) {
    BTNode *node = addsub_expr();
    return and_expr_tail(node);
}
// and_expr_tail := AND addsub_expr and_expr_tail | NiL
BTNode *and_expr_tail(BTNode *left) {
    BTNode *node = NULL;

    if (match(AND)) {
        node = makeNode(AND, getLexeme());
        advance();
        node->left = left;
        node->right = addsub_expr();
        return and_expr_tail(node);
    } else {
        return left;
    }
}
// addsub_expr := muldiv_expr addsub_expr_tail
BTNode *addsub_expr(void) {
    BTNode *node = muldiv_expr();
    return addsub_expr_tail(node);
}
// addsub_expr_tail := ADDSUB muldiv_expr addsub_expr_tail | NiL
BTNode *addsub_expr_tail(BTNode *left) {
    BTNode *node = NULL;

    if (match(ADDSUB)) {
        node = makeNode(ADDSUB, getLexeme());
        advance();
        node->left = left;
        node->right = muldiv_expr();
        return addsub_expr_tail(node);
    } else {
        return left;
    }
}
// muldiv_expr := unary_expr muldiv_expr_tail
BTNode *muldiv_expr(void) {
    BTNode *node = unary_expr();
    return muldiv_expr_tail(node);
}
// muldiv_expr_tail := MULDIV unary_expr muldiv_expr_tail | NiL
BTNode *muldiv_expr_tail(BTNode *left) {
    BTNode *node = NULL;

    if (match(MULDIV)) {
        node = makeNode(MULDIV, getLexeme());
        advance();
        node->left = left;
        node->right = unary_expr();
        return muldiv_expr_tail(node);
    } else {
        return left;
    }
}
// unary_expr := ADDSUB unary_expr | factor
BTNode *unary_expr(void) {
    BTNode* node = NULL;

    if (match(ADDSUB)) {
        node = makeNode(ADDSUB, getLexeme());
        advance();
        node->left = makeNode(INT, "0");
        node->right = unary_expr();
    } else {
        return factor();
    }
    return node;
}
// factor := INT | ID | INCDEC ID | LPAREN assign_expr RPAREN
BTNode *factor(void) {
    BTNode *retp = NULL, *left = NULL;

    if (match(INT)) {
        retp = makeNode(INT, getLexeme());
        advance();
    } else if (match(ID)) {
        left = makeNode(ID, getLexeme());
        advance();
        left->is_lvalue = 1;
        retp = left;
    } else if (match(INCDEC)) {
        retp = makeNode(INCDEC, getLexeme());
        retp->left = makeNode(INT, "0");
        advance();
        if (match(ID)) {
            retp->right = makeNode(ID, getLexeme());
            advance();
        } else {
            error(NOTNUMID);
        }
    } 
    else if (match(LPAREN)) {
        advance();
        retp = assign_expr();
        retp->is_lvalue = 0;
        if (match(RPAREN))
            advance();
        else
            error(MISPAREN);
    } else {
        error(NOTNUMID);
    }
    return retp;
}


// statement := ENDFILE | END | expr END
int statement(void) {
    // puts("in");
    BTNode *retp = NULL;

    if (match(ENDFILE)) {
        return 1;
    } else if (match(END)) {
        // printf(">> ");
        advance();
    } else {
        retp = assign_expr();
        if (match(END)) {
            // Check syntax
            if (DEBUG)
                printf("%d\n", evaluateTree(retp));
            else
                evaluateTree(retp);
            
            // printf(" === Assembly code ===\n");
            genCode(retp);
            // printf(" === Assembly code ===\n");
            // printf("Prefix traversal: ");
            // printPrefix(retp);
            // printf("\n");
            freeTree(retp);
            // printf(">> ");
            advance();
        } else {
            error(SYNTAXERR);
        }
    }
}

void err(ErrorType errorNum) {
    if (PRINTERR) {
        fprintf(stderr, "error: ");
        switch (errorNum) {
            case MISPAREN:
                fprintf(stderr, "mismatched parenthesis\n");
                break;
            case NOTNUMID:
                fprintf(stderr, "number or identifier expected\n");
                break;
            case NOTFOUND:
                fprintf(stderr, "variable not defined\n");
                break;
            case RUNOUT:
                fprintf(stderr, "out of memory\n");
                break;
            case NOTLVAL:
                fprintf(stderr, "lvalue required as an operand\n");
                break;
            case DIVZERO:
                fprintf(stderr, "divide by constant zero\n");
                break;
            case SYNTAXERR:
                fprintf(stderr, "syntax error\n");
                break;
            case NOTASSIGN:
                fprintf(stderr, " \"=\" expected\n");
                break;
            default:
                fprintf(stderr, "undefined error\n");
                break;
        }
    }
    printf("EXIT 1\n");
    exit(0);
}



/*============================================================================================
codeGen implementation
============================================================================================*/

int has_ID;

#define REG_NUMBER 8

int reg_used[REG_NUMBER]; // 8 register
int mem_idx = 0;
int variable_reg[256]; // address of variable in regiser

int alloc_reg(void) {
    for ( int i = 0 ; i < REG_NUMBER ; ++i ) {
        if ( !reg_used[i] ) {
            reg_used[i] = 1;
            return i;
        }
    }
    puts("\n\n\nRunning out of registers\n\n\n");
    return -1;
}

void free_reg(int r) {
    reg_used[r] = 0;
}

/*

Rule for genCode:

1. r0
r0

2. r1 = r0
MOV r1 r0
return r1;

3. r1 OP r0
OP r1 r0
return r1;

*/

void mov_rm(int r, int m) {
    printf("MOV r%d [%d]\n", r, m);
}

void mov_mr(int m, int r) {
    printf("MOV [%d] r%d\n", m, r);
}

void mov_rc(int r, int c) {
    printf("MOV r%d %d\n", r, c);
}

void mov_rr(int r, int R) {
    printf("MOV r%d r%d\n", r, R);
}

int genCode(BTNode *root) {
    int retaddr = 0, r0 = 0, r1 = 0;
    int l_mem;
    char* op = NULL;
    if (root != NULL) {
        switch (root->data) {
            case ID:
                retaddr = alloc_reg();
                mov_rm(retaddr, getaddress(root->lexeme));
                break;
            case INT:
                retaddr = alloc_reg();
                mov_rc(retaddr, atoi(root->lexeme));
                break;
            case ASSIGN:
                l_mem = getaddress(root->left->lexeme);
                if ( l_mem == -1 ) {
                    // not found
                    l_mem = setaddress(root->left->lexeme);
                }
                r1 = genCode(root->right);
                mov_mr(l_mem, r1);
                free_reg(r1);
                break;
            case ADDSUB_ASSIGN:
                l_mem = getaddress(root->left->lexeme);
                if ( l_mem == -1 ) {
                    // not found
                    l_mem = setaddress(root->left->lexeme);
                }
                r0 = alloc_reg();
                mov_rm(r0, l_mem);
                r1 = genCode(root->right);
                if ( strcmp(root->lexeme, "+=") == 0 ) {
                    printf("ADD r%d r%d\n", r0, r1);
                } else {
                    printf("SUB r%d r%d\n", r0, r1);
                }
                mov_mr(l_mem, r0);
                free_reg(r1);
                free_reg(r0);
                break;
            case INCDEC:
                l_mem = getaddress(root->right->lexeme);
                retaddr = alloc_reg();
                mov_rm(retaddr, l_mem);
                r0 = alloc_reg();
                mov_rc(r0, 1);
                if ( strcmp(root->lexeme, "++") == 0 )
                    printf("ADD r%d r%d\n", retaddr, r0);
                else
                    printf("SUB r%d r%d\n", retaddr, r0);
                free_reg(r0);
                mov_mr(l_mem, retaddr);
                break;
            case ADDSUB:
            case MULDIV:

                r0 = genCode(root->left);
                r1 = genCode(root->right);

                if (strcmp(root->lexeme, "+") == 0) {
                    op = "ADD";
                } else if (strcmp(root->lexeme, "-") == 0) {
                    op = "SUB";
                } else if (strcmp(root->lexeme, "*") == 0) {
                    op = "MUL";
                } else if (strcmp(root->lexeme, "/") == 0) {
                    op = "DIV";
                }

                printf("%s r%d r%d\n", op, r0, r1);
                retaddr = r0;
                free_reg(r1);
                break;
            case AND:
            case OR:
            case XOR:
                r0 = genCode(root->left);
                r1 = genCode(root->right);

                if (strcmp(root->lexeme, "&") == 0) {
                    op = "AND";
                } else if (strcmp(root->lexeme, "|") == 0) {
                    op = "OR";
                } else if (strcmp(root->lexeme, "^") == 0) {
                    op = "XOR";
                }

                printf("%s r%d r%d\n", op, r0, r1);
                retaddr = r0;
                free_reg(r1);
                break;
            default:
                retaddr = 0;
        }
    }
    return retaddr;
}

int evaluateTree(BTNode *root) {
    int retval = 0, lv = 0, rv = 0;

    if (root != NULL) {
        switch (root->data) {
            case ID:
                has_ID = 1;
                retval = getval(root->lexeme);
                break;
            case INT:
                retval = atoi(root->lexeme);
                break;
            case ASSIGN:
                rv = evaluateTree(root->right);
                retval = setval(root->left->lexeme, rv);
                break;
            case ADDSUB_ASSIGN:
                rv = evaluateTree(root->right);
                if ( strcmp(root->lexeme, "+=") == 0 )
                    retval = setval(root->left->lexeme, getval(root->left->lexeme) + rv);
                else
                    retval = setval(root->left->lexeme, getval(root->left->lexeme) - rv);
                break;
            case INCDEC:
                // printf("var = %s, val = %d\n", root->right->lexeme, getval(root->right->lexeme));
                if ( strcmp(root->lexeme, "++") == 0 )
                    retval = setval(root->right->lexeme, getval(root->right->lexeme)+1);
                else
                    retval = setval(root->right->lexeme, getval(root->right->lexeme)-1);
                break;
            case ADDSUB:
            case MULDIV:
                lv = evaluateTree(root->left);
                has_ID = 0;
                rv = evaluateTree(root->right);
                if (strcmp(root->lexeme, "+") == 0) {
                    retval = lv + rv;
                } else if (strcmp(root->lexeme, "-") == 0) {
                    retval = lv - rv;
                } else if (strcmp(root->lexeme, "*") == 0) {
                    retval = lv * rv;
                } else if (strcmp(root->lexeme, "/") == 0) {

                    
                    if (rv == 0) {
                        if ( has_ID ) {
                            retval = -1;
                        } else {
                            error(DIVZERO);
                        }
                    }
                    has_ID = 0;
                    retval = lv / rv;
                }
                break;
            case AND:
            case OR:
            case XOR:
                lv = evaluateTree(root->left);
                rv = evaluateTree(root->right);
                if (strcmp(root->lexeme, "&") == 0) {
                    retval = lv & rv;
                } else if (strcmp(root->lexeme, "|") == 0) {
                    retval = lv | rv;
                } else if (strcmp(root->lexeme, "^") == 0) {
                    retval = lv ^ rv;
                }
                break;
            default:
                retval = 0;
        }
    }
    return retval;
}

void printPrefix(BTNode *root) {
    if (root != NULL) {
        printf("%s ", root->lexeme);
        printPrefix(root->left);
        printPrefix(root->right);
    }
}



/*============================================================================================
main
============================================================================================*/

// This package is a calculator
// It works like a Python interpretor
// Example:
// >> y = 2
// >> z = 2
// >> x = 3 * y + 4 / (2 * z)
// It will print the answer of every line
// You should turn it into an expression compiler
// And print the assembly code according to the input

// This is the grammar used in this package
// You can modify it according to the spec and the slide
// statement  :=  ENDFILE | END | expr END
// expr    	  :=  term expr_tail
// expr_tail  :=  ADDSUB term expr_tail | NiL
// term 	  :=  factor term_tail
// term_tail  :=  MULDIV factor term_tail| NiL
// factor	  :=  INT | ADDSUB INT |
//		   	      ID  | ADDSUB ID  |
//		   	      ID ASSIGN expr |
//		   	      LPAREN expr RPAREN |
//		   	      ADDSUB LPAREN expr RPAREN

int main() {
    if (!DEBUG)
        // freopen("input.txt", "w", stdout);
        ;
    initTable();
    // printf(">> ");
    while (statement() != 1) {
        ;
    }
    // store x, y, z to reg 0, 1, 2
    printf("MOV r0 [0]\nMOV r1 [4]\nMOV r2 [8]\n");
    printf("EXIT 0");
    return 0;
}
