#include "Reader.hpp"

void Reader::add_server(const std::string& server_name) {
	int height{}, width{};
	getmaxyx(stdscr, height, width);
	const int LINES_PER_WINDOW = (height - 1) / 3;
	const int START_Y = connecting * LINES_PER_WINDOW;
	connecting++;

	WINDOW* window = newwin(LINES_PER_WINDOW, 0, START_Y, 0);
	scrollok(window, TRUE);
	waddstr(window, "Resolving server " + server_name + "...\n");
	wrefresh(window);

	resolver.async_resolve(server_name, "nntp",
		[this, server_name, window](const error_code& error, const resolver_results& results) {
			handle_resolve(error, results, server_name, window);
		}
	);
}

void Reader::handle_resolve(const error_code& error, const resolver_results& results,
							const std::string& server_name, WINDOW* window) {
	if (error) {
		connecting--;
		waddstr(window, "Error " + error.what() + " resolving server " + server_name + '\n');
		refresh();
		return;
	}

	connections.push_back(std::make_shared<Connection>(server_name, window, io_context, ssl_context, results));
}