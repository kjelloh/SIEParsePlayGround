//
//  main.cpp
//  sie
//
//  Created by Kjell-Olov Högdal on 2020-05-18.
//  Copyright © 2020 Kjell-Olov Högdal. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

auto format_and_output_to_cout = [](char ch) -> void {
       if (ch == 10) { // SIE file defined new-line control character
            std::cout << "<" << static_cast<int>(ch) << ">" << '\n'; // Show and perform new-line
        }
       else if (ch == 13) { // SIE-file NOT new-line but allowed
            std::cout << "<" << static_cast<int>(ch) << ">"; // Show but do NOT perform new line
        }
        else if (    (ch >= ' ')
                  && (ch <= '~')) { // 7 Bit ASCII
            std::cout << ch;
        }
        else { // Control character
            std::cout << "<" << static_cast<int>(ch) << ">" << ch;
        }
};

int main(int argc, const char * argv[]) {
    std::filesystem::path sie_file_path("/Users/kjell-olovhogdal/Documents/Github/SIEParsePlayGround/sie/2326 ITFied 1505-1604.se");
    std::ifstream sie_file(sie_file_path);
    char ch;
    sie_file.get(ch);
    while (sie_file.good()) {
        format_and_output_to_cout(ch);
        sie_file.get(ch); // next
    }

    std::cout << '\n';
    return 0;
}
