#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#include <ucontext.h>

#include "coroutine/coroutine.h"

#define STACK_SIZE (1024 * 1024)
#define DEFAULT_COROUTINE 16


//====================================================
// struct

struct coroutine;

typedef struct schedule {
    char stack[STACK_SIZE];
    ucontext_t main;
    
    int n_coroutine;
    int capacity;
    int running;

    struct coroutine **coroutines;
} Schedule;

typedef struct coroutine {
    coroutine_func func;
    void *ud;

    ucontext_t ctx;
    struct schedule *sch;

    ptrdiff_t cap;
    ptrdiff_t size;
    
    int status;
    char* stack;
} Coroutine;

Coroutine *_co_new(Schedule *sched, coroutine_func func, void *ud) {
    Coroutine*co = (Coroutine*)malloc(sizeof(*co));
    assert(co);

    co->func = func;
    co->ud = ud;
    co->sch = sched;

    co->cap = 0;
    co->size = 0;
    co->status = COROUTINE_READY;
    co->stack = NULL;

    return co;
}

void _co_delete(Coroutine *co) {
    if (co->stack) {
        free(co->stack);
    }
    free(co);
}

//=====================================================
// inferfaces

Schedule* coroutine_open() {
    Schedule *s = (Schedule*)malloc(sizeof(*s));
    assert(s);

    s->n_coroutine = 0;
    s->capacity = DEFAULT_COROUTINE;
    s->running = -1;
    s->coroutines = (Coroutine**)malloc(sizeof(Coroutine*) * s->capacity);
    assert(s->coroutines);

    memset(s->coroutines, 0, sizeof(Coroutine*) * s->capacity);

    return s;
}


void coroutine_close(Schedule *s) {
    for (int i = 0; i < s->capacity; i++) {
        Coroutine *co = s->coroutines[i];
        if (co) {
            _co_delete(co);
        }
    }
    free(s->coroutines);
    s->coroutines = NULL;
    free(s);
}


int coroutine_new(Schedule* s, coroutine_func func, void *ud) {
    Coroutine* co = _co_new(s, func, ud);

    if (s->n_coroutine >= s->capacity) {
        int id = s->capacity;
        
        s->coroutines = (Coroutine**)realloc(s->coroutines, s->capacity * 2 * sizeof(Coroutine*));
        assert(s->coroutines);
        // deal with realloc fails

        memset(s->coroutines + s->capacity, 0, sizeof(Coroutine*) * s->capacity);
        s->coroutines[s->capacity] = co;
        s->capacity *= 2;
        ++s->n_coroutine;

        return id;
    } else {
        for (int i = 0; i < s->capacity; i++) {
            int id = (i + s->n_coroutine) % s->capacity;
            if (s->coroutines[id] == NULL) {
                s->coroutines[id] = co;
                ++s->n_coroutine;
                return id;
            }
        }
    }

    assert(0);
    return -1;
}


int coroutine_status(Schedule *s, int id) {
    assert(id >= 0 && id < s->capacity);
    
    if (s->coroutines[id] == NULL) {
        return COROUTINE_DEAD;
    }
    return s->coroutines[id]->status;
}


int coroutine_running(Schedule *s) {
    return s->running;
}


static void _save_stack(Coroutine *co, char *top) {
    char dummy = 0;
    // top : at previous higher address stack frame
    // dummy : at the beginning of current lower address stack frame
    // BUT !!！ there is something subtle maybe. Some bytes will be added to the stack.
    assert(top - &dummy <= STACK_SIZE);

    if (co->cap < top - &dummy) {
        if (co->stack) {
            free(co->stack);
        }

        co->cap = top - &dummy;
        co->stack = (char*)malloc(co->cap);
    }
    co->size = top - &dummy;
    // save stacks
    memcpy(co->stack, &dummy, co->size);
}


void coroutine_yield(Schedule *s) {
    int id = s->running;
    assert(id >= 0);

    Coroutine *co = s->coroutines[id];
    assert((char*)&co > s->stack);  // could back to previous stack frame

    _save_stack(co, s->stack + STACK_SIZE);
    co->status = COROUTINE_SUSPEND;
    s->running = -1;

    // Symmetric coroutine
    swapcontext(&co->ctx, &s->main);  // swap to s->main
}


static void main_sched(uint32_t low32, uint32_t hi32) {
    uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
    Schedule *s = (Schedule*)ptr;

    int id = s->running;
    Coroutine *co = s->coroutines[id];

    co->func(s, co->ud);
    _co_delete(co);

    s->coroutines[id] = NULL;
    --s->n_coroutine;
    s->running = -1;
}


void coroutine_resume(struct schedule *s, int id) {
    assert(s->running == -1);  // schedule for main process
    assert(id >= 0 && id < s->capacity);

    Coroutine* co = s->coroutines[id];
    if (co == NULL) {
        return;
    }

    int status = co->status;
    switch (status) {
        case COROUTINE_READY: {
            getcontext(&co->ctx);
            co->ctx.uc_stack.ss_sp = s->stack;
            co->ctx.uc_stack.ss_size = STACK_SIZE;
            co->ctx.uc_link = &s->main;    // Symmetric coroutine 
            
            s->running = id;
            co->status = COROUTINE_RUNNING;

            uintptr_t ptr = (uintptr_t)s;
            
            makecontext(&co->ctx, (void (*)(void))main_sched, 2, (uint32_t)ptr, (uint32_t)(ptr >> 32));
            swapcontext(&s->main, &co->ctx);   // swap to co->ctx
            break;
        }
        case COROUTINE_SUSPEND: {
            memcpy(s->stack + STACK_SIZE - co->size, co->stack, co->size);
            s->running = id;
            co->status = COROUTINE_RUNNING;
            swapcontext(&s->main, &co->ctx);
            break;
        }
        default:
            assert(0);
    } 
}
