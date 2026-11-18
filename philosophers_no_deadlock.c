#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// mutex array for the 5 forks
pthread_mutex_t forks[5];

// arg is a generic pointer
void *single_thread(void *arg) {
	// treat arg as a pointer to an integer and dereference
	int id = *(int *) arg;
	int left = id;
	int right = (id+1) % 5;

	// no deadlock-strategy
	int first = (left > right) ? left : right;
	int second = (left > right) ? right : left;

	// mutex_lock is an implicit blocking function, waiting for unlocked
	// resource if not available
	while (1) {
		printf("%d is thinking...\n", id);
		usleep(100000);
		pthread_mutex_lock(&forks[first]);
		printf("%d has fork %d.\n", id, first);
		usleep(100000);
		pthread_mutex_lock(&forks[second]);
		printf("%d has fork %d.\n", id, second);
		printf("%d is eating...\n", id);
		usleep(10000);
		pthread_mutex_unlock(&forks[first]);
		pthread_mutex_unlock(&forks[second]);	
		printf("%d is full and thinking again...\n", id);
	}
	return NULL;
}

int main() {
	pthread_t threads[5];
	int ids[5];
	// initialize mutex at runtime (dynamic initalization) w/out attributes
	for (int i = 0; i < 5; i++) {
		pthread_mutex_init(&forks[i], NULL);
	}
	// create threads w/ philosopher IDs as arguments
	for (int i = 0; i < 5; i++) {
		ids[i]= i;
		pthread_create(&threads[i], NULL, single_thread, &ids[i]);
	}
	// let main thread wait for philosopher threads to terminate
	// this causes the main thread to never terminate in the deadlock case
	for (int i = 0; i < 5; i++) {
		pthread_join(threads[i], NULL);
	}
	// for syntactic completeness, clean up mutex dependencies
	for (int i = 0; i < 5; i++) {
		pthread_mutex_destroy(&forks[i]);
	}
	return 0;
}
