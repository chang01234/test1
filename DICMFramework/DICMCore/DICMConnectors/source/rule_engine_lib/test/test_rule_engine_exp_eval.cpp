extern "C" {
#include "rule_engine_exp_eval.h"
}
#include "DICMFrameworkTestFixture.hpp"

/* User functions definition for testing expr_eval function implementation */
struct nop_context
{
    void *p;
};

static void user_func_nop_cleanup(const struct expr_func *const f, void *c)
{
    (void)f;
    struct nop_context *nop = (struct nop_context *)c;
    hal_mem_free(nop->p);
}

static int32_t user_func_nop(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id)
{
    (void)args;
    struct nop_context *nop = (struct nop_context *)c;
    if (f->ctxsz == 0)
    {
        hal_mem_free(nop->p);
        return 0;
    }
    if (nop->p == NULL)
    {
        nop->p = hal_mem_malloc_prefer(10000, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    }
    return 0;
}

static int32_t user_func_add(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id)
{
    (void)f, (void)c;
    int32_t a = expr_eval(&vec_nth(args, 0));
    int32_t b = expr_eval(&vec_nth(args, 1));
    return a + b;
}

static int32_t user_func_next(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id)
{
    (void)f, (void)c;
    int32_t a = expr_eval(&vec_nth(args, 0));
    return a + 1;
}

/* User functions declaration for testing expr_eval function implmentation */
static struct expr_func user_funcs[] = {
    {"nop", user_func_nop, user_func_nop_cleanup, sizeof(struct nop_context), 0},
    {"add", user_func_add, NULL, 0, 0},
    {"next", user_func_next, NULL, 0, 0},
    {NULL, NULL, NULL, 0, 0},
};

/* Helper function used to test variable creation. It checks whether the specified
 *  "variables" are correctly created during the creation process.
 */
static void expr_eval_expected_vars(const char *expr_str, const std::vector<std::string> &variables)
{
    struct expr_var_list vars = {0};
    struct expr *expr = expr_create(expr_str, strlen(expr_str), &vars, user_funcs, 0);
    EXPECT_NE(expr, nullptr) << "'expr_created' failed for rule: " << expr_str;
    if (expr == nullptr)
    {
        return;
    }

    // Calculate the number of parsed variables
    int expr_var_numb = 0;
    for (struct expr_var *v = vars.head; v; v = v->next)
    {
        expr_var_numb++;
    }
    EXPECT_EQ(variables.size(), expr_var_numb) << "rule: " << expr_str;

    // expr_eval parser stores them in it's vector in revers order, start from the end
    std::vector<std::string>::const_reverse_iterator var_it = variables.rbegin();
    for (struct expr_var *v = vars.head; v; v = v->next, ++var_it)
    {
        EXPECT_STREQ(var_it->c_str(), v->name) << "rule: " << expr_str;
    }

    // Double check, trough expr_get_var_by_name
    for (const std::string &var : variables)
    {
        struct expr_var *expr_var_name = expr_get_var_by_name(&vars, var.c_str(), var.length());
        EXPECT_NE(expr_var_name, nullptr) << "rule: " << expr_str;
        if (expr_var_name)
        {
            EXPECT_STREQ(var.c_str(), expr_var_name->name) << "rule: " << expr_str;
        }
    }

    expr_destroy(expr, &vars);
}

/* Helper function used to test failure cases when creating
 * or destroying expressions and their associated variables.
 */
static void expr_eval_should_fail(const char *s)
{
    struct expr_var_list vars = {0};
    struct expr *e = expr_create(s, strlen(s), &vars, user_funcs, 0);
    EXPECT_EQ(e, nullptr) << "'expr_created' should return error for rule: " << s;
    expr_destroy(e, &vars);
}

/* Helper function used to test successful creation and destruction
 * when creating  or destroying expressions and their associated variables.
 */
static void expr_eval_should_pass(const char *s, int32_t expected)
{
    struct expr_var_list vars = {0};
    struct expr *e = expr_create(s, strlen(s), &vars, user_funcs, 0);
    EXPECT_NE(e, nullptr) << "'expr_created' failed for rule: " << s;
    if (e == nullptr)
    {
        return;
    }

    int32_t result = expr_eval(e);
    EXPECT_EQ(result, expected) << "'expr_eval' failed for rule: " << s;

    expr_destroy(e, &vars);
}

/* Helper function to tokenize a string - Lexer */
static std::vector<std::string> expr_eval_tokenize(const char *s)
{
    std::vector<std::string> tokens;
    int len = strlen(s);
    int flags = EXPR_TDEFAULT;

    while (len > 0)
    {
        int n = expr_next_token(s, len, &flags);
        if (n <= 0)
        {
            break;
        }

        tokens.push_back(std::string(s, n));
        s += n;
        len -= n;
    }

    return tokens;
}

/* Const Tests */
TEST(RuleEngineExprEvalConst, rule_engine_expr_eval_int)
{
    expr_eval_should_pass("1", 1);
    expr_eval_should_pass(" 1 ", 1);
    expr_eval_should_pass("12", 12);
}

TEST(RuleEngineExprEvalConst, rule_engine_expr_eval_float)
{
    // floating-point expressions are not supported
    expr_eval_should_fail("12.3");
}

/* Vector Tests */
TEST(RuleEngineExprEvalVector, vector_int)
{
    vec(int) ints = vec_init();
    ASSERT_EQ(vec_push(&ints, 3), 0);
    EXPECT_EQ(vec_len(&ints), 1);
    EXPECT_EQ(vec_peek(&ints), 3);
    EXPECT_EQ(vec_pop(&ints), 3);
    EXPECT_EQ(vec_len(&ints), 0);
    vec_free(&ints);
}

TEST(RuleEngineExprEvalVector, vector_string)
{
    vec(const char *) strings = vec_init();
    EXPECT_EQ(vec_push(&strings, (const char *)"hello"), 0);
    EXPECT_EQ(vec_push(&strings, (const char *)"world"), 0);
    EXPECT_EQ(vec_push(&strings, (const char *)"foo"), 0);
    EXPECT_EQ(vec_len(&strings), 3);
    if (strings.buf)
    {
        vec_free(&strings);
    }
}

/* Lexer Tests */
TEST(RuleEngineExprEvalLexer, empty_string)
{
    std::vector<std::string> tokens = expr_eval_tokenize("");
    EXPECT_TRUE(tokens.empty());
}

TEST(RuleEngineExprEvalLexer, single_number)
{
    std::vector<std::string> tokens = expr_eval_tokenize("1");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0], "1");
}

