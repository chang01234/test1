/*
 * blinky.c
 *
 *  Created on: Dec 14, 2021
 *      Author: nenad
 */

#if __STDC_VERSION__ >= 199901L
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>           /* Definition of SIGEV_* constants */
#include <sys/time.h>

#include "eds.h"

struct tick_context
{
    struct sigaction sigaction;
    pthread_t timer_thread;
    sem_t timer_lock;
};

static struct tick_context tick;

#define EVENT_TICK      1000

struct sm__workspace
{
    eds__etimer *tick;
    uint32_t period_ms;
    uint32_t instance;
};

static void*
std_alloc(void *context, size_t size)
{
    (void) context;

    return malloc(size);
}

static void
std_free(void *context, void *memory)
{
    (void) context;

    free(memory);
}

static eds__sm_action
sm__blink_off(eds__sm *sm, void *workspace, const eds__event *event);
static eds__sm_action
sm__blink_on(eds__sm *sm, void *workspace, const eds__event *event);
static eds__sm_action
sm__blink_init(eds__sm *sm, void *workspace, const eds__event *event);

static eds__sm_action
sm__blink_off(eds__sm *sm, void *workspace, const eds__event *event)
{
    struct sm__workspace *ws = workspace;

    switch (eds__event_id(event)) {
    case EDS__SM_EVENT__INIT:
        printf("SM%u: blink_off: init\n", ws->instance);
        return eds__sm_event_handled(sm);
    case EVENT_TICK:
        printf("SM%u: blink_off: event_tick\n", ws->instance);
        return eds__sm_transit_to(sm, sm__blink_on);
    default:
        return eds__sm_super_state(sm, &eds__sm_top_state);
    }
}

static eds__sm_action
sm__blink_on(eds__sm *sm, void *workspace, const eds__event *event)
{
    struct sm__workspace *ws = workspace;

    switch (eds__event_id(event)) {
    case EDS__SM_EVENT__INIT:
        printf("SM%u: blink_on: init\n", ws->instance);
        return eds__sm_event_handled(sm);
    case EVENT_TICK:
        printf("SM%u: blink_on: event_tick\n", ws->instance);
        return eds__sm_transit_to(sm, sm__blink_off);
    default:
        return eds__sm_super_state(sm, &eds__sm_top_state);
    }
}

static eds__sm_action
sm__blink_init(eds__sm *sm, void *workspace, const eds__event *event)
{
    struct sm__workspace *ws = workspace;
    eds__error error;
    eds__event *n_event;

    switch (eds__event_id(event)) {
    case EDS__SM_EVENT__INIT:
        printf("SM%u: init: init\n", ws->instance);
        error = eds__etimer_create(NULL, &ws->tick);
        assert(error == EDS__ERROR_NONE);
        error = eds__event_create(EVENT_TICK, 0, &n_event);
        assert(error == EDS__ERROR_NONE);
        error = eds__etimer_send_every(ws->tick, eds__agent_from_sm(sm), n_event, ws->period_ms);
        assert(error == EDS__ERROR_NONE);
        return eds__sm_transit_to(sm, sm__blink_on);
    default:
        return eds__sm_event_ignored(sm);
    }
}

static void
timer_handler(int sig_no)
{
    int error;

    (void) sig_no;
    error = sem_post(&tick.timer_lock);
    assert(error == 0);
}

#include <errno.h>
#include <string.h>

static void*
tick_thread(void *arg)
{
    (void) arg;

    for (;;) {
        int sem_error;
        eds__error error;

        do {
            sem_error = sem_wait(&tick.timer_lock);
        } while ((sem_error == -1) && (errno == EINTR));
        assert(sem_error == 0);
        error = eds__tick_process_all();
        assert(error == EDS__ERROR_NONE);
    }
    return NULL;
}

static void
tick_setup(void)
{
    int error;
    struct itimerval timer;

    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 10000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 10000;
    tick.sigaction.sa_handler = &timer_handler;

    error = sem_init(&tick.timer_lock, 0, 0);
    assert(error == 0);
    error = pthread_create(&tick.timer_thread, NULL, tick_thread, NULL);
    assert(error == 0);
    error = sigaction(SIGALRM, &tick.sigaction, NULL);
    assert(error == 0);
    error = setitimer(ITIMER_REAL, &timer, NULL);
    assert(error == 0);
}

int
main(void)
{
    eds__error error;
    eds__agent *sm_blink_agent1;
    eds__agent *sm_blink_agent2;
    eds__network *network;
    struct sm__workspace sm__blink_workspace1 =
        {
            .period_ms = 1000,
            .instance = 0
        };
    struct sm__workspace sm__blink_workspace2 =
        {
            .period_ms =500,
            .instance = 1
        };

    tick_setup();

    error = eds__mem_add_allocator(std_alloc, std_free, NULL, SIZE_MAX);
    assert(error == EDS__ERROR_NONE);

    error = eds__agent_create(sm__blink_init, &sm__blink_workspace1, NULL, &sm_blink_agent1);
    assert(error == EDS__ERROR_NONE);

    error = eds__agent_create(sm__blink_init, &sm__blink_workspace2, NULL, &sm_blink_agent2);
    assert(error == EDS__ERROR_NONE);

    error = eds__network_create(NULL, &network);
    assert(error == EDS__ERROR_NONE);

    error = eds__network_add_agent(network, sm_blink_agent1);
    assert(error == EDS__ERROR_NONE);

    error = eds__network_add_agent(network, sm_blink_agent2);
    assert(error == EDS__ERROR_NONE);

    error = eds__network_start(network);
    assert(error == EDS__ERROR_NONE);

    return 0;
}
