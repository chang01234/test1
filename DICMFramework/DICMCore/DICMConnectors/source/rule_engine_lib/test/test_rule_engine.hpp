extern "C" {
#include "connector_unittest.h"
#include "ddm2.h"
#include "ddm2_parameter_list.h"
#include "ddm_wrapper.h"
#include "rule_engine.h"
#include "rule_engine_def.h"
#include "rule_engine_exp_eval.h"
#include "sorted_container.h"
}
#include "DICMFrameworkTestFixture.hpp"

class RuleEngineTestFixture : public DICMFrameworkTestFixture
{
  protected:
    ddmw_t ddm_wrapper;
    rule_engine_inst_t rule_engine_inst;

    void SetUp() override
    {
        rule_engine_inst_config_t rule_engine_inst_config =
            {
                .rule_engine_specification_elements = RULE_ENGINE__MAX_NUMBER_OF_RULES,
                .rule_engine_ddm2_subscription_table_elements = RULE_ENGINE__DDM2_SUBSCRIPTION_DEPTH,
                .rule_engine_set_timers_elements = RULE_ENGINE__MAX_NUMBER_OF_TIMERS,
                .rule_engine_ddm_map_elements = RULE_ENGINE__DDM2_MAP_SIZE,
            };
        // Initialize rule engine instance
        memset(&rule_engine_inst, 0, sizeof(rule_engine_inst_t));
        rule_engine_inst.wrapper = &ddm_wrapper;
        // Init state, for TextFixtures that do not depend on broker reception(as test_rule_engine_parser.cpp)
        // For TestFixtures that depend on broker reception, the instance is set in the SetUp() method which has to be overridden
        rule_engine_inst.instance = 0;

        DICMFrameworkTestFixture::SetUp();
        DICMFrameworkTestFixture::setConnectorId(&connector_unittest.connector_id);
        DICMFrameworkTestFixture::SetupFramework();

        ddmw_init(&ddm_wrapper, &connector_unittest);
        ASSERT_EQ(rule_engine_initialize_instance(&rule_engine_inst, &rule_engine_inst_config), 1) << "Failed to initialize rule engine instance";
    }

    void TearDown() override
    {
        // Clear the containers
        rule_engine_delete_rules(&rule_engine_inst);

        DICMFrameworkTestFixture::TearDown();
    }

    struct rule_engine__specification *getSpecificationById(uint32_t rule_id)
    {
        return (struct rule_engine__specification *)sorted_container__access(rule_engine_inst.p_rule_engine_specifications, rule_id);
    }

    // Helper method to get the value of a DDM2 parameter
    ddmw_item_t *getDDM2ParameterByID(uint32_t parameter_id)
    {
        return (ddmw_item_t *)sorted_container__access(rule_engine_inst.p_rule_engine_ddm2_subscription_table, parameter_id);
    }

    int32_t getDDM2ParameterValue(ddmw_item_t *item)
    {
        int32_t ddm2_data;
        ddmw_get_data(item, &ddm2_data, sizeof(ddm2_data));
        return ddm2_data;
    }

    ddmw_action_t getDDM2ParameterType(ddmw_item_t *item)
    {
        return ddmw_get_type(item);
    }

    struct expr_var *getVariableByName(struct rule_engine__specification *rule_specification, const char *variable_name)
    {
        return expr_get_var_by_name(&rule_specification->expr.vars, variable_name, strlen(variable_name));
    }

    int32_t getVarListSize(const struct rule_engine__specification *rule_specification)
    {
        int32_t varListSize = 0;

        struct expr_var *var = nullptr;
        for (var = rule_specification->expr.vars.head; var; var = var->next)
        {
            varListSize++;
        }
        return varListSize;
    }

