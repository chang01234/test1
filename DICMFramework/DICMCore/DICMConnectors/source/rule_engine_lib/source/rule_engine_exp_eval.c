#include "rule_engine_exp_eval.h"
#include "configuration.h"

#include "esp_heap_caps.h"

/**********************************************************
 * Private types
 *********************************************************/
#define EXPR_UNARY (1 << 5)
#define EXPR_COMMA (1 << 6)
#define EXPR_TNEXT (1 << 7)

#define EXPR_PAREN_ALLOWED   0
#define EXPR_PAREN_EXPECTED  1
#define EXPR_PAREN_FORBIDDEN 2

#define EXPR_OPERATION_DIVIDE(x, y)    (((y) == 0) ? 0 : ((x) / (y)))
#define EXPR_OPERATION_REMAINDER(x, y) (((y) == 0) ? 0 : ((x) % (y)))

#define isfirstvarchr(c) \
    (((unsigned char)c >= '@' && c != '^' && c != '|') || c == '$')
#define isvarchr(c)                                                   \
    (((unsigned char)c >= '@' && c != '^' && c != '|') || c == '$' || \
     c == '#' || (c >= '0' && c <= '9'))

static const int prec[] = {0, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 5,
                           5, 5, 5, 5, 6, 7, 8, 9, 10, 11, 12, 0, 0, 0};

static const struct expr_eval__ops OPS[] = {
    {"-u", OP_UNARY_MINUS},
    {"!u", OP_UNARY_LOGICAL_NOT},
    {"^u", OP_UNARY_BITWISE_NOT},
    {"**", OP_POWER},
    {"*", OP_MULTIPLY},
    {"/", OP_DIVIDE},
    {"%", OP_REMAINDER},
    {"+", OP_PLUS},
    {"-", OP_MINUS},
    {"<<", OP_SHL},
    {">>", OP_SHR},
    {"<", OP_LT},
    {"<=", OP_LE},
    {">", OP_GT},
    {">=", OP_GE},
    {"==", OP_EQ},
    {"!=", OP_NE},
    {"&", OP_BITWISE_AND},
    {"|", OP_BITWISE_OR},
    {"^", OP_BITWISE_XOR},
    {"&&", OP_LOGICAL_AND},
    {"||", OP_LOGICAL_OR},
    {"=", OP_ASSIGN},
    {",", OP_COMMA},

    /* These are used by lexer and must be ignored by parser, so we put
       them at the end */
    {"-", OP_UNARY_MINUS},
    {"!", OP_UNARY_LOGICAL_NOT},
    {"^", OP_UNARY_BITWISE_NOT},
};

/**********************************************************
 * Memory management control
 *********************************************************/
void *exp_malloc(size_t size)
{
    return heap_caps_malloc_prefer(size, 2, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM, MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL);
}

void *exp_calloc(size_t nitems, size_t size)
{
    return heap_caps_calloc_prefer(nitems, size, 2, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM, MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL);
}

void *exp_realloc(void *ptr, size_t size)
{
    return heap_caps_realloc_prefer(ptr, size, 2, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM, MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL);
}

void exp_free(void *ptr)
{
    heap_caps_free(ptr);
}

/**********************************************************
 * Expression library implementation
 *********************************************************/
/**
 * FNV-1a hash
 *
 * FNV prime - 16777619
 * FNV offset - 2166136261
 **/
uint32_t expr_generate_hash_key(const char *const key)
{
    uint32_t hash = 2166136261U;

    for (const char *p = key; *p; p++)
    {
        hash ^= (uint32_t)(unsigned char)(*p);
        hash *= 16777619U;
    }

    return hash;
}

int vec_expand(char **buf, int *length, int *cap, int memsz)
{
    if ((*length + 1) > *cap)
    {
        void *ptr;
        int n = (*cap == 0) ? 1 : *cap << 1;
        ptr = exp_realloc(*buf, n * memsz);
        if (ptr == NULL)
        {
            return -1; /* allocation failed */
        }
        *buf = (char *)ptr;
        *cap = n;
    }
    return 0;
}

static int expr_is_unary(enum expr_type op)
{
    return op == OP_UNARY_MINUS || op == OP_UNARY_LOGICAL_NOT ||
           op == OP_UNARY_BITWISE_NOT;
}

