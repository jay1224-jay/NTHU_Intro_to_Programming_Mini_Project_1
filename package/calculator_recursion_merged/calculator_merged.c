#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
statement           := ENDFILE | END | assign_expr END
assign_expr         := ID ASSIGN assign_expr | ID ADDSUB_ASSIGN assign_expr | or_expr
or_expr             := xor_expr or_expr_tail
or_expr_tail        := OR xor_expr or_expr_tail | NiL
xor_expr            := and_expr xor_expr_tail
xor_expr_tail       := XOR and_expr xor_expr_tail | NiL
and_expr            := addsub_expr and_expr_tail
and_expr_tail       := AND addsub_expr and_expr_tail | NiL
addsub_expr         := muldiv_expr addsub_expr_tail
addsub_expr_tail    := ADDSUB muldiv_expr addsub_expr_tail | NiL
muldiv_expr         := unary_expr muldiv_expr_tail
muldiv_expr_tail    := MULDIV unary_expr muldiv_expr_tail | NiL
unary_expr          := ADDSUB unary_expr | factor
factor              := INT | ID | INCDEC ID | LPAREN assign_expr RPAREN
*/

// for lex
#define MAXLEN 256

// Token types
typedef enum {
    UNKNOWN, END, ENDFILE,
    INT, ID,
    ADDSUB, MULDIV,
    ASSIGN, ADDSUB_ASSIGN,
    AND, OR, XOR,
    INCDEC,
    LPAREN, RPAREN
} TokenSet;

TokenSet getToken(void);
TokenSet curToken = UNKNOWN;
char lexeme[MAXLEN];

// Test if a token matches the current token
int match(TokenSet token);
// Get the next token
void advance(void);
// Get the lexeme of the current token
char *getLexeme(void);


// for parser
#define TBLSIZE 64
// Set PRINTERR to 1 to print error message while calling error()
// Make sure you set PRINTERR to 0 before you submit your code
#define PRINTERR 0

// Call this macro to print error message and exit the program
// This will also print where you called it in your program
#define error(errorNum) { \
    if (PRINTERR) \
        fprintf(stderr, "error() called at %s:%d: ", __FILE__, __LINE__); \
    err(errorNum); \
}

// Error types
typedef enum {
    UNDEFINED, MISPAREN, NOTNUMID, NOTFOUND, RUNOUT, NOTLVAL, DIVZERO, SYNTAXERR
} ErrorType;

// Structure of the symbol table
typedef struct {
    int val;
    char name[MAXLEN];
} Symbol;

// Structure of a tree node
typedef struct _Node {
    TokenSet data;
    int val;
    char lexeme[MAXLEN];
    struct _Node *left;
    struct _Node *right;
} BTNode;

int sbcount = 0;
Symbol table[TBLSIZE];

// Initialize the symbol table with builtin variables
void initTable(void);
// Get the value of a variable
int getval(char *str);
// Set the value of a variable
int setval(char *str);
// Make a new node according to token type and lexeme
BTNode *makeNode(TokenSet tok, const char *lexe);
// Free the syntax tree
void freeTree(BTNode *root);
BTNode *factor(void);
BTNode *unary_expr(void);
BTNode *assign_expr(void);
BTNode *or_expr(void);
BTNode *or_expr_tail(BTNode *left);
BTNode *xor_expr(void);
BTNode *xor_expr_tail(BTNode *left);
BTNode *and_expr(void);
BTNode *and_expr_tail(BTNode *left);
BTNode *addsub_expr(void);
BTNode *addsub_expr_tail(BTNode *left);
BTNode *muldiv_expr(void);
BTNode *muldiv_expr_tail(BTNode *left);
void statement(void);
// Print error message and exit the program
void err(ErrorType errorNum);


// for codeGen
// Evaluate the syntax tree
int evaluateTree(BTNode *root);
int genCode(BTNode *root);
// Print the syntax tree in prefix
void printPrefix(BTNode *root);


