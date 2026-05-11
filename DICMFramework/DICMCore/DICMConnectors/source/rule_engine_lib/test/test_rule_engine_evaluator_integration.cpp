/**
 * @file test_rule_engine_evaluator_integration.cpp
 *
 * @brief Integration tests for the rule engine evaluator
 *
 * Tests rule engine evaluation with task handlers and broker communication
 *
 * @author Borjan Bozhinovski
 * @date June 2, 2025
 */

#include "test_rule_engine.hpp"

extern "C" {
#include "broker.h"
#include "connector_climate_zone_feature.h"
#include "connector_smart_eco_feature.h"
}

#define RULE_ENGINE_EVALUATOR_INTEGRATION_LOG_LEVEL_VERBOSE 0  // To be set to 0 by default when committing

#if RULE_ENGINE_EVALUATOR_INTEGRATION_LOG_LEVEL_VERBOSE == 1
#define RULE_ENGINE_EVALUATOR_INTEGRATION_LOG_LEVEL "*:5"  // Set log level for integration tests
#else
#define RULE_ENGINE_EVALUATOR_INTEGRATION_LOG_LEVEL "*:3"  // Set log level for integration tests
#endif

class RuleEngineEvaluatorIntegrationTestFixture : public RuleEngineTestFixture
{
  protected:
#define MIMIC_OWNER_DDM2_CLASSES_SIZE 12  // Number of parameter classes ID for mimic owner
#define DEBUG_LOGS_ENABLE()                                               \
    {                                                                     \
        const char *loglvl = RULE_ENGINE_EVALUATOR_INTEGRATION_LOG_LEVEL; \
        connector_send_frame_to_broker(                                   \
            DDMP2_CONTROL_SET,                                            \
            CFG0LOGLVL,                                                   \
            loglvl,                                                       \
            strlen(loglvl),                                               \
            connector_unittest.connector_id,                              \
            (TickType_t)portMAX_DELAY);                                   \
    }
    struct ddm2_class_owned
    {
        uint32_t parameter;
        void *raw_value;
        size_t raw_value_size;
    };

    static RuleEngineEvaluatorIntegrationTestFixture *rule_engine_eval_integration_instance;
    static struct ddm2_class_owned mimic_owner_ddm2_classes[MIMIC_OWNER_DDM2_CLASSES_SIZE];
    int slot_rule_engine = 0;  // has to 0, as the slot 0 is always used by the rule engine lib due the ddmw library dependencies
    int slot_mimic_owner = 1;  // mimic of the owner connector that should publish/set frames for slot 0
    void SetUp() override
    {
        // Note: The rule engine processes tasks through rule_engine_task_process(),
        // which is called from the connector's process_event callback.
        // No direct task handler assignment is needed.
        connector_unittest_enable_indexed_connector(nullptr, rule_engine_task_handler, slot_rule_engine);
        connector_unittest_enable_indexed_connector(nullptr, owner_task_handler, slot_mimic_owner);

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

        DICMFrameworkTestFixture::SetUp();
        DICMFrameworkTestFixture::setConnectorId(&connector_unittest.connector_id);
        DICMFrameworkTestFixture::SetupFramework();

        ddmw_init(&ddm_wrapper, &connector_unittest);
        rule_engine_inst.instance = ddmw_register(&ddm_wrapper, RULE0);
        ASSERT_NE(rule_engine_inst.instance, -1);
        ASSERT_EQ(rule_engine_initialize_instance(&rule_engine_inst, &rule_engine_inst_config), 1) << "Failed to initialize rule engine instance";
        rule_engine_eval_integration_instance = this;
    }

    void TearDown() override
    {
        rule_engine_eval_integration_instance = nullptr;
        memset(mimic_owner_ddm2_classes, 0, sizeof(mimic_owner_ddm2_classes));
        RuleEngineTestFixture::TearDown();
    }

    void verifyVariable(struct rule_engine__specification *spec, const struct expr_var *var,
                        int32_t expectedValue, uint8_t expectedType, uint32_t expectedID, uint8_t expectedTypeID, uint8_t expectedResolutionState)
    {
        ASSERT_NE(var, nullptr) << "Variable " << var->name << " not found in specification";

        EXPECT_EQ(expr_var_value(var), expectedValue)
            << "Rule: " << spec->expr.rule_string << " -> "
            << "Variable " << var->name << " has value " << expr_var_value(var)
            << ", expected " << expectedValue;

        EXPECT_EQ(expr_var_type(var), expectedType)
            << "Rule: " << spec->expr.rule_string << " -> "
            << "Variable " << var->name << " has type " << (int)expr_var_type(var)
            << ", expected " << (int)expectedType;

        EXPECT_EQ(expr_var_id(var), expectedID)
            << "Rule: " << spec->expr.rule_string << " -> "
            << "Variable " << var->name << " has ID 0x" << std::hex << expr_var_id(var)
            << ", expected 0x" << std::hex << expectedID;

        EXPECT_EQ(expr_var_type_id(var), expectedTypeID)
            << "Rule: " << spec->expr.rule_string << " -> "
            << "Variable " << var->name << " has type ID " << (int)expr_var_type_id(var)
            << ", expected " << (int)expectedTypeID;

        EXPECT_EQ(expr_var_type_is_resolved(var), expectedResolutionState)
            << "Rule: " << spec->expr.rule_string << " -> "
            << "Variable " << var->name << " resolution state is "
            << (int)expr_var_type_is_resolved(var)
            << ", expected " << (int)expectedResolutionState;
    }

    /* Task process invoked by the unittest connector.
     * Slot 0 is the unittest connector that should evaluate the received frames from the broker. Rule engine lib is always registering the frames only for the slot 0, as a dependency from the ddmw library.
     * Slot 1 is mimic of an owner connector that should publish/set frames for slot 0 (the rule engine considers it as separate connector comparing to slot 1)
     */
    static void rule_engine_task_handler(DDMP2_FRAME *pframe)
    {
        if (rule_engine_eval_integration_instance != nullptr)
        {
            int index = ddm2_parameter_list_lookup(pframe->frame.publish.parameter);
            switch (Ddm2_parameter_list_data[index].out_type)
            {
            case DDM2_TYPE_INT32_T:
                LOG(D, "rule_engine_task_handler: DDMP2_CONTROL_PUBLISH for parameter=0x%x with value=%d",
                    pframe->frame.publish.parameter, pframe->frame.publish.value.int32);
                break;
            case DDM2_TYPE_UINT32_T:
                LOG(D, "rule_engine_task_handler: DDMP2_CONTROL_PUBLISH for parameter=0x%x with value=%u",
                    pframe->frame.publish.parameter, pframe->frame.publish.value.uint32);
                break;
            case DDM2_TYPE_STRUCT:
                if (pframe->frame.publish.parameter == GW0INV)
                {
                    // Decode
                    uint32_t *params = (uint32_t *)&pframe->frame.publish.value.raw;
                    uint32_t count = ddmp2_value_size(pframe) / 4;
                    for (uint32_t i = 0; i < count; i++)
                    {
                        char name[64];
                        size_t name_size = sizeof(name);
                        LOG(D, "rule_engine_task_handler: DDMP2_CONTROL_PUBLISH for GW0INV for parameter class[%s][0x%X]",
                            ddm2_parameter_name(DDM2_PARAMETER_CLASS_INSTANCE(params[i]), (void *)name, &name_size), params[i]);
                    }
                }

                break;
            default:
                break;
            }
            rule_engine_task_process(&rule_engine_eval_integration_instance->rule_engine_inst, pframe);
        }
        else
        {
            LOG(E, "rule_engine_task_handler: rule_engine_eval_integration_instance is null, cannot process frame");
        }
    }

    int owner_init_class(struct ddm2_class_owned *ddm2_class)
    {
        // request a TEST instance from broker
        int instance = broker_register_instance(&ddm2_class->parameter, connector_unittest.connector_id + slot_mimic_owner);
        if (instance == (int)DDMP2_INVALID_INSTANCE)
        {
            return DDMP2_INVALID_INSTANCE;  // Skip if registration fails and return invalid instance
        }

        return instance;  // Return the registered instance
    }

    /* Mimic owner implementation */
    int owner_init_classes(struct ddm2_class_owned *ddm2_classes, size_t num_classes)
    {
        int instance = DDMP2_INVALID_INSTANCE;
        TRUE_CHECK_RETURNX(DDMP2_INVALID_INSTANCE, num_classes <= MIMIC_OWNER_DDM2_CLASSES_SIZE);
        // Initialize mimic owner DDM2 classes
        for (size_t i = 0; i < num_classes; ++i)
        {
            instance = owner_init_class(&ddm2_classes[i]);
            if (instance == (int)DDMP2_INVALID_INSTANCE)
            {
                LOG(E, "Failed to register instance for parameter=0x%x", ddm2_classes[i].parameter);
                break;
            }
            mimic_owner_ddm2_classes[i].parameter = ddm2_classes[i].parameter | DDM2_PARAMETER_INSTANCE(instance);
        }

        return instance;  // Return the last registered instance or DDMP2_INVALID_INSTANCE if none were registered
    }

    void owner_update_parameter_value(uint32_t parameter, void *value, size_t value_size)
    {
        for (size_t i = 0; i < ELEMENTS(mimic_owner_ddm2_classes); ++i)
        {
            if (mimic_owner_ddm2_classes[i].parameter == parameter)
            {
                mimic_owner_ddm2_classes[i].raw_value = value;
                mimic_owner_ddm2_classes[i].raw_value_size = value_size;
                return;
            }
        }
        LOG(E, "owner_set_value: Parameter 0x%x not found", parameter);
    }

    void owner_update_and_publish_parameter_value(uint32_t parameter, void *value, size_t value_size)
    {
        owner_update_parameter_value(parameter, value, value_size);
        connector_send_frame_to_broker(
            DDMP2_CONTROL_PUBLISH,
            parameter,
            value,
            value_size,
            connector_unittest.connector_id + slot_mimic_owner,
            (TickType_t)portMAX_DELAY);
    }

    /* Sends a SET frame to modify a parameter owned by the rule engine instance */
    void owner_set_rule_engine_owned_parameter(uint32_t parameter, void *value, size_t value_size)
    {
        uint32_t rule_engine_parameter_instance = parameter | DDM2_PARAMETER_INSTANCE(rule_engine_eval_integration_instance->rule_engine_inst.instance);

        connector_send_frame_to_broker(
            DDMP2_CONTROL_SET,
            rule_engine_parameter_instance,
            value,
            value_size,
            connector_unittest.connector_id + rule_engine_eval_integration_instance->slot_mimic_owner,
            (TickType_t)portMAX_DELAY);
    }

    static void owner_task_handler(DDMP2_FRAME *pframe)
    {
        if (pframe->source_connector != (connector_unittest.connector_id + rule_engine_eval_integration_instance->slot_rule_engine))
        {
            return;  // Ignore frames not from the rule engine connector(slot[0])
        }

        for (size_t i = 0; i < ELEMENTS(mimic_owner_ddm2_classes); ++i)
        {
            switch (pframe->frame.control)
            {
            case DDMP2_CONTROL_SUBSCRIBE:
                if (pframe->frame.subscribe.parameter == mimic_owner_ddm2_classes[i].parameter)
                {
                    connector_send_frame_to_broker(
                        DDMP2_CONTROL_PUBLISH,
                        mimic_owner_ddm2_classes[i].parameter,
                        mimic_owner_ddm2_classes[i].raw_value,
                        mimic_owner_ddm2_classes[i].raw_value_size,
                        connector_unittest.connector_id + rule_engine_eval_integration_instance->slot_mimic_owner,
                        (TickType_t)portMAX_DELAY);
                }
                break;
            case DDMP2_CONTROL_SET:
                if (pframe->frame.set.parameter == mimic_owner_ddm2_classes[i].parameter)
                {
                    memcpy(mimic_owner_ddm2_classes[i].raw_value, &pframe->frame.set.value, sizeof(mimic_owner_ddm2_classes[i].raw_value_size));
                    connector_send_frame_to_broker(
                        DDMP2_CONTROL_PUBLISH,
                        mimic_owner_ddm2_classes[i].parameter,
                        mimic_owner_ddm2_classes[i].raw_value,
                        mimic_owner_ddm2_classes[i].raw_value_size,
                        connector_unittest.connector_id + rule_engine_eval_integration_instance->slot_mimic_owner,
                        (TickType_t)portMAX_DELAY);
                }
                break;
            case DDMP2_CONTROL_PUBLISH:
                LOG(E, "owner_task_handler: Received DDMP2_CONTROL_PUBLISH frame for parameter=0x%x from connector=%d, but this is not expected in owner handler",
                    pframe->frame.publish.parameter,
                    pframe->source_connector);
                break;
            default:
                break;
            }
        }
    }
};

