#ifndef PRODUCER_H
#define PRODUCER_H

class Producer
{
    public:
        int * p_input;
        std::atomic_bool *p_done;
        pthread_barrier_t *p_barrier;
        pthread_mutex_t *p_mutex;

        pthread_cond_t *p_cond_input;
        pthread_cond_t *p_cond_process;
        
        Producer(
            int *p_input,
            std::atomic_bool *p_done,
            pthread_barrier_t *p_barrier,
            pthread_mutex_t *p_mutex,
            pthread_cond_t *p_cond_input,
            pthread_cond_t *p_cond_process) : p_input(p_input), p_done(p_done), p_barrier(p_barrier), p_mutex(p_mutex), p_cond_input(p_cond_input), p_cond_process(p_cond_process) {};
};            
            
 #endif           