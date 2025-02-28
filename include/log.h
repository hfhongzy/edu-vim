#ifndef ALAYAVIM_LOG_H
#define ALAYAVIM_LOG_H

#include <chrono>
#include <string>

#include "utility.h"
class Log {
protected:
  std::chrono::time_point<std::chrono::high_resolution_clock> timestamp;
public:
  Log() {
    timestamp = std::chrono::high_resolution_clock::now();
  }
  friend size_t duration(const Log &a, const Log &b) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(b.timestamp - a.timestamp).count();
  }
  virtual ~Log() = default;
};

class LogContent: public Log {
public:
  atomType type;
  int posX;
  std::string oldContent, newContent;

  LogContent(const atomType &type, const int &posX, const std::string &oldContent, const std::string &newContent):
          type(type), posX(posX), oldContent(oldContent), newContent(newContent) {}
};
class LogCursor: public Log {
public:
  int oldX, oldY;
  int newX, newY;
  LogCursor(int oldX, int oldY, int newX, int newY):
          oldX(oldX), oldY(oldY), newX(newX), newY(newY) {}
};

#endif //ALAYAVIM_LOG_H
