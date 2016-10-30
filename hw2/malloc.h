#ifndef __MALLOC_H__
#define __MALLOC_H__


enum ArenaStatus {
	ARENA_USED, ARENA_AVAILABLE
};

enum ArenaType {
	ARENA_MAIN, ARENA_THREAD
};

typedef struct MallocHeader {
	size_t size;
} MallocHeader;

typedef struct {
	Arena next;
	enum ArenaStatus status;
	enum ArenaType type;
	int main_thread_arena;
	int no_of_threads;

} Arena;

typedef struct {

} Bin;

typedef struct {

} BlockHeader;

int initializeArenas();
int intitializeMainArena();