static int expr_is_binary(enum expr_type op)
{
    return !expr_is_unary(op) && op != OP_CONST && op != OP_VAR &&
           op != OP_FUNC && op != OP_UNKNOWN;
}

static int expr_prec(enum expr_type a, enum expr_type b)
{
    int left =
        expr_is_binary(a) && a != OP_ASSIGN && a != OP_POWER && a != OP_COMMA;
    return (left && prec[a] >= prec[b]) || (prec[a] > prec[b]);
}

static enum expr_type expr_op(const char *s, size_t len, int unary)
{
    for (unsigned int i = 0; i < sizeof(OPS) / sizeof(OPS[0]); i++)
    {
        if ((strlen(OPS[i].s) == len) && (strncmp(OPS[i].s, s, len) == 0) &&
            ((unary == -1) || (expr_is_unary(OPS[i].op) == unary)))
        {
            return OPS[i].op;
        }
    }
    return OP_UNKNOWN;
}

static bool expr_parse_number(const char *s, size_t len, int32_t *const number)
{
    int32_t num = 0;
    unsigned int frac = 0;
    unsigned int digits = 0;
    for (unsigned int i = 0; i < len; i++)
    {
        if ((s[i] == '.') && (frac == 0))
        {
            LOG(E, "Floating-point numbers are not supported[%s]!", s);
            return false;
        }
        if (isdigit(s[i]))
        {
            digits++;
            if (frac > 0)
            {
                frac++;
            }
            num = num * 10 + (s[i] - '0');
        }
        else
        {
            return false;
        }
    }

    *number = num;

    return true;
}

static const struct expr_func *expr_func(const struct expr_func *const funcs, const char *s,
                                         size_t len)
{
    for (const struct expr_func *f = funcs; f->name; f++)
    {
        if ((strlen(f->name) == len) && (strncmp(f->name, s, len) == 0))
        {
            return f;
        }
    }
    return NULL;
}

