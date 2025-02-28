#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <cstdio>
#include <sstream>

#include "utility.h"
#include "core.h"

void routine(Core &core) {
  while (true) {
    char ch = getchar();
    core.clearPrompt();

    if (ch == REDO) {
      core.handleREDO();
    } else if (ch == TAB) {
      core.handleTAB();
    } else if (ch == ESC) {
      nonblock(STDIN_FILENO);
      // Distinguish ESC and ESC + [ + A
      char ch2 = getchar();
      char ch3 = getchar();
      if (ch2 == '[' && ch3 >= 'A' && ch3 <= 'D') {
        switch (ch3) {
          case 'A':
            core.handle(direction::UP);
            break;
          case 'B':
            core.handle(direction::DOWN);
            break;
          case 'C':
            core.handle(direction::RIGHT);
            break;
          case 'D':
            core.handle(direction::LEFT);
            break;
          default:
            break;
        }
      } else {
        core.handleESC();
      }
      nonblock(STDIN_FILENO, true);
    } else if (ch == ENTER) {
      core.handleENTER();
    } else if (ch == BACKSPACE) {
      core.handleBACKSPACE();
    } else {
      core.handle(ch);
    }
    if (core.end) {
      break;
    }
  }
}
int main(int argc, char const *argv[]) {
  std::vector<std::string> fileContent;
  if (argc <= 1) {
    std::cerr << "Please open at least one file." << std::endl;
    return 1;
  }
  std::vector<std::string> files;
  for (int i = 1; i < argc; ++i) {
    files.emplace_back(argv[i]);
  }
  Core core(files);

  struct termios oldt, newt;
  config_set(oldt, newt);

  routine(core);

  config_reset(oldt);
  return core.returnCode;
}
