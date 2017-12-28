#ifndef __JOB_SCHEDULER__
#define __JOB_SCHEDULER__

#include <pthread.h>
#include <stdlib.h>
#include "job.h"

#define JOBS_AMOUNT 30
#define THREADS_AMOUNT 10

typedef struct queue{
    int size;
    int amount_of_jobs;
    int position;
    job* my_jobs;
}queue;

typedef struct job_scheduler{
    pthread_t * threads;
    pthread_mutex_t mutex;
    pthread_cond_t condition_variable;
    queue* my_queue;
    int threads_amount;
}job_scheduler;

queue* create_queue();
job_scheduler* initialize_scheduler();
void submit_job(queue* my_queue,job* myjob);
void clean_job_table(queue* my_queue);
void destroy_job_scheduler(job_scheduler** myjob_scheduler);


#endif