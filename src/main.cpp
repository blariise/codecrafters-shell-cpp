#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <unordered_set>
#include <vector>
#include <sstream>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <string_view>
#include <sys/wait.h>
#include <unistd.h>

std::vector<std::string> splitCommand(const std::string& command) {
  std::vector<std::string> tokens {};
  std::istringstream iss(command);
  std::string token;

  while (iss >> token) {
    tokens.push_back(token);
  }
  return tokens;
}

std::vector<std::string> getEchoOutput(const std::string& input) {
  std::vector<std::string> args;
  std::string current_token;
  enum State {
    DEFAULT,
    SINGLE_QUOTE,
    DOUBLE_QUOTE
    } state = DEFAULT;

  bool escaped { false };
  std::size_t i { 0 };
  while (i < std::size(input)) {
    char c { input[i] };
    if (escaped) {
      current_token += c;
      escaped = false;
      i++;
      continue;
    }
    switch (state) {
      case DEFAULT:
        if (c == '\\') {
          escaped = true;
          i++;
        } else if (isspace(c)) {
          if (!current_token.empty()) {
            args.push_back(current_token);
            current_token.clear();
          }
          i++;
        } else if (c == '\'') {
          state = SINGLE_QUOTE;
          i++;
        } else if (c == '"') {
          state = DOUBLE_QUOTE;
          i++;
        } else {
          current_token += c;
          i++;
        }
        break;
      case SINGLE_QUOTE:
        if (c == '\'') {
          state = DEFAULT;
          i++;
        } else {
          current_token += c;
          i++;
        }
        break;
      case DOUBLE_QUOTE:
        if (c == '\\') {
          if (i + 1 < std::size(input)) {
            char next_c { input[i + 1] };
            if (next_c == '\\' || next_c == '$' || next_c == '"' || next_c == '\n') {
              current_token += next_c;
              i += 2;
            } else {
              current_token += '\\';
              i++;
            }
          } else {
            current_token += '\\';
            i++;
          }
        } else if (c == '"') {
          state = DEFAULT;
          i++;
        } else {
          current_token += c;
          i++;
        }
        break;
    }
  }
  if (!current_token.empty()) {
    args.push_back(current_token);
  }
  return args;
}

std::vector<std::string> getPaths() {
  const char* env_p { std::getenv("PATH") };
  
  if (!env_p)
    return {};

  std::string path {env_p};
  std::replace(path.begin(), path.end(), ':', ' ');
  return splitCommand(path);
}

std::filesystem::path getPathIfExists(const std::string& command) {
  std::vector paths {getPaths()};
  for (auto &path : paths) {
    const std::string full_path { path + '/' + command };

    if (std::filesystem::exists(full_path)) {
      return full_path;
    }

  }
  return {};
}

std::vector<std::string> getPathInVector(const std::string& path) {
  std::string result { path };
  std::replace(result.begin(), result.end(), '/', ' ');
  return splitCommand(result);
}

std::filesystem::path constructPath(std::filesystem::path& path, const std::string& dir) {
  if (std::filesystem::exists(static_cast<std::string>(path) + '/' + dir)) {
    return static_cast<std::string>(path) + '/' + dir;
  }
  return {};
}

std::filesystem::path constructPathFromVector(const std::vector<std::string>& path) {
  std::string real_path {};
  for (const auto& x : path) {
    real_path += '/' + x;
  }
  return real_path;
}

void movePathUp(std::filesystem::path& current_path) {
  std::vector splitted { getPathInVector(current_path) };
  splitted.pop_back();
  current_path = constructPathFromVector(splitted);
}

std::vector<std::string> split(const std::string& s, char delimiter) {
  std::vector<std::string> tokens {};
  std::string token {};
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream, token, delimiter)) {
    if (!token.empty())
      tokens.push_back(token);
  }
  return tokens;
}

