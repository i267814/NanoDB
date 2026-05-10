#pragma once
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "structures.h"
#include "types.h"

static const int MAX_TOKENS = 128;
static const int TOK_LEN    = 64;

// ─── Token ───────────────────────────────────────────────────────────────────
struct Token {
    char val[TOK_LEN];
    bool isOp;      // operator or keyword
    bool isLParen;
    bool isRParen;
    Token() { val[0]='\0'; isOp=false; isLParen=false; isRParen=false; }
};

inline bool isOperator(const char* s) {
    return (strcmp(s,">")==0 || strcmp(s,"<")==0 || strcmp(s,"==")==0 ||
            strcmp(s,"!=")==0|| strcmp(s,">=")==0 || strcmp(s,"<=")==0 ||
            strcmp(s,"AND")==0|| strcmp(s,"OR")==0 ||
            strcmp(s,"+")==0 || strcmp(s,"-")==0 ||
            strcmp(s,"*")==0 || strcmp(s,"/")==0 || strcmp(s,"%")==0);
}

inline int precedence(const char* op) {
    if (strcmp(op,"OR")==0)                     return 1;
    if (strcmp(op,"AND")==0)                    return 2;
    if (strcmp(op,"==")==0||strcmp(op,"!=")==0) return 3;
    if (strcmp(op,"<")==0 ||strcmp(op,">")==0 ||
        strcmp(op,"<=")==0||strcmp(op,">=")==0) return 4;
    if (strcmp(op,"+")==0 ||strcmp(op,"-")==0)  return 5;
    if (strcmp(op,"*")==0 ||strcmp(op,"/")==0 ||strcmp(op,"%")==0) return 6;
    return 0;
}

// ─── Tokenizer ───────────────────────────────────────────────────────────────
inline int tokenize(const char* expr, Token tokens[], int maxTok) {
    int n = 0;
    const char* p = expr;
    while (*p && n < maxTok) {
        while (*p == ' ') p++;
        if (!*p) break;

        Token t;
        if (*p == '(') { t.val[0]='('; t.val[1]='\0'; t.isLParen=true; p++; }
        else if (*p == ')') { t.val[0]=')'; t.val[1]='\0'; t.isRParen=true; p++; }
        else if (*p=='>'||*p=='<'||*p=='!'||*p=='=') {
            int i=0;
            while ((*p=='>'||*p=='<'||*p=='!'||*p=='=') && i<3)
                t.val[i++]=*p++;
            t.val[i]='\0'; t.isOp=true;
        }
        else if (*p=='+'||*p=='-'||*p=='*'||*p=='/'||*p=='%') {
            t.val[0]=*p++; t.val[1]='\0'; t.isOp=true;
        }
        else if (*p=='"') {
            p++; int i=0;
            while (*p && *p!='"' && i<TOK_LEN-1) t.val[i++]=*p++;
            if (*p=='"') p++;
            t.val[i]='\0';
        }
        else {
            int i=0;
            while (*p && *p!=' ' && *p!='(' && *p!=')' && i<TOK_LEN-1)
                t.val[i++]=*p++;
            t.val[i]='\0';
            if (strcmp(t.val,"AND")==0||strcmp(t.val,"OR")==0) t.isOp=true;
        }
        tokens[n++] = t;
    }
    return n;
}

// ─── Infix → Postfix (Shunting-Yard) ─────────────────────────────────────────
inline int infixToPostfix(Token in[], int n, Token out[]) {
    Stack<Token> opStack(64);
    int outIdx = 0;

    for (int i = 0; i < n; i++) {
        Token& t = in[i];
        if (t.isLParen) { opStack.push(t); }
        else if (t.isRParen) {
            while (!opStack.empty() && !opStack.peek().isLParen)
                out[outIdx++] = opStack.pop();
            if (!opStack.empty()) opStack.pop(); // discard '('
        }
        else if (t.isOp) {
            while (!opStack.empty() && !opStack.peek().isLParen &&
                   precedence(opStack.peek().val) >= precedence(t.val))
                out[outIdx++] = opStack.pop();
            opStack.push(t);
        }
        else { out[outIdx++] = t; }
    }
    while (!opStack.empty()) out[outIdx++] = opStack.pop();
    return outIdx;
}

