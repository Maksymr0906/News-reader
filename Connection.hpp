#pragma once

#include "Curses.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <vector>

namespace io = boost::asio;
using tcp = io::ip::tcp;
using error_code = boost::system::error_code;
using resolver_results = tcp::resolver::results_type;

enum class Status {
    NONE = 0,
    CAPABILITY_LIST_FOLLOWS = 101,
    SERVICE_AVAILABLE_POSTING_ALLOWED = 200,
    SERVICE_AVAILABLE_POSTING_PROHIBITED = 201,
    CLOSING_CONNECTION = 205,
    BEGIN_TLS_NEGOTIATION = 382,
    SERVICE_TEMPORARILY_UNAVAILABLE = 400,
    SERVICE_PERMANENTLY_UNAVAILABLE = 502
};

enum class Command {
    NONE = 0,
    CAPABILITIES
};

class Connection {
private:
    std::string name;
    WINDOW* window;
    io::streambuf buffer;
    io::ssl::stream<tcp::socket> socket;
    std::string command;
    bool using_TLS;
    bool is_data_returns;
    Status status;
    Status expected_status;
    std::vector<std::string> data_block;

    std::string get_line();
    std::string update_status();
    std::string get_command() const;
    void display_status(const std::string& text);
    void receive_line(std::function<void(const error_code& error, size_t bytes_transferred)> handler);
    void send_line(std::function<void(const error_code& error, size_t bytes_transferred)> handler);
    void send_command(const std::string& command, Status expected_status, bool is_data_returns);
    void handle_command_written(const error_code& error, size_t bytes_transferred);
    void handle_command_response(const error_code& error, size_t bytes_transferred);
    void handle_connect(const error_code& error, const tcp::endpoint& endpoint);
    void handle_greeting(const error_code& error, size_t bytes_transferred);
    void handle_data_block_line(const error_code& error, size_t bytes_transferred);
    void process_data_block();
    void process_command_success();
    void handle_TLS_handshake(const error_code& error);
public:
    Connection(const std::string& server_name, WINDOW* window, io::io_context& io, io::ssl::context& ssl_context, const resolver_results &results);
};