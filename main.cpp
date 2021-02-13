#include <iostream>
#include <pthread.h>
#include <atomic>
#include <unistd.h>
#include <memory>
#include <string.h>

#include "producer.h"
#include "interruptor.h"
#include "consumer.h"

using namespace std;

atomic_bool state=false;

int get_tid() 
{
    static atomic_int s_counter;
    
    thread_local unique_ptr<int> id = nullptr;
    
    if(id == nullptr)
        id = make_unique<int>(++s_counter);

    return *id;
}


void try_destroy_barrier(pthread_barrier_t * p_barrier)
{
    int status = pthread_barrier_wait(p_barrier);
      
    if(status == PTHREAD_BARRIER_SERIAL_THREAD)
        pthread_barrier_destroy(p_barrier);
}

 
void* producer_routine(void* arg) 
{      
    Producer* data = (Producer*)arg;       
    try_destroy_barrier(data->p_barrier);            
    while(true)
    {
        pthread_mutex_lock(data->p_mutex);        
        int status = scanf("%d", data->p_input);
        if(status == EOF)
        {                                
            *data->p_done = true;
            pthread_cond_broadcast(data->p_cond_input);
            pthread_mutex_unlock(data->p_mutex);        
            break;
        }
        state=true;
        pthread_cond_signal(data->p_cond_input);
        while (state) pthread_cond_wait(data->p_cond_process, data->p_mutex);
                                       
        pthread_mutex_unlock(data->p_mutex);        
    }
    return nullptr;
}
 
void* consumer_routine(void* arg) 
{
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    Consumer* data = (Consumer*)arg;    
    try_destroy_barrier(data->p_barrier);
    
    long sum = 0;
    
    for(;;)
    {                        
        pthread_mutex_lock(data->p_mutex);
        while (!state && !*data->p_done) {
          pthread_cond_wait(data->p_cond_input, data->p_mutex);
        }

        if(*data->p_done)
        {
            pthread_mutex_unlock(data->p_mutex);
            break;
        }
        sum += *data->p_input;
        state = false;
        pthread_cond_signal(data->p_cond_process);
        if(data->debug)
            printf("(%d, %ld)\n", get_tid(), sum);        
        pthread_mutex_unlock(data->p_mutex);        
        usleep(*data->p_sleep);
    }

    

    pthread_exit((void *)sum);

    return nullptr;
}
 
void* consumer_interruptor_routine(void* arg) 
{
    Interruptor *data = (Interruptor *)arg;
    try_destroy_barrier(data->p_barrier);

    while(!*data->p_done)
    {
        int index = rand() % data->num_consumers;
        if(*data->p_done) 
            break;
        pthread_cancel(data->consumers[index]);                
    }
    return nullptr;
}
 
int run_threads(int nThreads, int maxSleeping, bool debug) 
{
    int input;
    atomic<bool> done = false;
    pthread_barrier_t barrier;
    pthread_mutex_t mutex_p;
    
    pthread_cond_t cond_input;
    pthread_cond_t cond_process;

    pthread_t producer;
    pthread_t consumer_interruptor;
    pthread_t * consumers = new pthread_t[nThreads];    

    pthread_mutex_init(&mutex_p,NULL);

    pthread_cond_init(&cond_input, NULL);
    pthread_cond_init(&cond_process, NULL);

    pthread_barrier_init(&barrier, NULL, nThreads + 3);

    Producer *prod = new Producer(
        &input, 
        &done,
        &barrier,
        &mutex_p,
        &cond_input,
        &cond_process);

    pthread_create(&producer, NULL, producer_routine, (void *)prod); 

    Interruptor *inter = new Interruptor(
        &done,
        &barrier,
        nThreads,
        consumers);

    pthread_create(&consumer_interruptor, NULL, consumer_interruptor_routine, (void *)inter); 

    int sleep = 0; 
    if(maxSleeping > 0)
        sleep = (rand() % maxSleeping)*1000;

    Consumer** cons_array = new Consumer*[nThreads];
    for(int i = 0; i < nThreads; i++)
    {
        Consumer* cons = new Consumer(
            &input,
            &done,
            &sleep,
            &barrier,
            &mutex_p,
            &cond_input,
            &cond_process,
            debug);
        
        cons_array[i] = cons;
        pthread_create(&consumers[i], NULL, consumer_routine, (void *)cons);
    }

    try_destroy_barrier(&barrier);

    long* results = new long[nThreads];

    pthread_join(consumer_interruptor, NULL);

    pthread_join(producer, NULL);
    for(int i = 0; i < nThreads; i++)
        pthread_join(consumers[i], (void**)&results[i]);                
    pthread_mutex_destroy(&mutex_p);

    pthread_cond_destroy(&cond_input);
    pthread_cond_destroy(&cond_process);

    int end_sum = 0;
    for(int i = 0; i < nThreads; i++)
      end_sum += results[i];

    delete [] consumers;
    delete prod;
    delete inter;
    for(int i = 0; i < nThreads; i++)
        delete cons_array[i];
    delete [] cons_array; 
    delete [] results;

    return end_sum;
}  
 
int main(int argc, char* argv[]) 
{
    if (!(argc == 3 || argc == 4))
    {
        return -1;
    }

    int nThreads;
    int maxSleeping;
    bool debug = false;

    if (argc == 3)
    {
        nThreads = atoi(argv[1]);
        maxSleeping = atoi(argv[2]);
    }
    else if (!strcmp(argv[1], "-debug"))
    {
        debug = true;
        nThreads = atoi(argv[2]);
        maxSleeping = atoi(argv[3]);
    }
    else if (!strcmp(argv[2], "-debug"))
    {
        debug = true;
        nThreads = atoi(argv[1]);
        maxSleeping = atoi(argv[3]);
    }

    else if (!strcmp(argv[3], "-debug"))
    {
        debug = true;
        nThreads = atoi(argv[1]);
        maxSleeping = atoi(argv[2]);
    }
    else
        return -1;

    if (nThreads <= 0)
    {
        return -1;
    }

    if (maxSleeping < 0)
    {
        return -1;
    }
    std::cout << run_threads(nThreads, maxSleeping, debug) << std::endl;
    return 0;
}
