
#include "hybrid.h"

void hybrid_lock_init(hybrid_lock_t * lock)
{
    pthread_mutex_init(&(lock->_m), NULL);
    pthread_spin_init(&(lock->_s), 0);
}

void hybrid_lock_destroy(hybrid_lock_t * lock)
{
    pthread_mutex_destroy(&(lock->_m));
    pthread_spin_destroy(&(lock->_s));
}

void hybrid_lock_lock(hybrid_lock_t * lock)
{
    struct timeval begin, end;
    double try_cost;
    unsigned long long try_count = 100;

    gettimeofday(&begin, NULL);
    while(try_count--) {
        if(pthread_spin_trylock(&(lock->_s)) == 0) {
            if(pthread_mutex_trylock(&(lock->_m)) != 0) {
                pthread_spin_unlock(&(lock->_s));
                continue;
            }
            return;
        }
    }
    gettimeofday(&end, NULL);

    try_cost = (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec) / 1000000.0f;
    try_count = (unsigned long long)(100.0f / try_cost);

#ifdef PPRINT
    printf("try_cost: %.10f\n", try_cost);
    printf("try_count: %d\n", try_count);
#endif

    while(try_count--) {
        if(pthread_spin_trylock(&(lock->_s)) == 0) {
            if(pthread_mutex_trylock(&(lock->_m)) != 0) {
                pthread_spin_unlock(&(lock->_s));
                continue;
            }
            return;
        }
    }

#ifdef PPRINT
    printf("begin to mutex lock\n");
#endif

    pthread_mutex_lock(&(lock->_m));
    pthread_spin_lock(&(lock->_s));
}

void hybrid_lock_unlock(hybrid_lock_t * lock)
{
    pthread_mutex_unlock(&(lock->_m));
    pthread_spin_unlock(&(lock->_s));
}

