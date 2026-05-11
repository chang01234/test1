#ifndef RULE_ENGINE_EXP_EVAL_H_
#define RULE_ENGINE_EXP_EVAL_H_

#include "configuration.h"
#include <ctype.h> /* for isspace */
#include <limits.h>
#include <math.h> /* for pow */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Top-level token
#define EXPR_TOP      (1 << 0)
#define EXPR_TOPEN    (1 << 1)  // Opening parenthesis allowed
#define EXPR_TCLOSE   (1 << 2)  // Closing parenthesis allowed
#define EXPR_TNUMBER  (1 << 3)  // Number allowed
#define EXPR_TWORD    (1 << 4)  // Word (variable/function name) allowed
#define EXPR_TDEFAULT (EXPR_TOPEN | EXPR_TNUMBER | EXPR_TWORD)

void *exp_malloc(size_t size);
void *exp_calloc(size_t nitems, size_t size);
void *exp_realloc(void *ptr, size_t size);
void exp_free(void *ptr);

int expr_next_token(const char *s, size_t len, int *flags);

int vec_expand(char **buf, int *length, int *cap, int memsz);

#define vec(T)   \
    struct       \
    {            \
        T *buf;  \
        int len; \
        int cap; \
    }
#define vec_init() \
    {              \
        NULL, 0, 0}
#define vec_len(v) ((v)->len)
#define vec_unpack(v) \
    (char **)&(v)->buf, &(v)->len, &(v)->cap, sizeof(*(v)->buf)
#define vec_push(v, val) \
    vec_expand(vec_unpack(v)) ? -1 : ((v)->buf[(v)->len++] = (val), 0)
#define vec_nth(v, i) (v)->buf[i]
#define vec_peek(v)   (v)->buf[(v)->len - 1]
#define vec_pop(v)    (v)->buf[--(v)->len]
#define vec_free(v)   (exp_free((v)->buf), (v)->buf = NULL, (v)->len = (v)->cap = 0)
#define vec_foreach(v, var, iter)                                              \
    if ((v)->len > 0)                                                          \
        for ((iter) = 0; (iter) < (v)->len && (((var) = (v)->buf[(iter)]), 1); \
             ++(iter))

