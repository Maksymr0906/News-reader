#include "Reader.hpp"

WINDOW* createCommandWindow()
{
    int width;
    int height;
    getmaxyx(stdscr, height, width);
    return newwin(1, 0, height - 1, 0);
}

int main() {
    io::io_context io;
    Curses curses;
    Reader reader{ io };

    const std::vector<std::string> news_servers {
        "news.eternal-september.org", "news.gmane.io", "news.xmission.com"
    };

    for (const auto& server : news_servers) {
        reader.add_server(server);
    }
    WINDOW* commandWin = createCommandWindow();
    waddstr(commandWin, "Press a key to exit.");
    wrefresh(commandWin);
    io.run();
    wgetch(commandWin);
}