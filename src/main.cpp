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
#include <vector>
#include <variant>

auto format_and_output_ch_to_cout = [](char ch) -> void {
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
            std::cout << "<" << (ch & 0xFF) << ">" << ch;
        }
};

auto format_and_output_line_to_cout = [](const std::string& sLine) {
    for (auto ch : sLine) {
        if (    (ch >= ' ')
                  && (ch <= '~')) { // 7 Bit ASCII
            std::cout << ch;
        }
        else { // Control character
            std::cout << "<" << (ch & 0xFF) << ">";
        }
    }
};

using c_Tokens = std::vector<std::string>;

int main(int argc, const char * argv[]) {
    std::filesystem::path sie_file_path("/Users/kjell-olovhogdal/Documents/Github/SIEParsePlayGround/sie/2326 ITFied 1505-1604.se");
    std::ifstream sie_file(sie_file_path);
    char ch;
    sie_file.get(ch);
    unsigned int state = 0;
    std::string sToken{};
    c_Tokens tokens{};
    bool are_sub_element_tokens = false;
    int loop_count{0};
    std::string sLine{};
    while (sie_file.good()) {
//    while (++loop_count < 1000) {
        sLine.push_back(ch);
        /**
         * Tokenise and parse SIE-file.
         * The file is defined to be encoded using IBM PC 8-bitars extended ASCII (Codepage 437) (See https://en.wikipedia.org/wiki/Code_page_437)
         **/
        //std::cout << "\nState " << state << "\tsToken = <" << sToken << ">";
        switch (state) {
            case 0: { // Parse beginning of new line
                /**
                 * Each line must begin with a #-prefixed label
                 **/

                if (tokens.size() > 0) {
                    // Trace parsed tokens
                    if (are_sub_element_tokens) {
                        std::cout << "\n\tSUB-TOKENS =";
                    }
                    else {
                        std::cout << "\nTOKENS =";
                    }
                    for (auto& sToken : tokens) {
                        std::cout << " <" << sToken << ">";
                    }
                    tokens.clear(); // next line
                }

                if ((ch == 13) || (ch == 10)) {
                    // waiting for #-label any number of new lines are allowed (consume as white space)
                }
                else if (ch == '#') {
                    sToken += ch; // parse #-prefixed token
                    state = 1;
                }
                else if ((ch == '{') && !are_sub_element_tokens) {
                    // parse sub-elements (enter sub-elements "mode")
                    are_sub_element_tokens = true;
                }
                else if (are_sub_element_tokens && ((ch == ' ') || (ch == 9))) {
                    // Sub-element tokens are allowed to "begin" with white space (indented)
                }
                else if (ch == '}' && are_sub_element_tokens) {
                    // end of sub-element listing ok
                    are_sub_element_tokens = false; // Reset sub-element tokens state
                }
                else {
                    // ERROR
                    std::cout << "\nERROR: Line can't begin with ";
                    format_and_output_ch_to_cout(ch);
                }
            }
            break;

            case 1: /* Read #-prefixed label into token */ {

                if ((ch >= 'A') && (ch <= 'Z')) { 
                    // NOTE: The specification does not specify valid characters in a #-prefixed label.
                    // Here we restrict to 'A'...'Z' as this seems to fit all defined labels.
                    // Also, All ASCII encodings share the 7-bit area of encoded characters so
                    // this source file will encode 'A'..'Z' using the same encodings as the 
                    // IBM PC 8-bitars extended ASCII (Codepage 437) encoded SIE-file (safe).
                    sToken += ch;
                }
                else if ((ch == ' ') || (ch == 9) || (ch == 13)) {
                    // SIE file white-space == end-of-#-token
                    tokens.push_back(sToken); // push #-token
                    sToken = "";                        
                    state = 2; // Continue to parse tokens that are members of found #-element
                }
                else if (ch == 10) {
                    // End-of-line == End of #-element
                    if (tokens.size() > 0) {
                        tokens.push_back(sToken); // push #-token
                        sToken = "";                        
                    }
                    state = 0; // Go back to next #-element (on next line)

                    // Trace the parsed line
                    std::cout << "\nLINE=\"";
                    format_and_output_line_to_cout(sLine);
                    std::cout << "\"";
                    sLine.clear();
                }
                else {
                    // Error, invalid input character
                    std::cout << "\n\t" << "ERROR: Invalid #-label character ";
                    format_and_output_ch_to_cout(ch);
                    sToken = "";
                    state = 0; // Go back to next #-element (on next line)

                    // Trace the parsed line
                    std::cout << "\nLINE=\"";
                    format_and_output_line_to_cout(sLine);
                    std::cout << "\"";
                    sLine.clear();
                }
            }
            break;

            case 2: /* Skip white spaces to next member token */ {

                if ((ch == ' ') || (ch == 9) || (ch == 13)) {
                    // Skip white spaces
                }
                else if (ch == 10) {
                    // End of line = end of #-element
                    if (sToken.size() > 0) {
                        tokens.push_back(sToken);
                        sToken = "";
                    }
                    state = 0; // Go back to next #-element (on next line)

                    // Trace the parsed line
                    std::cout << "\nLINE=\"";
                    format_and_output_line_to_cout(sLine);
                    std::cout << "\"";
                    sLine.clear();
                }
                else if (ch == '"') {
                    // "..." enclosed value token
                    state = 4;
                }
                else {
                    sToken += ch;
                    state = 3; // Read token content                    
                }
            }
            break;

            case 3: /* Read content (value) of #-element member token */ {

                if ((ch == ' ') || (ch == 9)) {
                    // SIE file white-space == end-of-#-token
                    tokens.push_back(sToken); // push #-token
                    sToken = "";
                    state = 2; // Continue to parse tokens that are members of found #-element
                }
                else if (ch == 10) {
                    // End-of-line == End of #-element
                    tokens.push_back(sToken);
                    sToken = "";
                    state = 0; // Go back to next #-element (on next line)

                    // Trace the parsed line
                    std::cout << "\nLINE=\"";
                    format_and_output_line_to_cout(sLine);
                    std::cout << "\"";
                    sLine.clear();
                }
                else if (ch == 13) {
                    // Skip optional carrige return
                }
                else {
                    // A "value" character
                    sToken += ch;
                }
            }
            break;

            case 4: /* Read "..." enclosed value characters into token */ {

                if (ch == '"') {
                    // End of "..." enclosed value token
                    tokens.push_back(sToken); // Push back even empty token enclosed in "..."
                    sToken = "";
                    state = 2; // Continue to parse tokens that are members of found #-element
                }
                else {
                    sToken += ch; // Add everyting betwenn "..." to token
                }
            }
            break;

        }
        sie_file.get(ch); // next
    }

    std::cout << '\n';
    return 0;
}
