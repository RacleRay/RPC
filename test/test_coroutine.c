#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "coroutine/coroutine.h"


struct args {
    int param;
};


// tasks to run in coroutines
static void task(struct schedule* s, void *ud) {
    struct args* arg = (struct args*)ud;
    int a = arg->param;
    for (int i = 0; i < 5; i++) {
        // run and yeild
        printf("coroutine %d: %d - %d\n", coroutine_running(s), a, i);
        coroutine_yield(s);
    }
}


// schedule the whole coroutine running process
static void test(struct schedule *s) {
    struct args a = {1};
    struct args b = {2};
    struct args c = {3};
    struct args d = {4};
    struct args e = {5};

    // create coroutines
    int co1 = coroutine_new(s, task, &a);
    int co2 = coroutine_new(s, task, &b);
    int co3 = coroutine_new(s, task, &c);
    int co4 = coroutine_new(s, task, &d);
    int co5 = coroutine_new(s, task, &e);

    printf("test start\n");

    // run coroutines
    // while 
    while (coroutine_status(s, co1) && coroutine_status(s, co2) 
           && coroutine_status(s, co3) && coroutine_status(s, co4)
           && coroutine_status(s, co5)){
        coroutine_resume(s, co1);
        coroutine_resume(s, co2);
        coroutine_resume(s, co3);
        coroutine_resume(s, co4);
        coroutine_resume(s, co5);
    }
    printf("test end\n");
}


int main() {
    struct schedule *sched = coroutine_open();

    // run coroutines created
    test(sched);

    // sleep(1);

    coroutine_close(sched);

    return 0;
}