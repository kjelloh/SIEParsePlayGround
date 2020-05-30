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

/**
 * SIE file white space between "tokens
 * */
bool is_white_space(char ch) {
    return (ch == ' ') || (ch == 9);
}

bool is_valid_new_line(char ch) {
    return (ch == 10);
}

bool is_optional_new_line(char ch) {
    return (ch == 13);
}

bool is_valid_or_optional_new_line(char ch) {
    return is_valid_new_line(ch) || is_optional_new_line(ch);
}

auto format_and_output_ch_to_cout = [](char ch) -> void {
       if (is_valid_new_line(ch)) { // SIE file defined new-line control character
            std::cout << "<" << static_cast<int>(ch) << ">" << '\n'; // Show and perform new-line
        }
       else if (is_optional_new_line(ch)) { // SIE-file NOT new-line but allowed
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
using c_SubEntries = std::vector<c_Tokens>;

class c_SIEFileEntry {
    friend std::ostream& operator<<(std::ostream& os, c_SIEFileEntry file_entry);
public:
    c_SIEFileEntry(const c_Tokens tokens) 
        :  m_tokens{tokens}, m_sub_entries{} {}

    bool has_sub_entries() {return m_sub_entries.size() > 0;}
    c_Tokens const& tokens() {return m_tokens;}
    c_SubEntries const& sub_entries() {return m_sub_entries;}

    void add_sub_entry(const c_Tokens sub_entry) {m_sub_entries.push_back(sub_entry);}

private:
    c_Tokens m_tokens;
    c_SubEntries m_sub_entries;
};

std::ostream& operator<<(std::ostream& os, c_SIEFileEntry entry) {
    os << "\n";
    bool first_token = true;
    for (auto& token : entry.tokens()) {
        if (!first_token) {
            os << "\t";
        }
        os << token;
        first_token = false;
    }
    if (entry.has_sub_entries()) {
        for (auto& sub_entry : entry.sub_entries()) {
            os << "\n\t";
            bool first_token = true;
            for (auto& token : sub_entry) {
                if (!first_token) {
                    os << "\t";
                }
                std::cout << token;
                first_token = false;
            }
        
        }
    }
    return os;
}

using c_SIEFileEntries = std::vector<c_SIEFileEntry>;

c_SIEFileEntries parse_sie_file(std::ifstream& sie_file) {
    c_SIEFileEntries sie_file_entries{};

    if (sie_file) {
        unsigned int state = 0;
        bool are_sub_element_tokens = false;
        std::string sToken{};
        c_Tokens tokens{};

        int loop_count{0}; // For Debug trace
        std::string sLine{}; // For Debug trace

        char ch;
        sie_file.get(ch);
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

                    if (tokens.size() > 0) {
                        // Trace parsed tokens
                        if (are_sub_element_tokens) {
                            std::cout << "\n\tSUB-TOKENS =";
                            sie_file_entries.back().add_sub_entry(tokens);
                        }
                        else {
                            std::cout << "\nTOKENS =";
                            sie_file_entries.push_back(c_SIEFileEntry(tokens));
                        }
                        for (auto& sToken : tokens) {
                            std::cout << " <" << sToken << ">";
                        }
                        tokens.clear(); // next line
                    }

                    if (is_valid_or_optional_new_line(ch)) {
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
                    else if (are_sub_element_tokens && is_white_space(ch)) {
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
                    else if (is_white_space(ch) || is_optional_new_line(ch)) {
                        // SIE file white-space == end-of-#-token
                        tokens.push_back(sToken); // push #-token
                        sToken = "";                        
                        state = 2; // Continue to parse tokens that are members of found #-element
                    }
                    else if (is_valid_new_line(ch)) {
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

                    if (is_white_space(ch) || is_optional_new_line(ch)) {
                        // Skip white spaces
                    }
                    else if (is_valid_new_line(ch)) {
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

                    if (is_white_space(ch)) {
                        // SIE file white-space == end-of-#-token
                        tokens.push_back(sToken); // push #-token
                        sToken = "";
                        state = 2; // Continue to parse tokens that are members of found #-element
                    }
                    else if (is_valid_new_line(ch)) {
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
                    else if (is_optional_new_line(ch)) {
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
            sie_file.get(ch); // next char
        }

        // Dump parsed entries
        for (auto& entry : sie_file_entries) {
            std::cout << entry; // use custom << operator for the entry
        }
    }
    return sie_file_entries;
}

int main(int argc, const char * argv[]) {
    // Choose and open SIE file
    std::string sSIEFileName = (argc > 1) ? argv[1] : "../sie/2326 ITFied 1505-1604.se";
    std::filesystem::path sie_file_path(sSIEFileName);
    std::ifstream sie_file(sie_file_path);

    // User feed back
    if (sie_file) {
        std::cout << "\nWill Open File " << sie_file_path;
    }
    else {
        std::cout << "\nUnknown File " << sie_file_path;
    }
    std::cout << "\nPress any key...";
    char dummy_char;
    std::cin.get(dummy_char);

    // Parse the SIE file
    c_SIEFileEntries sie_file_entries = parse_sie_file(sie_file);

    // Dump parsed entries
    for (auto& entry : sie_file_entries) {
        std::cout << entry;
    }

    // Exit
    std::cout << '\n';
    return 0;
}