int expr_next_token(const char *s, size_t len, int *flags)
{
    unsigned int i = 0;
    if (len == 0)
    {
        return 0;
    }
    char c = s[0];
    if (c == '#')
    {
        for (; i < len && s[i] != '\n'; i++)
            ;
        return i;
    }
    else if (c == '\n')
    {
        for (; i < len && isspace(s[i]); i++)
            ;
        if (*flags & EXPR_TOP)
        {
            if (i == len || s[i] == ')')
            {
                *flags = *flags & (~EXPR_COMMA);
            }
            else
            {
                *flags = EXPR_TNUMBER | EXPR_TWORD | EXPR_TOPEN | EXPR_COMMA;
            }
        }
        return i;
    }
    else if (isspace(c))
    {
        while (i < len && isspace(s[i]) && s[i] != '\n')
        {
            i++;
        }
        return i;
    }
    else if (isdigit(c))
    {
        if ((*flags & EXPR_TNUMBER) == 0)
        {
            return -1;  // unexpected number
        }
        *flags = EXPR_TOP | EXPR_TCLOSE;
        while ((c == '.' || isdigit(c)) && i < len)
        {
            i++;
            c = s[i];
        }
        return i;
    }
    else if (isfirstvarchr(c))
    {
        if ((*flags & EXPR_TWORD) == 0)
        {
            return -2;  // unexpected word
        }
        *flags = EXPR_TOP | EXPR_TOPEN | EXPR_TCLOSE;
        /* Handling curly braces in struct variable names
         * This happens when parsing identifiers/variables like:
         * - class{inst}param
         * Here { and } are treated as part of the variable name
         */
        int brace_count = 0;                     // Positive when braces are opened, should be 0 at end
        unsigned int last_open_pos = UINT_MAX;   // Position of last '{' character
        unsigned int last_close_pos = UINT_MAX;  // Position of last '}' character
        int has_content_in_braces = 0;           // Flag to track if there's content between braces

        while (i < len)
        {
            if (c == '{')
            {
                brace_count++;  // Found opening brace, increase nesting level
                last_open_pos = i;
                has_content_in_braces = 0;  // Reset content flag for the new brace
            }
            else if (c == '}')
            {
                // Check if we have empty braces (nothing between { and })
                if (!has_content_in_braces || (i == last_open_pos + 1))
                {
                    LOG(E, "Empty braces not allowed!");
                    return -7;
                }
                brace_count--;  // Found closing brace, decrease nesting level
                last_close_pos = i;
                if (brace_count < 0)
                {
                    break;  // More closing than opening braces - invalid
                }
            }
            else if (brace_count > 0)
            {
                // We're inside braces - validate character
                if (c == ',')
                {
                    LOG(E, "Commas not allowed inside braces!");
                    return -8;
                }
                if (!isfirstvarchr(c) && !isdigit(c) && (c != '$'))
                {
                    LOG(E, "Only variable characters allowed in struct parameters!");
                    return -9;
                }
                // Check for the first character after opening brace
                if ((i == last_open_pos + 1) && (isdigit(c) || (c == '$')))
                {
                    LOG(E, "First character inside braces can't be a digit or $");
                    return -11;
                }
                has_content_in_braces = 1;  // Mark that we have valid content in braces
            }

            if (!isvarchr(c))
            {
                break;  // Stop when we hit any non-variable character
            }
            i++;
            if (i < len)
            {
                c = s[i];
            }
        }

        // Ensure all opened braces were properly closed
        if (brace_count != 0)
        {
            LOG(E, "Unmatched braces (either unclosed '{' or extra '}')");
            return -6;
        }

        // If we have braces, ensure there's at least one valid variable character after them
        if ((last_close_pos != UINT_MAX) && (last_close_pos == i - 1))
        {
            LOG(E, "Variable name must have at least one character after braces");
            return -10;
        }

        return i;
    }
    else if ((c == '(') || (c == ')'))
    {
        if ((c == '(') && ((*flags & EXPR_TOPEN) != 0))
        {
            *flags = EXPR_TNUMBER | EXPR_TWORD | EXPR_TOPEN | EXPR_TCLOSE;
        }
        else if ((c == ')') && ((*flags & EXPR_TCLOSE) != 0))
        {
            *flags = EXPR_TOP | EXPR_TCLOSE;
        }
        else
        {
            return -3;  // unexpected parenthesis
        }
        return 1;
    }
    else
    {
        if ((*flags & EXPR_TOP) == 0)
        {
            if (expr_op(&c, 1, 1) == OP_UNKNOWN)
            {
                return -4;  // missing expected operand
            }
            *flags = EXPR_TNUMBER | EXPR_TWORD | EXPR_TOPEN | EXPR_UNARY;
            return 1;
        }
        else
        {
            int found = 0;
            while (!isvarchr(c) && !isspace(c) && (c != '(') && (c != ')') && (i < len))
            {
                if (expr_op(s, i + 1, 0) != OP_UNKNOWN)
                {
                    found = 1;
                }
                else if (found)
                {
                    break;
                }
                i++;
                c = s[i];
            }
            if (!found)
            {
                return -5;  // unknown operator
            }
            *flags = EXPR_TNUMBER | EXPR_TWORD | EXPR_TOPEN;
            return i;
        }
    }
}

static int expr_bind(const char *s, size_t len, vec_expr_t *es)
{
    enum expr_type op = expr_op(s, len, -1);
    if (op == OP_UNKNOWN)
    {
        return -1;
    }

    if (expr_is_unary(op))
    {
        if (vec_len(es) < 1)
        {
            return -1;
        }
        struct expr arg = vec_pop(es);
        struct expr unary = expr_init();
        unary.type = op;
        vec_push(&unary.param.op.args, arg);
        vec_push(es, unary);
    }
    else
    {
        if (vec_len(es) < 2)
        {
            return -1;
        }
        struct expr b = vec_pop(es);
        struct expr a = vec_pop(es);
        struct expr binary = expr_init();
        binary.type = op;
        if ((op == OP_ASSIGN) && (a.type != OP_VAR))
        {
            return -1; /* Bad assignment */
        }
        vec_push(&binary.param.op.args, a);
        vec_push(&binary.param.op.args, b);
        vec_push(es, binary);
    }
    return 0;
}

static struct expr expr_const(int32_t value)
{
    struct expr e = expr_init();
    e.type = OP_CONST;
    e.param.num.value = value;
    return e;
}

static struct expr expr_varref(struct expr_var *v)
{
    struct expr e = expr_init();
    e.type = OP_VAR;
    e.param.var.value = &v->value;
    e.param.var.updated = &v->updated;
    e.param.var.name = v->name;
    e.param.var.id = &v->id;
    return e;
}