TEST(RuleEngineExprEvalLexer, simple_addition)
{
    std::vector<std::string> tokens = expr_eval_tokenize("1+11");
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "1");
    EXPECT_EQ(tokens[1], "+");
    EXPECT_EQ(tokens[2], "11");
}

TEST(RuleEngineExprEvalLexer, simple_multiplication)
{
    std::vector<std::string> tokens = expr_eval_tokenize("1*11");
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "1");
    EXPECT_EQ(tokens[1], "*");
    EXPECT_EQ(tokens[2], "11");
}

TEST(RuleEngineExprEvalLexer, power_operation)
{
    std::vector<std::string> tokens = expr_eval_tokenize("1**11");
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "1");
    EXPECT_EQ(tokens[1], "**");
    EXPECT_EQ(tokens[2], "11");
}

TEST(RuleEngineExprEvalLexer, power_operation_with_negative)
{
    std::vector<std::string> tokens = expr_eval_tokenize("1**-11");
    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0], "1");
    EXPECT_EQ(tokens[1], "**");
    EXPECT_EQ(tokens[2], "-");
    EXPECT_EQ(tokens[3], "11");
}

/* Empty expression Tests */
TEST(RuleEngineExprEvalEmpty, empty_expression)
{
    expr_eval_should_pass("", 0);
    expr_eval_should_pass("  ", 0);
    expr_eval_should_pass("  \t \n ", 0);
}

/* Unary expression Tests */
TEST(RuleEngineExprEvalUnary, unary_expression)
{
    expr_eval_should_pass("-1", -1);
    expr_eval_should_pass("--1", -(-1));
    expr_eval_should_pass("!0 ", !0);
    expr_eval_should_pass("!2 ", !2);
    expr_eval_should_pass("^3", ~3);
}

