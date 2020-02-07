#ifndef EXTERNAL_PROCESS_CHECK
#define EXTERNAL_PROCESS_CHECK

static const char *socket_name_file;

static const char *socket_name_str;
static bool remote_client;

void init_socket_name(const char* the_socket_name) {
    socket_name_str = the_socket_name;
}

void set_remote_client(bool value) {
    remote_client = value;
}

bool get_remote_client() {
    return remote_client;
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

#endif