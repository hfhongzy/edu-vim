#ifndef ALAYAVIM_UTILITY_H
#define ALAYAVIM_UTILITY_H

#include <iostream>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <vector>
#include <string>
#include <cstdio>
#include <chrono>
#include <fstream>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <algorithm>

constexpr char ESC = 27;
constexpr char ENTER = 10;
constexpr char BACKSPACE = 127;
constexpr char TAB = 9;
constexpr char REDO = 18;

constexpr int UNDO_REDO_INTERVAL = 500;

enum class programState {
  Normal = 0,
  Insert = 1,
  Command = 2,
};

enum class direction {
  UP = 0,
  DOWN = 1,
  LEFT = 2,
  RIGHT = 3,
};

enum class atomType {
  MODIFY = 0,
  DELETE = 1,
  INSERT = 2,
};


int nonblock(int fd, bool undo = false);
void config_set(struct termios &oldt, struct termios &newt);
void config_reset(struct termios &oldt);
std::string align_num(int x, int width);

namespace ANSI {
  std::string grey(const std::string &s);
  std::string red(const std::string &s);
  std::string cyan(const std::string &s);
  std::string purple(const std::string &s);
  std::string clearScreen();
  std::string clearBuffer();
  std::string cursorPosition(int x, int y);
  std::string backspace();
}

#endif //ALAYAVIM_UTILITY_H
