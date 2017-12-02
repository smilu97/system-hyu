
// #define PPRINT

#include <pthread.h>
#include <sys/time.h>
#include <stdio.h>

typedef struct {
    pthread_mutex_t _m;
    pthread_spinlock_t _s;
} hybrid_lock_t;

void hybrid_lock_init(hybrid_lock_t * lock);
void hybrid_lock_destroy(hybrid_lock_t * lock);
void hybrid_lock_lock(hybrid_lock_t * lock);
void hybrid_lock_unlock(hybrid_lock_t * lock);

