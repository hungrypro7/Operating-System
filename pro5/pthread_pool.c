/*
 * Copyright(c) 2021-2023 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
* --------------한양대학교 ERICA ICT융합학부 2019098068 이찬영------------------
 * 06.07
 * 스레드풀을 위해 worker, pthread_pool_init, pthread_pool_submit, pthread_pool_shutdown 함수를 구현하였습니다.
 * 뮤텍스락 1개와 조건변수 2개를 사용하여 스레드풀 기능을 구현하였습니다.
 */

#include "pthread_pool.h"
#include <stdlib.h>

/*
 * 풀에 있는 일꾼(일벌) 스레드가 수행할 함수이다.
 * FIFO 대기열에서 기다리고 있는 작업을 하나씩 꺼내서 실행한다.
 * 대기열에 작업이 없으면 새 작업이 들어올 때까지 기다린다.
 * 이 과정을 스레드풀이 종료될 때까지 반복한다.
 */
static void *worker(void *param)
{
    pthread_pool_t *pool = (pthread_pool_t *)param;     // 스레드풀 제어블록 생성

    while(1) {
        pthread_mutex_lock(&pool->mutex);      // 일꾼 스레드의 동기화를 위함

        // 대기열에 작업이 없고, 스레드풀이 실행 상태이면 빈 대기열에 새 작업이 들어올 때까지 기다림
        while(pool->q_len == 0 && pool->running == true) {
            pthread_cond_wait(&pool->full, &pool->mutex);
        }

        if(!pool->running) {    // 스레드풀이 종료되면, 반복문 탈출
          pthread_mutex_unlock(&(pool->mutex));
          break;
        }

        // 큐로부터 다음 작업을 얻음
        task_t task = pool->q[pool->q_front];
        pool->q_front = (pool->q_front + 1) % pool->q_size;
        pool->q_len--;

        // 만약, 대기열의 자리가 하나 생기거나, 대기열의 길이가 0이면, &pool->empty에 시그널 보냄
        if(pool->q_len == pool->q_size-1 || pool->q_len == 0) {     
          pthread_cond_signal(&pool->empty);
        }

        pthread_mutex_unlock(&pool->mutex);

        // task를 수행한다.
        task.function(task.param);
    }
    pthread_exit(NULL);
}

/*
 * 스레드풀을 생성한다. bee_size는 일꾼(일벌) 스레드의 개수이고, queue_size는 대기열의 용량이다.
 * bee_size는 POOL_MAXBSIZE를, queue_size는 POOL_MAXQSIZE를 넘을 수 없다.
 * 일꾼 스레드와 대기열에 필요한 공간을 할당하고 변수를 초기화한다.
 * 일꾼 스레드의 동기화를 위해 사용할 상호배타 락과 조건변수도 초기화한다.
 * 마지막 단계에서는 일꾼 스레드를 생성하여 각 스레드가 worker() 함수를 실행하게 한다.
 * 대기열로 사용할 원형 버퍼의 용량이 일꾼 스레드의 수보다 작으면 효율을 극대화할 수 없다.
 * 이런 경우 사용자가 요청한 queue_size를 bee_size로 상향 조정한다.
 * 성공하면 POOL_SUCCESS를, 실패하면 POOL_FAIL을 리턴한다.
 */
int pthread_pool_init(pthread_pool_t *pool, size_t bee_size, size_t queue_size)
{
    // 만약 queue_size 나 일꾼 스레드의 개수가 최대치를 초과하면 POOL_FAIL을 리턴
    if (queue_size > POOL_MAXQSIZE || bee_size > POOL_MAXBSIZE) {
        return POOL_FAIL;
    }

    if (bee_size > queue_size) {
        queue_size = bee_size;
    }

    // 일꾼 스레드와 대기열에 필요한 공간 할당
    pool->bee = (pthread_t *)malloc(bee_size * sizeof(pthread_t));
    pool->q = (task_t *)malloc(queue_size * sizeof(task_t));

    if (pool->bee == NULL || pool->q == NULL) {     // 일꾼 스레드 배열이 없거나 원형 버퍼가 없을 때
        return POOL_FAIL;
    }

    // 변수 초기화
    pool->running = true;
    pool->q_size = queue_size;
    pool->q_front = 0;
    pool->q_len = 0;
    pool->bee_size = bee_size;

    // 동기화를 위한 상호배타 락 및 조건변수 초기화
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->full, NULL);
    pthread_cond_init(&pool->empty, NULL);

    // 일꾼 스레드 생성
    int i;
    for (i = 0; i < bee_size; ++i) {
        pthread_create(&pool->bee[i], NULL, worker, (void*)pool);
    }

    return POOL_SUCCESS;    // 작업 요청이 성공하면 리턴
}

