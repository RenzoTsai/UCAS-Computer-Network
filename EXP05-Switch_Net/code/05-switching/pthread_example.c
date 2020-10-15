#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

pthread_mutex_t output_lock;

struct thread_arg {
	int index;
	int duration;
};

void *thread(void * _arg)
{
	struct thread_arg *arg = _arg;
	sleep(arg->duration);

	pthread_mutex_lock(&output_lock);
	fprintf(stdout, "Thread %d wakes up after %d seconds.\n", \
			arg->index, arg->duration);
	fflush(stdout);
	pthread_mutex_unlock(&output_lock);
}

int main()
{
	pthread_t t1, t2;

	struct thread_arg arg1, arg2;
	arg1.index = 1;
	arg1.duration = rand() % 5;
	arg2.index = 2;
	arg2.duration = rand() % 5;

	pthread_mutex_init(&output_lock, NULL);

	pthread_create(&t1, NULL, thread, &arg1);
	pthread_create(&t2, NULL, thread, &arg2);

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	return 0;
}