#define EXPR_EVAL__LOG(level, format, ...) \
    esp_log_write(ESP_LOG_INFO, FILENAME, LOG_FORMAT(level, "%s(%d): " format), esp_log_timestamp(), FILENAME, __func__, __LINE__, ##__VA_ARGS__)

#define RULE_ENGINE__PRINT_VECTOR_VARIABLES(vec_expr)                                 \
    do                                                                                \
    {                                                                                 \
        int i;                                                                        \
        struct expr expression;                                                       \
        vec_foreach(vec_expr, expression, i) RULE_ENGINE__PRINT_VARIABLE(expression); \
    } while (0)  // print all variables in a vector

#define RULE_ENGINE__PRINT_VARIABLE(expression)                                                            \
    do                                                                                                     \
    {                                                                                                      \
        struct expr *param = &expression;                                                                  \
        if (param->type == OP_VAR)                                                                         \
            EXPR_EVAL__LOG(I, " Variable[%s], value[%d]", param->param.var.name, *param->param.var.value); \
    } while (0)  // print variable name and value

enum expr_type
{
    OP_UNKNOWN,
    OP_UNARY_MINUS,
    OP_UNARY_LOGICAL_NOT,
    OP_UNARY_BITWISE_NOT,

    OP_POWER,
    OP_DIVIDE,
    OP_MULTIPLY,
    OP_REMAINDER,

    OP_PLUS,
    OP_MINUS,

    OP_SHL,
    OP_SHR,

    OP_LT,
    OP_LE,
    OP_GT,
    OP_GE,
    OP_EQ,
    OP_NE,

    OP_BITWISE_AND,
    OP_BITWISE_OR,
    OP_BITWISE_XOR,

    OP_LOGICAL_AND,
    OP_LOGICAL_OR,

    OP_ASSIGN,
    OP_COMMA,

    OP_CONST,
    OP_VAR,
    OP_FUNC,
};

struct expr_eval__ops
{
    const char *s;
    const enum expr_type op;
};

#define expr_init() \
    {               \
        .type = (enum expr_type)0}

struct expr_func;

typedef vec(struct expr) vec_expr_t;
/**
 * @brief Vector type for storing pointers to AST expression nodes
 *
 * This type stores pointers to the original expression nodes in the AST,
 * ensuring memory safety when traversing and manipulating the expression tree.
 * Unlike vec_expr_t which stores copies of expression nodes, vec_expr_ptr_t
 * maintains references to the original nodes, preventing issues with invalid
 * pointer access after vector operations.
 *
 * Usage: vec_expr_ptr_t func_vector = vec_init();
 *        Access elements with: vec_nth(&func_vector, i) (returns struct expr*)
 */
typedef vec(struct expr *) vec_expr_ptr_t;

typedef void (*exprfn_cleanup_t)(const struct expr_func *const f, void *context);
typedef int32_t (*exprfn_t)(const struct expr_func *const f, vec_expr_t *args, void *context, uint32_t expr_id);

struct expr_string
{
    const char *s;
    int n;
};

struct expr_arg
{
    int oslen;
    int eslen;
    vec_expr_t args;
};

/*
 * Functions
 */
struct expr_func
{
    const char *name;
    exprfn_t f;
    exprfn_cleanup_t cleanup;
    size_t ctxsz;
    uint32_t expr_id;
};

/*
 * Variables
 */
struct expr_var
{
    struct expr_var *next;
    int32_t value;

    uint32_t id;
    uint8_t type : 3; /*!< Variable type determines how ID is established:
                       *   - RULE_ENGINE__VAR_TYPE_DDM2: ID comes from ddm2_parameter_list
                       *   - RULE_ENGINE__VAR_TYPE_GLOBAL/LOCAL: ID generated via expr_generate_hash_key()
                       *   - RULE_ENGINE__VAR_TYPE_DDM2_DYN: ID from ddm2_parameter_list, with instance
                       *     context using RULE_ENGINE__VAR_TYPE_DDM2_INSTNACE type
                       *   - RULE_ENGINE__VAR_TYPE_DDM2_INST: ID set from class instance
                       */
    uint8_t type_id : 4;
    uint8_t type_is_resolved : 1;

    uint8_t ddm_create : 2;
    uint8_t updated;

    char name[];
};

struct expr
{
    enum expr_type type;
    union
    {
        struct
        {
            int32_t value;
        } num;
        struct
        {
            int32_t *value;
            uint8_t *updated;
            uint32_t *id;
            char *name;
        } var;
        struct
        {
            vec_expr_t args;
        } op;
        struct
        {
            const struct expr_func *f;
            vec_expr_t args;
            void *context;
            uint32_t expr_id;
        } func;
    } param;
};

typedef vec(struct expr_string) vec_str_t;
typedef vec(struct expr_arg) vec_arg_t;

struct expr_var_list
{
    struct expr_var *head;
};

uint32_t expr_generate_hash_key(const char *const key);
struct expr *expr_create(const char *s, size_t len, struct expr_var_list *vars, const struct expr_func *const funcs, uint32_t expr_id);
int32_t expr_eval(struct expr *e);

struct expr_var *expr_get_var_by_id_and_type(struct expr_var_list *vars, uint32_t var_id, uint8_t type);
struct expr_var *expr_get_var_by_id_and_type_id(struct expr_var_list *vars, uint32_t var_id, uint8_t var_type, uint8_t var_type_id);
struct expr_var *expr_get_var_by_name(struct expr_var_list *vars, const char *s, size_t len);
struct expr_var *expr_var(struct expr_var_list *vars, const char *s, size_t len);
bool expr_var_updated(struct expr_var *var);

/**
 * @brief Traverses AST and collects pointers to nodes of specified type
 *
 * This function recursively traverses the Abstract Syntax Tree (AST) starting
 * from the given expression node and collects pointers to all nodes that match
 * the specified expression type. The collected pointers reference the original
 * AST nodes, ensuring memory safety and allowing direct manipulation of the
 * expression tree.
 *
 * @param e The root expression node to start traversal from
 * @param type The expression type to search for (e.g., OP_FUNC)
 * @param vector Pointer to a vec_expr_ptr_t that will store the found node pointers
 */
void expr_get_node_for_type_from_ast(struct expr *e, enum expr_type type, vec_expr_ptr_t *vector);

static inline void expr_set_var(struct expr_var *const var, int32_t value)
{
    ((var == NULL) ? EXPR_EVAL__LOG(E, "var == NULL") : (void)(var->value = value));
}

static inline void expr_set_var_id(struct expr_var *const var, uint32_t id)
{
    ((var == NULL) ? EXPR_EVAL__LOG(E, "var == NULL") : (void)(var->id = id));
}

static inline void expr_set_var_type(struct expr_var *const var, uint8_t type)
{
    ((var == NULL) ? EXPR_EVAL__LOG(E, "var == NULL") : (void)(var->type = type));
}

static inline void expr_set_var_type_id(struct expr_var *const var, uint8_t type_id)
{
    ((var == NULL) ? EXPR_EVAL__LOG(E, "var == NULL") : (void)(var->type_id = type_id));
}

static inline void expr_set_var_type_is_resolved(struct expr_var *const var, uint8_t type_is_resolved)
{
    ((var == NULL) ? EXPR_EVAL__LOG(E, "var == NULL") : (void)(var->type_is_resolved = type_is_resolved));
}

static inline void expr_set_var_type_ddm_create(struct expr_var *const var, uint8_t ddm_create)
{
    ((var == NULL) ? EXPR_EVAL__LOG(E, "var == NULL") : (void)(var->ddm_create = ddm_create));
}

static inline void expr_set_var_updated(struct expr_var *const var, uint8_t updated)
{
    ((var == NULL) ? EXPR_EVAL__LOG(E, "var == NULL") : (void)(var->updated = updated));
}

static inline int32_t expr_var_value(const struct expr_var *const var)
{
    return var->value;
}

static inline uint32_t expr_var_id(const struct expr_var *const var)
{
    return var->id;
}

static inline uint8_t expr_var_type(const struct expr_var *const var)
{
    return var->type;
}

static inline uint8_t expr_var_type_id(const struct expr_var *const var)
{
    return var->type_id;
}

static inline uint8_t expr_var_type_is_resolved(const struct expr_var *const var)
{
    return var->type_is_resolved;
}

static inline uint8_t expr_var_type_ddm_create(const struct expr_var *const var)
{
    return var->ddm_create;
}

void expr_destroy(struct expr *e, struct expr_var_list *vars);

#endif /* RULE_ENGINE_EXP_EVAL_H_ */
