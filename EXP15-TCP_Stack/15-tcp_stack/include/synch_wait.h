#ifndef __SYNCH_WAIT_H__
#define __SYNCH_WAIT_H__

#include <pthread.h>
#include <stdlib.h>

struct synch_wait {
	pthread_mutex_t lock;		// mutex lock
	pthread_cond_t cond;		// condition variable to synch
	int notified;				// whether ready to read/write
	int dead;					// whether dead
	int sleep;					// whether others are waiting
};

// initialize all the variables
static inline void wait_init(struct synch_wait *wait)
{
	pthread_cond_init(&wait->cond, NULL);
	pthread_mutex_init(&wait->lock, NULL);
	wait->dead = 0;
	wait->notified = 0;
	wait->sleep = 0;
}

// exit and notify all others
static inline void wait_exit(struct synch_wait *wait)
{
	pthread_mutex_lock(&wait->lock);
	if (wait->dead)
		goto unlock;
	wait->dead = 1;
	if (wait->sleep)
		pthread_cond_broadcast(&wait->cond);

unlock:
	pthread_mutex_unlock(&wait->lock);
}

// sleep on waiting for notification
static inline int sleep_on(struct synch_wait *wait)
{
	pthread_mutex_lock(&wait->lock);
	if (wait->dead)
		goto unlock;
	wait->sleep = 1;
	if (!wait->notified)
		pthread_cond_wait(&wait->cond, &wait->lock);
	wait->notified = 0;
	wait->sleep = 0;
unlock:
	pthread_mutex_unlock(&wait->lock);

	return -(wait->dead);
}

// notify others
static inline int wake_up(struct synch_wait *wait)
{
	pthread_mutex_lock(&wait->lock);
	if (wait->dead)
		goto unlock;

	if (!wait->notified) {
		wait->notified = 1;
		if (wait->sleep)
			pthread_cond_signal(&wait->cond);
	}

unlock:
	pthread_mutex_unlock(&wait->lock);
	return -(wait->dead);
}

// allocate a wait struct 
static inline struct synch_wait *alloc_wait_struct()
{
	struct synch_wait *wait = malloc(sizeof(struct synch_wait));
	wait_init(wait);

	return wait;
}

// free the wait struct
static inline void free_wait_struct(struct synch_wait *wait)
{
	free(wait);
}

#endif