// Static member definitions
RuleEngineEvaluatorIntegrationTestFixture *RuleEngineEvaluatorIntegrationTestFixture::rule_engine_eval_integration_instance = nullptr;
RuleEngineEvaluatorIntegrationTestFixture::ddm2_class_owned RuleEngineEvaluatorIntegrationTestFixture::mimic_owner_ddm2_classes[MIMIC_OWNER_DDM2_CLASSES_SIZE] = {};

TEST_F(RuleEngineEvaluatorIntegrationTestFixture, InventoryResolutionInstancesWithoutCallbackSameClassInstances)
{
    const char *rule0_str = "inventory(g_groupX_instance, GROUP0)";
    const char *rule1_str = "inventory(g_groupY_instance, GROUP0)";

    static const struct rule_engine__rule rules[] =
        {
            {.name = "TestRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
            {.name = "TestRule1", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
        };

    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), RULE_ENGINE__OK) << "Failed to add rule: " << rules[0].name;
    EXPECT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[1]), RULE_ENGINE__INV_RULE_FORMAT) << "Rule should not be added as it uses same class instance: " << rules[1].name;
}

TEST_F(RuleEngineEvaluatorIntegrationTestFixture, InventoryResolutionInstancesWithoutCallbackDifferentClassInstances)
{
    DEBUG_LOGS_ENABLE();

    const char *rule0_str = "inventory(g_groupX_instance, GROUP0)";
    const char *rule1_str = "inventory(g_groupY_instance, GROUP2)";

    static const struct rule_engine__rule rules[] =
        {
            {.name = "TestRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
            {.name = "TestRule1", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
        };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    for (size_t i = 0; i < ELEMENTS(rules); ++i)
    {
        ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[i]), RULE_ENGINE__OK) << "Failed to add rule: " << rules[i].name;
    }

    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    struct expr_var *g_groupX_instance_var = getVariableByName(rule0_spec, "g_groupX_instance");
    struct rule_engine__specification *rule1_spec = getSpecificationById(1);
    struct expr_var *g_groupY_instance_var = getVariableByName(rule1_spec, "g_groupY_instance");
    ASSERT_NE(g_groupX_instance_var, nullptr) << "Variable " << g_groupX_instance_var->name << " not found in specification";
    ASSERT_NE(g_groupY_instance_var, nullptr) << "Variable " << g_groupY_instance_var->name << " not found in specification";

    // g_groupX_instance_var
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_ddm_create(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupY_instance_var
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_ddm_create(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Now register the mimic owner DDM2 classes
    mimic_owner_ddm2_classes[0].parameter = GROUP0;  // Instance 0
    int instance_rule_0 = owner_init_class(&mimic_owner_ddm2_classes[0]);
    ASSERT_NE(instance_rule_0, DDMP2_INVALID_INSTANCE) << "Failed to register instance 0 for GROUP0";
    // g_groupX_instance_var should be resolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_ddm_create(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var should remain unresolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_ddm_create(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Now register instance 1. Should be same as before, since instance 1 is not used in the rules
    mimic_owner_ddm2_classes[1].parameter = GROUP0;  // Instance 1
    ASSERT_NE(owner_init_class(&mimic_owner_ddm2_classes[1]), DDMP2_INVALID_INSTANCE) << "Failed to register instance for GROUP1";
    // g_groupX_instance_var should remain resolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_ddm_create(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var should remain unresolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_ddm_create(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Now deregister, g_groupX_instance should be unresolved again, g_groupY_instance should remain unresolved
    uint32_t unavailable = 0;
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[0].parameter, &unavailable, sizeof(unavailable));  // Instance 1
    // g_groupX_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_ddm_create(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupY_instance_var should remain unresolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_ddm_create(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Now register again, should be resolved again
    owner_init_class(&mimic_owner_ddm2_classes[0]);  // Instance 0
    // g_groupX_instance_var
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_ddm_create(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_ddm_create(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Now register the second instance
    mimic_owner_ddm2_classes[2].parameter = GROUP0;  // Instance 2
    int instance_rule_1 = owner_init_class(&mimic_owner_ddm2_classes[2]);
    ASSERT_NE(instance_rule_1, DDMP2_INVALID_INSTANCE) << "Failed to register instance for GROUP2";
    // g_groupX_instance_var should remain resolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_ddm_create(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var should be resolved now
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_1));
    EXPECT_EQ(expr_var_type_ddm_create(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);

    // Now deregister the second instance, g_groupY_instance_var should be unresolved again
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[2].parameter, &unavailable, sizeof(unavailable));  // Instance 2
    // g_groupX_instance_var should remain resolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_ddm_create(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var should be resolved now
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_ddm_create(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    vPortResumeScheduler();
}

/**
 * Test comprehensive inventory resolution with user callback covering:
 * 1. Multiple rules with same inventory class but different callback conditions
 * 2. Dynamic resolution/unresolution based on changing conditions
 * 3. Independent rule behavior (one rule change doesn't affect others)
 * 4. Inventory availability (AVL=1/0) handling
 * 5. Callback decision changes causing rule state transitions
 *
 * Rule 0: Resolves when SmartEco conditions are met (GROUP0ID==SMARTECO && GROUP0TYPE!=0)
 * Rule 1: Resolves when ClimateZone conditions are met (GROUP0ID==CLIMATEZONE && GROUP0TYPE==2)
 */
TEST_F(RuleEngineEvaluatorIntegrationTestFixture, InventoryResolutionInstancesWithCallback)
{
    DEBUG_LOGS_ENABLE();

    const char *rule0_str = "inventory(g_groupX_instance, GROUP0)";
    const char *rule1_str = "inventory(g_groupY_instance, GROUP0)";

    static const struct rule_engine__rule rules[] =
        {
            {.name = "TestRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
            {.name = "TestRule1", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
        };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    rule_engine_set_inventory_user_cb(
        &rule_engine_inst,
        [](uint32_t rule_id, uint32_t ddm2_class, uint32_t current_instance) -> int
        {
            LOG(W, "Inventory user callback for rule %d, class 0x%X, current instance %d",
                rule_id, ddm2_class, current_instance);
            int desired_instance = current_instance;
            switch (DDMP2_INVENTORY_CLASS(ddm2_class))
            {
            case GROUP0:
                if (DDMP2_INVENTORY_AVL(ddm2_class))
                {
                    if (rule_id == 0)
                    {
                        // Check if this inventory update is for instance 0 (SmartEco instance)
                        if (current_instance == DDMP2_INVALID_INSTANCE)
                        {
                            if ((DDM2_PARAMETER_BASE_INSTANCE(mimic_owner_ddm2_classes[2].parameter) == GROUP0TYPE) &&
                                (DDM2_PARAMETER_BASE_INSTANCE(mimic_owner_ddm2_classes[1].parameter) == GROUP0ID))
                            {
                                if ((*(int32_t *)mimic_owner_ddm2_classes[2].raw_value != 0) && (*(int32_t *)mimic_owner_ddm2_classes[1].raw_value == GROUP0TYPE_SMARTECO))
                                {
                                    // SmartEco conditions met, resolve to this instance
                                    desired_instance = DDM2_PARAMETER_INSTANCE_FIELD(ddm2_class);
                                }
                                else
                                {
                                    // SmartEco conditions not met, unresolve
                                }
                            }
                        }
                        else
                        {
                            // Rule 0 already has a resolved instance
                            desired_instance = current_instance;
                        }
                    }
                    else if (rule_id == 1)
                    {
                        // Check if this inventory update is for instance 1 (ClimateZone instance)
                        if (current_instance == DDMP2_INVALID_INSTANCE)
                        {
                            if ((DDM2_PARAMETER_BASE_INSTANCE(mimic_owner_ddm2_classes[5].parameter) == GROUP0TYPE) &&
                                (DDM2_PARAMETER_BASE_INSTANCE(mimic_owner_ddm2_classes[4].parameter) == GROUP0ID))
                            {
                                if ((*(int32_t *)mimic_owner_ddm2_classes[5].raw_value == 2) && (*(int32_t *)mimic_owner_ddm2_classes[4].raw_value == GROUP0TYPE_CLIMATEZONE))
                                {
                                    // ClimateZone conditions met, resolve to this instance
                                    desired_instance = DDM2_PARAMETER_INSTANCE_FIELD(ddm2_class);
                                }
                                else
                                {
                                    // ClimateZone conditions not met, unresolve
                                }
                            }
                        }
                        else
                        {
                            // Rule 1 already has a resolved instance
                            desired_instance = current_instance;
                        }
                    }
                }
                else
                {
                    // Inventory not available - check if this affects our current instance
                    uint32_t unavailable_instance_id = DDM2_PARAMETER_INSTANCE_FIELD(ddm2_class);
                    if (unavailable_instance_id == current_instance)
                    {
                        // Our current instance became unavailable, unresolve
                        desired_instance = DDMP2_INVALID_INSTANCE;
                    }
                    else
                    {
                        // A different instance became unavailable, maintain current state
                        desired_instance = current_instance;
                    }
                }
                break;
            default:
                LOG(I, "Rule %d: Inventory class 0x%X not handled", rule_id, DDMP2_INVENTORY_CLASS(ddm2_class));
                break;
            }
            return desired_instance;
        });

    for (size_t i = 0; i < ELEMENTS(rules); ++i)
    {
        ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[i]), RULE_ENGINE__OK) << "Failed to add rule: " << rules[i].name;
    }

    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    struct expr_var *g_groupX_instance_var = getVariableByName(rule0_spec, "g_groupX_instance");
    struct rule_engine__specification *rule1_spec = getSpecificationById(1);
    struct expr_var *g_groupY_instance_var = getVariableByName(rule1_spec, "g_groupY_instance");
    ASSERT_NE(g_groupX_instance_var, nullptr) << "Variable " << g_groupX_instance_var->name << " not found in specification";
    ASSERT_NE(g_groupY_instance_var, nullptr) << "Variable " << g_groupY_instance_var->name << " not found in specification";

    // Initial state: Both rules should be unresolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Setup instance 0 parameters for rule 0 (SmartEco conditions)
    int32_t group_id_smarteco = GROUP0TYPE_SMARTECO;
    int32_t group_type_smarteco = 1;
    mimic_owner_ddm2_classes[1].parameter = GROUP0ID | DDM2_PARAMETER_INSTANCE(0);
    mimic_owner_ddm2_classes[2].parameter = GROUP0TYPE | DDM2_PARAMETER_INSTANCE(0);
    owner_update_parameter_value(mimic_owner_ddm2_classes[1].parameter, &group_id_smarteco, sizeof(group_id_smarteco));      // Set GROUP0ID to SmartEco
    owner_update_parameter_value(mimic_owner_ddm2_classes[2].parameter, &group_type_smarteco, sizeof(group_type_smarteco));  // Set GROUP0TYPE to non-zero

    // Register GROUP0 instance 0 - rule 0 should resolve
    mimic_owner_ddm2_classes[0].parameter = GROUP0;  // Instance 0
    int instance_rule_0 = owner_init_class(&mimic_owner_ddm2_classes[0]);
    ASSERT_NE(instance_rule_0, DDMP2_INVALID_INSTANCE) << "Failed to register instance 0 for GROUP0";
    EXPECT_EQ(instance_rule_0, 0) << "Expected instance 0";

    // Verify rule 0 resolved with instance 0, rule 1 still unresolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);

    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Setup instance 1 parameters for rule 1 (ClimateZone conditions)
    int32_t group_id_climate = GROUP0TYPE_CLIMATEZONE;
    int32_t group_type_climate = 2;
    mimic_owner_ddm2_classes[4].parameter = GROUP0ID | DDM2_PARAMETER_INSTANCE(1);
    mimic_owner_ddm2_classes[5].parameter = GROUP0TYPE | DDM2_PARAMETER_INSTANCE(1);
    owner_update_parameter_value(mimic_owner_ddm2_classes[4].parameter, &group_id_climate, sizeof(group_id_climate));      // Set GROUP0ID to ClimateZone
    owner_update_parameter_value(mimic_owner_ddm2_classes[5].parameter, &group_type_climate, sizeof(group_type_climate));  // Set GROUP0TYPE to expected value

    // Register GROUP0 instance 1 - rule 1 should resolve
    mimic_owner_ddm2_classes[3].parameter = GROUP0;  // Instance 1
    int instance_rule_1 = owner_init_class(&mimic_owner_ddm2_classes[3]);
    ASSERT_NE(instance_rule_1, DDMP2_INVALID_INSTANCE) << "Failed to register instance 1 for GROUP0";
    EXPECT_EQ(instance_rule_1, 1) << "Expected instance 1";

    // Verify both rules resolved with their respective instances
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);

    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_1));
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);

    // Test scenario: Deregister instance 0 (inventory not available)
    int32_t unavailable = 0;
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[0].parameter, &unavailable, sizeof(unavailable));  // AVL=0 for instance 0

    // When instance 0 becomes unavailable, rule 0 should be unresolved,
    // but rule 1 should remain resolved since it uses instance 1 which is still available
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_1));
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);

    // Test scenario: Deregister instance 1 (inventory not available)
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[3].parameter, &unavailable, sizeof(unavailable));  // AVL=0 for instance 1
    // groupX_instance_var should remain unresolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupY_instance_var should unresolve
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    vPortResumeScheduler();
}

