//
// Condition Variables using Futex
// Created by Bhanu Jain on 07/11/2016.
//

#include <pthread.h>
#include <linux/futex.h>
#include <sys/time.h>


typedef struct cond_t {
    atomic_int val;
} cond_t;

int cond_init(cond_t *cond_ptr);

int cond_wait(cond_t *cond_ptr, pthread_mutex_t *mutex_ptr);

int cond_signal(cond_t *cond_ptr);


int cond_init(cond_t *cond_ptr) {
    atomic_init(&cond_ptr->val, 0);
    return 0;
}

int cond_wait(cond_t *cond_ptr, pthread_mutex_t *mutex_ptr) {
    pthread_mutex_unlock(mutex_ptr);
    sys_futex(&cond_ptr->val, FUTEX_WAIT_PRIVATE, cond_ptr->val, NULL, NULL, 0);
    return 0;
}

int cond_signal(cond_t *cond_ptr) {
    atomic_fetch_add(&cond_ptr->val, 1);
    sys_futex(&cond_ptr->val, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
    return 0;
}