/* Binary expression Tests */
TEST(RuleEngineExprEvalBinary, binary_expression)
{
    expr_eval_should_pass("1+2", 1 + 2);
    expr_eval_should_pass("10-2", 10 - 2);
    expr_eval_should_pass("2*3", 2 * 3);
    expr_eval_should_pass("2+3*4", 2 + 3 * 4);
    expr_eval_should_pass("2*3+4", 2 * 3 + 4);
    expr_eval_should_pass("2+3/2", 2 + 3 / 2);
    expr_eval_should_pass("1/3*6/4*2", 1 / 3 * 6 / 4 * 2);
    expr_eval_should_pass("1*3/6*4/2", 1 * 3 / 6 * 4 / 2);
    expr_eval_should_pass("6/2+8*4/2", 19);
    expr_eval_should_pass("3/2", 3 / 2);
    expr_eval_should_pass("(3/2)|0", 3 / 2);
    expr_eval_should_pass("(3/0)", 0);
    expr_eval_should_pass("(3/0)|0", 0);
    expr_eval_should_pass("(3%0)", 0);
    expr_eval_should_pass("(3%0)|0", 0);
    expr_eval_should_pass("2**3", 8);
    expr_eval_should_pass("9**(1/2)", pow(9, 1 / 2));
    expr_eval_should_pass("1+2<<3", (1 + 2) << 3);
    expr_eval_should_pass("2<<3", 2 << 3);
    expr_eval_should_pass("12>>2", 12 >> 2);
    expr_eval_should_pass("1<2", 1 < 2);
    expr_eval_should_pass("2<2", 2 < 2);
    expr_eval_should_pass("3<2", 3 < 2);
    expr_eval_should_pass("1>2", 1 > 2);
    expr_eval_should_pass("2>2", 2 > 2);
    expr_eval_should_pass("3>2", 3 > 2);
    expr_eval_should_pass("1==2", 1 == 2);
    expr_eval_should_pass("2==2", 2 == 2);
    expr_eval_should_pass("3==2", 3 == 2);
    expr_eval_should_pass("1<=2", 1 <= 2);
    expr_eval_should_pass("2<=2", 2 <= 2);
    expr_eval_should_pass("3<=2", 3 <= 2);
    expr_eval_should_pass("1>=2", 1 >= 2);
    expr_eval_should_pass("2>=2", 2 >= 2);
    expr_eval_should_pass("3>=2", 3 >= 2);
    expr_eval_should_pass("123&42", 123 & 42);
    expr_eval_should_pass("123^42", 123 ^ 42);

    expr_eval_should_pass("1-1+1+1", 1 - 1 + 1 + 1);
    expr_eval_should_pass("2**2**3", 256); /* 2^(2^3), not (2^2)^3 */
}

/* Logical expression Tests */
TEST(RuleEngineExprEvalLogical, logical_expression)
{
    expr_eval_should_pass("2&&3", 3);
    expr_eval_should_pass("0&&3", 0);
    expr_eval_should_pass("3&&0", 0);
    expr_eval_should_pass("2||3", 2);
    expr_eval_should_pass("0||3", 3);
    expr_eval_should_pass("2||0", 2);
    expr_eval_should_pass("0||0", 0);

    expr_eval_should_pass("1&&(3%0)", 0);
    expr_eval_should_pass("(3%0)&&1", 0);
    expr_eval_should_pass("1||(3%0)", 1);
    expr_eval_should_pass("(3%0)||1", 1);
}

/* Parens expression Tests */
TEST(RuleEngineExprEvalParens, parens_expression)
{
    expr_eval_should_pass("(1+2)*3", (1 + 2) * 3);
    expr_eval_should_pass("(1)", 1);
    expr_eval_should_pass("(2)", 2);
    expr_eval_should_pass("((2))", 2);
    expr_eval_should_pass("(((3)))", 3);
    expr_eval_should_pass("(((3)))*(1+(2))", 9);
}

/* Assign expression Tests */
TEST(RuleEngineExprEvalAssign, assign_expression)
{
    expr_eval_should_pass("x=5", 5);
    expr_eval_should_pass("x=y=3", 3);
}

/* Comma expression Tests */
TEST(RuleEngineExprEvalComma, comma_expression)
{
    expr_eval_should_pass("2,3,4", 4);
    expr_eval_should_pass("2+3,4*5", 4 * 5);
    expr_eval_should_pass("x=5, x", 5);
    expr_eval_should_pass("x=5, y = 3, x+y", 8);
    expr_eval_should_pass("x=5, x=(x!=0)", 1);
    expr_eval_should_pass("x=5, x = x+1", 6);
}

/* Function expression Tests */
TEST(RuleEngineExprEvalFunction, function_expression)
{
    expr_eval_should_pass("add(1,2) + next(3)", 7);
    expr_eval_should_pass("add(1,next(2))", 4);
    expr_eval_should_pass("add(1,1+1) + add(2*2+1,2)", 10);
    expr_eval_should_pass("nop()", 0);
    expr_eval_should_pass("x=2,add(1, next(x))", 4);
    expr_eval_should_pass("$(zero), zero()", 0);
    expr_eval_should_pass("$(zero), zero(1, 2, 3)", 0);
    expr_eval_should_pass("$(one, 1), one()+one(1)+one(1, 2, 4)", 3);
    expr_eval_should_pass("$(number, 1), $(number, 2+3), number()", 5);
}

/* Name collision expression Tests */
TEST(RuleEngineExprEvalNameCollision, name_collision_expression)
{
    expr_eval_should_pass("next=5", 5);
    expr_eval_should_pass("next=2,next(5)+next", 8);
}

