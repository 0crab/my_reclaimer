#ifndef MY_RECLAIMER_MULTI_LEVEL_QUEUE_H
#define MY_RECLAIMER_MULTI_LEVEL_QUEUE_H

#include "item.h"
#include "buffer_queue.h"

template <typename T>
class MultiLevelQueue{
public:
    MultiLevelQueue();

    T * allocate(uint64_t len);
    void deallocate(T * ptr);
    void free_limbobag(BufferQueue<T> * freebag);

    void dump();

private:

    BufferQueue<T> BFQS[MAX_ORDER];

};

template <typename T>
MultiLevelQueue<T>::MultiLevelQueue() {
    uint64_t s = BASE_SIZE;
    //size,listhead has been initialized
    for (auto &i : this->BFQS) {
        i.set_unif_size(s);
        s *= FACTOR;
    }
}

template <typename T>
void MultiLevelQueue<T>::dump() {
    for(int i=0;i<MAX_ORDER;i++){
        std::cout<<"ORDER:\t"<<i<<" unif_size:\t"<<BFQS[i].get_unif_size()<<" size\t"<<BFQS[i].get_size()<<std::endl;
    }
}

template <typename T>
T * MultiLevelQueue<T>::allocate(uint64_t len) {
    void *p;
    //size is too large,malloc directly
    if(len>MAX_UNIF_SIZE){
        p=malloc(len);
        return (T *)p;
    }

    //serch in bufqueue
    //since small unif_sizes are used more frequently,we start with the header
    int i=0;
    while(BFQS[i].get_unif_size() < len) i++;

    return BFQS[i].remove(len);

}

template<typename T>
void MultiLevelQueue<T>::deallocate(T *ptr) {
    uint64_t len = ptr->get_struct_len();
    if(len>MAX_UNIF_SIZE){
        free(ptr);
    }

    int i=0;
    while(BFQS[i].get_unif_size()<len) i++;

    BFQS[i].add(ptr,len);
}


template<typename T>
void MultiLevelQueue<T>::free_limbobag(BufferQueue<T> * freebag) {

    while(freebag->get_size() > 0){
        T * p = freebag->pop();
        deallocate(p);
    }
}

#endif //MY_RECLAIMER_MULTI_LEVEL_QUEUE_H