bool command_exists(std::string& executable) {
  if ((executable.front() == '"' && executable.back() == '"') ||
    (executable.front() == '\'' && executable.back() == '\'')) {
    executable = executable.substr(1, executable.size() - 2);
  }

  if (executable.find('/') != std::string::npos)
    return access(executable.c_str(), X_OK) == 0;

  const char* path = std::getenv("PATH");
  if (!path)
    return false;

  std::vector<std::string> dirs { split(path, ':') };
  for (const auto& dir : dirs) {
    std::string full_path { dir + "/" + executable };
    if (access(full_path.c_str(), X_OK) == 0) {
      return true;
    }
  }
  return false;
}

void execute_command(const std::string& input) {
  std::string executable {};
  std::string arguments {};
  size_t executable_end {0};
  bool in_quote { false };
  char quote_char {'\0'};
    
  for (std::size_t i { 0 }; i < std::size(input); ++i) {
    if (!in_quote && (input[i] == '\'' || input[i] == '"')) {
      in_quote = true;
      quote_char = input[i];
      executable_end = i + 1;
      while (executable_end < std::size(input) && input[executable_end] != quote_char) {
        ++executable_end;
      }
      if (executable_end < std::size(input)) {
        ++executable_end;
        break;
      }
    } else if (!in_quote && input[i] == ' ') {
      executable_end = i;
      break;
    }
  }

  executable = input.substr(0, executable_end);
  if (executable_end < input.size()) {
    arguments = input.substr(executable_end);
  }

  std::string exe_to_check { executable };
  if (!command_exists(exe_to_check)) {
    std::cerr << "Command not found: " << exe_to_check << '\n';
    return;
  }

  if (executable.find(' ') != std::string::npos &&
    (executable.front() != '"' && executable.front() != '\'')) {
        executable = "\"" + executable + "\"";
  }

  std::system((executable + arguments).c_str());
}

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::unordered_set<std::string_view> commands {
    "exit",
    "echo",
    "type",
    "pwd",
    "cd",
  };
  std::filesystem::path current_path { std::filesystem::current_path() };
  while(true) {	  
    std::cout << "$ ";

    std::string input;
    std::getline(std::cin, input);

    std::vector<std::string> args { splitCommand(input) };

    if (args.empty())
      continue;

    const std::string cmd { args[0] };

    if (commands.contains(cmd)) {

      if (cmd == "exit" && args[1] == "0")
        return 0;

      if (cmd == "echo") {
        if (std::size(args) == 1) {
          std::cout << '\n';
          continue;
        }
        input = input.substr(5);
        std::vector<std::string> args = getEchoOutput(input);
        if (!args.empty()) {
          std::cout << args[0];
          for (std::size_t i { 1 }; i < std::size(args); ++i) {
            std::cout << " " << args[i];
          }
        }
        std::cout << '\n';
      }

      if (cmd == "pwd") {
        std::cout << static_cast<std::string>(current_path) << '\n';
      }

      if (cmd == "cd") {
        std::string path { args[1] };
        std::vector<std::string> rel_path { getPathInVector(path) };

        if (rel_path[0] == "~") {
          const char* env_p { std::getenv("HOME") };
          current_path = env_p;
          continue;
        }

        if (rel_path[0] == "." || rel_path[0] == "..") {
          for (const auto& p : rel_path) {
            if (p == ".")
              continue;
            else if (p == "..") {
              movePathUp(current_path);
            } else {
              current_path = constructPath(current_path, p);
            }
          }
        } else {
            if (std::filesystem::exists(path)) {
              current_path = path;
            } else {
              std::cout << "cd: " << path << ": No such file or directory\n"; 
            }
        }
      }

      if (cmd == "type") {
        bool flag {false};

        if (commands.contains(args[1]))
          std::cout << args[1] << " is a shell builtin\n";
        else {
          if(std::filesystem::path path {getPathIfExists(args[1])}; path != "")
            std::cout << args[1] << " is " << static_cast<std::string>(path) << '\n';
          else
            std::cout << args[1] << ": not found\n";
        }
      }

    } else {
      std::string temp { input };
      for (auto x : temp) {
        if (x == '"' || x == '\'') {
          execute_command(input);
          continue;
        }
      }
      if (getPathIfExists(args[0]) != "")
        system(input.c_str());
      else
        std::cout << args[0] << ": command not found\n";
    }
  }
  return 0;
}
