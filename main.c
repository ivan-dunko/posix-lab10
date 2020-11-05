#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define ERROR_CODE -1
#define SUCCESS_CODE 0
#define PRINT_CNT 10
#define THREAD_MSG "routine\n"
#define MAIN_MSG "main\n"

#define lock pthread_mutex_lock
#define unlock pthread_mutex_unlock

pthread_mutex_t ma, mb, mc;

void exitWithFailure(const char *msg, int err){
    errno = err;
    fprintf(stderr, "%s256 : %s256", msg, strerror(errno));
    exit(EXIT_FAILURE);
}

void assertSuccess(const char *msg, int errcode){
    if (errcode != SUCCESS_CODE)
        exitWithFailure(msg, errcode);
}

void assertInThreadSuccess(int errcode){
    if (errcode != SUCCESS_CODE)
        pthread_exit((void*)errcode);
}

int printLine(size_t print_cnt, const char *str){
    for (size_t i = 0; i < print_cnt; ++i){
        int len = strlen(str);
        if (len == ERROR_CODE)
            return ERROR_CODE;
    
        int err = write(STDIN_FILENO, str, len);
        if (err == ERROR_CODE)
            return ERROR_CODE;
    }

    return SUCCESS_CODE;
}

void lockSuccessAssertion(pthread_mutex_t *mtx, const char *msg){
    if (mtx == NULL)
        return;

    int err = lock(mtx);
    assertSuccess(msg, err);
}

void unlockSuccessAssertion(pthread_mutex_t *mtx, const char *msg){
    if (mtx == NULL)
        return;

    int err = unlock(mtx);
    assertSuccess(msg, err);
}

void *routine(void *data){
    lockSuccessAssertion(&mb, "routine");

    sleep(1);
    for (int i = 0; i < PRINT_CNT; ++i){
        lockSuccessAssertion(&mc, "routine");
        printf(THREAD_MSG);
        unlockSuccessAssertion(&mb, "routine");
        lockSuccessAssertion(&ma, "routine");
        unlockSuccessAssertion(&mc, "routine");
        lockSuccessAssertion(&mb, "routine");
        unlockSuccessAssertion(&ma, "routine");
    }

    unlockSuccessAssertion(&mb, "routine");

    return SUCCESS_CODE;
}

void initMutexSuccessAssertion(
    pthread_mutex_t *mtx, 
    pthread_mutexattr_t *mtx_attr,
    const char *msg){
    if (mtx == NULL || mtx_attr)
        return;

    int err = pthread_mutex_init(&ma, NULL);
    assertSuccess(msg, err);
}

void init(const char *err_msg){
    initMutexSuccessAssertion(&ma, NULL, err_msg);
    initMutexSuccessAssertion(&mb, NULL, err_msg);
    initMutexSuccessAssertion(&mc, NULL, err_msg);
}

void destroyMutexSuccessAssertion(pthread_mutex_t *mtx, const char *msg){
    if (mtx == NULL)
        return;

    int err = pthread_mutex_destroy(mtx);
    assertSuccess(msg, err);
}

void releaseResources(const char *msg){
    destroyMutexSuccessAssertion(&ma, msg);
    destroyMutexSuccessAssertion(&mb, msg);
    destroyMutexSuccessAssertion(&mc, msg);
}

int main(int argc, char **argv){
    pthread_t pid;

    init("init");

    lockSuccessAssertion(&mc, "main:init");

    int err = pthread_create(&pid, NULL, routine, NULL);
    sleep(1);
    if (err != SUCCESS_CODE)
        exitWithFailure("main", err);

    for (int i = 0; i < PRINT_CNT; ++i){
        lockSuccessAssertion(&ma, "main");
        printf(MAIN_MSG);
        unlockSuccessAssertion(&mc, "main");
        lockSuccessAssertion(&mb, "main");
        unlockSuccessAssertion(&ma, "main");
        lockSuccessAssertion(&mc, "main");
        unlockSuccessAssertion(&mb, "main");
    }

    unlockSuccessAssertion(&mc, "main");
    err = pthread_join(pid, NULL);
    assertSuccess("main", err);

    releaseResources("main:releaseResources");

    pthread_exit((void*)(EXIT_SUCCESS));
}
