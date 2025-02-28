#include <vector>
#include <string>
#include <memory>

#include "log.h"
#include "filemanager.h"
#include "utility.h"

void FileManager::getTerminalSize() {
  struct winsize w{};
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  terminalHeight = w.ws_row;
  terminalWidth = w.ws_col;
}

void FileManager::splitLine(const std::string &line, std::vector<std::string> &output, int lineid) const {
  int len = (int)line.size();
  if (len == 0) {
    output.push_back(numbered ? ANSI::grey(align_num(lineid, lineWidth - 1)) + " " : "");
    return ;
  }
  for (int i = 0; i < len; i += width) {
    if (numbered) {
      if (lineid == -1) {
        output.push_back(std::string(lineWidth - 1, ' ')
                         + " " + line.substr(i, width));
      } else {
        output.push_back(ANSI::grey(align_num(lineid, lineWidth - 1))
                         + " " + line.substr(i, width));
        lineid = -1;
      }
    } else {
      output.push_back(line.substr(i, width));
    }
  }
}

FileManager::FileManager(const std::vector<std::string> &fileContent,
            std::string name) : filename(std::move(name)) {
  content = std::make_shared<std::vector<std::string>>(fileContent);
  assert(!fileContent.empty());
  getTerminalSize();
}

FileManager::FileManager(FileManager &&other) noexcept :
        filename(other.filename),
        content(std::move(other.content)),
        log(std::move(other.log)),
        prompt(std::move(other.prompt)),
        ephemeral(other.ephemeral),
        saved(other.saved),
        numbered(other.numbered),
        posX(other.posX),
        posY(other.posY),
        terminalHeight(other.terminalHeight),
        terminalWidth(other.terminalWidth),
        windowStartX(other.windowStartX),
        lineWidth(other.lineWidth),
        width(other.width),
        where(other.where) {
  other.content = nullptr;
}
[[nodiscard]]
bool FileManager::isSaved() const {
  return saved;
}
void FileManager::setNumber() {
  numbered = true;
}
void FileManager::setNoNumber() {
  numbered = false;
}

void FileManager::commitModify(int pos, const std::string &newContent) {
  saved = false;
  if (where < log.size()) {
    log.erase(log.begin() + where, log.end());
  }
  log.push_back(std::make_unique<LogContent>(LogContent(atomType::MODIFY,
                                                        pos, (*content)[pos], newContent)));
  (*content)[pos] = newContent;
  where += 1;
}
void FileManager::commitInsert(int pos, const std::string &newContent) {
  saved = false;
  if (where < log.size()) {
    log.erase(log.begin() + where, log.end());
  }
  log.push_back(std::make_unique<LogContent>(LogContent(atomType::INSERT, pos, "", newContent)));
  content->insert(content->begin() + pos, newContent);
  where += 1;
}
void FileManager::commitDelete(int pos) {
  saved = false;
  if (where < log.size()) {
    log.erase(log.begin() + where, log.end());
  }
  log.push_back(std::make_unique<LogContent>(LogContent(atomType::DELETE, posX, (*content)[pos], "")));
  content->erase(content->begin() + pos);
  where += 1;
}
void FileManager::undo(const std::unique_ptr<Log> &log_) {
  saved = false;
  if (dynamic_cast<LogContent *>(log_.get()) == nullptr) {
    auto &log = dynamic_cast<LogCursor &>(*log_);
    posX = log.oldX;
    posY = log.oldY;
  } else {
    auto &log = dynamic_cast<LogContent &>(*log_);
    switch (log.type) {
      case atomType::MODIFY:
        (*content)[log.posX] = log.oldContent;
        break;
      case atomType::INSERT:
        content->erase(content->begin() + log.posX);
        break;
      case atomType::DELETE:
        content->insert(content->begin() + log.posX, log.oldContent);
        break;
    }
  }
}
void FileManager::redo(const std::unique_ptr<Log> &log_) {
  saved = false;
  if (dynamic_cast<LogContent *>(log_.get()) == nullptr) {
    auto &log = dynamic_cast<LogCursor &>(*log_);
    posX = log.newX;
    posY = log.newY;
  } else {
    auto &log = dynamic_cast<LogContent &>(*log_);
    switch (log.type) {
      case atomType::MODIFY:
        (*content)[log.posX] = log.newContent;
        break;
      case atomType::INSERT:
        content->insert(content->begin() + log.posX, log.newContent);
        break;
      case atomType::DELETE:
        content->erase(content->begin() + log.posX);
        break;
    }
  }
}
bool FileManager::undo() {
  if (where == 0) {
    return false;
  }
  int oldWhere = where;
  -- where;
  while (where > 0 && duration(*log[where - 1], *log[where]) < UNDO_REDO_INTERVAL) {
    -- where;
  }
  for (int i = oldWhere - 1; i >= where; -- i) {
    undo(log[i]);
  }
  display();
  return true;
}
bool FileManager::redo() {
  if (where == log.size()) {
    return false;
  }
  int oldWhere = where;
  ++ where;
  while (where < log.size() && duration(*log[where - 1], *log[where]) < UNDO_REDO_INTERVAL) {
    ++ where;
  }
  for (int i = oldWhere; i < where; ++ i) {
    redo(log[i]);
  }
  display();
  return true;
}


