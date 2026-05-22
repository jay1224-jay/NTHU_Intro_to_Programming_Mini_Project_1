#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXLEN 256
#define TBLSIZE 256
#define PRINTERR 0
#define DEBUG 0
#define max(a, b) (a > b ? a : b)

typedef enum {
    UNKNOWN, END, ENDFILE,
    INT, ID,
    ADDSUB, MULDIV,
    INCDEC,
    AND, OR, XOR,
    ADDSUB_ASSIGN,
    ASSIGN,
    LPAREN, RPAREN
} TokenSet;

typedef enum {
    UNDEFINED, MISPAREN, NOTNUMID, NOTFOUND, RUNOUT, NOTLVAL, DIVZERO, SYNTAXERR, NOTASSIGN
} ErrorType;

typedef struct {
    int val;
    char name[MAXLEN];
    int address;
} Symbol;

typedef struct _Node {
    TokenSet data;
    int val;
    int is_lvalue;
    char lexeme[MAXLEN];
    struct _Node *left;
    struct _Node *right;
} BTNode;

void err(ErrorType errorNum);
#define error(errorNum) { err(errorNum); }

int match(TokenSet token);
void advance(void);
char *getLexeme(void);

void initTable(void);
int getval(char *str);
int getaddress(char *str);
int setval(char *str, int val);
int setaddress(char *str);
BTNode *makeNode(TokenSet tok, const char *lexe);
void freeTree(BTNode *root);

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
BTNode *unary_expr(void);
BTNode *factor(void);
int statement(void);

int evaluateTree(BTNode *root);
int genCode(BTNode *root);

/* ---- symbol table ---- */
int sbcount = 0, mem_count = 0;
Symbol table[TBLSIZE];

void initTable(void) {
    strcpy(table[0].name, "x"); table[0].address = 0;
    strcpy(table[1].name, "y"); table[1].address = 4;
    strcpy(table[2].name, "z"); table[2].address = 8;
    sbcount = 3;
    mem_count = 3;
}

int getval(char *str) {
    for (int i = 0; i < sbcount; i++)
        if (strcmp(str, table[i].name) == 0)
            return table[i].val;
    error(NOTFOUND);
    return 0;
}

int getaddress(char *str) {
    for (int i = 0; i < mem_count; i++)
        if (strcmp(str, table[i].name) == 0)
            return table[i].address;
    return -1;
}

int setaddress(char *str) {
    for (int i = 0; i < mem_count; i++)
        if (strcmp(str, table[i].name) == 0)
            return table[i].address;
    if (mem_count >= TBLSIZE) error(RUNOUT);
    strcpy(table[mem_count].name, str);
    table[mem_count].address = mem_count * 4;
    return table[mem_count++].address;
}

int setval(char *str, int val) {
    for (int i = 0; i < sbcount; i++) {
        if (strcmp(str, table[i].name) == 0) {
            table[i].val = val;
            return val;
        }
    }
    if (sbcount >= TBLSIZE) error(RUNOUT);
    strcpy(table[sbcount].name, str);
    table[sbcount].val = val;
    sbcount++;
    return val;
}

/* ---- lexer ---- */
static TokenSet curToken = UNKNOWN;
static char lexeme[MAXLEN];