static struct expr expr_binary(enum expr_type type, struct expr a,
                               struct expr b)
{
    struct expr e = expr_init();
    e.type = type;
    vec_push(&e.param.op.args, a);
    vec_push(&e.param.op.args, b);
    return e;
}

static inline void expr_copy(struct expr *dst, struct expr *src)
{
    int i;
    struct expr arg;
    dst->type = src->type;
    if (src->type == OP_FUNC)
    {
        dst->param.func.f = src->param.func.f;
        vec_foreach(&src->param.func.args, arg, i)
        {
            struct expr tmp = expr_init();
            expr_copy(&tmp, &arg);
            vec_push(&dst->param.func.args, tmp);
        }
        if (src->param.func.f->ctxsz > 0)
        {
            dst->param.func.context = exp_calloc(1, src->param.func.f->ctxsz);
        }
        dst->param.func.expr_id = src->param.func.expr_id;
    }
    else if (src->type == OP_CONST)
    {
        dst->param.num.value = src->param.num.value;
    }
    else if (src->type == OP_VAR)
    {
        dst->param.var.value = src->param.var.value;
        dst->param.var.name = src->param.var.name;
        dst->param.var.updated = src->param.var.updated;
        dst->param.var.id = src->param.var.id;
    }
    else
    {
        vec_foreach(&src->param.op.args, arg, i)
        {
            struct expr tmp = expr_init();
            expr_copy(&tmp, &arg);
            vec_push(&dst->param.op.args, tmp);
        }
    }
}

static void expr_destroy_args(struct expr *e)
{
    int i;
    struct expr arg;
    if (e->type == OP_FUNC)
    {
        vec_foreach(&e->param.func.args, arg, i) { expr_destroy_args(&arg); }
        vec_free(&e->param.func.args);
        if (e->param.func.context != NULL)
        {
            if (e->param.func.f->cleanup != NULL)
            {
                e->param.func.f->cleanup(e->param.func.f, e->param.func.context);
            }
            exp_free(e->param.func.context);
        }
    }
    else if ((e->type != OP_CONST) && (e->type != OP_VAR))
    {
        vec_foreach(&e->param.op.args, arg, i) { expr_destroy_args(&arg); }
        vec_free(&e->param.op.args);
    }
}

