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

int main(int argc, const char * argv[]) {
    std::filesystem::path sie_file_path("/Users/kjell-olovhogdal/Resilio Sync/XCode/MySIEParser/sie/sie/2326 ITFied 1505-1604.se");
    std::ifstream sie_file(sie_file_path);
    char ch;
    sie_file.get(ch);
    while (sie_file.good()) {
        if (ch == 10) { // SIE file defined new-line control character
            std::cout << "\n";
        }
        else if (    (ch >= ' ')
                  && (ch <= '}')) { // visible character
            std::cout << ch;
        }
        else { // Control character
            std::cout << "<" << static_cast<int>(ch) << ">";
        }
        sie_file.get(ch);
    }

    std::cout << '\n';
    return 0;
}
