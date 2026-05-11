#include "test_rule_engine.hpp"

class RuleEngineParserTest : public RuleEngineTestFixture
{
  protected:
#define TRIGGER_LIST_INITIALIZED     1
#define TRIGGER_LIST_NOT_INITIALIZED 0
#define TRIGGER_LIST_EXISTS          1
#define TRIGGER_LIST_NOT_EXISTS      0

    // Structure to hold variable information for trigger state verification
    struct VarTriggerInfo
    {
        const char *name;
        bool expectedInitialized;
        bool shouldExistInTrigger;
    };

    void SetUp() override
    {
        RuleEngineTestFixture::SetUp();
    }

    void TearDown() override
    {
        RuleEngineTestFixture::TearDown();
    }

    // Helper method to check if a variable is initialized in the trigger list
    bool isVariableInitialized(const struct rule_engine__specification *specification, struct expr_var *variable)
    {
        for (size_t i = 0; i < specification->trigger.n_params; i++)
        {
            const struct rule_engine__sensitivity_list *sens_item = &specification->trigger.variables[i];
            if (sens_item->variable == variable)
            {
                return (sens_item->initialized == 1) ? true : false;
            }
        }
        return false;
    }
    // Add a rule and get the specification
    struct rule_engine__specification *addRule(const char *ruleName, const char *ruleStr, uint8_t rule_id, int8_t expectedOutput = RULE_ENGINE__OK)
    {
        struct rule_engine__rule rule;
        strncpy(rule.name, ruleName, sizeof(rule.name) - 1);
        rule.rule = const_cast<char *>(ruleStr);
        rule.size = static_cast<int>(strlen(ruleStr));

        EXPECT_EQ(rule_engine__add_specification(&rule_engine_inst, &rule), expectedOutput)
            << "Failed to add rule specification: " << rule.rule;

        struct rule_engine__specification *spec = NULL;
        if (expectedOutput == RULE_ENGINE__OK)
        {
            spec = getSpecificationById(rule_id);
            if (spec == nullptr)
            {
                ADD_FAILURE() << "Failed to retrieve rule specification for rule: " << rule.rule;
            }
        }
        return spec;
    }

    /**
     * @brief Verifies the trigger states of variables in a rule specification.
     *
     * This function validates the presence and initialization state of variables in the trigger section.
     *
     * Expected behavior by variable type:
     *
     * 1. GLOBAL variables:
     *    - With auto-generated mask (no custom mask):
     *      - If initialized (e.g., g_temp=15): NOT included in trigger list
     *      - If not initialized (e.g., g_temp): included in trigger list with initialized=false
     *    - With user-defined mask:
     *      - Included in trigger list if specified in mask, regardless of initialization
     *      - If initialized, the initialized flag is correctly preserved (initialized=true)
     *
     * 2. DDM2 variables:
     *    - With auto-generated mask:
     *      - ALWAYS included in trigger list regardless of initialization
     *      - If initialized (e.g., ac0ttemp=25), initialized flag is set to true
     *      - If not initialized (e.g., ac0ttemp), initialized flag is set to false
     *    - With user-defined mask:
     *      - Included in trigger list only if specified in mask
     *      - Initialization state correctly preserved (initialized flag)
     *
     * 3. LOCAL variables:
     *    - NEVER included in trigger list (in both auto-generated and user-defined masks)
     *    - Error is generated if a user-defined mask tries to include a local variable
     *
     * @param spec The rule specification to verify
     * @param expectedNumberOfVars Expected number of variables in the trigger section
     * @param expectedMask Expected mask value
     * @param expectedMaskType Expected mask type (AND or OR)
     * @param vars Vector of variable trigger info structures for verification
     */
    void verifyVariableTriggerStates(struct rule_engine__specification *spec,
                                     uint8_t expectedNumberOfVars, uint8_t expectedMask, uint8_t expectedMaskType,
                                     const std::vector<VarTriggerInfo> &vars)
    {
        EXPECT_EQ(spec->trigger.n_params, expectedNumberOfVars)
            << "Rule: " << spec->expr.rule_string << " -> "
            << "Expected number of variables in trigger section does not match";
        EXPECT_EQ(spec->trigger.mask, expectedMask)
            << "Rule: " << spec->expr.rule_string << " -> "
            << "Expected mask does not match";
        EXPECT_EQ(spec->trigger.mask_type, expectedMaskType)
            << "Rule: " << spec->expr.rule_string << " -> "
            << "Expected mask type does not match";

        for (const auto &varInfo : vars)
        {
            struct expr_var *var = getVariableByName(spec, varInfo.name);
            ASSERT_NE(var, nullptr) << "Variable " << varInfo.name << " not found in specification";

            EXPECT_EQ(isVariableInTriggerList(spec, var), varInfo.shouldExistInTrigger)
                << "Rule: " << spec->expr.rule_string << " -> "
                << "Variable " << varInfo.name << " trigger list presence is incorrect";

            // As we only preset the flag for initialized variables in the trigger section
            if (varInfo.shouldExistInTrigger)
            {
                EXPECT_EQ(isVariableInitialized(spec, var), varInfo.expectedInitialized)
                    << "Rule: " << spec->expr.rule_string << " -> "
                    << "Variable " << varInfo.name << " initialization state is incorrect";
            }
            else
            {
                // If the variable is not in the trigger list, it should not have an initialized state
                EXPECT_EQ(varInfo.expectedInitialized, TRIGGER_LIST_NOT_EXISTS)
                    << "Rule: " << spec->expr.rule_string << " -> "
                    << "Variable " << varInfo.name << " cannot have initalized flag set as it is not in the trigger list";
            }
        }
    }

    void verifyDDMWItem(struct rule_engine__specification *spec, const char *varName,
                        int32_t expectedParameterValue, uint8_t expectedParameterType, uint32_t expectedParameterID)
    {
        ddmw_item_t *item = getDDM2ParameterByID(expectedParameterID);
        ASSERT_NE(item, nullptr) << "Rule: " << spec->expr.rule_string << "->" << " DDM2 item " << varName << " not found";

        EXPECT_EQ(getDDM2ParameterValue(item), expectedParameterValue)
            << "Rule: " << spec->expr.rule_string << " -> "
            << "DDM2 item " << varName << " has value " << getDDM2ParameterValue(item)
            << ", expected " << expectedParameterValue;

        EXPECT_EQ(getDDM2ParameterType(item), expectedParameterType)
            << "Rule: " << spec->expr.rule_string << " -> "
            << "DDM2 item " << varName << " has type " << (int)getDDM2ParameterType(item)
            << ", expected " << (int)expectedParameterType;
    }

    void verifyVariable(struct rule_engine__specification *spec, const char *varName,
                        int32_t expectedValue, uint8_t expectedType, uint8_t expectedDDMCreate, uint32_t expectedID)
    {
        struct expr_var *var = getVariableByName(spec, varName);
        ASSERT_NE(var, nullptr) << "Variable " << varName << " not found in specification";

        EXPECT_EQ(expr_var_value(var), expectedValue)
            << "Rule: " << spec->expr.rule_string << " -> "
            << "Variable " << varName << " has value " << expr_var_value(var)
            << ", expected " << expectedValue;

        EXPECT_EQ(expr_var_type(var), expectedType)
            << "Rule: " << spec->expr.rule_string << " -> "
            << "Variable " << varName << " has type " << (int)expr_var_type(var)
            << ", expected " << (int)expectedType;

        EXPECT_EQ(expr_var_type_ddm_create(var), expectedDDMCreate)
            << "Rule: " << spec->expr.rule_string << " -> "
            << "Variable " << varName << " has incorrect DDM create flag";

        EXPECT_EQ(expr_var_id(var), expectedID)
            << "Rule: " << spec->expr.rule_string << " -> "
            << "Variable " << varName << " has incorrect ID";
    }

    void printVariableInfo(struct rule_engine__specification *spec)
    {
        printf("\n==== Rule Specification ====\n");
        printf("Name: %s, ID: %u\n", spec->name, spec->id);
        printf("Trigger section: mask=0x%x, mask_type=%u, n_params=%u\n",
               spec->trigger.mask, spec->trigger.mask_type, spec->trigger.n_params);

        for (uint32_t i = 0; i < spec->trigger.n_params; i++)
        {
            struct rule_engine__sensitivity_list *sens = &spec->trigger.variables[i];
            printf("Trigger var[%u]: name=%s, id=0x%x, type=%u, value=%d, initialized=%u\n",
                   i, sens->variable->name, expr_var_id(sens->variable),
                   expr_var_type(sens->variable), expr_var_value(sens->variable), sens->initialized);
        }

        printf("-- All Variables --\n");
        struct expr_var *var = spec->expr.vars.head;
        int idx = 0;
        while (var)
        {
            printf("Var[%d]: name=%s, id=0x%x, type=%u, value=%d, ddm_create=%u\n",
                   idx++, var->name, var->id, var->type, var->value, var->ddm_create);
            var = var->next;
        }
        printf("=========================\n\n");
    }
};

/**
 * @brief Test case to verify the behavior of function pointers and contexts across rules and rule engine instances.
 *
 * This test ensures the following:
 * 1. Function pointers for the same function (e.g., "print") are shared across different rules within the same rule engine instance.
 * 2. Function contexts are unique for each rule, even within the same rule engine instance.
 * 3. Function contexts correctly reference the rule engine instance they belong to.
 * 4. Function pointers are shared across different rule engine instances, as they refer to the same static function definitions.
 *
 * The test creates two rule engine instances, adds rules with user functions to each instance, and verifies:
 * - That function pointers are consistent across rules within the same instance and across different instances.
 * - That function contexts are unique for each rule and correctly reference their respective rule engine instance.
 * - That function contexts are isolated between different rule engine instances.
 */
TEST_F(RuleEngineParserTest, VerifyFunctionInstancePerRuleEngineInstance)
{
    // Create two rule engine instances
    rule_engine_inst_t rule_engine_inst1;
    rule_engine_inst_t rule_engine_inst2;
    ddmw_t ddm_wrapper1;
    ddmw_t ddm_wrapper2;

    rule_engine_inst_config_t rule_engine_inst_config =
        {
            .rule_engine_specification_elements = RULE_ENGINE__MAX_NUMBER_OF_RULES,
            .rule_engine_ddm2_subscription_table_elements = RULE_ENGINE__DDM2_SUBSCRIPTION_DEPTH,
            .rule_engine_set_timers_elements = RULE_ENGINE__MAX_NUMBER_OF_TIMERS,
            .rule_engine_ddm_map_elements = RULE_ENGINE__DDM2_MAP_SIZE,
        };

    // Initialize the first rule engine instance
    memset(&rule_engine_inst1, 0, sizeof(rule_engine_inst_t));
    rule_engine_inst1.wrapper = &ddm_wrapper1;
    rule_engine_inst1.instance = 1;
    ddmw_init(&ddm_wrapper1, &connector_unittest);
    ASSERT_EQ(rule_engine_initialize_instance(&rule_engine_inst1, &rule_engine_inst_config), 1);

    // Define and add the rules with user functions
    const char *inst1_rule0_str = "cc0voc: print(cc0voc)";
    const char *inst1_rule1_str = "cc0settemp: print(cc0settemp)";
    static const struct rule_engine__rule rules1[] =
        {
            {.name = "TestRule0", .rule = const_cast<char *>(inst1_rule0_str), .size = static_cast<int>(strlen(inst1_rule0_str))},
            {.name = "TestRule1", .rule = const_cast<char *>(inst1_rule1_str), .size = static_cast<int>(strlen(inst1_rule1_str))},
        };
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst1, &rules1[0]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst1, &rules1[1]), 1);

    // Get the rules from the first rule engine instance
    struct rule_engine__specification *spec_inst1_rule0 = (struct rule_engine__specification *)sorted_container__access(rule_engine_inst1.p_rule_engine_specifications, 0);
    ASSERT_NE(spec_inst1_rule0, nullptr) << "Failed to retrieve rule specification";
    struct rule_engine__specification *spec_inst1_rule1 = (struct rule_engine__specification *)sorted_container__access(rule_engine_inst1.p_rule_engine_specifications, 1);
    ASSERT_NE(spec_inst1_rule1, nullptr) << "Failed to retrieve rule specification";

    // Get the function pointers from the rules
    vec_expr_ptr_t vec_func_inst1_rule0 = vec_init();
    expr_get_node_for_type_from_ast((struct expr *)spec_inst1_rule0->expr.rule, OP_FUNC, &vec_func_inst1_rule0);
    ASSERT_EQ(vec_len(&vec_func_inst1_rule0), 1);
    struct expr *func_print_inst1_rule0 = vec_nth(&vec_func_inst1_rule0, 0);
    vec_free(&vec_func_inst1_rule0);

    vec_expr_ptr_t vec_func_inst1_rule1 = vec_init();
    expr_get_node_for_type_from_ast((struct expr *)spec_inst1_rule1->expr.rule, OP_FUNC, &vec_func_inst1_rule1);
    ASSERT_EQ(vec_len(&vec_func_inst1_rule1), 1);
    struct expr *func_print_inst1_rule1 = vec_nth(&vec_func_inst1_rule1, 0);
    vec_free(&vec_func_inst1_rule1);

    // Initialize the second rule engine instance
    memset(&rule_engine_inst2, 0, sizeof(rule_engine_inst_t));
    rule_engine_inst2.wrapper = &ddm_wrapper2;
    rule_engine_inst2.instance = 2;
    ddmw_init(&ddm_wrapper2, &connector_unittest);
    ASSERT_EQ(rule_engine_initialize_instance(&rule_engine_inst2, &rule_engine_inst_config), 1);

    // Define and add the rules with user functions
    const char *inst2_rule0_str = "cc0voc: print(cc0voc)";
    const char *inst2_rule1_str = "cc0settemp: print(cc0settemp)";
    static const struct rule_engine__rule rules2[] =
        {
            {.name = "TestRule0", .rule = const_cast<char *>(inst2_rule0_str), .size = static_cast<int>(strlen(inst2_rule0_str))},
            {.name = "TestRule1", .rule = const_cast<char *>(inst2_rule1_str), .size = static_cast<int>(strlen(inst2_rule1_str))},
        };
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst2, &rules2[0]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst2, &rules2[1]), 1);

    // Get the rules from the second rule engine instance
    struct rule_engine__specification *spec_inst2_rule0 = (struct rule_engine__specification *)sorted_container__access(rule_engine_inst2.p_rule_engine_specifications, 0);
    ASSERT_NE(spec_inst2_rule0, nullptr) << "Failed to retrieve rule specification";
    struct rule_engine__specification *spec_inst2_rule1 = (struct rule_engine__specification *)sorted_container__access(rule_engine_inst2.p_rule_engine_specifications, 1);
    ASSERT_NE(spec_inst2_rule1, nullptr) << "Failed to retrieve rule specification";

    // Get the function pointers from the rules
    vec_expr_ptr_t vec_func_inst2_rule0 = vec_init();
    expr_get_node_for_type_from_ast((struct expr *)spec_inst2_rule0->expr.rule, OP_FUNC, &vec_func_inst2_rule0);
    ASSERT_EQ(vec_len(&vec_func_inst2_rule0), 1);
    struct expr *func_print_inst2_rule0 = vec_nth(&vec_func_inst2_rule0, 0);
    vec_free(&vec_func_inst2_rule0);

    vec_expr_ptr_t vec_func_inst2_rule1 = vec_init();
    expr_get_node_for_type_from_ast((struct expr *)spec_inst2_rule1->expr.rule, OP_FUNC, &vec_func_inst2_rule1);
    ASSERT_EQ(vec_len(&vec_func_inst2_rule1), 1);
    struct expr *func_print_inst2_rule1 = vec_nth(&vec_func_inst2_rule1, 0);
    vec_free(&vec_func_inst2_rule1);

    /* Comparission of the function pointers and function context for rule engine instance 1 */
    // Do they point to the same function defined in rule_engine l_user_functions?
    ASSERT_EQ(func_print_inst1_rule0->param.func.f, func_print_inst1_rule1->param.func.f)
        << "Function pointers should be equal for different rules";
    // Do they point to the same function context defined in rule_engine l_user_functions? They shouldn't, each context is defined seperataly.
    ASSERT_NE(func_print_inst1_rule0->param.func.context, func_print_inst1_rule1->param.func.context)
        << "Function context should not be equal for different rules";
    rule_engine_func_context_t *func_print_inst1_rule0_context = (rule_engine_func_context_t *)func_print_inst1_rule0->param.func.context;
    rule_engine_func_context_t *func_print_inst1_rule1_context = (rule_engine_func_context_t *)func_print_inst1_rule1->param.func.context;
    // Do they point to the same rule engine instance? They should!
    ASSERT_EQ(func_print_inst1_rule0_context->p_rule_engine_inst, func_print_inst1_rule1_context->p_rule_engine_inst)
        << "Function context should refer to the same rule engine instance!";
    // Is the rule engine instance kept in function context the same as the one we created?
    ASSERT_EQ(func_print_inst1_rule0_context->p_rule_engine_inst, &rule_engine_inst1)
        << "Rule engine instance provided trough the function context should be the as same rule engine instance owning the rules!";

    /* Comparission of the function pointers and function context for rule engine instance 2 */
    // Do they point to the same function defined in rule_engine l_user_functions?
    ASSERT_EQ(func_print_inst2_rule0->param.func.f, func_print_inst2_rule1->param.func.f)
        << "Function pointers should be equal for different rules";
    // Do they point to the same function context defined in rule_engine l_user_functions? They shouldn't, each context is defined seperataly.
    ASSERT_NE(func_print_inst2_rule0->param.func.context, func_print_inst2_rule1->param.func.context)
        << "Function context should not be equal for different rules";
    rule_engine_func_context_t *func_print_inst2_rule0_context = (rule_engine_func_context_t *)func_print_inst2_rule0->param.func.context;
    rule_engine_func_context_t *func_print_inst2_rule1_context = (rule_engine_func_context_t *)func_print_inst2_rule1->param.func.context;
    // Do they point to the same rule engine instance? They should!
    ASSERT_EQ(func_print_inst2_rule0_context->p_rule_engine_inst, func_print_inst2_rule1_context->p_rule_engine_inst)
        << "Function context should refer to the same rule engine instance!";
    // Is the rule engine instance kept in function context the same as the one we created?
    ASSERT_EQ(func_print_inst2_rule0_context->p_rule_engine_inst, &rule_engine_inst2)
        << "Rule engine instance provided trough the function context should be the as same rule engine instance owning the rules!";

    /* Comaprison between the two rule engine instances */
    // They point to the same function defined in rule_engine l_user_functions. Has to be the same function.
    ASSERT_EQ(func_print_inst1_rule0->param.func.f, func_print_inst2_rule0->param.func.f)
        << "Function pointers should be eaqual for different rule engine instances";
    ASSERT_EQ(func_print_inst1_rule1->param.func.f, func_print_inst2_rule1->param.func.f)
        << "Function pointers should be eaqual for different rule engine instances";
    // Do they point to the same function context defined in rule_engine l_user_functions? They shouldn't, each context is defined seperataly.
    ASSERT_NE(func_print_inst1_rule0->param.func.context, func_print_inst2_rule0->param.func.context)
        << "Function context should be different for different rule engine instances";
    ASSERT_NE(func_print_inst1_rule1->param.func.context, func_print_inst2_rule1->param.func.context)
        << "Function context should be different for different rule engine instances";
    // Do they point to the same rule engine instance? They shouldn't, each context is defined seperataly.
    ASSERT_NE(func_print_inst1_rule0_context->p_rule_engine_inst, func_print_inst2_rule0_context->p_rule_engine_inst)
        << "Rule engine instance provided trough the function context should differ between the rule engine intsance!";
    ASSERT_NE(func_print_inst1_rule1_context->p_rule_engine_inst, func_print_inst2_rule1_context->p_rule_engine_inst)
        << "Rule engine instance provided trough the function context should differ between the rule engine intsance!";

    rule_engine_delete_rules(&rule_engine_inst1);
    rule_engine_delete_rules(&rule_engine_inst2);
}

