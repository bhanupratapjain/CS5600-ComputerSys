//
// Created by bhanu on 04/11/2016.
//

#include <stdio.h>
#include "arena.h"
#include "bin.h"

int no_of_arenas = 0;
int no_of_processors = 1;
arena_t *global_arena_ptr = NULL;
__thread arena_t *thread_arena_ptr = NULL;
pthread_mutex_t arena_init_lock = PTHREAD_MUTEX_INITIALIZER;

arena_t *find_available_arena() {
    arena_t *arena_itr = global_arena_ptr;
    arena_t *arena_final_ptr = global_arena_ptr;
    while (arena_itr != NULL) {

        if (arena_itr->no_of_threads < arena_final_ptr->no_of_threads) {
            arena_final_ptr = arena_itr;
        }
        arena_itr = arena_itr->next;
    }
    return arena_final_ptr;
}

void add_arena(arena_t *arena_ptr) {
    if (global_arena_ptr == NULL) {
        global_arena_ptr = arena_ptr;
        return;
    }
    arena_t *arena_itr = global_arena_ptr;
    arena_t *p_arena_itr = NULL;
    while (arena_itr != NULL) {
        p_arena_itr = arena_itr;
        arena_itr = arena_itr->next;
    }
    p_arena_itr->next = arena_ptr;
}


arena_t *initialize_arenas() {
    pthread_mutex_lock(&arena_init_lock); //Arena Init Lock
    if (no_of_arenas == no_of_processors) {
        /*Don' Create New Arena*/
        /*Move Global Pointer to Available arena*/
        arena_t *arena_ptr = find_available_arena();
        arena_ptr->no_of_threads++;
        pthread_mutex_unlock(&arena_init_lock);
        return arena_ptr;
    }
    /*Create New Arena*/
    arena_t *thread_arena_ptr = (arena_t *) sbrk(sizeof(arena_t));
    thread_arena_ptr->next = NULL;
    thread_arena_ptr->no_of_threads = 1;

    /*Add arena to Global Arena Linked List*/
    add_arena(thread_arena_ptr);

    /*Initialzie BINS*/
    initialize_bins(thread_arena_ptr);

    /*Increase the no. of Arenas*/
    no_of_arenas += 1;

    pthread_mutex_unlock(&arena_init_lock);
    return thread_arena_ptr;
}

void fork_prepare() {
    acquire_locks();
}

void fork_parent() {
    release_locks();
}

void fork_child() {
    release_locks();
}

void acquire_locks() {
    pthread_mutex_lock(&arena_init_lock);
    arena_t *arena_ptr = global_arena_ptr;
    while (arena_ptr) {
        pthread_mutex_lock(&arena_ptr->lock);
        arena_ptr = arena_ptr->next;
    }
}

void release_locks() {
    arena_t *arena_ptr = global_arena_ptr;
    while (arena_ptr) {
        pthread_mutex_unlock(&arena_ptr->lock);
        arena_ptr = arena_ptr->next;
    }
    pthread_mutex_unlock(&arena_init_lock);
}