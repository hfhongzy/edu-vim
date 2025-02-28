#ifndef ALAYAVIM_FILEMANAGER_H
#define ALAYAVIM_FILEMANAGER_H

#include <vector>
#include <string>
#include <memory>

#include "log.h"

class FileManager {
private:
  const std::string filename;
  std::shared_ptr<std::vector<std::string>> content;
  std::vector<std::unique_ptr<Log>> log;

  std::string prompt;
  std::string board;

  bool ephemeral = false;
  bool saved = true;
  bool numbered = false;

  int posX = 0;
  int posY = 0;
  int terminalHeight = 0;
  int terminalWidth = 0;
  int windowStartX = 0;
  int lineWidth = 0;
  int width = 0;
  int where = 0; // log[where]

  void getTerminalSize();

  void splitLine(const std::string &line, std::vector<std::string> &output, int lineid) const;

public:
  FileManager(const std::vector<std::string> &fileContent,
              std::string name);
  FileManager(FileManager &&other) noexcept;

  static void clearTerminal() {
    printf("%s%s", ANSI::clearScreen().c_str(), ANSI::clearBuffer().c_str());
    printf("%s", ANSI::cursorPosition(1, 1).c_str());
    fflush(stdout);
  }

  bool isSaved() const;
  void setNumber();
  void setNoNumber();
  void commitModify(int pos, const std::string &newContent);
  void commitInsert(int pos, const std::string &newContent);
  void commitDelete(int pos);
  void undo(const std::unique_ptr<Log> &log_);
  void redo(const std::unique_ptr<Log> &log_);
  bool undo();
  bool redo();

  void setPrompt(const std::string &p, bool e = false);
  void updateCommandDisplay() const;
  void display();
  void moveCursor(direction d);
  void toLastChar();
  void toNextLine(bool logFlag = false);
  void toLineFront();
  void toLineEnd();
  void toLastLine();
  void toFirstLine();
  bool jumpTo(int line);
  void enter();
  void backspace();
  void deleteLine();
  void copyLine();
  void pasteLine();
  void insertChar(char c);
  std::string replace_str(const std::string &s, const std::string &pattern, const std::string &replacement, int &occurs, int row);
  std::pair<int, int> replace(const std::string &pattern, const std::string &replacement, bool inFile);
  std::string fileInfo();
  void save(bool print = false);
  void clearPrompt();
  void openPrompt();
  void filePrompt();
};

#endif //ALAYAVIM_FILEMANAGER_H