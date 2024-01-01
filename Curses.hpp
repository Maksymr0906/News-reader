#include <curses.h>
#include <string>

class Curses {
public:
	Curses() {
		initscr();
	}

	~Curses() {
		endwin();
	}
};

inline int waddstr(WINDOW* window, const std::string& str) {
	return waddstr(window, str.c_str());
}