TEST_F(RuleEngineEvaluatorIntegrationTestFixture, DynamicInstancesInvalidOrderOfInstances)
{
    const char *rule0_str = "inventory(g_groupX_instance, GROUP0)";
    const char *rule1_str = "dyn_instance(g_groupZ_instance, group{g_groupY_instance}interface, GROUP0, 2)";
    const char *rule2_str = "dyn_instance(g_groupY_instance, group{g_groupX_instance}interface, GROUP0, 0)";

    static const struct rule_engine__rule rules[] =
        {
            {.name = "TestRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
            {.name = "TestRule1", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
            {.name = "TestRule2", .rule = const_cast<char *>(rule2_str), .size = static_cast<int>(strlen(rule2_str))}};

    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), RULE_ENGINE__OK) << "Failed to add rule: " << rules[0].name;
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[1]), RULE_ENGINE__INV_DYN_VAR_NAME) << "g_groupY_instance cannot be used before it is defined!: " << rules[1].name;
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[2]), RULE_ENGINE__OK) << "Failed to add rule: " << rules[2].name;
}

TEST_F(RuleEngineEvaluatorIntegrationTestFixture, DynamicInstancesDynInstanceFunc)
{
    DEBUG_LOGS_ENABLE();

    const char *rule0_str = "inventory(g_groupX_instance, GROUP0)";
    const char *rule1_str = "dyn_instance(g_groupY_instance, group{g_groupX_instance}interface, GROUP0, 0)";

    static const struct rule_engine__rule rules[] =
        {
            {.name = "TestRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
            {.name = "TestRule1", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
        };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    for (size_t i = 0; i < ELEMENTS(rules); ++i)
    {
        ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[i]), RULE_ENGINE__OK) << "Failed to add rule: " << rules[i].name;
    }

    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    struct expr_var *g_groupX_instance_var = getVariableByName(rule0_spec, "g_groupX_instance");
    struct rule_engine__specification *rule1_spec = getSpecificationById(1);
    struct expr_var *g_groupY_instance_var = getVariableByName(rule1_spec, "g_groupY_instance");
    struct expr_var *group_g_groupX_instance_interface = getVariableByName(rule1_spec, "group{g_groupX_instance}interface");
    ASSERT_NE(g_groupX_instance_var, nullptr) << "Variable " << g_groupX_instance_var->name << " not found in specification";
    ASSERT_NE(g_groupY_instance_var, nullptr) << "Variable " << g_groupY_instance_var->name << " not found in specification";
    ASSERT_NE(group_g_groupX_instance_interface, nullptr) << "Variable " << group_g_groupX_instance_interface->name << " not found in specification";

    // Initial state: Both rules should be unresolved
    // g_groupX_instance_var
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group{g_groupX_instance}interface
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupY_instance_var
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Resolve g_groupX_instance_var by registering GROUP0 instance.
    mimic_owner_ddm2_classes[0].parameter = GROUP0;
    int instance_rule_0 = owner_init_class(&mimic_owner_ddm2_classes[0]);
    ASSERT_NE(instance_rule_0, DDMP2_INVALID_INSTANCE);

    // Now g_groupX_instance_var should be resolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group{g_groupX_instance}interface should be resolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // Now g_groupY_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Unregister GROUP0 instance.
    int32_t unavailable = 0;
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[0].parameter, &unavailable, sizeof(unavailable));  // AVL=0 for instance 0
    // g_groupX_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group{g_groupX_instance}interface should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupY_instance_var should remain unresolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Now register GROUP0 instance again.
    instance_rule_0 = owner_init_class(&mimic_owner_ddm2_classes[0]);
    ASSERT_NE(instance_rule_0, DDMP2_INVALID_INSTANCE);
    // g_groupX_instance_var should be resolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group{g_groupX_instance}interface should be resolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Now register GROUP0 instance with different instance ID.
    mimic_owner_ddm2_classes[1].parameter = GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0 + 1);  // next_instance;
    int instance_rule_1 = owner_init_class(&mimic_owner_ddm2_classes[1]);
    ASSERT_NE(instance_rule_1, DDMP2_INVALID_INSTANCE);
    // Use it to update GROUP0INTERFACE with different instance ID i.e. LINKEDCLASS_T
    uint32_t classes[] = {GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_1), PROD0};
    mimic_owner_ddm2_classes[2].parameter = GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0);
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[2].parameter, &classes, sizeof(classes));

    // g_groupX_instance_var should be resolved with same instance ID
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group{g_groupX_instance}interface should be resolved with same instance ID
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var should be resolved with different instance ID
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_1));  // instance_rule_0 + 1
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);

    // Now unregister GROUP1 instance with instance_rule_1 and publish GROUP0INTERFACE with updated classes.
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[1].parameter, &unavailable, sizeof(unavailable));  // AVL=0 for instance 1
    uint32_t classes_lost_group1[] = {PROD0};
    memcpy(classes, classes_lost_group1, sizeof(classes_lost_group1));
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[2].parameter, &classes, sizeof(classes));
    // g_groupX_instance_var should be resolved with instance_rule_0
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group{g_groupX_instance}interface should be resolved with instance_rule_0
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Now register GROUP0 instance again with different instance ID.
    mimic_owner_ddm2_classes[1].parameter = GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0 + 2);  // next_instance;
    instance_rule_1 = owner_init_class(&mimic_owner_ddm2_classes[1]);
    ASSERT_NE(instance_rule_1, DDMP2_INVALID_INSTANCE);
    // Use it to update GROUP0INTERFACE with different instance ID i.e. LINKEDCLASS_T
    uint32_t classes_lost_group2[] = {GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_1), PROD0};
    memcpy(classes, classes_lost_group2, sizeof(classes_lost_group2));
    mimic_owner_ddm2_classes[2].parameter = GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0);
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[2].parameter, &classes, sizeof(classes));
    // g_groupX_instance_var should be resolved with same instance ID
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group{g_groupX_instance}interface should be resolved with same instance ID
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var should be resolved with different instance ID (instance_rule_0 + 2)
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_1));  // instance_rule_0 + 2
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);

    // Now unregister GROUP0 instance with instance_rule_0 and verify that both instances and dynamic paramter are unresloved.
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[0].parameter, &unavailable, sizeof(unavailable));  // AVL=0 for instance 0
    // g_groupX_instance_var should be resolved with instance_rule_0
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group{g_groupX_instance}interface should be resolved with instance_rule_0
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupY_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    vPortResumeScheduler();
}

