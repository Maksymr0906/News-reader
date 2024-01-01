#pragma once

#include "Connection.hpp"
#include <memory>

class Reader {
private:
	io::io_context &io_context;
	io::ssl::context ssl_context;
	tcp::resolver resolver;
	std::vector<std::shared_ptr<Connection>> connections;
	int connecting;

	void handle_resolve(const error_code& error, const resolver_results& results,
						const std::string& server, WINDOW* window);
public:
	Reader(io::io_context& io) 
		:io_context{ io }, resolver{ io_context }, 
		 connecting{}, ssl_context{io::ssl::context::method::tlsv12_client} {}

	void add_server(const std::string& server_name);
};