struct expr *expr_create(const char *s, size_t len,
                         struct expr_var_list *vars,
                         const struct expr_func *const funcs,
                         uint32_t expr_id)
{
    int32_t num;
    struct expr_var *v;
    const char *id = NULL;
    size_t idn = 0;

    struct expr *result = NULL;

    vec_expr_t es = vec_init();
    vec_str_t os = vec_init();
    vec_arg_t as = vec_init();

    struct macro
    {
        char *name;
        vec_expr_t body;
    };
    vec(struct macro) macros = vec_init();

    int flags = EXPR_TDEFAULT;
    int paren = EXPR_PAREN_ALLOWED;
    for (;;)
    {
        int n = expr_next_token(s, len, &flags);
        if (n == 0)
        {
            break;
        }
        else if (n < 0)
        {
            goto cleanup;
        }
        const char *tok = s;
        s = s + n;
        len = len - n;
        if (*tok == '#')
        {
            continue;
        }
        if (flags & EXPR_UNARY)
        {
            if (n == 1)
            {
                switch (*tok)
                {
                case '-':
                    tok = "-u";
                    break;
                case '^':
                    tok = "^u";
                    break;
                case '!':
                    tok = "!u";
                    break;
                default:
                    goto cleanup;
                }
                n = 2;
            }
        }
        if ((*tok == '\n') && ((flags & EXPR_COMMA)))
        {
            flags = flags & (~EXPR_COMMA);
            n = 1;
            tok = ",";
        }
        if (isspace(*tok))
        {
            continue;
        }
        int paren_next = EXPR_PAREN_ALLOWED;

        if (idn > 0)
        {
            if ((n == 1) && (*tok == '('))
            {
                int i;
                int has_macro = 0;
                struct macro m;
                vec_foreach(&macros, m, i)
                {
                    if ((strlen(m.name) == idn) && (strncmp(m.name, id, idn) == 0))
                    {
                        has_macro = 1;
                        break;
                    }
                }
                if ((idn == 1 && id[0] == '$') || has_macro ||
                    expr_func(funcs, id, idn) != NULL)
                {
                    struct expr_string str = {id, (int)idn};
                    vec_push(&os, str);
                    paren = EXPR_PAREN_EXPECTED;
                }
                else
                {
                    goto cleanup; /* invalid function name */
                }
            }
            else if ((v = expr_var(vars, id, idn)) != NULL)
            {
                vec_push(&es, expr_varref(v));
                paren = EXPR_PAREN_FORBIDDEN;
            }
            id = NULL;
            idn = 0;
        }

        if ((n == 1) && (*tok == '('))
        {
            if (paren == EXPR_PAREN_EXPECTED)
            {
                struct expr_string str = {"{", 1};
                vec_push(&os, str);
                struct expr_arg arg = {vec_len(&os), vec_len(&es), vec_init()};
                vec_push(&as, arg);
            }
            else if (paren == EXPR_PAREN_ALLOWED)
            {
                struct expr_string str = {"(", 1};
                vec_push(&os, str);
            }
            else
            {
                goto cleanup;  // Bad call
            }
        }
        else if (paren == EXPR_PAREN_EXPECTED)
        {
            goto cleanup;  // Bad call
        }
        else if ((n == 1) && (*tok == ')'))
        {
            int minlen = (vec_len(&as) > 0 ? vec_peek(&as).oslen : 0);
            while (vec_len(&os) > minlen && *vec_peek(&os).s != '(' &&
                   *vec_peek(&os).s != '{')
            {
                struct expr_string str = vec_pop(&os);
                if (expr_bind(str.s, str.n, &es) == -1)
                {
                    goto cleanup;
                }
            }
            if (vec_len(&os) == 0)
            {
                goto cleanup;  // Bad parens
            }
            struct expr_string str = vec_pop(&os);
            if ((str.n == 1) && (*str.s == '{'))
            {
                str = vec_pop(&os);
                struct expr_arg arg = vec_pop(&as);
                if (vec_len(&es) > arg.eslen)
                {
                    vec_push(&arg.args, vec_pop(&es));
                }
                if ((str.n == 1) && (str.s[0] == '$'))
                {
                    if (vec_len(&arg.args) < 1)
                    {
                        vec_free(&arg.args);
                        goto cleanup; /* too few arguments for $() function */
                    }
                    struct expr *u = &vec_nth(&arg.args, 0);
                    if (u->type != OP_VAR)
                    {
                        vec_free(&arg.args);
                        goto cleanup; /* first argument is not a variable */
                    }
                    for (struct expr_var *v = vars->head; v; v = v->next)
                    {
                        if (&v->value == u->param.var.value)
                        {
                            struct macro m = {v->name, arg.args};
                            vec_push(&macros, m);
                            break;
                        }
                    }
                    vec_push(&es, expr_const(0));
                }
                else
                {
                    int i = 0;
                    int found = -1;
                    struct macro m;
                    vec_foreach(&macros, m, i)
                    {
                        if ((strlen(m.name) == (size_t)str.n) &&
                            (strncmp(m.name, str.s, str.n) == 0))
                        {
                            found = i;
                        }
                    }
                    if (found != -1)
                    {
                        m = vec_nth(&macros, found);
                        struct expr root = expr_const(0);
                        struct expr *p = &root;
                        /* Assign macro parameters */
                        for (int j = 0; j < vec_len(&arg.args); j++)
                        {
                            char varname[4];
                            snprintf(varname, sizeof(varname) - 1, "$%d", (j + 1));
                            struct expr_var *v = expr_var(vars, varname, strlen(varname));
                            struct expr ev = expr_varref(v);
                            struct expr assign =
                                expr_binary(OP_ASSIGN, ev, vec_nth(&arg.args, j));
                            *p = expr_binary(OP_COMMA, assign, expr_const(0));
                            p = &vec_nth(&p->param.op.args, 1);
                        }
                        /* Expand macro body */
                        for (int j = 1; j < vec_len(&m.body); j++)
                        {
                            if (j < (vec_len(&m.body) - 1))
                            {
                                *p = expr_binary(OP_COMMA, expr_const(0), expr_const(0));
                                expr_copy(&vec_nth(&p->param.op.args, 0), &vec_nth(&m.body, j));
                            }
                            else
                            {
                                expr_copy(p, &vec_nth(&m.body, j));
                            }
                            p = &vec_nth(&p->param.op.args, 1);
                        }
                        vec_push(&es, root);
                        vec_free(&arg.args);
                    }
                    else
                    {
                        const struct expr_func *f = expr_func(funcs, str.s, str.n);
                        struct expr bound_func = expr_init();
                        bound_func.type = OP_FUNC;
                        bound_func.param.func.f = f;
                        bound_func.param.func.args = arg.args;
                        bound_func.param.func.expr_id = expr_id;
                        if (f->ctxsz > 0)
                        {
                            void *p = exp_calloc(1, f->ctxsz);
                            if (p == NULL)
                            {
                                goto cleanup; /* allocation failed */
                            }
                            bound_func.param.func.context = p;
                        }
                        vec_push(&es, bound_func);
                    }
                }
            }
            paren_next = EXPR_PAREN_FORBIDDEN;
        }
        else if (expr_parse_number(tok, n, &num) == true)
        {
            vec_push(&es, expr_const(num));
            paren_next = EXPR_PAREN_FORBIDDEN;
        }
        else if (expr_op(tok, n, -1) != OP_UNKNOWN)
        {
            enum expr_type op = expr_op(tok, n, -1);
            struct expr_string o2 = {NULL, 0};
            if (vec_len(&os) > 0)
            {
                o2 = vec_peek(&os);
            }
            for (;;)
            {
                if ((n == 1) && (*tok == ',') && (vec_len(&os) > 0))
                {
                    struct expr_string str = vec_peek(&os);
                    if (str.n == 1 && *str.s == '{')
                    {
                        struct expr e = vec_pop(&es);
                        vec_push(&vec_peek(&as).args, e);
                        break;
                    }
                }
                enum expr_type type2 = expr_op(o2.s, o2.n, -1);
                if (!((type2 != OP_UNKNOWN) && expr_prec(op, type2)))
                {
                    struct expr_string str = {tok, n};
                    vec_push(&os, str);
                    break;
                }

                if (expr_bind(o2.s, o2.n, &es) == -1)
                {
                    goto cleanup;
                }
                (void)vec_pop(&os);
                if (vec_len(&os) > 0)
                {
                    o2 = vec_peek(&os);
                }
                else
                {
                    o2.n = 0;
                }
            }
        }
        else
        {
            if ((n > 0) && !isdigit(*tok))
            {
                /* Valid identifier, a variable or a function */
                id = tok;
                idn = n;
            }
            else
            {
                goto cleanup;  // Bad variable name, e.g. '2.3.4' or '4ever'
            }
        }
        paren = paren_next;
    }

    if (idn > 0)
    {
        vec_push(&es, expr_varref(expr_var(vars, id, idn)));
    }

    while (vec_len(&os) > 0)
    {
        struct expr_string rest = vec_pop(&os);
        if ((rest.n == 1) && (*rest.s == '(' || *rest.s == ')'))
        {
            goto cleanup;  // Bad paren
        }
        if (expr_bind(rest.s, rest.n, &es) == -1)
        {
            goto cleanup;
        }
    }

    result = (struct expr *)exp_calloc(1, sizeof(struct expr));
    if (result != NULL)
    {
        if (vec_len(&es) == 0)
        {
            result->type = OP_CONST;
        }
        else
        {
            *result = vec_pop(&es);
        }
    }

    int i, j;
    struct macro m;
    struct expr e;
    struct expr_arg a;
cleanup:
    vec_foreach(&macros, m, i)
    {
        struct expr e;
        vec_foreach(&m.body, e, j) { expr_destroy_args(&e); }
        vec_free(&m.body);
    }
    vec_free(&macros);

    vec_foreach(&es, e, i) { expr_destroy_args(&e); }
    vec_free(&es);

    vec_foreach(&as, a, i)
    {
        vec_foreach(&a.args, e, j) { expr_destroy_args(&e); }
        vec_free(&a.args);
    }
    vec_free(&as);

    /*vec_foreach(&os, o, i) {vec_free(&m.body);}*/
    vec_free(&os);
    return result;
}