/*============================================================================================
lex implementation
============================================================================================*/

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
    } else if (c == '+' || c == '-') {
        lexeme[0] = c;

        c = fgetc(stdin);
        if ( c == lexeme[0] ) {
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
    } else if (c == '&') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return AND;
    } else if (c == '|') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return OR;
    } else if (c == '^') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return XOR;
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
    } else if (isalpha(c) || c == '_' ) {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while ( isalpha(c) || isdigit(c) || c == '_' ) {
            lexeme[i] = c;
            c = fgetc(stdin);
            i++;
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

void initTable(void) {
    strcpy(table[0].name, "x");
    table[0].val = 0;
    strcpy(table[1].name, "y");
    table[1].val = 4;
    strcpy(table[2].name, "z");
    table[2].val = 8;
    sbcount = 3;
}

int getval(char *str) {
    int i = 0;

    for (i = 0; i < sbcount; i++)
        if (strcmp(str, table[i].name) == 0)
            return table[i].val;

    if (sbcount >= TBLSIZE)
        error(RUNOUT);

    error(NOTFOUND);
    return -1;
}

int setval(char *str) {
    int i = 0;

    for (i = 0; i < sbcount; i++) {
        if (strcmp(str, table[i].name) == 0) {
            return table[i].val;
        }
    }

    if (sbcount >= TBLSIZE)
        error(RUNOUT);

    strcpy(table[sbcount].name, str);
    table[sbcount].val = sbcount*4;
    sbcount++;
    return table[sbcount-1].val;
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
    if ( match(ASSIGN) && left->data == ID ) {
        advance();
        retp = makeNode(ASSIGN, "=");
        retp->left = left;
        retp->right = assign_expr();
    } else if ( match(ADDSUB_ASSIGN) && left->data == ID ) {
        char old = getLexeme()[0];
        advance();
        retp = makeNode(ASSIGN, "=");
        retp->left = left;
        if ( old == '+' )
            retp->right = makeNode(ADDSUB, "+");
        else
            retp->right = makeNode(ADDSUB, "-");
        retp->right->left = makeNode(ID, left->lexeme);
        retp->right->right = assign_expr();
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
    // printf("%s\n", getLexeme());
    if (match(INT)) {
        retp = makeNode(INT, getLexeme());
        advance();
    } else if (match(ID)) {
        retp = makeNode(ID, getLexeme());
        advance();
    } else if (match(INCDEC)) {
        char old_lex = getLexeme()[0];
        retp = makeNode(ASSIGN, "=");
        advance();
        if (match(ID)) {
            retp->left = makeNode(ID, getLexeme());
            if ( old_lex == '+' )
                retp->right = makeNode(ADDSUB, "+");
            else
                retp->right = makeNode(ADDSUB, "-");
            retp->right->left = makeNode(ID, retp->left->lexeme);
            retp->right->right = makeNode(INT, "1");
            advance();
        } else {
            error(NOTNUMID);
        }
    } 
    else if (match(LPAREN)) {
        advance();
        retp = assign_expr();
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
void statement(void) {
    BTNode *retp = NULL;

    if (match(ENDFILE)) {
        /*
        printf("MOV r0 [0]\n");
        printf("MOV r1 [4]\n");
        printf("MOV r2 [8]\n");
        */
        printf("EXIT 0\n");
        exit(0);
    } else if (match(END)) {
        advance();
    } else {
        retp = assign_expr();
        if (match(END)) {
            genCode(retp);
            freeTree(retp);
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

int reg_used[8] = {0};

int alloc_reg(void) {
    for ( int i = 3 ; i < 8 ; ++i ) {
        if ( !reg_used[i] ) {
            reg_used[i] = 1;
            return i;
        }
    }
    error(0);
    return -1;
}

void free_reg(int i) {
    if ( 3 <= i )
    reg_used[i] = 0;
}

void reset_reg(void) {
    for ( int i = 0 ; i < 8 ; ++i ) {
        free_reg(i);
    }
}

int evaluateTree(BTNode *root) {
    int retval = 0, lv = 0, rv = 0;

    if (root != NULL) {
        switch (root->data) {
            case ID:
                retval = 0;
                break;
            case INT:
                retval = atoi(root->lexeme);
                break;
            case ASSIGN:
                rv = evaluateTree(root->right);
                retval = setval(root->left->lexeme);
                break;
            case ADDSUB:
            case MULDIV:
            case AND:
            case OR:
            case XOR:
                lv = evaluateTree(root->left);
                rv = evaluateTree(root->right);
                if (strcmp(root->lexeme, "+") == 0) {
                    retval = lv + rv;
                } else if (strcmp(root->lexeme, "-") == 0) {
                    retval = lv - rv;
                } else if (strcmp(root->lexeme, "*") == 0) {
                    retval = lv * rv;
                } else if (strcmp(root->lexeme, "/") == 0) {
                    if (rv == 0)
                        error(DIVZERO);
                    retval = lv / rv;
                } else if (strcmp(root->lexeme, "&") == 0) {
                    retval = lv & rv;
                }  else if (strcmp(root->lexeme, "|") == 0) {
                    retval = lv | rv;
                }  else if (strcmp(root->lexeme, "^") == 0) {
                    retval = lv ^ rv;
                }
                break;
            default:
                retval = 0;
        }
    }
    return retval;
}

#define max(a, b) (a > b ? a : b)

int get_height(BTNode* root) {
    if ( root == NULL ) {
        return 0;
    }
    if ( strcmp(root->lexeme, "x") == 0 || strcmp(root->lexeme, "y") == 0 || strcmp(root->lexeme, "z") == 0 ) {
        return 0;
    }
    int lh = get_height(root->left);
    int rh = get_height(root->right);
    return max(lh, rh) + 1;
}

int hasID(BTNode* root) {
    if ( root == NULL ) {
        return 0;
    }
    if ( root->data == ID ) {
        return 1;
    }
    return hasID(root->left) || hasID(root->right);
}

int genCode(BTNode *root) {
    int retval = 0;
    int r0, r1, r_result, lh, rh;
    if (root != NULL) {
        switch (root->data) {
            case ID:
                if ( strcmp(root->lexeme, "x") == 0 ) {
                    return 0;
                }
                if ( strcmp(root->lexeme, "y") == 0 ) {
                    return 1;
                }
                if ( strcmp(root->lexeme, "z") == 0 ) {
                    return 2;
                }
                r0 = alloc_reg();
                retval = r0;
                printf("MOV r%d [%d]\n", r0, getval(root->lexeme));
                break;
            case INT:
                r0 = alloc_reg();
                retval = r0;
                printf("MOV r%d %d\n", r0, atoi(root->lexeme));
                break;
            case ASSIGN:
                r1 = genCode(root->right);
                // r0 = setval(root->left->lexeme);
                if ( strcmp(root->left->lexeme, "x") == 0 ) {
                    free_reg(r1);
                    printf("MOV r0 r%d\n", r1);
                    return 0;
                }
                if ( strcmp(root->left->lexeme, "y") == 0 ) {
                    free_reg(r1);
                    printf("MOV r1 r%d\n", r1);
                    return 1;
                }
                if ( strcmp(root->left->lexeme, "z") == 0 ) {
                    free_reg(r1);
                    printf("MOV r2 r%d\n", r1);
                    return 2;
                }
                printf("MOV [%d] r%d\n", setval(root->left->lexeme), r1);
                retval = r1;
                break;
            case ADDSUB:
            case MULDIV:
            case AND:
            case OR:
            case XOR:
                lh = get_height(root->left);
                rh = get_height(root->right);

                if ( lh > rh ) {
                    r0 = genCode(root->left);
                    r1 = genCode(root->right);
                } else {
                    r1 = genCode(root->right);
                    r0 = genCode(root->left);
                }
                if (r0 < 3) {
                    r_result = alloc_reg();
                    printf("MOV r%d r%d\n", r_result, r0);
                } else {
                    r_result = r0;
                }
                if (strcmp(root->lexeme, "+") == 0) {
                    printf("ADD r%d r%d\n", r_result, r1);
                } else if (strcmp(root->lexeme, "-") == 0) {
                    printf("SUB r%d r%d\n", r_result, r1);
                } else if (strcmp(root->lexeme, "*") == 0) {
                    printf("MUL r%d r%d\n", r_result, r1);
                } else if (strcmp(root->lexeme, "/") == 0) {
                    if (evaluateTree(root->right) == 0 && !hasID(root->right))
                        error(DIVZERO);
                    printf("DIV r%d r%d\n", r_result, r1);
                } else if (strcmp(root->lexeme, "&") == 0) {
                    printf("AND r%d r%d\n", r_result, r1);
                }  else if (strcmp(root->lexeme, "|") == 0) {
                    printf("OR r%d r%d\n", r_result, r1);
                }  else if (strcmp(root->lexeme, "^") == 0) {
                    printf("XOR r%d r%d\n", r_result, r1);
                }
                if (r0 < 3) {
                    free_reg(r1);
                    free_reg(r0);
                    retval = r_result;
                } else {
                    free_reg(r1);
                    retval = r0;
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
    initTable();
    freopen("input.txt", "w", stdout);
    printf("MOV r0 [0]\n");
    printf("MOV r1 [4]\n");
    printf("MOV r2 [8]\n");
    while (1) {
        statement();
        reset_reg();
        reg_used[0] = reg_used[1] = reg_used[2] = 1; // x y z
    }
    return 0;
}
