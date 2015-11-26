//
// Created by kamlesh on 23/11/15.
//

#include "http_connection.h"
#include <vector>
#include <http_msg_parser.h>

using namespace pigeon::tcp;

http_connection::http_connection(asio::io_context& io_context)
: client(io_context){
	init_parser();
	request = make_shared<http_request>();
}

asio::ip::tcp::socket&http_connection::socket() {
	return client;
}


void http_connection::init_parser() {

	parser = (http_parser*)malloc(sizeof(http_parser));
	http_parser_init(parser, HTTP_REQUEST);
	std::memset(&parser_settings, 0, sizeof(parser_settings));

	parser_settings.on_url = [](http_parser* parser, const char* at, size_t len) -> int {

		http_connection* con = static_cast<http_connection*>(parser->data);
		if (at && con->request) {
			char *data = (char *)malloc(sizeof(char)* len + 1);
			strncpy(data, at, len);
			data[len] = '\0';
			con->request->url += data;
			free(data);
		}
		return 0;

	};

	parser_settings.on_header_field = [](http_parser* parser, const char* at, size_t len) -> int {

		http_connection* con = static_cast<http_connection*>(parser->data);
		if (at && con->request) {
			string s;
			char *data = (char *)malloc(sizeof(char)* len + 1);
			strncpy(data, at, len);
			data[len] = '\0';
			s += data;
			free(data);

			con->request->set_header_field(s);
		}
		return 0;

	};

	parser_settings.on_header_value = [](http_parser* parser, const char* at, size_t len) -> int {

		http_connection* con = static_cast<http_connection*>(parser->data);
		if (at && con->request) {
			string s;
			char *data = (char *)malloc(sizeof(char)* len + 1);
			strncpy(data, at, len);
			data[len] = '\0';
			s += data;
			free(data);
			con->request->set_header_value(s);
		}
		return 0;

	};

	parser_settings.on_headers_complete = [](http_parser* parser) -> int {

		http_connection* con = static_cast<http_connection*>(parser->data);
		con->request->method = parser->method;
		con->request->http_major_version = parser->http_major;
		con->request->http_minor_version = parser->http_minor;
		return 0;

	};

	parser_settings.on_body = [](http_parser* parser, const char* at, size_t len) -> int {

		http_connection* con = static_cast<http_connection*>(parser->data);
		if (at && con->request) {

			char *data = (char *)malloc(sizeof(char)* len + 1);
			strncpy(data, at, len);
			data[len] = '\0';
			con->request->content += data;
			free(data);

		}
		return 0;

	};

	parser_settings.on_message_complete = [](http_parser* parser) -> int {

		http_connection* con = static_cast<http_connection*>(parser->data);

		con->do_write();

		return 0;

	};
}

void http_connection::parse_request(size_t nread) {
	
	size_t parsed;
	parsed = (size_t)http_parser_execute(parser, &parser_settings, buffer.data(), nread);

}

void http_connection::process_request() {
	

	 
}

void http_connection::do_write() {

	asio::async_write(client, asio::buffer(response->message),
		[this](std::error_code ec, std::size_t /*length*/)
	{
		if (!ec)
		{
			client.close();
		}
	});

}


void http_connection::do_read()
{
    auto self(shared_from_this());
    socket().async_read_some(asio::buffer(buffer),
                            [this, self](std::error_code ec, std::size_t bytes_transferred)
                            {
                                if (!ec)
                                {
                                    parser->data = (void*)this;
                                    parse_request(bytes_transferred);
                                }
                                else if (ec != asio::error::operation_aborted)
                                {

                                }
                            });
}