int32_t expr_eval(struct expr *e)
{
    int32_t n;
    switch (e->type)
    {
    case OP_UNARY_MINUS:
        return -(expr_eval(&e->param.op.args.buf[0]));
    case OP_UNARY_LOGICAL_NOT:
        return !(expr_eval(&e->param.op.args.buf[0]));
    case OP_UNARY_BITWISE_NOT:
        return ~(expr_eval(&e->param.op.args.buf[0]));
    case OP_POWER:
        return pow(expr_eval(&e->param.op.args.buf[0]),
                   expr_eval(&e->param.op.args.buf[1]));
    case OP_MULTIPLY:
        return expr_eval(&e->param.op.args.buf[0]) *
               expr_eval(&e->param.op.args.buf[1]);
    case OP_DIVIDE:
        n = expr_eval(&e->param.op.args.buf[1]);
        return EXPR_OPERATION_DIVIDE(expr_eval(&e->param.op.args.buf[0]), n);
    case OP_REMAINDER:
        n = expr_eval(&e->param.op.args.buf[1]);
        return EXPR_OPERATION_REMAINDER(expr_eval(&e->param.op.args.buf[0]), n);
    case OP_PLUS:
        return expr_eval(&e->param.op.args.buf[0]) +
               expr_eval(&e->param.op.args.buf[1]);
    case OP_MINUS:
        return expr_eval(&e->param.op.args.buf[0]) -
               expr_eval(&e->param.op.args.buf[1]);
    case OP_SHL:
        return expr_eval(&e->param.op.args.buf[0]) << expr_eval(&e->param.op.args.buf[1]);
    case OP_SHR:
        return expr_eval(&e->param.op.args.buf[0]) >>
               expr_eval(&e->param.op.args.buf[1]);
    case OP_LT:
        return expr_eval(&e->param.op.args.buf[0]) <
               expr_eval(&e->param.op.args.buf[1]);
    case OP_LE:
        return expr_eval(&e->param.op.args.buf[0]) <=
               expr_eval(&e->param.op.args.buf[1]);
    case OP_GT:
        return expr_eval(&e->param.op.args.buf[0]) >
               expr_eval(&e->param.op.args.buf[1]);
    case OP_GE:
        return expr_eval(&e->param.op.args.buf[0]) >=
               expr_eval(&e->param.op.args.buf[1]);
    case OP_EQ:
        return expr_eval(&e->param.op.args.buf[0]) ==
               expr_eval(&e->param.op.args.buf[1]);
    case OP_NE:
        return expr_eval(&e->param.op.args.buf[0]) !=
               expr_eval(&e->param.op.args.buf[1]);
    case OP_BITWISE_AND:
        return expr_eval(&e->param.op.args.buf[0]) &
               expr_eval(&e->param.op.args.buf[1]);
    case OP_BITWISE_OR:
        return expr_eval(&e->param.op.args.buf[0]) |
               expr_eval(&e->param.op.args.buf[1]);
    case OP_BITWISE_XOR:
        return expr_eval(&e->param.op.args.buf[0]) ^
               expr_eval(&e->param.op.args.buf[1]);
    case OP_LOGICAL_AND:
        n = expr_eval(&e->param.op.args.buf[0]);
        if (n != 0)
        {
            n = expr_eval(&e->param.op.args.buf[1]);
            if (n != 0)
            {
                return n;
            }
        }
        return 0;
    case OP_LOGICAL_OR:
        n = expr_eval(&e->param.op.args.buf[0]);
        if (n != 0)
        {
            return n;
        }
        else
        {
            n = expr_eval(&e->param.op.args.buf[1]);
            if (n != 0)
            {
                return n;
            }
        }
        return 0;
    case OP_ASSIGN:
        n = expr_eval(&e->param.op.args.buf[1]);
        if (vec_nth(&e->param.op.args, 0).type == OP_VAR)
        {
            *e->param.op.args.buf[0].param.var.value = n;
            *e->param.op.args.buf[0].param.var.updated = true;
        }
        return n;
    case OP_COMMA:
        expr_eval(&e->param.op.args.buf[0]);
        return expr_eval(&e->param.op.args.buf[1]);
    case OP_CONST:
        return e->param.num.value;
    case OP_VAR:
        return *e->param.var.value;
    case OP_FUNC:
        return e->param.func.f->f(e->param.func.f, &e->param.func.args,
                                  e->param.func.context, e->param.func.expr_id);
    default:
        return 0;
    }
}

