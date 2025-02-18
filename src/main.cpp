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
#include <sstream>

std::vector<std::string> splitCommand(const std::string& command) {
  std::vector<std::string> tokens {};
  std::istringstream iss(command);
  std::string token;

  while (iss >> token) {
    tokens.push_back(token);
  }
  return tokens;
}

std::vector<std::size_t>getQuotesIdx(std::string_view input, const char& c) {
  std::vector<std::size_t> quotesIdx {};
  for (std::size_t i{0}; i < std::size(input); ++i) {
    if (input[i] == c)
      quotesIdx.push_back(i);
  }
  return {quotesIdx};
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

// std::string echoOutput(const std::vector<std::vector<std::size_t>, std::vector<std::size_t>> quotes) {
//x   return "hello";
//help }  

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
