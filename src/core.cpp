#include "core.h"
#include <vector>
#include <string>

Core::Core(const std::vector<std::string> &files) {
  buffer.clear();
  for (const auto &file : files) {
    std::vector<std::string> content;
    std::ifstream in(file);
    if (!in.is_open()) {
      content.emplace_back("");
    } else {
      std::string line;
      while (std::getline(in, line)) {
        content.emplace_back(line);
      }
    }
    buffer.emplace_back(content, file);
  }
  buffer.front().display();
}
void Core::save() {
  buffer[currentFile].save(true);
}
int Core::saveAll() {
  int modified = 0;
  for (auto &file: buffer) {
    if (!file.isSaved()) {
      modified++;
      file.save(false);
    }
  }
  return modified;
}
void Core::handleESC() {
  lastChar = 0;
  switch (state) {
    case programState::Normal:
      buffer[currentFile].setPrompt(ANSI::purple("[Hint] Type :q to quit"), true);
      break;
    case programState::Command:
      break;
    case programState::Insert:
      state = programState::Normal;
      buffer[currentFile].setPrompt(ANSI::cyan("[NORMAL]"), true);
      break;
  }
}
void Core::exit(int code) {
  FileManager::clearTerminal();
  end = true;
  returnCode = code;
}
void Core::handleTAB() {
  lastChar = 0;
  constexpr int tabSize = 4;
  for (int i = 0; i < tabSize; ++i)
    buffer[currentFile].insertChar(' ');
}
void Core::handleBACKSPACE() {
  lastChar = 0;
  switch (state) {
    case programState::Normal:
      buffer[currentFile].toLastChar();
      break ;
    case programState::Command:
      if (command.empty()) {
        state = programState::Normal;
        buffer[currentFile].setPrompt("");
      } else {
        command.pop_back();
        printf("%s", ANSI::backspace().c_str());
      }
      break;
    case programState::Insert:
      buffer[currentFile].backspace();
      break;
  }
}
void Core::clearPrompt() {
  if (state != programState::Command) {
    buffer[currentFile].clearPrompt();
  }
}
bool Core::validReplace(std::string command, std::pair<int, int> &info) {
  if (command.size() < 5) return false;
  bool inFile = command[0] == '%';
  if (inFile) {
    command = command.substr(1);
  }
  if (command.size() < 5)
    return false;
  if (command[0] != 's')
    return false;
  if (command.back() != 'g')
    return false;
  if (command[1] != '/' || command[(int)(command.size()) - 2] != '/') return false;
  int i = 2;
  while (i + 2 < command.size() && command[i] != '/') {
    i ++;
  }
  if (i + 2 >= command.size()) return false;
  std::string pattern = command.substr(2, i - 2);
  std::string replacement = command.substr(i + 1, (int)(command.size()) - i - 3);
  info = buffer[currentFile].replace(pattern, replacement, inFile);
  return true;
}
void Core::handleREDO() {
  auto res = buffer[currentFile].redo();
  if (!res) {
    buffer[currentFile].setPrompt(ANSI::purple("No more redo."), true);
  } else {
    buffer[currentFile].setPrompt(ANSI::purple("A redo finished."), true);
  }
}
void Core::handleUNDO() {
  auto res = buffer[currentFile].undo();
  if (!res) {
    buffer[currentFile].setPrompt(ANSI::purple("No more undo."), true);
  } else {
    buffer[currentFile].setPrompt(ANSI::purple("A undo finished."), true);
  }
}
void Core::handleENTER() {
  lastChar = 0;
  std::pair<int, int> info;
  switch (state) {
    case programState::Normal:
      buffer[currentFile].toNextLine();
      break ;
    case programState::Command:
      buffer[currentFile].setPrompt("");
      if (command == "q" || command == "q!") {
        if (command.back() != '!' && !buffer[currentFile].isSaved()) {
          buffer[currentFile].setPrompt(ANSI::purple("[Warning] You should save by :w first, or :wq. "));
          state = programState::Normal;
        } else {
          exit(0);
        }
      } else if (command == "w" || command == "w!") {
        save();
        state = programState::Normal;
      } else if (command == "wq" || command == "wq!") {
        save();
        exit(0);
      } else if (command == "wa" || command == "wa!") {
        int cnt = saveAll();
        state = programState::Normal;
        buffer[currentFile].setPrompt(ANSI::purple("[Saved all " + std::to_string(buffer.size())
                                                   + " files (" + std::to_string(cnt) + " modified)]"), true);
      } else if (command == "next" || command == "n" || command == "next!" || command == "n!") {
        if (command.back() != '!' && !buffer[currentFile].isSaved()) {
          buffer[currentFile].setPrompt(ANSI::purple("[Warning] You should save by :w first. (Or add ! to override)"));
        } else if (currentFile + 1 < buffer.size()) {
          buffer[currentFile].setPrompt("");
          currentFile++;
          buffer[currentFile].openPrompt();
        } else {
          buffer[currentFile].setPrompt(ANSI::purple("No next file."), true);
        }
        state = programState::Normal;
      } else if (command == "prev" || command == "p" || command == "prev!" || command == "p!") {
        if (command.back() != '!' && !buffer[currentFile].isSaved()) {
          buffer[currentFile].setPrompt(ANSI::purple("[Warning] You should save by :w first. (Or add ! to override)"));
        } else if (currentFile > 0) {
          buffer[currentFile].setPrompt("");
          currentFile--;
          buffer[currentFile].openPrompt();
        } else {
          buffer[currentFile].setPrompt(ANSI::purple("No previous file."), true);
        }
        state = programState::Normal;
      } else if (command == "first" || command == "first!") {
        if (!currentFile) {
          buffer[currentFile].setPrompt(ANSI::purple("Already at the first file."), true);
        } else if (command.back() != '!' && !buffer[currentFile].isSaved()) {
          buffer[currentFile].setPrompt(ANSI::purple("[Warning] You should save by :w first. (Or add ! to override) "));
        } else {
          buffer[currentFile].setPrompt("");
          currentFile = 0;
          buffer[currentFile].openPrompt();
        }
        state = programState::Normal;
      } else if (command == "last" || command == "last!") {
        if (currentFile + 1 == buffer.size()) {
          buffer[currentFile].setPrompt(ANSI::purple("Already at the last file."), true);
        } else if (command.back() != '!' && !buffer[currentFile].isSaved()) {
          buffer[currentFile].setPrompt(ANSI::purple("[Warning] You should save by :w first. "));
        } else {
          buffer[currentFile].setPrompt("");
          currentFile = (int)buffer.size() - 1;
          state = programState::Normal;
          buffer[currentFile].openPrompt();
        }
        state = programState::Normal;
      } else if (command == "file") {
        buffer[currentFile].filePrompt();
        state = programState::Normal;
      } else if (validReplace(command, info)) {
        state = programState::Normal;
        if (!info.second)
          buffer[currentFile].setPrompt(ANSI::purple("Pattern not found."), true);
        else
          buffer[currentFile].setPrompt(ANSI::purple("Replaced " + std::to_string(info.second)
                                                     + " occurrence(s) in " + std::to_string(info.first) + " line(s)."), true);
      } else if (command == "set number") {
        for (auto &file: buffer) {
          file.setNumber();
        }
        state = programState::Normal;
        buffer[currentFile].display();
      } else if (command == "set nonumber") {
        for (auto &file: buffer) {
          file.setNoNumber();
        }
        state = programState::Normal;
        buffer[currentFile].display();
      } else if (std::all_of(command.begin(), command.end(), ::isdigit)) {
        state = programState::Normal;
        if (!buffer[currentFile].jumpTo(std::stoi(command))) {
          buffer[currentFile].setPrompt(ANSI::purple("Invalid Line Number."), true);
        }
      } else {
        state = programState::Normal;
        buffer[currentFile].setPrompt(ANSI::purple("Invalid Command."), true);
      }
      break;
    case programState::Insert:
      buffer[currentFile].enter();
      break;
  }
}
void Core::handle(direction ch) {
  lastChar = 0;
  switch (state) {
    case programState::Normal:
      buffer[currentFile].moveCursor(ch);
      break;
    case programState::Command:
      // check history command
      break;
    case programState::Insert:
      buffer[currentFile].moveCursor(ch);
      break;
  }
}
void Core::handle(char ch) {
  switch (state) {
    case programState::Normal:
      if (ch == 'u') {
        handleUNDO();
      } else if (ch == 'i') {
        state = programState::Insert;
        buffer[currentFile].setPrompt(ANSI::red("[INSERT]"), true);
      } else if (ch == ':') {
        state = programState::Command;
        command = "";
        buffer[currentFile].setPrompt(ANSI::purple(":"));
        buffer[currentFile].updateCommandDisplay();
      } else if (ch == 'h' || ch == 'j' || ch == 'k' || ch == 'l') {
        switch (ch) {
          case 'h':
            buffer[currentFile].moveCursor(direction::LEFT);
            break;
          case 'j':
            buffer[currentFile].moveCursor(direction::DOWN);
            break;
          case 'k':
            buffer[currentFile].moveCursor(direction::UP);
            break;
          case 'l':
            buffer[currentFile].moveCursor(direction::RIGHT);
            break;
          default:
            break;
        }
      } else if (ch == '0' || ch == '$') {
        if (ch == '0') buffer[currentFile].toLineFront();
        else buffer[currentFile].toLineEnd();
      } else if (ch == 'G') {
        buffer[currentFile].toLastLine();
      } else if (ch == 'g' && lastChar == 'g') {
        buffer[currentFile].toFirstLine();
        ch = 0;
      } else if (ch == 'd' && lastChar == 'd') {
        buffer[currentFile].deleteLine();
        ch = 0;
      } else if (ch == 'y' && lastChar == 'y') {
        buffer[currentFile].copyLine();
        ch = 0;
      } else if (ch == 'p') {
        buffer[currentFile].pasteLine();
      }

      break;
    case programState::Command:
      printf("%s", ANSI::purple(std::string(1, ch)).c_str());
      command.push_back(ch);
      break;
    case programState::Insert:
      buffer[currentFile].insertChar(ch);
      break;
  }
  lastChar = ch;
}