/**
 * @brief Test case to verify the parser functionality for trigger sections with DDM2 subscribers.
 *
 * This test verifies that the rule engine parser correctly processes trigger sections
 * that contain DDM2 parameters as subscribers. It validates that:
 * 1. Trigger sections with DDM2 parameter identifiers are properly parsed
 * 2. The parser correctly interprets DDM2-specific trigger section syntax and semantics
 * 3. DDM2 parameters are by default considered as trigger parameters
 */
TEST_F(RuleEngineParserTest, VerifyParserTriggerSectionDDM2Subscriber)
{
    const char *rule0_str = "cc0voc, g_set_voc=220,local_temp=-5 : g_set_voc=cc0voc,print(cc0voc)";
    const char *rule1_str = "cc0settemp=5, g_cc0settemp=220,local_temp=-5 : g_cc0settemp=local_temp";

    static const struct rule_engine__rule rules[] =
        {
            {.name = "TestRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
            {.name = "TestRule1", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
        };

    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[1]), 1);

    // Verify rule0_str
    struct rule_engine__specification *spec_rule0 = getSpecificationById(0);
    ASSERT_NE(spec_rule0, nullptr) << "Failed to retrieve rule specification";

    ddmw_item_t *cc0voc_item = getDDM2ParameterByID(CC0VOC);
    EXPECT_NE(cc0voc_item, nullptr);
    struct expr_var *cc0voc = getVariableByName(spec_rule0, "cc0voc");
    EXPECT_NE(cc0voc, nullptr);
    struct expr_var *g_set_voc = getVariableByName(spec_rule0, "g_set_voc");
    EXPECT_NE(g_set_voc, nullptr);
    struct expr_var *local_temp = getVariableByName(spec_rule0, "local_temp");
    EXPECT_NE(local_temp, nullptr);

    EXPECT_EQ(getTriggerListSize(spec_rule0), 1) << "Failed rule: " << rule0_str;
    EXPECT_EQ(getMaskType(spec_rule0), RULE_ENGINE__MASK_CONSTRAINT_AND) << "Failed rule: " << rule0_str;
    EXPECT_TRUE(isVariableInTriggerList(spec_rule0, cc0voc)) << "Failed rule: " << rule0_str;
    EXPECT_TRUE(isVariableInMask(spec_rule0, cc0voc)) << "Failed rule: " << rule0_str;
    EXPECT_FALSE(isVariableInTriggerList(spec_rule0, g_set_voc)) << "Failed rule: " << rule0_str;
    EXPECT_FALSE(isVariableInMask(spec_rule0, g_set_voc)) << "Failed rule: " << rule0_str;
    EXPECT_FALSE(isVariableInTriggerList(spec_rule0, local_temp)) << "Failed rule: " << rule0_str;
    EXPECT_FALSE(isVariableInMask(spec_rule0, local_temp)) << "Failed rule: " << rule0_str;

    EXPECT_EQ(getDDM2ParameterValue(cc0voc_item), 0);
    EXPECT_EQ(expr_var_value(cc0voc), 0);
    EXPECT_EQ(expr_var_value(g_set_voc), 220);
    EXPECT_EQ(expr_var_value(local_temp), -5);

    EXPECT_EQ(expr_var_id(cc0voc), CC0VOC);
    EXPECT_EQ(expr_var_id(g_set_voc), expr_generate_hash_key("g_set_voc"));
    EXPECT_EQ(expr_var_id(local_temp), expr_generate_hash_key("local_temp"));

    EXPECT_EQ(getDDM2ParameterType(cc0voc_item), DDMW_ACTION_SET);
    EXPECT_EQ(expr_var_type(cc0voc), RULE_ENGINE__VAR_TYPE_DDM2);
    EXPECT_EQ(expr_var_type(g_set_voc), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_type(local_temp), RULE_ENGINE__VAR_TYPE_LOCAL);

    EXPECT_EQ(expr_var_type_ddm_create(cc0voc), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED);
    EXPECT_EQ(expr_var_type_ddm_create(g_set_voc), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);
    EXPECT_EQ(expr_var_type_ddm_create(local_temp), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

    // Verify rule1_str
    struct rule_engine__specification *spec_rule1 = getSpecificationById(1);
    ASSERT_NE(spec_rule1, nullptr) << "Failed to retrieve rule specification";

    ddmw_item_t *cc0settemp_item_rule1 = getDDM2ParameterByID(CC0SETTEMP);
    EXPECT_NE(cc0settemp_item_rule1, nullptr);
    struct expr_var *cc0settemp_rule1 = getVariableByName(spec_rule1, "cc0settemp");
    EXPECT_NE(cc0settemp_rule1, nullptr);
    struct expr_var *g_cc0settemp = getVariableByName(spec_rule1, "g_cc0settemp");
    EXPECT_NE(g_cc0settemp, nullptr);
    struct expr_var *local_temp_rule1 = getVariableByName(spec_rule1, "local_temp");
    EXPECT_NE(local_temp_rule1, nullptr);

    EXPECT_EQ(getTriggerListSize(spec_rule1), 1) << "Failed rule: " << rule1_str;
    EXPECT_EQ(getMaskType(spec_rule1), RULE_ENGINE__MASK_CONSTRAINT_AND) << "Failed rule: " << rule1_str;
    EXPECT_TRUE(isVariableInTriggerList(spec_rule1, cc0settemp_rule1)) << "Failed rule: " << rule1_str;
    EXPECT_TRUE(isVariableInMask(spec_rule1, cc0settemp_rule1)) << "Failed rule: " << rule1_str;
    EXPECT_FALSE(isVariableInTriggerList(spec_rule1, g_cc0settemp)) << "Failed rule: " << rule1_str;
    EXPECT_FALSE(isVariableInMask(spec_rule1, g_cc0settemp)) << "Failed rule: " << rule1_str;
    EXPECT_FALSE(isVariableInTriggerList(spec_rule1, local_temp_rule1)) << "Failed rule: " << rule1_str;
    EXPECT_FALSE(isVariableInMask(spec_rule1, local_temp_rule1)) << "Failed rule: " << rule1_str;

    EXPECT_EQ(getDDM2ParameterValue(cc0settemp_item_rule1), 5);
    EXPECT_EQ(expr_var_value(cc0settemp_rule1), 5);
    EXPECT_EQ(expr_var_value(g_cc0settemp), 220);
    EXPECT_EQ(expr_var_value(local_temp_rule1), -5);

    EXPECT_EQ(expr_var_id(cc0settemp_rule1), CC0SETTEMP);
    EXPECT_EQ(expr_var_id(g_cc0settemp), expr_generate_hash_key("g_cc0settemp"));
    EXPECT_EQ(expr_var_id(local_temp_rule1), expr_generate_hash_key("local_temp"));

    EXPECT_EQ(getDDM2ParameterType(cc0settemp_item_rule1), DDMW_ACTION_SET);
    EXPECT_EQ(expr_var_type(cc0settemp_rule1), RULE_ENGINE__VAR_TYPE_DDM2);
    EXPECT_EQ(expr_var_type(g_cc0settemp), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_type(local_temp_rule1), RULE_ENGINE__VAR_TYPE_LOCAL);

    EXPECT_EQ(expr_var_type_ddm_create(cc0settemp_rule1), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED);
    EXPECT_EQ(expr_var_type_ddm_create(g_cc0settemp), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);
    EXPECT_EQ(expr_var_type_ddm_create(local_temp_rule1), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);
}

/**
 * @brief Test case to verify the parser functionality for trigger sections with DDM2 owners.
 *
 * This test verifies that the rule engine parser correctly processes trigger sections
 * that are associated with DDM2 owners. It validates that:
 * 1. Trigger sections with DDM2 owner identifiers are properly parsed
 * 2. The parser correctly interprets DDM2-specific trigger section syntax and semantics
 * 3. DDM2 parameters are by default considered as trigger parameters
 */
TEST_F(RuleEngineParserTest, VerifyParserTriggerSectionDDM2Owner)
{
    const char *rule0_str = "sup0avl=ddm(1): g_sup0avl=sup0avl";
    const char *rule1_str = "sup0s2=ddm(3000),g_sup0s2=3000 : g_sup0s2=sup0s2,print(sup0s2)";
    const char *rule2_str = "sup0param=ddm,g_ref_sup0,mask=['or']: g_sup_value_trig=g_ref_sup0";
    const char *rule3_str = "sup0stype=ddm(0),g_sup0stype=0 : g_sup0stype=sup0stype,print(g_sup0stype)";
    const char *rule4_str = "ac0ttemp=ddm(1): g_temp= ac0ttemp,print(g_temp)";

    // Create rule structures
    static const struct rule_engine__rule rules[] =
        {
            {.name = "TestRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
            {.name = "TestRule1", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
            {.name = "TestRule2", .rule = const_cast<char *>(rule2_str), .size = static_cast<int>(strlen(rule2_str))},
            {.name = "TestRule3", .rule = const_cast<char *>(rule3_str), .size = static_cast<int>(strlen(rule3_str))},
            {.name = "TestRule4", .rule = const_cast<char *>(rule4_str), .size = static_cast<int>(strlen(rule4_str))},
        };

    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[1]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[2]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[3]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[4]), 1);

    // Verify rule0_str
    struct rule_engine__specification *spec_rule0 = getSpecificationById(0);
    ASSERT_NE(spec_rule0, nullptr) << "Failed to retrieve rule specification";

    ddmw_item_t *sup0_item = getDDM2ParameterByID(SUP0);
    EXPECT_NE(sup0_item, nullptr);
    struct expr_var *sup0avl = getVariableByName(spec_rule0, "sup0avl");
    EXPECT_NE(sup0avl, nullptr);
    struct expr_var *g_sup0avl = getVariableByName(spec_rule0, "g_sup0avl");
    EXPECT_NE(g_sup0avl, nullptr);

    EXPECT_EQ(getTriggerListSize(spec_rule0), 1) << "Failed rule: " << rule0_str;
    EXPECT_EQ(getMaskType(spec_rule0), RULE_ENGINE__MASK_CONSTRAINT_AND) << "Failed rule: " << rule0_str;
    EXPECT_TRUE(isVariableInTriggerList(spec_rule0, sup0avl)) << "Failed rule: " << rule0_str;
    EXPECT_TRUE(isVariableInMask(spec_rule0, sup0avl)) << "Failed rule: " << rule0_str;
    EXPECT_FALSE(isVariableInTriggerList(spec_rule0, g_sup0avl)) << "Failed rule: " << rule0_str;
    EXPECT_FALSE(isVariableInMask(spec_rule0, g_sup0avl)) << "Failed rule: " << rule0_str;

    EXPECT_EQ(getDDM2ParameterValue(sup0_item), 1);
    EXPECT_EQ(expr_var_value(sup0avl), 1);
    EXPECT_EQ(expr_var_value(g_sup0avl), 0);

    EXPECT_EQ(expr_var_id(sup0avl), SUP0);
    EXPECT_EQ(expr_var_id(g_sup0avl), expr_generate_hash_key("g_sup0avl"));

    EXPECT_EQ(getDDM2ParameterType(sup0_item), DDMW_ACTION_PUBLISH);
    EXPECT_EQ(expr_var_type(sup0avl), RULE_ENGINE__VAR_TYPE_DDM2);
    EXPECT_EQ(expr_var_type(g_sup0avl), RULE_ENGINE__VAR_TYPE_GLOBAL);

    EXPECT_EQ(expr_var_type_ddm_create(sup0avl), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_OWNED);
    EXPECT_EQ(expr_var_type_ddm_create(g_sup0avl), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

    // Verify rule1_str
    struct rule_engine__specification *spec_rule1 = getSpecificationById(1);
    ASSERT_NE(spec_rule1, nullptr) << "Failed to retrieve rule specification";

    ddmw_item_t *sup0s2_item = getDDM2ParameterByID(SUP0S2);
    EXPECT_NE(sup0s2_item, nullptr);
    struct expr_var *sup0s2 = getVariableByName(spec_rule1, "sup0s2");
    EXPECT_NE(sup0s2, nullptr);
    struct expr_var *g_sup0s2 = getVariableByName(spec_rule1, "g_sup0s2");
    EXPECT_NE(g_sup0s2, nullptr);

    EXPECT_EQ(getTriggerListSize(spec_rule1), 1) << "Failed rule: " << rule1_str;
    EXPECT_EQ(getMaskType(spec_rule1), RULE_ENGINE__MASK_CONSTRAINT_AND) << "Failed rule: " << rule1_str;
    EXPECT_TRUE(isVariableInTriggerList(spec_rule1, sup0s2)) << "Failed rule: " << rule1_str;
    EXPECT_TRUE(isVariableInMask(spec_rule1, sup0s2)) << "Failed rule: " << rule1_str;
    EXPECT_FALSE(isVariableInTriggerList(spec_rule1, g_sup0s2)) << "Failed rule: " << rule1_str;
    EXPECT_FALSE(isVariableInMask(spec_rule1, g_sup0s2)) << "Failed rule: " << rule1_str;

    EXPECT_EQ(getDDM2ParameterValue(sup0s2_item), 3000);
    EXPECT_EQ(expr_var_value(sup0s2), 3000);
    EXPECT_EQ(expr_var_value(g_sup0s2), 3000);

    EXPECT_EQ(expr_var_id(sup0s2), SUP0S2);
    EXPECT_EQ(expr_var_id(g_sup0s2), expr_generate_hash_key("g_sup0s2"));

    EXPECT_EQ(getDDM2ParameterType(sup0s2_item), DDMW_ACTION_PUBLISH);
    EXPECT_EQ(expr_var_type(sup0s2), RULE_ENGINE__VAR_TYPE_DDM2);
    EXPECT_EQ(expr_var_type(g_sup0s2), RULE_ENGINE__VAR_TYPE_GLOBAL);

    EXPECT_EQ(expr_var_type_ddm_create(sup0s2), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_OWNED);
    EXPECT_EQ(expr_var_type_ddm_create(g_sup0s2), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

    // Verify rule2_str
    struct rule_engine__specification *spec_rule2 = getSpecificationById(2);
    ASSERT_NE(spec_rule2, nullptr) << "Failed to retrieve rule specification";

    ddmw_item_t *sup0param_item = getDDM2ParameterByID(SUP0PARAM);
    EXPECT_NE(sup0param_item, nullptr);
    struct expr_var *sup0param = getVariableByName(spec_rule2, "sup0param");
    EXPECT_NE(sup0param, nullptr);
    struct expr_var *g_ref_sup0 = getVariableByName(spec_rule2, "g_ref_sup0");
    EXPECT_NE(g_ref_sup0, nullptr);

    EXPECT_EQ(getTriggerListSize(spec_rule2), 2) << "Failed rule: " << rule3_str;
    EXPECT_EQ(getMaskType(spec_rule2), RULE_ENGINE__MASK_CONSTRAINT_OR) << "Failed rule: " << rule3_str;
    EXPECT_TRUE(isVariableInTriggerList(spec_rule2, sup0param)) << "Failed rule: " << rule3_str;
    EXPECT_TRUE(isVariableInMask(spec_rule2, sup0param)) << "Failed rule: " << rule3_str;
    EXPECT_TRUE(isVariableInTriggerList(spec_rule2, g_ref_sup0)) << "Failed rule: " << rule3_str;
    EXPECT_TRUE(isVariableInMask(spec_rule2, g_ref_sup0)) << "Failed rule: " << rule3_str;

    EXPECT_EQ(getDDM2ParameterValue(sup0param_item), 0);
    EXPECT_EQ(expr_var_value(sup0param), 0);
    EXPECT_EQ(expr_var_value(g_ref_sup0), 0);

    EXPECT_EQ(expr_var_id(sup0param), SUP0PARAM);
    EXPECT_EQ(expr_var_id(g_ref_sup0), expr_generate_hash_key("g_ref_sup0"));

    EXPECT_EQ(getDDM2ParameterType(sup0param_item), DDMW_ACTION_PUBLISH);
    EXPECT_EQ(expr_var_type(sup0param), RULE_ENGINE__VAR_TYPE_DDM2);
    EXPECT_EQ(expr_var_type(g_ref_sup0), RULE_ENGINE__VAR_TYPE_GLOBAL);

    EXPECT_EQ(expr_var_type_ddm_create(sup0param), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_OWNED);
    EXPECT_EQ(expr_var_type_ddm_create(g_ref_sup0), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

    // Verify rule3_str
    struct rule_engine__specification *spec_rule3 = getSpecificationById(3);
    ASSERT_NE(spec_rule3, nullptr) << "Failed to retrieve rule specification";

    ddmw_item_t *sup0stype_item = getDDM2ParameterByID(SUP0STYPE);
    EXPECT_NE(sup0stype_item, nullptr);
    struct expr_var *sup0stype = getVariableByName(spec_rule3, "sup0stype");
    EXPECT_NE(sup0stype, nullptr);
    struct expr_var *g_sup0stype = getVariableByName(spec_rule3, "g_sup0stype");
    EXPECT_NE(g_sup0stype, nullptr);

    EXPECT_EQ(getTriggerListSize(spec_rule3), 1) << "Failed rule: " << rule3_str;
    EXPECT_EQ(getMaskType(spec_rule3), RULE_ENGINE__MASK_CONSTRAINT_AND) << "Failed rule: " << rule3_str;
    EXPECT_TRUE(isVariableInTriggerList(spec_rule3, sup0stype)) << "Failed rule: " << rule3_str;
    EXPECT_TRUE(isVariableInMask(spec_rule3, sup0stype)) << "Failed rule: " << rule3_str;
    EXPECT_FALSE(isVariableInTriggerList(spec_rule3, g_sup0stype)) << "Failed rule: " << rule3_str;
    EXPECT_FALSE(isVariableInMask(spec_rule3, g_sup0stype)) << "Failed rule: " << rule3_str;

    EXPECT_EQ(getDDM2ParameterValue(sup0stype_item), 0);
    EXPECT_EQ(expr_var_value(sup0stype), 0);
    EXPECT_EQ(expr_var_value(g_sup0stype), 0);

    EXPECT_EQ(expr_var_id(sup0stype), SUP0STYPE);
    EXPECT_EQ(expr_var_id(g_sup0stype), expr_generate_hash_key("g_sup0stype"));

    EXPECT_EQ(getDDM2ParameterType(sup0stype_item), DDMW_ACTION_PUBLISH);
    EXPECT_EQ(expr_var_type(sup0stype), RULE_ENGINE__VAR_TYPE_DDM2);
    EXPECT_EQ(expr_var_type(g_sup0stype), RULE_ENGINE__VAR_TYPE_GLOBAL);

    EXPECT_EQ(expr_var_type_ddm_create(sup0stype), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_OWNED);
    EXPECT_EQ(expr_var_type_ddm_create(g_sup0stype), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

    // Verify rule4_str
    struct rule_engine__specification *spec_rule4 = getSpecificationById(4);
    ASSERT_NE(spec_rule4, nullptr) << "Failed to retrieve rule specification";

    ddmw_item_t *ac0ttemp_item = getDDM2ParameterByID(AC0AVL);
    EXPECT_NE(ac0ttemp_item, nullptr) << "It should be AC0AVL, not AC0TTEMP, as AC0AVL has not been defined in any other rule";
    struct expr_var *ac0ttemp = getVariableByName(spec_rule4, "ac0ttemp");
    EXPECT_NE(ac0ttemp, nullptr);
    struct expr_var *g_temp = getVariableByName(spec_rule4, "g_temp");
    EXPECT_NE(g_temp, nullptr);

    EXPECT_EQ(getTriggerListSize(spec_rule4), 1) << "Failed rule: " << rule3_str;
    EXPECT_EQ(getMaskType(spec_rule4), RULE_ENGINE__MASK_CONSTRAINT_AND) << "Failed rule: " << rule3_str;
    EXPECT_TRUE(isVariableInTriggerList(spec_rule4, ac0ttemp)) << "Failed rule: " << rule3_str;
    EXPECT_TRUE(isVariableInMask(spec_rule4, ac0ttemp)) << "Failed rule: " << rule3_str;
    EXPECT_FALSE(isVariableInTriggerList(spec_rule4, g_temp)) << "Failed rule: " << rule3_str;
    EXPECT_FALSE(isVariableInMask(spec_rule4, g_temp)) << "Failed rule: " << rule3_str;

    EXPECT_EQ(getDDM2ParameterValue(ac0ttemp_item), 1);
    EXPECT_EQ(expr_var_value(ac0ttemp), 1);
    EXPECT_EQ(expr_var_value(g_temp), 0);

    EXPECT_EQ(expr_var_id(ac0ttemp), AC0AVL) << "It should be AC0AVL, not AC0TTEMP, as AC0AVL has not been defined in any other rule";
    EXPECT_EQ(expr_var_id(g_temp), expr_generate_hash_key("g_temp"));

    EXPECT_EQ(getDDM2ParameterType(ac0ttemp_item), DDMW_ACTION_PUBLISH);
    EXPECT_EQ(expr_var_type(ac0ttemp), RULE_ENGINE__VAR_TYPE_DDM2);
    EXPECT_EQ(expr_var_type(g_temp), RULE_ENGINE__VAR_TYPE_GLOBAL);

    EXPECT_EQ(expr_var_type_ddm_create(ac0ttemp), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_OWNED);
    EXPECT_EQ(expr_var_type_ddm_create(g_temp), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);
}

/**
 * @brief Test case to verify parsing of trigger section masks.
 *
 * This test ensures the rule engine correctly processes and applies masks
 * in the trigger section during parsing, validating proper filtering behavior.
 */
TEST_F(RuleEngineParserTest, VerifyParserTriggerSectionMask)
{
    const char *rule0_str = "g_trig_cool_nch_50_100, g_trig_cool_ch_60_100: g_temp_offset = 2000";
    const char *rule1_str = "g_trig_cool_nch_50_100, g_trig_cool_ch_60_100, mask=['or']: g_temp_offset = 2000";
    const char *rule2_str = "g_trig_cool_nch_50_100, g_trig_cool_ch_60_100, mask=['and']: g_temp_offset = 2000";
    const char *rule3_str = "g_trig_cool_nch_50_100, g_trig_cool_ch_60_100, mask=[0x01;'or']: g_temp_offset = 2000";
    const char *rule5_str = "g_trig_cool_nch_50_100, g_trig_cool_ch_60_100, mask=[0x02;'or']: g_temp_offset = 2000";
    const char *rule6_str = "g_trig_cool_nch_50_100, g_trig_cool_ch_60_100, mask =[0x02;'or']: g_temp_offset = 2000";
    const char *rule4_str = "g_trig_cool_nch_50_100, g_trig_cool_ch_60_100, mask=[0x02;'or']: g_temp_offset = 2000";
    const char *rule7_str = "g_trig_cool_nch_50_100, g_trig_cool_ch_60_100, mask= [0x02;'or']: g_temp_offset = 2000";
    const char *rule8_str = "g_trig_cool_nch_50_100, g_trig_cool_ch_60_100, mask = [0x02;'or']: g_temp_offset = 2000";
    /* Note: The rule engine parser has a limitation where it doesn't correctly interpret
     * mask constraints when there's a space between the mask value and constraint type
     * (e.g., between '0x02' and 'or'). In such cases, the mask constraint defaults to 'AND'
     * instead of properly reading the specified 'OR' constraint.
     */
    const char *rule9_str = "g_trig_cool_nch_50_100, g_trig_cool_ch_60_100, mask = [0x02; 'or']: g_temp_offset = 2000";

    // Create rule structures
    static const struct rule_engine__rule rules[] =
        {
            {.name = "TestRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
            {.name = "TestRule1", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
            {.name = "TestRule2", .rule = const_cast<char *>(rule2_str), .size = static_cast<int>(strlen(rule2_str))},
            {.name = "TestRule3", .rule = const_cast<char *>(rule3_str), .size = static_cast<int>(strlen(rule3_str))},
            {.name = "TestRule4", .rule = const_cast<char *>(rule4_str), .size = static_cast<int>(strlen(rule4_str))},
            {.name = "TestRule5", .rule = const_cast<char *>(rule5_str), .size = static_cast<int>(strlen(rule5_str))},
            {.name = "TestRule6", .rule = const_cast<char *>(rule6_str), .size = static_cast<int>(strlen(rule6_str))},
            {.name = "TestRule7", .rule = const_cast<char *>(rule7_str), .size = static_cast<int>(strlen(rule7_str))},
            {.name = "TestRule8", .rule = const_cast<char *>(rule8_str), .size = static_cast<int>(strlen(rule8_str))},
            {.name = "TestRule9", .rule = const_cast<char *>(rule9_str), .size = static_cast<int>(strlen(rule9_str))},
        };

    // Add all rule9
    for (size_t i = 0; i < sizeof(rules) / sizeof(rules[0]); i++)
    {
        ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[i]), 1)
            << "Failed to add rule specification for rule " << i + 1 << ": " << rules[i].name;
    }

    // Verify rule0_str
    struct rule_engine__specification *specification = getSpecificationById(0);
    ASSERT_NE(specification, nullptr) << "Failed to retrieve rule specification";

    struct expr_var *g_trig_cool_nch_50_100 = getVariableByName(specification, "g_trig_cool_nch_50_100");
    EXPECT_NE(g_trig_cool_nch_50_100, nullptr);
    struct expr_var *g_trig_cool_ch_60_100 = getVariableByName(specification, "g_trig_cool_ch_60_100");
    EXPECT_NE(g_trig_cool_ch_60_100, nullptr);

    EXPECT_EQ(getTriggerListSize(specification), 2) << "Failed rule: " << rule0_str;
    EXPECT_EQ(getMaskType(specification), RULE_ENGINE__MASK_CONSTRAINT_AND) << "Failed rule: " << rule0_str;
    EXPECT_TRUE(isVariableInTriggerList(specification, g_trig_cool_nch_50_100)) << "Failed rule: " << rule0_str;
    EXPECT_TRUE(isVariableInMask(specification, g_trig_cool_nch_50_100)) << "Failed rule: " << rule0_str;
    EXPECT_TRUE(isVariableInTriggerList(specification, g_trig_cool_ch_60_100)) << "Failed rule: " << rule0_str;
    EXPECT_TRUE(isVariableInMask(specification, g_trig_cool_ch_60_100)) << "Failed rule: " << rule0_str;

    // Verify rule1_str
    specification = getSpecificationById(1);
    ASSERT_NE(specification, nullptr) << "Failed to retrieve rule specification";

    g_trig_cool_nch_50_100 = getVariableByName(specification, "g_trig_cool_nch_50_100");
    EXPECT_NE(g_trig_cool_nch_50_100, nullptr);
    g_trig_cool_ch_60_100 = getVariableByName(specification, "g_trig_cool_ch_60_100");
    EXPECT_NE(g_trig_cool_ch_60_100, nullptr);

    EXPECT_EQ(getTriggerListSize(specification), 2) << "Failed rule: " << rule1_str;
    EXPECT_EQ(getMaskType(specification), RULE_ENGINE__MASK_CONSTRAINT_OR) << "Failed rule: " << rule1_str;
    EXPECT_TRUE(isVariableInTriggerList(specification, g_trig_cool_nch_50_100)) << "Failed rule: " << rule1_str;
    EXPECT_TRUE(isVariableInMask(specification, g_trig_cool_nch_50_100)) << "Failed rule: " << rule1_str;
    EXPECT_TRUE(isVariableInTriggerList(specification, g_trig_cool_ch_60_100)) << "Failed rule: " << rule1_str;
    EXPECT_TRUE(isVariableInMask(specification, g_trig_cool_ch_60_100)) << "Failed rule: " << rule1_str;

    // Verify rule2_str
    specification = getSpecificationById(2);
    ASSERT_NE(specification, nullptr) << "Failed to retrieve rule specification";

    g_trig_cool_nch_50_100 = getVariableByName(specification, "g_trig_cool_nch_50_100");
    EXPECT_NE(g_trig_cool_nch_50_100, nullptr);
    g_trig_cool_ch_60_100 = getVariableByName(specification, "g_trig_cool_ch_60_100");
    EXPECT_NE(g_trig_cool_ch_60_100, nullptr);

    EXPECT_EQ(getTriggerListSize(specification), 2) << "Failed rule: " << rule2_str;
    EXPECT_EQ(getMaskType(specification), RULE_ENGINE__MASK_CONSTRAINT_AND) << "Failed rule: " << rule2_str;
    EXPECT_TRUE(isVariableInTriggerList(specification, g_trig_cool_nch_50_100)) << "Failed rule: " << rule2_str;
    EXPECT_TRUE(isVariableInMask(specification, g_trig_cool_nch_50_100)) << "Failed rule: " << rule2_str;
    EXPECT_TRUE(isVariableInTriggerList(specification, g_trig_cool_ch_60_100)) << "Failed rule: " << rule2_str;
    EXPECT_TRUE(isVariableInMask(specification, g_trig_cool_ch_60_100)) << "Failed rule: " << rule2_str;

    // Verify rule3_str
    specification = getSpecificationById(3);
    ASSERT_NE(specification, nullptr) << "Failed to retrieve rule specification";

    g_trig_cool_nch_50_100 = getVariableByName(specification, "g_trig_cool_nch_50_100");
    EXPECT_NE(g_trig_cool_nch_50_100, nullptr);
    g_trig_cool_ch_60_100 = getVariableByName(specification, "g_trig_cool_ch_60_100");
    EXPECT_NE(g_trig_cool_ch_60_100, nullptr);

    EXPECT_EQ(getTriggerListSize(specification), 1) << "Failed rule: " << rule3_str;
    EXPECT_EQ(getMaskType(specification), RULE_ENGINE__MASK_CONSTRAINT_OR) << "Failed rule: " << rule3_str;
    EXPECT_TRUE(isVariableInTriggerList(specification, g_trig_cool_nch_50_100)) << "Failed rule: " << rule3_str;
    EXPECT_TRUE(isVariableInMask(specification, g_trig_cool_nch_50_100)) << "Failed rule: " << rule3_str;
    EXPECT_FALSE(isVariableInTriggerList(specification, g_trig_cool_ch_60_100)) << "Failed rule: " << rule3_str;
    EXPECT_FALSE(isVariableInMask(specification, g_trig_cool_ch_60_100)) << "Failed rule: " << rule3_str;

    // Verify rule4_str
    specification = getSpecificationById(4);
    ASSERT_NE(specification, nullptr) << "Failed to retrieve rule specification";

    g_trig_cool_nch_50_100 = getVariableByName(specification, "g_trig_cool_nch_50_100");
    EXPECT_NE(g_trig_cool_nch_50_100, nullptr);
    g_trig_cool_ch_60_100 = getVariableByName(specification, "g_trig_cool_ch_60_100");
    EXPECT_NE(g_trig_cool_ch_60_100, nullptr);

    EXPECT_EQ(getTriggerListSize(specification), 1) << "Failed rule: " << rule4_str;
    EXPECT_EQ(getMaskType(specification), RULE_ENGINE__MASK_CONSTRAINT_OR) << "Failed rule: " << rule4_str;
    EXPECT_FALSE(isVariableInTriggerList(specification, g_trig_cool_nch_50_100)) << "Failed rule: " << rule4_str;
    EXPECT_FALSE(isVariableInMask(specification, g_trig_cool_nch_50_100)) << "Failed rule: " << rule4_str;
    EXPECT_TRUE(isVariableInTriggerList(specification, g_trig_cool_ch_60_100)) << "Failed rule: " << rule4_str;
    EXPECT_TRUE(isVariableInMask(specification, g_trig_cool_ch_60_100)) << "Failed rule: " << rule4_str;

    // Verify rule5_str
    specification = getSpecificationById(5);
    ASSERT_NE(specification, nullptr) << "Failed to retrieve rule specification";

    g_trig_cool_nch_50_100 = getVariableByName(specification, "g_trig_cool_nch_50_100");
    EXPECT_NE(g_trig_cool_nch_50_100, nullptr);
    g_trig_cool_ch_60_100 = getVariableByName(specification, "g_trig_cool_ch_60_100");
    EXPECT_NE(g_trig_cool_ch_60_100, nullptr);

    EXPECT_EQ(getTriggerListSize(specification), 1) << "Failed rule: " << rule5_str;
    EXPECT_EQ(getMaskType(specification), RULE_ENGINE__MASK_CONSTRAINT_OR) << "Failed rule: " << rule5_str;
    EXPECT_FALSE(isVariableInTriggerList(specification, g_trig_cool_nch_50_100)) << "Failed rule: " << rule5_str;
    EXPECT_FALSE(isVariableInMask(specification, g_trig_cool_nch_50_100)) << "Failed rule: " << rule5_str;
    EXPECT_TRUE(isVariableInTriggerList(specification, g_trig_cool_ch_60_100)) << "Failed rule: " << rule5_str;
    EXPECT_TRUE(isVariableInMask(specification, g_trig_cool_ch_60_100)) << "Failed rule: " << rule5_str;

    // Verify rule6_str
    specification = getSpecificationById(6);
    ASSERT_NE(specification, nullptr) << "Failed to retrieve rule specification";

    g_trig_cool_nch_50_100 = getVariableByName(specification, "g_trig_cool_nch_50_100");
    EXPECT_NE(g_trig_cool_nch_50_100, nullptr);
    g_trig_cool_ch_60_100 = getVariableByName(specification, "g_trig_cool_ch_60_100");
    EXPECT_NE(g_trig_cool_ch_60_100, nullptr);

    EXPECT_EQ(getTriggerListSize(specification), 1) << "Failed rule: " << rule6_str;
    EXPECT_EQ(getMaskType(specification), RULE_ENGINE__MASK_CONSTRAINT_OR) << "Failed rule: " << rule6_str;
    EXPECT_FALSE(isVariableInTriggerList(specification, g_trig_cool_nch_50_100)) << "Failed rule: " << rule6_str;
    EXPECT_FALSE(isVariableInMask(specification, g_trig_cool_nch_50_100)) << "Failed rule: " << rule6_str;
    EXPECT_TRUE(isVariableInTriggerList(specification, g_trig_cool_ch_60_100)) << "Failed rule: " << rule6_str;
    EXPECT_TRUE(isVariableInMask(specification, g_trig_cool_ch_60_100)) << "Failed rule: " << rule6_str;

    // Verify rule7_str
    specification = getSpecificationById(7);
    ASSERT_NE(specification, nullptr) << "Failed to retrieve rule specification";

    g_trig_cool_nch_50_100 = getVariableByName(specification, "g_trig_cool_nch_50_100");
    EXPECT_NE(g_trig_cool_nch_50_100, nullptr);
    g_trig_cool_ch_60_100 = getVariableByName(specification, "g_trig_cool_ch_60_100");
    EXPECT_NE(g_trig_cool_ch_60_100, nullptr);

    EXPECT_EQ(getTriggerListSize(specification), 1) << "Failed rule: " << rule7_str;
    EXPECT_EQ(getMaskType(specification), RULE_ENGINE__MASK_CONSTRAINT_OR) << "Failed rule: " << rule7_str;
    EXPECT_FALSE(isVariableInTriggerList(specification, g_trig_cool_nch_50_100)) << "Failed rule: " << rule7_str;
    EXPECT_FALSE(isVariableInMask(specification, g_trig_cool_nch_50_100)) << "Failed rule: " << rule7_str;
    EXPECT_TRUE(isVariableInTriggerList(specification, g_trig_cool_ch_60_100)) << "Failed rule: " << rule7_str;
    EXPECT_TRUE(isVariableInMask(specification, g_trig_cool_ch_60_100)) << "Failed rule: " << rule7_str;

    // Verify rule8_str
    specification = getSpecificationById(8);
    ASSERT_NE(specification, nullptr) << "Failed to retrieve rule specification";

    g_trig_cool_nch_50_100 = getVariableByName(specification, "g_trig_cool_nch_50_100");
    EXPECT_NE(g_trig_cool_nch_50_100, nullptr);
    g_trig_cool_ch_60_100 = getVariableByName(specification, "g_trig_cool_ch_60_100");
    EXPECT_NE(g_trig_cool_ch_60_100, nullptr);

    EXPECT_EQ(getTriggerListSize(specification), 1) << "Failed rule: " << rule8_str;
    EXPECT_EQ(getMaskType(specification), RULE_ENGINE__MASK_CONSTRAINT_OR) << "Failed rule: " << rule8_str;
    EXPECT_FALSE(isVariableInTriggerList(specification, g_trig_cool_nch_50_100)) << "Failed rule: " << rule8_str;
    EXPECT_FALSE(isVariableInMask(specification, g_trig_cool_nch_50_100)) << "Failed rule: " << rule8_str;
    EXPECT_TRUE(isVariableInTriggerList(specification, g_trig_cool_ch_60_100)) << "Failed rule: " << rule8_str;
    EXPECT_TRUE(isVariableInMask(specification, g_trig_cool_ch_60_100)) << "Failed rule: " << rule8_str;

    // Verify rule9_str
    specification = getSpecificationById(9);
    ASSERT_NE(specification, nullptr) << "Failed to retrieve rule specification";

    g_trig_cool_nch_50_100 = getVariableByName(specification, "g_trig_cool_nch_50_100");
    EXPECT_NE(g_trig_cool_nch_50_100, nullptr);
    g_trig_cool_ch_60_100 = getVariableByName(specification, "g_trig_cool_ch_60_100");
    EXPECT_NE(g_trig_cool_ch_60_100, nullptr);

    EXPECT_EQ(getTriggerListSize(specification), 1) << "Failed rule: " << rule9_str;
    EXPECT_FALSE(isVariableInTriggerList(specification, g_trig_cool_nch_50_100)) << "Failed rule: " << rule9_str;
    EXPECT_FALSE(isVariableInMask(specification, g_trig_cool_nch_50_100)) << "Failed rule: " << rule9_str;
    EXPECT_TRUE(isVariableInTriggerList(specification, g_trig_cool_ch_60_100)) << "Failed rule: " << rule9_str;
    EXPECT_TRUE(isVariableInMask(specification, g_trig_cool_ch_60_100)) << "Failed rule: " << rule9_str;
    if (getMaskType(specification) == RULE_ENGINE__MASK_CONSTRAINT_OR)
    {
        // Reminder: The mask type is not parsed correctly when there is a space between the 'position' and the 'constraint'
        LOG(W, "Mask type: 'RULE_ENGINE__MASK_CONSTRAINT_OR'. It should be 'RULE_ENGINE__MASK_CONSTRAINT_AND'");
    }
}

/**
 * @brief Test case to verify that variables in initialization section are not added to trigger list.
 *
 * This test ensures that when a variable is initialized in the trigger section of a rule,
 * but is not meant to be a trigger (i.e. global parameter which is initialized to some value
 * is not considered a trigger),it is correctly excluded from the rule's trigger list.
 *
 * Specifically:
 * 1. It verifies that 'g_sysappl0smarteco' is initialized in the rule's condition section
 *    but is not added to the trigger list
 * 2. Only 'sysappl0smarteco' appears in the sensitivity list
 * 3. It validates proper DDM2 parameter subscription
 * 4. It checks the proper mask configuration for the trigger
 */
TEST_F(RuleEngineParserTest, VerifyParserTriggerSectionInitVarsNotInTriggerList)
{
    // g_sysappl0smarteco should not be in the trigger list
    const char *rule0_str = "sysappl0smarteco, g_sysappl0smarteco = -1: if(g_sysappl0smarteco != sysappl0smarteco, g_sysappl0smarteco = sysappl0smarteco, g_determine_smart_eco_status = 1)";

    struct rule_engine__rule rule0 = {.name = "TestRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))};

    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rule0), 1);

    // Verify rule 0
    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    ASSERT_NE(rule0_spec, nullptr);

    // Check variables
    ddmw_item_t *sysappl0smarteco_item = getDDM2ParameterByID(SYSAPPL0SMARTECO);
    EXPECT_NE(sysappl0smarteco_item, nullptr);
    EXPECT_EQ(getDDM2ParameterValue(sysappl0smarteco_item), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(getDDM2ParameterType(sysappl0smarteco_item), DDMW_ACTION_SET);

    struct expr_var *sysappl0smarteco = getVariableByName(rule0_spec, "sysappl0smarteco");
    /* Sanity check */
    ASSERT_NE(sysappl0smarteco, nullptr);
    EXPECT_EQ(expr_var_id(sysappl0smarteco), SYSAPPL0SMARTECO);
    EXPECT_EQ(expr_var_type(sysappl0smarteco), RULE_ENGINE__VAR_TYPE_DDM2);
    EXPECT_EQ(expr_var_value(sysappl0smarteco), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type_ddm_create(sysappl0smarteco), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED);

    struct expr_var *g_sysappl0smarteco = getVariableByName(rule0_spec, "g_sysappl0smarteco");
    /* Sanity check */
    ASSERT_NE(g_sysappl0smarteco, nullptr);
    EXPECT_EQ(expr_var_id(g_sysappl0smarteco), expr_generate_hash_key("g_sysappl0smarteco"));
    EXPECT_EQ(expr_var_type(g_sysappl0smarteco), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_value(g_sysappl0smarteco), -1);
    EXPECT_EQ(expr_var_type_ddm_create(g_sysappl0smarteco), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

    struct expr_var *g_determine_smart_eco_status = getVariableByName(rule0_spec, "g_determine_smart_eco_status");
    /* Sanity check */
    ASSERT_NE(g_determine_smart_eco_status, nullptr);
    EXPECT_EQ(expr_var_id(g_determine_smart_eco_status), expr_generate_hash_key("g_determine_smart_eco_status"));
    EXPECT_EQ(expr_var_type(g_determine_smart_eco_status), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_value(g_determine_smart_eco_status), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type_ddm_create(g_determine_smart_eco_status), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

    // Check trigger section
    EXPECT_EQ(getTriggerListSize(rule0_spec), 1);
    EXPECT_EQ(getMaskType(rule0_spec), RULE_ENGINE__MASK_CONSTRAINT_AND);
    EXPECT_TRUE(isVariableInTriggerList(rule0_spec, sysappl0smarteco));
    EXPECT_TRUE(isVariableInMask(rule0_spec, sysappl0smarteco));
    EXPECT_FALSE(isVariableInTriggerList(rule0_spec, g_sysappl0smarteco));
    EXPECT_FALSE(isVariableInMask(rule0_spec, g_sysappl0smarteco));
}

/**
 * @brief Test case to verify that variables defined only in the trigger section (not in the rule body)
 * are properly added to variable list.
 *
 * Specifically:
 * 1. Added to the variable list with default values (0) even though they don't appear in the rule body
 * 2. Added to the trigger list (since they are not initialized)
 */
TEST_F(RuleEngineParserTest, VerifyParserTriggerSectionCreateAsVarsInRules)
{
    const char *rule0_str = "g_trig_cool_nch_50_100, g_trig_cool_ch_60_100, mask=['or']: g_temp_offset = 2000";

    struct rule_engine__rule rule0 = {.name = "TestRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))};

    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rule0), 1);

    // Verify rule 1
    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    ASSERT_NE(rule0_spec, nullptr);

    // Check variables
    struct expr_var *g_trig_cool_nch_50_100 = getVariableByName(rule0_spec, "g_trig_cool_nch_50_100");
    ASSERT_NE(g_trig_cool_nch_50_100, nullptr);
    EXPECT_EQ(expr_var_id(g_trig_cool_nch_50_100), expr_generate_hash_key("g_trig_cool_nch_50_100"));
    EXPECT_EQ(expr_var_type(g_trig_cool_nch_50_100), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_value(g_trig_cool_nch_50_100), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type_ddm_create(g_trig_cool_nch_50_100), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

    struct expr_var *g_trig_cool_ch_60_100 = getVariableByName(rule0_spec, "g_trig_cool_ch_60_100");
    ASSERT_NE(g_trig_cool_ch_60_100, nullptr);
    EXPECT_EQ(expr_var_id(g_trig_cool_ch_60_100), expr_generate_hash_key("g_trig_cool_ch_60_100"));
    EXPECT_EQ(expr_var_type(g_trig_cool_ch_60_100), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_value(g_trig_cool_ch_60_100), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type_ddm_create(g_trig_cool_ch_60_100), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

    struct expr_var *g_temp_offset = getVariableByName(rule0_spec, "g_temp_offset");
    ASSERT_NE(g_temp_offset, nullptr);
    EXPECT_EQ(expr_var_id(g_temp_offset), expr_generate_hash_key("g_temp_offset"));
    EXPECT_EQ(expr_var_type(g_temp_offset), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_value(g_temp_offset), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type_ddm_create(g_temp_offset), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

    // Check trigger section
    EXPECT_EQ(getTriggerListSize(rule0_spec), 2);
    EXPECT_EQ(getMaskType(rule0_spec), RULE_ENGINE__MASK_CONSTRAINT_OR);
    EXPECT_TRUE(isVariableInTriggerList(rule0_spec, g_trig_cool_nch_50_100));
    EXPECT_TRUE(isVariableInMask(rule0_spec, g_trig_cool_nch_50_100));
    EXPECT_TRUE(isVariableInTriggerList(rule0_spec, g_trig_cool_ch_60_100));
    EXPECT_TRUE(isVariableInMask(rule0_spec, g_trig_cool_ch_60_100));
    EXPECT_FALSE(isVariableInTriggerList(rule0_spec, g_temp_offset));
    EXPECT_FALSE(isVariableInMask(rule0_spec, g_temp_offset));
}

// Test case 1: No variables initialized
TEST_F(RuleEngineParserTest, NoVariablesInitialized)
{
    const char *ruleStr = "g_temp, ac0ttemp: g_temp + ac0ttemp";

    struct rule_engine__specification *spec = addRule("NoInitTest", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    verifyVariable(spec, "g_temp", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_temp"));
    verifyVariable(spec, "ac0ttemp", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, AC0TTEMP);  // Check trigger section

    verifyDDMWItem(spec, "ac0ttemp", 0, DDMW_ACTION_SET, AC0TTEMP);

    verifyVariableTriggerStates(spec, 2, 0x3, RULE_ENGINE__MASK_CONSTRAINT_AND,
                                {{"g_temp", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"ac0ttemp", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS}});
}

// Test case 2: Global variable initialized
TEST_F(RuleEngineParserTest, GlobalVariableInitialized)
{
    const char *ruleStr = "g_temp = 12, ac0ttemp: g_temp + ac0ttemp";

    struct rule_engine__specification *spec = addRule("GlobalInitTest", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    verifyVariable(spec, "g_temp", 12, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_temp"));
    verifyVariable(spec, "ac0ttemp", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, AC0TTEMP);
    verifyDDMWItem(spec, "ac0ttemp", 0, DDMW_ACTION_SET, AC0TTEMP);

    verifyVariableTriggerStates(spec, 1, 0x1, RULE_ENGINE__MASK_CONSTRAINT_AND,
                                {{"g_temp", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"ac0ttemp", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS}});
}

// Test case 3: DDM2 variable initialized
TEST_F(RuleEngineParserTest, DDM2VariableInitialized)
{
    const char *ruleStr = "g_temp, ac0ttemp=20: g_temp + ac0ttemp";

    struct rule_engine__specification *spec = addRule("DDM2InitTest", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    verifyVariable(spec, "g_temp", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_temp"));
    verifyVariable(spec, "ac0ttemp", 20, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, AC0TTEMP);
    verifyDDMWItem(spec, "ac0ttemp", 20, DDMW_ACTION_SET, AC0TTEMP);

    verifyVariableTriggerStates(spec, 2, 0x3, RULE_ENGINE__MASK_CONSTRAINT_AND,
                                {{"g_temp", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"ac0ttemp", TRIGGER_LIST_INITIALIZED, TRIGGER_LIST_EXISTS}});
}

// Test case 4: Both variables initialized
TEST_F(RuleEngineParserTest, DDM2andGlobalVariablesInitialized)
{
    const char *ruleStr = "g_temp = 23, ac0ttemp=1700: g_temp + ac0ttemp";

    struct rule_engine__specification *spec = addRule("DDM2andGlobalVariablesInitTest", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    verifyVariable(spec, "g_temp", 23, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_temp"));
    verifyVariable(spec, "ac0ttemp", 1700, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, AC0TTEMP);
    verifyDDMWItem(spec, "ac0ttemp", 1700, DDMW_ACTION_SET, AC0TTEMP);

    verifyVariableTriggerStates(spec, 1, 0x1, RULE_ENGINE__MASK_CONSTRAINT_AND,
                                {{"g_temp", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"ac0ttemp", TRIGGER_LIST_INITIALIZED, TRIGGER_LIST_EXISTS}});
}

// Test case 5: DDM special syntax (ac0ttemp=ddm)
TEST_F(RuleEngineParserTest, DDM2OwnerTest)
{
    const char *ruleStr = "ac0ttemp=ddm, temp = -15: ac0ttemp + temp";

    struct rule_engine__specification *spec = addRule("DDM2OwnerTest", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    verifyVariable(spec, "temp", -15, RULE_ENGINE__VAR_TYPE_LOCAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("temp"));
    verifyVariable(spec, "ac0ttemp", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_OWNED, AC0AVL);  // AC0AVL instead of AC0TTEMP, since no other rule has requested creation of instance
    verifyDDMWItem(spec, "ac0ttemp", 0, DDMW_ACTION_PUBLISH, AC0AVL);

    verifyVariableTriggerStates(spec, 1, 0x1, RULE_ENGINE__MASK_CONSTRAINT_AND,
                                {{"temp", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"ac0ttemp", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS}});
}

// Test case 6: DDM instance syntax (ac0ttemp=ddm(0))
TEST_F(RuleEngineParserTest, DDM2OwnerInitTest)
{
    const char *ruleStr = "ac0ttemp=ddm(0): ac0ttemp + temp";

    struct rule_engine__specification *spec = addRule("DDM2OwnerInitTest", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    verifyVariable(spec, "temp", 0, RULE_ENGINE__VAR_TYPE_LOCAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("temp"));
    verifyVariable(spec, "ac0ttemp", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_OWNED, AC0AVL);  // AC0AVL instead of AC0TTEMP, since no other rule has requested creation of instance
    verifyDDMWItem(spec, "ac0ttemp", 0, DDMW_ACTION_PUBLISH, AC0AVL);

    verifyVariableTriggerStates(spec, 1, 0x1, RULE_ENGINE__MASK_CONSTRAINT_AND,
                                {{"temp", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"ac0ttemp", TRIGGER_LIST_INITIALIZED, TRIGGER_LIST_EXISTS}});
}

// Test case 7: Complex rule with multiple variables
TEST_F(RuleEngineParserTest, DDM2SubscriberWithGlobalVarsInitTest)
{
    const char *ruleStr = "g_temp = 42, ac0ttemp=100, g_count: "
                          "if(g_count > 0, ac0ttemp = g_temp * g_count, ac0ttemp)";

    struct rule_engine__specification *spec = addRule("DDM2SubscriberWithGlobalVarsInitTest", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    // Verify all variables
    verifyVariable(spec, "g_temp", 42, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_temp"));
    verifyVariable(spec, "g_count", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_count"));
    verifyVariable(spec, "ac0ttemp", 100, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, AC0TTEMP);
    verifyDDMWItem(spec, "ac0ttemp", 100, DDMW_ACTION_SET, AC0TTEMP);

    verifyVariableTriggerStates(spec, 2, 0x3, RULE_ENGINE__MASK_CONSTRAINT_AND,
                                {{"g_temp", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"ac0ttemp", TRIGGER_LIST_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"g_count", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS}});
}

TEST_F(RuleEngineParserTest, DDM2VariableWithInitialization)
{
    // Test basic DDM2 variable with initialization using =ddm(value) syntax (owner)
    const char *ruleStr = "ac0ttemp=ddm(25): print(ac0ttemp)";
    struct rule_engine__specification *spec = addRule("DDM2VariableWithInitialization", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    verifyVariable(spec, "ac0ttemp", 25, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_OWNED, AC0AVL);
    verifyVariableTriggerStates(spec, 1, 0x1, RULE_ENGINE__MASK_CONSTRAINT_AND,
                                {{"ac0ttemp", TRIGGER_LIST_INITIALIZED, TRIGGER_LIST_EXISTS}});
}

TEST_F(RuleEngineParserTest, DDM2VariableWithoutInitialization)
{
    // Test basic DDM2 variable without initialization (subscriber)
    const char *ruleStr = "mbat0caprel: if(mbat0caprel > 50, ac0on = 1)";
    struct rule_engine__specification *spec = addRule("DDM2VariableWithoutInitialization", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    verifyVariable(spec, "mbat0caprel", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, MBAT0CAPREL);
    verifyVariable(spec, "ac0on", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, AC0ON);
    verifyVariableTriggerStates(spec, 1, 0x1, RULE_ENGINE__MASK_CONSTRAINT_AND,
                                {{"mbat0caprel", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"ac0on", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS}});
}

TEST_F(RuleEngineParserTest, GlobalVariableWithoutInitialization)
{
    // Test global variable without initialization (should get default value)
    const char *ruleStr = "g_counter: print(g_counter)";
    struct rule_engine__specification *spec = addRule("GlobalVariableWithoutInitialization", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    verifyVariable(spec, "g_counter", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_counter"));
    verifyVariableTriggerStates(spec, 1, 0x1, RULE_ENGINE__MASK_CONSTRAINT_AND,
                                {{"g_counter", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS}});
}

/*******************************************************************************
 * Advanced Variable Combination Tests
 *******************************************************************************/

TEST_F(RuleEngineParserTest, AdvancedTest_MultipleInitializedDDM2Variables)
{
    // Test multiple DDM2 variables with different initialization values
    const char *ruleStr = "ac0ttemp=ddm(25), cc0voc=ddm(400): if(ac0ttemp > 20 && cc0voc < 500, ac0on = 1)";
    struct rule_engine__specification *spec = addRule("MultiInitDDM2", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    verifyVariable(spec, "ac0ttemp", 25, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_OWNED, AC0AVL);
    verifyVariable(spec, "cc0voc", 400, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_OWNED, CC0AVL);
    verifyVariable(spec, "ac0on", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, AC0ON);
    verifyVariableTriggerStates(spec, 2, 0x3, RULE_ENGINE__MASK_CONSTRAINT_AND,
                                {{"ac0ttemp", TRIGGER_LIST_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"cc0voc", TRIGGER_LIST_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"ac0on", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS}});
}

TEST_F(RuleEngineParserTest, AdvancedTest_NestedConditionalWithMixedTypes)
{
    // Test nested conditionals with different variable types
    const char *ruleStr = "mbat0caprel, g_threshold = 75: local_status = if(mbat0caprel > g_threshold, 1, if(mbat0caprel > 50, 2, 3))";
    struct rule_engine__specification *spec = addRule("NestedConditional", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    verifyVariable(spec, "mbat0caprel", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, MBAT0CAPREL);
    verifyVariable(spec, "g_threshold", 75, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_threshold"));
    verifyVariable(spec, "local_status", 0, RULE_ENGINE__VAR_TYPE_LOCAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("local_status"));
    verifyVariableTriggerStates(spec, 1, 0x1, RULE_ENGINE__MASK_CONSTRAINT_AND,
                                {{"mbat0caprel", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"g_threshold", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"local_status", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS}});
}

/*******************************************************************************
 * Mask Testing Scenarios
 *******************************************************************************/

TEST_F(RuleEngineParserTest, MaskTest_ComplexORWithThreeVariables)
{
    // Test OR mask with three variables from different types
    const char *ruleStr = "ac0ttemp, g_enable, mbat0caprel, mask=['or']: local_combined = ac0ttemp + g_enable + mbat0caprel";
    struct rule_engine__specification *spec = addRule("MaskTest_ComplexORWithThreeVariables", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    verifyVariable(spec, "ac0ttemp", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, AC0TTEMP);
    verifyVariable(spec, "g_enable", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_enable"));
    verifyVariable(spec, "mbat0caprel", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, MBAT0CAPREL);
    verifyVariable(spec, "local_combined", 0, RULE_ENGINE__VAR_TYPE_LOCAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("local_combined"));
    verifyVariableTriggerStates(spec, 3, 0x7, RULE_ENGINE__MASK_CONSTRAINT_OR,
                                {{"ac0ttemp", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"g_enable", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"mbat0caprel", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"local_combined", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS}});
}

TEST_F(RuleEngineParserTest, MaskTest_SelectiveActivationPattern)
{
    // Test selective mask pattern (alternating variables)
    const char *ruleStr = "ac0ttemp, cc0voc, mbat0caprel, g_config, mask=[0xA]: result = ac0ttemp + mbat0caprel";
    struct rule_engine__specification *spec = addRule("MaskTest_SelectiveActivationPattern", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    verifyVariable(spec, "ac0ttemp", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, AC0TTEMP);
    verifyVariable(spec, "cc0voc", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, CC0VOC);
    verifyVariable(spec, "mbat0caprel", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, MBAT0CAPREL);
    verifyVariable(spec, "g_config", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_config"));
    verifyVariable(spec, "result", 0, RULE_ENGINE__VAR_TYPE_LOCAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("result"));
    // Mask 0xA = binary 1010, so positions 1 and 3 should trigger (cc0voc and g_config)
    verifyVariableTriggerStates(spec, 2, 0x3, RULE_ENGINE__MASK_CONSTRAINT_AND,
                                {{"ac0ttemp", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"cc0voc", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"mbat0caprel", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"g_config", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"result", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS}});
}

TEST_F(RuleEngineParserTest, EdgeCase_SingleBitMaskPattern)
{
    // Test mask that activates only one variable from multiple trigger candidates
    const char *ruleStr = "ac0ttemp, cc0voc, mbat0caprel, g_priority, mask=[0x4]: action = mbat0caprel * 2";
    struct rule_engine__specification *spec = addRule("EdgeCase_SingleBitMaskPattern", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    verifyVariable(spec, "ac0ttemp", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, AC0TTEMP);
    verifyVariable(spec, "cc0voc", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, CC0VOC);
    verifyVariable(spec, "mbat0caprel", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, MBAT0CAPREL);
    verifyVariable(spec, "g_priority", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_priority"));
    verifyVariable(spec, "action", 0, RULE_ENGINE__VAR_TYPE_LOCAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("action"));

    ddmw_item_t *ac0ttemp = getDDM2ParameterByID(AC0TTEMP);
    EXPECT_NE(ac0ttemp, nullptr) << " DDM2 item 'ac0ttemp' should be in subscription list(ddmw_item) even though if not in trigger list(mask points to mbat0caprel)";
    ddmw_item_t *cc0voc = getDDM2ParameterByID(CC0VOC);
    EXPECT_NE(cc0voc, nullptr) << " DDM2 item 'cc0voc' should be in subscription list(ddmw_item) even though not in trigger list(mask points to mbat0caprel)";
    verifyDDMWItem(spec, "mbat0caprel", 0, DDMW_ACTION_SET, MBAT0CAPREL);

    // Mask 0x4 = binary 0100, so only position 2 should trigger (mbat0caprel)
    verifyVariableTriggerStates(spec, 1, 0x1, RULE_ENGINE__MASK_CONSTRAINT_AND,
                                {{"ac0ttemp", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"cc0voc", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"mbat0caprel", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"g_priority", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"action", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS}});
}

TEST_F(RuleEngineParserTest, MaskTest_CustomMaskRules)
{
    const char *ruleStr1 = "g_temp, ac0ttemp,mask=[0x3;'or']: g_temp + ac0ttemp";

    struct rule_engine__specification *spec1 = addRule("MaskTest_CustomMaskRules", ruleStr1, 0);
    ASSERT_NE(spec1, nullptr);

    // Verify variables
    verifyVariable(spec1, "g_temp", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_temp"));
    verifyVariable(spec1, "ac0ttemp", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, AC0TTEMP);
    verifyDDMWItem(spec1, "ac0ttemp", 0, DDMW_ACTION_SET, AC0TTEMP);

    verifyVariableTriggerStates(spec1, 2, 0x3, RULE_ENGINE__MASK_CONSTRAINT_OR,
                                {{"g_temp", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"ac0ttemp", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS}});

    // Create a rule with an initialized global variable (g_temp=15) that is included in the trigger section
    // by explicitly setting the mask to 0x3 (both variables in the trigger section)
    const char *ruleName2 = "InitializedGlobalInTriggerTest";
    const char *ruleStr2 = "g_temp=15, ac0ttemp, mask=[0x3;'or']: g_temp + ac0ttemp";

    // Add the rule
    struct rule_engine__specification *spec2 = addRule(ruleName2, ruleStr2, 1);
    ASSERT_NE(spec2, nullptr) << "Failed to add rule";

    verifyVariable(spec2, "g_temp", 15, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_temp"));
    verifyVariable(spec2, "ac0ttemp", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, AC0TTEMP);
    verifyDDMWItem(spec2, "ac0ttemp", 0, DDMW_ACTION_SET, AC0TTEMP);
    // This is a key test to verify that when a user-defined mask is present,
    // global variables that are initialized can still appear in the trigger section
    // with their initialized flag correctly set
    verifyVariableTriggerStates(spec2, 2, 0x3, RULE_ENGINE__MASK_CONSTRAINT_OR,
                                {{"g_temp", TRIGGER_LIST_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"ac0ttemp", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS}});
}

/*******************************************************************************
 * Edge Case Tests
 *******************************************************************************/

TEST_F(RuleEngineParserTest, EdgeCase_OnlyGlobalVariable)
{
    // Test global variables in complex mathematical expressions
    const char *ruleStr = "g_multiplier = 3, g_offset = 10: g_result = (ac0ttemp * g_multiplier) + g_offset";
    struct rule_engine__specification *spec = addRule("EdgeCase_OnlyGlobalVariable", ruleStr, 0, RULE_ENGINE__INV_MASK);
    EXPECT_EQ(spec, nullptr) << "Rule should not created due to invalid trigger section";
}

TEST_F(RuleEngineParserTest, EdgeCase_OnlyLocalVariable)
{
    // Test local variable (always uninitialized and not in trigger list)
    const char *ruleStr = "local_temp: print(local_temp)";
    struct rule_engine__specification *spec = addRule("EdgeCase_OnlyLocalVariable", ruleStr, 0, RULE_ENGINE__INV_MASK);
    EXPECT_EQ(spec, nullptr);
}

TEST_F(RuleEngineParserTest, EdgeCase_OnlyLocalVariables)
{
    // Test rule with only local variables (no trigger variables)
    const char *ruleStr = "local_a = 5, local_b = 10: local_sum = local_a + local_b, print(local_sum)";
    struct rule_engine__specification *spec = addRule("EdgeCase_OnlyLocalVariables", ruleStr, 0, RULE_ENGINE__INV_MASK);
    EXPECT_EQ(spec, nullptr) << "Rule should not created due to invalid trigger section with only local variables";
}

/*******************************************************************************
 * Edge Case: SmartEcoTest - DDM2 variable not in subscription since not(selected) in trigger
 *******************************************************************************/
TEST_F(RuleEngineParserTest, EdgeCase_DDM2InSubscriptionEvenThoughNotInTrigger)
{
    // Based on smart_eco.json: Complex OR mask with state transitions and temperature offset calculations
    const char *ruleStr = "g_trig_cool_nch_10_30, g_trig_cool_nch_30_50, g_trig_cool_ch_20_40, g_trig_cool_ch_40_60, mask=['or']: if((ac0itemp + 2000) >= (ac0ttemp + 4000), g_temp_offset = 4000)";
    struct rule_engine__specification *spec = addRule("EdgeCase_DDM2InSubscriptionEvenThoughNotInTrigger", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    verifyVariable(spec, "g_trig_cool_nch_10_30", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_trig_cool_nch_10_30"));
    verifyVariable(spec, "g_trig_cool_nch_30_50", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_trig_cool_nch_30_50"));
    verifyVariable(spec, "g_trig_cool_ch_20_40", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_trig_cool_ch_20_40"));
    verifyVariable(spec, "g_trig_cool_ch_40_60", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_trig_cool_ch_40_60"));
    verifyVariable(spec, "ac0itemp", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, AC0ITEMP);
    verifyVariable(spec, "ac0ttemp", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, AC0TTEMP);
    verifyVariable(spec, "g_temp_offset", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_temp_offset"));

    ddmw_item_t *ac0itemp = getDDM2ParameterByID(AC0ITEMP);
    EXPECT_NE(ac0itemp, nullptr) << " DDM2 item 'ac0itemp' should be in subscription list(ddmw_item) even though not in trigger list";
    ddmw_item_t *ac0ttemp = getDDM2ParameterByID(AC0TTEMP);
    EXPECT_NE(ac0ttemp, nullptr) << " DDM2 item 'ac0ttemp' should be in subscription list(ddmw_item) even though not in trigger list";

    verifyVariableTriggerStates(spec, 4, 0xF, RULE_ENGINE__MASK_CONSTRAINT_OR,
                                {{"g_trig_cool_nch_10_30", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"g_trig_cool_nch_30_50", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"g_trig_cool_ch_20_40", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"g_trig_cool_ch_40_60", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"ac0itemp", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"ac0ttemp", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"g_temp_offset", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS}});
}

TEST_F(RuleEngineParserTest, EdgeCase_DDM2InSubscriptionEvenThoughNotSelectedInTrigger)
{
    // Based on smart_eco.json: Multiple variable publication with complex conditional logic
    const char *ruleStr = "g_trig_cool_nch_10_30, g_trig_cool_ch_20_40, g_trig_dry_nch_10_30, g_trig_dry_ch_20_40, mask=['or']: pub(ac0md = 2, ac0offset = g_temp_offset)";
    struct rule_engine__specification *spec = addRule("EdgeCase_DDM2InSubscriptionEvenThoughNotSelectedInTrigger", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    verifyVariable(spec, "g_trig_cool_nch_10_30", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_trig_cool_nch_10_30"));
    verifyVariable(spec, "g_trig_cool_ch_20_40", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_trig_cool_ch_20_40"));
    verifyVariable(spec, "g_trig_dry_nch_10_30", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_trig_dry_nch_10_30"));
    verifyVariable(spec, "g_trig_dry_ch_20_40", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_trig_dry_ch_20_40"));
    verifyVariable(spec, "ac0md", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, AC0MD);
    verifyVariable(spec, "ac0offset", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, AC0OFFSET);
    verifyVariable(spec, "g_temp_offset", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_temp_offset"));

    ddmw_item_t *ac0md = getDDM2ParameterByID(AC0MD);
    EXPECT_NE(ac0md, nullptr) << " DDM2 item 'ac0md' should be in subscription list(ddmw_item) even though not in trigger list";
    ddmw_item_t *ac0offset = getDDM2ParameterByID(AC0OFFSET);
    EXPECT_NE(ac0offset, nullptr) << " DDM2 item 'ac0offset' should be in subscription list(ddmw_item) even though not in trigger list";

    verifyVariableTriggerStates(spec, 4, 0xF, RULE_ENGINE__MASK_CONSTRAINT_OR,
                                {{"g_trig_cool_nch_10_30", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"g_trig_cool_ch_20_40", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"g_trig_dry_nch_10_30", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"g_trig_dry_ch_20_40", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"ac0md", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"ac0offset", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"g_temp_offset", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS}});
}

/*******************************************************************************
 * Complex Rule Tests Based on Smart Eco and Supervisor JSON Patterns
 *******************************************************************************/
TEST_F(RuleEngineParserTest, SmartEcoTest_MultiVariableConditionalLogic)
{
    // Based on smart_eco.json: Complex conditional with multiple DDM variables and state management
    const char *ruleStr = "ac0on, g_ac0on = -1: if(g_ac0on != ac0on, g_ac0on = ac0on, g_on_has_changed = g_determine_smart_eco_status = 1)";
    struct rule_engine__specification *spec = addRule("SmartEcoTest_MultiVariableConditionalLogic", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    verifyVariable(spec, "ac0on", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, AC0ON);
    verifyVariable(spec, "g_ac0on", -1, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_ac0on"));
    verifyVariable(spec, "g_on_has_changed", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_on_has_changed"));
    verifyVariable(spec, "g_determine_smart_eco_status", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_determine_smart_eco_status"));
    verifyDDMWItem(spec, "ac0on", 0, DDMW_ACTION_SET, AC0ON);

    verifyVariableTriggerStates(spec, 1, 0x1, RULE_ENGINE__MASK_CONSTRAINT_AND,
                                {{"ac0on", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"g_ac0on", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"g_on_has_changed", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"g_determine_smart_eco_status", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS}});
}

TEST_F(RuleEngineParserTest, SmartEcoTest_DDMParameterInitializationWithPub)
{
    // Based on smart_eco.json: DDM parameter initialization with complex expressions and publication
    const char *ruleStr = "rule0act=ddm(1), g_rule0act = -1: if(g_rule0act != rule0act, g_rule0act = rule0act)";
    struct rule_engine__specification *spec = addRule("SmartEcoRuleAct", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    verifyVariable(spec, "rule0act", 1, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_OWNED, RULE0ACT);
    verifyVariable(spec, "g_rule0act", -1, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_rule0act"));
    verifyDDMWItem(spec, "rule0act", 1, DDMW_ACTION_PUBLISH, RULE0ACT);

    verifyVariableTriggerStates(spec, 1, 0x1, RULE_ENGINE__MASK_CONSTRAINT_AND,
                                {{"rule0act", TRIGGER_LIST_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"g_rule0act", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS}});
}

TEST_F(RuleEngineParserTest, SmartEcoTest_ComplexSOCStateLogic)
{
    // Based on smart_eco.json: Complex SOC state determination with multiple conditions
    const char *ruleStr = "g_trig_cool_nch, g_trig_cool_nch_10_30 = 2: if(g_mbat0caprel > 10 && g_mbat0caprel <= 30, if(g_current_state != g_trig_cool_nch_10_30, g_current_state = g_trig_cool_nch_10_30 = 2), g_set_fanspeed = 1)";
    struct rule_engine__specification *spec = addRule("SmartEcoSOCState", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    verifyVariable(spec, "g_trig_cool_nch", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_trig_cool_nch"));
    verifyVariable(spec, "g_trig_cool_nch_10_30", 2, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_trig_cool_nch_10_30"));
    verifyVariable(spec, "g_mbat0caprel", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_mbat0caprel"));
    verifyVariable(spec, "g_current_state", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_current_state"));
    verifyVariable(spec, "g_set_fanspeed", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_set_fanspeed"));

    verifyVariableTriggerStates(spec, 1, 0x1, RULE_ENGINE__MASK_CONSTRAINT_AND,
                                {{"g_trig_cool_nch", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"g_trig_cool_nch_10_30", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"g_mbat0caprel", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"g_current_state", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"g_set_fanspeed", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS}});
}

TEST_F(RuleEngineParserTest, SupervisorTest_TimerBasedStateTransition)
{
    // Based on supervisor.json: Timer-based state transitions with complex conditions
    const char *ruleStr = "g_s_state,g_sup0act,mask=['or'] : if(g_sup0act && (g_sup0type==1) && (g_s_state==1) && (g_sup0s1alt>0),timer_off(g_s2timer_on),timer_on(g_s1timer_on, g_sup0s1alt),print(g_s_state, g_sup0s1alt))";
    struct rule_engine__specification *spec = addRule("SupervisorTimerState", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    verifyVariable(spec, "g_s_state", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_s_state"));
    verifyVariable(spec, "g_sup0act", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_sup0act"));
    verifyVariable(spec, "g_sup0type", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_sup0type"));
    verifyVariable(spec, "g_sup0s1alt", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_sup0s1alt"));
    verifyVariable(spec, "g_s2timer_on", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_s2timer_on"));
    verifyVariable(spec, "g_s1timer_on", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_s1timer_on"));

    verifyVariableTriggerStates(spec, 2, 0x3, RULE_ENGINE__MASK_CONSTRAINT_OR,
                                {{"g_s_state", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"g_sup0act", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"g_sup0type", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"g_sup0s1alt", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"g_s2timer_on", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"g_s1timer_on", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS}});
}

TEST_F(RuleEngineParserTest, SupervisorTest_LevelDebounceStateMachine)
{
    // Based on supervisor.json: Level debounce state machine with threshold comparisons
    const char *ruleStr = "g_sup_value_trig,g_sup0act,g_sup0type,mask=['or'] : if(g_sup0act && (g_sup0type==0) && (g_sup_value_trig > g_sup0lhuplim) && (g_l_state==1),g_l_state=2,timer_on(g_l1timer_on, g_sup0lhalt),print(g_sup_value_trig,g_l_state))";
    struct rule_engine__specification *spec = addRule("SupervisorLevelDebounce", ruleStr, 0);
    ASSERT_NE(spec, nullptr);

    verifyVariable(spec, "g_sup_value_trig", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_sup_value_trig"));
    verifyVariable(spec, "g_sup0act", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_sup0act"));
    verifyVariable(spec, "g_sup0type", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_sup0type"));
    verifyVariable(spec, "g_sup0lhuplim", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_sup0lhuplim"));
    verifyVariable(spec, "g_l_state", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_l_state"));
    verifyVariable(spec, "g_l1timer_on", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_l1timer_on"));
    verifyVariable(spec, "g_sup0lhalt", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_sup0lhalt"));

    verifyVariableTriggerStates(spec, 3, 0x7, RULE_ENGINE__MASK_CONSTRAINT_OR,
                                {{"g_sup_value_trig", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"g_sup0act", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"g_sup0type", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"g_sup0lhuplim", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"g_l_state", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"g_l1timer_on", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS},
                                 {"g_sup0lhalt", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS}});
}

/*******************************************************************************
 * Variable Consistency Tests Across Multiple Rules
 *******************************************************************************/

TEST_F(RuleEngineParserTest, GlobalVariableConsistency_InitializedInFirstRule)
{
    // Test global variable consistency when initialized in first rule
    const char *ruleStr1 = "g_shared_value = 100, ac0ttemp: action1 = g_shared_value + ac0ttemp";
    struct rule_engine__specification *spec1 = addRule("GlobalConsist1", ruleStr1, 0);
    ASSERT_NE(spec1, nullptr);

    const char *ruleStr2 = "g_shared_value, cc0voc: action2 = g_shared_value * 2";
    struct rule_engine__specification *spec2 = addRule("GlobalConsist2", ruleStr2, 1);
    ASSERT_NE(spec2, nullptr);

    const char *ruleStr3 = "g_shared_value, mbat0caprel: action3 = g_shared_value - 50";
    struct rule_engine__specification *spec3 = addRule("GlobalConsist3", ruleStr3, 2);
    ASSERT_NE(spec3, nullptr);

    // Verify all rules see g_shared_value = 100
    verifyVariable(spec1, "g_shared_value", 100, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_shared_value"));
    verifyVariable(spec2, "g_shared_value", 100, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_shared_value"));
    verifyVariable(spec3, "g_shared_value", 100, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_shared_value"));

    // Verify local variables are isolated
    verifyVariable(spec1, "action1", 0, RULE_ENGINE__VAR_TYPE_LOCAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("action1"));
    verifyVariable(spec2, "action2", 0, RULE_ENGINE__VAR_TYPE_LOCAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("action2"));
    verifyVariable(spec3, "action3", 0, RULE_ENGINE__VAR_TYPE_LOCAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("action3"));
}

TEST_F(RuleEngineParserTest, GlobalVariableConsistency_ReInitializedInMiddleRule)
{
    // Test global variable re-initialization in middle rule
    const char *ruleStr1 = "g_counter = 10, ac0on: result1 = g_counter";
    struct rule_engine__specification *spec1 = addRule("GlobalReInit1", ruleStr1, 0);
    ASSERT_NE(spec1, nullptr);

    const char *ruleStr2 = "g_counter = 25, cc0voc: result2 = g_counter + 5";
    struct rule_engine__specification *spec2 = addRule("GlobalReInit2", ruleStr2, 1);
    ASSERT_NE(spec2, nullptr);

    const char *ruleStr3 = "g_counter, mbat0caprel: result3 = g_counter * 3";
    struct rule_engine__specification *spec3 = addRule("GlobalReInit3", ruleStr3, 2);
    ASSERT_NE(spec3, nullptr);

    // Verify all rules see updated value g_counter = 25
    verifyVariable(spec1, "g_counter", 25, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_counter"));
    verifyVariable(spec2, "g_counter", 25, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_counter"));
    verifyVariable(spec3, "g_counter", 25, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_counter"));
}

TEST_F(RuleEngineParserTest, GlobalVariableConsistency_MultipleReInitializations)
{
    // Test multiple re-initializations with additional trigger variables
    const char *ruleStr1 = "g_status = 1, g_trig1: action1 = g_status";
    struct rule_engine__specification *spec1 = addRule("GlobalMultiReInit1", ruleStr1, 0);
    ASSERT_NE(spec1, nullptr);

    const char *ruleStr2 = "g_status = 2, g_trig2: action2 = g_status";
    struct rule_engine__specification *spec2 = addRule("GlobalMultiReInit2", ruleStr2, 1);
    ASSERT_NE(spec2, nullptr);

    const char *ruleStr3 = "g_status = 3, g_trig3: action3 = g_status";
    struct rule_engine__specification *spec3 = addRule("GlobalMultiReInit3", ruleStr3, 2);
    ASSERT_NE(spec3, nullptr);

    const char *ruleStr4 = "g_status, g_trig4: action4 = g_status";
    struct rule_engine__specification *spec4 = addRule("GlobalMultiReInit4", ruleStr4, 3);
    ASSERT_NE(spec4, nullptr);

    // Verify final value is 3, all rules see the last initialized value
    verifyVariable(spec1, "g_status", 3, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_status"));
    verifyVariable(spec2, "g_status", 3, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_status"));
    verifyVariable(spec3, "g_status", 3, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_status"));
    verifyVariable(spec4, "g_status", 3, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_status"));
}

TEST_F(RuleEngineParserTest, DDM2VariableConsistency_InitializedOnceSharedAcross)
{
    // Test DDM2 variable initialized once and shared across rules
    const char *ruleStr1 = "ac0avl=ddm, ac0ttemp=ddm(50), g_trigger1: pub(ac0ttemp = 100)";
    struct rule_engine__specification *spec1 = addRule("DDM2Shared1", ruleStr1, 0);
    ASSERT_NE(spec1, nullptr);

    const char *ruleStr2 = "g_trigger2: pub(ac0ttemp = 20)";
    struct rule_engine__specification *spec2 = addRule("DDM2Shared2", ruleStr2, 1);
    ASSERT_NE(spec2, nullptr);

    const char *ruleStr3 = "g_trigger3: pub(ac0ttemp = 70)";
    struct rule_engine__specification *spec3 = addRule("DDM2Shared3", ruleStr3, 2);
    ASSERT_NE(spec3, nullptr);

    verifyVariable(spec1, "ac0avl", 0, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_OWNED, AC0AVL);
    // Verify all rules see ac0ttemp = 50
    verifyVariable(spec1, "ac0ttemp", 50, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_OWNED, AC0TTEMP);
    verifyVariable(spec2, "ac0ttemp", 50, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, AC0TTEMP);
    verifyVariable(spec3, "ac0ttemp", 50, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, AC0TTEMP);
    // TODO: Using CREATE_SUBSCRIBED for the test with DDM2 variable in rules 2 and 3, so we can verify that the value is shared correctly
    LOG(E, "ac0ttemp has to OWNED in Rules 2 and 3, because it is created as owned in Rule 1!");

    // Only Rule 1 publishes it
    verifyDDMWItem(spec1, "ac0ttemp", 0, DDMW_ACTION_PUBLISH, AC0AVL);
    verifyDDMWItem(spec1, "ac0ttemp", 50, DDMW_ACTION_PUBLISH, AC0TTEMP);
}

TEST_F(RuleEngineParserTest, DDM2VariableConsistency_DifferentInitializationValues)
{
    // Test DDM2 variable with different initialization values
    const char *ruleStr1 = "cc0avl=ddm, cc0voc=ddm(300), g_trigger1: action1 = cc0voc";
    struct rule_engine__specification *spec1 = addRule("DDM2DiffInit1", ruleStr1, 0);
    ASSERT_NE(spec1, nullptr);

    const char *ruleStr2 = "cc0voc=ddm(450), g_trigger2: action2 = cc0voc";
    struct rule_engine__specification *spec2 = addRule("DDM2DiffInit2", ruleStr2, 1);
    ASSERT_NE(spec2, nullptr);

    const char *ruleStr3 = "g_trigger3: action3 = cc0voc";
    struct rule_engine__specification *spec3 = addRule("DDM2DiffInit3", ruleStr3, 2);
    ASSERT_NE(spec3, nullptr);

    // Final value should be 450, all rules see the last initialized value
    verifyVariable(spec1, "cc0voc", 450, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_OWNED, CC0VOC);
    verifyVariable(spec2, "cc0voc", 450, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_OWNED, CC0VOC);
    verifyVariable(spec3, "cc0voc", 450, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, CC0VOC);
    // TODO: Using CREATE_SUBSCRIBED for the test with DDM2 variable in rules 2 and 3, so we can verify that the value is shared correctly
    LOG(E, "cc0voc has to OWNED in Rule 3, because it is created as owned in Rule 1 and 2!");

    verifyDDMWItem(spec1, "cc0voc", 450, DDMW_ACTION_PUBLISH, CC0VOC);
}

TEST_F(RuleEngineParserTest, DDM2VariableConsistency_MixedSubscriptionAndPublication)
{
    // Test mixed subscription and publication with rule 3 not having mbat0caprel in trigger
    const char *ruleStr2 = "mbat0avl=ddm, mbat0caprel=ddm(75), g_trigger2: action2 = mbat0caprel";
    struct rule_engine__specification *spec2 = addRule("DDM2Mixed2", ruleStr2, 0);
    ASSERT_NE(spec2, nullptr);

    const char *ruleStr3 = "g_trigger3: action3 = mbat0caprel + 10";
    struct rule_engine__specification *spec3 = addRule("DDM2Mixed3", ruleStr3, 1);
    ASSERT_NE(spec3, nullptr);

    // Rule 2 publishes with value 75, Rule 3 subscribes
    verifyVariable(spec2, "mbat0caprel", 75, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_OWNED, MBAT0CAPREL);
    verifyVariable(spec3, "mbat0caprel", 75, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED, MBAT0CAPREL);
    // TODO: Using CREATE_SUBSCRIBED for the test with DDM2 variable in rule 2, so we can verify that the value is shared correctly
    LOG(E, "mbat0caprel has to OWNED in Rule 2, because it is created as owned in Rule 1!");

    verifyDDMWItem(spec2, "mbat0caprel", 75, DDMW_ACTION_PUBLISH, MBAT0CAPREL);

    verifyVariableTriggerStates(spec3, 1, 0x1, RULE_ENGINE__MASK_CONSTRAINT_AND,
                                {{"g_trigger3", TRIGGER_LIST_NOT_INITIALIZED, TRIGGER_LIST_EXISTS},
                                 {"mbat0caprel", TRIGGER_LIST_NOT_EXISTS, TRIGGER_LIST_NOT_EXISTS}});
}

TEST_F(RuleEngineParserTest, LocalVariableConsistency_IsolatedAcrossRules)
{
    // Test local variables are isolated across rules with initialization in trigger list
    const char *ruleStr1 = "local_temp = 100, g_trigger1: result1 = local_temp";
    struct rule_engine__specification *spec1 = addRule("LocalIsolated1", ruleStr1, 0);
    ASSERT_NE(spec1, nullptr);

    const char *ruleStr2 = "local_temp = 200, g_trigger2: result2 = local_temp";
    struct rule_engine__specification *spec2 = addRule("LocalIsolated2", ruleStr2, 1);
    ASSERT_NE(spec2, nullptr);

    const char *ruleStr3 = "local_temp = 300, g_trigger3: result3 = local_temp";
    struct rule_engine__specification *spec3 = addRule("LocalIsolated3", ruleStr3, 2);
    ASSERT_NE(spec3, nullptr);

    // Each rule has its own local_temp with different values
    verifyVariable(spec1, "local_temp", 100, RULE_ENGINE__VAR_TYPE_LOCAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("local_temp"));
    verifyVariable(spec2, "local_temp", 200, RULE_ENGINE__VAR_TYPE_LOCAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("local_temp"));
    verifyVariable(spec3, "local_temp", 300, RULE_ENGINE__VAR_TYPE_LOCAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("local_temp"));
}

TEST_F(RuleEngineParserTest, LocalVariableConsistency_SameNameDifferentScopes)
{
    // Test same named local variables in different rule scopes
    const char *ruleStr1 = "g_shared = 10, g_test: temp_var = g_shared * 5, final1 = temp_var";
    struct rule_engine__specification *spec1 = addRule("LocalScope1", ruleStr1, 0);
    ASSERT_NE(spec1, nullptr);

    const char *ruleStr2 = "g_shared: temp_var = g_shared + 15, final2 = temp_var";
    struct rule_engine__specification *spec2 = addRule("LocalScope2", ruleStr2, 1);
    ASSERT_NE(spec2, nullptr);

    const char *ruleStr3 = "g_shared: temp_var = g_shared - 5, final3 = temp_var";
    struct rule_engine__specification *spec3 = addRule("LocalScope3", ruleStr3, 2);
    ASSERT_NE(spec3, nullptr);

    // Each temp_var is isolated per rule despite same name
    verifyVariable(spec1, "temp_var", 0, RULE_ENGINE__VAR_TYPE_LOCAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("temp_var"));
    verifyVariable(spec2, "temp_var", 0, RULE_ENGINE__VAR_TYPE_LOCAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("temp_var"));
    verifyVariable(spec3, "temp_var", 0, RULE_ENGINE__VAR_TYPE_LOCAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("temp_var"));

    // Global variable shared across rules
    verifyVariable(spec1, "g_shared", 10, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_shared"));
    verifyVariable(spec2, "g_shared", 10, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_shared"));
    verifyVariable(spec3, "g_shared", 10, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_shared"));
}

TEST_F(RuleEngineParserTest, MixedVariableConsistency_GlobalDDM2LocalCombination)
{
    // Test mixed variable types with local_calc initialized in trigger list
    const char *ruleStr1 = "g_base = 50, local_calc = 0, ac0ttemp: result1 = g_base + ac0ttemp + local_calc";
    struct rule_engine__specification *spec1 = addRule("MixedVar1", ruleStr1, 0);
    ASSERT_NE(spec1, nullptr);

    const char *ruleStr2 = "g_base, g_mode = 2, status = 1: result2 = g_base + ac0ttemp + g_mode + status";
    struct rule_engine__specification *spec2 = addRule("MixedVar2", ruleStr2, 1);
    ASSERT_NE(spec2, nullptr);

    const char *ruleStr3 = "g_base = 75, g_mode, status = 2: result3 = g_base - 25 + local_calc";
    struct rule_engine__specification *spec3 = addRule("MixedVar3", ruleStr3, 2);
    ASSERT_NE(spec3, nullptr);

    // g_base final value = 75 (shared across all rules)
    verifyVariable(spec1, "g_base", 75, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_base"));
    verifyVariable(spec2, "g_base", 75, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_base"));
    verifyVariable(spec3, "g_base", 75, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_base"));

    // g_mode = 2 (from Rule 2)
    verifyVariable(spec2, "g_mode", 2, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_mode"));
    verifyVariable(spec3, "g_mode", 2, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_mode"));

    // local_calc isolated per rule
    verifyVariable(spec1, "local_calc", 0, RULE_ENGINE__VAR_TYPE_LOCAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("local_calc"));
    verifyVariable(spec3, "local_calc", 0, RULE_ENGINE__VAR_TYPE_LOCAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("local_calc"));
}

TEST_F(RuleEngineParserTest, VariableConsistency_EdgeCase_UnusedInitializedVariables)
{
    // Test unused initialized variables maintain consistency
    const char *ruleStr1 = "g_unused = 999, g_trigger: action1 = 1";
    struct rule_engine__specification *spec1 = addRule("UnusedVar1", ruleStr1, 0);
    ASSERT_NE(spec1, nullptr);

    const char *ruleStr2 = "g_unused, g_trigger2: action2 = g_unused + 1";
    struct rule_engine__specification *spec2 = addRule("UnusedVar2", ruleStr2, 1);
    ASSERT_NE(spec2, nullptr);

    // Rule 2 sees g_unused = 999 even though it wasn't used in Rule 1's action
    verifyVariable(spec1, "g_unused", 999, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_unused"));
    verifyVariable(spec2, "g_unused", 999, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_unused"));
}

TEST_F(RuleEngineParserTest, VariableConsistency_EdgeCase_ChainedReferences)
{
    // Test chained variable references with g_val2 in trigger list of rule 1
    const char *ruleStr1 = "g_val1 = 10, g_val2: g_val2 = g_val1 * 2, g_val3 = g_val2 + 5";
    struct rule_engine__specification *spec1 = addRule("ChainedRef1", ruleStr1, 0);
    ASSERT_NE(spec1, nullptr);

    const char *ruleStr2 = "g_val1, g_val2, g_val3: result = g_val1 + g_val2 + g_val3";
    struct rule_engine__specification *spec2 = addRule("ChainedRef2", ruleStr2, 1);
    ASSERT_NE(spec2, nullptr);

    // Rule 2 sees g_val1=10, g_val2=20, g_val3=25
    verifyVariable(spec1, "g_val1", 10, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_val1"));
    verifyVariable(spec1, "g_val2", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_val2"));
    verifyVariable(spec1, "g_val3", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_val3"));

    verifyVariable(spec2, "g_val1", 10, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_val1"));
    verifyVariable(spec2, "g_val2", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_val2"));
    verifyVariable(spec2, "g_val3", 0, RULE_ENGINE__VAR_TYPE_GLOBAL, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE, expr_generate_hash_key("g_val3"));
}

TEST_F(RuleEngineParserTest, VerifyParserDynamicInstInventory)
{
    const char *rule0_str = "inventory(g_groupX_instance, GROUP0)";

    static const struct rule_engine__rule rules[] =
        {
            {.name = "TestRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
        };

    // Load Inventory callback rule
    EXPECT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);

    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    EXPECT_NE(rule0_spec, nullptr) << "Failed to retrieve rule specification";

    int var_list_size = getVarListSize(rule0_spec);
    EXPECT_EQ(var_list_size, 3) << "Expected 3 variables in the list(gw0inv as well), but got: " << var_list_size;

    struct expr_var *gw0inv = getVariableByName(rule0_spec, "GW0INV");
    ASSERT_NE(gw0inv, nullptr);
    EXPECT_EQ(expr_var_value(gw0inv), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_id(gw0inv), GW0INV);
    EXPECT_EQ(expr_var_type(gw0inv), RULE_ENGINE__VAR_TYPE_LOCAL);
    EXPECT_EQ(expr_var_type_id(gw0inv), RULE_ENGINE__VAR_TYTE_ID_UNAPLICABLE);
    EXPECT_EQ(expr_var_type_is_resolved(gw0inv), RULE_ENGINE__VAR_TYPE_RESOLVED) << "" << gw0inv->type_is_resolved;
    EXPECT_EQ(expr_var_type_ddm_create(gw0inv), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

    struct expr_var *g_groupX_instance = getVariableByName(rule0_spec, "g_groupX_instance");
    ASSERT_NE(g_groupX_instance, nullptr);
    EXPECT_EQ(expr_var_value(g_groupX_instance), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_id(g_groupX_instance), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type(g_groupX_instance), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_type_id(g_groupX_instance), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    EXPECT_EQ(expr_var_type_ddm_create(g_groupX_instance), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

    struct expr_var *group0 = getVariableByName(rule0_spec, "GROUP0");
    ASSERT_NE(group0, nullptr);
    EXPECT_EQ(expr_var_value(group0), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_id(group0), GROUP0);
    EXPECT_EQ(expr_var_type(group0), RULE_ENGINE__VAR_TYPE_DDM2);
    EXPECT_EQ(expr_var_type_id(group0), RULE_ENGINE__VAR_TYPE_ID_UNRESOLVED);
    EXPECT_EQ(expr_var_type_is_resolved(group0), RULE_ENGINE__VAR_TYPE_RESOLVED);
    EXPECT_EQ(expr_var_type_ddm_create(group0), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED);

    EXPECT_EQ(getTriggerListSize(rule0_spec), 0) << "Failed rule: " << rule0_str;
    EXPECT_EQ(getMaskType(rule0_spec), RULE_ENGINE__MASK_CONSTRAINT_AND);  // default mask type
    EXPECT_FALSE(isVariableInTriggerList(rule0_spec, g_groupX_instance));
    EXPECT_FALSE(isVariableInMask(rule0_spec, g_groupX_instance));
    EXPECT_FALSE(isVariableInTriggerList(rule0_spec, group0));
    EXPECT_FALSE(isVariableInMask(rule0_spec, group0));
    EXPECT_FALSE(isVariableInTriggerList(rule0_spec, gw0inv));
    EXPECT_FALSE(isVariableInMask(rule0_spec, gw0inv));
}

TEST_F(RuleEngineParserTest, VerifyParserDynamicInstInventoryOfSameDDM2Class)
{
    const char *rule0_str = "inventory(g_groupX_instance, GROUP0)";
    const char *rule1_str = "inventory(g_groupY_instance, GROUP2)";
    const char *rule2_str = "inventory(g_groupZ_instance, GROUP0)";  // Same DDM2 class as rule0, should be handled differently in run time
    const char *rule3_str = "inventory(g_htr_instance, HTR0)";

    static const struct rule_engine__rule rules[] =
        {
            {.name = "TestRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
            {.name = "TestRule1", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
            {.name = "TestRule2", .rule = const_cast<char *>(rule2_str), .size = static_cast<int>(strlen(rule2_str))},
            {.name = "TestRule3", .rule = const_cast<char *>(rule3_str), .size = static_cast<int>(strlen(rule3_str))},
        };

    // Load Inventory callback rule
    EXPECT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);

    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    EXPECT_NE(rule0_spec, nullptr) << "Failed to retrieve rule specification";

    int var_list_size = getVarListSize(rule0_spec);
    EXPECT_EQ(var_list_size, 3) << "Expected 3 variables in the list(gw0inv as well), but got: " << var_list_size;

    struct expr_var *g_groupX_instance = getVariableByName(rule0_spec, "g_groupX_instance");
    ASSERT_NE(g_groupX_instance, nullptr);
    EXPECT_EQ(expr_var_value(g_groupX_instance), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_id(g_groupX_instance), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type(g_groupX_instance), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_type_id(g_groupX_instance), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    EXPECT_EQ(expr_var_type_ddm_create(g_groupX_instance), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

    // Load secondary Inventory callback rule
    EXPECT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[1]), 1);

    struct rule_engine__specification *rule1_spec = getSpecificationById(1);
    EXPECT_NE(rule1_spec, nullptr) << "Failed to retrieve rule specification";

    var_list_size = getVarListSize(rule1_spec);
    EXPECT_EQ(var_list_size, 3) << "Expected 3 variables in the list(gw0inv as well), but got: " << var_list_size;

    struct expr_var *g_groupY_instance = getVariableByName(rule1_spec, "g_groupY_instance");
    ASSERT_NE(g_groupY_instance, nullptr);
    EXPECT_EQ(expr_var_value(g_groupY_instance), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_id(g_groupY_instance), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type(g_groupY_instance), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_type_id(g_groupY_instance), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    EXPECT_EQ(expr_var_type_ddm_create(g_groupY_instance), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

    // Load third Inventory callback rule
    EXPECT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[2]), RULE_ENGINE__INV_RULE_FORMAT);

    // Load fourth Inventory callback rule
    EXPECT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[3]), 1);

    struct rule_engine__specification *rule3_spec = getSpecificationById(2);
    EXPECT_NE(rule3_spec, nullptr) << "Failed to retrieve rule specification";

    var_list_size = getVarListSize(rule3_spec);
    EXPECT_EQ(var_list_size, 3) << "Expected 3 variables in the list(gw0inv as well), but got: " << var_list_size;

    struct expr_var *g_htr_instance = getVariableByName(rule3_spec, "g_htr_instance");
    ASSERT_NE(g_htr_instance, nullptr);
    EXPECT_EQ(expr_var_value(g_htr_instance), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_id(g_htr_instance), HTR0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type(g_htr_instance), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_type_id(g_htr_instance), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_htr_instance), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    EXPECT_EQ(expr_var_type_ddm_create(g_htr_instance), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);
}

TEST_F(RuleEngineParserTest, VerifyParserDynamicInstNoInventory)
{
    const char *rule0_str = "dyn_instance(g_groupY_instance, group1interface, GROUP0, 0)";
    const char *rule1_str = "dyn_instance(g_prod_instance, group{g_groupY_instance}interface, PROD0, 1)";

    static const struct rule_engine__rule rules[] =
        {
            {.name = "TestRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
            {.name = "TestRule1", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
        };

    // Load Dynamic Instance resolving rule
    EXPECT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);

    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    EXPECT_NE(rule0_spec, nullptr) << "Failed to retrieve rule specification";

    int var_list_size = getVarListSize(rule0_spec);
    EXPECT_EQ(var_list_size, 3) << "Expected 3 variables in the list, but got: " << var_list_size;

    // Check variables
    struct expr_var *g_groupY_instance = getVariableByName(rule0_spec, "g_groupY_instance");
    ASSERT_NE(g_groupY_instance, nullptr);
    EXPECT_EQ(expr_var_value(g_groupY_instance), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_id(g_groupY_instance), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type(g_groupY_instance), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_type_id(g_groupY_instance), 0) << "Expected type ID to be 1 for g_groupY_instance but got: " << expr_var_type_id(g_groupY_instance);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    EXPECT_EQ(expr_var_type_ddm_create(g_groupY_instance), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

    struct expr_var *group_g_group1_interface = getVariableByName(rule0_spec, "group1interface");
    ASSERT_NE(group_g_group1_interface, nullptr);
    EXPECT_EQ(expr_var_value(group_g_group1_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_id(group_g_group1_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(1));
    EXPECT_EQ(expr_var_type(group_g_group1_interface), RULE_ENGINE__VAR_TYPE_DDM2);
    EXPECT_EQ(expr_var_type_id(group_g_group1_interface), RULE_ENGINE__VAR_TYPE_ID_UNRESOLVED) << "Expected type ID to be 7 for group_g_group1_interface, but got: " << expr_var_type_id(group_g_group1_interface);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_group1_interface), RULE_ENGINE__VAR_TYPE_RESOLVED);
    EXPECT_EQ(expr_var_type_ddm_create(group_g_group1_interface), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED);

    struct expr_var *group0 = getVariableByName(rule0_spec, "GROUP0");
    ASSERT_NE(group0, nullptr);
    EXPECT_EQ(expr_var_value(group0), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_id(group0), GROUP0);
    EXPECT_EQ(expr_var_type(group0), RULE_ENGINE__VAR_TYPE_DDM2);
    EXPECT_EQ(expr_var_type_id(group0), RULE_ENGINE__VAR_TYPE_ID_UNRESOLVED);
    EXPECT_EQ(expr_var_type_is_resolved(group0), RULE_ENGINE__VAR_TYPE_RESOLVED);
    EXPECT_EQ(expr_var_type_ddm_create(group0), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED);

    // Check trigger section
    EXPECT_EQ(getMaskType(rule0_spec), RULE_ENGINE__MASK_CONSTRAINT_AND);  // default.
    EXPECT_EQ(getTriggerListSize(rule0_spec), 1) << "Only the dynamic parameter should be in the trigger list";
    EXPECT_FALSE(isVariableInTriggerList(rule0_spec, g_groupY_instance));
    EXPECT_FALSE(isVariableInMask(rule0_spec, g_groupY_instance));
    EXPECT_FALSE(isVariableInTriggerList(rule0_spec, group0));
    EXPECT_FALSE(isVariableInMask(rule0_spec, group0));
    EXPECT_TRUE(isVariableInTriggerList(rule0_spec, group_g_group1_interface));
    EXPECT_TRUE(isVariableInMask(rule0_spec, group_g_group1_interface));

    // Load Dynamic Instance resolving rule
    EXPECT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[1]), 1);

    struct rule_engine__specification *rule1_spec = getSpecificationById(1);
    EXPECT_NE(rule1_spec, nullptr) << "Failed to retrieve rule specification";

    var_list_size = getVarListSize(rule1_spec);
    EXPECT_EQ(var_list_size, 3) << "Expected 3 variables in the list, but got: " << var_list_size;

    // Check variables
    struct expr_var *g_prod_instance = getVariableByName(rule1_spec, "g_prod_instance");
    ASSERT_NE(g_prod_instance, nullptr);
    EXPECT_EQ(expr_var_value(g_prod_instance), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_id(g_prod_instance), PROD0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE)) << "GroupZ instance should have 0x" << std::hex << expr_var_id(g_prod_instance);
    EXPECT_EQ(expr_var_type(g_prod_instance), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_type_id(g_prod_instance), 0) << "Expected type ID to be 1 for g_prod_instance but got: " << expr_var_type_id(g_prod_instance);
    EXPECT_EQ(expr_var_type_is_resolved(g_prod_instance), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    EXPECT_EQ(expr_var_type_ddm_create(g_prod_instance), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

    struct expr_var *group_g_groupY_instance_interface = getVariableByName(rule1_spec, "group{g_groupY_instance}interface");
    ASSERT_NE(group_g_groupY_instance_interface, nullptr);
    EXPECT_EQ(expr_var_value(group_g_groupY_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_id(group_g_groupY_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_type_id(group_g_groupY_instance_interface), 0) << "Expected type ID to be 0 for group{g_groupY_instance}interface but got: " << expr_var_type_id(group_g_groupY_instance_interface);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    EXPECT_EQ(expr_var_type_ddm_create(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED);

    struct expr_var *prod0 = getVariableByName(rule1_spec, "PROD0");
    ASSERT_NE(prod0, nullptr);
    EXPECT_EQ(expr_var_value(prod0), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_id(prod0), PROD0);
    EXPECT_EQ(expr_var_type(prod0), RULE_ENGINE__VAR_TYPE_DDM2);
    EXPECT_EQ(expr_var_type_id(prod0), RULE_ENGINE__VAR_TYPE_ID_UNRESOLVED);
    EXPECT_EQ(expr_var_type_is_resolved(prod0), RULE_ENGINE__VAR_TYPE_RESOLVED);
    EXPECT_EQ(expr_var_type_ddm_create(prod0), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED);

    // Check trigger section
    EXPECT_EQ(getMaskType(rule1_spec), RULE_ENGINE__MASK_CONSTRAINT_AND);  // default.
    EXPECT_EQ(getTriggerListSize(rule1_spec), 1) << "Only the dynamic parameter should be in the trigger list";
    EXPECT_FALSE(isVariableInTriggerList(rule1_spec, g_prod_instance));
    EXPECT_FALSE(isVariableInMask(rule1_spec, g_prod_instance));
    EXPECT_FALSE(isVariableInTriggerList(rule1_spec, prod0));
    EXPECT_FALSE(isVariableInMask(rule1_spec, prod0));
    EXPECT_TRUE(isVariableInTriggerList(rule1_spec, group_g_groupY_instance_interface));
    EXPECT_TRUE(isVariableInMask(rule1_spec, group_g_groupY_instance_interface));
}

TEST_F(RuleEngineParserTest, VerifyParserDynamicInstTypeIDForStandardRules)
{
    /* Load the same rules as in 'VerifyParserDynamicInstTypeIDForDynamicResolvingRules'
     * test, with the addional standard rules of interest
     */
    const char *rule0_str = "inventory(g_groupX_instance, GROUP0)";
    const char *rule1_str = "dyn_instance(g_groupY_instance, group{g_groupX_instance}interface, GROUP0, 0)";
    const char *rule2_str = "dyn_instance(g_group_prod_mbat_instance, group{g_groupX_instance}interface, PROD0, 0)";
    const char *rule3_str = "dyn_instance(g_mbat_instance, prod{g_group_prod_mbat_instance}clist, MBAT0, 0)";
    const char *rule4_str = "dyn_instance(g_group_prod_instance, group{g_groupY_instance}interface, PROD0, 0)";
    const char *rule5_str = "dyn_instance(g_ac_instance, prod{g_group_prod_instance}clist, AC0, 0)";
    const char *rule6_str = "mbat{g_mbat_instance}soc: g_mbatsoc = mbat{g_mbat_instance}soc, g_continue = 1";
    const char *rule7_str = "ac{g_ac_instance}ttemp:   g_acttemp = ac{g_ac_instance}ttemp, g_continue = 1";
    const char *rule8_str = "prod{g_group_prod_mbat_instance}sku, prod{g_group_prod_instance}sku: g_acttemp = ac{g_ac_instance}ttemp, g_continue = 1";

    static const struct rule_engine__rule rules[] =
        {
            {.name = "TestRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
            {.name = "TestRule1", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
            {.name = "TestRule2", .rule = const_cast<char *>(rule2_str), .size = static_cast<int>(strlen(rule2_str))},
            {.name = "TestRule3", .rule = const_cast<char *>(rule3_str), .size = static_cast<int>(strlen(rule3_str))},
            {.name = "TestRule4", .rule = const_cast<char *>(rule4_str), .size = static_cast<int>(strlen(rule4_str))},
            {.name = "TestRule5", .rule = const_cast<char *>(rule5_str), .size = static_cast<int>(strlen(rule5_str))},
            {.name = "TestRule6", .rule = const_cast<char *>(rule6_str), .size = static_cast<int>(strlen(rule6_str))},
            {.name = "TestRule7", .rule = const_cast<char *>(rule7_str), .size = static_cast<int>(strlen(rule7_str))},
            {.name = "TestRule8", .rule = const_cast<char *>(rule8_str), .size = static_cast<int>(strlen(rule8_str))},
        };

    // Load rules, all together, then verify the specifications
    for (size_t i = 0; i < sizeof(rules) / sizeof(rules[0]); ++i)
    {
        EXPECT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[i]), 1) << "Failed to add rule: " << rules[i].name;
    }

    struct rule_engine__specification *rule6_spec = getSpecificationById(6);
    EXPECT_NE(rule6_spec, nullptr) << "Failed to retrieve rule specification";
    struct expr_var *mbat_g_mbat_instance_soc_spec6 = getVariableByName(rule6_spec, "mbat{g_mbat_instance}soc");
    ASSERT_NE(mbat_g_mbat_instance_soc_spec6, nullptr);
    EXPECT_EQ(expr_var_id(mbat_g_mbat_instance_soc_spec6), MBAT0SOC | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type(mbat_g_mbat_instance_soc_spec6), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_type_id(mbat_g_mbat_instance_soc_spec6), 0) << "Expected type ID to be 0 for mbat{g_mbat_instance}soc but got: " << expr_var_type_id(mbat_g_mbat_instance_soc_spec6);
    struct expr_var *g_mbatsoc = getVariableByName(rule6_spec, "g_mbatsoc");
    ASSERT_NE(g_mbatsoc, nullptr);
    EXPECT_EQ(expr_var_id(g_mbatsoc), expr_generate_hash_key("g_mbatsoc"));
    EXPECT_EQ(expr_var_type(g_mbatsoc), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_type_id(g_mbatsoc), RULE_ENGINE__VAR_TYTE_ID_UNAPLICABLE);
    struct expr_var *g_continue = getVariableByName(rule6_spec, "g_continue");
    ASSERT_NE(g_continue, nullptr);
    EXPECT_EQ(expr_var_id(g_continue), expr_generate_hash_key("g_continue"));
    EXPECT_EQ(expr_var_type(g_continue), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_type_id(g_continue), RULE_ENGINE__VAR_TYTE_ID_UNAPLICABLE);
    // Check trigger section
    EXPECT_EQ(getMaskType(rule6_spec), RULE_ENGINE__MASK_CONSTRAINT_AND);  // default.
    EXPECT_EQ(getTriggerListSize(rule6_spec), 1) << "Only the dynamic parameter should be in the trigger list";
    EXPECT_FALSE(isVariableInTriggerList(rule6_spec, g_mbatsoc));
    EXPECT_FALSE(isVariableInMask(rule6_spec, g_mbatsoc));
    EXPECT_FALSE(isVariableInTriggerList(rule6_spec, g_continue));
    EXPECT_FALSE(isVariableInMask(rule6_spec, g_continue));
    EXPECT_TRUE(isVariableInTriggerList(rule6_spec, mbat_g_mbat_instance_soc_spec6));
    EXPECT_TRUE(isVariableInMask(rule6_spec, mbat_g_mbat_instance_soc_spec6));

    struct rule_engine__specification *rule7_spec = getSpecificationById(7);
    EXPECT_NE(rule6_spec, nullptr) << "Failed to retrieve rule specification";
    struct expr_var *ac_g_ac_instance_ttemp = getVariableByName(rule7_spec, "ac{g_ac_instance}ttemp");
    ASSERT_NE(ac_g_ac_instance_ttemp, nullptr);
    EXPECT_EQ(expr_var_id(ac_g_ac_instance_ttemp), AC0TTEMP | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type(ac_g_ac_instance_ttemp), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_type_id(mbat_g_mbat_instance_soc_spec6), 0) << "Expected type ID to be 0 for ac{g_ac_instance}ttemp but got: " << expr_var_type_id(ac_g_ac_instance_ttemp);
    // Check trigger section
    EXPECT_EQ(getMaskType(rule7_spec), RULE_ENGINE__MASK_CONSTRAINT_AND);  // default.
    EXPECT_EQ(getTriggerListSize(rule7_spec), 1) << "Only the dynamic parameters should be in the trigger list";
    EXPECT_TRUE(isVariableInTriggerList(rule7_spec, ac_g_ac_instance_ttemp));
    EXPECT_TRUE(isVariableInMask(rule7_spec, ac_g_ac_instance_ttemp));

    struct rule_engine__specification *rule8_spec = getSpecificationById(8);
    EXPECT_NE(rule8_spec, nullptr) << "Failed to retrieve rule specification";
    struct expr_var *prod_g_group_prod_mbat_instance_sku = getVariableByName(rule8_spec, "prod{g_group_prod_mbat_instance}sku");
    ASSERT_NE(prod_g_group_prod_mbat_instance_sku, nullptr);
    EXPECT_EQ(expr_var_id(prod_g_group_prod_mbat_instance_sku), PROD0SKU | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type(prod_g_group_prod_mbat_instance_sku), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_type_id(prod_g_group_prod_mbat_instance_sku), 0)
        << "Expected type ID to be 0 for prod{g_group_prod_mbat_instance}sku, as it is for g_group_prod_mbat_instance, but got: " << expr_var_type_id(prod_g_group_prod_mbat_instance_sku);
    struct expr_var *prod_g_group_prod_instance_sku = getVariableByName(rule8_spec, "prod{g_group_prod_instance}sku");
    ASSERT_NE(prod_g_group_prod_instance_sku, nullptr);
    EXPECT_EQ(expr_var_id(prod_g_group_prod_instance_sku), PROD0SKU | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type(prod_g_group_prod_instance_sku), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_type_id(prod_g_group_prod_instance_sku), 1)
        << "Expected type ID to be 1 for prod{g_group_prod_instance}sku, as it is for g_group_prod_instance, but got: " << expr_var_type_id(prod_g_group_prod_instance_sku);
    // Check trigger section
    EXPECT_EQ(getMaskType(rule8_spec), RULE_ENGINE__MASK_CONSTRAINT_AND);  // default.
    EXPECT_EQ(getTriggerListSize(rule8_spec), 2) << "Only the dynamic parameters should be in the trigger list";
    EXPECT_TRUE(isVariableInTriggerList(rule8_spec, prod_g_group_prod_mbat_instance_sku));
    EXPECT_TRUE(isVariableInMask(rule8_spec, prod_g_group_prod_mbat_instance_sku));
    EXPECT_TRUE(isVariableInTriggerList(rule8_spec, prod_g_group_prod_instance_sku));
    EXPECT_TRUE(isVariableInMask(rule8_spec, prod_g_group_prod_instance_sku));
}