void expr_get_node_for_type_from_ast(struct expr *e, enum expr_type type, vec_expr_ptr_t *vector)
{
    // First, check if current node matches the type we're looking for
    if (e->type == type)
    {
        vec_push(vector, e);  // Push pointer to the original node
    }

    // Then recursively traverse the AST
    switch (e->type)
    {
    case OP_FUNC:
    {
        // Recursively search in function arguments
        for (int i = 0; i < e->param.func.args.len; i++)
        {
            expr_get_node_for_type_from_ast(&e->param.func.args.buf[i], type, vector);
        }
        break;
    }
    case OP_UNARY_MINUS:
    case OP_UNARY_LOGICAL_NOT:
    case OP_UNARY_BITWISE_NOT:
        expr_get_node_for_type_from_ast(&e->param.op.args.buf[0], type, vector);
        break;
    case OP_ASSIGN:
        expr_get_node_for_type_from_ast(&e->param.op.args.buf[1], type, vector);
        break;
    case OP_POWER:
    case OP_MULTIPLY:
    case OP_DIVIDE:
    case OP_REMAINDER:
    case OP_PLUS:
    case OP_MINUS:
    case OP_SHL:
    case OP_SHR:
    case OP_LT:
    case OP_LE:
    case OP_GT:
    case OP_GE:
    case OP_EQ:
    case OP_NE:
    case OP_BITWISE_AND:
    case OP_BITWISE_OR:
    case OP_BITWISE_XOR:
    case OP_LOGICAL_AND:
    case OP_LOGICAL_OR:
    case OP_COMMA:
        expr_get_node_for_type_from_ast(&e->param.op.args.buf[0], type, vector);
        expr_get_node_for_type_from_ast(&e->param.op.args.buf[1], type, vector);
        break;
    case OP_CONST:
    case OP_VAR:
    case OP_UNKNOWN:
    default:
        // End of AST node - no children to traverse
        break;
    }
}