/*
 * 스레드풀에서 실행시킬 함수와 인자의 주소를 넘겨주며 작업을 요청한다.
 * 스레드풀의 대기열이 꽉 찬 상황에서 flag이 POOL_NOWAIT이면 즉시 POOL_FULL을 리턴한다.
 * POOL_WAIT이면 대기열에 빈 자리가 나올 때까지 기다렸다가 넣고 나온다.
 * 작업 요청이 성공하면 POOL_SUCCESS를 리턴한다.
 */
int pthread_pool_submit(pthread_pool_t *pool, void (*f)(void *p), void *p, int flag)
{   
    pthread_mutex_lock(&pool->mutex);

    // 대기열이 꽉 찬 상황
    if (pool->q_len == pool->q_size) {
      if (flag == POOL_NOWAIT) {
        pthread_mutex_unlock(&pool->mutex);
        return POOL_FULL;     // 즉시 리턴
      }
      if (flag == POOL_WAIT) {
        while (pool->q_len == pool->q_size) {
          pthread_cond_wait(&pool->empty, &pool->mutex);    // 대기열에 빈 자리가 나올 때까지 기다렸다가 넣고 나옴
        }
      }
    }

    // 작업을 큐에 추가하고 작업을 요청한다.
    pool->q[(pool->q_front + pool->q_len) % pool->q_size].function = f;
    pool->q[(pool->q_front + pool->q_len) % pool->q_size].param = p;
    pool->q_len++;

    // 작업이 추가되면 새 작업이 들어오길 기다리고 있는 스레드에게 신호를 보낸다.
    pthread_cond_signal(&pool->full);
    pthread_mutex_unlock(&pool->mutex);

    return POOL_SUCCESS;    // 작업 요청이 성공하면 리턴
}

/*
 * 스레드풀을 종료한다. 일꾼 스레드가 현재 작업 중이면 그 작업을 마치게 한다.
 * how의 값이 POOL_COMPLETE이면 대기열에 남아 있는 모든 작업을 마치고 종료한다.
 * POOL_DISCARD이면 대기열에 새 작업이 남아 있어도 더 이상 수행하지 않고 종료한다.
 * 부모 스레드는 종료된 일꾼 스레드와 조인한 후에 스레드풀에 할당된 자원을 반납한다.
 * 스레드를 종료시키기 위해 철회를 생각할 수 있으나 바람직하지 않다.
 * 락을 소유한 스레드를 중간에 철회하면 교착상태가 발생하기 쉽기 때문이다.
 * 종료가 완료되면 POOL_SUCCESS를 리턴한다.
 */
int pthread_pool_shutdown(pthread_pool_t *pool, int how)
{
    // 일꾼 스레드가 현재 작업 중이면 대기열에 남아 있는 모든 작업을 마칠 때까지 기다림
    if (how == POOL_COMPLETE) {
        // 큐가 비어질 때까지 기다린다.
        pthread_mutex_lock(&pool->mutex);
        while (pool->q_len > 0) {
          pthread_cond_wait(&pool->empty, &pool->mutex);    
        }
        pthread_mutex_unlock(&pool->mutex);
    }
    pool->running = false;      // 더 이상 수행하지 않고 종료함
    pthread_cond_broadcast(&pool->full);    // pool->full 조건변수에 신호를 보냄

    // 모든 일꾼 스레드를 종료시킨다.
    int i;
    for (i = 0; i < pool->bee_size; ++i) {
        pthread_join(pool->bee[i], NULL);
    }
    
    // 할당된 자원을 반납한다.
    free(pool->bee);
    free(pool->q);
    
    // 뮤텍스와 조건변수를 destroy
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->full);
    pthread_cond_destroy(&pool->empty);
    
    return POOL_SUCCESS;    // 작업 요청이 성공하면 리턴
}