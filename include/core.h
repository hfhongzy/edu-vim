#ifndef ALAYAVIM_CORE_H
#define ALAYAVIM_CORE_H

#include "utility.h"
#include "filemanager.h"

class Core {

  programState state = programState::Normal;
  std::vector<FileManager> buffer;
  char lastChar = 0;
  int currentFile = 0;

public:
  bool end = false;
  int returnCode = 0;

  Core(const std::vector<std::string> &files);
  void save();
  int saveAll();
  void handleESC();
  void exit(int code);
  void handleTAB();
  void handleBACKSPACE();

  std::string command;
  void clearPrompt();
  bool validReplace(std::string command, std::pair<int, int> &info);
  void handleREDO();
  void handleUNDO();
  void handleENTER();
  void handle(direction ch);
  void handle(char ch);
};

#endif //ALAYAVIM_CORE_H
