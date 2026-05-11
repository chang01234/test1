/*
 * test_rule_engine.cpp
 *
 *  Created on: April 23, 2025
 *      Author: Borjan Bozhinovski
 */

#include "test_rule_engine.hpp"

class RuleEngineEvaluatorTestFixture : public RuleEngineTestFixture
{
  protected:
    void SetUp() override
    {
        RuleEngineTestFixture::SetUp();
    }

    void TearDown() override
    {
        RuleEngineTestFixture::TearDown();
    }

    void receivePublishFrameFromBrokerMimic(DDMP2_FRAME *myFrame, uint32_t ddm2_parameter, const void *value, size_t value_size)
    {
        // Mimic receiving a publish frame from the broker for a specific DDM2 parameter from the mimic owner connector
        static uint8_t connector_id = connector_unittest.connector_id + 1;  // Use a different connector ID for mimic owner
        ddmp2_create_publish(myFrame, ddm2_parameter, value, value_size, connector_id);
    }

    void receiveSetFrameFromBrokerMimic(DDMP2_FRAME *myFrame, uint32_t ddm2_parameter, const void *value, size_t value_size)
    {
        // Mimic receiving a publish frame from the broker for a specific DDM2 parameter from the mimic owner connector
        static uint8_t connector_id = connector_unittest.connector_id + 1;  // Use a different connector ID for mimic owner
        ddmp2_create_set(myFrame, ddm2_parameter, value, value_size, connector_id);
    }

    void receiveSubFrameFromBrokerMimic(DDMP2_FRAME *myFrame, uint32_t ddm2_parameter)
    {
        // Mimic receiving a publish frame from the broker for a specific DDM2 parameter from the mimic owner connector
        static uint8_t connector_id = connector_unittest.connector_id + 1;  // Use a different connector ID for mimic owner
        ddmp2_create_subscribe(myFrame, ddm2_parameter, connector_id);
    }
};

TEST_F(RuleEngineEvaluatorTestFixture, VerifySimpleRuleEvaluation)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    const char *rule0_str = "mbat0caprel, g_mbat0caprel = -1: if(g_mbat0caprel != mbat0caprel, g_mbat0caprel = mbat0caprel, g_determine_caprel = 1)";

    static const struct rule_engine__rule rules[] =
        {
            {.name = "TestRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
        };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Load first rule
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);

    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    ASSERT_NE(rule0_spec, nullptr) << "Failed to retrieve rule specification";

    // Check if the rule has sent subscription frame for mbat0caprel
    EXPECT_EQ(getNumSentDDMP2Frames(), 1) << "We should have sent one DDMP2 frame" << std::endl;
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.source_connector, connector_unittest.connector_id) << "Sent frame should be from the rule engine connector";
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE);
    EXPECT_EQ(myFrame.frame.subscribe.parameter, MBAT0CAPREL);
    EXPECT_EQ(getNumReceivedDDMP2Frames(), 0) << "We should NOT have received a DDMP2 frame" << std::endl;

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    int32_t mbat0caprel_value = 50;
    receivePublishFrameFromBrokerMimic(&myFrame, MBAT0CAPREL, &mbat0caprel_value, sizeof(mbat0caprel_value));

    // Process the received frame
    rule_engine_task_process(&rule_engine_inst, &myFrame);

    /* Rule engine is not activated, even though it receved publish frame, it should not trigger the rule */
    struct expr_var *mbat0caprel_rule0 = getVariableByName(rule0_spec, "mbat0caprel");
    ASSERT_NE(mbat0caprel_rule0, nullptr);
    EXPECT_EQ(expr_var_value(mbat0caprel_rule0), 0);

    struct expr_var *g_mbat0caprel_rule0 = getVariableByName(rule0_spec, "g_mbat0caprel");
    ASSERT_NE(g_mbat0caprel_rule0, nullptr);
    EXPECT_EQ(expr_var_value(g_mbat0caprel_rule0), -1);

    struct expr_var *g_determine_caprel_rule0 = getVariableByName(rule0_spec, "g_determine_caprel");
    ASSERT_NE(g_determine_caprel_rule0, nullptr);
    EXPECT_EQ(expr_var_value(g_determine_caprel_rule0), 0);

    /* we should not send DDMP2 frame*/
    EXPECT_EQ(getNumSentDDMP2Frames(), 0) << "We should have NOT sent DDMP2 frame" << std::endl;
}

/**
 * @brief Test case for simple DDM2 parameter triggering:
 *
 * Scenario:
 * 1. Load a single rule that monitors one DDM2 parameter and updates a global variable
 * 2. Send a DDMP2 frame with the monitored parameter
 * 3. Verify the rule triggers and updates the global variable correctly
 * 4. Verify proper DDM2 subscription behavior
 */
TEST_F(RuleEngineEvaluatorTestFixture, VerifyDDM2ParameterTriggeringSimple)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    const char *rule_str = "mbat0caprel: g_battery_level = mbat0caprel";

    static const struct rule_engine__rule rules[] = {
        {.name = "SimpleDDM2Rule", .rule = const_cast<char *>(rule_str), .size = static_cast<int>(strlen(rule_str))},
    };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Load the rule
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);

    struct rule_engine__specification *rule_spec = getSpecificationById(0);
    ASSERT_NE(rule_spec, nullptr) << "Failed to retrieve rule specification";

    // Check DDM2 parameter setup
    ddmw_item_t *mbat0caprel_item = getDDM2ParameterByID(MBAT0CAPREL);
    EXPECT_NE(mbat0caprel_item, nullptr);
    EXPECT_EQ(getDDM2ParameterValue(mbat0caprel_item), 0);
    EXPECT_EQ(getDDM2ParameterType(mbat0caprel_item), DDMW_ACTION_SET);

    // Check rule variables
    struct expr_var *mbat0caprel_var = getVariableByName(rule_spec, "mbat0caprel");
    ASSERT_NE(mbat0caprel_var, nullptr);
    EXPECT_EQ(expr_var_type(mbat0caprel_var), RULE_ENGINE__VAR_TYPE_DDM2);
    EXPECT_EQ(expr_var_id(mbat0caprel_var), MBAT0CAPREL);
    EXPECT_EQ(expr_var_value(mbat0caprel_var), 0);

    struct expr_var *g_battery_level_var = getVariableByName(rule_spec, "g_battery_level");
    ASSERT_NE(g_battery_level_var, nullptr);
    EXPECT_EQ(expr_var_type(g_battery_level_var), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_value(g_battery_level_var), 0);

    // Check trigger section
    EXPECT_EQ(getTriggerListSize(rule_spec), 1);
    EXPECT_EQ(getMaskType(rule_spec), RULE_ENGINE__MASK_CONSTRAINT_AND);
    EXPECT_TRUE(isVariableInTriggerList(rule_spec, mbat0caprel_var));
    EXPECT_TRUE(isVariableInMask(rule_spec, mbat0caprel_var));
    EXPECT_FALSE(isVariableInTriggerList(rule_spec, g_battery_level_var));

    // Verify subscription frame was sent
    EXPECT_EQ(getNumSentDDMP2Frames(), 1) << "Should have sent one subscription frame";
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE);
    EXPECT_EQ(myFrame.frame.subscribe.parameter, MBAT0CAPREL);

    // Activate rule engine
    // NOTE: This test simulates immediate activation with subsequent frame processing in the same iteration.
    // The next rule_engine_task_process() call will: activate the engine, send resubscriptions, and process frames.
    // In real-world scenario, RULE0ACT activation (via external connectors or GW0INV logic) and subsequent publish
    // frames would be processed across separate iterations, not bundled together as in this test scenario.
    rule_engine_activate(&rule_engine_inst, 1);

    int32_t mbat0caprel_value = 75;
    receivePublishFrameFromBrokerMimic(&myFrame, MBAT0CAPREL, &mbat0caprel_value, sizeof(mbat0caprel_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    // Verify re-subscription was sent, as the rule engine was activated
    EXPECT_EQ(getNumSentDDMP2Frames(), 1);
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE);
    EXPECT_EQ(myFrame.frame.subscribe.parameter, MBAT0CAPREL);
    // Verify variable values were updated correctly
    EXPECT_EQ(getDDM2ParameterValue(mbat0caprel_item), mbat0caprel_value);
    EXPECT_EQ(expr_var_value(mbat0caprel_var), mbat0caprel_value);
    EXPECT_EQ(expr_var_value(g_battery_level_var), mbat0caprel_value);

    clearSentDDMP2Frames();

    vPortResumeScheduler();
}

/**
 * @brief Test case for multiple DDM2 parameters triggering the same rule:
 *
 * Scenario:
 * 1. Load a rule that monitors two DDM2 parameters (mbat0caprel and ac0on) and updates global variables
 * 2. Send DDMP2 frames for each parameter separately
 * 3. Verify the rule triggers for each parameter change independently
 * 4. Verify proper DDM2 subscription behavior for both parameters
 * 5. Verify that both parameters can trigger the same rule logic
 */