static TokenSet getToken(void) {
    int i = 0;
    char c = '\0';

    while ((c = fgetc(stdin)) == ' ' || c == '\t');

    if (isdigit(c)) {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while (isdigit(c) && i < MAXLEN) {
            lexeme[i++] = c;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return INT;
    } else if (c == '&') {
        lexeme[0] = c; lexeme[1] = '\0';
        return AND;
    } else if (c == '|') {
        lexeme[0] = c; lexeme[1] = '\0';
        return OR;
    } else if (c == '^') {
        lexeme[0] = c; lexeme[1] = '\0';
        return XOR;
    } else if (c == '+' || c == '-') {
        lexeme[0] = c;
        c = fgetc(stdin);
        if (c == lexeme[0]) {
            lexeme[1] = c; lexeme[2] = '\0';
            return INCDEC;
        } else if (c == '=') {
            lexeme[1] = c; lexeme[2] = '\0';
            return ADDSUB_ASSIGN;
        }
        ungetc(c, stdin);
        lexeme[1] = '\0';
        return ADDSUB;
    } else if (c == '*' || c == '/') {
        lexeme[0] = c; lexeme[1] = '\0';
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
    } else if (isalpha(c) || c == '_') {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while ((isalpha(c) || isdigit(c) || c == '_') && i < MAXLEN) {
            lexeme[i++] = c;
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

void advance(void) { curToken = getToken(); }

int match(TokenSet token) {
    if (curToken == UNKNOWN) advance();
    return token == curToken;
}

char *getLexeme(void) { return lexeme; }

/* ---- tree ---- */
BTNode *makeNode(TokenSet tok, const char *lexe) {
    BTNode *node = (BTNode*)malloc(sizeof(BTNode));
    strcpy(node->lexeme, lexe);
    node->data = tok;
    node->val = 0;
    node->left = NULL;
    node->right = NULL;
    node->is_lvalue = 0;
    return node;
}

void freeTree(BTNode *root) {
    if (root) {
        freeTree(root->left);
        freeTree(root->right);
        free(root);
    }
}

/* ---- parser ---- */

// assign_expr := ID ASSIGN assign_expr | ID ADDSUB_ASSIGN assign_expr | or_expr
BTNode *assign_expr(void) {
    BTNode *left = or_expr(), *retp;

    if (match(ASSIGN)) {
        if (left->is_lvalue != 1 || left->data != ID)
            error(NOTLVAL);
        retp = makeNode(ASSIGN, getLexeme());
        advance();
        retp->left = left;
        retp->right = assign_expr();
    } else if (match(ADDSUB_ASSIGN)) {
        if (left->is_lvalue != 1 || left->data != ID)
            error(NOTLVAL);
        BTNode *node = makeNode(ASSIGN, "=");
        node->left = left;
        node->right = makeNode(ADDSUB, (getLexeme()[0] == '+') ? "+" : "-");
        advance();
        node->right->left = makeNode(ID, left->lexeme);
        node->right->right = assign_expr();
        return node;
    } else {
        retp = left;
    }
    return retp;
}

BTNode *or_expr(void) {
    BTNode *node = xor_expr();
    return or_expr_tail(node);
}

BTNode *or_expr_tail(BTNode *left) {
    if (match(OR)) {
        BTNode *node = makeNode(OR, getLexeme());
        advance();
        node->left = left;
        node->right = xor_expr();
        return or_expr_tail(node);
    }
    return left;
}

BTNode *xor_expr(void) {
    BTNode *node = and_expr();
    return xor_expr_tail(node);
}

BTNode *xor_expr_tail(BTNode *left) {
    if (match(XOR)) {
        BTNode *node = makeNode(XOR, getLexeme());
        advance();
        node->left = left;
        node->right = and_expr();
        return xor_expr_tail(node);
    }
    return left;
}

BTNode *and_expr(void) {
    BTNode *node = addsub_expr();
    return and_expr_tail(node);
}

BTNode *and_expr_tail(BTNode *left) {
    if (match(AND)) {
        BTNode *node = makeNode(AND, getLexeme());
        advance();
        node->left = left;
        node->right = addsub_expr();
        return and_expr_tail(node);
    }
    return left;
}

BTNode *addsub_expr(void) {
    BTNode *node = muldiv_expr();
    return addsub_expr_tail(node);
}

BTNode *addsub_expr_tail(BTNode *left) {
    if (match(ADDSUB)) {
        BTNode *node = makeNode(ADDSUB, getLexeme());
        advance();
        node->left = left;
        node->right = muldiv_expr();
        return addsub_expr_tail(node);
    }
    return left;
}

BTNode *muldiv_expr(void) {
    BTNode *node = unary_expr();
    return muldiv_expr_tail(node);
}

BTNode *muldiv_expr_tail(BTNode *left) {
    if (match(MULDIV)) {
        BTNode *node = makeNode(MULDIV, getLexeme());
        advance();
        node->left = left;
        node->right = unary_expr();
        return muldiv_expr_tail(node);
    }
    return left;
}

BTNode *unary_expr(void) {
    if (match(ADDSUB)) {
        BTNode *node = makeNode(ADDSUB, getLexeme());
        advance();
        node->left = makeNode(INT, "0");
        node->right = unary_expr();
        return node;
    }
    return factor();
}

BTNode *factor(void) {
    BTNode *retp = NULL;

    if (match(INT)) {
        retp = makeNode(INT, getLexeme());
        advance();
    } else if (match(ID)) {
        retp = makeNode(ID, getLexeme());
        advance();
        retp->is_lvalue = 1;
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
    } else if (match(LPAREN)) {
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

int statement(void) {
    BTNode *retp = NULL;

    if (match(ENDFILE)) {
        printf("MOV r0 [0]\nMOV r1 [4]\nMOV r2 [8]\n");
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
    return 0;
}

void err(ErrorType errorNum) {
    if (PRINTERR) {
        fprintf(stderr, "error: ");
        switch (errorNum) {
            case MISPAREN:   fprintf(stderr, "mismatched parenthesis\n"); break;
            case NOTNUMID:   fprintf(stderr, "number or identifier expected\n"); break;
            case NOTFOUND:   fprintf(stderr, "variable not defined\n"); break;
            case RUNOUT:     fprintf(stderr, "out of memory\n"); break;
            case NOTLVAL:    fprintf(stderr, "lvalue required as an operand\n"); break;
            case DIVZERO:    fprintf(stderr, "divide by constant zero\n"); break;
            case SYNTAXERR:  fprintf(stderr, "syntax error\n"); break;
            default:         fprintf(stderr, "undefined error\n"); break;
        }
    }
    printf("EXIT 1\n");
    exit(0);
}

/* ---- code generation ---- */
#define REG_NUMBER 8
int reg_used[REG_NUMBER];
int has_ID;

int alloc_reg(void) {
    for (int i = 0; i < REG_NUMBER; i++) {
        if (!reg_used[i]) {
            reg_used[i] = 1;
            return i;
        }
    }
    return -1;
}

void reset_reg(void) {
    for (int i = 0; i < REG_NUMBER; i++)
        reg_used[i] = 0;
}

void free_reg(int r) {
    reg_used[r] = 0;
}

int find_height(BTNode *root) {
    if (!root) return 0;
    int lh = find_height(root->left);
    int rh = find_height(root->right);
    return 1 + max(lh, rh);
}

int evaluateTree(BTNode *root) {
    int retval = 0, lv = 0, rv = 0;
    if (!root) return 0;
    switch (root->data) {
        case ID:
            has_ID = 1;
            retval = 0;
            break;
        case INT:
            retval = atoi(root->lexeme);
            break;
        case ASSIGN:
            has_ID = 1;
            rv = evaluateTree(root->right);
            retval = setval(root->left->lexeme, rv);
            break;
        case ADDSUB_ASSIGN:
            has_ID = 1;
            rv = evaluateTree(root->right);
            if (strcmp(root->lexeme, "+=") == 0)
                retval = setval(root->left->lexeme, getval(root->left->lexeme) + rv);
            else
                retval = setval(root->left->lexeme, getval(root->left->lexeme) - rv);
            break;
        case INCDEC:
            has_ID = 1;
            if (strcmp(root->lexeme, "++") == 0)
                retval = setval(root->right->lexeme, getval(root->right->lexeme) + 1);
            else
                retval = setval(root->right->lexeme, getval(root->right->lexeme) - 1);
            break;
        case ADDSUB:
        case MULDIV:
            lv = evaluateTree(root->left);
            rv = evaluateTree(root->right);
            if      (strcmp(root->lexeme, "+") == 0) retval = lv + rv;
            else if (strcmp(root->lexeme, "-") == 0) retval = lv - rv;
            else if (strcmp(root->lexeme, "*") == 0) retval = lv * rv;
            else if (strcmp(root->lexeme, "/") == 0) retval = lv / rv;
            break;
        case AND: case OR: case XOR:
            lv = evaluateTree(root->left);
            rv = evaluateTree(root->right);
            if      (strcmp(root->lexeme, "&") == 0) retval = lv & rv;
            else if (strcmp(root->lexeme, "|") == 0) retval = lv | rv;
            else if (strcmp(root->lexeme, "^") == 0) retval = lv ^ rv;
            break;
        default:
            retval = 0;
    }
    return retval;
}

int genCode(BTNode *root) {
    int retaddr = 0, r0 = 0, r1 = 0;
    int l_mem;
    char *op = NULL;
    int h0, h1;

    if (!root) return 0;

    switch (root->data) {
        case ID:

            /*
            TODO: Register Value Caching
            */

            retaddr = alloc_reg();
            if (getaddress(root->lexeme) == -1)
                error(NOTFOUND);
            printf("MOV r%d [%d]\n", retaddr, getaddress(root->lexeme));
            break;

        case INT:
            retaddr = alloc_reg();
            printf("MOV r%d %d\n", retaddr, atoi(root->lexeme));
            break;

        case ASSIGN:
            l_mem = getaddress(root->left->lexeme);
            if (l_mem == -1)
                l_mem = setaddress(root->left->lexeme);
            r1 = genCode(root->right);
            printf("MOV [%d] r%d\n", l_mem, r1);
            retaddr = r1;
            break;

        case INCDEC:
            l_mem = getaddress(root->right->lexeme);
            if (l_mem == -1)
                error(NOTFOUND);
            retaddr = alloc_reg();
            printf("MOV r%d [%d]\n", retaddr, l_mem);
            r0 = alloc_reg();
            printf("MOV r%d 1\n", r0);
            if (strcmp(root->lexeme, "++") == 0)
                printf("ADD r%d r%d\n", retaddr, r0);
            else
                printf("SUB r%d r%d\n", retaddr, r0);
            free_reg(r0);
            printf("MOV [%d] r%d\n", l_mem, retaddr);
            break;

        case ADDSUB:
        case MULDIV:
            h0 = find_height(root->left);
            h1 = find_height(root->right);
            if (h0 > h1) {
                r0 = genCode(root->left);
                r1 = genCode(root->right);
            } else {
                r1 = genCode(root->right);
                r0 = genCode(root->left);
            }
            if      (strcmp(root->lexeme, "+") == 0) op = "ADD";
            else if (strcmp(root->lexeme, "-") == 0) op = "SUB";
            else if (strcmp(root->lexeme, "*") == 0) op = "MUL";
            else if (strcmp(root->lexeme, "/") == 0) {
                op = "DIV";
                has_ID = 0;
                if (evaluateTree(root->right) == 0 && !has_ID)
                    error(DIVZERO);
            }
            printf("%s r%d r%d\n", op, r0, r1);
            retaddr = r0;
            free_reg(r1);
            break;

        case AND:
        case OR:
        case XOR:
            h0 = find_height(root->left);
            h1 = find_height(root->right);
            if (h0 > h1) {
                r0 = genCode(root->left);
                r1 = genCode(root->right);
            } else {
                r1 = genCode(root->right);
                r0 = genCode(root->left);
            }
            if      (strcmp(root->lexeme, "&") == 0) op = "AND";
            else if (strcmp(root->lexeme, "|") == 0) op = "OR";
            else if (strcmp(root->lexeme, "^") == 0) op = "XOR";
            printf("%s r%d r%d\n", op, r0, r1);
            retaddr = r0;
            free_reg(r1);
            break;

        default:
            retaddr = 0;
    }
    return retaddr;
}

int main(void) {
    initTable();
    while (statement() != 1) {
        reset_reg();
    }
    return 0;
}