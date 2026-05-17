#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "codeGen.h"

int sbcount = 0;
Symbol table[TBLSIZE];

void initTable(void) {
    strcpy(table[0].name, "x");
    table[0].address = 0;
    strcpy(table[1].name, "y");
    table[1].address = 4;
    strcpy(table[2].name, "z");
    table[2].address = 8;
    sbcount = 3;
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
    for (i = 0; i < sbcount; i++) {
        if (strcmp(str, table[i].name) == 0) {
            return table[i].address;
        }
    }
    // error(NOTFOUND);
    return -1;
}

int setaddress(char *str) {
    int i = 0;

    for (i = 0; i < sbcount; i++) {
        if (strcmp(str, table[i].name) == 0) {
            return table[i].address;
        }
    }

    if (sbcount >= TBLSIZE)
        error(RUNOUT);
    
    strcpy(table[sbcount].name, str);
    table[sbcount].address = (sbcount) * 4;
    return table[sbcount++].address;
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
        if (!left->is_lvalue) {
            error(NOTLVAL);
        }
        retp = makeNode(ASSIGN, getLexeme());
        advance();
        retp->left = left; // ID
        retp->right = assign_expr(); // assign_expr
    } else if ( match(ADDSUB_ASSIGN) ) {
        if(!left->is_lvalue) {
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
        node->left = 0;
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
            
            printf("%d\n", evaluateTree(retp));
            
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
    exit(0);
}
