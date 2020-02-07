#ifndef SOCKET_FUNCTIONS
#define SOCKET_FUNCTIONS

#include "script_data_transfer_objects.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <zmq.hpp>
#include <fstream>
#include <functional>

#include "debug_message.h"
#include "exaudflib/zmqcontainer.pb.h"
#include "script_data_transfer_objects_wrapper.h"
#include "external_process_check.h"

#include <mutex>
#define DONT_EXPOSE_SWIGVM_PARAMS
#include "exaudflib.h"
#undef DONT_EXPOSE_SWIGVM_PARAMS

using namespace SWIGVMContainers;
using namespace std;
using namespace google::protobuf;

static SWIGVM_params_t * SWIGVM_params_ref = nullptr;

extern "C" {
void set_SWIGVM_params(SWIGVM_params_t* p) {
    SWIGVM_params_ref = p;
}
}

static SWIGVMExceptionHandler exchandler;

//static exascript_vmtype vm_type;
static exascript_request request;
static exascript_response response;

static string output_buffer;

static string g_database_name;
static string g_database_version;
static string g_script_name;
static string g_script_schema;
static string g_current_user;
static string g_scope_user;
static string g_current_schema;
static string g_source_code;
static unsigned long long g_session_id;
static unsigned long g_statement_id;
static unsigned int g_node_count;
static unsigned int g_node_id;
static unsigned long long g_vm_id;
static bool g_singleCallMode;
static single_call_function_id_e g_singleCallFunction;
static ExecutionGraph::ImportSpecification g_singleCall_ImportSpecificationArg;
static ExecutionGraph::ExportSpecification g_singleCall_ExportSpecificationArg;
static ExecutionGraph::StringDTO g_singleCall_StringArg;


mutex zmq_socket_mutex;
static bool use_zmq_socket_locks = false;


void socket_send(zmq::socket_t &socket, zmq::message_t &zmsg)
{
    DBG_FUNC_BEGIN(std::cerr);
#ifdef LOG_COMMUNICATION
    stringstream sb;
    uint32_t len = zmsg.size();
    sb << "/tmp/zmqcomm_log_" << ::getpid() << "_send.data";
    int fd = ::open(sb.str().c_str(), O_CREAT | O_APPEND | O_WRONLY, 00644);
    if (fd >= 0) {
        if (::write(fd, &len, sizeof(uint32_t)) == -1 ) {perror("Log communication");}
        if (::write(fd, zmsg.data(), len) == -1) {perror("Log communication");}
        ::close(fd);
    }
#endif
    for (;;) {
        try {
            if (use_zmq_socket_locks) {
                zmq_socket_mutex.lock();
            }
            if (socket.send(zmsg) == true) {
                if (use_zmq_socket_locks) {
                    zmq_socket_mutex.unlock();
                }
                return;
            }
            external_process_check();
        } catch (std::exception &err) {
            external_process_check();
        } catch (...) {
            external_process_check();
        }
        if (use_zmq_socket_locks) {
            zmq_socket_mutex.unlock();
        }
        ::usleep(100000);
    }
    if (use_zmq_socket_locks) {
        zmq_socket_mutex.unlock();
    }
}

bool socket_recv(zmq::socket_t &socket, zmq::message_t &zmsg, bool return_on_error=false)
{
    DBG_FUNC_BEGIN(std::cerr);
    for (;;) {
        try {
            if (use_zmq_socket_locks) {
            zmq_socket_mutex.lock();
            }
            if (socket.recv(&zmsg) == true) {
#ifdef LOG_COMMUNICATION
                stringstream sb;
                uint32_t len = zmsg.size();
                sb << "/tmp/zmqcomm_log_" << ::getpid() << "_recv.data";
                int fd = ::open(sb.str().c_str(), O_CREAT | O_APPEND | O_WRONLY, 00644);
                if (fd >= 0) {
                    if (::write(fd, &len, sizeof(uint32_t)) == -1) {perror("Log communication");}
                    if (::write(fd, zmsg.data(), len) == -1) {perror("Log communication");}
                    ::close(fd);
                }
#endif
                if (use_zmq_socket_locks) {
                    zmq_socket_mutex.unlock();
                }
                return true;
            }
            external_process_check();
        } catch (std::exception &err) {
            external_process_check();

        } catch (...) {
            external_process_check();
        }
        if (use_zmq_socket_locks) {
            zmq_socket_mutex.unlock();
        }
        if (return_on_error) return false;
        ::usleep(100000);
    }
    if (use_zmq_socket_locks) {
        zmq_socket_mutex.unlock();
    }
    return false;
}

