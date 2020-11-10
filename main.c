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

typedef struct Context{
    pthread_mutex_t *mtx_a, *mtx_b, *mtx_c;
    int return_code;
} Context;

pthread_mutex_t mtx_a, mtx_b, mtx_c;

void exitWithFailure(const char *msg, int err){
    errno = err;
    fprintf(stderr, "%s256 : %s256", msg, strerror(errno));
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
    if (data == NULL)
        exitWithFailure("routine", EINVAL);
    Context *cntx = (Context*)data;
    if (cntx->mtx_a == NULL || cntx->mtx_b == NULL
         || cntx->mtx_c == NULL)
         exitWithFailure("routine", EINVAL);

    lockSuccessAssertion(cntx->mtx_b, "routine");

    sleep(1);
    for (int i = 0; i < PRINT_CNT; ++i){
        lockSuccessAssertion(cntx->mtx_c, "routine");
        printf(THREAD_MSG);
        unlockSuccessAssertion(cntx->mtx_b, "routine");
        lockSuccessAssertion(cntx->mtx_a, "routine");
        unlockSuccessAssertion(cntx->mtx_c, "routine");
        lockSuccessAssertion(cntx->mtx_b, "routine");
        unlockSuccessAssertion(cntx->mtx_a, "routine");
    }

    unlockSuccessAssertion(&mtx_b, "routine");

    return SUCCESS_CODE;
}

void initMutexSuccessAssertion(
    pthread_mutex_t *mtx, 
    pthread_mutexattr_t *mtx_attr,
    const char *msg){
    if (mtx == NULL || mtx_attr)
        return;

    int err = pthread_mutex_init(&mtx_a, NULL);
    assertSuccess(msg, err);
}

void init(const char *err_msg){
    initMutexSuccessAssertion(&mtx_a, NULL, err_msg);
    initMutexSuccessAssertion(&mtx_b, NULL, err_msg);
    initMutexSuccessAssertion(&mtx_c, NULL, err_msg);
}

int initContext(
                Context *cntx, 
                pthread_mutex_t *mtx_a,
                pthread_mutex_t *mtx_b,
                pthread_mutex_t *mtx_c){
    
    if (cntx == NULL || mtx_a == NULL 
        || mtx_b == NULL || mtx_c == NULL)
        return EINVAL;

    cntx->mtx_a = mtx_a;
    cntx->mtx_b = mtx_b;
    cntx->mtx_c = mtx_c;
    cntx->return_code = SUCCESS_CODE;

    return SUCCESS_CODE;
}

void destroyMutexSuccessAssertion(pthread_mutex_t *mtx, const char *msg){
    if (mtx == NULL)
        return;

    int err = pthread_mutex_destroy(mtx);
    assertSuccess(msg, err);
}

void releaseResources(const char *msg){
    destroyMutexSuccessAssertion(&mtx_a, msg);
    destroyMutexSuccessAssertion(&mtx_b, msg);
    destroyMutexSuccessAssertion(&mtx_c, msg);
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

    for (int i = 0; i < PRINT_CNT; ++i){
        lockSuccessAssertion(&mtx_a, "main");
        printf(MAIN_MSG);
        unlockSuccessAssertion(&mtx_c, "main");
        lockSuccessAssertion(&mtx_b, "main");
        unlockSuccessAssertion(&mtx_a, "main");
        lockSuccessAssertion(&mtx_c, "main");
        unlockSuccessAssertion(&mtx_b, "main");
    }

    unlockSuccessAssertion(&mtx_c, "main");
    err = pthread_join(pid, NULL);
    assertSuccess("main", err);

    releaseResources("main:releaseResources");

    pthread_exit((void*)(EXIT_SUCCESS));
}
