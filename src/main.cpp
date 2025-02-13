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

std::vector<std::string> splitCommand(const std::string& command) {
  std::vector<std::string> tokens {};
  std::istringstream iss(command);
  std::string token;

  while (iss >> token) {
    tokens.push_back(token);
  }
  return tokens;
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

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::unordered_set<std::string_view> commands {
    "exit",
    "echo",
    "type",
    "pwd",
    "cd",
    "cat"
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
        std::string str {};
        if (input.at(5) == '\'' && input.back() == '\'') {
          std::string temp {input};
          temp.erase(0, 6);
          temp.pop_back();
          str.append(temp);

        } else {
            for (std::size_t i {1}; i < std::size(args); ++i) {
              str.append(args[i]);
              str.append(" ");
            }
            str.pop_back();
        }
        
        std::cout << str << '\n';
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

      if (cmd == "cat") {
        
      }

    } else {
        if(getPathIfExists(cmd) != "") {
          std::string full_cmd {cmd};
          std::system(input.c_str());
        }
        else
          std::cout << cmd << ": command not found\n";
    }
  }
  return 0;
}