TEST_F(RuleEngineEvaluatorIntegrationTestFixture, DynamicInstancesDynInstanceFuncSameInstance)
{
    DEBUG_LOGS_ENABLE();

    enum mimic_owner_slots
    {
        SLOT_GROUPX = 0,
        SLOT_GROUPY,
        SLOT_GROUP2,
        SLOT_GROUP3,
        SLOT_GROUPXINTERFACE,
        SLOT_GROUPYINTERFACE
    };

    const char *rule0_str = "inventory(g_groupX_instance, GROUP0)";
    const char *rule1_str = "dyn_instance(g_groupY_instance, group{g_groupX_instance}interface, GROUP0, 0)";
    const char *rule2_str = "dyn_instance(g_groupZ_instance, group{g_groupX_instance}interface, GROUP0, 2)";

    static const struct rule_engine__rule rules[] =
        {
            {.name = "TestRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
            {.name = "TestRule1", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
            {.name = "TestRule2", .rule = const_cast<char *>(rule2_str), .size = static_cast<int>(strlen(rule2_str))}};

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    for (size_t i = 0; i < ELEMENTS(rules); ++i)
    {
        ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[i]), RULE_ENGINE__OK) << "Failed to add rule: " << rules[i].name;
    }

    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    struct expr_var *g_groupX_instance_var = getVariableByName(rule0_spec, "g_groupX_instance");
    struct rule_engine__specification *rule1_spec = getSpecificationById(1);
    struct expr_var *g_groupY_instance_var = getVariableByName(rule1_spec, "g_groupY_instance");
    struct expr_var *group_g_groupX_instance_interface_rule1 = getVariableByName(rule1_spec, "group{g_groupX_instance}interface");
    struct rule_engine__specification *rule2_spec = getSpecificationById(2);
    struct expr_var *g_groupZ_instance_var = getVariableByName(rule2_spec, "g_groupZ_instance");
    struct expr_var *group_g_groupX_instance_interface_rule2 = getVariableByName(rule1_spec, "group{g_groupX_instance}interface");
    ASSERT_NE(g_groupX_instance_var, nullptr) << "Variable " << g_groupX_instance_var->name << " not found in specification";
    ASSERT_NE(g_groupY_instance_var, nullptr) << "Variable " << g_groupY_instance_var->name << " not found in specification";
    ASSERT_NE(g_groupZ_instance_var, nullptr) << "Variable " << g_groupZ_instance_var->name << " not found in specification";
    ASSERT_NE(group_g_groupX_instance_interface_rule1, nullptr) << "Variable " << group_g_groupX_instance_interface_rule1->name << " not found in specification";
    ASSERT_NE(group_g_groupX_instance_interface_rule2, nullptr) << "Variable " << group_g_groupX_instance_interface_rule2->name << " not found in specification";

    // g_groupX_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupX_instance_interface_rule1 should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_rule1), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_rule1), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupY_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupX_instance_interface_rule2 should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_rule1), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_rule1), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupZ_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupZ_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupZ_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupZ_instance_var), 2);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Resolve g_groupX_instance_var by registering GROUP0 instance.
    mimic_owner_ddm2_classes[SLOT_GROUPX].parameter = GROUP0;
    int instance_rule_0 = owner_init_class(&mimic_owner_ddm2_classes[SLOT_GROUPX]);
    ASSERT_NE(instance_rule_0, DDMP2_INVALID_INSTANCE);  // Ensure the instance is valid
    // Now g_groupX_instance_var should be resolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupX_instance_interface_rule1 should be resolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_rule1), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_rule1), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupX_instance_interface_rule2 should be resolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_rule2), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_rule2), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupZ_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupZ_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupZ_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupZ_instance_var), 2);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Unregister GROUP0 instance.
    int32_t unavailable = 0;
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_GROUPX].parameter, &unavailable, sizeof(unavailable));  // AVL=0 for instance 0
    // g_groupX_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupX_instance_interface_rule1 should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_rule1), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_rule1), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupY_instance_var should remain unresolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupX_instance_interface_rule2 should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_rule2), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_rule2), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupZ_instance_var should remain unresolved
    EXPECT_EQ(expr_var_value(g_groupZ_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupZ_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupZ_instance_var), 2);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Now register GROUP0 instance again.
    instance_rule_0 = owner_init_class(&mimic_owner_ddm2_classes[SLOT_GROUPX]);
    ASSERT_NE(instance_rule_0, DDMP2_INVALID_INSTANCE);
    // g_groupX_instance_var should be resolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupX_instance_interface_rule1 should be resolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_rule1), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_rule1), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var should remain unresolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupX_instance_interface_rule2 should be resolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_rule2), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_rule2), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupZ_instance_var should remain unresolved
    EXPECT_EQ(expr_var_value(g_groupZ_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupZ_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupZ_instance_var), 2);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Now register GROUP0 instance with different instance ID.
    mimic_owner_ddm2_classes[SLOT_GROUPY].parameter = GROUP0;  // next_instance;
    int instance_rule_1 = owner_init_class(&mimic_owner_ddm2_classes[SLOT_GROUPY]);
    ASSERT_NE(instance_rule_1, DDMP2_INVALID_INSTANCE);
    // Use it to update GROUP0INTERFACE with different instance ID i.e. LINKEDCLASS_T
    uint32_t classes[] =
        {
            GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_1),
            PROD0};
    mimic_owner_ddm2_classes[SLOT_GROUPXINTERFACE].parameter = GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0);
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_GROUPXINTERFACE].parameter, &classes, sizeof(classes));
    // g_groupX_instance_var should remain resolved with instance_rule_0
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupX_instance_interface_rule1 should remain resolved with instance_rule_0
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_rule1), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_rule1), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var should resolve with different instance ID (instance_rule_1)
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_1));  // instance_rule_0 + 1
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupX_instance_interface_rule2 should remain resolved with instance_rule_0
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_rule2), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_rule2), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupZ_instance_var should remain unresolved
    EXPECT_EQ(expr_var_value(g_groupZ_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupZ_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupZ_instance_var), 2);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Now register GROUP0 instance with different instance ID.
    mimic_owner_ddm2_classes[SLOT_GROUP2].parameter = GROUP0;  // next_instance;
    int instance_rule_2 = owner_init_class(&mimic_owner_ddm2_classes[SLOT_GROUP2]);
    ASSERT_NE(instance_rule_2, DDMP2_INVALID_INSTANCE);
    // Use it to update GROUP0INTERFACE with different instance ID i.e. LINKEDCLASS_T
    uint32_t classes_updated_group2[] =
        {
            GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_1),
            PROD0,
            GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_2)};
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_GROUPXINTERFACE].parameter, &classes_updated_group2, sizeof(classes_updated_group2));
    // g_groupX_instance_var should remain resolved with instance_rule_0
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupX_instance_interface_rule1 should remain resolved with instance_rule_0
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_rule1), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_rule1), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var should resolve with different instance ID (instance_rule_1)
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_1));  // instance_rule_0 + 1
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupX_instance_interface_rule2 should resolve with different instance ID (instance_rule_1)
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_rule2), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_rule2), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupZ_instance_var should remain unresolved
    EXPECT_EQ(expr_var_value(g_groupZ_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupZ_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupZ_instance_var), 2);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Now register GROUP0 instance with different instance ID.
    mimic_owner_ddm2_classes[SLOT_GROUP3].parameter = GROUP0;  // next_instance;
    int instance_rule_3 = owner_init_class(&mimic_owner_ddm2_classes[SLOT_GROUP3]);
    ASSERT_NE(instance_rule_3, DDMP2_INVALID_INSTANCE);
    // Use it to update GROUP0INTERFACE with different instance ID i.e. LINKEDCLASS_T
    uint32_t classes_updated_group3[] =
        {
            GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_1),
            PROD0,
            GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_2),
            GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_3)};
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_GROUPXINTERFACE].parameter, &classes_updated_group3, sizeof(classes_updated_group3));
    // g_groupX_instance_var should remain resolved with instance_rule_0
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupX_instance_interface_rule1 should remain resolved with instance_rule_0
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_rule1), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_rule1), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var should remain resolved with different instance ID (instance_rule_1)
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_1));  //  0x1F010100
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupX_instance_interface_rule2 should resolve with different instance ID (instance_rule_2)
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_rule2), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_rule2), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupZ_instance_var should be resolved with different instance ID (instance_rule_3)
    EXPECT_EQ(expr_var_value(g_groupZ_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupZ_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_3));  // 0x1F010300
    EXPECT_EQ(expr_var_type_id(g_groupZ_instance_var), 2);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);

    // Now deregister GROUP0 instance 2 and remove it from GROUP0INTERFACE
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_GROUP2].parameter, &unavailable, sizeof(unavailable));  // AVL=0 for instance 2
    // Use it to update GROUP0INTERFACE with different instance ID i.e. LINKEDCLASS_T
    uint32_t classes_remove_group2[] =
        {
            GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_1),
            PROD0,
            GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_3)};
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_GROUPXINTERFACE].parameter, &classes_remove_group2, sizeof(classes_remove_group2));
    // g_groupX_instance_var should remain resolved with instance_rule_0
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));  //  0x1F010000
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupX_instance_interface_rule1 should remain resolved with instance_rule_0
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_rule1), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));  //  0x1F010006
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_rule1), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var should remain resolved with instance_rule_1
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_1));  //  0x1F010100
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupX_instance_interface_rule2 should remain resolved with instance_rule_0
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_rule2), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));  //  0x1F010006
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_rule2), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupZ_instance_var should unresolved
    EXPECT_EQ(expr_var_value(g_groupZ_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupZ_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupZ_instance_var), 2);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Now register GROUP0instance with reused instance 2 by rthe broker.
    mimic_owner_ddm2_classes[SLOT_GROUP2].parameter = GROUP0;  // broker will reuse instance 2;
    instance_rule_2 = owner_init_class(&mimic_owner_ddm2_classes[SLOT_GROUP2]);
    ASSERT_NE(instance_rule_2, DDMP2_INVALID_INSTANCE);
    // Use it to update GROUP0INTERFACE with different instance ID i.e. LINKEDCLASS_T
    uint32_t classes_reapply_group2[] = {
        GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_1),
        PROD0,
        GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_3),
        GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_2)};
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_GROUPXINTERFACE].parameter, &classes_reapply_group2, sizeof(classes_reapply_group2));
    // g_groupX_instance_var should remain resolved with instance_rule_0
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));  //  0x1F010000
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupX_instance_interface_rule1 should remain resolved with instance_rule_0
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_rule1), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));  //  0x1F010006
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_rule1), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var should remain resolved with instance_rule_1
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_1));  //  0x1F010100
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupX_instance_interface_rule2 should remain resolved with instance_rule_0
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_rule2), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));  //  0x1F010006
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_rule2), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupZ_instance_var should resolved
    EXPECT_EQ(expr_var_value(g_groupZ_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupZ_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_2));  //  0x1F010200
    EXPECT_EQ(expr_var_type_id(g_groupZ_instance_var), 2);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);

    // Now deregister GROUP0 instance 0. All should unresolve.
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_GROUPX].parameter, &unavailable, sizeof(unavailable));  // AVL=0 for instance 0
    // g_groupX_instance_var should unresolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupX_instance_interface_rule1 should unresolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_rule1), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_rule1), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_rule1), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupY_instance_var should remain unresolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupX_instance_interface_rule2 should unresolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_rule2), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_rule2), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_rule2), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupZ_instance_var should unresolved
    EXPECT_EQ(expr_var_value(g_groupZ_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupZ_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupZ_instance_var), 2);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    vPortResumeScheduler();
}

TEST_F(RuleEngineEvaluatorIntegrationTestFixture, DynamicInstancesDynInstanceFuncDifferentInstanceReverseInstanceOrderUsage)
{
    DEBUG_LOGS_ENABLE();

    enum mimic_owner_slots
    {
        SLOT_GROUPX = 0,
        SLOT_GROUPY,
        SLOT_GROUPZ,
        SLOT_GROUPXINTERFACE,
        SLOT_GROUPYINTERFACE
    };

    const char *rule0_str = "inventory(g_groupX_instance, GROUP0)";
    const char *rule1_str = "dyn_instance(g_groupY_instance, group{g_groupX_instance}interface, GROUP0, 0)";
    const char *rule2_str = "dyn_instance(g_groupZ_instance, group{g_groupY_instance}interface, GROUP0, 2)";

    static const struct rule_engine__rule rules[] =
        {
            {.name = "TestRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
            {.name = "TestRule1", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
            {.name = "TestRule2", .rule = const_cast<char *>(rule2_str), .size = static_cast<int>(strlen(rule2_str))}};

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    rule_engine_set_inventory_user_cb(
        &rule_engine_inst,
        [](uint32_t rule_id, uint32_t ddm2_class, uint32_t current_instance) -> int
        {
            int desired_instance = current_instance;
            switch (DDMP2_INVENTORY_CLASS(ddm2_class))
            {
            case GROUP0:
                if (DDMP2_INVENTORY_AVL(ddm2_class))
                {
                    if (rule_id == 0)
                    {
                        // Check if this inventory update is for instance 2
                        LOG(I, "Rule %d: Inventory class 0x%X for GROUP0 instance %d", rule_id, ddm2_class, DDM2_PARAMETER_INSTANCE_FIELD(ddm2_class));
                        if (current_instance == DDMP2_INVALID_INSTANCE)
                        {
                            LOG(I, "Rule %d: Inventory class 0x%X for GROUP0 instance %d", rule_id, ddm2_class, DDM2_PARAMETER_INSTANCE_FIELD(ddm2_class));
                            if (DDMP2_INVENTORY_CLASS_INSTANCE(ddm2_class) == (GROUP0 | DDM2_PARAMETER_INSTANCE(2)))
                            {
                                desired_instance = DDM2_PARAMETER_INSTANCE_FIELD(ddm2_class);
                                LOG(I, "Rule %d: Inventory class 0x%X for GROUP0 instance %d resolved to %d", rule_id, ddm2_class, DDM2_PARAMETER_INSTANCE_FIELD(ddm2_class), desired_instance);
                            }
                        }
                        else
                        {
                            // Rule 0 already has a resolved instance
                            desired_instance = current_instance;
                        }
                    }
                    else
                    {
                        // Do not care, we have only one 'inventory' rule for GROUP0
                    }
                }
                else
                {
                    // Inventory not available - check if this affects our current instance
                    uint32_t unavailable_instance_id = DDM2_PARAMETER_INSTANCE_FIELD(ddm2_class);
                    if (unavailable_instance_id == current_instance)
                    {
                        // Our current instance became unavailable, unresolve
                        desired_instance = DDMP2_INVALID_INSTANCE;
                    }
                    else
                    {
                        // A different instance became unavailable, maintain current state
                        desired_instance = current_instance;
                    }
                }
                break;
            default:
                LOG(I, "Rule %d: Inventory class 0x%X not handled", rule_id, DDMP2_INVENTORY_CLASS(ddm2_class));
                break;
            }
            return desired_instance;
        });

    for (size_t i = 0; i < ELEMENTS(rules); ++i)
    {
        ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[i]), RULE_ENGINE__OK) << "Failed to add rule: " << rules[i].name;
    }

    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    struct expr_var *g_groupX_instance_var = getVariableByName(rule0_spec, "g_groupX_instance");
    struct rule_engine__specification *rule1_spec = getSpecificationById(1);
    struct expr_var *g_groupY_instance_var = getVariableByName(rule1_spec, "g_groupY_instance");
    struct expr_var *group_g_groupX_instance_interface = getVariableByName(rule1_spec, "group{g_groupX_instance}interface");
    struct rule_engine__specification *rule2_spec = getSpecificationById(2);
    struct expr_var *g_groupZ_instance_var = getVariableByName(rule2_spec, "g_groupZ_instance");
    struct expr_var *group_g_groupY_instance_interface = getVariableByName(rule2_spec, "group{g_groupY_instance}interface");
    ASSERT_NE(g_groupX_instance_var, nullptr) << "Variable " << g_groupX_instance_var->name << " not found in specification";
    ASSERT_NE(g_groupY_instance_var, nullptr) << "Variable " << g_groupY_instance_var->name << " not found in specification";
    ASSERT_NE(g_groupZ_instance_var, nullptr) << "Variable " << g_groupZ_instance_var->name << " not found in specification";
    ASSERT_NE(group_g_groupX_instance_interface, nullptr) << "Variable " << group_g_groupX_instance_interface->name << " not found in specification";
    ASSERT_NE(group_g_groupY_instance_interface, nullptr) << "Variable " << group_g_groupY_instance_interface->name << " not found in specification";

    // g_groupX_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupX_instance_interface should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupY_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupY_instance_interface should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupY_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupY_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupY_instance_interface), 1);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupZ_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupZ_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupZ_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupZ_instance_var), 2);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Now register GROUP0 instance with instance ID 0.
    mimic_owner_ddm2_classes[SLOT_GROUPX].parameter = GROUP0;  // next_instance
    int instance_rule_2 = owner_init_class(&mimic_owner_ddm2_classes[SLOT_GROUPX]);
    ASSERT_NE(instance_rule_2, DDMP2_INVALID_INSTANCE) << "Failed to initialize GROUP0 instance";
    // g_groupX_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupX_instance_interface should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupY_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupY_instance_interface should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupY_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupY_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupY_instance_interface), 1);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupZ_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupZ_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupZ_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupZ_instance_var), 2);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Now register GROUP0 instance with instance next ID 1.
    mimic_owner_ddm2_classes[SLOT_GROUPY].parameter = GROUP0;  // next_instance
    int instance_rule_1 = owner_init_class(&mimic_owner_ddm2_classes[SLOT_GROUPY]);
    ASSERT_NE(instance_rule_1, DDMP2_INVALID_INSTANCE) << "Failed to initialize GROUP0 instance";
    // g_groupX_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupX_instance_interface should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupY_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupY_instance_interface should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupY_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupY_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupY_instance_interface), 1);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupZ_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupZ_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupZ_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupZ_instance_var), 2);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Now register GROUP0 instance with instance next ID 2.
    mimic_owner_ddm2_classes[SLOT_GROUPZ].parameter = GROUP0;  // next_instance
    int instance_rule_0 = owner_init_class(&mimic_owner_ddm2_classes[SLOT_GROUPZ]);
    ASSERT_NE(instance_rule_0, DDMP2_INVALID_INSTANCE) << "Failed to initialize GROUP0 instance";
    // g_groupX_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupX_instance_interface should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupY_instance_interface should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupY_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupY_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupY_instance_interface), 1);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupZ_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupZ_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupZ_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupZ_instance_var), 2);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Now update GROUP0INTERFACE with GROUP0
    mimic_owner_ddm2_classes[SLOT_GROUPYINTERFACE].parameter = GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_1);
    uint32_t classes_group_g_groupY_instance_interface[] =
        {
            GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_2),
        };
    // Should not affect resolution of g_groupY_instance_var, group_g_groupY_instance_interface and g_groupZ_instance_var
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_GROUPYINTERFACE].parameter, &classes_group_g_groupY_instance_interface, sizeof(classes_group_g_groupY_instance_interface));
    // g_groupX_instance_var should remain resolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupX_instance_interface should remain resolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupY_instance_interface should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupY_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupY_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupY_instance_interface), 1);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupZ_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupZ_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupZ_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupZ_instance_var), 2);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    // Now update GROUP0INTERFACE with GROUP0
    mimic_owner_ddm2_classes[SLOT_GROUPXINTERFACE].parameter = GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0);
    uint32_t classes_group_g_groupX_instance_interface[] =
        {
            GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_1),
        };
    // Should affect resolution of g_groupY_instance_var and with that group_g_groupY_instance_interface but not g_groupZ_instance_var since should resolve on 2 position
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_GROUPXINTERFACE].parameter, &classes_group_g_groupX_instance_interface, sizeof(classes_group_g_groupX_instance_interface));
    // g_groupX_instance_var should remain resolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupX_instance_interface should remain resolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var should be resolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_1));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupY_instance_interface should be resolved
    EXPECT_EQ(expr_var_value(group_g_groupY_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupY_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_1));
    EXPECT_EQ(expr_var_type_id(group_g_groupY_instance_interface), 1);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupZ_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupZ_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupZ_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupZ_instance_var), 2);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    uint32_t classes_update_group_g_groupX_instance_interface[] =
        {
            GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_1),
            GROUP0 | DDM2_PARAMETER_INSTANCE(7),
            GROUP0 | DDM2_PARAMETER_INSTANCE(9),
        };
    // Should affect resolution of g_groupZ_instance_var as we are publishing with GROUP class on 2 position
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_GROUPYINTERFACE].parameter, &classes_update_group_g_groupX_instance_interface, sizeof(classes_update_group_g_groupX_instance_interface));
    // g_groupX_instance_var should remain resolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupX_instance_interface should remain resolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var should remain resolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_1));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupY_instance_interface should remain resolved
    EXPECT_EQ(expr_var_value(group_g_groupY_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupY_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_1));
    EXPECT_EQ(expr_var_type_id(group_g_groupY_instance_interface), 1);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupZ_instance_var should be resolved
    EXPECT_EQ(expr_var_value(g_groupZ_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupZ_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(9));
    EXPECT_EQ(expr_var_type_id(g_groupZ_instance_var), 2);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);

    // Now remove everything from GROUPXINTERFACE
    // Should affect unresolution of g_groupY_instance, group{g_groupY_instance}interface and g_groupZ_instance
    uint32_t classes_remove_from_group_g_groupX_instance_interface[] = {};  // Empty array to remove all instances
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_GROUPXINTERFACE].parameter, &classes_remove_from_group_g_groupX_instance_interface, sizeof(classes_remove_from_group_g_groupX_instance_interface));
    // g_groupX_instance_var should remain resolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupX_instance_interface should remain resolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_rule_0));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var should unresolve
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupY_instance_interface should unresolve
    EXPECT_EQ(expr_var_value(group_g_groupY_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupY_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupY_instance_interface), 1);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupZ_instance_var should unresolve
    EXPECT_EQ(expr_var_value(g_groupZ_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupZ_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupZ_instance_var), 2);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupZ_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    vPortResumeScheduler();
}

