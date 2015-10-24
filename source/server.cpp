//
// Created by kamlesh on 17/10/15.
//

#include <stdio.h>
#include <signal.h>
#include <iostream>
#include <sstream>
#include <assert.h>
#include "http_handler.h"
#include "cache.h"
#include <http_parser.h>
#include "server.h"
#include "http_context.h"

#define MAX_WRITE_HANDLES 1000


using namespace pigeon;


uv_loop_t* uv_loop;
uv_tcp_t uv_tcp;
http_parser_settings parser_settings;

function<void(uv_stream_t *socket, int status)> server::on_connect = nullptr;
function<void(uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf)> server::on_read = nullptr;
function<void(uv_work_t *req)> server::on_render = nullptr;
function<void(uv_work_t *req)> server::on_render_complete = nullptr;
function<void(uv_write_t *req, int status)> server::on_send_complete = nullptr;
function<void(uv_handle_t *handler)> server::on_close = nullptr;




typedef struct {

    uv_tcp_t handle;
    http_parser parser;
    uv_write_t write_req;
    http_context* context;

} client_t;

typedef struct {

    uv_work_t request;
    client_t* client;
    bool error;
    char* result;
    unsigned long length;

} msg_baton_t;

server::server() { }

server::~server() { }

void server::start() {

    m_settings = new settings;
    m_cache = new cache;

    m_settings->load_setting();

    m_log_file = m_settings->get_log_location();

    signal(SIGPIPE, SIG_IGN);

    setup_thread_pool();

    uv_loop = uv_default_loop();

    m_cache->load(m_settings->get_resource_location());

    initialise_parser();
    initialise_tcp();

    int r = uv_tcp_init(uv_loop, &uv_tcp);

    if(r != 0){
        logger::get(m_settings)->write(LogType::Error, Severity::Critical, uv_err_name(r));
    }

    r = uv_tcp_keepalive(&uv_tcp,1,60);
    if(r != 0){
        logger::get(m_settings)->write(LogType::Error, Severity::Critical, uv_err_name(r));
    }

    string _address = m_settings->get_address();
    int _port = m_settings->get_port();

    struct sockaddr_in address;
    r = uv_ip4_addr(_address.c_str(), _port, &address);
    if(r != 0){
        logger::get(m_settings)->write(LogType::Error, Severity::Critical, uv_err_name(r));
    }

    r = uv_tcp_bind(&uv_tcp, (const struct sockaddr*)&address, 0);
    if(r != 0){
        logger::get(m_settings)->write(LogType::Error, Severity::Critical, uv_err_name(r));
    }

    r = uv_listen((uv_stream_t*)&uv_tcp, MAX_WRITE_HANDLES,  [](uv_stream_t *socket, int status) {
        on_connect(socket, status);
    });

    if(r != 0){
        logger::get(m_settings)->write(LogType::Error, Severity::Critical, uv_err_name(r));
    }

    logger::get(m_settings)->write(LogType::Information, Severity::Medium, "Server Started...");

    uv_run(uv_loop, UV_RUN_DEFAULT);


}

void server::stop() {

    uv_stop(uv_loop);

}

void server::setup_thread_pool() {

    stringstream ss;
    ss << m_settings->get_num_worker_threads();
    setenv("UV_THREADPOOL_SIZE", ss.str().c_str(), 1);

}

