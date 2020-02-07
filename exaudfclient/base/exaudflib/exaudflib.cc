#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <fcntl.h>
#include "debug_message.h"
#include <zmq.hpp>
// swig lib
#include <limits>
#include <unistd.h>

#define DONT_EXPOSE_SWIGVM_PARAMS
#include "exaudflib.h"
#undef DONT_EXPOSE_SWIGVM_PARAMS
#include "swig_vm_containers.h"
#include "check_thread.h"

#ifdef PROTEGRITY_PLUGIN_CLIENT
#include <protegrityclient.h>
#endif

using namespace SWIGVMContainers;
using namespace std;
using namespace google::protobuf;

#ifndef PROTEGRITY_PLUGIN_CLIENT
__thread SWIGVM_params_t* SWIGVMContainers::SWIGVM_params; // this is not used in the file, but defined to satisfy the "extern" requirement from exaudflib.h
#endif

#ifndef NDEBUG
#define SWIGVM_LOG_CLIENT
#endif

static pid_t my_pid; //parent_pid,

void delete_vm(SWIGVM*& vm){
    if (vm != nullptr)
    {
        delete vm;
        vm = nullptr;
    }
}

void print_args(int argc,char**argv){
    for (int i = 0; i<argc; i++)
    {
        cerr << "zmqcontainerclient argv[" << i << "] = " << argv[i] << endl;
    }
}