void FileManager::setPrompt(const std::string &p, bool e) {
  prompt = p;
  this->ephemeral = e;
  display();
}
void FileManager::updateCommandDisplay() const {
  printf("%s", ANSI::cursorPosition(terminalHeight, 2).c_str());
  fflush(stdout);
}
void FileManager::display() {
  std::vector<std::string> output;
  lineWidth = 0;
  if (numbered) {
    lineWidth = (int)std::max(4ul, 1 + std::to_string(content->size()).size());
  }
  width = terminalWidth - lineWidth;

  assert (width > 0);
  size_t cursorX = 0, cursorY = 0;
  for (size_t i = 0; i < content->size(); ++i) {
    if (posX == i) {
      cursorX = output.size() + (posY / width);
      cursorY = posY % width + lineWidth;
    }
    splitLine((*content)[i], output, (int)i + 1);
  }

  size_t height = terminalHeight;
  if (!prompt.empty()) {
    height --;
  }
  if (cursorX >= windowStartX + height) {
    windowStartX = cursorX - height + 1;
  } else if (cursorX < windowStartX) {
    windowStartX = cursorX;
  }
  size_t endLine = std::min(windowStartX + height, output.size());
  clearTerminal();
  size_t row = 0;

  for (size_t i = windowStartX; i < endLine; ++i) {
    printf("%s", output[i].c_str());
    row ++;
    if (row != terminalHeight) {
      printf("\n");
      fflush(stdout);
    }
  }
  if (prompt.size() > 0) {
    while (row < terminalHeight - 1) {
      printf("\n");
      row ++;
    }
    printf("%s", prompt.c_str());
    fflush(stdout);
  }
  printf("%s", ANSI::cursorPosition(cursorX - windowStartX + 1, cursorY + 1).c_str());
  fflush(stdout);
}
void FileManager::moveCursor(direction d) {
  switch (d) {
    case direction::UP:
      if (posX > 0) {
        posX--;
        posY = std::min((int)(*content)[posX].size(), posY);
        display();
      }
      break;
    case direction::DOWN:
      if (posX + 1 < content->size()) {
        posX++;
        posY = std::min((int)(*content)[posX].size(), posY);
        display();
      }
      break;
    case direction::LEFT:
      if (posY > 0) {
        posY--;
        display();
      }
      break;
    case direction::RIGHT:
      if (posY < (int)(*content)[posX].size()) {
        posY++;
        display();
      }
      break;
  }

}
void FileManager::toLastChar() {
  if (posX || posY) {
    if (posY == 0) {
      posX --;
      posY = (*content)[posX].size();
      if (posY)
        -- posY;
    } else {
      posY --;
    }
    display();
  }
}
void FileManager::toNextLine(bool logFlag) {
  if (posX + 1 < content->size()) {
    int oldX = posX, oldY = posY;
    posX ++;
    posY = 0;
    display();
    if (logFlag) {
      log.push_back(std::make_unique<LogCursor>(LogCursor(oldX, oldY, posX, posY)));
      where += 1;
    }
  }
}
void FileManager::toLineFront() {
  posY = 0;
  display();
}
void FileManager::toLineEnd() {
  posY = (*content)[posX].size();
  if (posY)
    posY --;
  display();
}
void FileManager::toLastLine() {
  posX = content->empty() ? 0 : (int)content->size() - 1;
  posY = 0;
  display();
}
void FileManager::toFirstLine() {
  posX = 0;
  posY = 0;
  display();
}
bool FileManager::jumpTo(int line) {
  if (line >= 1 && line <= content->size()) {
    posX = line - 1;
    posY = 0;
    display();
    return true;
  }
  return false;
}
void FileManager::enter() {
  std::string head = (*content)[posX].substr(0, posY);
  std::string remaining = (*content)[posX].substr(posY);
  commitModify(posX, head);
  commitInsert(posX + 1, remaining);
  toNextLine(true);
}
void FileManager::backspace() {
  int oldX = posX, oldY = posY;
  if (posY > 0) {
    auto tmp = (*content)[posX];
    tmp = tmp.erase(posY - 1, 1);
    commitModify(posX, tmp);
    posY--;
  } else if (posX > 0) {
    posY = (*content)[posX - 1].size();
    std::string newRow = (*content)[posX - 1] + (*content)[posX];
    commitModify(posX - 1, newRow);
    commitDelete(posX);
    posX--;
  }
  log.push_back(std::make_unique<LogCursor>(LogCursor(oldX, oldY, posX, posY)));
  where += 1;
  display();
}
void FileManager::deleteLine() {
  if (content->empty())
    return;
  int oldX = posX, oldY = posY;
  posY = 0;
  commitDelete(posX);
  if (posX + 1 >= content->size() && posX) {
    posX --;
  }
  log.push_back(std::make_unique<LogCursor>(LogCursor(oldX, oldY, posX, posY)));
  where += 1;
  display();
}
void FileManager::copyLine() {
  if (content->empty()) return;
  board = (*content)[posX];
}
void FileManager::pasteLine() {
  if (board.empty()) return;
  commitInsert(posX + 1, board);
  toNextLine(true);
  display();
}
void FileManager::insertChar(char c) {
  std::string tmp = (*content)[posX];
  tmp.insert(posY, 1, c);
  commitModify(posX, tmp);
  log.push_back(std::make_unique<LogCursor>(LogCursor(posX, posY, posX, posY + 1)));
  where += 1;
  posY ++;
  display();
}
std::string FileManager::replace_str(const std::string &s, const std::string &pattern, const std::string &replacement, int &occurs, int row) {
  std::string current, result;
  int col = 0;
  bool found = false;
  for (auto c: s) {
    if (row == posX && col == posY) {
      posY = result.size() + current.size();
      found = true;
    }
    current += c;
    if (current.size() >= pattern.size()) {
      if (current.substr(current.size() - pattern.size()) == pattern) {
        if (row == posX && found) {
          posY = std::min(posY, (int)(result.size() + current.size() - pattern.size()));
        }
        result += current.substr(0, current.size() - pattern.size());
        result += replacement;
        current.clear();
        occurs += 1;
      }
    }
    col ++;
  }
  result += current;
  if (row == posX) {
    posY = std::min(posY, (int)result.size());
  }
  return result;
}
std::pair<int, int> FileManager::replace(const std::string &pattern, const std::string &replacement, bool inFile) {
  saved = false;
  int cntLine = 0, cnt = 0;
  if (inFile) {
    int pos = 0;
    for (auto &line: (*content)) {
      int occurs = 0;
      auto res = replace_str(line, pattern, replacement, occurs, pos);
      if (occurs > 0) {
        cnt += occurs;
        cntLine ++;
        commitModify(pos, res);
      }
      pos ++;
    }
  } else {
    std::string res = replace_str((*content)[posX], pattern, replacement, cnt, posX);
    cntLine = 1;
    if (cnt > 0) {
      commitModify(posX, res);
    }
  }
  return {cntLine, cnt};
}
std::string FileManager::fileInfo() {
  size_t bytes = 0;
  for (const std::string &line: (*content)) {
    if (bytes) ++ bytes;
    bytes += line.size();
  }
  return " [" + std::to_string(content->size()) + " lines]"
         + " [" + std::to_string(bytes) + " bytes]";
}
void FileManager::save(bool print) {
  std::ofstream out(filename);
  if (!out) {
    std::cerr << "Cannot write files." << std::endl;
    return ;
  }
  for (const auto &line : (*content)) {
    out << line << std::endl;
  }
  saved = true;
  if (print)
    setPrompt(ANSI::purple("[Saved " + filename + "]"), true);
}
void FileManager::clearPrompt() {
  if (ephemeral) {
    setPrompt("");
  }
}
void FileManager::openPrompt() {
  setPrompt(ANSI::purple("[Opened " + filename + "]"), true);
}
void FileManager::filePrompt() {
  setPrompt(ANSI::purple(filename + fileInfo() + (saved ? " [Saved]" : " [Not Saved]")), true);
}