///setup all the callbacks bu assign lambda expressions to the std::function
void server::initialise_tcp() {

    on_close = [&](uv_handle_t *handle) {

        try {

            client_t* client = (client_t*) handle->data;
            delete client->context;
            free(client);

        } catch (std::exception ex) {

        }


    };

    on_send_complete = [&](uv_write_t *req, int status) {

        try {

            if(status != 0){
                logger::get(m_settings)->write(LogType::Error, Severity::Critical, uv_err_name(status));
            }
            if (!uv_is_closing((uv_handle_t*)req->handle))
            {
                msg_baton_t *closure = static_cast<msg_baton_t *>(req->data);
                delete closure;
                uv_close((uv_handle_t*)req->handle, [](uv_handle_t *handle){
                    on_close(handle);
                });
            }

        } catch (std::exception& ex){

        }

    };


    on_render_complete = [&](uv_work_t *req) {

        try {

            msg_baton_t *closure = static_cast<msg_baton_t *>(req->data);
            client_t* client = (client_t*) closure->client;


            uv_buf_t resbuf;
            resbuf.base = closure->result;
            resbuf.len = (size_t)closure->length;

            client->write_req.data = closure;

            int r = uv_write(&client->write_req,
                             (uv_stream_t*)&client->handle,
                             &resbuf,
                             1,
                             [](uv_write_t *req, int status) {
                                 on_send_complete(req, status);
                             });

            if(r != 0){
                logger::get(m_settings)->write(LogType::Error, Severity::Critical, uv_err_name(r));
            }

        } catch (std::exception& ex) {

        }

    };

    on_render = [&](uv_work_t *req) {

        try {

            msg_baton_t *closure = static_cast<msg_baton_t *>(req->data);
            client_t* client = (client_t*) closure->client;

            process(client->context);

            closure->result = (char*) client->context->response->message.c_str();
            closure->length = client->context->response->message.size();

        } catch (std::exception& ex) {

        }

    };

    on_read = [&](uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf){

        try {

            ssize_t parsed;
            client_t *client = (client_t *) tcp->data;
            if (nread >= 0) {
                parsed = (ssize_t) http_parser_execute(
                        &client->parser, &parser_settings, buf->base, nread);
                if (parsed < nread) {
                    logger::get(m_settings)->write(LogType::Error, Severity::Critical, "parse failed");
                    uv_close((uv_handle_t *) &client->handle, [](uv_handle_t *handle){
                        on_close(handle);
                    });
                }
            } else {
                if (nread != UV_EOF) {
                    logger::get(m_settings)->write(LogType::Error, Severity::Critical, "read failed");
                }
                uv_close((uv_handle_t *) &client->handle, [](uv_handle_t *handle){
                    on_close(handle);
                });
            }
            free(buf->base);
        }
        catch(std::exception& ex){

            throw std::runtime_error(ex.what());

        }
    };

    on_connect = [&](uv_stream_t *server_handle, int status) {

        try {

            assert((uv_tcp_t*)server_handle == &uv_tcp);

            client_t* client = (client_t*)malloc(sizeof(client_t));

            client->context = new http_context;
            client->context->Settings = m_settings;
            client->context->Cache = m_cache;


            uv_tcp_init(uv_loop, &client->handle);
            http_parser_init(&client->parser, HTTP_REQUEST);

            client->parser.data = client;
            client->handle.data = client;

            int r = uv_accept(server_handle, (uv_stream_t*)&client->handle);
            if(r != 0){
                logger::get(m_settings)->write(LogType::Error, Severity::Critical, uv_err_name(r));
            }
            uv_read_start((uv_stream_t*)&client->handle,
                          [&](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
                              *buf = uv_buf_init((char *)malloc(suggested_size), suggested_size);
                          },
                          [&](uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf) {
                              on_read(tcp, nread, buf);
                          });


        } catch (std::exception& ex){

        }

    };

}

void server::initialise_parser() {


    parser_settings.on_url = [](http_parser* parser, const char* at, size_t len) -> int {
        client_t* client = (client_t*) parser->data;
        if (at && client->context->request) {
            string s(at, len);
            client->context->request->url = s;
        }
        return 0;
    };

    parser_settings.on_header_field = [](http_parser* parser, const char* at, size_t len) -> int {

                client_t* client = (client_t*) parser->data;
                if (at && client->context->request) {
                    string s(at, len);
                    client->context->request->set_header_field(s);
                }
                return 0;

    };

    parser_settings.on_header_value = [](http_parser* parser, const char* at, size_t len) -> int {

                client_t* client = (client_t*) parser->data;
                if (at && client->context->request) {
                    string s(at, len);
                    client->context->request->set_header_value(s);
                }
                return 0;

    };

    parser_settings.on_headers_complete = [](http_parser* parser) -> int {

                client_t* client = (client_t*) parser->data;
                client->context->request->method = parser->method;
                client->context->request->http_major_version = parser->http_major;
                client->context->request->http_minor_version = parser->http_minor;
                return 0;

    };

    parser_settings.on_body = [](http_parser* parser, const char* at, size_t len) -> int {

                client_t* client = (client_t*) parser->data;
                if (at && client->context->request) {
                    string s(at, len);
                    client->context->request->content = s;
                }
                return 0;

    };

    parser_settings.on_message_complete = [](http_parser* parser) -> int {

        client_t* client = (client_t*) parser->data;
        msg_baton_t *closure = new msg_baton_t();
        closure->request.data = closure;
        closure->client = client;
        closure->error = false;

        client->context->request->is_api = is_api(client->context->request->url);
        parse_query_string(*client->context->request);

        int status = uv_queue_work(uv_loop,
                                   &closure->request,
                                   [](uv_work_t *req) { on_render(req);},
                                   [](uv_work_t *req, int status) { on_render_complete(req);});

    };



}

