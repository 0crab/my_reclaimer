#include <iostream>
#include <atomic>
#include <thread>
#include "item.h"
#include "random_gen.h"
#include "tracer.h"

#include "reclaimer_ebr_token.h"

#define DEFAULT_ITEM_LEN 24

using namespace std;

static int THREAD_NUM ;
static int TEST_NUM;
static int TEST_TIME;
static double SKEW;
static double WRITE_RATIO;
static int BUCKET_NUM;

uint64_t false_count;

typedef struct Request {
    char *key;
    uint16_t key_len;
    char *value;
    uint16_t value_len;
};

atomic<uint64_t> * buckets;
Request *requests;

bool * writelist;
uint64_t *runtimelist;

atomic<int> stopMeasure(0);
uint64_t runner_count;

Reclaimer_ebr_token * myReclaimer;


void concurrent_worker(int tid){
    debug_tid = tid;
    myReclaimer->initThread(tid);

    uint64_t index = 0;
    Tracer t;
    t.startTime();
    while(stopMeasure.load(memory_order_relaxed) == 0){
        for(size_t i = 0; i < TEST_NUM; i++){
            Request & req = requests[i];
            if(writelist[i]){
                index = *(uint64_t *)req.key;
                //assert(index == *(uint64_t *)req.value);

                Item *ptr = (Item *) myReclaimer->allocate(tid,ITEM_LEN_ALLOC(req.key_len,req.value_len));

                init_item(ptr,req.key,req.key_len,req.value,req.value_len);

                uint64_t old;
                do{
                    old = buckets[index].load();
                }while(!buckets[index].compare_exchange_strong(old, (uint64_t) ptr)); //changed memory order

                Item *oldptr = (Item *) old;

                uint64_t k,v;
                k = *(uint64_t *)ITEM_KEY(oldptr);
                //v = *(uint64_t *)ITEM_VALUE(oldptr);

                assert(*(uint64_t *)ITEM_KEY(oldptr) == index);
                uint32_t vl = ITEM_VALUE_LEN(oldptr);
                int tmpv = vl % 10;
                string tmps(vl,'0' +tmpv);
                if(strncmp(tmps.c_str(),ITEM_VALUE(oldptr),vl)) assert(false);

                myReclaimer->deallocate(tid,oldptr);

            }else{
                index =  *(uint64_t *)req.key;
                //assert(index == *(uint64_t *)req.value);

                Item *ptr = (Item *) myReclaimer->load(tid, std::ref(buckets[index]));


                assert(*(uint64_t *)ITEM_KEY(ptr) == index);
                uint32_t vl = ITEM_VALUE_LEN(ptr);
                int tmpv = vl % 10;
                string tmps(vl,'0' +tmpv);
                if(strncmp(tmps.c_str(),ITEM_VALUE(ptr),vl)) assert(false);
                //if(*(uint64_t *)ITEM_VALUE(ptr) != index) false_count++;


                myReclaimer->read(tid);
            }
        }

        __sync_fetch_and_add(&runner_count,TEST_NUM);
        uint64_t tmptruntime = t.fetchTime();
        if(tmptruntime / 1000000 >= TEST_TIME){
            stopMeasure.store(1, memory_order_relaxed);
        }
    }
    runtimelist[tid] = t.getRunTime();
    dump_debug_thread_work_info();
}

void prepare_data(){

    double skew = SKEW;

    uint64_t *loads = new uint64_t[TEST_NUM]();
    RandomGenerator<uint64_t>::generate(loads,BUCKET_NUM-1,TEST_NUM,skew);

    srand((unsigned) time(NULL));
    //init_req

    int kl = 8,vl;
    requests = new Request[TEST_NUM];
    for (size_t i = 0; i < TEST_NUM; i++) {

        requests[i].key_len = kl;
        requests[i].key = (char *) calloc(1, kl * sizeof(char));
        *((size_t *) requests[i].key) = loads[i];

        vl = rand() % 505;
        //vl=600;
        requests[i].value_len = vl;
        requests[i].value = (char *) calloc(1, vl * sizeof(char));

        int tmpv = vl % 10;
        string tmps(vl,'0' +tmpv);
        memcpy(requests[i].value,tmps.c_str(),vl);

    }

    delete[] loads;

}

int main(int argc, char **argv){
    if (argc == 7) {
        THREAD_NUM = stol(argv[1]);
        BUCKET_NUM = stol(argv[2]);
        TEST_NUM = stol(argv[3]);
        SKEW = stod(argv[4]);
        WRITE_RATIO = stod(argv[5]);
        TEST_TIME = stol(argv[6]);
    } else {
        printf("./a.out <thread_num> <bucket_num> <test_num> <skew> <write_ratio> <test_time>\n");
        return 0;
    }

    cout<<"thread_num "<<THREAD_NUM<<endl<<
        "bucket_num "<<BUCKET_NUM<<endl<<
        "test_num "<<TEST_NUM<<endl<<
        "skew "<<SKEW<<endl<<
        "write_ratio "<<WRITE_RATIO<<endl<<
        "test_time "<<TEST_TIME<<endl;

    myReclaimer = new Reclaimer_ebr_token(THREAD_NUM);


    buckets = new std::atomic<uint64_t>[BUCKET_NUM];

    myReclaimer->initThread(0);

    prepare_data();

    for (size_t i = 0; i < BUCKET_NUM ; i++) {
        Item *ptr = (Item *) myReclaimer->allocate(0,ITEM_LEN_ALLOC(8,8));
        size_t idx = i;

        char tmps[10] = "88888888";
        init_item(ptr,(char *)&i,8,tmps,8);
        buckets[idx].store((uint64_t) ptr);
    }



    runtimelist = new uint64_t[THREAD_NUM]();


    srand(time(NULL));
    writelist = new bool[TEST_NUM];
    for(size_t i = 0; i < TEST_NUM; i++ ){
        writelist[i] = rand() *1.0 / RAND_MAX * 100 < WRITE_RATIO;
    }

    vector<thread> threads;
    for(size_t i = 0; i < THREAD_NUM; i++)  threads.push_back(thread(concurrent_worker,i));
    for(size_t i = 0; i < THREAD_NUM; i++)  threads[i].join();


    double runtime = 0 , throughput = 0;
    for(size_t i = 0 ; i < THREAD_NUM; i++)
        runtime += runtimelist[i];
    runtime /= THREAD_NUM;
    throughput = runner_count * 1.0 / runtime;
    cout<<"runner_count "<<runner_count<<endl;
    cout<<"runtime "<<runtime / 1000000<<"s"<<endl;
    cout<<"***throughput "<<throughput<<endl<<endl;

    cout<<"false count "<<false_count<<endl;

    myReclaimer->dump();

}