/* Fancy variable names expression Tests */
TEST(RuleEngineExprEvalFancyVarName, fancy_var_name_expression)
{
    expr_eval_should_pass("one=1", 1);
    expr_eval_should_pass("один=1", 1);
    expr_eval_should_pass("six=6, seven=7, six*seven", 42);
    expr_eval_should_pass("шість=6, сім=7, шість*сім", 42);
    expr_eval_should_pass("六=6, 七=7, 六*七", 42);
    expr_eval_should_pass("ταῦ=1618, 3*ταῦ", 3 * 1618);
    expr_eval_should_pass("$(ταῦ, 1618), 3*ταῦ()", 3 * 1618);
    expr_eval_should_pass("x#4=12, x#3=3, x#4+x#3", 15);
}

/* Auto comma expression Tests */
TEST(RuleEngineExprEvalAutoComma, auto_comma_expression)
{
    expr_eval_should_pass("a=3\na+2\n", 5);
    expr_eval_should_pass("a=3\n\n\na+2\n", 5);
    expr_eval_should_pass("\n\na=\n3\n\n\na+2\n", 5);
    expr_eval_should_pass("\n\n3\n\n", 3);
    expr_eval_should_pass("\n\n\n\n", 0);
    expr_eval_should_pass("3\n\n\n\n", 3);
    expr_eval_should_pass("a=3\nb=4\na", 3);
    expr_eval_should_pass("(\n2+3\n)\n", 5);
    expr_eval_should_pass("a=\n3*\n(4+\n3)\na+\na\n", 42);
}

/* Bad syntax Tests */
TEST(RuleEngineExprEvalBadSyntax, bad_syntax_expression)
{
    expr_eval_should_fail("(");
    expr_eval_should_fail(")");
    expr_eval_should_fail("()3");
    expr_eval_should_fail("()x");
    expr_eval_should_fail("0^+1");
    expr_eval_should_fail("()\\");
    expr_eval_should_fail("().");
    expr_eval_should_fail("4ever");
    expr_eval_should_fail("(2+3");
    expr_eval_should_fail("(-2");
    expr_eval_should_fail("*2");
    expr_eval_should_fail("nop=");
    expr_eval_should_fail("nop(");
    expr_eval_should_fail("unknownfunc()");
    expr_eval_should_fail("$(recurse, recurse()), recurse()");
    expr_eval_should_fail("),");
    expr_eval_should_fail("+(");
    expr_eval_should_fail("2=3");
    expr_eval_should_fail("2.3.4");
    expr_eval_should_fail("1()");
    expr_eval_should_fail("x()");
    expr_eval_should_fail(",");
    expr_eval_should_fail("1,,2");
    expr_eval_should_fail("nop(,x)");
    expr_eval_should_fail("nop(x=)>1");
    expr_eval_should_fail("1 x");
    expr_eval_should_fail("1++");
    expr_eval_should_fail("foo((x))");
    expr_eval_should_fail("nop(x))");
    expr_eval_should_fail("nop((x)");
    expr_eval_should_fail("$($())");
    expr_eval_should_fail("$(1)");
    expr_eval_should_fail("$()");
}

/* Curly braces variables */
TEST(RuleEngineExprEvalCurlyBracesVars, curly_braces_vars)
{
    expr_eval_should_fail("{");
    expr_eval_should_fail("}");
    expr_eval_should_fail("{}");
    expr_eval_should_fail("var{1");
    expr_eval_should_fail("var{");
    expr_eval_should_fail("var}");
    expr_eval_should_fail("var{test");
    expr_eval_should_fail("var{test}");
    expr_eval_should_fail("var{test}x{tst}");

    expr_eval_expected_vars("var{int}x=5", {"var{int}x"});
    expr_eval_expected_vars("var{int}x, var{int}y", {"var{int}x", "var{int}y"});
    expr_eval_expected_vars("var{int}x<5+var", {"var{int}x", "var"});
    expr_eval_expected_vars("var{test}test-5", {"var{test}test"});
    expr_eval_expected_vars("var{test}test+var1**var7, add(var{test}test, var1)", {"var{test}test", "var1", "var7"});
    expr_eval_expected_vars("add(var{test}test, var1)", {"var{test}test", "var1"});
    expr_eval_expected_vars("var{var{test}y}x", {"var{var{test}y}x"});
    expr_eval_expected_vars("var{test}x{tst}a", {"var{test}x{tst}a"});

    expr_eval_should_pass("var{test}test = 1, var{test2}test =  2, add(var{test}test, var{test2}test)", 3);
    expr_eval_should_pass("var{test}test = 1, var{test2}test = -2, add(var{test}test, var{test2}test)", -1);
    expr_eval_should_pass("var{test}test = 1, var{test}test2 = -2, add(var{test}test, var{test}test2)", -1);
}