TEST_F(RuleEngineEvaluatorIntegrationTestFixture, ComplexDynamicResolution)
{
    DEBUG_LOGS_ENABLE();

    enum mimic_owner_slot
    {
        SLOT_GROUPX = 0,
        SLOT_GROUPY,
        SLOT_PRODMBAT,
        SLOT_MBAT,
        SLOT_PROD,
        SLOT_AC,
        SLOT_GROUPXINTERFACE,
        SLOT_GROUPYINTERFACE,
        SLOT_PRODMBATCLIST,
        SLOT_PRODCLIST,
    };
    const char *rule0_str = "inventory(g_groupX_instance, GROUP0)";
    const char *rule1_str = "dyn_instance(g_groupY_instance, group{g_groupX_instance}interface, GROUP0, 0)";
    const char *rule2_str = "dyn_instance(g_group_prod_mabat_instance, group{g_groupX_instance}interface, PROD0, 0)";
    const char *rule3_str = "dyn_instance(g_mbat_instance, prod{g_group_prod_mabat_instance}clist, MBAT0, 0)";
    const char *rule4_str = "dyn_instance(g_group_prod_instance, group{g_groupY_instance}interface, PROD0, 0)";
    const char *rule5_str = "dyn_instance(g_ac_instance, prod{g_group_prod_instance}clist, AC0, 0)";
    const char *rule6_str = "mbat{g_mbat_instance}soc, counter = 0: g_mbatsoc = mbat{g_mbat_instance}soc, counter = counter + 1";
    const char *rule7_str = "ac{g_ac_instance}ttemp, counter = 0: g_acttemp = ac{g_ac_instance}ttemp, counter = counter + 1";

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
        };

    // Load first rule
    for (size_t i = 0; i < ELEMENTS(rules); ++i)
    {
        ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[i]), 1) << "Failed to add rule: " << rules[i].name;
    }

    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    struct expr_var *g_groupX_instance_var = getVariableByName(rule0_spec, "g_groupX_instance");
    struct rule_engine__specification *rule1_spec = getSpecificationById(1);
    struct expr_var *g_groupY_instance_var = getVariableByName(rule1_spec, "g_groupY_instance");
    struct expr_var *group_g_groupX_instance_interface_1 = getVariableByName(rule1_spec, "group{g_groupX_instance}interface");
    struct rule_engine__specification *rule2_spec = getSpecificationById(2);
    struct expr_var *g_group_prod_mabat_instance = getVariableByName(rule2_spec, "g_group_prod_mabat_instance");
    struct expr_var *group_g_groupX_instance_interface_2 = getVariableByName(rule2_spec, "group{g_groupX_instance}interface");
    struct rule_engine__specification *rule3_spec = getSpecificationById(3);
    struct expr_var *g_mbat_instance = getVariableByName(rule3_spec, "g_mbat_instance");
    struct expr_var *prod_g_group_prod_mabat_instance_clist = getVariableByName(rule3_spec, "prod{g_group_prod_mabat_instance}clist");
    struct rule_engine__specification *rule4_spec = getSpecificationById(4);
    struct expr_var *g_group_prod_instance = getVariableByName(rule4_spec, "g_group_prod_instance");
    struct expr_var *group_g_groupY_instance_interface = getVariableByName(rule4_spec, "group{g_groupY_instance}interface");
    struct rule_engine__specification *rule5_spec = getSpecificationById(5);
    struct expr_var *g_ac_instance = getVariableByName(rule5_spec, "g_ac_instance");
    struct expr_var *prod_g_group_prod_instance_clist = getVariableByName(rule5_spec, "prod{g_group_prod_instance}clist");
    struct rule_engine__specification *rule6_spec = getSpecificationById(6);
    struct expr_var *g_mbatsoc = getVariableByName(rule6_spec, "g_mbatsoc");
    struct expr_var *mbat_g_mbat_instance_soc = getVariableByName(rule6_spec, "mbat{g_mbat_instance}soc");
    struct rule_engine__specification *rule7_spec = getSpecificationById(7);
    struct expr_var *g_acttemp = getVariableByName(rule7_spec, "g_acttemp");
    struct expr_var *ac_g_ac_instance_ttemp = getVariableByName(rule7_spec, "ac{g_ac_instance}ttemp");
    ASSERT_NE(g_groupX_instance_var, nullptr) << "Variable " << g_groupX_instance_var->name << " not found in specification";
    ASSERT_NE(g_groupY_instance_var, nullptr) << "Variable " << g_groupY_instance_var->name << " not found in specification";
    ASSERT_NE(group_g_groupX_instance_interface_1, nullptr) << "Variable " << group_g_groupX_instance_interface_1->name << " not found in specification";
    ASSERT_NE(g_group_prod_mabat_instance, nullptr) << "Variable " << g_group_prod_mabat_instance->name << " not found in specification";
    ASSERT_NE(group_g_groupX_instance_interface_2, nullptr) << "Variable " << group_g_groupX_instance_interface_2->name << " not found in specification";
    ASSERT_NE(g_mbat_instance, nullptr) << "Variable " << g_mbat_instance->name << " not found in specification";
    ASSERT_NE(prod_g_group_prod_mabat_instance_clist, nullptr) << "Variable " << prod_g_group_prod_mabat_instance_clist->name << " not found in specification";
    ASSERT_NE(g_group_prod_instance, nullptr) << "Variable " << g_group_prod_instance->name << " not found in specification";
    ASSERT_NE(group_g_groupY_instance_interface, nullptr) << "Variable " << group_g_groupY_instance_interface->name << " not found in specification";
    ASSERT_NE(g_ac_instance, nullptr) << "Variable " << g_ac_instance->name << " not found in specification";
    ASSERT_NE(prod_g_group_prod_instance_clist, nullptr) << "Variable " << prod_g_group_prod_instance_clist->name << " not found in specification";
    ASSERT_NE(g_mbatsoc, nullptr) << "Variable " << g_mbatsoc->name << " not found in specification";
    ASSERT_NE(mbat_g_mbat_instance_soc, nullptr) << "Variable " << mbat_g_mbat_instance_soc->name << " not found in specification";
    ASSERT_NE(g_acttemp, nullptr) << "Variable " << g_acttemp->name << " not found in specification";
    ASSERT_NE(ac_g_ac_instance_ttemp, nullptr) << "Variable " << ac_g_ac_instance_ttemp->name << " not found in specification";

    // g_groupX_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupX_instance_interface_1 should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_1), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_1), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_1), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_1), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_1), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupY_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupY_instance_interface should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupY_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupY_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupY_instance_interface), 1);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_group_prod_mabat_instance should be unresolved
    EXPECT_EQ(expr_var_value(g_group_prod_mabat_instance), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_group_prod_mabat_instance), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_group_prod_mabat_instance), PROD0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_group_prod_mabat_instance), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_group_prod_mabat_instance), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupX_instance_interface_2 should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_2), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_2), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_2), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_2), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_2), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_mbat_instance should be unresolved
    EXPECT_EQ(expr_var_value(g_mbat_instance), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_mbat_instance), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_mbat_instance), MBAT0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_mbat_instance), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_mbat_instance), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // prod_g_group_prod_mabat_instance_clist should be unresolved
    EXPECT_EQ(expr_var_value(prod_g_group_prod_mabat_instance_clist), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(prod_g_group_prod_mabat_instance_clist), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(prod_g_group_prod_mabat_instance_clist), PROD0CLIST | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(prod_g_group_prod_mabat_instance_clist), 0);
    EXPECT_EQ(expr_var_type_is_resolved(prod_g_group_prod_mabat_instance_clist), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_group_prod_instance should be unresolved
    EXPECT_EQ(expr_var_value(g_group_prod_instance), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_group_prod_instance), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_group_prod_instance), PROD0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_group_prod_instance), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_group_prod_instance), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupY_instance_interface should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupY_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupY_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupY_instance_interface), 1);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_ac_instance should be unresolved
    EXPECT_EQ(expr_var_value(g_ac_instance), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_ac_instance), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_ac_instance), AC0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_ac_instance), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_ac_instance), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // prod_g_group_prod_instance_clist should be unresolved
    EXPECT_EQ(expr_var_value(prod_g_group_prod_instance_clist), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(prod_g_group_prod_instance_clist), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(prod_g_group_prod_instance_clist), PROD0CLIST | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(prod_g_group_prod_instance_clist), 1);
    EXPECT_EQ(expr_var_type_is_resolved(prod_g_group_prod_instance_clist), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_mbatsoc should be resolved
    EXPECT_EQ(expr_var_value(g_mbatsoc), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_mbatsoc), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_id(g_mbatsoc), expr_generate_hash_key("g_mbatsoc"));
    EXPECT_EQ(expr_var_type_id(g_mbatsoc), RULE_ENGINE__VAR_TYTE_ID_UNAPLICABLE);
    EXPECT_EQ(expr_var_type_is_resolved(g_mbatsoc), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // mbat_g_mbat_instance_soc should be unresolved
    EXPECT_EQ(expr_var_value(mbat_g_mbat_instance_soc), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(mbat_g_mbat_instance_soc), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(mbat_g_mbat_instance_soc), MBAT0SOC | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(mbat_g_mbat_instance_soc), 0);
    EXPECT_EQ(expr_var_type_is_resolved(mbat_g_mbat_instance_soc), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_acttemp should be resolved
    EXPECT_EQ(expr_var_value(g_acttemp), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_acttemp), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_id(g_acttemp), expr_generate_hash_key("g_acttemp"));
    EXPECT_EQ(expr_var_type_id(g_acttemp), RULE_ENGINE__VAR_TYTE_ID_UNAPLICABLE);
    EXPECT_EQ(expr_var_type_is_resolved(g_acttemp), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // ac_g_ac_instance_ttemp should be unresolved
    EXPECT_EQ(expr_var_value(ac_g_ac_instance_ttemp), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(ac_g_ac_instance_ttemp), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(ac_g_ac_instance_ttemp), AC0TTEMP | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(ac_g_ac_instance_ttemp), 0);
    EXPECT_EQ(expr_var_type_is_resolved(ac_g_ac_instance_ttemp), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    LOG(E, "SLOT_GROUPX");
    mimic_owner_ddm2_classes[SLOT_GROUPX].parameter = GROUP0;
    int instance_groupX = owner_init_class(&mimic_owner_ddm2_classes[SLOT_GROUPX]);
    LOG(E, "SLOT_GROUPY");
    mimic_owner_ddm2_classes[SLOT_GROUPY].parameter = GROUP0;
    int instance_groupY = owner_init_class(&mimic_owner_ddm2_classes[SLOT_GROUPY]);
    LOG(E, "SLOT_PRODMBAT");
    mimic_owner_ddm2_classes[SLOT_PRODMBAT].parameter = PROD0;
    int instance_prodmbat = owner_init_class(&mimic_owner_ddm2_classes[SLOT_PRODMBAT]);
    LOG(E, "SLOT_MBAT");
    mimic_owner_ddm2_classes[SLOT_MBAT].parameter = MBAT0;
    int instance_mbat = owner_init_class(&mimic_owner_ddm2_classes[SLOT_MBAT]);
    LOG(E, "SLOT_PROD");
    mimic_owner_ddm2_classes[SLOT_PROD].parameter = PROD0;
    int instance_prod = owner_init_class(&mimic_owner_ddm2_classes[SLOT_PROD]);
    LOG(E, "SLOT_AC");
    mimic_owner_ddm2_classes[SLOT_AC].parameter = AC0;
    int instance_ac = owner_init_class(&mimic_owner_ddm2_classes[SLOT_AC]);

    LOG(E, "SLOT_GROUPXINTERFACE");
    mimic_owner_ddm2_classes[SLOT_GROUPXINTERFACE].parameter = GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_groupX);
    uint32_t classes_group_g_groupX_instance_interface[] =
        {
            GROUP0 | DDM2_PARAMETER_INSTANCE(instance_groupY),
            PROD0 | DDM2_PARAMETER_INSTANCE(instance_prodmbat),
        };
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_GROUPXINTERFACE].parameter, classes_group_g_groupX_instance_interface, sizeof(classes_group_g_groupX_instance_interface));

    LOG(E, "SLOT_PRODMBATCLIST");
    mimic_owner_ddm2_classes[SLOT_PRODMBATCLIST].parameter = PROD0CLIST | DDM2_PARAMETER_INSTANCE(instance_prodmbat);
    uint32_t classes_prod_g_group_prod_mabat_instance_clist[] =
        {
            MBAT0 | DDM2_PARAMETER_INSTANCE(instance_mbat),
        };
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_PRODMBATCLIST].parameter, classes_prod_g_group_prod_mabat_instance_clist, sizeof(classes_prod_g_group_prod_mabat_instance_clist));

    LOG(E, "SLOT_GROUPYINTERFACE");
    mimic_owner_ddm2_classes[SLOT_GROUPYINTERFACE].parameter = GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_groupY);
    uint32_t classes_group_g_groupY_instance_interface[] =
        {
            PROD0 | DDM2_PARAMETER_INSTANCE(instance_prod),
        };
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_GROUPYINTERFACE].parameter, classes_group_g_groupY_instance_interface, sizeof(classes_group_g_groupY_instance_interface));

    LOG(E, "SLOT_PRODCLIST");
    mimic_owner_ddm2_classes[SLOT_PRODCLIST].parameter = PROD0CLIST | DDM2_PARAMETER_INSTANCE(instance_prod);
    uint32_t classes_prod_g_group_prod_instance_clist[] =
        {
            AC0 | DDM2_PARAMETER_INSTANCE(instance_ac),
        };
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_PRODCLIST].parameter, classes_prod_g_group_prod_instance_clist, sizeof(classes_prod_g_group_prod_instance_clist));

    // All shoud be resolved now
    // g_groupX_instance_var should be resolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_groupX));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupX_instance_interface_1 should be resolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_1), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_1), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_1), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_groupX));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_1), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_1), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_groupY_instance_var should be resolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(instance_groupY));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_group_prod_mabat_instance should be resolved
    EXPECT_EQ(expr_var_value(g_group_prod_mabat_instance), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_group_prod_mabat_instance), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_group_prod_mabat_instance), PROD0 | DDM2_PARAMETER_INSTANCE(instance_prodmbat));
    EXPECT_EQ(expr_var_type_id(g_group_prod_mabat_instance), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_group_prod_mabat_instance), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupX_instance_interface_2 should be resolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_2), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_2), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_2), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_groupX));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_2), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_2), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_mbat_instance should be resolved
    EXPECT_EQ(expr_var_value(g_mbat_instance), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_mbat_instance), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_mbat_instance), MBAT0 | DDM2_PARAMETER_INSTANCE(instance_mbat));
    EXPECT_EQ(expr_var_type_id(g_mbat_instance), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_mbat_instance), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // prod_g_group_prod_mabat_instance_clist should be resolved
    EXPECT_EQ(expr_var_value(prod_g_group_prod_mabat_instance_clist), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(prod_g_group_prod_mabat_instance_clist), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(prod_g_group_prod_mabat_instance_clist), PROD0CLIST | DDM2_PARAMETER_INSTANCE(instance_prodmbat));
    EXPECT_EQ(expr_var_type_id(prod_g_group_prod_mabat_instance_clist), 0);
    EXPECT_EQ(expr_var_type_is_resolved(prod_g_group_prod_mabat_instance_clist), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_group_prod_instance should be resolved
    EXPECT_EQ(expr_var_value(g_group_prod_instance), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_group_prod_instance), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_group_prod_instance), PROD0 | DDM2_PARAMETER_INSTANCE(instance_prod));
    EXPECT_EQ(expr_var_type_id(g_group_prod_instance), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_group_prod_instance), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // group_g_groupY_instance_interface should be resolved
    EXPECT_EQ(expr_var_value(group_g_groupY_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupY_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_groupY));
    EXPECT_EQ(expr_var_type_id(group_g_groupY_instance_interface), 1);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_ac_instance should be resolved
    EXPECT_EQ(expr_var_value(g_ac_instance), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_ac_instance), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_ac_instance), AC0 | DDM2_PARAMETER_INSTANCE(instance_ac));
    EXPECT_EQ(expr_var_type_id(g_ac_instance), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_ac_instance), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // prod_g_group_prod_instance_clist should be resolved
    EXPECT_EQ(expr_var_value(prod_g_group_prod_instance_clist), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(prod_g_group_prod_instance_clist), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(prod_g_group_prod_instance_clist), PROD0CLIST | DDM2_PARAMETER_INSTANCE(instance_prod));
    EXPECT_EQ(expr_var_type_id(prod_g_group_prod_instance_clist), 1);
    EXPECT_EQ(expr_var_type_is_resolved(prod_g_group_prod_instance_clist), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_mbatsoc should be resolved
    EXPECT_EQ(expr_var_value(g_mbatsoc), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_mbatsoc), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_id(g_mbatsoc), expr_generate_hash_key("g_mbatsoc"));
    EXPECT_EQ(expr_var_type_id(g_mbatsoc), RULE_ENGINE__VAR_TYTE_ID_UNAPLICABLE);
    EXPECT_EQ(expr_var_type_is_resolved(g_mbatsoc), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // mbat_g_mbat_instance_soc should be resolved
    EXPECT_EQ(expr_var_value(mbat_g_mbat_instance_soc), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(mbat_g_mbat_instance_soc), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(mbat_g_mbat_instance_soc), MBAT0SOC | DDM2_PARAMETER_INSTANCE(instance_mbat));
    EXPECT_EQ(expr_var_type_id(mbat_g_mbat_instance_soc), 0);
    EXPECT_EQ(expr_var_type_is_resolved(mbat_g_mbat_instance_soc), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // g_acttemp should be resolved
    EXPECT_EQ(expr_var_value(g_acttemp), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_acttemp), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_id(g_acttemp), expr_generate_hash_key("g_acttemp"));
    EXPECT_EQ(expr_var_type_id(g_acttemp), RULE_ENGINE__VAR_TYTE_ID_UNAPLICABLE);
    EXPECT_EQ(expr_var_type_is_resolved(g_acttemp), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // ac_g_ac_instance_ttemp should be resolved
    EXPECT_EQ(expr_var_value(ac_g_ac_instance_ttemp), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(ac_g_ac_instance_ttemp), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(ac_g_ac_instance_ttemp), AC0TTEMP | DDM2_PARAMETER_INSTANCE(instance_ac));
    EXPECT_EQ(expr_var_type_id(ac_g_ac_instance_ttemp), 0);
    EXPECT_EQ(expr_var_type_is_resolved(ac_g_ac_instance_ttemp), RULE_ENGINE__VAR_TYPE_RESOLVED);

    // Remove root instance, all should be unresolved
    int32_t unavialbe = 0;
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_GROUPX].parameter, &unavialbe, sizeof(unavialbe));
    // g_groupX_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupX_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupX_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupX_instance_var), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupX_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupX_instance_interface_1 should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_1), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_1), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_1), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_1), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_1), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_groupY_instance_var should be unresolved
    EXPECT_EQ(expr_var_value(g_groupY_instance_var), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_groupY_instance_var), GROUP0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_groupY_instance_var), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_groupY_instance_var), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupY_instance_interface should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupY_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupY_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupY_instance_interface), 1);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_group_prod_mabat_instance should be unresolved
    EXPECT_EQ(expr_var_value(g_group_prod_mabat_instance), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_group_prod_mabat_instance), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_group_prod_mabat_instance), PROD0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_group_prod_mabat_instance), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_group_prod_mabat_instance), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupX_instance_interface_2 should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupX_instance_interface_2), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupX_instance_interface_2), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupX_instance_interface_2), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupX_instance_interface_2), 0);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupX_instance_interface_2), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_mbat_instance should be unresolved
    EXPECT_EQ(expr_var_value(g_mbat_instance), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_mbat_instance), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_mbat_instance), MBAT0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_mbat_instance), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_mbat_instance), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // prod_g_group_prod_mabat_instance_clist should be unresolved
    EXPECT_EQ(expr_var_value(prod_g_group_prod_mabat_instance_clist), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(prod_g_group_prod_mabat_instance_clist), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(prod_g_group_prod_mabat_instance_clist), PROD0CLIST | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(prod_g_group_prod_mabat_instance_clist), 0);
    EXPECT_EQ(expr_var_type_is_resolved(prod_g_group_prod_mabat_instance_clist), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_group_prod_instance should be unresolved
    EXPECT_EQ(expr_var_value(g_group_prod_instance), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_group_prod_instance), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_group_prod_instance), PROD0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_group_prod_instance), 1);
    EXPECT_EQ(expr_var_type_is_resolved(g_group_prod_instance), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // group_g_groupY_instance_interface should be unresolved
    EXPECT_EQ(expr_var_value(group_g_groupY_instance_interface), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(group_g_groupY_instance_interface), GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(group_g_groupY_instance_interface), 1);
    EXPECT_EQ(expr_var_type_is_resolved(group_g_groupY_instance_interface), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_ac_instance should be unresolved
    EXPECT_EQ(expr_var_value(g_ac_instance), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_ac_instance), RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    EXPECT_EQ(expr_var_id(g_ac_instance), AC0 | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(g_ac_instance), 0);
    EXPECT_EQ(expr_var_type_is_resolved(g_ac_instance), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // prod_g_group_prod_instance_clist should be unresolved
    EXPECT_EQ(expr_var_value(prod_g_group_prod_instance_clist), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(prod_g_group_prod_instance_clist), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(prod_g_group_prod_instance_clist), PROD0CLIST | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(prod_g_group_prod_instance_clist), 1);
    EXPECT_EQ(expr_var_type_is_resolved(prod_g_group_prod_instance_clist), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_mbatsoc should be resolved
    EXPECT_EQ(expr_var_value(g_mbatsoc), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_mbatsoc), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_id(g_mbatsoc), expr_generate_hash_key("g_mbatsoc"));
    EXPECT_EQ(expr_var_type_id(g_mbatsoc), RULE_ENGINE__VAR_TYTE_ID_UNAPLICABLE);
    EXPECT_EQ(expr_var_type_is_resolved(g_mbatsoc), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // mbat_g_mbat_instance_soc should be unresolved
    EXPECT_EQ(expr_var_value(mbat_g_mbat_instance_soc), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(mbat_g_mbat_instance_soc), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(mbat_g_mbat_instance_soc), MBAT0SOC | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(mbat_g_mbat_instance_soc), 0);
    EXPECT_EQ(expr_var_type_is_resolved(mbat_g_mbat_instance_soc), RULE_ENGINE__VAR_TYPE_UNRESOLVED);
    // g_acttemp should be resolved
    EXPECT_EQ(expr_var_value(g_acttemp), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(g_acttemp), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_id(g_acttemp), expr_generate_hash_key("g_acttemp"));
    EXPECT_EQ(expr_var_type_id(g_acttemp), RULE_ENGINE__VAR_TYTE_ID_UNAPLICABLE);
    EXPECT_EQ(expr_var_type_is_resolved(g_acttemp), RULE_ENGINE__VAR_TYPE_RESOLVED);
    // ac_g_ac_instance_ttemp should be unresolved
    EXPECT_EQ(expr_var_value(ac_g_ac_instance_ttemp), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type(ac_g_ac_instance_ttemp), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    EXPECT_EQ(expr_var_id(ac_g_ac_instance_ttemp), AC0TTEMP | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    EXPECT_EQ(expr_var_type_id(ac_g_ac_instance_ttemp), 0);
    EXPECT_EQ(expr_var_type_is_resolved(ac_g_ac_instance_ttemp), RULE_ENGINE__VAR_TYPE_UNRESOLVED);

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    vPortResumeScheduler();
}

TEST_F(RuleEngineEvaluatorIntegrationTestFixture, ComplexDynamicResolutionTriggerStandardRules)
{
    DEBUG_LOGS_ENABLE();

    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    enum mimic_owner_slot
    {
        SLOT_GROUPX = 0,
        SLOT_GROUPY,
        SLOT_PRODMBAT,
        SLOT_MBAT,
        SLOT_PROD,
        SLOT_AC,
        SLOT_GROUPXINTERFACE,
        SLOT_GROUPYINTERFACE,
        SLOT_PRODMBATCLIST,
        SLOT_PRODCLIST,
        SLOT_MBATSOC,
        SLOT_ACTTEMP,
    };
    const char *rule0_str = "inventory(g_groupX_instance, GROUP0)";
    const char *rule1_str = "dyn_instance(g_groupY_instance, group{g_groupX_instance}interface, GROUP0, 0)";
    const char *rule2_str = "dyn_instance(g_group_prod_mabat_instance, group{g_groupX_instance}interface, PROD0, 0)";
    const char *rule3_str = "dyn_instance(g_mbat_instance, prod{g_group_prod_mabat_instance}clist, MBAT0, 0)";
    const char *rule4_str = "dyn_instance(g_group_prod_instance, group{g_groupY_instance}interface, PROD0, 0)";
    const char *rule5_str = "dyn_instance(g_ac_instance, prod{g_group_prod_instance}clist, AC0, 0)";
    const char *rule6_str = "mbat{g_mbat_instance}soc, counter = 0: g_mbatsoc = mbat{g_mbat_instance}soc, counter = counter + 1, print(counter)";
    const char *rule7_str = "ac{g_ac_instance}ttemp, counter = 0: g_acttemp = ac{g_ac_instance}ttemp, counter = counter + 1";

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
        };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Load first rule
    for (size_t i = 0; i < ELEMENTS(rules); ++i)
    {
        ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[i]), 1) << "Failed to add rule: " << rules[i].name;
    }

    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    struct expr_var *g_groupX_instance_var = getVariableByName(rule0_spec, "g_groupX_instance");
    struct rule_engine__specification *rule1_spec = getSpecificationById(1);
    struct expr_var *g_groupY_instance_var = getVariableByName(rule1_spec, "g_groupY_instance");
    struct expr_var *group_g_groupX_instance_interface_1 = getVariableByName(rule1_spec, "group{g_groupX_instance}interface");
    struct rule_engine__specification *rule2_spec = getSpecificationById(2);
    struct expr_var *g_group_prod_mabat_instance = getVariableByName(rule2_spec, "g_group_prod_mabat_instance");
    struct expr_var *group_g_groupX_instance_interface_2 = getVariableByName(rule2_spec, "group{g_groupX_instance}interface");
    struct rule_engine__specification *rule3_spec = getSpecificationById(3);
    struct expr_var *g_mbat_instance = getVariableByName(rule3_spec, "g_mbat_instance");
    struct expr_var *prod_g_group_prod_mabat_instance_clist = getVariableByName(rule3_spec, "prod{g_group_prod_mabat_instance}clist");
    struct rule_engine__specification *rule4_spec = getSpecificationById(4);
    struct expr_var *g_group_prod_instance = getVariableByName(rule4_spec, "g_group_prod_instance");
    struct expr_var *group_g_groupY_instance_interface = getVariableByName(rule4_spec, "group{g_groupY_instance}interface");
    struct rule_engine__specification *rule5_spec = getSpecificationById(5);
    struct expr_var *g_ac_instance = getVariableByName(rule5_spec, "g_ac_instance");
    struct expr_var *prod_g_group_prod_instance_clist = getVariableByName(rule5_spec, "prod{g_group_prod_instance}clist");
    struct rule_engine__specification *rule6_spec = getSpecificationById(6);
    struct expr_var *g_mbatsoc = getVariableByName(rule6_spec, "g_mbatsoc");
    struct expr_var *mbat_g_mbat_instance_soc = getVariableByName(rule6_spec, "mbat{g_mbat_instance}soc");
    struct expr_var *mbat_counter = getVariableByName(rule6_spec, "counter");
    struct rule_engine__specification *rule7_spec = getSpecificationById(7);
    struct expr_var *g_acttemp = getVariableByName(rule7_spec, "g_acttemp");
    struct expr_var *ac_g_ac_instance_ttemp = getVariableByName(rule7_spec, "ac{g_ac_instance}ttemp");
    ASSERT_NE(g_groupX_instance_var, nullptr) << "Variable " << g_groupX_instance_var->name << " not found in specification";
    ASSERT_NE(g_groupY_instance_var, nullptr) << "Variable " << g_groupY_instance_var->name << " not found in specification";
    ASSERT_NE(group_g_groupX_instance_interface_1, nullptr) << "Variable " << group_g_groupX_instance_interface_1->name << " not found in specification";
    ASSERT_NE(g_group_prod_mabat_instance, nullptr) << "Variable " << g_group_prod_mabat_instance->name << " not found in specification";
    ASSERT_NE(group_g_groupX_instance_interface_2, nullptr) << "Variable " << group_g_groupX_instance_interface_2->name << " not found in specification";
    ASSERT_NE(g_mbat_instance, nullptr) << "Variable " << g_mbat_instance->name << " not found in specification";
    ASSERT_NE(prod_g_group_prod_mabat_instance_clist, nullptr) << "Variable " << prod_g_group_prod_mabat_instance_clist->name << " not found in specification";
    ASSERT_NE(g_group_prod_instance, nullptr) << "Variable " << g_group_prod_instance->name << " not found in specification";
    ASSERT_NE(group_g_groupY_instance_interface, nullptr) << "Variable " << group_g_groupY_instance_interface->name << " not found in specification";
    ASSERT_NE(g_ac_instance, nullptr) << "Variable " << g_ac_instance->name << " not found in specification";
    ASSERT_NE(prod_g_group_prod_instance_clist, nullptr) << "Variable " << prod_g_group_prod_instance_clist->name << " not found in specification";
    ASSERT_NE(g_mbatsoc, nullptr) << "Variable " << g_mbatsoc->name << " not found in specification";
    ASSERT_NE(mbat_g_mbat_instance_soc, nullptr) << "Variable " << mbat_g_mbat_instance_soc->name << " not found in specification";
    ASSERT_NE(g_acttemp, nullptr) << "Variable " << g_acttemp->name << " not found in specification";
    ASSERT_NE(ac_g_ac_instance_ttemp, nullptr) << "Variable " << ac_g_ac_instance_ttemp->name << " not found in specification";

    mimic_owner_ddm2_classes[SLOT_GROUPX].parameter = GROUP0;
    int instance_groupX = owner_init_class(&mimic_owner_ddm2_classes[SLOT_GROUPX]);
    mimic_owner_ddm2_classes[SLOT_GROUPY].parameter = GROUP0;
    int instance_groupY = owner_init_class(&mimic_owner_ddm2_classes[SLOT_GROUPY]);
    mimic_owner_ddm2_classes[SLOT_PRODMBAT].parameter = PROD0;
    int instance_prodmbat = owner_init_class(&mimic_owner_ddm2_classes[SLOT_PRODMBAT]);
    mimic_owner_ddm2_classes[SLOT_MBAT].parameter = MBAT0;
    int instance_mbat = owner_init_class(&mimic_owner_ddm2_classes[SLOT_MBAT]);
    mimic_owner_ddm2_classes[SLOT_PROD].parameter = PROD0;
    int instance_prod = owner_init_class(&mimic_owner_ddm2_classes[SLOT_PROD]);
    mimic_owner_ddm2_classes[SLOT_AC].parameter = AC0;
    int instance_ac = owner_init_class(&mimic_owner_ddm2_classes[SLOT_AC]);

    // Resolve all classes and dynamic parameters
    mimic_owner_ddm2_classes[SLOT_GROUPXINTERFACE].parameter = GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_groupX);
    uint32_t classes_group_g_groupX_instance_interface[] =
        {
            GROUP0 | DDM2_PARAMETER_INSTANCE(instance_groupY),
            PROD0 | DDM2_PARAMETER_INSTANCE(instance_prodmbat),
        };
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_GROUPXINTERFACE].parameter, classes_group_g_groupX_instance_interface, sizeof(classes_group_g_groupX_instance_interface));

    mimic_owner_ddm2_classes[SLOT_PRODMBATCLIST].parameter = PROD0CLIST | DDM2_PARAMETER_INSTANCE(instance_prodmbat);
    uint32_t classes_prod_g_group_prod_mabat_instance_clist[] =
        {
            MBAT0 | DDM2_PARAMETER_INSTANCE(instance_mbat),
        };
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_PRODMBATCLIST].parameter, classes_prod_g_group_prod_mabat_instance_clist, sizeof(classes_prod_g_group_prod_mabat_instance_clist));

    mimic_owner_ddm2_classes[SLOT_GROUPYINTERFACE].parameter = GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(instance_groupY);
    uint32_t classes_group_g_groupY_instance_interface[] =
        {
            PROD0 | DDM2_PARAMETER_INSTANCE(instance_prod),
        };
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_GROUPYINTERFACE].parameter, classes_group_g_groupY_instance_interface, sizeof(classes_group_g_groupY_instance_interface));

    mimic_owner_ddm2_classes[SLOT_PRODCLIST].parameter = PROD0CLIST | DDM2_PARAMETER_INSTANCE(instance_prod);
    uint32_t classes_prod_g_group_prod_instance_clist[] =
        {
            AC0 | DDM2_PARAMETER_INSTANCE(instance_ac),
        };
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_PRODCLIST].parameter, classes_prod_g_group_prod_instance_clist, sizeof(classes_prod_g_group_prod_instance_clist));

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Now instances and dynamic parameters are resolved
    // Rule engine instance is DEACTIVATED, so STADARD rules should not be evaluated
    uint32_t mbat0soc_value = 50;
    mimic_owner_ddm2_classes[SLOT_MBATSOC].parameter = MBAT0SOC | DDM2_PARAMETER_INSTANCE(1);
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_MBATSOC].parameter, &mbat0soc_value, sizeof(mbat0soc_value));
    /* we shouldn't have recevied publish frame for MBAT1SOC as we are not subsribed to this instance */
    EXPECT_EQ(getNumReceivedDDMP2Frames(), 0) << "We should have not received DDMP2 frame" << std::endl;
    EXPECT_EQ(expr_var_value(mbat_g_mbat_instance_soc), 0) << "mbat_g_mbat_instance_soc should not be updated as rule engine is deactivated";

    mimic_owner_ddm2_classes[SLOT_MBATSOC].parameter = MBAT0SOC | DDM2_PARAMETER_INSTANCE(instance_mbat);
    owner_update_and_publish_parameter_value(mimic_owner_ddm2_classes[SLOT_MBATSOC].parameter, &mbat0soc_value, sizeof(mbat0soc_value));
    /* we should have recevied publish frame for MBAT1SOC as we are subsribed to instance_mbat */
    EXPECT_EQ(getNumReceivedDDMP2Frames(), 1) << "We should have received DDMP2 frame" << std::endl;
    EXPECT_EQ(getNextReceivedDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.source_connector, connector_unittest.connector_id + slot_mimic_owner) << "Received frame should be from the mimic owner connector";
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_PUBLISH);
    EXPECT_EQ(myFrame.frame.publish.parameter, MBAT0SOC | DDM2_PARAMETER_INSTANCE(instance_mbat));
    EXPECT_EQ(myFrame.frame.publish.value.int32, mbat0soc_value);  // Value should be mbat0soc_value
    EXPECT_EQ(expr_var_value(mbat_g_mbat_instance_soc), 0);        // mbat_g_mbat_instance_soc should not be updated as rule engine is deactivated but ddm item is updated

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    /* Activate the rule engine via RULE0ACT parameter.
     * This mimics real-world activation where RULE0ACT SET frames are processed through the normal
     * task pipeline (rule_engine_task_process), unlike direct rule_engine_activate() calls.
     * The engine will resubscribe to parameters in that iteration as a separate event. */
    int32_t enable = 1;
    owner_set_rule_engine_owned_parameter(RULE0ACT | DDM2_PARAMETER_INSTANCE(rule_engine_inst.instance), &enable, sizeof(enable));  // Set RULE0ACT to 1 to activate the rule engine()

    /* rule engine evaluated the SET frame and sends re-subscription */
    /* rule_engine_task_handler() -> broker -> owner_task_hanldler() */
    int numb_of_received_frames = getNumReceivedDDMP2Frames();
    EXPECT_EQ(numb_of_received_frames, 6) << "We should have received DDMP2 frame";
    EXPECT_EQ(getNextReceivedDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.source_connector, connector_unittest.connector_id + slot_mimic_owner) << "Received frame should be from the mimic owner connector";
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SET);
    EXPECT_EQ(myFrame.frame.set.parameter, RULE0ACT | DDM2_PARAMETER_INSTANCE(rule_engine_inst.instance));
    EXPECT_EQ(myFrame.frame.set.value.int32, 1);

    // Check if the rule has sent re-subscription frames, since now RULE0ACT is enabled.
    // At this point, the rule engine re-evaluates the subscription
    EXPECT_EQ(getNumSentDDMP2Frames(), 6) << "We should have sent one DDMP2 frame";
    for (size_t i = 0; i < 6; ++i)
    {
        EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0) << i << " Failed to get next sent frame";
        EXPECT_EQ(myFrame.source_connector, connector_unittest.connector_id + slot_rule_engine) << i << " Sent frame should be from the rule engine connector";
        EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE) << i << " Control should be SUBSCRIBE";
    }

    // owner_task_hanldler -> broker -> rule_engine_task_handler -> broker
    /* we recevied publish frame as requested by the re-subscription*/
    for (int i = 0; i < numb_of_received_frames - 1; ++i)
    {
        EXPECT_EQ(getNextReceivedDDMP2Frame(&myFrame, &frame_size), 0) << i << " Failed to get next received frame";
        EXPECT_EQ(myFrame.source_connector, connector_unittest.connector_id + slot_mimic_owner) << i << " Received frame should be from the mimic owner connector";
        EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_PUBLISH) << i << " Control should be PUBLISH";
        if (myFrame.frame.publish.parameter == (MBAT0SOC | DDM2_PARAMETER_INSTANCE(instance_mbat)))
        {
            LOG(E, "Received publish frame for MBAT0SOC with value: %d", myFrame.frame.publish.value.int32);
            EXPECT_EQ(myFrame.frame.publish.value.int32, mbat0soc_value);
        }
    }
    EXPECT_EQ(expr_var_value(mbat_g_mbat_instance_soc), mbat0soc_value);
    EXPECT_EQ(expr_var_value(g_mbatsoc), mbat0soc_value);
    EXPECT_EQ(expr_var_value(mbat_counter), 2) << "mbat_counter should be 2 since we received two publish frames for MBAT0SOC, once when rule engine was deactivated and once on resubscription";

    vPortResumeScheduler();
}

