## Build Your Own Vim Editor

This is a light version of the Vim editor for educational purposes. It implements the basic functionalities of Vim. Now it only support Linux and macOS platforms.


## Features

Usage: `./alayavim <file> [<file> ...]`

- Normal mode
  - ⬅️⬇️⬆️➡️ to move the cursor
  - `BACKSPACE` to move to the previous
    character
  - `ENTER` to move to the next line
  - `i` to enter insert mode
  - `:` to enter command mode
  - `h`, `j`, `k`, `l` to move left, down, up, and right respectively
  - `0` to move to the beginning of the line
  - `$` to move to the end of the line
  - `G` to move to the end of the file
  - `gg` to move to the beginning of the file
  - `dd` to delete the current line
  - `yy` to copy the current line
  - `p` to paste the copied line
  - `u` to undo the last operation
  - `ctrl + r` to redo the last operation
- Insert mode
  - `ESC` to switch to normal mode
- Command mode
  - `:q` to quit (`:q!` to force quit)
  - `:w` to save
  - `:wq` to save and quit
  - `:wa` to save all files
  - `:file` to display the file information
  - `:n` or `:next` to go to the 
    next file (add `!` to override)
  - `:p` or `:prev` to go to the 
    previous file (add `!` to override)
  - `:first` to go to the first file
  - `:last` to go to the last file
  - `:set number` to display line numbers
  - `:set nonumber` to hide line numbers
  - `:s/old/new/g` to replace `old` with `new` in the current **line**
  - `:%s/old/new/g` to replace `old` with `new` in the current **file**
  - `:<number>` to go to the line number

## Build

```bash
mkdir -p build/
cd build/
cmake ..
make -j
./alayavim <file> [<file> ...] 
```

## Project Structure

- `main.cpp` is the program entry. 
- `core.cpp` implements `Core` class, which serves as a centralized controller to send commands to different file managers
- `filemanager.cpp` contains the `FileManager` class to manage file contents and the corresponding cursor position. It controls the terminal display too.
- `log.h` contains `Log` class to record the operations for undo and redo.
- `utility.cpp` contains utility functions (e.g., ANSI).

## Implementation Details

- Support editor-like display style in terminal
  - Forbid buffer (i.e., must press "enter" to complete input) and echo (the character you pressed occurs in the terminal) to capture the key press. It is in `<unistd.h>`, a POSIX (Portable Operating System Interface) header file.
  ```cpp
  void config_set(struct termios &oldt, struct termios &newt) {
    tcgetattr(STDIN_FILENO, &oldt);  // terminal configuration
    newt = oldt;
    newt.c_lflag &= ~ICANON;  // buffer forbidden
    newt.c_lflag &= ~ECHO;    // echo forbidden
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);  // new configuration
  }
  ```
  - Distinguish the ESC key (`^`) from the arrow keys (`^[A`, `^[B`, `^[C`, `^[D`): switch to nonblocking mode temporarily to read two more characters.

    ```cpp
    int switch_to_nonblock(int fd) {
      int flags = fcntl(fd, F_GETFL, 0);
      if (flags == -1) {
        perror("fcntl");
        return -1;
      }
      
      flags |= O_NONBLOCK;
      
      if (fcntl(fd, F_SETFL, flags) == -1) {
        perror("fcntl");
        return -1;
      }
      return 0;
    }
    ```
  - ANSI escape sequences are a set of control codes used to control the formatting, color, and behavior of text in command-line interfaces. See `utility.h` for detailed examples.
- `ddd` combo should not delete the lines twice.
- Replace
  - The cursor should move to a reasonable position.
    - If the cursor is within one of the replaced patterns, it moves to the start of the new word.
    - If the cursor is beyond the end of this line, it moves to the end of the line.
    - Otherwise, it sticks to the adjacent word, instead of staying in the original position.
- Undo and Redo
  - If two adjacent operations are done within 500ms, they are considered as a single operation in undo and redo.
  - The cursor will move to the original position and the view adjusts accordingly.
