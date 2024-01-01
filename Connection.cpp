#include "Connection.hpp"

Connection::Connection(const std::string& server_name, WINDOW* window, io::io_context& io, io::ssl::context& ssl_context, const resolver_results& results)
	:name{ server_name }, window{ window }, socket{ io, ssl_context }, using_TLS{}, is_data_returns{} {
	display_status("Connecting");
	io::async_connect(socket.lowest_layer(), results, [this](const error_code& error, const tcp::endpoint& ep) {
			handle_connect(error, ep);
		}
	);
}

std::string Connection::get_command() const {
	return command.substr(0, command.size() - 2);
}

void Connection::display_status(const std::string& text) {
	waddstr(window, name + (using_TLS ? "[S]: " : ": ") + text + '\n');
	wrefresh(window);
	napms(200);
}

std::string Connection::get_line() {
	std::string  line;
	std::istream str(&buffer);
	std::getline(str, line);
	line.pop_back();
	return line;
}

std::string Connection::update_status() {
	std::string line = get_line();
	const int status = (line.at(0) - '0') * 100 + (line.at(1) - '0') * 10 + (line.at(2) - '0');
	this->status = static_cast<Status>(status);
	return line;
}

void Connection::receive_line(std::function<void(const error_code& error, size_t bytes_transferred)> handler) {
	if (using_TLS) {
		io::async_read_until(socket, buffer, '\r\n', handler);
	}
	else {
		io::async_read_until(socket.next_layer(), buffer, '\r\n', handler);
	}
}

void Connection::send_line(std::function<void(const error_code& error, size_t bytes_transferred)> handler) {
	if (using_TLS) {
		io::async_write(socket, io::buffer(command), handler);
	}
	else {
		io::async_write(socket.next_layer(), io::buffer(command), handler);
	}
}

void Connection::handle_connect(const error_code& error, const tcp::endpoint& endpoint) {
	if (error) {
		display_status("Error " + error.what() + " connecting to server " + name);
		return;
	}

	display_status("Connected");
	receive_line([this](const error_code& error, size_t bytes_transferred) {handle_greeting(error, bytes_transferred); });
}

void Connection::handle_greeting(const error_code& error, size_t bytes_transferred) {
	if (error) {
		display_status("Error: " + error.what() + " greeting server " + name);
		return;
	}

	display_status(update_status());
	if (status == Status::SERVICE_AVAILABLE_POSTING_ALLOWED || status == Status::SERVICE_AVAILABLE_POSTING_PROHIBITED) {
		send_command("CAPABILITIES", Status::CAPABILITY_LIST_FOLLOWS, true);
	}
}

void Connection::send_command(const std::string& command, Status expected_status, bool is_data_returns) {
	this->command = command + "\r\n";
	this->expected_status = expected_status;
	this->is_data_returns = is_data_returns;
	data_block.clear();
	send_line([this](const error_code& error, size_t bytes_transferred) {handle_command_written(error, bytes_transferred); });
}

void Connection::handle_command_written(const error_code& error, size_t bytes_transferred) {
	if (error) {
		display_status("Error: " + error.what() + " sending " + get_command() + " command");
		return;
	}

	display_status("Sending " + get_command() + " command");
	receive_line([this](const error_code& ec, size_t len) { handle_command_response(ec, len); });
}

void Connection::handle_command_response(const error_code& error, size_t bytes_transferred) {
	if (error) {
		display_status("Error: " + error.what() + " reading status for " + name);
		return;
	}

	display_status(update_status());
	if (status != expected_status) {
		display_status("Error: status != expected status");
		return;
	}

	if (is_data_returns) {
		receive_line([this](const error_code& error, size_t bytes_transferred) {handle_data_block_line(error, bytes_transferred); });
	}
	else {
		process_command_success();
	}
}

void Connection::handle_data_block_line(const error_code& error, size_t bytes_transferred) {
	if (error) {
		display_status("Error: " + error.what() + " reading data block line.");
		return;
	}

	const std::string line = get_line();
	if (line != ".") {
		display_status(line);
		data_block.push_back(line);
		receive_line([this](const error_code& error, size_t bytes_transferred) {handle_data_block_line(error, bytes_transferred); });
	}
	else {
		process_data_block();
	}
}

void Connection::process_data_block() {
	if (get_command() == "CAPABILITIES") {
		if (using_TLS) {
			send_command("QUIT", Status::CLOSING_CONNECTION, false);
		}
		else {
			bool is_supports_TLS{};
			for (const auto& line : data_block) {
				if (line == "STARTTLS") {
					is_supports_TLS = true;
					break;
				}
			}

			if (is_supports_TLS) {
				send_command("STARTTLS", Status::BEGIN_TLS_NEGOTIATION, false);
			}
		}
	}
}

void Connection::process_command_success() {
	if (get_command() == "STARTTLS") {
		socket.async_handshake(io::ssl::stream_base::client, [this](const error_code& error) {handle_TLS_handshake(error); });
	}
}

void Connection::handle_TLS_handshake(const error_code& error) {
	if (error) {
		display_status("Error:  " + error.what() + " negotiating TLS " + name);
		return;
	}

	using_TLS = true;
	send_command("CAPABILITIES", Status::CAPABILITY_LIST_FOLLOWS, true);
}