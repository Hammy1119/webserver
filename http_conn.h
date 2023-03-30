#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include "locker.h"
#include <sys/uio.h>

class http_conn
{
public:
    static const int FILENAME_LEN = 200;        // �ļ�������󳤶�
    static const int READ_BUFFER_SIZE = 2048;   // ���������Ĵ�С
    static const int WRITE_BUFFER_SIZE = 1024;  // д�������Ĵ�С
    
    // HTTP���󷽷�������ֻ֧��GET
    enum METHOD {GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT};
    
    /*
        �����ͻ�������ʱ����״̬����״̬
        CHECK_STATE_REQUESTLINE:��ǰ���ڷ���������
        CHECK_STATE_HEADER:��ǰ���ڷ���ͷ���ֶ�
        CHECK_STATE_CONTENT:��ǰ���ڽ���������
    */
    enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };
    
    /*
        ����������HTTP����Ŀ��ܽ�������Ľ����Ľ��
        NO_REQUEST          :   ������������Ҫ������ȡ�ͻ�����
        GET_REQUEST         :   ��ʾ�����һ����ɵĿͻ�����
        BAD_REQUEST         :   ��ʾ�ͻ������﷨����
        NO_RESOURCE         :   ��ʾ������û����Դ
        FORBIDDEN_REQUEST   :   ��ʾ�ͻ�����Դû���㹻�ķ���Ȩ��
        FILE_REQUEST        :   �ļ�����,��ȡ�ļ��ɹ�
        INTERNAL_ERROR      :   ��ʾ�������ڲ�����
        CLOSED_CONNECTION   :   ��ʾ�ͻ����Ѿ��ر�������
    */
    enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };
    
    // ��״̬�������ֿ���״̬�����еĶ�ȡ״̬���ֱ��ʾ
    // 1.��ȡ��һ���������� 2.�г��� 3.���������Ҳ�����
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };
public:
    http_conn(){}
    ~http_conn(){}
public:
    void init(int sockfd, const sockaddr_in& addr); // ��ʼ���½��ܵ�����
    void close_conn();  // �ر�����
    void process(); // ����ͻ�������
    bool read();// ��������
    bool write();// ������д
private:
    void init();    // ��ʼ������
    HTTP_CODE process_read();    // ����HTTP����
    bool process_write( HTTP_CODE ret );    // ���HTTPӦ��

    // ������һ�麯����process_read�����Է���HTTP����
    HTTP_CODE parse_request_line( char* text );
    HTTP_CODE parse_headers( char* text );
    HTTP_CODE parse_content( char* text );
    HTTP_CODE do_request();
    char* get_line() { return m_read_buf + m_start_line; }
    LINE_STATUS parse_line();

    // ��һ�麯����process_write���������HTTPӦ��
    void unmap();
    bool add_response( const char* format, ... );
    bool add_content( const char* content );
    bool add_content_type();
    bool add_status_line( int status, const char* title );
    bool add_headers( int content_length );
    bool add_content_length( int content_length );
    bool add_linger();
    bool add_blank_line();

public:
    static int m_epollfd;       // ����socket�ϵ��¼�����ע�ᵽͬһ��epoll�ں��¼��У��������óɾ�̬��
    static int m_user_count;    // ͳ���û�������

private:
    int m_sockfd;           // ��HTTP���ӵ�socket�ͶԷ���socket��ַ
    sockaddr_in m_address;
    
    char m_read_buf[ READ_BUFFER_SIZE ];    // ��������
    int m_read_idx;                         // ��ʶ�����������Ѿ�����Ŀͻ������ݵ����һ���ֽڵ���һ��λ��
    int m_checked_idx;                      // ��ǰ���ڷ������ַ��ڶ��������е�λ��
    int m_start_line;                       // ��ǰ���ڽ������е���ʼλ��

    CHECK_STATE m_check_state;              // ��״̬����ǰ������״̬
    METHOD m_method;                        // ���󷽷�

    char m_real_file[ FILENAME_LEN ];       // �ͻ������Ŀ���ļ�������·���������ݵ��� doc_root + m_url, doc_root����վ��Ŀ¼
    char* m_url;                            // �ͻ������Ŀ���ļ����ļ���
    char* m_version;                        // HTTPЭ��汾�ţ����ǽ�֧��HTTP1.1
    char* m_host;                           // ������
    int m_content_length;                   // HTTP�������Ϣ�ܳ���
    bool m_linger;                          // HTTP�����Ƿ�Ҫ�󱣳�����

    char m_write_buf[ WRITE_BUFFER_SIZE ];  // д������
    int m_write_idx;                        // д�������д����͵��ֽ���
    char* m_file_address;                   // �ͻ������Ŀ���ļ���mmap���ڴ��е���ʼλ��
    struct stat m_file_stat;                // Ŀ���ļ���״̬��ͨ�������ǿ����ж��ļ��Ƿ���ڡ��Ƿ�ΪĿ¼���Ƿ�ɶ�������ȡ�ļ���С����Ϣ
    struct iovec m_iv[2];                   // ���ǽ�����writev��ִ��д���������Զ�������������Ա������m_iv_count��ʾ��д�ڴ���������
    int m_iv_count;

    int bytes_to_send;              // ��Ҫ���͵����ݵ��ֽ���
    int bytes_have_send;            // �Ѿ����͵��ֽ���
};

#endif