TEST_F(RuleEngineEvaluatorTestFixture, VerifyDDM2ParameterTriggeringMultiple)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    const char *rule_str = "mbat0caprel, ac0on: g_last_updated = mbat0caprel + ac0on, g_trigger_count = g_trigger_count + 1";

    static const struct rule_engine__rule rules[] = {
        {.name = "MultipleDDM2Rule", .rule = const_cast<char *>(rule_str), .size = static_cast<int>(strlen(rule_str))},
    };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Load the rule
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);

    struct rule_engine__specification *rule_spec = getSpecificationById(0);
    ASSERT_NE(rule_spec, nullptr) << "Failed to retrieve rule specification";

    // Check DDM2 parameters setup
    ddmw_item_t *mbat0caprel_item = getDDM2ParameterByID(MBAT0CAPREL);
    EXPECT_NE(mbat0caprel_item, nullptr);
    EXPECT_EQ(getDDM2ParameterValue(mbat0caprel_item), 0);

    ddmw_item_t *ac0on_item = getDDM2ParameterByID(AC0ON);
    EXPECT_NE(ac0on_item, nullptr);
    EXPECT_EQ(getDDM2ParameterValue(ac0on_item), 0);

    // Check rule variables
    struct expr_var *mbat0caprel_var = getVariableByName(rule_spec, "mbat0caprel");
    ASSERT_NE(mbat0caprel_var, nullptr);
    EXPECT_EQ(expr_var_type(mbat0caprel_var), RULE_ENGINE__VAR_TYPE_DDM2);
    EXPECT_EQ(expr_var_id(mbat0caprel_var), MBAT0CAPREL);
    EXPECT_EQ(expr_var_value(mbat0caprel_var), 0);

    struct expr_var *ac0on_var = getVariableByName(rule_spec, "ac0on");
    ASSERT_NE(ac0on_var, nullptr);
    EXPECT_EQ(expr_var_type(ac0on_var), RULE_ENGINE__VAR_TYPE_DDM2);
    EXPECT_EQ(expr_var_id(ac0on_var), AC0ON);
    EXPECT_EQ(expr_var_value(ac0on_var), 0);

    struct expr_var *g_last_updated_var = getVariableByName(rule_spec, "g_last_updated");
    ASSERT_NE(g_last_updated_var, nullptr);
    EXPECT_EQ(expr_var_type(g_last_updated_var), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_id(g_last_updated_var), expr_generate_hash_key("g_last_updated"));
    EXPECT_EQ(expr_var_value(g_last_updated_var), 0);

    struct expr_var *g_trigger_count_var = getVariableByName(rule_spec, "g_trigger_count");
    ASSERT_NE(g_trigger_count_var, nullptr);
    EXPECT_EQ(expr_var_type(g_trigger_count_var), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_id(g_trigger_count_var), expr_generate_hash_key("g_trigger_count"));
    EXPECT_EQ(expr_var_value(g_trigger_count_var), 0);

    // Check trigger section - both DDM2 parameters should be in trigger list
    EXPECT_EQ(getTriggerListSize(rule_spec), 2);
    EXPECT_EQ(getMaskType(rule_spec), RULE_ENGINE__MASK_CONSTRAINT_AND);
    EXPECT_TRUE(isVariableInTriggerList(rule_spec, mbat0caprel_var));
    EXPECT_TRUE(isVariableInTriggerList(rule_spec, ac0on_var));
    EXPECT_TRUE(isVariableInMask(rule_spec, mbat0caprel_var));
    EXPECT_TRUE(isVariableInMask(rule_spec, ac0on_var));

    // Verify subscription frames were sent for both parameters
    EXPECT_EQ(getNumSentDDMP2Frames(), 2) << "Should have sent two subscription frames";
    // Check first subscription
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE);
    EXPECT_TRUE(myFrame.frame.subscribe.parameter == MBAT0CAPREL || myFrame.frame.subscribe.parameter == AC0ON);
    // Check second subscription
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE);
    EXPECT_TRUE(myFrame.frame.subscribe.parameter == MBAT0CAPREL || myFrame.frame.subscribe.parameter == AC0ON);

    // Activate rule engine
    // NOTE: This test simulates immediate activation with subsequent frame processing in the same iteration.
    // The next rule_engine_task_process() call will: activate the engine, send resubscriptions, and process frames.
    // In real-world scenario, RULE0ACT activation (via external connectors or GW0INV logic) and subsequent publish
    // frames would be processed across separate iterations, not bundled together as in this test scenario.
    rule_engine_activate(&rule_engine_inst, 1);

    // Test 1: Update mbat0caprel parameter
    int32_t mbat0caprel_value = 60;
    receivePublishFrameFromBrokerMimic(&myFrame, MBAT0CAPREL, &mbat0caprel_value, sizeof(mbat0caprel_value));
    // Process the received frame
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    // Verify re-subscription was sent, as the rule engine was activated
    EXPECT_EQ(getNumSentDDMP2Frames(), 2);
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE);
    EXPECT_EQ(myFrame.frame.subscribe.parameter, AC0ON);
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE);
    EXPECT_EQ(myFrame.frame.subscribe.parameter, MBAT0CAPREL);
    // Verify rule was not triggered and variables not updated
    EXPECT_EQ(getDDM2ParameterValue(mbat0caprel_item), mbat0caprel_value);
    EXPECT_EQ(expr_var_value(mbat0caprel_var), mbat0caprel_value);  // value has been updated
    EXPECT_EQ(expr_var_value(g_last_updated_var), 0);               // ac0on still 0, not evaluated yet
    EXPECT_EQ(expr_var_value(g_trigger_count_var), 0);              // ac0on still 0, not evaluated yet

    clearSentDDMP2Frames();

    // Test 2: Update ac0on parameter
    int32_t ac0on_value = 1;
    receivePublishFrameFromBrokerMimic(&myFrame, AC0ON, &ac0on_value, sizeof(ac0on_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    // Verify that we didn't send any new frames
    EXPECT_EQ(getNumSentDDMP2Frames(), 0);
    // Verify rule was triggered and variables updated
    EXPECT_EQ(getDDM2ParameterValue(ac0on_item), ac0on_value);
    EXPECT_EQ(expr_var_value(ac0on_var), ac0on_value);
    EXPECT_EQ(expr_var_value(g_last_updated_var), mbat0caprel_value + ac0on_value);  // 60 + 1 = 61
    EXPECT_EQ(expr_var_value(g_trigger_count_var), 1);

    vPortResumeScheduler();
}

/**
 * @brief Test case for DDM2 parameters with AND mask behavior:
 *
 * Scenario:
 * 1. Load a rule with AND mask that requires multiple DDM2 parameters to be updated
 * 2. Update only one parameter - rule should not fully execute
 * 3. Update the second parameter - rule should now execute completely
 * 4. Verify AND mask logic correctly gates rule execution
 * 5. Test that subsequent updates to either parameter trigger the rule
 */
TEST_F(RuleEngineEvaluatorTestFixture, VerifyDDM2ParameterTriggeringWithANDMask)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    // Rule with AND mask - both parameters must be updated for full execution
    const char *rule_str = "mbat0caprel, ac0on: if(mbat0caprel > 50 && ac0on > 0, g_both_conditions_met = 1), if(mbat0caprel <= 50 || ac0on == 0, g_both_conditions_met = 0)";

    static const struct rule_engine__rule rules[] = {
        {.name = "ANDMaskRule", .rule = const_cast<char *>(rule_str), .size = static_cast<int>(strlen(rule_str))},
    };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Load the rule
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);

    struct rule_engine__specification *rule_spec = getSpecificationById(0);
    ASSERT_NE(rule_spec, nullptr) << "Failed to retrieve rule specification";

    // Check rule variables
    struct expr_var *mbat0caprel_var = getVariableByName(rule_spec, "mbat0caprel");
    ASSERT_NE(mbat0caprel_var, nullptr);
    EXPECT_EQ(expr_var_type(mbat0caprel_var), RULE_ENGINE__VAR_TYPE_DDM2);

    struct expr_var *ac0on_var = getVariableByName(rule_spec, "ac0on");
    ASSERT_NE(ac0on_var, nullptr);
    EXPECT_EQ(expr_var_type(ac0on_var), RULE_ENGINE__VAR_TYPE_DDM2);

    struct expr_var *g_both_conditions_met_var = getVariableByName(rule_spec, "g_both_conditions_met");
    ASSERT_NE(g_both_conditions_met_var, nullptr);
    EXPECT_EQ(expr_var_type(g_both_conditions_met_var), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_value(g_both_conditions_met_var), 0);

    // Verify AND mask setup
    EXPECT_EQ(getTriggerListSize(rule_spec), 2);
    EXPECT_EQ(getMaskType(rule_spec), RULE_ENGINE__MASK_CONSTRAINT_AND);
    EXPECT_TRUE(isVariableInTriggerList(rule_spec, mbat0caprel_var));
    EXPECT_TRUE(isVariableInTriggerList(rule_spec, ac0on_var));
    EXPECT_TRUE(isVariableInMask(rule_spec, mbat0caprel_var));
    EXPECT_TRUE(isVariableInMask(rule_spec, ac0on_var));

    // Clear subscription frames
    clearSentDDMP2Frames();

    // Activate rule engine
    // NOTE: This test simulates immediate activation with subsequent frame processing in the same iteration.
    // The next rule_engine_task_process() call will: activate the engine, send resubscriptions, and process frames.
    // In real-world scenario, RULE0ACT activation (via external connectors or GW0INV logic) and subsequent publish
    // frames would be processed across separate iterations, not bundled together as in this test scenario.
    rule_engine_activate(&rule_engine_inst, 1);

    // Test 1: Update only mbat0caprel (should not fully execute due to AND mask)
    int32_t mbat0caprel_value = 75;  // > 50, satisfies first condition
    receivePublishFrameFromBrokerMimic(&myFrame, MBAT0CAPREL, &mbat0caprel_value, sizeof(mbat0caprel_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    // Verify re-subscription was sent, as the rule engine was activated
    EXPECT_EQ(getNumSentDDMP2Frames(), 2);
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE);
    EXPECT_EQ(myFrame.frame.subscribe.parameter, AC0ON);
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE);
    EXPECT_EQ(myFrame.frame.subscribe.parameter, MBAT0CAPREL);
    // Verify mbat0caprel was updated but rule logic hasn't fully executed yet
    EXPECT_EQ(expr_var_value(mbat0caprel_var), mbat0caprel_value);  // Value updated
    EXPECT_EQ(expr_var_value(ac0on_var), 0);                        // Still default value
    EXPECT_EQ(expr_var_value(g_both_conditions_met_var), 0);        // AND mask should prevent full execution until both conditions are met

    // Test 2: Update ac0on parameter - now both parameters should be updated
    int32_t ac0on_value = 1;  // > 0, satisfies second condition
    receivePublishFrameFromBrokerMimic(&myFrame, AC0ON, &ac0on_value, sizeof(ac0on_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(getNumSentDDMP2Frames(), 0);  // Verify that we didn't send any new frames
    // Now both conditions are met (mbat0caprel > 50 && ac0on > 0), rule should execute fully
    EXPECT_EQ(expr_var_value(mbat0caprel_var), mbat0caprel_value);  // Value the same as before
    EXPECT_EQ(expr_var_value(ac0on_var), ac0on_value);              // Value updated
    EXPECT_EQ(expr_var_value(g_both_conditions_met_var), 1);        // Both conditions met

    // Test 3: Update mbat0caprel to a value that doesn't meet condition.
    mbat0caprel_value = 30;  // <= 50, first condition now false
    // Update mbat0caprel to a value <= 50, should not trigger rule execution
    receivePublishFrameFromBrokerMimic(&myFrame, MBAT0CAPREL, &mbat0caprel_value, sizeof(mbat0caprel_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    // Publish AC0ON to mimic a scenario where we update both parameter
    receivePublishFrameFromBrokerMimic(&myFrame, AC0ON, &ac0on_value, sizeof(ac0on_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(getNumSentDDMP2Frames(), 0);  // Verify that we didn't send any new frames
    // Now first condition is false, should set g_both_conditions_met to 0
    EXPECT_EQ(expr_var_value(mbat0caprel_var), mbat0caprel_value);
    EXPECT_EQ(expr_var_value(ac0on_var), ac0on_value);        // Still 1
    EXPECT_EQ(expr_var_value(g_both_conditions_met_var), 0);  // First condition false

    vPortResumeScheduler();
}

/**
 * @brief Test case for DDM2 parameters with OR mask behavior:
 *
 * Scenario:
 * 1. Load a rule with OR mask that can be triggered by any of multiple DDM2 parameters
 * 2. Update first parameter - rule should execute completely
 * 3. Update second parameter - rule should execute again
 * 4. Verify OR mask logic allows rule execution on any parameter change
 * 5. Test that each parameter update independently triggers the rule
 */
TEST_F(RuleEngineEvaluatorTestFixture, VerifyDDM2ParameterTriggeringWithORMask)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    // Rule with OR mask - any parameter update should trigger execution
    const char *rule_str = "mbat0caprel, ac0on, mask=['or']: g_last_trigger_source = mbat0caprel * 100 + ac0on, g_execution_count = g_execution_count + 1";

    static const struct rule_engine__rule rules[] = {
        {.name = "ORMaskRule", .rule = const_cast<char *>(rule_str), .size = static_cast<int>(strlen(rule_str))},
    };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Load the rule
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);

    struct rule_engine__specification *rule_spec = getSpecificationById(0);
    ASSERT_NE(rule_spec, nullptr) << "Failed to retrieve rule specification";

    // Check rule variables
    struct expr_var *mbat0caprel_var = getVariableByName(rule_spec, "mbat0caprel");
    ASSERT_NE(mbat0caprel_var, nullptr);
    EXPECT_EQ(expr_var_value(mbat0caprel_var), 0);  // Default value
    struct expr_var *ac0on_var = getVariableByName(rule_spec, "ac0on");
    ASSERT_NE(ac0on_var, nullptr);
    EXPECT_EQ(expr_var_value(ac0on_var), 0);  // Default value
    struct expr_var *g_last_trigger_source_var = getVariableByName(rule_spec, "g_last_trigger_source");
    ASSERT_NE(g_last_trigger_source_var, nullptr);
    EXPECT_EQ(expr_var_value(g_last_trigger_source_var), 0);
    struct expr_var *g_execution_count_var = getVariableByName(rule_spec, "g_execution_count");
    ASSERT_NE(g_execution_count_var, nullptr);
    EXPECT_EQ(expr_var_value(g_execution_count_var), 0);

    // Verify OR mask setup
    EXPECT_EQ(getTriggerListSize(rule_spec), 2);
    EXPECT_EQ(getMaskType(rule_spec), RULE_ENGINE__MASK_CONSTRAINT_OR);
    EXPECT_TRUE(isVariableInTriggerList(rule_spec, mbat0caprel_var));
    EXPECT_TRUE(isVariableInTriggerList(rule_spec, ac0on_var));
    EXPECT_TRUE(isVariableInMask(rule_spec, mbat0caprel_var));
    EXPECT_TRUE(isVariableInMask(rule_spec, ac0on_var));

    // Clear subscription frames
    clearSentDDMP2Frames();

    // Activate rule engine
    // NOTE: This test simulates immediate activation with subsequent frame processing in the same iteration.
    // The next rule_engine_task_process() call will: activate the engine, send resubscriptions, and process frames.
    // In real-world scenario, RULE0ACT activation (via external connectors or GW0INV logic) and subsequent publish
    // frames would be processed across separate iterations, not bundled together as in this test scenario.
    rule_engine_activate(&rule_engine_inst, 1);

    // Test 1: Update only mbat0caprel - should execute completely due to OR mask
    int32_t mbat0caprel_value = 45;
    receivePublishFrameFromBrokerMimic(&myFrame, MBAT0CAPREL, &mbat0caprel_value, sizeof(mbat0caprel_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    // Verify re-subscription was sent, as the rule engine was activated
    EXPECT_EQ(getNumSentDDMP2Frames(), 2);
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE);
    EXPECT_EQ(myFrame.frame.subscribe.parameter, AC0ON);
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE);
    EXPECT_EQ(myFrame.frame.subscribe.parameter, MBAT0CAPREL);
    // Verify rule executed completely with OR mask
    EXPECT_EQ(expr_var_value(mbat0caprel_var), mbat0caprel_value);
    EXPECT_EQ(expr_var_value(ac0on_var), 0);                                            // Still default
    EXPECT_EQ(expr_var_value(g_last_trigger_source_var), mbat0caprel_value * 100 + 0);  // mbat0caprel(45)*100 + ac0on_var(0)
    EXPECT_EQ(expr_var_value(g_execution_count_var), 1);

    // Test 2: Update ac0on parameter - should execute again due to OR mask
    int32_t ac0on_value = 1;
    receivePublishFrameFromBrokerMimic(&myFrame, AC0ON, &ac0on_value, sizeof(ac0on_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(getNumSentDDMP2Frames(), 0);
    // Verify rule executed again
    EXPECT_EQ(expr_var_value(mbat0caprel_var), mbat0caprel_value);  // Still 45
    EXPECT_EQ(expr_var_value(ac0on_var), ac0on_value);
    EXPECT_EQ(expr_var_value(g_last_trigger_source_var), mbat0caprel_value * 100 + ac0on_value);  // 4501
    EXPECT_EQ(expr_var_value(g_execution_count_var), 2);

    // Test 3: Update mbat0caprel again - should execute a third time
    mbat0caprel_value = 80;
    receivePublishFrameFromBrokerMimic(&myFrame, MBAT0CAPREL, &mbat0caprel_value, sizeof(mbat0caprel_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(getNumSentDDMP2Frames(), 0);
    // Verify rule executed a third time
    EXPECT_EQ(expr_var_value(mbat0caprel_var), mbat0caprel_value);                                // Now 80
    EXPECT_EQ(expr_var_value(ac0on_var), ac0on_value);                                            // Still 1
    EXPECT_EQ(expr_var_value(g_last_trigger_source_var), mbat0caprel_value * 100 + ac0on_value);  // 8001
    EXPECT_EQ(expr_var_value(g_execution_count_var), 3);
    // Verify variable consistency
    EXPECT_TRUE(verifyVariableValueThroughRules("g_last_trigger_source", mbat0caprel_value * 100 + ac0on_value));
    EXPECT_TRUE(verifyVariableValueThroughRules("g_execution_count", 3));

    vPortResumeScheduler();
}

/**
 * @brief Test case for DDM2 parameters with OR mask behavior:
 *
 * Scenario:
 * 1. Load rule with multiple global variables in OR mask
 * 2. Verify any global variable change triggers the rule
 * 3. Test multiple global variable updates from different sources
 * 4. Verify rule executes independently for each global variable change
 */
TEST_F(RuleEngineEvaluatorTestFixture, VerifyGlobalVariableTriggeringWithORMask)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    // Setup rules with OR mask for global variables
    const char *rule0_str = "mbat0caprel: g_temp1 = mbat0caprel";
    const char *rule1_str = "ac0on: g_temp2 = ac0on";
    const char *rule2_str = "g_temp1, g_temp2, mask=['or']: g_last_sensor = g_temp1 * 1000 + g_temp2, g_trigger_count = g_trigger_count + 1";

    static const struct rule_engine__rule rules[] = {
        {.name = "Sensor1Rule", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
        {.name = "Sensor2Rule", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
        {.name = "ORMaskRule", .rule = const_cast<char *>(rule2_str), .size = static_cast<int>(strlen(rule2_str))},
    };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Load rules
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[1]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[2]), 1);

    struct rule_engine__specification *rule2_spec = getSpecificationById(2);
    ASSERT_NE(rule2_spec, nullptr);

    // Verify OR mask setup
    EXPECT_EQ(getMaskType(rule2_spec), RULE_ENGINE__MASK_CONSTRAINT_OR);
    EXPECT_EQ(getTriggerListSize(rule2_spec), 2);

    struct expr_var *g_temp1_var = getVariableByName(rule2_spec, "g_temp1");
    struct expr_var *g_temp2_var = getVariableByName(rule2_spec, "g_temp2");
    ASSERT_NE(g_temp1_var, nullptr);
    ASSERT_NE(g_temp2_var, nullptr);

    EXPECT_TRUE(isVariableInTriggerList(rule2_spec, g_temp1_var));
    EXPECT_TRUE(isVariableInTriggerList(rule2_spec, g_temp2_var));

    // Clear subscription frames
    clearSentDDMP2Frames();

    // Activate rule engine
    // NOTE: This test simulates immediate activation with subsequent frame processing in the same iteration.
    // The next rule_engine_task_process() call will: activate the engine, send resubscriptions, and process frames.
    // In real-world scenario, RULE0ACT activation (via external connectors or GW0INV logic) and subsequent publish
    // frames would be processed across separate iterations, not bundled together as in this test scenario.
    rule_engine_activate(&rule_engine_inst, 1);

    // Test 1: Update temp1 via mbat0caprel
    int32_t mbat0caprel_value = 15;
    receivePublishFrameFromBrokerMimic(&myFrame, MBAT0CAPREL, &mbat0caprel_value, sizeof(mbat0caprel_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    // Verify re-subscription was sent, as the rule engine was activated
    EXPECT_EQ(getNumSentDDMP2Frames(), 2);
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE);
    EXPECT_EQ(myFrame.frame.subscribe.parameter, MBAT0CAPREL);
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE);
    EXPECT_EQ(myFrame.frame.subscribe.parameter, AC0ON);

    // Verify g_temp1 updated and triggered OR mask rule
    EXPECT_TRUE(verifyVariableValueThroughRules("g_temp1", mbat0caprel_value));
    EXPECT_TRUE(verifyVariableValueThroughRules("g_temp2", 0));                               // Still default
    EXPECT_TRUE(verifyVariableValueThroughRules("g_last_sensor", mbat0caprel_value * 1000));  // 15000
    EXPECT_TRUE(verifyVariableValueThroughRules("g_trigger_count", 1));

    // Test 2: Update sensor2 via ac0on
    int32_t ac0on_value = 3;
    receivePublishFrameFromBrokerMimic(&myFrame, AC0ON, &ac0on_value, sizeof(ac0on_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(getNumSentDDMP2Frames(), 0);
    // Verify g_temp2 updated and triggered OR mask rule again
    EXPECT_TRUE(verifyVariableValueThroughRules("g_temp1", mbat0caprel_value));                             // Still 15
    EXPECT_TRUE(verifyVariableValueThroughRules("g_temp2", ac0on_value));                                   // Now 3
    EXPECT_TRUE(verifyVariableValueThroughRules("g_last_sensor", mbat0caprel_value * 1000 + ac0on_value));  // 15003
    EXPECT_TRUE(verifyVariableValueThroughRules("g_trigger_count", 2));

    // Test 3: Update mbat0caprel again - should execute a third time
    mbat0caprel_value = 20;
    receivePublishFrameFromBrokerMimic(&myFrame, MBAT0CAPREL, &mbat0caprel_value, sizeof(mbat0caprel_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(getNumSentDDMP2Frames(), 0);
    // Verify OR mask rule triggered a third time
    EXPECT_TRUE(verifyVariableValueThroughRules("g_temp1", mbat0caprel_value));                             // Now 20
    EXPECT_TRUE(verifyVariableValueThroughRules("g_temp2", ac0on_value));                                   // Still 3
    EXPECT_TRUE(verifyVariableValueThroughRules("g_last_sensor", mbat0caprel_value * 1000 + ac0on_value));  // 20003
    EXPECT_TRUE(verifyVariableValueThroughRules("g_trigger_count", 3));

    vPortResumeScheduler();
}

/*******************************************************************************
 * Global Variable Triggering Tests
 *******************************************************************************/

/**
 * @brief Test case for basic global variable triggering between rules:
 *
 * Scenario:
 * 1. Load two rules:
 *    - Rule 0: Updates g_shared_value when DDM2 parameter changes
 *    - Rule 1: Triggered by g_shared_value changes, performs calculations
 * 2. Send DDMP2 frame to trigger Rule 0
 * 3. Verify Rule 0 updates g_shared_value
 * 4. Verify Rule 1 is triggered by g_shared_value change and executes properly
 * 5. Test global variable state consistency across rules
 */
TEST_F(RuleEngineEvaluatorTestFixture, VerifyGlobalVariableTriggeringSimple)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    // Rule 0: Updates global variable when DDM2 parameter changes
    const char *rule0_str = "mbat0caprel: g_shared_value = mbat0caprel * 2, g_debug_counter = g_debug_counter + 1";
    // Rule 1: Triggered by global variable change
    const char *rule1_str = "g_shared_value: g_result = g_shared_value + 100, g_rule1_executions = g_rule1_executions + 1";

    static const struct rule_engine__rule rules[] = {
        {.name = "GlobalUpdateRule", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
        {.name = "GlobalTriggerRule", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
    };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Load rules
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[1]), 1);

    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    ASSERT_NE(rule0_spec, nullptr) << "Failed to retrieve rule 0 specification";
    struct rule_engine__specification *rule1_spec = getSpecificationById(1);
    ASSERT_NE(rule1_spec, nullptr) << "Failed to retrieve rule 1 specification";

    // Verify Rule 0 variables
    struct expr_var *mbat0caprel_var = getVariableByName(rule0_spec, "mbat0caprel");
    ASSERT_NE(mbat0caprel_var, nullptr);
    EXPECT_EQ(expr_var_type(mbat0caprel_var), RULE_ENGINE__VAR_TYPE_DDM2);
    EXPECT_EQ(expr_var_value(mbat0caprel_var), 0);

    struct expr_var *g_shared_value_var_r0 = getVariableByName(rule0_spec, "g_shared_value");
    ASSERT_NE(g_shared_value_var_r0, nullptr);
    EXPECT_EQ(expr_var_type(g_shared_value_var_r0), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_value(g_shared_value_var_r0), 0);

    struct expr_var *g_debug_counter_var = getVariableByName(rule0_spec, "g_debug_counter");
    ASSERT_NE(g_debug_counter_var, nullptr);
    EXPECT_EQ(expr_var_type(g_debug_counter_var), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_value(g_debug_counter_var), 0);

    // Verify Rule 1 variables
    struct expr_var *g_shared_value_var_r1 = getVariableByName(rule1_spec, "g_shared_value");
    ASSERT_NE(g_shared_value_var_r1, nullptr);
    EXPECT_EQ(expr_var_type(g_shared_value_var_r1), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_value(g_shared_value_var_r1), 0);

    struct expr_var *g_result_var = getVariableByName(rule1_spec, "g_result");
    ASSERT_NE(g_result_var, nullptr);
    EXPECT_EQ(expr_var_type(g_result_var), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_value(g_result_var), 0);

    struct expr_var *g_rule1_executions_var = getVariableByName(rule1_spec, "g_rule1_executions");
    ASSERT_NE(g_rule1_executions_var, nullptr);
    EXPECT_EQ(expr_var_type(g_rule1_executions_var), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_value(g_rule1_executions_var), 0);

    // Verify trigger list setup
    EXPECT_EQ(getTriggerListSize(rule0_spec), 1);
    EXPECT_TRUE(isVariableInTriggerList(rule0_spec, mbat0caprel_var));

    EXPECT_EQ(getTriggerListSize(rule1_spec), 1);
    EXPECT_TRUE(isVariableInTriggerList(rule1_spec, g_shared_value_var_r1));

    // Clear subscription frames
    clearSentDDMP2Frames();

    // Activate rule engine
    // NOTE: This test simulates immediate activation with subsequent frame processing in the same iteration.
    // The next rule_engine_task_process() call will: activate the engine, send resubscriptions, and process frames.
    // In real-world scenario, RULE0ACT activation (via external connectors or GW0INV logic) and subsequent publish
    // frames would be processed across separate iterations, not bundled together as in this test scenario.
    rule_engine_activate(&rule_engine_inst, 1);

    // Send DDMP2 frame to trigger Rule 0
    int32_t mbat0caprel_value = 30;
    receivePublishFrameFromBrokerMimic(&myFrame, MBAT0CAPREL, &mbat0caprel_value, sizeof(mbat0caprel_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    // Verify re-subscription was sent, as the rule engine was activated
    EXPECT_EQ(getNumSentDDMP2Frames(), 1);
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE);
    EXPECT_EQ(myFrame.frame.subscribe.parameter, MBAT0CAPREL);
    // Verify Rule 0 executed and updated global variable
    EXPECT_EQ(expr_var_value(mbat0caprel_var), mbat0caprel_value);
    EXPECT_EQ(expr_var_value(g_shared_value_var_r0), mbat0caprel_value * 2);  // 60
    EXPECT_EQ(expr_var_value(g_debug_counter_var), 1);
    // Verify Rule 1 was triggered by global variable change and executed
    EXPECT_EQ(expr_var_value(g_shared_value_var_r1), mbat0caprel_value * 2);  // 60
    EXPECT_EQ(expr_var_value(g_result_var), (mbat0caprel_value * 2) + 100);   // 160
    EXPECT_EQ(expr_var_value(g_rule1_executions_var), 1);
    // Verify global variable consistency across rules
    EXPECT_TRUE(verifyVariableValueThroughRules("mbat0caprel", mbat0caprel_value));
    EXPECT_TRUE(verifyVariableValueThroughRules("g_shared_value", 60));
    EXPECT_TRUE(verifyVariableValueThroughRules("g_result", 160));
    EXPECT_TRUE(verifyVariableValueThroughRules("g_debug_counter", 1));
    EXPECT_TRUE(verifyVariableValueThroughRules("g_rule1_executions", 1));

    // Test second trigger to verify consistent behavior
    mbat0caprel_value = 50;
    receivePublishFrameFromBrokerMimic(&myFrame, MBAT0CAPREL, &mbat0caprel_value, sizeof(mbat0caprel_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(getNumSentDDMP2Frames(), 0);
    // Verify both rules executed again
    EXPECT_EQ(expr_var_value(g_shared_value_var_r0), mbat0caprel_value * 2);  // 100
    EXPECT_EQ(expr_var_value(g_debug_counter_var), 2);
    EXPECT_EQ(expr_var_value(g_result_var), (mbat0caprel_value * 2) + 100);  // 200
    EXPECT_EQ(expr_var_value(g_rule1_executions_var), 2);
    // Verify global variable consistency after second trigger
    EXPECT_TRUE(verifyVariableValueThroughRules("mbat0caprel", mbat0caprel_value));
    EXPECT_TRUE(verifyVariableValueThroughRules("g_shared_value", 100));
    EXPECT_TRUE(verifyVariableValueThroughRules("g_result", 200));
    EXPECT_TRUE(verifyVariableValueThroughRules("g_debug_counter", 2));
    EXPECT_TRUE(verifyVariableValueThroughRules("g_rule1_executions", 2));

    vPortResumeScheduler();
}

/**
 * @brief Test case for multiple global variable triggers in a chain:
 *
 * Scenario:
 * 1. Load three rules:
 *    - Rule 0: Updates g_var1 when DDM2 parameter changes
 *    - Rule 1: Updates g_var2 when g_var1 changes
 *    - Rule 2: Updates g_var3 when g_var2 changes
 * 2. Send DDMP2 frame to trigger the chain
 * 3. Verify cascading global variable updates through all rules
 * 4. Test that each rule in the chain executes exactly once per trigger
 */
TEST_F(RuleEngineEvaluatorTestFixture, VerifyGlobalVariableTriggeringChain)
{
    DDMP2_FRAME myFrame;

    // Rule chain with global variable triggers
    const char *rule0_str = "ac0on: g_var1 = ac0on * 10, g_step1_count = g_step1_count + 1";
    const char *rule1_str = "g_var1: g_var2 = g_var1 + 200, g_step2_count = g_step2_count + 1";
    const char *rule2_str = "g_var2: g_var3 = g_var2 * 3, g_step3_count = g_step3_count + 1";

    static const struct rule_engine__rule rules[] = {
        {.name = "ChainRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
        {.name = "ChainRule1", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
        {.name = "ChainRule2", .rule = const_cast<char *>(rule2_str), .size = static_cast<int>(strlen(rule2_str))},
    };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Load rules
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[1]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[2]), 1);

    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    struct rule_engine__specification *rule1_spec = getSpecificationById(1);
    struct rule_engine__specification *rule2_spec = getSpecificationById(2);
    ASSERT_NE(rule0_spec, nullptr);
    ASSERT_NE(rule1_spec, nullptr);
    ASSERT_NE(rule2_spec, nullptr);

    // Get all variables and verify initial state
    struct expr_var *ac0on_var = getVariableByName(rule0_spec, "ac0on");
    struct expr_var *g_var1_var_r0 = getVariableByName(rule0_spec, "g_var1");
    struct expr_var *g_var1_var_r1 = getVariableByName(rule1_spec, "g_var1");
    struct expr_var *g_var2_var_r1 = getVariableByName(rule1_spec, "g_var2");
    struct expr_var *g_var2_var_r2 = getVariableByName(rule2_spec, "g_var2");
    struct expr_var *g_var3_var = getVariableByName(rule2_spec, "g_var3");

    ASSERT_NE(ac0on_var, nullptr);
    ASSERT_NE(g_var1_var_r0, nullptr);
    ASSERT_NE(g_var1_var_r1, nullptr);
    ASSERT_NE(g_var2_var_r1, nullptr);
    ASSERT_NE(g_var2_var_r2, nullptr);
    ASSERT_NE(g_var3_var, nullptr);

    EXPECT_EQ(expr_var_value(ac0on_var), 0);      // Default value
    EXPECT_EQ(expr_var_value(g_var1_var_r0), 0);  // Default value
    EXPECT_EQ(expr_var_value(g_var1_var_r1), 0);  // Default value
    EXPECT_EQ(expr_var_value(g_var2_var_r1), 0);  // Default value
    EXPECT_EQ(expr_var_value(g_var2_var_r2), 0);  // Default value
    EXPECT_EQ(expr_var_value(g_var3_var), 0);     // Default value

    // Verify trigger lists
    EXPECT_TRUE(isVariableInTriggerList(rule0_spec, ac0on_var));
    EXPECT_TRUE(isVariableInTriggerList(rule1_spec, g_var1_var_r1));
    EXPECT_TRUE(isVariableInTriggerList(rule2_spec, g_var2_var_r2));

    // Remove subscription frames
    clearSentDDMP2Frames();

    // Activate rule engine
    // NOTE: This test simulates immediate activation with subsequent frame processing in the same iteration.
    // The next rule_engine_task_process() call will: activate the engine, send resubscriptions, and process frames.
    // In real-world scenario, RULE0ACT activation (via external connectors or GW0INV logic) and subsequent publish
    // frames would be processed across separate iterations, not bundled together as in this test scenario.
    rule_engine_activate(&rule_engine_inst, 1);

    // Trigger the chain
    int32_t ac0on_value = 5;
    receivePublishFrameFromBrokerMimic(&myFrame, AC0ON, &ac0on_value, sizeof(ac0on_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(getNumSentDDMP2Frames(), 1);  // re-subscription frames sent
    clearSentDDMP2Frames();                 // Clear sent resubscription frames

    // Verify the chain executed completely
    EXPECT_EQ(expr_var_value(ac0on_var), ac0on_value);                      // 5
    EXPECT_EQ(expr_var_value(g_var1_var_r0), ac0on_value * 10);             // 50
    EXPECT_EQ(expr_var_value(g_var1_var_r1), ac0on_value * 10);             // 50
    EXPECT_EQ(expr_var_value(g_var2_var_r1), (ac0on_value * 10) + 200);     // 250
    EXPECT_EQ(expr_var_value(g_var2_var_r2), (ac0on_value * 10) + 200);     // 250
    EXPECT_EQ(expr_var_value(g_var3_var), ((ac0on_value * 10) + 200) * 3);  // 750

    // Verify step counters
    EXPECT_TRUE(verifyVariableValueThroughRules("g_step1_count", 1));
    EXPECT_TRUE(verifyVariableValueThroughRules("g_step2_count", 1));
    EXPECT_TRUE(verifyVariableValueThroughRules("g_step3_count", 1));

    // Verify global variable consistency
    EXPECT_TRUE(verifyVariableValueThroughRules("g_var1", 50));
    EXPECT_TRUE(verifyVariableValueThroughRules("g_var2", 250));
    EXPECT_TRUE(verifyVariableValueThroughRules("g_var3", 750));

    vPortResumeScheduler();
}

/**
 * @brief Test case for global variable triggering with initialized global variables:
 *
 * Scenario:
 * 1. Load rules with initialized global variables
 * 2. Verify initialized global variables are NOT in trigger lists (auto-generated mask)
 * 3. Test that rules with initialized globals still work correctly when triggered by other means
 * 4. Verify initialized values are preserved and shared across rules
 */
TEST_F(RuleEngineEvaluatorTestFixture, VerifyGlobalVariableTriggeringWithInitialization)
{
    DDMP2_FRAME myFrame;

    // Rules with initialized global variables
    const char *rule0_str = "g_config = 100, mbat0caprel: g_calculated = mbat0caprel + g_config, g_operations = g_operations + 1";
    const char *rule1_str = "g_calculated: g_final_result = g_calculated * 2, g_config = g_config + 10";

    static const struct rule_engine__rule rules[] = {
        {.name = "InitGlobalRule", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
        {.name = "UseGlobalRule", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
    };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Load rules
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[1]), 1);

    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    struct rule_engine__specification *rule1_spec = getSpecificationById(1);
    ASSERT_NE(rule0_spec, nullptr);
    ASSERT_NE(rule1_spec, nullptr);

    // Verify variables and their initial values
    struct expr_var *g_config_var_r0 = getVariableByName(rule0_spec, "g_config");
    struct expr_var *g_config_var_r1 = getVariableByName(rule1_spec, "g_config");
    struct expr_var *mbat0caprel_var = getVariableByName(rule0_spec, "mbat0caprel");
    struct expr_var *g_calculated_var_r0 = getVariableByName(rule0_spec, "g_calculated");
    struct expr_var *g_calculated_var_r1 = getVariableByName(rule1_spec, "g_calculated");

    ASSERT_NE(g_config_var_r0, nullptr);
    ASSERT_NE(g_config_var_r1, nullptr);
    ASSERT_NE(mbat0caprel_var, nullptr);
    ASSERT_NE(g_calculated_var_r0, nullptr);
    ASSERT_NE(g_calculated_var_r1, nullptr);

    // Verify initialized global variable value
    EXPECT_EQ(expr_var_value(g_config_var_r0), 100);
    EXPECT_EQ(expr_var_value(g_config_var_r1), 100);

    // Verify trigger lists - initialized global should NOT be in trigger list for rule0
    EXPECT_EQ(getTriggerListSize(rule0_spec), 1);
    EXPECT_TRUE(isVariableInTriggerList(rule0_spec, mbat0caprel_var));
    EXPECT_FALSE(isVariableInTriggerList(rule0_spec, g_config_var_r0));  // Initialized, so not in trigger

    // Rule1 should have g_calculated in trigger list (not initialized in rule1)
    EXPECT_EQ(getTriggerListSize(rule1_spec), 1);
    EXPECT_TRUE(isVariableInTriggerList(rule1_spec, g_calculated_var_r1));

    // Remove subscription frames
    clearSentDDMP2Frames();

    // Activate rule engine
    // NOTE: This test simulates immediate activation with subsequent frame processing in the same iteration.
    // The next rule_engine_task_process() call will: activate the engine, send resubscriptions, and process frames.
    // In real-world scenario, RULE0ACT activation (via external connectors or GW0INV logic) and subsequent publish
    // frames would be processed across separate iterations, not bundled together as in this test scenario.
    rule_engine_activate(&rule_engine_inst, 1);

    // Trigger rule0 with DDM2 parameter
    int32_t mbat0caprel_value = 25;
    receivePublishFrameFromBrokerMimic(&myFrame, MBAT0CAPREL, &mbat0caprel_value, sizeof(mbat0caprel_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(getNumSentDDMP2Frames(), 1);  // re-subscription frames sent
    clearSentDDMP2Frames();                 // Clear sent resubscription frames

    // Verify rule0 executed and used initialized value
    EXPECT_EQ(expr_var_value(mbat0caprel_var), mbat0caprel_value);
    EXPECT_EQ(expr_var_value(g_calculated_var_r0), mbat0caprel_value + 100);  // 125
    EXPECT_TRUE(verifyVariableValueThroughRules("g_operations", 1));
    // Verify rule1 was triggered by g_calculated change
    EXPECT_EQ(expr_var_value(g_calculated_var_r1), mbat0caprel_value + 100);                        // 125
    EXPECT_TRUE(verifyVariableValueThroughRules("g_final_result", (mbat0caprel_value + 100) * 2));  // 250
    // Verify g_config was updated by rule1 and is consistent
    EXPECT_TRUE(verifyVariableValueThroughRules("g_config", 110));  // 100 + 10

    vPortResumeScheduler();
}

// ==================== LOCAL VARIABLE SCOPE TESTS ====================

/**
 * Test that local variables in different rules are properly isolated
 * and don't interfere with each other, even when they have the same name.
 */
TEST_F(RuleEngineEvaluatorTestFixture, VerifyLocalVariableScopeIsolation)
{
    DDMP2_FRAME myFrame;

    // Rule 0: Uses local variable 'local_val'
    const char *rule0_spec = "mbat0caprel: local_val = mbat0caprel * 2, g_result0 = local_val";
    // Rule 1: Uses local variable with same name 'local_val' but different logic
    const char *rule1_spec = "ac0ttemp: local_val = ac0ttemp + 100, g_result1 = local_val";

    static const struct rule_engine__rule rules[] = {
        {.name = "LocalRule1", .rule = const_cast<char *>(rule0_spec), .size = static_cast<int>(strlen(rule0_spec))},
        {.name = "LocalRule2", .rule = const_cast<char *>(rule1_spec), .size = static_cast<int>(strlen(rule1_spec))},
    };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[1]), 1);

    struct rule_engine__specification *rule0_spec_ptr = getSpecificationById(0);
    struct rule_engine__specification *rule1_spec_ptr = getSpecificationById(1);
    ASSERT_NE(rule0_spec_ptr, nullptr);
    ASSERT_NE(rule1_spec_ptr, nullptr);

    // Verify local variables are properly isolated
    struct expr_var *local_val_r0 = getVariableByName(rule0_spec_ptr, "local_val");
    struct expr_var *local_val_r1 = getVariableByName(rule1_spec_ptr, "local_val");
    ASSERT_NE(local_val_r0, nullptr);
    ASSERT_NE(local_val_r1, nullptr);

    // Local variables should not be in trigger lists
    EXPECT_FALSE(isVariableInTriggerList(rule0_spec_ptr, local_val_r0));
    EXPECT_FALSE(isVariableInTriggerList(rule1_spec_ptr, local_val_r1));

    // Remove subscription frames
    clearSentDDMP2Frames();

    // Activate rule engine
    // NOTE: This test simulates immediate activation with subsequent frame processing in the same iteration.
    // The next rule_engine_task_process() call will: activate the engine, send resubscriptions, and process frames.
    // In real-world scenario, RULE0ACT activation (via external connectors or GW0INV logic) and subsequent publish
    // frames would be processed across separate iterations, not bundled together as in this test scenario.
    rule_engine_activate(&rule_engine_inst, 1);

    // Trigger rule0 with mbat0caprel
    int32_t mbat0caprel_value = 15;
    receivePublishFrameFromBrokerMimic(&myFrame, MBAT0CAPREL, &mbat0caprel_value, sizeof(mbat0caprel_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(getNumSentDDMP2Frames(), 2);  // re-subscription frames sent
    clearSentDDMP2Frames();                 // Clear sent resubscription frames
    // Verify rule0 executed with its local_val = 30
    EXPECT_EQ(expr_var_value(local_val_r0), 30);
    EXPECT_TRUE(verifyVariableValueThroughRules("g_result0", 30));
    EXPECT_EQ(expr_var_value(local_val_r1), 0);  // Verify rule1's local_val is still unchanged

    // Trigger rule1 with ac0ttemp
    int32_t ac0ttemp_value = 25;
    receivePublishFrameFromBrokerMimic(&myFrame, AC0TTEMP, &ac0ttemp_value, sizeof(ac0ttemp_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(getNumSentDDMP2Frames(), 0);
    // Verify rule1 executed with its local_val = 125
    EXPECT_EQ(expr_var_value(local_val_r1), 125);
    EXPECT_TRUE(verifyVariableValueThroughRules("g_result1", 125));
    // Verify rule0's local_val is still 30 (unchanged)
    EXPECT_EQ(expr_var_value(local_val_r0), 30);
    EXPECT_TRUE(verifyVariableValueThroughRules("g_result0", 30));

    vPortResumeScheduler();
}

/**
 * Test local variable persistence within a single rule execution
 * and verify that local variables reset properly between rule executions.
 */
TEST_F(RuleEngineEvaluatorTestFixture, VerifyLocalVariablePersistenceInRule)
{
    DDMP2_FRAME myFrame;

    // Rule with multiple local variable operations in sequence
    const char *rule0_spec = "mbat0caprel: temp1 = mbat0caprel + 10, temp2 = temp1 * 2, g_result = temp1 + temp2";

    static const struct rule_engine__rule rules[] = {
        {.name = "LocalRule1", .rule = const_cast<char *>(rule0_spec), .size = static_cast<int>(strlen(rule0_spec))},
    };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);

    struct rule_engine__specification *rule0_spec_ptr = getSpecificationById(0);
    ASSERT_NE(rule0_spec_ptr, nullptr);

    struct expr_var *temp1_var = getVariableByName(rule0_spec_ptr, "temp1");
    struct expr_var *temp2_var = getVariableByName(rule0_spec_ptr, "temp2");
    ASSERT_NE(temp1_var, nullptr);
    ASSERT_NE(temp2_var, nullptr);

    clearSentDDMP2Frames();

    // Activate rule engine
    // NOTE: This test simulates immediate activation with subsequent frame processing in the same iteration.
    // The next rule_engine_task_process() call will: activate the engine, send resubscriptions, and process frames.
    // In real-world scenario, RULE0ACT activation (via external connectors or GW0INV logic) and subsequent publish
    // frames would be processed across separate iterations, not bundled together as in this test scenario.
    rule_engine_activate(&rule_engine_inst, 1);

    // First execution with mbat0caprel = 5
    int32_t mbat0caprel_value = 5;
    receivePublishFrameFromBrokerMimic(&myFrame, MBAT0CAPREL, &mbat0caprel_value, sizeof(mbat0caprel_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(getNumSentDDMP2Frames(), 1);  // re-subscription frames sent
    clearSentDDMP2Frames();                 // Clear sent resubscription frames
    // Verify calculations: temp1 = 15, temp2 = 30, result = 45
    EXPECT_EQ(expr_var_value(temp1_var), 15);
    EXPECT_EQ(expr_var_value(temp2_var), 30);
    EXPECT_TRUE(verifyVariableValueThroughRules("g_result", 45));

    // Second execution with mbat0caprel = 10
    mbat0caprel_value = 10;
    receivePublishFrameFromBrokerMimic(&myFrame, MBAT0CAPREL, &mbat0caprel_value, sizeof(mbat0caprel_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(getNumSentDDMP2Frames(), 0);
    // Verify new calculations: temp1 = 20, temp2 = 40, result = 60
    EXPECT_EQ(expr_var_value(temp1_var), 20);
    EXPECT_EQ(expr_var_value(temp2_var), 40);
    EXPECT_TRUE(verifyVariableValueThroughRules("g_result", 60));

    vPortResumeScheduler();
}

/***********************
 * PUBLISHING AND SET OPERATIONS TESTS
 ***********************/

/**
 * Test DDM2 parameter publishing from rules using SET operations
 * and verify proper frame generation and transmission.
 */
TEST_F(RuleEngineEvaluatorTestFixture, VerifyDDM2ParameterPublishing)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    // Rule that publishes to DDM2 parameters
    const char *rule0_spec = "mbat0caprel: pub(ac0ttemp=mbat0caprel)";
    const char *rule1_spec = "mbat0caprel: pub(ac0on=1)";

    static const struct rule_engine__rule rules[] = {
        {.name = "LocalRule1", .rule = const_cast<char *>(rule0_spec), .size = static_cast<int>(strlen(rule0_spec))},
        {.name = "LocalRule2", .rule = const_cast<char *>(rule1_spec), .size = static_cast<int>(strlen(rule1_spec))},
    };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[1]), 1);

    // Remove subscription frames
    clearSentDDMP2Frames();

    // Activate rule engine
    // NOTE: This test simulates immediate activation with subsequent frame processing in the same iteration.
    // The next rule_engine_task_process() call will: activate the engine, send resubscriptions, and process frames.
    // In real-world scenario, RULE0ACT activation (via external connectors or GW0INV logic) and subsequent publish
    // frames would be processed across separate iterations, not bundled together as in this test scenario.
    rule_engine_activate(&rule_engine_inst, 1);

    // Trigger rule with mbat0caprel
    int32_t mbat0caprel_value = 25;
    receivePublishFrameFromBrokerMimic(&myFrame, MBAT0CAPREL, &mbat0caprel_value, sizeof(mbat0caprel_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(getNumSentDDMP2Frames(), 3);  // Should have SET 2 frames and SUBSCRIBE 1 frames
    // Read the one subscription frame in order to clear it
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    // Verify SET frame for ac0ttemp and ac0on
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SET);
    EXPECT_EQ(myFrame.frame.set.parameter, AC0TTEMP);
    EXPECT_EQ(myFrame.frame.set.value.int32, mbat0caprel_value);
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SET);
    EXPECT_EQ(myFrame.frame.set.parameter, AC0ON);
    EXPECT_EQ(myFrame.frame.set.value.int32, 1);

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    mbat0caprel_value = 50;
    receivePublishFrameFromBrokerMimic(&myFrame, MBAT0CAPREL, &mbat0caprel_value, sizeof(mbat0caprel_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(getNumSentDDMP2Frames(), 1);  // Should have SET 1 frames
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SET);
    EXPECT_EQ(myFrame.frame.set.parameter, AC0TTEMP);
    EXPECT_EQ(myFrame.frame.set.value.int32, mbat0caprel_value);

    vPortResumeScheduler();
}

/**
 * @brief Test case for rule chaining and conditional variable publishing:
 *
 * Scenario:
 * 1. Load two interdependent rules:
 *    - Rule 0: Monitors mbat0caprel parameter, sets g_mbat0caprel and activates g_determine_caprel when value changes
 *    - Rule 1: Triggered by g_determine_caprel, evaluates g_mbat0caprel value and conditionally publishes parameters
 * 2. Send a DDMP2 frame with mbat0caprel=50
 *    - Expect Rule 0 to update g_mbat0caprel=50 and set g_determine_caprel=1
 *    - Expect Rule 1 to be triggered but no publish (g_mbat0caprel > 5)
 * 3. Send a DDMP2 frame with mbat0caprel=5
 *    - Expect Rule 0 to update g_mbat0caprel=5
 *    - Expect Rule 1 to evaluate g_mbat0caprel <= 5 and publish ac0on=1 and sysappl0smarteco=0
 * 4. Verify proper variable propagation across rules and correct DDM2 parameter handling
 */
TEST_F(RuleEngineEvaluatorTestFixture, VerifyRuleChainingAndConditionalPublishing)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    const char *rule0_str = "mbat0caprel, g_mbat0caprel = -1: if(g_mbat0caprel != mbat0caprel, g_mbat0caprel = mbat0caprel, g_determine_caprel = 1)";
    const char *rule1_str = "g_determine_caprel: if(g_mbat0caprel <= 5, pub(ac0on = 1, sysappl0smarteco = 0))";

    static const struct rule_engine__rule rules[] =
        {
            {.name = "TestRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
            {.name = "TestRule1", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
        };

    vPortPauseScheduler();

    clearSentDDMP2Frames();

    // Load first rule
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);

    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    ASSERT_NE(rule0_spec, nullptr) << "Failed to retrieve rule specification";

    // Check variables
    ddmw_item_t *mbat0caprel_item = getDDM2ParameterByID(MBAT0CAPREL);
    EXPECT_NE(mbat0caprel_item, nullptr);
    EXPECT_EQ(getDDM2ParameterValue(mbat0caprel_item), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(getDDM2ParameterType(mbat0caprel_item), DDMW_ACTION_SET);

    struct expr_var *mbat0caprel_rule0 = getVariableByName(rule0_spec, "mbat0caprel");
    ASSERT_NE(mbat0caprel_rule0, nullptr);
    EXPECT_EQ(expr_var_id(mbat0caprel_rule0), MBAT0CAPREL);
    EXPECT_EQ(expr_var_type(mbat0caprel_rule0), RULE_ENGINE__VAR_TYPE_DDM2);
    EXPECT_EQ(expr_var_value(mbat0caprel_rule0), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type_ddm_create(mbat0caprel_rule0), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED);

    struct expr_var *g_mbat0caprel_rule0 = getVariableByName(rule0_spec, "g_mbat0caprel");
    ASSERT_NE(g_mbat0caprel_rule0, nullptr);
    EXPECT_EQ(expr_var_id(g_mbat0caprel_rule0), expr_generate_hash_key("g_mbat0caprel"));
    EXPECT_EQ(expr_var_type(g_mbat0caprel_rule0), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_value(g_mbat0caprel_rule0), -1);
    EXPECT_EQ(expr_var_type_ddm_create(g_mbat0caprel_rule0), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

    struct expr_var *g_determine_caprel_rule0 = getVariableByName(rule0_spec, "g_determine_caprel");
    ASSERT_NE(g_determine_caprel_rule0, nullptr);
    EXPECT_EQ(expr_var_id(g_determine_caprel_rule0), expr_generate_hash_key("g_determine_caprel"));
    EXPECT_EQ(expr_var_type(g_determine_caprel_rule0), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_value(g_determine_caprel_rule0), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type_ddm_create(g_determine_caprel_rule0), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

    // Check trigger section
    EXPECT_EQ(getTriggerListSize(rule0_spec), 1);
    EXPECT_EQ(getMaskType(rule0_spec), RULE_ENGINE__MASK_CONSTRAINT_AND);
    EXPECT_TRUE(isVariableInTriggerList(rule0_spec, mbat0caprel_rule0));
    EXPECT_TRUE(isVariableInMask(rule0_spec, mbat0caprel_rule0));
    EXPECT_FALSE(isVariableInTriggerList(rule0_spec, g_mbat0caprel_rule0));
    EXPECT_FALSE(isVariableInMask(rule0_spec, g_mbat0caprel_rule0));
    EXPECT_FALSE(isVariableInTriggerList(rule0_spec, g_determine_caprel_rule0));
    EXPECT_FALSE(isVariableInMask(rule0_spec, g_determine_caprel_rule0));

    // Check if the rule has sent subscription frame for mbat0caprel
    EXPECT_EQ(getNumSentDDMP2Frames(), 1) << "We should have sent one DDMP2 frame" << std::endl;
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.source_connector, connector_unittest.connector_id) << "Sent frame should be from the rule engine connector";
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE);
    EXPECT_EQ(myFrame.frame.subscribe.parameter, MBAT0CAPREL)
        << "frame: 0x" << std::hex << myFrame.frame.subscribe.parameter << " vs expected: 0x" << std::hex << MBAT0CAPREL;
    // Although redudant, there is no owner in the system, so we check that we did not receive any frames
    EXPECT_EQ(getNumReceivedDDMP2Frames(), 0) << "We should NOT have received a DDMP2 frame" << std::endl;

    clearSentDDMP2Frames();

    // Load second rule
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[1]), 1);

    struct rule_engine__specification *rule1_spec = getSpecificationById(1);
    ASSERT_NE(rule1_spec, nullptr) << "Failed to retrieve rule specification";

    // Check variables
    ddmw_item_t *ac0on_item = getDDM2ParameterByID(AC0ON);
    EXPECT_NE(ac0on_item, nullptr) << "AC0ON should be in the DDM2 list" << std::endl;

    ddmw_item_t *sysappl0smarteco_item = getDDM2ParameterByID(SYSAPPL0SMARTECO);
    EXPECT_NE(sysappl0smarteco_item, nullptr) << "SYSAPPL0SMARTECO should be in the DDM2 list" << std::endl;

    struct expr_var *ac0on_rule1 = getVariableByName(rule1_spec, "ac0on");
    ASSERT_NE(ac0on_rule1, nullptr);
    EXPECT_EQ(expr_var_id(ac0on_rule1), AC0ON);
    EXPECT_EQ(expr_var_type(ac0on_rule1), RULE_ENGINE__VAR_TYPE_DDM2);
    EXPECT_EQ(expr_var_value(ac0on_rule1), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type_ddm_create(ac0on_rule1), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED);

    struct expr_var *sysappl0smarteco_rule1 = getVariableByName(rule1_spec, "sysappl0smarteco");
    ASSERT_NE(sysappl0smarteco_rule1, nullptr);
    EXPECT_EQ(expr_var_id(sysappl0smarteco_rule1), SYSAPPL0SMARTECO);
    EXPECT_EQ(expr_var_type(sysappl0smarteco_rule1), RULE_ENGINE__VAR_TYPE_DDM2);
    EXPECT_EQ(expr_var_value(sysappl0smarteco_rule1), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type_ddm_create(sysappl0smarteco_rule1), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED);

    struct expr_var *g_determine_caprel_rule1 = getVariableByName(rule1_spec, "g_determine_caprel");
    ASSERT_NE(g_determine_caprel_rule1, nullptr);
    EXPECT_EQ(expr_var_id(g_determine_caprel_rule1), expr_generate_hash_key("g_determine_caprel"));
    EXPECT_EQ(expr_var_type(g_determine_caprel_rule1), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_value(g_determine_caprel_rule1), RULE_ENGINE__VAR_VALUE_INIT);
    EXPECT_EQ(expr_var_type_ddm_create(g_determine_caprel_rule1), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

    struct expr_var *g_mbat0caprel_rule1 = getVariableByName(rule1_spec, "g_mbat0caprel");
    ASSERT_NE(g_mbat0caprel_rule1, nullptr);
    EXPECT_EQ(expr_var_id(g_mbat0caprel_rule1), expr_generate_hash_key("g_mbat0caprel"));
    EXPECT_EQ(expr_var_type(g_mbat0caprel_rule1), RULE_ENGINE__VAR_TYPE_GLOBAL);
    EXPECT_EQ(expr_var_value(g_mbat0caprel_rule1), -1);
    EXPECT_EQ(expr_var_type_ddm_create(g_mbat0caprel_rule1), RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

    // Check trigger section
    EXPECT_EQ(getTriggerListSize(rule1_spec), 1);
    EXPECT_EQ(getMaskType(rule1_spec), RULE_ENGINE__MASK_CONSTRAINT_AND);
    EXPECT_TRUE(isVariableInTriggerList(rule1_spec, g_determine_caprel_rule1));
    EXPECT_TRUE(isVariableInMask(rule1_spec, g_determine_caprel_rule1));

    /* Check if the rule has not sent subscription frame for ac0on and sysappl0smarteco
     * as they are not in the DDM2 list(since they are used only for publish)
     */
    EXPECT_EQ(getNumSentDDMP2Frames(), 0) << "We should have NOT sent DDMP2 frame" << std::endl;

    clearSentDDMP2Frames();

    // Activate rule engine
    // NOTE: This test simulates immediate activation with subsequent frame processing in the same iteration.
    // The next rule_engine_task_process() call will: activate the engine, send resubscriptions, and process frames.
    // In real-world scenario, RULE0ACT activation (via external connectors or GW0INV logic) and subsequent publish
    // frames would be processed across separate iterations, not bundled together as in this test scenario.
    rule_engine_activate(&rule_engine_inst, 1);

    /* First itteration, we recevied publish frame for MBAT0CAPREL*/
    int32_t mbat0caprel_value = 50;
    receivePublishFrameFromBrokerMimic(&myFrame, MBAT0CAPREL, &mbat0caprel_value, sizeof(mbat0caprel_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    // Check if the rule has sent re-subscription frame for mbat0caprel since we have re-activated the rule engine
    EXPECT_EQ(getNumSentDDMP2Frames(), 1) << "We should have sent one DDMP2 frame" << std::endl;
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.source_connector, connector_unittest.connector_id);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE);
    EXPECT_EQ(myFrame.frame.subscribe.parameter, MBAT0CAPREL)
        << "frame: 0x" << std::hex << myFrame.frame.subscribe.parameter << " vs expected: 0x" << std::hex << MBAT0CAPREL;

    // Now check the status of the variables and their values
    EXPECT_EQ(getDDM2ParameterValue(mbat0caprel_item), mbat0caprel_value);  // 50
    EXPECT_EQ(expr_var_value(g_mbat0caprel_rule0), mbat0caprel_value);      // 50
    EXPECT_EQ(expr_var_value(g_mbat0caprel_rule1), mbat0caprel_value);      // 50
    EXPECT_EQ(expr_var_value(g_determine_caprel_rule0), 1);
    EXPECT_EQ(expr_var_value(g_determine_caprel_rule1), 1);
    EXPECT_TRUE(verifyVariableValueThroughRules("mbat0caprel", mbat0caprel_value));    // 50
    EXPECT_TRUE(verifyVariableValueThroughRules("g_mbat0caprel", mbat0caprel_value));  // 50
    EXPECT_TRUE(verifyVariableValueThroughRules("g_determine_caprel", 1));

    clearSentDDMP2Frames();

    /* Second iterration, we recevied new publish frame dor MBAT0CAPREL.
     * Should trigger publish of AC0ON and SYSAPPL0SMARTECO
     */
    mbat0caprel_value = 5;
    receivePublishFrameFromBrokerMimic(&myFrame, MBAT0CAPREL, &mbat0caprel_value, sizeof(mbat0caprel_value));
    rule_engine_task_process(&rule_engine_inst, &myFrame);

    EXPECT_EQ(getNumSentDDMP2Frames(), 2) << "We should have sent 2 DDMP2 frames" << std::endl;
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SET);
    EXPECT_EQ(myFrame.frame.set.parameter, SYSAPPL0SMARTECO)
        << "frame: 0x" << std::hex << myFrame.frame.set.parameter << " vs expected: 0x" << std::hex << SYSAPPL0SMARTECO;

    EXPECT_EQ(getDDM2ParameterValue(mbat0caprel_item), mbat0caprel_value);  // 5
    EXPECT_EQ(expr_var_value(ac0on_rule1), 1);
    EXPECT_EQ(expr_var_value(sysappl0smarteco_rule1), 0);
    EXPECT_EQ(expr_var_value(g_mbat0caprel_rule1), mbat0caprel_value);
    EXPECT_EQ(expr_var_value(g_mbat0caprel_rule0), mbat0caprel_value);  // 5
    EXPECT_EQ(expr_var_value(g_mbat0caprel_rule1), mbat0caprel_value);  // 5
    EXPECT_EQ(expr_var_value(g_determine_caprel_rule0), 1);
    EXPECT_EQ(expr_var_value(g_determine_caprel_rule1), 1);
    EXPECT_TRUE(verifyVariableValueThroughRules("ac0on", 1));
    EXPECT_TRUE(verifyVariableValueThroughRules("mbat0caprel", mbat0caprel_value));    // 5
    EXPECT_TRUE(verifyVariableValueThroughRules("g_mbat0caprel", mbat0caprel_value));  // 5
    EXPECT_TRUE(verifyVariableValueThroughRules("g_determine_caprel", 1));

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    vPortResumeScheduler();
}

/***
 * @brief Tests the inverse trigger logic behavior of the rule engine evaluator.
 *
 * This test verifies that rules can be triggered in a cascading manner through inverse logic:
 * 1. Sets up three rules with dependencies
 * 2. Triggers the third rule by publishing AC0ON=1
 * 3. Verifies that the cascading rule execution happens in the expected order
 * 4. Confirms proper parameter values are published to the broker
 */
TEST_F(RuleEngineEvaluatorTestFixture, VerifyInverseTriggerLogic1)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    // Rule that publishes to DDM2 parameters
    const char *rule0_spec = "g_first_rule: pub(ac0on=2)";
    const char *rule1_spec = "g_second_rule, trigger = 1: g_first_rule = trigger, pub(ac0ttemp=20)";
    const char *rule2_spec = "ac0on, trigger = 1: g_second_rule = trigger";

    static const struct rule_engine__rule rules[] = {
        {.name = "LocalRule1", .rule = const_cast<char *>(rule0_spec), .size = static_cast<int>(strlen(rule0_spec))},
        {.name = "LocalRule2", .rule = const_cast<char *>(rule1_spec), .size = static_cast<int>(strlen(rule1_spec))},
        {.name = "LocalRule3", .rule = const_cast<char *>(rule2_spec), .size = static_cast<int>(strlen(rule2_spec))},
    };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[1]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[2]), 1);

    // Remove subscription frames
    clearSentDDMP2Frames();

    // Activate rule engine
    // NOTE: This test simulates immediate activation with subsequent frame processing in the same iteration.
    // The next rule_engine_task_process() call will: activate the engine, send resubscriptions, and process frames.
    // In real-world scenario, RULE0ACT activation (via external connectors or GW0INV logic) and subsequent publish
    // frames would be processed across separate iterations, not bundled together as in this test scenario.
    rule_engine_activate(&rule_engine_inst, 1);

    // Test cascading rule triggers:
    // 1. The third rule will be triggered by AC0ON
    // 2. This will set g_second_rule = 1
    // 3. Since g_second_rule is set, rule1 should also execute
    // 4. That sets AC0TTEMP = 20 and g_first_rule will then trigger rule0
    // 5. resulting in AC0ON = 2
    int32_t ac0on = 1;
    receivePublishFrameFromBrokerMimic(&myFrame, AC0ON, &ac0on, sizeof(ac0on));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(getNumSentDDMP2Frames(), 3);  // Should have SET 2 frames and SUBSCRIBE 1 frames
    // Read the subscription frame in order to clear them
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    // Verify SET frame for ac0ttemp and ac0on
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SET);
    EXPECT_EQ(myFrame.frame.set.parameter, AC0ON);
    EXPECT_EQ(myFrame.frame.set.value.int32, 2);
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SET);
    EXPECT_EQ(myFrame.frame.set.parameter, AC0TTEMP);
    EXPECT_EQ(myFrame.frame.set.value.int32, 20);

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    vPortResumeScheduler();
}

/***
 * @brief Tests the inverse trigger logic behavior of the rule engine evaluator.
 *
 * This test verifies that rules can be triggered in a cascading manner through inverse logic:
 * 1. Sets up three rules with dependencies
 * 2. Triggers the third rule by publishing AC0ON=1
 * 3. Verifies that the cascading rule execution happens in the expected order
 * 4. Confirms proper parameter values are published to the broker
 */
TEST_F(RuleEngineEvaluatorTestFixture, VerifyInverseTriggerLogic2)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    // Rule that publishes to DDM2 parameters
    const char *rule0_spec = "g_first_rule, trigger = 1: g_second_rule = trigger, pub(ac0on=2)";
    const char *rule1_spec = "g_second_rule: pub(ac0ttemp=20)";
    const char *rule2_spec = "ac0on, trigger = 1: g_first_rule = trigger";

    static const struct rule_engine__rule rules[] = {
        {.name = "LocalRule1", .rule = const_cast<char *>(rule0_spec), .size = static_cast<int>(strlen(rule0_spec))},
        {.name = "LocalRule2", .rule = const_cast<char *>(rule1_spec), .size = static_cast<int>(strlen(rule1_spec))},
        {.name = "LocalRule3", .rule = const_cast<char *>(rule2_spec), .size = static_cast<int>(strlen(rule2_spec))},
    };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[1]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[2]), 1);

    // Remove subscription frames
    clearSentDDMP2Frames();

    // Activate rule engine
    // NOTE: This test simulates immediate activation with subsequent frame processing in the same iteration.
    // The next rule_engine_task_process() call will: activate the engine, send resubscriptions, and process frames.
    // In real-world scenario, RULE0ACT activation (via external connectors or GW0INV logic) and subsequent publish
    // frames would be processed across separate iterations, not bundled together as in this test scenario.
    rule_engine_activate(&rule_engine_inst, 1);

    // Test cascading rule triggers:
    // 1. The third rule will be triggered by AC0ON
    // 2. This will set g_first_rule = 1
    // 3. Since g_first_rule is set, rule0 should also execute
    // 4. That sets AC0TTEMP = 20 and g_first_rule will then trigger rule0
    // 5. resulting in AC0ON = 2
    int32_t ac0on = 1;
    receivePublishFrameFromBrokerMimic(&myFrame, AC0ON, &ac0on, sizeof(ac0on));
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(getNumSentDDMP2Frames(), 3);  // Should have SET 2 frames and SUBSCRIBE 1 frames
    // Read the subscription frame in order to clear them
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    // Verify SET frame for ac0ttemp and ac0on
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SET);
    EXPECT_EQ(myFrame.frame.set.parameter, AC0ON);
    EXPECT_EQ(myFrame.frame.set.value.int32, 2);
    EXPECT_EQ(getNextSentDDMP2Frame(&myFrame, &frame_size), 0);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SET);
    EXPECT_EQ(myFrame.frame.set.parameter, AC0TTEMP);
    EXPECT_EQ(myFrame.frame.set.value.int32, 20);

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    vPortResumeScheduler();
}

TEST_F(RuleEngineEvaluatorTestFixture, VerifyDDMGetData)
{
    DDMP2_FRAME myFrame;

    // Rule that publishes to DDM2 parameters
    const char *rule0_str = "ac0status: value = ddm_get_data(ac0status, 2, 2)";
    const char *rule1_str = "ac0status, i = 2, size = 2: value = ddm_get_data(ac0status, i, size)";
    const char *rule2_str = "ac0status, index = 1, offset = 2: value = ddm_get_data(ac0status, offset * index, size = 2)";

    static const struct rule_engine__rule rules[] = {
        {.name = "LocalRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
        {.name = "LocalRule1", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
        {.name = "LocalRule2", .rule = const_cast<char *>(rule2_str), .size = static_cast<int>(strlen(rule2_str))},
    };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[1]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[2]), 1);

    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    ASSERT_NE(rule0_spec, nullptr) << "Failed to retrieve rule specification";
    struct rule_engine__specification *rule1_spec = getSpecificationById(1);
    ASSERT_NE(rule0_spec, nullptr) << "Failed to retrieve rule specification";
    struct rule_engine__specification *rule2_spec = getSpecificationById(2);
    ASSERT_NE(rule2_spec, nullptr) << "Failed to retrieve rule specification";

    // Check DDM2 parameter setup
    ddmw_item_t *ac0status_item = getDDM2ParameterByID(AC0STATUS);
    EXPECT_NE(ac0status_item, nullptr);
    EXPECT_EQ(getDDM2ParameterValue(ac0status_item), 0);
    EXPECT_EQ(getDDM2ParameterType(ac0status_item), DDMW_ACTION_SET);
    // Check rule variables
    struct expr_var *ac0status_var = getVariableByName(rule0_spec, "ac0status");
    ASSERT_NE(ac0status_var, nullptr);
    EXPECT_EQ(expr_var_type(ac0status_var), RULE_ENGINE__VAR_TYPE_DDM2);
    // Check rule variables
    struct expr_var *value_var_rule0 = getVariableByName(rule0_spec, "value");
    ASSERT_NE(value_var_rule0, nullptr);
    EXPECT_EQ(expr_var_type(value_var_rule0), RULE_ENGINE__VAR_TYPE_LOCAL);
    // Check rule variables
    struct expr_var *value_var_rule1 = getVariableByName(rule1_spec, "value");
    ASSERT_NE(value_var_rule1, nullptr);
    EXPECT_EQ(expr_var_type(value_var_rule1), RULE_ENGINE__VAR_TYPE_LOCAL);
    // Check rule variables
    struct expr_var *value_var_rule2 = getVariableByName(rule2_spec, "value");
    ASSERT_NE(value_var_rule2, nullptr);
    EXPECT_EQ(expr_var_type(value_var_rule2), RULE_ENGINE__VAR_TYPE_LOCAL);

    // Remove subscription frames
    clearSentDDMP2Frames();

    // Activate rule engine
    // NOTE: This test simulates immediate activation with subsequent frame processing in the same iteration.
    // The next rule_engine_task_process() call will: activate the engine, send resubscriptions, and process frames.
    // In real-world scenario, RULE0ACT activation (via external connectors or GW0INV logic) and subsequent publish
    // frames would be processed across separate iterations, not bundled together as in this test scenario.
    rule_engine_activate(&rule_engine_inst, 1);

    size_t ac0status_elements = 2;
    size_t size_of_ac0status = sizeof(ERROR_T) + ac0status_elements * sizeof(uint16_t);
    ERROR_T *ac0status = (ERROR_T *)hal_mem_malloc(size_of_ac0status, HAL_MEM_INTERNAL_RAM);
    ASSERT_NE(ac0status, nullptr) << "Failed to allocate memory for ac0status";
    ac0status->error[0] = GENERIC_COMMUNICATION_ERROR;                        // Example error code
    ac0status->error[1] = AIRC_EVAPORATOR_TEMP_SENSOR_HEAT_PROTECTION_ERROR;  // Second error
    receivePublishFrameFromBrokerMimic(&myFrame, AC0STATUS, ac0status, size_of_ac0status);
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(expr_var_value(value_var_rule0), AIRC_EVAPORATOR_TEMP_SENSOR_HEAT_PROTECTION_ERROR);  // Should get the second error code
    EXPECT_EQ(expr_var_value(value_var_rule1), AIRC_EVAPORATOR_TEMP_SENSOR_HEAT_PROTECTION_ERROR);  // Should get the second error code
    EXPECT_EQ(expr_var_value(value_var_rule2), AIRC_EVAPORATOR_TEMP_SENSOR_HEAT_PROTECTION_ERROR);  // Should get the second error code

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    ac0status_elements = 5;
    size_of_ac0status = sizeof(ERROR_T) + ac0status_elements * sizeof(uint16_t);
    ac0status = (ERROR_T *)hal_mem_realloc_prefer(ac0status, size_of_ac0status, HAL_MEM_INTERNAL_RAM, HAL_MEM_INTERNAL_RAM);
    ASSERT_NE(ac0status, nullptr) << "Failed to reallocate memory for ac0status";
    // swap error codes to test ddm_get_data with same offsets
    ac0status->error[0] = AIRC_EVAPORATOR_TEMP_SENSOR_HEAT_PROTECTION_ERROR;  // Example error code
    ac0status->error[1] = GENERIC_COMMUNICATION_ERROR;                        // Second error
    receivePublishFrameFromBrokerMimic(&myFrame, AC0STATUS, ac0status, size_of_ac0status);
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(expr_var_value(value_var_rule0), GENERIC_COMMUNICATION_ERROR);  // Should get the second error code
    EXPECT_EQ(expr_var_value(value_var_rule1), GENERIC_COMMUNICATION_ERROR);  // Should get the second error code
    EXPECT_EQ(expr_var_value(value_var_rule2), GENERIC_COMMUNICATION_ERROR);  // Should get the second error code

    hal_mem_free(ac0status);

    vPortResumeScheduler();
}

TEST_F(RuleEngineEvaluatorTestFixture, DDMGetArrayLength)
{
    DDMP2_FRAME myFrame;

    // Rules for testing ddm_get_array_length with different array types
    const char *rule0_str = "mccc0csettemp: csettemp_len = ddm_get_array_length(mccc0csettemp, 0, 4)";
    const char *rule1_str = "mccc0ctemprng, i = 0: ctemprng_len = ddm_get_array_length(mccc0ctemprng, i, stride = 8)";
    const char *rule2_str = "hmi0event: hmi_event_len = ddm_get_array_length(hmi0event, 0 + 4, 1)";

    static const struct rule_engine__rule rules[] = {
        {.name = "ArrayLenRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
        {.name = "ArrayLenRule1", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
        {.name = "ArrayLenRule2", .rule = const_cast<char *>(rule2_str), .size = static_cast<int>(strlen(rule2_str))},
    };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Add all rules
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[1]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[2]), 1);

    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    ASSERT_NE(rule0_spec, nullptr) << "Failed to retrieve rule specification";
    struct rule_engine__specification *rule1_spec = getSpecificationById(1);
    ASSERT_NE(rule1_spec, nullptr) << "Failed to retrieve rule specification";
    struct rule_engine__specification *rule2_spec = getSpecificationById(2);
    ASSERT_NE(rule2_spec, nullptr) << "Failed to retrieve rule specification";

    // Get variables for checking values
    struct expr_var *csettemp_len = getVariableByName(rule0_spec, "csettemp_len");
    ASSERT_NE(csettemp_len, nullptr);
    EXPECT_EQ(expr_var_type(csettemp_len), RULE_ENGINE__VAR_TYPE_LOCAL);

    struct expr_var *ctemprng_len = getVariableByName(rule1_spec, "ctemprng_len");
    ASSERT_NE(ctemprng_len, nullptr);
    EXPECT_EQ(expr_var_type(ctemprng_len), RULE_ENGINE__VAR_TYPE_LOCAL);

    struct expr_var *hmi_event_len = getVariableByName(rule2_spec, "hmi_event_len");
    ASSERT_NE(hmi_event_len, nullptr);
    EXPECT_EQ(expr_var_type(hmi_event_len), RULE_ENGINE__VAR_TYPE_LOCAL);

    // Activate rule engine
    rule_engine_activate(&rule_engine_inst, 1);

    // Clear subscription frames
    clearSentDDMP2Frames();

    // Test TEMPARR_T array
    size_t csettemp_elements = 3;
    size_t size_of_csettemp = sizeof(TEMPARR_T) + (csettemp_elements * sizeof(int32_t));
    TEMPARR_T *mccc0csettemp = (TEMPARR_T *)hal_mem_malloc(size_of_csettemp, HAL_MEM_INTERNAL_RAM);
    ASSERT_NE(mccc0csettemp, nullptr) << "Failed to allocate memory for mccc0csettemp";
    receivePublishFrameFromBrokerMimic(&myFrame, MCCC0CSETTEMP, mccc0csettemp, size_of_csettemp);
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(expr_var_value(csettemp_len), csettemp_elements);  // Should have 3 elements

    csettemp_elements = 5;
    size_of_csettemp = sizeof(TEMPARR_T) + (csettemp_elements * sizeof(int32_t));
    mccc0csettemp = (TEMPARR_T *)hal_mem_realloc_prefer(mccc0csettemp, size_of_csettemp, HAL_MEM_INTERNAL_RAM, HAL_MEM_INTERNAL_RAM);
    receivePublishFrameFromBrokerMimic(&myFrame, MCCC0CSETTEMP, mccc0csettemp, size_of_csettemp);
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(expr_var_value(csettemp_len), csettemp_elements);  // Should have 5 elements

    // Test TEMPRANGEARR_T array
    size_t ctemprng_elements = 4;
    size_t size_of_ctemprng = sizeof(TEMPRANGEARR_T) + (ctemprng_elements * (2 * sizeof(int32_t)));
    TEMPRANGEARR_T *mccc0ctemprng = (TEMPRANGEARR_T *)hal_mem_malloc(size_of_ctemprng, HAL_MEM_INTERNAL_RAM);
    ASSERT_NE(mccc0ctemprng, nullptr) << "Failed to allocate memory for mccc0ctemprng";
    receivePublishFrameFromBrokerMimic(&myFrame, MCCC0CTEMPRNG, mccc0ctemprng, size_of_ctemprng);
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(expr_var_value(ctemprng_len), ctemprng_elements);  // Should have 4 elements

    // Test HMI0EVENT array
    size_t hmi_event_elements = 6;
    size_t size_of_hmi_event = sizeof(HMI0EVENT_T) + (hmi_event_elements * sizeof(uint8_t));
    HMI0EVENT_T *hmi0event = (HMI0EVENT_T *)hal_mem_malloc(size_of_hmi_event, HAL_MEM_INTERNAL_RAM);
    ASSERT_NE(hmi0event, nullptr) << "Failed to allocate memory for hmi0event";
    receivePublishFrameFromBrokerMimic(&myFrame, HMI0EVENT, hmi0event, size_of_hmi_event);
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(expr_var_value(hmi_event_len), hmi_event_elements);  // Should have 6 elements

    // Cleanup
    hal_mem_free(mccc0csettemp);
    hal_mem_free(mccc0ctemprng);
    hal_mem_free(hmi0event);

    vPortResumeScheduler();
}

TEST_F(RuleEngineEvaluatorTestFixture, DDMSetData)
{
    DDMP2_FRAME myFrame;

    // Rules for testing ddm_get_data and ddm_set_data
    const char *rule0_str = "mccc0errst: error_val = ddm_set_data(mccc0errst, offset = 0, 2, value = 524)";
    const char *rule1_str = "prod0prop, inst_offset=2: prop_val = ddm_set_data(prod0prop, inst_offset, size=1, 2)";

    static const struct rule_engine__rule rules[] = {
        {.name = "GetSetRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
        {.name = "GetSetRule1", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
    };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Add all rules
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[1]), 1);

    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    ASSERT_NE(rule0_spec, nullptr) << "Failed to retrieve rule specification";
    struct rule_engine__specification *rule1_spec = getSpecificationById(1);
    ASSERT_NE(rule1_spec, nullptr) << "Failed to retrieve rule specification";

    // Get DDM2 parameter setup
    ddmw_item_t *mccc0errst_item = getDDM2ParameterByID(MCCC0ERRST);
    EXPECT_NE(mccc0errst_item, nullptr);
    EXPECT_EQ(getDDM2ParameterValue(mccc0errst_item), 0);
    EXPECT_EQ(getDDM2ParameterType(mccc0errst_item), DDMW_ACTION_SET);
    ddmw_item_t *prod0prop_item = getDDM2ParameterByID(PROD0PROP);
    EXPECT_NE(prod0prop_item, nullptr);
    EXPECT_EQ(getDDM2ParameterValue(prod0prop_item), 0);
    EXPECT_EQ(getDDM2ParameterType(prod0prop_item), DDMW_ACTION_SET);
    // Get variables for checking values
    struct expr_var *g_error_val_var = getVariableByName(rule0_spec, "error_val");
    ASSERT_NE(g_error_val_var, nullptr);
    EXPECT_EQ(expr_var_type(g_error_val_var), RULE_ENGINE__VAR_TYPE_LOCAL);
    struct expr_var *g_prop_val_var = getVariableByName(rule1_spec, "prop_val");
    ASSERT_NE(g_prop_val_var, nullptr);
    EXPECT_EQ(expr_var_type(g_prop_val_var), RULE_ENGINE__VAR_TYPE_LOCAL);

    // Activate rule engine
    rule_engine_activate(&rule_engine_inst, 1);

    // Clear subscription frames
    clearSentDDMP2Frames();

    // Test ERROR_T set data with mccc0errst
    size_t mccc0errst_elements = 3;
    size_t size_of_mccc0errst = sizeof(ERROR_T) + mccc0errst_elements * sizeof(uint16_t);
    ERROR_T *mccc0errst = (ERROR_T *)hal_mem_malloc(size_of_mccc0errst, HAL_MEM_INTERNAL_RAM);
    ASSERT_NE(mccc0errst, nullptr) << "Failed to allocate memory for mccc0errst";
    mccc0errst->error[0] = GENERIC_COMMUNICATION_ERROR;
    mccc0errst->error[1] = AIRC_EVAPORATOR_TEMP_SENSOR_HEAT_PROTECTION_ERROR;
    mccc0errst->error[2] = GENERIC_NO_ERRORS;
    receivePublishFrameFromBrokerMimic(&myFrame, MCCC0ERRST, mccc0errst, size_of_mccc0errst);
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(expr_var_value(g_error_val_var), 1);                                                 // Should get validation that it has override the value
    ERROR_T *mccc0errst_item_data = (ERROR_T *)mccc0errst_item->data;                              // Get the data from the item
    EXPECT_EQ(mccc0errst_item_data->error[0], MC_CONTROLLER_OVER_TEMPERATURE_ERROR);               // Should set the first error code to 524
    EXPECT_EQ(mccc0errst_item_data->error[1], AIRC_EVAPORATOR_TEMP_SENSOR_HEAT_PROTECTION_ERROR);  // Should keep the second error code
    EXPECT_EQ(mccc0errst_item_data->error[2], GENERIC_NO_ERRORS);                                  // Should keep the third error code

    // Test PROD0PROP_T set data
    size_t prop_elements = 2;
    size_t size_of_prop = sizeof(PROD0PROP_T) + prop_elements * sizeof(uint32_t);
    PROD0PROP_T *prop = (PROD0PROP_T *)hal_mem_malloc(size_of_prop, HAL_MEM_INTERNAL_RAM);
    ASSERT_NE(prop, nullptr) << "Failed to allocate memory for prod0prop";
    prop->inst = 0;           // Instance index
    prop->classes[0] = 0x42;  // Example property value
    prop->classes[1] = 0x99;  // Another classes value
    receivePublishFrameFromBrokerMimic(&myFrame, PROD0PROP, prop, size_of_prop);
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(expr_var_value(g_prop_val_var), 1);                            // Should get validation that it has override the value
    PROD0PROP_T *prod0prop_item_data = (PROD0PROP_T *)prod0prop_item->data;  // Get the data from the item
    EXPECT_EQ(prod0prop_item_data->inst, 2);                                 // Should set the instance index to 2

    // Cleanup
    hal_mem_free(mccc0errst);
    hal_mem_free(prop);

    vPortResumeScheduler();
}

TEST_F(RuleEngineEvaluatorTestFixture, DDMFindValue)
{
    DDMP2_FRAME myFrame;

    // Rules for testing ddm_find_array_value
    const char *rule0_str = "ac0status: err_idx = ddm_find_array_value(ac0status, 0, 2, 2, 270, 0)";
    const char *rule1_str = "mccc0ctemprng: min = ddm_find_array_value(mccc0ctemprng, 0, 8, 4, 30, 0)";
    const char *rule2_str = "mccc0ctemprng: max = ddm_find_array_value(mccc0ctemprng, 4, 8, 4, 40, 0)";

    static const struct rule_engine__rule rules[] = {
        {.name = "FindValRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
        {.name = "FindValRule1", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
        {.name = "FindValRule2", .rule = const_cast<char *>(rule2_str), .size = static_cast<int>(strlen(rule2_str))},
    };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Add all rules
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[1]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[2]), 1);

    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    ASSERT_NE(rule0_spec, nullptr) << "Failed to retrieve rule specification";
    struct rule_engine__specification *rule1_spec = getSpecificationById(1);
    ASSERT_NE(rule1_spec, nullptr) << "Failed to retrieve rule specification";
    struct rule_engine__specification *rule2_spec = getSpecificationById(2);
    ASSERT_NE(rule2_spec, nullptr) << "Failed to retrieve rule specification";

    // Get variables for checking values
    struct expr_var *err_idx_var = getVariableByName(rule0_spec, "err_idx");
    ASSERT_NE(err_idx_var, nullptr);
    EXPECT_EQ(expr_var_type(err_idx_var), RULE_ENGINE__VAR_TYPE_LOCAL);
    struct expr_var *min_var = getVariableByName(rule1_spec, "min");
    ASSERT_NE(min_var, nullptr);
    EXPECT_EQ(expr_var_type(min_var), RULE_ENGINE__VAR_TYPE_LOCAL);
    struct expr_var *max_var = getVariableByName(rule2_spec, "max");
    ASSERT_NE(max_var, nullptr);
    EXPECT_EQ(expr_var_type(max_var), RULE_ENGINE__VAR_TYPE_LOCAL);

    // Activate rule engine
    rule_engine_activate(&rule_engine_inst, 1);

    // Clear subscription frames
    clearSentDDMP2Frames();

    // Test ERROR_T find value
    size_t ac0status_elements = 5;
    size_t size_of_ac0status = sizeof(ERROR_T) + ac0status_elements * sizeof(uint16_t);
    ERROR_T *ac0status = (ERROR_T *)hal_mem_malloc(size_of_ac0status, HAL_MEM_INTERNAL_RAM);
    ASSERT_NE(ac0status, nullptr) << "Failed to allocate memory for ac0status";
    ac0status->error[0] = GENERIC_COMMUNICATION_ERROR;
    ac0status->error[1] = AIRC_EVAPORATOR_TEMP_SENSOR_HEAT_PROTECTION_ERROR;
    ac0status->error[2] = AIRC_COMPRESSOR_DISCHARGE_TEMP_SENSOR_ERROR;
    ac0status->error[3] = AIRC_CONDENSER_FAN_ERROR;
    ac0status->error[4] = AIRC_COMPRESSOR_DRV_IPM_ERROR;
    receivePublishFrameFromBrokerMimic(&myFrame, AC0STATUS, ac0status, size_of_ac0status);
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(expr_var_value(err_idx_var), 3);  // Should find at index 3

    size_t mccc0ctemprng_elements = 2;
    size_t size_of_mccc0ctemprng = sizeof(TEMPRANGEARR_T) + mccc0ctemprng_elements * (2 * sizeof(int32_t));
    TEMPRANGEARR_T *mccc0ctemprng = (TEMPRANGEARR_T *)hal_mem_malloc(size_of_mccc0ctemprng, HAL_MEM_INTERNAL_RAM);
    ASSERT_NE(mccc0ctemprng, nullptr) << "Failed to allocate memory for mccc0ctemprng";
    mccc0ctemprng->temp_range[0].mintemp = 10;
    mccc0ctemprng->temp_range[0].maxtemp = 20;
    mccc0ctemprng->temp_range[1].mintemp = 30;
    mccc0ctemprng->temp_range[1].maxtemp = 40;
    receivePublishFrameFromBrokerMimic(&myFrame, MCCC0CTEMPRNG, mccc0ctemprng, size_of_mccc0ctemprng);
    rule_engine_task_process(&rule_engine_inst, &myFrame);
    EXPECT_EQ(expr_var_value(min_var), 1);  // Should find at index 1
    EXPECT_EQ(expr_var_value(max_var), 1);  // Should find at index 1

    // Cleanup
    hal_mem_free(ac0status);
    hal_mem_free(mccc0ctemprng);

    vPortResumeScheduler();
}

TEST_F(RuleEngineEvaluatorTestFixture, DDMOwnerGetSetValue)
{
    DDMP2_FRAME myFrame;
    size_t frame_size;

    // Rules for testing ddm_find_array_value
    const char *rule0_str = "ac0on=ddm(5), ac0avl=ddm(1), ac0status=ddm, mask=['or']: data = ddm_get_data(ac0status, 4, 2), g_counter = g_counter + 1";
    const char *rule1_str = "g_counter, g_counter = 0: if (g_counter == 2, ddm_set_data(ac0status, 4, 2, 256)), if (g_counter == 3, pub(ac0status), print(g_counter))";

    static const struct rule_engine__rule rules[] = {
        {.name = "SetGetValRule0", .rule = const_cast<char *>(rule0_str), .size = static_cast<int>(strlen(rule0_str))},
        {.name = "SetGetValRule1", .rule = const_cast<char *>(rule1_str), .size = static_cast<int>(strlen(rule1_str))},
    };

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Add all rules
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[0]), 1);
    ASSERT_EQ(rule_engine__add_specification(&rule_engine_inst, &rules[1]), 1);

    struct rule_engine__specification *rule0_spec = getSpecificationById(0);
    ASSERT_NE(rule0_spec, nullptr) << "Failed to retrieve rule specification";
    struct rule_engine__specification *rule1_spec = getSpecificationById(1);
    ASSERT_NE(rule1_spec, nullptr) << "Failed to retrieve rule specification";

    // Get DDM2 parameter setup(we are the owners)
    ddmw_item_t *ac0status_item = getDDM2ParameterByID(AC0STATUS);
    ASSERT_NE(ac0status_item, nullptr);
    EXPECT_EQ(getDDM2ParameterValue(ac0status_item), 0);
    EXPECT_EQ(getDDM2ParameterType(ac0status_item), DDMW_ACTION_PUBLISH);
    // Get variables for checking values
    struct expr_var *data_var = getVariableByName(rule0_spec, "data");
    ASSERT_NE(data_var, nullptr);
    EXPECT_EQ(expr_var_type(data_var), RULE_ENGINE__VAR_TYPE_LOCAL);

    // Activate rule engine
    rule_engine_activate(&rule_engine_inst, 1);

    // Clear subscription frames
    clearSentDDMP2Frames();

    receiveSubFrameFromBrokerMimic(&myFrame, AC0STATUS);    // DDMW depends on subscribe frame in order to generate publish frame)
    rule_engine_task_process(&rule_engine_inst, &myFrame);  // process the frame
    receiveSubFrameFromBrokerMimic(&myFrame, AC0ON);        // DDMW depends on subscribe frame in order to generate publish frame)
    rule_engine_task_process(&rule_engine_inst, &myFrame);  // process the frame
    EXPECT_EQ(getNumSentDDMP2Frames(), 3);                  // Should have PUBLISH 3 frames, for AC0AVL, AC0STATUS and AC0ON
    // Clear publish frames
    clearSentDDMP2Frames();

    // Test ERROR_T find value
    size_t ac0status_elements = 5;
    size_t size_of_ac0status = sizeof(ERROR_T) + ac0status_elements * sizeof(uint16_t);
    ERROR_T *ac0status_send_buffer = (ERROR_T *)hal_mem_malloc(size_of_ac0status, HAL_MEM_INTERNAL_RAM);
    ASSERT_NE(ac0status_send_buffer, nullptr) << "Failed to allocate memory for ac0status_send_buffer";
    ac0status_send_buffer->error[0] = GENERIC_COMMUNICATION_ERROR;
    ac0status_send_buffer->error[1] = AIRC_EVAPORATOR_TEMP_SENSOR_HEAT_PROTECTION_ERROR;
    ac0status_send_buffer->error[2] = AIRC_COMPRESSOR_DISCHARGE_TEMP_SENSOR_ERROR;
    ac0status_send_buffer->error[3] = AIRC_CONDENSER_FAN_ERROR;
    ac0status_send_buffer->error[4] = AIRC_COMPRESSOR_DRV_IPM_ERROR;

    ERROR_T *ac0status_item_data = (ERROR_T *)ac0status_item->data;  // Get the data from the item
    EXPECT_EQ(ac0status_item_data->error[2], 0);                     // Initially should be 0

    /* Receiving SET data for AC0STATUS, ddmw by default will trigger
     * PUBLISH frame(since AC0STATUS is updated)
     */
    receiveSetFrameFromBrokerMimic(&myFrame, AC0STATUS, ac0status_send_buffer, size_of_ac0status);  // Simulate a SET frame from broker(we are the owners)
    rule_engine_task_process(&rule_engine_inst, &myFrame);                                          // process the frame
    EXPECT_EQ(getNumSentDDMP2Frames(), 1);                                                          // Should have PUBLISH 1 frame, for AC0STATUS
    getNextSentDDMP2Frame(&myFrame, &frame_size);                                                   // Get the next sent frame
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_PUBLISH);
    EXPECT_EQ(myFrame.frame.publish.parameter, AC0STATUS);
    EXPECT_EQ(ac0status_send_buffer->error[2], ((uint16_t *)myFrame.frame.publish.value.raw)[2]);  // Should get the third error code (AIRC_COMPRESSOR_DISCHARGE_TEMP_SENSOR_ERROR)
    EXPECT_EQ(ac0status_send_buffer->error[2], ac0status_item_data->error[2]);                     // Should be the same as third error code (AIRC_COMPRESSOR_DISCHARGE_TEMP_SENSOR_ERROR)
    EXPECT_EQ(ac0status_send_buffer->error[2], expr_var_value(data_var));                          // Should get the third error code (AIRC_COMPRESSOR_DISCHARGE_TEMP_SENSOR_ERROR)

    // Clear sent frames
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    /* Second SET of the same frame so we can trigger the second rule(g_counter == 2).
     * PUBLISH will not be triggered as the value of AC0STATUS is not changed by the SET frame and also
     * ddm_set_data() should not PUBLISH by default
     */
    receiveSetFrameFromBrokerMimic(&myFrame, AC0STATUS, ac0status_send_buffer, size_of_ac0status);  // Simulate a SET frame from broker(we are the owners)
    rule_engine_task_process(&rule_engine_inst, &myFrame);                                          // process the frame
    EXPECT_EQ(getNumSentDDMP2Frames(), 0);                                                          // Should have PUBLISH 0 frames, as the AC0STATUS value is not changed
    EXPECT_EQ(ac0status_item_data->error[2], AIRC_ROOM_TEMP_SENSOR_ERROR);                          // But the ddm_set_data() should have changed the value to AIRC_ROOM_TEMP_SENSOR_ERROR

    /* Second SET of the same frame so we can trigger the second rule(g_counter == 3).
     * PUBLISH will be triggered as the value of AC0STATUS is changed by the ddm_set_data() call
     * in the previous(g_counter == 2) iteration.
     */
    int ac0on = 5;  // Set AC0ON to 5. Since it is the same value as before, it should not trigger a PUBLISH
    ERROR_T *ac0status_receive_buffer = (ERROR_T *)(myFrame.frame.publish.value.raw);
    receiveSetFrameFromBrokerMimic(&myFrame, AC0ON, &ac0on, sizeof(ac0on));  // Simulate a SET frame from broker(we are the owners)
    rule_engine_task_process(&rule_engine_inst, &myFrame);                   // process the frame
    EXPECT_EQ(getNumSentDDMP2Frames(), 1);                                   // Should have PUBLISH 1 frames, as ==3 wants to manually publish the frame even though the value is not changed
    getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_PUBLISH);
    EXPECT_EQ(myFrame.frame.publish.parameter, AC0STATUS);
    EXPECT_EQ(ac0status_item_data->error[2], AIRC_ROOM_TEMP_SENSOR_ERROR);         // Same as in the previous iteration
    EXPECT_EQ(ac0status_item_data->error[2], ac0status_receive_buffer->error[2]);  // Same as in the previous iteration
    EXPECT_EQ(ac0status_receive_buffer->error[0], GENERIC_COMMUNICATION_ERROR);
    EXPECT_EQ(ac0status_receive_buffer->error[1], AIRC_EVAPORATOR_TEMP_SENSOR_HEAT_PROTECTION_ERROR);
    EXPECT_EQ(ac0status_receive_buffer->error[2], AIRC_ROOM_TEMP_SENSOR_ERROR);
    EXPECT_EQ(ac0status_receive_buffer->error[3], AIRC_CONDENSER_FAN_ERROR);
    EXPECT_EQ(ac0status_receive_buffer->error[4], AIRC_COMPRESSOR_DRV_IPM_ERROR);

    // Cleanup
    hal_mem_free(ac0status_send_buffer);
    vPortResumeScheduler();
}
