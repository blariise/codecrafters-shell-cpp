#include <iostream>
#include <unordered_set>
#include <vector>
#include <sstream>
#include <string>

std::vector<std::string> splitCommand(const std::string& command) {
  std::vector<std::string> tokens {};
  std::istringstream iss(command);
  std::string token;

  while (iss >> token) {
    tokens.push_back(token);
  }
  return tokens;
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

    std::size_t args_num { std::size(args) };

    if (args[0] == "exit" && args[1] == "0") {
      return 0;
    } else if (args[0] == "echo") {
      std::string str {};
      for (std::size_t i {1}; i < args_num; ++i) {
        str.append(args[i]);
	str.append(" ");
      }
      str.pop_back();
      std::cout << str << '\n';
    } else if (args[0] == "type") {
      if (commands.contains(args[1]))
        std::cout << args[1] << " is a shell builtin\n";
      else
	std::cout << args[1] << ": not found\n";
    } else {
      std::cout << args[0] << ": command not found\n";
    }

/*
    if (input == "exit 0") {
      break; 
    } else if (std::size_t found {input.find("echo")}; found!=std::string::npos) {
      std::cout << input.substr(5, std::size(input)) << '\n';
    } else if (input.find("type")!=std::string::npos) {
      std::string command {input.substr(5, std::size(input))};
      if ( commands.find(command) != commands.end()) { 
        std::cout << command << " is a shell builtin";
      }
    } else {	    
      std::cout << input << ": command not found\n";
    }
*/

  }
  return 0;
}
