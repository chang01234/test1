/*
 * DICMFrameworkTestFixture.hpp
 *
 *  Created on: 14 nov. 2023
 *      Author: Andlun
 */

#ifndef DICMFRAMEWORKTESTFIXTURE_HPP_
#define DICMFRAMEWORKTESTFIXTURE_HPP_
extern "C" {
#if !defined(DISABLE_WRAP_FUNCTIONS)
#include "configuration.h"
#include "connector.h"
#include "ddm2.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#endif
#include "DICMFramework.h"
}
#include "gtest/gtest.h"
#include <stddef.h>

using ::testing::Test;

class DICMFrameworkTestFixture : public Test
{
  public:
#if !defined(DISABLE_WRAP_FUNCTIONS)
    void incSentDDMP2Frames()
    {
        ++numSentFrames;
    }
    void incReceivedDDMP2Frames()
    {
        ++numReceivedFrames;
    }
    void setConnectorId(uint8_t *id)
    {
        if (*id == 0)
        {
            // Init phase
            *id = 0xff;
        }
        this->connector_id = id;
    }
    uint8_t getConnectorId()
    {
        return *this->connector_id;
    }
    RingbufHandle_t getFromRingBufHandle()
    {
        return this->from_connector;
    }
    RingbufHandle_t getToRingBufHandle()
    {
        return this->to_connector;
    }
    DICMFrameworkTestFixture() : Test()
    {
        mInstance = this;
        this->from_ringbuffer_storage = malloc(TO_CONNECTOR_RINGBUFFER_SIZE);
        this->from_connector = xRingbufferCreateStatic(TO_CONNECTOR_RINGBUFFER_SIZE, RINGBUF_TYPE_NOSPLIT, (uint8_t *)this->from_ringbuffer_storage, &this->from_ringbuffer);
        this->to_ringbuffer_storage = malloc(TO_CONNECTOR_RINGBUFFER_SIZE);
        this->to_connector = xRingbufferCreateStatic(TO_CONNECTOR_RINGBUFFER_SIZE, RINGBUF_TYPE_NOSPLIT, (uint8_t *)this->to_ringbuffer_storage, &this->to_ringbuffer);
        numSentFrames = 0;
        numReceivedFrames = 0;
    }
    static DICMFrameworkTestFixture *getInstance()
    {
        return mInstance;
    }
#endif
    static int app_main(int argc, const char **argv)
    {
        printf("Running app_main() from %s\n", __FILE__);
        testing::InitGoogleTest(&argc, (char **)argv);
        int testresult = RUN_ALL_TESTS();
        return testresult;
    }

  protected:
#if !defined(DISABLE_WRAP_FUNCTIONS)
    uint8_t *connector_id = NULL;  //!< \~ Connector ID,                                   provided by broker
    void *from_ringbuffer_storage;
    RingbufHandle_t from_connector;      //!< \~ Copy outgoing traffic  to broker in this ringbuffer
    StaticRingbuffer_t from_ringbuffer;  //!< \~ Ringbuffer data structure
    int numSentFrames;
    void *to_ringbuffer_storage;
    RingbufHandle_t to_connector;      //!< \~ Copy outgoing traffic  to broker in this ringbuffer
    StaticRingbuffer_t to_ringbuffer;  //!< \~ Ringbuffer data structure
    int numReceivedFrames;
#endif
    void SetupFramework()
    {
        DICMFramework_start();
    }
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
#if !defined(DISABLE_WRAP_FUNCTIONS)
    static inline DICMFrameworkTestFixture *mInstance = NULL;

    int getNumReceivedDDMP2Frames()
    {
        return numReceivedFrames;
    }
    void clearReceivedDDMP2Frames()
    {
        DDMP2_FRAME *pFrame;
        size_t fSize = 0;
        do
        {
            pFrame = (DDMP2_FRAME *)xRingbufferReceive(this->to_connector, &fSize, 0);
            if (pFrame)
            {
                vRingbufferReturnItem(this->to_connector, (void *)pFrame);
            }
        } while (pFrame);
        numReceivedFrames = 0;
    }