extern "C" {

SWIGVMContainers::SWIGMetadata* create_SWIGMetaData() {
    return new SWIGVMContainers::SWIGMetadata_Impl();
}

SWIGVMContainers::AbstractSWIGTableIterator* create_SWIGTableIterator() {
    return new SWIGVMContainers::SWIGTableIterator_Impl();
}

SWIGVMContainers::SWIGRAbstractResultHandler* create_SWIGResultHandler(SWIGVMContainers::SWIGTableIterator* table_iterator) {
    return new SWIGVMContainers::SWIGResultHandler_Impl(table_iterator);
}

int exaudfclient_main(std::function<SWIGVM*()>vmMaker,int argc,char**argv)
{
    assert(SWIGVM_params_ref != nullptr);

#ifdef PROTEGRITY_PLUGIN_CLIENT
    stringstream socket_name_ss;
#endif
    string socket_name = argv[1];
    char* socket_name_str = argv[1];
    socket_name_file = argv[1];

    init_socket_name(socket_name_str);

    set_remote_client(false);
    my_pid = ::getpid(); //TODO unused?

    zmq::context_t context(1);

    DBG_COND_FUNC_CALL(cerr, print_args(argc,argv));

    if (socket_name.length() > 4 ) {
#ifdef PROTEGRITY_PLUGIN_CLIENT
        // protegrity client has no arguments
#else
        if (! ((strcmp(argv[2], "lang=python") == 0)
               || (strcmp(argv[2], "lang=r") == 0)
               || (strcmp(argv[2], "lang=java") == 0)
               || (strcmp(argv[2], "lang=streaming") == 0)
               || (strcmp(argv[2], "lang=benchmark") == 0)) )
        {
            cerr << "Remote VM type '" << argv[2] << "' not supported." << endl;
            return 2;
        }
#endif
    } else {
        cerr << "socket name '" << socket_name << "' is invalid." << endl;
        abort();
    }

    if (strncmp(socket_name_str, "tcp:", 4) == 0) {
        set_remote_client(true);
    }

    if (socket_name.length() > 6 && strncmp(socket_name_str, "ipc:", 4) == 0)
    {        
#ifdef PROTEGRITY_PLUGIN_CLIENT
/*
    DO NOT REMOVE, required for Exasol 6.2
*/
        if (strncmp(socket_name_str, "ipc:///tmp/", 11) == 0) {
            socket_name_ss << "ipc://" << getenv("NSEXEC_TMP_PATH") << '/' << &(socket_name_file[11]);
            socket_name = socket_name_ss.str();
            socket_name_str = strdup(socket_name_ss.str().c_str());
            socket_name_file = socket_name_str;
        }
#endif
        socket_name_file = &(socket_name_file[6]);
    }

    DBG_STREAM_MSG(cerr,"### SWIGVM starting " << argv[0] << " with name '" << socket_name << " (" << ::getppid() << ',' << ::getpid() << "): '" << argv[1] << '\'');

    start_check_thread();


    int linger_timeout = 0;
    int recv_sock_timeout = 1000;
    int send_sock_timeout = 1000;

    if (get_remote_client()) {
        recv_sock_timeout = 10000;
        send_sock_timeout = 5000;
    }

reinit:

    DBGMSG(cerr,"Reinit");
    zmq::socket_t socket(context, ZMQ_REQ);

    socket.setsockopt(ZMQ_LINGER, &linger_timeout, sizeof(linger_timeout));
    socket.setsockopt(ZMQ_RCVTIMEO, &recv_sock_timeout, sizeof(recv_sock_timeout));
    socket.setsockopt(ZMQ_SNDTIMEO, &send_sock_timeout, sizeof(send_sock_timeout));

    if (get_remote_client()) socket.bind(socket_name_str);
    else socket.connect(socket_name_str);

    SWIGVM_params_ref->sock = &socket;
    SWIGVM_params_ref->exch = &exchandler;

    SWIGVM*vm=nullptr;

    if (!send_init(socket, socket_name)) {
        if (!get_remote_client() && exchandler.exthrowed) {
            send_close(socket, exchandler.exmsg);
            goto error;
        }else{
            goto reinit;
        }
    }

    SWIGVM_params_ref->dbname = (char*) g_database_name.c_str();
    SWIGVM_params_ref->dbversion = (char*) g_database_version.c_str();
    SWIGVM_params_ref->script_name = (char*) g_script_name.c_str();
    SWIGVM_params_ref->script_schema = (char*) g_script_schema.c_str();
    SWIGVM_params_ref->current_user = (char*) g_current_user.c_str();
    SWIGVM_params_ref->current_schema = (char*) g_current_schema.c_str();
    SWIGVM_params_ref->scope_user = (char*) g_scope_user.c_str();
    SWIGVM_params_ref->script_code = (char*) g_source_code.c_str();
    SWIGVM_params_ref->session_id = g_session_id;
    SWIGVM_params_ref->statement_id = g_statement_id;
    SWIGVM_params_ref->node_count = g_node_count;
    SWIGVM_params_ref->node_id = g_node_id;
    SWIGVM_params_ref->vm_id = g_vm_id;
    SWIGVM_params_ref->singleCallMode = g_singleCallMode;

    try {
        vm = vmMaker();
        if (vm == nullptr) {
            send_close(socket, "Unknown or unsupported VM type");
            goto error;
        }
        if (vm->exception_msg.size()>0) {
            throw SWIGVM::exception(vm->exception_msg.c_str());
        }

        use_zmq_socket_locks = vm->useZmqSocketLocks();

        if (g_singleCallMode) {
            ExecutionGraph::EmptyDTO noArg; // used as dummy arg
            for (;;) {
                // in single call mode, after MT_RUN from the client,
                // EXASolution responds with a CALL message that specifies
                // the single call function to be made
                if (!send_run(socket)) {
                    break;
                }

                assert(g_singleCallFunction != single_call_function_id_e::SC_FN_NIL);
                try {
                    const char* result = nullptr;
                    switch (g_singleCallFunction)
                    {
                        case single_call_function_id_e::SC_FN_NIL:
                            break;
                        case single_call_function_id_e::SC_FN_DEFAULT_OUTPUT_COLUMNS:
                            result = vm->singleCall(g_singleCallFunction,noArg);
                            break;
                        case single_call_function_id_e::SC_FN_GENERATE_SQL_FOR_IMPORT_SPEC:
                            assert(!g_singleCall_ImportSpecificationArg.isEmpty());
                            result = vm->singleCall(g_singleCallFunction,g_singleCall_ImportSpecificationArg);
                            g_singleCall_ImportSpecificationArg = ExecutionGraph::ImportSpecification();  // delete the last argument
                            break;
                        case single_call_function_id_e::SC_FN_GENERATE_SQL_FOR_EXPORT_SPEC:
                            assert(!g_singleCall_ExportSpecificationArg.isEmpty());
                            result = vm->singleCall(g_singleCallFunction,g_singleCall_ExportSpecificationArg);
                            g_singleCall_ExportSpecificationArg = ExecutionGraph::ExportSpecification();  // delete the last argument
                            break;
                        case single_call_function_id_e::SC_FN_VIRTUAL_SCHEMA_ADAPTER_CALL:
                            assert(!g_singleCall_StringArg.isEmpty());
                            result = vm->singleCall(g_singleCallFunction,g_singleCall_StringArg);
                            break;
                    }
                    if (vm->exception_msg.size()>0) {
                        send_close(socket, vm->exception_msg);
                        goto error;
                    }

                    if (vm->calledUndefinedSingleCall.size()>0) {
                        send_undefined_call(socket, vm->calledUndefinedSingleCall);
                    } else {
                        send_return(socket,result);
                    }

                    if (!send_done(socket)) {
                        break;
                    }
                } catch(...) {}
            }
        } else {
            for(;;) {
                if (!send_run(socket))
                    break;
                SWIGVM_params_ref->inp_force_finish = false;
                while(!vm->run_())
                {
                    if (vm->exception_msg.size()>0) {
                        send_close(socket, vm->exception_msg);
                        goto error;
                    }
                }
                if (!send_done(socket))
                    break;
            }
        }

        if (vm != nullptr)
        {
            vm->shutdown();
            if (vm->exception_msg.size()>0) {
                send_close(socket, vm->exception_msg);
                goto error;
            }
        }
        send_finished(socket);
    }  catch (SWIGVM::exception &err) {
        DBG_STREAM_MSG(cerr,"### SWIGVM crashing with name '" << socket_name << " (" << ::getppid() << ',' << ::getpid() << "): " << err.what());
        send_close(socket, err.what());
        goto error;
    } catch (std::exception &err) {
        DBG_STREAM_MSG(cerr,"### SWIGVM crashing with name '" << socket_name << " (" << ::getppid() << ',' << ::getpid() << "): " << err.what());
        send_close(socket, err.what());
        goto error;
    } catch (...) {
        DBG_STREAM_MSG(cerr,"### SWIGVM crashing with name '" << socket_name << " (" << ::getppid() << ',' << ::getpid() << ')');
        send_close(socket, "Internal/Unknown error");
        goto error;
    }


    DBG_STREAM_MSG(cerr,"### SWIGVM finishing with name '" << socket_name << " (" << ::getppid() << ',' << ::getpid() << ')');

    delete_vm(vm);
    stop_socket(socket);
    return 0;

error:
    delete_vm(vm);
    stop_socket(socket);
    return 1;
}


} // extern "C"
