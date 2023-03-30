#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "locker.h"
#include "http_conn.h"

// �̳߳��࣬��������Ϊģ������Ϊ�˴��븴�ã�ģ�����T��������
template<typename T>
class threadpool {
public:
    /*thread_number���̳߳����̵߳�������max_requests������������������ġ��ȴ���������������*/
    threadpool(int thread_number = 8, int max_requests = 10000);
    ~threadpool();
    bool append(T* request);

private:
    /*�����߳����еĺ����������ϴӹ���������ȡ������ִ��֮*/
    static void* worker(void* arg);
    void run();

private:
    // �̵߳�����
    int m_thread_number;  
    
    // �����̳߳ص����飬��СΪm_thread_number    
    pthread_t * m_threads;

    // ����������������ġ��ȴ���������������  
    int m_max_requests; 
    
    // �������
    std::list< T* > m_workqueue;  

    // ����������еĻ�����
    locker m_queuelocker;   

    // �Ƿ���������Ҫ����
    sem m_queuestat;

    // �Ƿ�����߳�          
    bool m_stop;                    
};



#endif
