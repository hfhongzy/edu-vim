#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <cstdio>
#include <sstream>
#include <iomanip>

#include "utility.h"

int nonblock(int fd, bool undo) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    perror("fcntl");
    return -1;
  }
  if (undo) {
    flags &= ~O_NONBLOCK;
  } else {
    flags |= O_NONBLOCK;
  }
  if (fcntl(fd, F_SETFL, flags) == -1) {
    perror("fcntl");
    return -1;
  }

  return 0;
}
void config_set(struct termios &oldt, struct termios &newt) {
  tcgetattr(STDIN_FILENO, &oldt);  // terminal configuration
  newt = oldt;
  newt.c_lflag &= ~ICANON;  // buffer forbidden
  newt.c_lflag &= ~ECHO;    // echo forbidden
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);  // new configuration
}
void config_reset(struct termios &oldt) {
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // recover
}

std::string align_num(int x, int width) {
  std::ostringstream oss;
  oss << std::setw(width) << std::right << x;
  return oss.str();
}
namespace ANSI {
  std::string grey(const std::string &s) {
    return "\033[90m" + s + "\033[0m";
  }
  std::string red(const std::string &s) {
    return "\033[91m" + s + "\033[0m";
  }
  std::string cyan(const std::string &s) {
    return "\033[96m" + s + "\033[0m";
  }
  std::string purple(const std::string &s) {
    return "\033[95m" + s + "\033[0m";
  }
  std::string clearScreen() {
    return "\033[2J";
  }
  std::string clearBuffer() {
//    return "";
    return "\033[3J";
  }
  std::string cursorPosition(int x, int y) {
    return "\033[" + std::to_string(x) + ";" + std::to_string(y) + "H";
  }
  std::string backspace() {
    return "\033[D \033[D";
  }
}
