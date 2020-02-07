#ifndef CHECK_THREAD
#define CHECK_THREAD

#include <thread>

pthread_t check_thread;

static pid_t my_pid; //parent_pid,
static const char *socket_name_file;

static const char *socket_name_str;
static bool remote_client;

void init_socket_name(const char* the_socket_name) {
    socket_name_str = the_socket_name;
}

static void external_process_check()
{
    if (remote_client) return;
    if (::access(socket_name_file, F_OK) != 0) {
        ::sleep(1); // give me a chance to die with my parent process
        cerr << "exaudfclient aborting ... cannot access socket file " << socket_name_str+6 << "." << endl;
        DBG_STREAM_MSG(cerr,"### SWIGVM aborting with name '" << socket_name_str << "' (" << ::getppid() << ',' << ::getpid() << ')');
        ::abort();
    }
}

void set_remote_client(bool value) {
    remote_client = value;
}

bool get_remote_client() {
    return remote_client;
}


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

void print_args(int argc,char**argv){
    for (int i = 0; i<argc; i++)
    {
        cerr << "zmqcontainerclient argv[" << i << "] = " << argv[i] << endl;
    }
}

void stop_all(zmq::socket_t& socket){
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