    int getNumSentDDMP2Frames()
    {
        return numSentFrames;
    }
    void clearSentDDMP2Frames()
    {
        DDMP2_FRAME *pFrame;
        size_t fSize = 0;
        do
        {
            pFrame = (DDMP2_FRAME *)xRingbufferReceive(this->from_connector, &fSize, 0);
            if (pFrame)
            {
                vRingbufferReturnItem(this->from_connector, (void *)pFrame);
            }
        } while (pFrame);
        numSentFrames = 0;
    }
    int getNextSentDDMP2Frame(DDMP2_FRAME *outFrame, size_t *frame_size)
    {
        DDMP2_FRAME *l_frame;
        size_t fSize = 0;

        if (outFrame == NULL)
        {
            return -1;
        }
        l_frame = (DDMP2_FRAME *)xRingbufferReceive(this->from_connector, &fSize, 0);
        if ((fSize != 0) && (l_frame != NULL))
        {
            memcpy(outFrame, l_frame, fSize);
            vRingbufferReturnItem(this->from_connector, (void *)l_frame);
            numSentFrames--;
            if (frame_size != NULL)
            {
                *frame_size = fSize;
            }
            return 0;
        }
        return -1;
    }
    int getNextReceivedDDMP2Frame(DDMP2_FRAME *outFrame, size_t *frame_size)
    {
        DDMP2_FRAME *l_frame;
        size_t fSize = 0;

        if (outFrame == NULL)
        {
            return -1;
        }
        l_frame = (DDMP2_FRAME *)xRingbufferReceive(this->to_connector, &fSize, 0);
        if ((fSize != 0) && (l_frame != NULL))
        {
            memcpy(outFrame, l_frame, fSize);
            vRingbufferReturnItem(this->to_connector, (void *)l_frame);
            numReceivedFrames--;
            if (frame_size != NULL)
            {
                *frame_size = fSize;
            }
            return 0;
        }
        return -1;
    }
#endif
};
// static DICMFrameworkTestFixture::mInstance = NULL;

extern "C" int app_main(int main_argc, const char **main_argv)
{
    return DICMFrameworkTestFixture::app_main(main_argc, main_argv);
}

#if !defined(DISABLE_WRAP_FUNCTIONS)

extern "C" int __real_connector_forward_frame_to_connector(const DDMP2_FRAME *pframe, const int destination_connector);
extern "C" int __wrap_connector_forward_frame_to_connector(const DDMP2_FRAME *pframe, const int destination_connector)
{
    assert(pframe);
    // Copy to my local ringbuffer
    DICMFrameworkTestFixture *myObj = DICMFrameworkTestFixture::getInstance();
    if ((uint8_t)destination_connector == myObj->getConnectorId())
    {
        TRUE_CHECK(xRingbufferSend(myObj->getToRingBufHandle(), pframe, pframe->frame_size + DDMP2_METADATA_SIZE, 0));
        myObj->incReceivedDDMP2Frames();
    }
    return __real_connector_forward_frame_to_connector(pframe, destination_connector);
}
extern "C" int __real_connector_send_frame_to_connector(const DDMP2_CONTROL_ENUM control, const uint32_t parameter, const void *value, const uint8_t value_size, const uint8_t connector, const TickType_t timeout);
extern "C" int __wrap_connector_send_frame_to_connector(const DDMP2_CONTROL_ENUM control, const uint32_t parameter, const void *value, const uint8_t value_size, const uint8_t connector, const TickType_t timeout)
{
    // Copy to my local ringbuffer
    DICMFrameworkTestFixture *myObj = DICMFrameworkTestFixture::getInstance();

    if (connector == myObj->getConnectorId())
    {
        DDMP2_FRAME frame;
        int create_frame_result;

        switch (control)
        {
        case DDMP2_CONTROL_PUBLISH:
            TRUE_CHECK(create_frame_result = ddmp2_create_publish(&frame, parameter, value, value_size, connector));
            break;
        case DDMP2_CONTROL_SET:
            TRUE_CHECK(create_frame_result = ddmp2_create_set(&frame, parameter, value, value_size, connector));
            break;
        case DDMP2_CONTROL_SUBSCRIBE:
            TRUE_CHECK(create_frame_result = ddmp2_create_subscribe(&frame, parameter, connector));
            break;
        case DDMP2_CONTROL_UNSUBSCRIBE:
            TRUE_CHECK(create_frame_result = ddmp2_create_unsubscribe(&frame, parameter, connector));
            break;
        case DDMP2_CONTROL_NOP:
            TRUE_CHECK(create_frame_result = ddmp2_create_nop(&frame, connector));
            break;
        case DDMP2_CONTROL_REG:
            TRUE_CHECK(create_frame_result = ddmp2_create_reg(&frame, parameter, connector));
            break;
        case DDMP2_CONTROL_MESSAGE:
            TRUE_CHECK(create_frame_result = ddmp2_create_message(&frame, (uint8_t)parameter, connector));
            break;
        case DDMP2_CONTROL_MULTIBROKER:
            if (value_size != sizeof(uint64_t))
            {
                LOG(E, "Bad value size: %d", value_size);
                return 0;
            }
            TRUE_CHECK(create_frame_result = ddmp2_create_multibroker(&frame, parameter, *((uint64_t *)value), connector));
            break;
        case DDMP2_CONTROL_GENERIC:
            TRUE_CHECK(create_frame_result = ddmp2_create_generic(&frame, parameter, value, value_size, connector));
            break;
        default:
            LOG(E, "Bad control byte: 0x%x", control);
            return 0;
        }
        if (!create_frame_result)
        {
            return 0;
        }
        BaseType_t send_result;
        TRUE_CHECK(send_result = xRingbufferSend(myObj->getToRingBufHandle(), &frame, frame.frame_size + DDMP2_METADATA_SIZE, timeout));
        myObj->incReceivedDDMP2Frames();
    }
    return __real_connector_send_frame_to_connector(control, parameter, value, value_size, connector, timeout);
}

