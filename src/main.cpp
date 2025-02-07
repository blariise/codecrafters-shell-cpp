#include <iostream>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  while(true) {	  
    std::cout << "$ ";

    std::string input;
    std::getline(std::cin, input);
    if (input == "exit 0") {
      break; 
    } else if (std::size_t found {input.find("echo")}; found!=std::string::npos) {
      std::cout << input.substr(5, std::size(input)) << '\n';
    } else {	    
      std::cout << input << ": command not found\n";
    }
  }
  return 0;
}
