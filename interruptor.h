#ifndef INTERRUPTOR_H
#define INTERRUPTOR_H

class Interruptor
{
    public:        
        std::atomic_bool *p_done;
        pthread_barrier_t *p_barrier;
        int num_consumers;
        pthread_t *consumers;
        
        Interruptor(
            std::atomic_bool *p_done,
            pthread_barrier_t *p_barrier,
            int num_consumers,
            pthread_t *consumers) : p_done(p_done), p_barrier(p_barrier), num_consumers(num_consumers), consumers(consumers) {};
};          
            
 #endif           