struct expr_var *expr_var(struct expr_var_list *vars, const char *s,
                          size_t len)
{
    struct expr_var *v = NULL;
    if ((len == 0) || !isfirstvarchr(*s))
    {
        return NULL;
    }
    for (v = vars->head; v; v = v->next)
    {
        if ((strlen(v->name) == len) && (strncmp(v->name, s, len) == 0))
        {
            return v;
        }
    }
    v = (struct expr_var *)exp_calloc(1, sizeof(struct expr_var) + len + 1);
    if (v == NULL)
    {
        return NULL; /* allocation failed */
    }
    v->next = vars->head;
    v->id = 0;
    v->ddm_create = 0;
    v->value = 0;
    strncpy(v->name, s, len);
    v->name[len] = '\0';
    v->updated = false;
    vars->head = v;
    return v;
}

struct expr_var *expr_get_var_by_name(struct expr_var_list *vars, const char *s,
                                      size_t len)
{
    struct expr_var *v = NULL;
    if ((len == 0) || !isfirstvarchr(*s))
    {
        return NULL;
    }

    for (v = vars->head; v; v = v->next)
    {
        if ((strlen(v->name) == len) && (strncmp(v->name, s, len) == 0))
        {
            return v;
        }
    }

    return v;
}

struct expr_var *expr_get_var_by_id_and_type(struct expr_var_list *vars, uint32_t var_id, uint8_t var_type)
{
    struct expr_var *v = NULL;

    for (v = vars->head; v; v = v->next)
    {
        if ((v->id == var_id) && (v->type == var_type))
        {
            return v;
        }
    }

    return v;
}

struct expr_var *expr_get_var_by_id_and_type_id(struct expr_var_list *vars, uint32_t var_id, uint8_t var_type, uint8_t var_type_id)
{
    struct expr_var *v = NULL;

    for (v = vars->head; v; v = v->next)
    {
        if ((v->id == var_id) && (v->type == var_type) && (v->type_id == var_type_id))
        {
            return v;
        }
    }

    return v;
}

bool expr_var_updated(struct expr_var *var)
{
    bool updated = var->updated;
    var->updated = false;
    return updated;
}

void expr_destroy(struct expr *e, struct expr_var_list *vars)
{
    if (e != NULL)
    {
        expr_destroy_args(e);
        exp_free(e);
    }
    if (vars != NULL)
    {
        for (struct expr_var *v = vars->head; v;)
        {
            struct expr_var *next = v->next;
            exp_free(v);
            v = next;
        }
    }
}