bool send_init(zmq::socket_t &socket, const string client_name)
{
    request.Clear();
    request.set_type(MT_CLIENT);
    request.set_connection_id(0);
    exascript_client *req = request.mutable_client();
    req->set_client_name(client_name);
    if (!request.SerializeToString(&output_buffer)) {
        exchandler.setException("Communication error: failed to serialize data");
        return false;
    }
    zmq::message_t zmsg((void*)output_buffer.c_str(), output_buffer.length(), NULL, NULL);
    socket_send(socket, zmsg);

    zmq::message_t zmsgrecv;
    response.Clear();
    if (!socket_recv(socket, zmsgrecv, true))
        return false;
    if (!response.ParseFromArray(zmsgrecv.data(), zmsgrecv.size())) {
        exchandler.setException("Failed to parse data");
        return false;
    }

    SWIGVM_params_ref->connection_id = response.connection_id();
#ifdef SWIGVM_LOG_CLIENT
    stringstream sb; sb << std::hex << SWIGVM_params_ref->connection_id;
    cerr << "### SWIGVM connected with id " << sb.str() << endl;
#endif
    if (response.type() == MT_CLOSE) {
        if (response.close().has_exception_message())
            exchandler.setException(response.close().exception_message().c_str());
        else exchandler.setException("Connection closed by server");
        return false;
    }
    if (response.type() != MT_INFO) {
        exchandler.setException("Wrong message type, should be MT_INFO");
        return false;
    }
    const exascript_info &rep = response.info();
    g_database_name = rep.database_name();
    g_database_version = rep.database_version();
    g_script_name = rep.script_name();
    g_script_schema = rep.script_schema();
    g_current_user = rep.current_user();
    g_scope_user = rep.scope_user();
    if (g_scope_user.size()==0) {         // for backward compatibility when testing with EXASOL 6.0.8 installations at OTTO Brain
        g_scope_user=g_current_user;
    }
    g_current_schema = rep.current_schema();
    g_source_code = rep.source_code();
    g_session_id = rep.session_id();
    g_statement_id = rep.statement_id();
    g_node_count = rep.node_count();
    g_node_id = rep.node_id();
    g_vm_id = rep.vm_id();
    //vm_type = rep.vm_type();


    SWIGVM_params_ref->maximal_memory_limit = rep.maximal_memory_limit();
    struct rlimit d;
    d.rlim_cur = d.rlim_max = rep.maximal_memory_limit();
    if (setrlimit(RLIMIT_RSS, &d) != 0)
#ifdef SWIGVM_LOG_CLIENT
        cerr << "WARNING: Failed to set memory limit" << endl;
#else
        throw SWIGVM::exception("Failed to set memory limit");
#endif
    d.rlim_cur = d.rlim_max = 0;    // 0 for no core dumps, RLIM_INFINITY to enable coredumps of any size
    if (setrlimit(RLIMIT_CORE, &d) != 0)
#ifdef SWIGVM_LOG_CLIENT
        cerr << "WARNING: Failed to set core limit" << endl;
#else
        throw SWIGVM::exception("Failed to set core limit");
#endif
    /* d.rlim_cur = d.rlim_max = 65536; */
    getrlimit(RLIMIT_NOFILE,&d);
    if (d.rlim_max < 32768)
    {
        //#ifdef SWIGVM_LOG_CLIENT
        cerr << "WARNING: Reducing RLIMIT_NOFILE below 32768" << endl;
        //#endif
    }
    d.rlim_cur = d.rlim_max = std::min(32768,(int)d.rlim_max);
    if (setrlimit(RLIMIT_NOFILE, &d) != 0)
#ifdef SWIGVM_LOG_CLIENT
        cerr << "WARNING: Failed to set nofile limit" << endl;
#else
        throw SWIGVM::exception("Failed to set nofile limit");
#endif
    d.rlim_cur = d.rlim_max = 32768;
    if (setrlimit(RLIMIT_NPROC, &d) != 0)
    {
        cerr << "WARNING: Failed to set nproc limit to 32k trying 8k ..." << endl;
        d.rlim_cur = d.rlim_max = 8192;
        if (setrlimit(RLIMIT_NPROC, &d) != 0)
#ifdef SWIGVM_LOG_CLIENT
            cerr << "WARNING: Failed to set nproc limit" << endl;
#else
            throw SWIGVM::exception("Failed to set nproc limit");
#endif
    }

    { /* send meta request */
        request.Clear();
        request.set_type(MT_META);
        request.set_connection_id(SWIGVM_params_ref->connection_id);
        if (!request.SerializeToString(&output_buffer)) {
            exchandler.setException("Communication error: failed to serialize data");
            return false;
        }
        zmq::message_t zmsg((void*)output_buffer.c_str(), output_buffer.length(), NULL, NULL);
        socket_send(socket, zmsg);
    } /* receive meta response */
    {   zmq::message_t zmsg;
        socket_recv(socket, zmsg);
        response.Clear();
        if (!response.ParseFromArray(zmsg.data(), zmsg.size())) {
            exchandler.setException("Communication error: failed to parse data");
            return false;
        }
        if (response.type() == MT_CLOSE) {
            if (response.close().has_exception_message())
                exchandler.setException(response.close().exception_message().c_str());
            else exchandler.setException("Connection closed by server");
            return false;
        }
        if (response.type() != MT_META) {
            exchandler.setException("Wrong message type, should be META");
            return false;
        }
        const exascript_metadata &rep = response.meta();
        g_singleCallMode = rep.single_call_mode();
        SWIGVM_params_ref->inp_iter_type = (SWIGVM_itertype_e)(rep.input_iter_type());
        SWIGVM_params_ref->out_iter_type = (SWIGVM_itertype_e)(rep.output_iter_type());
        for (int col = 0; col < rep.input_columns_size(); ++col) {
            const exascript_metadata_column_definition &coldef = rep.input_columns(col);
            SWIGVM_params_ref->inp_names->push_back(coldef.name());
            SWIGVM_params_ref->inp_types->push_back(SWIGVM_columntype_t());
            SWIGVM_columntype_t &coltype = SWIGVM_params_ref->inp_types->back();
            coltype.len = 0; coltype.prec = 0; coltype.scale = 0;
            coltype.type_name = coldef.type_name();
            switch (coldef.type()) {
            case PB_UNSUPPORTED:
                exchandler.setException("Unsupported column type found");
                return false;
            case PB_DOUBLE:
                coltype.type = DOUBLE;
                break;
            case PB_INT32:
                coltype.type = INT32;
                coltype.prec = coldef.precision();
                coltype.scale = coldef.scale();
                break;
            case PB_INT64:
                coltype.type = INT64;
                coltype.prec = coldef.precision();
                coltype.scale = coldef.scale();
                break;
            case PB_NUMERIC:
                coltype.type = NUMERIC;
                coltype.prec = coldef.precision();
                coltype.scale = coldef.scale();
                break;
            case PB_TIMESTAMP:
                coltype.type = TIMESTAMP;
                break;
            case PB_DATE:
                coltype.type = DATE;
                break;
            case PB_STRING:
                coltype.type = STRING;
                coltype.len = coldef.size();
                break;
            case PB_BOOLEAN:
                coltype.type = BOOLEAN;
                break;
            default:
                exchandler.setException("Unknown column type found");
                return false;
            }
        }
        for (int col = 0; col < rep.output_columns_size(); ++col) {
            const exascript_metadata_column_definition &coldef = rep.output_columns(col);
            SWIGVM_params_ref->out_names->push_back(coldef.name());
            SWIGVM_params_ref->out_types->push_back(SWIGVM_columntype_t());
            SWIGVM_columntype_t &coltype = SWIGVM_params_ref->out_types->back();
            coltype.len = 0; coltype.prec = 0; coltype.scale = 0;
            coltype.type_name = coldef.type_name();
            switch (coldef.type()) {
            case PB_UNSUPPORTED:
                exchandler.setException("Unsupported column type found");
                return false;
            case PB_DOUBLE:
                coltype.type = DOUBLE;
                break;
            case PB_INT32:
                coltype.type = INT32;
                coltype.prec = coldef.precision();
                coltype.scale = coldef.scale();
                break;
            case PB_INT64:
                coltype.type = INT64;
                coltype.prec = coldef.precision();
                coltype.scale = coldef.scale();
                break;
            case PB_NUMERIC:
                coltype.type = NUMERIC;
                coltype.prec = coldef.precision();
                coltype.scale = coldef.scale();
                break;
            case PB_TIMESTAMP:
                coltype.type = TIMESTAMP;
                break;
            case PB_DATE:
                coltype.type = DATE;
                break;
            case PB_STRING:
                coltype.type = STRING;
                coltype.len = coldef.size();
                break;
            case PB_BOOLEAN:
                coltype.type = BOOLEAN;
                break;
            default:
                exchandler.setException("Unknown column type found");
                return false;
            }
        }
    }
    return true;
}

