#ifndef CHECK_THREAD
#define CHECK_THREAD

#include "external_process_check.h"
#include <thread>

pthread_t check_thread;

static bool keep_checking = true;

void *check_thread_routine(void* data)
{
    while(keep_checking) {
        external_process_check();
        ::usleep(100000);
    }
    return NULL;

}

void start_check_thread() {
    if (!remote_client)
        pthread_create(&check_thread, NULL, check_thread_routine, NULL);
}

void stop_check_thread() {
    keep_checking = false;
}

void cancel_check_thread() {
    ::pthread_cancel(check_thread);
}

void stop_socket(zmq::socket_t& socket){
    socket.close();
    stop_check_thread();
    if (!get_remote_client()) {
        cancel_check_thread();
        ::unlink(socket_name_file);
    } else {
        ::sleep(3); // give other components time to shutdown
    }
}

#endif