extern "C" int __real_connector_forward_frame_to_broker(const DDMP2_FRAME *const pframe);
extern "C" int __wrap_connector_forward_frame_to_broker(const DDMP2_FRAME *const pframe)
{
    assert(pframe);
    uint8_t source_connector = pframe->source_connector;
    // Copy to my local ringbuffer
    DICMFrameworkTestFixture *myObj = DICMFrameworkTestFixture::getInstance();

    if (source_connector == myObj->getConnectorId())
    {
        TRUE_CHECK(xRingbufferSend(myObj->getFromRingBufHandle(), pframe, pframe->frame_size + DDMP2_METADATA_SIZE, 0));
        myObj->incSentDDMP2Frames();
    }
    return __real_connector_forward_frame_to_broker(pframe);
}

extern "C" int __real_connector_send_frame_to_broker(const DDMP2_CONTROL_ENUM control, const uint32_t parameter, const void *value, const uint8_t value_size, uint8_t source_connector, TickType_t timeout);
extern "C" int __wrap_connector_send_frame_to_broker(const DDMP2_CONTROL_ENUM control, const uint32_t parameter, const void *value, const uint8_t value_size, uint8_t source_connector, TickType_t timeout)
{
    // Copy to my local ringbuffer
    DICMFrameworkTestFixture *myObj = DICMFrameworkTestFixture::getInstance();
    if (source_connector == myObj->getConnectorId())
    {
        size_t frame_size;
        DDMP2_FRAME frame_to_send;

        if (value_size)
        {
            TRUE_CHECK_RETURN0(value != NULL);
        }

        TRUE_CHECK_RETURN0(frame_size = ddmp2_full_frame_size(control, value_size));
        frame_to_send.source_connector = source_connector;
        frame_to_send.frame.control = control;
        frame_to_send.frame_size = ddmp2_raw_frame_size(control, value_size);

        switch (control)
        {
        case DDMP2_CONTROL_PUBLISH:
        {
            frame_to_send.frame.publish.parameter = parameter;
            if (value_size)
            {
                memcpy(frame_to_send.frame.publish.value.raw, value, value_size);
            }
            break;
        }
        case DDMP2_CONTROL_SET:
        {
            frame_to_send.frame.set.parameter = parameter;
            if (value_size)
            {
                memcpy(frame_to_send.frame.set.value.raw, value, value_size);
            }
            break;
        }
        case DDMP2_CONTROL_SUBSCRIBE:
        {
            frame_to_send.frame.subscribe.parameter = parameter;
            break;
        }
        case DDMP2_CONTROL_NOP:
        {
            break;
        }
        case DDMP2_CONTROL_REG:
        {
            frame_to_send.frame.reg.device_class = parameter;
            break;
        }
        case DDMP2_CONTROL_MESSAGE:
        {
            frame_to_send.frame.message.id = (uint8_t)parameter;
            break;
        }
        case DDMP2_CONTROL_MULTIBROKER:
        {
            TRUE_CHECK_RETURN0(value_size == sizeof(uint64_t));
            frame_to_send.frame.multibroker.data = *((uint64_t *)value);
            break;
        }
        case DDMP2_CONTROL_GENERIC:
        {
            LOG(E, "Generic control type is not supported by the broker");
            return 0;
        }
        case DDMP2_CONTROL_UNSUBSCRIBE:
        {
            frame_to_send.frame.unsubscribe.parameter = parameter;
            break;
        }
        default:
        {
            LOG(E, "Bad control byte: 0x%x", control);
            return 0;
        }
        }

        TRUE_CHECK(xRingbufferSend(myObj->getFromRingBufHandle(), &frame_to_send, frame_size, timeout));
        myObj->incSentDDMP2Frames();
    }
    return __real_connector_send_frame_to_broker(control, parameter, value, value_size, source_connector, timeout);
}
#endif
#endif /* DICMFRAMEWORKTESTFIXTURE_HPP_ */