void send_close(zmq::socket_t &socket, const string &exmsg)
{
    request.Clear();
    request.set_type(MT_CLOSE);
    request.set_connection_id(SWIGVM_params_ref->connection_id);
    exascript_close *req = request.mutable_close();
    if (exmsg != "") req->set_exception_message(exmsg);
    request.SerializeToString(&output_buffer);
    zmq::message_t zmsg((void*)output_buffer.c_str(), output_buffer.length(), NULL, NULL);
    socket_send(socket, zmsg);

    { /* receive finished response, so we know that the DB knows that we are going to close and
         all potential exceptions have been received on DB side */
        zmq::message_t zmsg;
        socket_recv(socket, zmsg);
        response.Clear();
        if(!response.ParseFromArray(zmsg.data(), zmsg.size()))
            throw SWIGVM::exception("Communication error: failed to parse data");
        else if (response.type() != MT_FINISHED)
            throw SWIGVM::exception("Wrong response type, should be finished");
    }
}

bool send_run(zmq::socket_t &socket)
{
    {
        /* send done request */
        request.Clear();
        request.set_type(MT_RUN);
        request.set_connection_id(SWIGVM_params_ref->connection_id);
        if (!request.SerializeToString(&output_buffer))
        {
            throw SWIGVM::exception("Communication error: failed to serialize data");
        }
        zmq::message_t zmsg((void*)output_buffer.c_str(), output_buffer.length(), NULL, NULL);
        socket_send(socket, zmsg);
    } { /* receive done response */
        zmq::message_t zmsg;
        socket_recv(socket, zmsg);
        response.Clear();
        if (!response.ParseFromArray(zmsg.data(), zmsg.size()))
            throw SWIGVM::exception("Communication error: failed to parse data");
        if (response.type() == MT_CLOSE) {
            if (response.close().has_exception_message())
                throw SWIGVM::exception(response.close().exception_message().c_str());
            throw SWIGVM::exception("Wrong response type, got empty close response");
        } else if (response.type() == MT_CLEANUP) {
            return false;
        } else if (g_singleCallMode && response.type() == MT_CALL) {
            assert(g_singleCallMode);
            exascript_single_call_rep sc = response.call();
            g_singleCallFunction = static_cast<single_call_function_id_e>(sc.fn());

            switch (g_singleCallFunction)
            {
            case single_call_function_id_e::SC_FN_NIL:
            case single_call_function_id_e::SC_FN_DEFAULT_OUTPUT_COLUMNS:
                break;
            case single_call_function_id_e::SC_FN_GENERATE_SQL_FOR_IMPORT_SPEC:
            {

                if (!sc.has_import_specification())
                {
                    throw SWIGVM::exception("internal error: SC_FN_GENERATE_SQL_FOR_IMPORT_SPEC without import specification");
                }
                const import_specification_rep& is_proto = sc.import_specification();
                g_singleCall_ImportSpecificationArg = ExecutionGraph::ImportSpecification(is_proto.is_subselect());
                if (is_proto.has_connection_information())
                {
                    const connection_information_rep& ci_proto = is_proto.connection_information();
                    ExecutionGraph::ConnectionInformation connection_info(ci_proto.kind(), ci_proto.address(), ci_proto.user(), ci_proto.password());
                    g_singleCall_ImportSpecificationArg.setConnectionInformation(connection_info);
                }
                if (is_proto.has_connection_name())
                {
                    g_singleCall_ImportSpecificationArg.setConnectionName(is_proto.connection_name());
                }
                for (int i=0; i<is_proto.subselect_column_specification_size(); i++)
                {
                    const ::exascript_metadata_column_definition& cdef = is_proto.subselect_column_specification(i);
                    const ::std::string& cname = cdef.name();
                    const ::std::string& ctype = cdef.type_name();
                    g_singleCall_ImportSpecificationArg.appendSubselectColumnName(cname);
                    g_singleCall_ImportSpecificationArg.appendSubselectColumnType(ctype);
                }
                for (int i=0; i<is_proto.parameters_size(); i++)
                {
                    const ::key_value_pair& kvp = is_proto.parameters(i);
                    g_singleCall_ImportSpecificationArg.addParameter(kvp.key(), kvp.value());
                }
            }
                break;
            case single_call_function_id_e::SC_FN_GENERATE_SQL_FOR_EXPORT_SPEC:
            {
                if (!sc.has_export_specification())
                {
                    throw SWIGVM::exception("internal error: SC_FN_GENERATE_SQL_FOR_EXPORT_SPEC without export specification");
                }
                const export_specification_rep& es_proto = sc.export_specification();
                g_singleCall_ExportSpecificationArg = ExecutionGraph::ExportSpecification();
                if (es_proto.has_connection_information())
                {
                    const connection_information_rep& ci_proto = es_proto.connection_information();
                    ExecutionGraph::ConnectionInformation connection_info(ci_proto.kind(), ci_proto.address(), ci_proto.user(), ci_proto.password());
                    g_singleCall_ExportSpecificationArg.setConnectionInformation(connection_info);
                }
                if (es_proto.has_connection_name())
                {
                    g_singleCall_ExportSpecificationArg.setConnectionName(es_proto.connection_name());
                }
                for (int i=0; i<es_proto.parameters_size(); i++)
                {
                    const ::key_value_pair& kvp = es_proto.parameters(i);
                    g_singleCall_ExportSpecificationArg.addParameter(kvp.key(), kvp.value());
                }
                g_singleCall_ExportSpecificationArg.setTruncate(es_proto.has_truncate());
                g_singleCall_ExportSpecificationArg.setReplace(es_proto.has_replace());
                if (es_proto.has_created_by())
                {
                    g_singleCall_ExportSpecificationArg.setCreatedBy(es_proto.created_by());
                }
                for (int i=0; i<es_proto.source_column_names_size(); i++)
                {
                    const string name = es_proto.source_column_names(i);
                    g_singleCall_ExportSpecificationArg.addSourceColumnName(name);
                }
            }
                break;
            case single_call_function_id_e::SC_FN_VIRTUAL_SCHEMA_ADAPTER_CALL:
                if (!sc.has_json_arg())
                {
                    throw SWIGVM::exception("internal error: SC_FN_VIRTUAL_SCHEMA_ADAPTER_CALL without json arg");
                }
                const std::string json = sc.json_arg();
                g_singleCall_StringArg = ExecutionGraph::StringDTO(json);
                break;
            }

            return true;
        } else if (response.type() != MT_RUN) {
            throw SWIGVM::exception("Wrong response type, should be MT_RUN");
        }
    }
    return true;
}

