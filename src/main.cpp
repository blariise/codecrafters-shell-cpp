#include <algorithm>
#include <iostream>
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


int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::unordered_set<std::string> commands {"exit", "echo", "type"};
  while(true) {	  
    std::cout << "$ ";

    std::string input;
    std::getline(std::cin, input);

    std::vector<std::string> args { splitCommand(input) };

    if (args.empty())
      continue;

    std::string_view cmd { args[0]};

    if (!commands.contains(cmd)) {
      std::cout << cmd << ": command not found\n";
    } else {

      if (cmd == "exit" && args[1] == "0")
        return 0;

      if (cmd == "echo") {
        std::string str {};
        for (std::size_t i {1}; i < std::size(args); ++i) {
          str.append(args[i]);
          str.append(" ");
        }
        str.pop_back();
        std::cout << str << '\n';
      }

      if (cmd == "type") {
        bool flag {false};

        if (commands.contains(cmd))
          std::cout << args[1] << " is a shell builtin\n";
        else {
          std::vector paths {getPaths()};
          
          for (auto &path : paths) {
            const std::string full_path { path + '/' + args[1] };

            if (std::filesystem::exists(full_path)) {
              std::cout << args[1] << " is " << full_path << '\n';
              flag = true;
              break;
            }

          }
        if (!flag)
          std::cout << args[1] << ": not found\n";
        }
      }
    }
  }
  return 0;
}