// ─── Print postfix expression ─────────────────────────────────────────────────
inline void printPostfix(Token pf[], int n) {
    printf("[LOG] Postfix: ");
    for (int i = 0; i < n; i++) printf("%s ", pf[i].val);
    printf("\n");
}

// ─── Evaluate postfix against a Row ──────────────────────────────────────────
// colNames[] and colCount tell us which column name maps to which field index
inline float resolveVal(const char* tok, Row* row, const char** colNames, int colCount) {
    // Is it a number?
    char* ep;
    float fv = strtof(tok, &ep);
    if (ep != tok) return fv;
    // Is it a column name?
    for (int i = 0; i < colCount; i++) {
        if (strcmp(tok, colNames[i]) == 0) {
            Field* f = row->fields[i];
            if (f->typeId() == 0) return (float)static_cast<IntField*>(f)->value;
            if (f->typeId() == 1) return static_cast<FloatField*>(f)->value;
        }
    }
    return 0.0f;
}

inline const char* resolveStr(const char* tok, Row* row, const char** colNames, int colCount) {
    for (int i = 0; i < colCount; i++) {
        if (strcmp(tok, colNames[i]) == 0) {
            Field* f = row->fields[i];
            if (f->typeId() == 2) return static_cast<StringField*>(f)->value;
        }
    }
    return tok;
}

// Returns true if row satisfies the postfix expression
inline bool evaluatePostfix(Token pf[], int n, Row* row,
                             const char** colNames, int colCount) {
    Stack<float> valStack(64);

    for (int i = 0; i < n; i++) {
        const char* tok = pf[i].val;
        if (!pf[i].isOp) {
            // push numeric / column value
            valStack.push(resolveVal(tok, row, colNames, colCount));
        } else {
            if (strcmp(tok,"AND")==0) {
                float b=valStack.pop(), a=valStack.pop();
                valStack.push((a!=0 && b!=0) ? 1.0f : 0.0f);
            } else if (strcmp(tok,"OR")==0) {
                float b=valStack.pop(), a=valStack.pop();
                valStack.push((a!=0 || b!=0) ? 1.0f : 0.0f);
            } else {
                float b=valStack.pop(), a=valStack.pop();
                float res=0;
                if      (strcmp(tok,">")==0)  res = a>b;
                else if (strcmp(tok,"<")==0)  res = a<b;
                else if (strcmp(tok,"==")==0) res = a==b;
                else if (strcmp(tok,"!=")==0) res = a!=b;
                else if (strcmp(tok,">=")==0) res = a>=b;
                else if (strcmp(tok,"<=")==0) res = a<=b;
                else if (strcmp(tok,"+")==0)  res = a+b;
                else if (strcmp(tok,"-")==0)  res = a-b;
                else if (strcmp(tok,"*")==0)  res = a*b;
                else if (strcmp(tok,"/")==0)  res = b!=0 ? a/b : 0;
                else if (strcmp(tok,"%")==0)  res = b!=0 ? (float)((int)a%(int)b) : 0;
                valStack.push(res);
            }
        }
    }
    return valStack.empty() ? false : (valStack.pop() != 0.0f);
}

// ─── String-aware comparison helper ──────────────────────────────────────────
inline bool evaluatePostfixStr(Token pf[], int n, Row* row,
                                const char** colNames, int colCount) {
    // For string comparisons (==, !=) we handle them specially
    // For purely numeric we fall through to evaluatePostfix
    // Simple pass: check if any token is a string literal in the postfix
    // We build a combined evaluator using a char* stack for strings
    // and a float stack for numbers. For brevity we handle the common pattern:
    // colname literal == (or !=)
    if (n == 3 && pf[2].isOp &&
        (strcmp(pf[2].val,"==")==0 || strcmp(pf[2].val,"!=")==0)) {
        const char* lhs = resolveStr(pf[0].val, row, colNames, colCount);
        const char* rhs = resolveStr(pf[1].val, row, colNames, colCount);
        bool eq = (strcmp(lhs,rhs)==0);
        return (strcmp(pf[2].val,"==")==0) ? eq : !eq;
    }
    return evaluatePostfix(pf, n, row, colNames, colCount);
}