bool send_return(zmq::socket_t &socket, const char* result)
{
  assert(result != nullptr);
    {   /* send return request */
        request.Clear();
        request.set_type(MT_RETURN);
        ::exascript_return_req* rr = new ::exascript_return_req();
        rr->set_result(result);
        request.set_allocated_call_result(rr);
        request.set_connection_id(SWIGVM_params_ref->connection_id);
        if (!request.SerializeToString(&output_buffer))
            throw SWIGVM::exception("Communication error: failed to serialize data");
        zmq::message_t zmsg((void*)output_buffer.c_str(), output_buffer.length(), NULL, NULL);
        socket_send(socket, zmsg);
    } { /* receive return response */
        zmq::message_t zmsg;
        socket_recv(socket, zmsg);
        response.Clear();
        if (!response.ParseFromArray(zmsg.data(), zmsg.size()))
            throw SWIGVM::exception("Communication error: failed to parse data");
        if (response.type() == MT_CLOSE) {
            if (response.close().has_exception_message())
                throw SWIGVM::exception(response.close().exception_message().c_str());
            throw SWIGVM::exception("Wrong response type, got empty close response");
        } else if (response.type() == MT_CLEANUP) {
            return false;
        } else if (response.type() != MT_RETURN) {
            throw SWIGVM::exception("Wrong response type, should be MT_RETURN");
        }
    }
    return true;
}