TEST_F(RuleEngineEvaluatorIntegrationTestFixture, DynamicInstancesSmartEco)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    const char *rule0_str = "mbat0caprel, g_mbat0caprel = -1, execution_counter = 0: if(g_mbat0caprel != mbat0caprel, g_mbat0caprel = mbat0caprel, g_determine_caprel = 1), execution_counter=execution_counter+1";

    static const struct rule_engine__rule rules[] =
        {
            {.name = "TestRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
        };

    // Initialize mimic owner DDM2 classes with MBAT0CAPREL
    mimic_owner_ddm2_classes[0].parameter = MBAT0CAPREL;
    ASSERT_NE(owner_init_classes(mimic_owner_ddm2_classes, 1), DDMP2_INVALID_INSTANCE);

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    /* Prepare value for MBAT0CAPREL in the mimic owner.
     * It will be PUBLISHED on next subscription request of rule_engine__add_specfication. */
    uint32_t mbat0caprel_value = 50;
    mimic_owner_ddm2_classes[0].parameter = MBAT0CAPREL;
    owner_update_parameter_value(MBAT0CAPREL, &mbat0caprel_value, sizeof(mbat0caprel_value));  // Set MBAT0CAPREL to 50

    // Load first rule
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);

    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    ASSERT_NE(rule0_spec, nullptr) << "Failed to retrieve rule specification";

    struct expr_var *mbat0caprel_rule0 = getVariableByName(rule0_spec, "mbat0caprel");
    struct expr_var *g_mbat0caprel_rule0 = getVariableByName(rule0_spec, "g_mbat0caprel");
    struct expr_var *g_determine_caprel_rule0 = getVariableByName(rule0_spec, "g_determine_caprel");
    struct expr_var *execution_counter_rule0 = getVariableByName(rule0_spec, "execution_counter");
    ASSERT_NE(mbat0caprel_rule0, nullptr);
    ASSERT_NE(g_mbat0caprel_rule0, nullptr);
    ASSERT_NE(g_determine_caprel_rule0, nullptr);
    ASSERT_NE(execution_counter_rule0, nullptr);

    // Check if the rule has sent subscription frame for mbat0caprel
    EXPECT_EQ(getNumSentDDMP2Frames(), 1) << "We should have sent one DDMP2 frame" << std::endl;
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.source_connector, connector_unittest.connector_id + slot_rule_engine) << "Sent frame should be from the rule engine connector";
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE);
    EXPECT_EQ(myFrame.frame.subscribe.parameter, MBAT0CAPREL);

    clearSentDDMP2Frames();
    // Clearing of the received frames should be ommited as we need them for verification

    /* broker -> owner_task_handler() -> broker -> rule_engine_task_handler() */

    /* we recevied publish frame for MBAT0CAPREL from slot[1]*/
    EXPECT_EQ(getNumReceivedDDMP2Frames(), 1) << "We should have received DDMP2 frame" << std::endl;
    EXPECT_EQ(getNextReceivedDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.source_connector, connector_unittest.connector_id + slot_mimic_owner) << "Received frame should be from the mimic owner connector";
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_PUBLISH);
    EXPECT_EQ(myFrame.frame.publish.parameter, MBAT0CAPREL);
    EXPECT_EQ(myFrame.frame.publish.value.int32, mbat0caprel_value);
    /* TODO: Now it is updated, althoug rule_engine is still deactivated (RULE0ACT = 0). This has to be fixed. */
    EXPECT_EQ(expr_var_value(mbat0caprel_rule0), 0);
    EXPECT_EQ(expr_var_value(g_mbat0caprel_rule0), -1);
    EXPECT_EQ(expr_var_value(g_determine_caprel_rule0), 0);
    EXPECT_EQ(expr_var_value(execution_counter_rule0), 0);

    /* Activate the rule engine via RULE0ACT parameter.
     * This mimics real-world activation where RULE0ACT SET frames are processed through the normal
     * task pipeline (rule_engine_task_process), unlike direct rule_engine_activate() calls.
     * The engine will resubscribe to parameters in that iteration as a separate event. */
    int32_t enable = 1;
    owner_set_rule_engine_owned_parameter(RULE0ACT | DDM2_PARAMETER_INSTANCE(rule_engine_inst.instance), &enable, sizeof(enable));  // Set RULE0ACT to 1 to activate the rule engine()

    /* rule engine evaluated the SET frame and sends re-subscription */
    /* rule_engine_task_handler() -> broker -> owner_task_hanldler() */
    EXPECT_EQ(getNumReceivedDDMP2Frames(), 2) << "We should have received DDMP2 frame" << std::endl;
    EXPECT_EQ(getNextReceivedDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.source_connector, connector_unittest.connector_id + slot_mimic_owner) << "Received frame should be from the mimic owner connector";
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SET);
    EXPECT_EQ(myFrame.frame.set.parameter, RULE0ACT | DDM2_PARAMETER_INSTANCE(rule_engine_inst.instance));
    EXPECT_EQ(myFrame.frame.set.value.int32, 1);

    // Check if the rule has sent re-subscription frame for mbat0caprel, since now RULE0ACT is enabled.
    // At this point, the rule engine re-evaluates the subscription
    EXPECT_EQ(getNumSentDDMP2Frames(), 1) << "We should have sent one DDMP2 frame" << std::endl;
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.source_connector, connector_unittest.connector_id + slot_rule_engine) << "Sent frame should be from the rule engine connector";
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE);
    EXPECT_EQ(myFrame.frame.subscribe.parameter, MBAT0CAPREL);

    // owner_task_hanldler -> broker -> rule_engine_task_handler -> broker
    /* we recevied publish frame as requested by the re-subscription for MBAT0CAPREL from slot[1]*/
    EXPECT_EQ(getNextReceivedDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.source_connector, connector_unittest.connector_id + slot_mimic_owner) << "Received frame should be from the mimic owner connector";
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_PUBLISH);
    EXPECT_EQ(myFrame.frame.publish.parameter, MBAT0CAPREL);
    EXPECT_EQ(myFrame.frame.publish.value.int32, mbat0caprel_value);

    EXPECT_EQ(expr_var_value(mbat0caprel_rule0), mbat0caprel_value);
    EXPECT_EQ(expr_var_value(g_mbat0caprel_rule0), mbat0caprel_value);
    EXPECT_EQ(expr_var_value(g_determine_caprel_rule0), 1);
    EXPECT_EQ(expr_var_value(execution_counter_rule0), 2) << "mbat_counter should be 2 since we received two publish frames for MBAT0CAPREL, once when rule engine was deactivated and once on resubscription";

    vPortResumeScheduler();
}
extern "C" void disable_connectors(void)
{
    LOG(I, "Disable smarteco connector and climate zone connector");
    connector_smart_eco_feature.disabled = 1;
    connector_climate_zone_feature.disabled = 1;
}
