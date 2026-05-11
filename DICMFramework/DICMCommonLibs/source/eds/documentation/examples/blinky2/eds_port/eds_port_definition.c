/*
 * eds_port_definition.c
 *
 *  Created on: Dec 14, 2021
 *      Author: nenad
 */

#define _POSIX_C_SOURCE 200809L

#include "eds_port.h"

#include <semaphore.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>

static pthread_mutex_t s__critical_mutex;

#define ALIGN_UP(num, align)                                       \
        (((num) + (align) - 1u) & ~((align) - 1u))

static void critical_mutex_init(void)
{
    pthread_mutexattr_t mutextattr;
    int error;

    error = pthread_mutexattr_init(&mutextattr);
    assert(error == 0);
    error = pthread_mutexattr_settype(&mutextattr, PTHREAD_MUTEX_RECURSIVE);
    assert(error == 0);
    error = pthread_mutex_init(&s__critical_mutex, NULL);
    assert(error == 0);
}

void
eds_port__sleep_init(struct eds_port__sleep *sleep)
{
    int error;

    error = sem_init(&sleep->semaphore, 0, 0);
    assert(error == 0);
}

void
eds_port__sleep_wait(struct eds_port__sleep *sleep)
{
    int error;

    do {
        error = sem_wait(&sleep->semaphore);
    } while ((error == -1) && (errno == EINTR));
    assert((error == 0) || (errno == EINTR));
}


void
eds_port__sleep_signal(struct eds_port__sleep *sleep)
{
    int error;

    error = sem_post(&sleep->semaphore);
    assert(error == 0);
}

void
eds_port__critical_lock(struct eds_port__critical *critical)
{
    int error;

    (void)critical;
    error = pthread_mutex_lock(&s__critical_mutex);
    assert(error == 0);
}

void
eds_port__critical_unlock(struct eds_port__critical *critical)
{
    int error;

    (void)critical;
    error = pthread_mutex_unlock(&s__critical_mutex);
    assert(error == 0);
}

uint_fast8_t
eds_port__ffs(uint32_t value)
{
    return (uint_fast8_t)(31u - (unsigned)__builtin_clz(value));
}

size_t
eds_port__align_up(size_t non_aligned_value)
{
    return ALIGN_UP(non_aligned_value, sizeof(void *));
}

uint32_t
eds_port__tick_duration_ms(void)
{
    return 10u;
}

uint32_t
eds_port__tick_from_ms(uint32_t ms)
{
    return ms / 10;
}

void
eds_port__init(void)
{
    critical_mutex_init();
}