void send_undefined_call(zmq::socket_t &socket, const std::string& fn)
{
    {   /* send return request */
        request.Clear();
        request.set_type(MT_UNDEFINED_CALL);
        ::exascript_undefined_call_req* uc = new ::exascript_undefined_call_req();
        uc->set_remote_fn(fn);
        request.set_allocated_undefined_call(uc);
        request.set_connection_id(SWIGVM_params_ref->connection_id);
        if (!request.SerializeToString(&output_buffer))
            throw SWIGVM::exception("Communication error: failed to serialize data");
        zmq::message_t zmsg((void*)output_buffer.c_str(), output_buffer.length(), NULL, NULL);
        socket_send(socket, zmsg);
    } { /* receive return response */
        zmq::message_t zmsg;
        socket_recv(socket, zmsg);
        response.Clear();
        if (!response.ParseFromArray(zmsg.data(), zmsg.size()))
            throw SWIGVM::exception("Communication error: failed to parse data");
        if (response.type() != MT_UNDEFINED_CALL) {
            throw SWIGVM::exception("Wrong response type, should be MT_UNDEFINED_CALL");
        }
    }
}


bool send_done(zmq::socket_t &socket)
{
    {   /* send done request */
        request.Clear();
        request.set_type(MT_DONE);
        request.set_connection_id(SWIGVM_params_ref->connection_id);
        if (!request.SerializeToString(&output_buffer))
            throw SWIGVM::exception("Communication error: failed to serialize data");
        zmq::message_t zmsg((void*)output_buffer.c_str(), output_buffer.length(), NULL, NULL);
        socket_send(socket, zmsg);
    } 
    { /* receive done response */
        zmq::message_t zmsg;
        socket_recv(socket, zmsg);
        response.Clear();
        if (!response.ParseFromArray(zmsg.data(), zmsg.size()))
            throw SWIGVM::exception("Communication error: failed to parse data");
        if (response.type() == MT_CLOSE) {
            if (response.close().has_exception_message())
                throw SWIGVM::exception(response.close().exception_message().c_str());
            throw SWIGVM::exception("Wrong response type, got empty close response");
        } else if (response.type() == MT_CLEANUP) {
            return false;
        } else if (response.type() != MT_DONE)
            throw SWIGVM::exception("Wrong response type, should be MT_DONE");
    }
    return true;
}

void send_finished(zmq::socket_t &socket)
{
    {   /* send done request */
        request.Clear();
        request.set_type(MT_FINISHED);
        request.set_connection_id(SWIGVM_params_ref->connection_id);
        if (!request.SerializeToString(&output_buffer))
            throw SWIGVM::exception("Communication error: failed to serialize data");
        zmq::message_t zmsg((void*)output_buffer.c_str(), output_buffer.length(), NULL, NULL);
        socket_send(socket, zmsg);
    } { /* receive done response */
        zmq::message_t zmsg;
        socket_recv(socket, zmsg);
        response.Clear();
        if(!response.ParseFromArray(zmsg.data(), zmsg.size()))
            throw SWIGVM::exception("Communication error: failed to parse data");
        if (response.type() == MT_CLOSE) {
            if (response.close().has_exception_message())
                throw SWIGVM::exception(response.close().exception_message().c_str());
            throw SWIGVM::exception("Wrong response type, got empty close response");
        } else if (response.type() != MT_FINISHED)
            throw SWIGVM::exception("Wrong response type, should be finished");
    }
}


#endif