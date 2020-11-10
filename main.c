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

#define MTX_A 0
#define MTX_B 1
#define MTX_C 2
#define MUTEX_CNT 3

typedef struct Context{
    pthread_mutex_t **mtxs;
    int return_code;
} Context;

pthread_mutex_t mtx_a, mtx_b, mtx_c;

void exitWithFailure(const char *msg, int err){
    errno = err;
    fprintf(stderr, "%.256s : %.256s", msg, strerror(errno));
    exit(EXIT_FAILURE);
}

void assertSuccess(const char *msg, int errcode){
    if (errcode != SUCCESS_CODE)
        exitWithFailure(msg, errcode);
}

void assertInThreadSuccess(int errcode, Context *cntx){
    if (errcode != SUCCESS_CODE){
        cntx->return_code = errcode;
        pthread_exit((void*)ERROR_CODE);
    }
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

void initMutexSuccessAssertion(
    pthread_mutex_t *mtx, 
    pthread_mutexattr_t *mtx_attr,
    const char *msg){
    if (mtx == NULL)
        return;

    int err = pthread_mutex_init(mtx, mtx_attr);
    assertSuccess(msg, err);
}

void init(const char *err_msg){
    pthread_mutexattr_t mtx_attr;
    int err = pthread_mutexattr_init(&mtx_attr);
    assertSuccess("init", err);
    err = pthread_mutexattr_settype(&mtx_attr, PTHREAD_MUTEX_ERRORCHECK);
    assertSuccess("init", err);
    initMutexSuccessAssertion(&mtx_a, &mtx_attr, err_msg);
    initMutexSuccessAssertion(&mtx_b, &mtx_attr, err_msg);
    initMutexSuccessAssertion(&mtx_c, &mtx_attr, err_msg);

    err = pthread_mutexattr_destroy(&mtx_attr);
    assertSuccess("init", err);
}

int initContext(
                Context *cntx, 
                pthread_mutex_t *mtx_a,
                pthread_mutex_t *mtx_b,
                pthread_mutex_t *mtx_c){
    
    if (cntx == NULL || mtx_a == NULL 
        || mtx_b == NULL || mtx_c == NULL)
        return EINVAL;

    errno = SUCCESS_CODE;
    cntx->mtxs = (pthread_mutex_t**)malloc(sizeof(pthread_mutex_t) * MUTEX_CNT);
    if (cntx->mtxs == NULL || errno != SUCCESS_CODE)
        exitWithFailure("init", ENOMEM);
    cntx->mtxs[0] = mtx_a;
    cntx->mtxs[1] = mtx_b;
    cntx->mtxs[2] = mtx_c;

    cntx->return_code = SUCCESS_CODE;

    return SUCCESS_CODE;
}

void destroyMutexSuccessAssertion(pthread_mutex_t *mtx, const char *msg){
    if (mtx == NULL)
        return;

    int err = pthread_mutex_destroy(mtx);
    assertSuccess(msg, err);
}

void releaseResources(Context *cntx, const char *msg){
    free(cntx->mtxs);
    destroyMutexSuccessAssertion(&mtx_a, msg);
    destroyMutexSuccessAssertion(&mtx_b, msg);
    destroyMutexSuccessAssertion(&mtx_c, msg);
}

void iteration(Context *cntx, const char *print_msg, 
                size_t start_mtx, const char *err_msg){
    
    lockSuccessAssertion(cntx->mtxs[start_mtx], err_msg);
    printf(print_msg);
    unlockSuccessAssertion(cntx->mtxs[(start_mtx + 2) % MUTEX_CNT], err_msg);
    lockSuccessAssertion(cntx->mtxs[(start_mtx + 1) % MUTEX_CNT], err_msg);
    unlockSuccessAssertion(cntx->mtxs[start_mtx], err_msg);
    lockSuccessAssertion(cntx->mtxs[(start_mtx + 2) % MUTEX_CNT], err_msg);
    unlockSuccessAssertion(cntx->mtxs[(start_mtx + 1) % MUTEX_CNT], err_msg);
}

void *routine(void *data){
    if (data == NULL)
        exitWithFailure("routine", EINVAL);
    Context *cntx = (Context*)data;
    if (cntx->mtxs == NULL)
         exitWithFailure("routine", EINVAL);

    lockSuccessAssertion(cntx->mtxs[MTX_B], "routine");

    sleep(1);
    for (int i = 0; i < PRINT_CNT; ++i)
        iteration(cntx, THREAD_MSG, MTX_C, "routine");

    unlockSuccessAssertion(&mtx_b, "routine");

    return SUCCESS_CODE;
}

int main(int argc, char **argv){
    pthread_t pid;

    init("init");

    lockSuccessAssertion(&mtx_c, "main:init");

    Context cntx;
    initContext(&cntx, &mtx_a, &mtx_b, &mtx_c);

    int err = pthread_create(&pid, NULL, routine, (void*)(&cntx));
    sleep(1);
    if (err != SUCCESS_CODE)
        exitWithFailure("main", err);

    for (int i = 0; i < PRINT_CNT; ++i)
        iteration(&cntx, MAIN_MSG, MTX_A, "main");

    unlockSuccessAssertion(&mtx_c, "main");
    err = pthread_join(pid, NULL);
    assertSuccess("main", err);

    releaseResources(&cntx, "main:releaseResources");

    pthread_exit((void*)(EXIT_SUCCESS));
}
