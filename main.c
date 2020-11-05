/* 
 * File:   din_phil.c
 * Author: nd159473 (Nickolay Dalmatov, Sun Microsystems)
 * adapted from http://developers.sun.com/sunstudio/downloads/ssx/tha/tha_using_deadlock.html
 *
 * Created on January 1, 1970, 9:53 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#define PHILO 5
#define DELAY 30000
#define FOOD 50

#define ERROR_CODE -1
#define SUCCESS_CODE 0

pthread_mutex_t forks[PHILO];
pthread_t phils[PHILO];
pthread_mutex_t foodlock;

int sleep_seconds = 0;

void exitWithFailure(const char *msg, int errcode){
    errno = errcode;
    fprintf(stderr, "%.256s:%.256s\n", msg, strerror(errno));
    exit(EXIT_FAILURE);
}

void assertSuccess(char *msg, int errcode){
    if (errcode != SUCCESS_CODE)
        exitWithFailure(msg, errcode);
}

void assertInThreadSuccess(int errcode){
    if (errcode != SUCCESS_CODE)
        pthread_exit((void*)errcode);
}

int food_on_table(int *res){
    static int food = FOOD;
    int myfood;

    int err = pthread_mutex_lock (&foodlock);
    if (err != SUCCESS_CODE)
        return err;

    if (food > 0)
        food--;

    myfood = food;
    err = pthread_mutex_unlock (&foodlock);
    if (err != SUCCESS_CODE)
        return err;
        
    *res = myfood;
    return SUCCESS_CODE;
}

int get_fork(
            int phil,
            int fork,
            char *hand){
    int err = pthread_mutex_lock (&forks[fork]);
    if (err != SUCCESS_CODE)
        return err;

    printf ("Philosopher %d: got %s fork %d\n", phil, hand, fork);
    return SUCCESS_CODE;
}

void down_forks(
            int f1,
            int f2){
    pthread_mutex_unlock (&forks[f1]);
    pthread_mutex_unlock (&forks[f2]);
}

void *philosopher (void *num){
    int id;
    int left_fork, right_fork, f;

    id = (int)num;
    printf ("Philosopher %d sitting down to dinner.\n", id);
    right_fork = id;
    left_fork = id + 1;

    /* Wrap around the forks. */
    if (left_fork == PHILO)
        left_fork = 0;

    int err = food_on_table(&f);
    assertInThreadSuccess(err);
    while (f != 0) {

        /* Thanks to philosophers #1 who would like to 
            * take a nap before picking up the forks, the other
            * philosophers may be able to eat their dishes and 
            * not deadlock.
            */
        if (id == 1)
            sleep (sleep_seconds);

        printf ("Philosopher %d: get dish %d.\n", id, f);
        if (id % 2 == 0){
            err = get_fork(id, right_fork, "right");
            assertInThreadSuccess(err);
            err = get_fork(id, left_fork, "left ");
            assertInThreadSuccess(err);
        }
        else{
            err = get_fork(id, left_fork, "left ");
            assertInThreadSuccess(err);
            err = get_fork(id, right_fork, "right");
            assertInThreadSuccess(err);
        }

        printf ("Philosopher %d: eating.\n", id);
        usleep (DELAY * (FOOD - f + 1));
        down_forks(left_fork, right_fork);

        err = food_on_table(&f);
        assertInThreadSuccess(err);
    }

    printf ("Philosopher %d is done eating.\n", id);
    return (NULL);
}

int main (int argn, char **argv){
    int i;

    if (argn == 2)
        sleep_seconds = atoi(argv[1]);
    if (sleep_seconds < 0)
        exitWithFailure("main", EINVAL);

    int err = pthread_mutex_init(&foodlock, NULL);
    assertSuccess("main: pthread_mutex_init", err);

    for (i = 0; i < PHILO; i++){
        err = pthread_mutex_init(&forks[i], NULL);
        assertSuccess("main: pthread_mutex_init", err);
    }

    for (i = 0; i < PHILO; i++){
        err = pthread_create(&phils[i], NULL, philosopher, (void *)i);
        assertSuccess("main: pthread_create", err);
    }

    for (i = 0; i < PHILO; i++){
        int ret_code = SUCCESS_CODE;
        printf("lala\n");
        err = pthread_join(phils[i], (void*)(&ret_code));
        assertSuccess("main: pthread_join", err);
        assertSuccess("philosopher", ret_code);
    }

    printf("Philosophers are done.\n");
    return 0;
}