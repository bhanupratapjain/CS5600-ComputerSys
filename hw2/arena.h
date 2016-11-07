//
// Created by bhanu on 05/11/2016.
//

#ifndef ARENA_H
#define ARENA_H

#include <pthread.h>

#define MAX_BINS 3

typedef struct arena_t {
    struct arena_t *next;
    int no_of_threads;
    struct bin_t *bins[MAX_BINS];
    pthread_mutex_t lock;
} arena_t;

/*
 * Thread Arenas
 */
extern int no_of_arenas;
extern struct arena_t *global_arena_ptr;
extern __thread struct arena_t *thread_arena_ptr;
extern pthread_mutex_t arena_init_lock;
extern int no_of_processors;

arena_t *find_available_arena();

void add_arena(struct arena_t *arena_ptr);

arena_t *initialize_arenas();

void fork_prepare();

void fork_parent();

void fork_child();

void acquire_locks();

void release_locks();

int check_addr(void *ptr);

#endif //ARENA_H