    // Helper function to verify that a variable has the same value across all rules
    // If it's a DDM2 parameter, also verify against the subscription table value
    bool verifyVariableValueThroughRules(const char *variable_name, int32_t expected_value)
    {
        bool is_verified = false;
        bool is_ddm2_param = false;
        ddmw_item_t *item = nullptr;
        int32_t first_rule_value = 0;
        bool first_rule = true;

        for (size_t i = 0; i < sorted_container__occupied(rule_engine_inst.p_rule_engine_specifications); i++)
        {
            struct rule_engine__specification *spec =
                (struct rule_engine__specification *)sorted_container__access(rule_engine_inst.p_rule_engine_specifications, i);

            struct expr_var *var = getVariableByName(spec, variable_name);
            if (var != nullptr)
            {
                // Determine if it's a DDM2 parameter
                if (first_rule)
                {
                    first_rule_value = expr_var_value(var);
                    first_rule = false;

                    if (expr_var_type(var) == RULE_ENGINE__VAR_TYPE_DDM2)
                    {
                        is_ddm2_param = true;
                        item = getDDM2ParameterByID(expr_var_id(var));
                    }
                }

                // Verify variable value matches the expected value
                EXPECT_EQ(expr_var_value(var), expected_value)
                    << "Variable '" << variable_name << "' in rule " << i << " has value "
                    << expr_var_value(var) << ", expected " << expected_value;

                // Also verify that value is consistent across all rules
                EXPECT_EQ(expr_var_value(var), first_rule_value)
                    << "Variable '" << variable_name << "' has inconsistent values across rules";

                is_verified = expr_var_value(var) == expected_value;
                if (!is_verified)
                {
                    break;  // Exit loop if we find a match
                }
                is_verified = expr_var_value(var) == first_rule_value;
                if (!is_verified)
                {
                    break;  // Exit loop if we find a match
                }
            }
        }
        // If this variable is a DDM2 parameter that we're subscribed to,
        // also verify that its value in the subscription table matches the expected value
        if (is_ddm2_param && item)
        {
            int32_t ddm2_value = getDDM2ParameterValue(item);
            EXPECT_EQ(ddm2_value, expected_value)
                << "DDM2 parameter value (" << ddm2_value << ") doesn't match expected value (" << expected_value << ")";
            is_verified = ddm2_value == expected_value;
        }

        return is_verified;
    }

    // Helper methods for trigger list
    size_t getTriggerListSize(const struct rule_engine__specification *specification)
    {
        return specification->trigger.n_params;
    }

    int getMaskType(const struct rule_engine__specification *specification)
    {
        return specification->trigger.mask_type;
    }

    // Helper method to check if a variable is in the trigger list. Address of the variable is used for comparison,
    // as it should the same address as the one in the rule engine specification.
    bool isVariableInTriggerList(const struct rule_engine__specification *specification, struct expr_var *variable)
    {
        bool is_var_in_trigg_list = false;
        for (size_t i = 0; i < getTriggerListSize(specification); i++)
        {
            const struct rule_engine__sensitivity_list *sens_item = &specification->trigger.variables[i];
            if (sens_item->variable == variable)
            {
                is_var_in_trigg_list = true;
                break;
            }
        }
        return is_var_in_trigg_list;
    }

    // Helper method to check if a variable is included in the rule's trigger mask
    bool isVariableInMask(const struct rule_engine__specification *specification, const struct expr_var *variable)
    {
        bool is_var_a_trigger = false;
        // First find the variable in the trigger variables array
        for (uint32_t i = 0; i < getTriggerListSize(specification); i++)
        {
            const struct rule_engine__sensitivity_list *sens_item = &specification->trigger.variables[i];
            // Check if this is the variable we're looking for
            if (sens_item->variable == variable)
            {
                // Check if the corresponding bit is set in the mask
                // Each variable's position in the array corresponds to its bit in the mask
                if (RULE_ENGINE__IS_TRIGGER_PARAM(specification->trigger.mask, i))
                {
                    is_var_a_trigger = true;
                }
                break;
            }
        }
        // Variable not found in trigger section
        return is_var_a_trigger;
    }
};
