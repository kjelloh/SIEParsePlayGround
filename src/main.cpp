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
#include <optional>
#include <algorithm>

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
    c_Tokens const& tokens() const {return m_tokens;}
    c_SubEntries const& sub_entries() const {return m_sub_entries;}

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

    std::cout << "\nSIE Parse to BEGIN - Press any key...";
    char dummy_char;
    std::cin.get(dummy_char);

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
    }

    std::cout << "\nSIE Parse END - Press any key...";
    std::cin.get(dummy_char);

    return sie_file_entries;
}

struct c_SIEFileAmount {
    std::string m_amount;
};

using c_OptionalSIEFileAmount = std::optional<c_SIEFileAmount>;

struct c_AnnualReportEntry {
    std::string m_caption;
    c_OptionalSIEFileAmount m_value;
};

std::ostream& operator<<(std::ostream& os, c_AnnualReportEntry entry) {
    os << entry.m_caption << "\t";
    if (entry.m_value) {
        os << entry.m_value->m_amount;
    }
    else {
        os << "NULL";       
    }
    return os;
}

c_OptionalSIEFileAmount get_IB_Amount(const c_SIEFileEntries& sie_file_entries,int year_index,std::string account_number) {
    c_OptionalSIEFileAmount result;
    auto iter = std::find_if(
         sie_file_entries.begin()
        ,sie_file_entries.end()
        ,[year_index, account_number](const c_SIEFileEntry& entry) {
            return (     (entry.tokens()[0] == "#IB")
                     and (entry.tokens()[2] == account_number)
                     and (entry.tokens()[1] == std::to_string(year_index)));
        }
    );
    if (iter != sie_file_entries.end()) {
        result = {iter->tokens()[3]};
    }
    return result;
}

c_AnnualReportEntry create_annual_report_entry(std::string caption, c_OptionalSIEFileAmount amount) {
    return  {caption,amount};
}
         
using c_AnnualReport = std::vector<c_AnnualReportEntry>;

c_AnnualReport create_annual_report(const c_SIEFileEntries& sie_file_entries) {
    c_AnnualReport result;
    // Förändringar i eget kapital / Vid årets ingång / Aktiekapital
    result.push_back(create_annual_report_entry(
         "Förändringar i eget kapital / Vid årets ingång / Aktiekapital"
        ,get_IB_Amount(sie_file_entries,0,"2081")));    
    return result;
}

void generate_rtf_file(std::filesystem::path const& sie_file_path,c_AnnualReport const& annual_report) {

    // This seems to be microsoft official RTF 1.9.1 specification for download?
    // https://interoperability.blob.core.windows.net/files/Archive_References/[MSFT-RTF].pdf
    // NOTE: This pdf does not seem to be searhcable (image scanned only?)

    // This seesm to be a web based RTF 1.6 specification
    // http://latex2rtf.sourceforge.net/rtfspec.html

    /**
     * From RTF Specification for <header>
     * <header>     \rtf <charset> \deff? <fonttbl> <filetbl>? <colortbl>? <stylesheet>? <listtables>? <revtbl>?

     * <fonttbl>    '{' \fonttbl (<fontinfo> | ('{' <fontinfo> '}'))+ '}'
     * <filetbl>	'{\*' \filetbl ('{' <fileinfo> '}')+ '}'
     * <colortbl>	'{' \colortbl <colordef>+ '}'
     * <colordef>	\red ? & \green ? & \blue ? ';'
     * <stylesheet>	'{' \stylesheet <style>+ '}'
     * <listtables> <listtable>? <listoverridetable>?
     * <listtable>	'{' \*\listtable <list>+ '}'
     * <revtbl>     \*\revtbl
     * 
     * Our example <header>

        {\rtf1\ansi\ansicpg1252\cocoartf2513
        \cocoatextscaling0\cocoaplatform0{\fonttbl\f0\froman\fcharset0 Times-Bold;\f1\froman\fcharset0 Times-Roman;}
        {\colortbl;\red255\green255\blue255;\red0\green0\blue0;\red191\green191\blue191;}
        {\*\expandedcolortbl;;\cssrgb\c0\c0\c0;\csgray\c79525;}

     * Our interpretation
     * Spec:        Our instantiation:
     * \rtf         \rtf1       // RTF version 1
     * <charset>    \ansi
     *              \ansicpg1252
     * ??           \cocoartf2513
                    \cocoatextscaling0
                    \cocoaplatform0
       \deff?       -
       <fonttbl>    {
                    \fonttbl
                    \f0
                    \froman
                    \fcharset0 Times-Bold;
                    \f1
                    \froman
                    \fcharset0 Times-Roman;
                    }
     * <filetbl>    -
     * <colortbl>   {
     *              \colortbl
     *                  ;
     *                  \red255\green255\blue255;
     *                  \red0\green0\blue0;
     *                  \red191\green191\blue191;
     *              }
     * ??           {
     * ??           \*\expandedcolortbl
     * ??           ;
     * ??           ;
     * ??           \cssrgb\c0\c0\c0;
     * ??           \csgray\c79525;
     * ??           }
     * 
     * <stylesheet> -
     * <listtable>  -

     * From RTF Specification for <document>
     * <document>	<info>? <docfmt>* <section>+
     * 
     * 	    
     *              \paperw11900
     *              \paperh16840
     *              \margl1440
     *              \margr1440
     *              \vieww51000
     *              \viewh27380
     *              \viewkind1
     * 
     * \deftabN     \deftab720

                    \pard
                    \pardeftab720
                    \sa240
                    \partightenfactor0
                    
    * From RTF specification http://latex2rtf.sourceforge.net/rtfspec_7.html#rtfspec_tabledef
    * "There is no RTF table group; instead, tables are specified as paragraph properties."

    <row>	(<tbldef> <cell>+ <tbldef> \row) | (<tbldef> <cell>+ \row) | (<cell>+ <tbldef> \row)
    <cell>	(<nestrow>? <tbldef>?) & <textpar>+ \cell

     **/


/**
 * Adding a row to existing 4 row 5 column table results in the following changes to the rtf-file (Apple TextEdit generated file)
 * 
 * -3 {\colortbl;\red255\green255\blue255;\red0\green0\blue0;\red191\green191\blue191;}
 * +3 {\colortbl;\red255\green255\blue255;\red0\green0\blue0;\red191\green191\blue191;\red191\green191\blue191;}
 * 
 * -4 {\*\expandedcolortbl;;\cssrgb\c0\c0\c0;\csgray\c79525;}
 * +4 {\*\expandedcolortbl;;\cssrgb\c0\c0\c0;\csgray\c79525;\csgray\c79525;}
 *
 * -76 \itap1\trowd \taflags1 \trgaph108\trleft-108 \trbrdrl\brdrnil \trbrdrt\brdrnil \trbrdrr\brdrnil
 * +76 \itap1\trowd \taflags1 \trgaph108\trleft-108 \trbrdrl\brdrnil \trbrdrr\brdrnil
 * 
 * + between line 92 & 93 new section
    \cf2 \cell \row

    \itap1\trowd \taflags1 \trgaph108\trleft-108 \trbrdrl\brdrnil \trbrdrt\brdrnil \trbrdrr\brdrnil 
    \clvertalc \clshdrawnil \clbrdrt\brdrs\brdrw20\brdrcf4 \clbrdrl\brdrs\brdrw20\brdrcf4 \clbrdrb\brdrs\brdrw20\brdrcf4 \clbrdrr\brdrs\brdrw20\brdrcf4 \clpadl100 \clpadr100 \gaph\cellx1728
    \clvertalc \clshdrawnil \clbrdrt\brdrs\brdrw20\brdrcf4 \clbrdrl\brdrs\brdrw20\brdrcf4 \clbrdrb\brdrs\brdrw20\brdrcf4 \clbrdrr\brdrs\brdrw20\brdrcf4 \clpadl100 \clpadr100 \gaph\cellx3456
    \clvertalc \clshdrawnil \clbrdrt\brdrs\brdrw20\brdrcf4 \clbrdrl\brdrs\brdrw20\brdrcf4 \clbrdrb\brdrs\brdrw20\brdrcf4 \clbrdrr\brdrs\brdrw20\brdrcf4 \clpadl100 \clpadr100 \gaph\cellx5184
    \clvertalc \clshdrawnil \clbrdrt\brdrs\brdrw20\brdrcf4 \clbrdrl\brdrs\brdrw20\brdrcf4 \clbrdrb\brdrs\brdrw20\brdrcf4 \clbrdrr\brdrs\brdrw20\brdrcf4 \clpadl100 \clpadr100 \gaph\cellx6912
    \clvertalc \clshdrawnil \clbrdrt\brdrs\brdrw20\brdrcf4 \clbrdrl\brdrs\brdrw20\brdrcf4 \clbrdrb\brdrs\brdrw20\brdrcf4 \clbrdrr\brdrs\brdrw20\brdrcf4 \clpadl100 \clpadr100 \gaph\cellx8640
    \pard\intbl\itap1\pardeftab720\sa240\partightenfactor0
    \cf2 \cell 
    \pard\intbl\itap1\pardeftab720\sa240\partightenfactor0
    \cf2 \cell 
    \pard\intbl\itap1\pardeftab720\sa240\partightenfactor0
    \cf2 \cell 
    \pard\intbl\itap1\pardeftab720\sa240\partightenfactor0
    \cf2 \cell 
    \pard\intbl\itap1\pardeftab720\sa240\partightenfactor0 

**/

/**
 * Adding a column to existing 4 row 5 column table results in the following changes to the rtf-file (Apple TextEdit generated file)
 * 
 * -3 {\colortbl;\red255\green255\blue255;\red0\green0\blue0;\red191\green191\blue191;}
 * +3 {\colortbl;\red255\green255\blue255;\red0\green0\blue0;\red191\green191\blue191;\red191\green191\blue191;}
 * 
 *  - rows 13..16               + rows 13..17

    ... = \clvertalc \clshdrawnil \clbrdrt\brdrs\brdrw20\brdrcf3 \clbrdrl\brdrs\brdrw20\brdrcf3 \clbrdrb\brdrs\brdrw20\brdrcf3 \clbrdrr\brdrs\brdrw20\brdrcf3 \clpadl100 \clpadr100

    ... \gaph\cellx1728         ... \gaph\cellx1440
    ... \gaph\cellx3456         ... \gaph\cellx2880
    ... \gaph\cellx5184         ... \gaph\cellx4320
    ... \gaph\cellx6912         ... \gaph\cellx5760
    ... \gaph\cellx8640         ... \gaph\cellx7200

    ... = \clvertalc \clshdrawnil \clbrdrt\brdrs\brdrw20\brdrcf4 \clbrdrl\brdrs\brdrw20\brdrcf4 \clbrdrb\brdrs\brdrw20\brdrcf4 \clbrdrr\brdrs\brdrw20\brdrcf4 \clpadl100 \clpadr100

                                ... \gaph\cellx8640
 *
 * -36 \f1\b0\fs24 \cell \row
 * +37 \f1\b0\fs24 \cell 
 * +38 \pard\intbl\itap1\pardeftab720\sa240\partightenfactor0
 * +39 \cf2 \cell \row 
 *
 *  - rows 39..43               + rows 42..47 
  
     ... = \clvertalc \clshdrawnil \clbrdrt\brdrs\brdrw20\brdrcf3 \clbrdrl\brdrs\brdrw20\brdrcf3 \clbrdrb\brdrs\brdrw20\brdrcf3 \clbrdrr\brdrs\brdrw20\brdrcf3 \clpadl100 \clpadr100

 * ... \gaph\cellx1728          ... \gaph\cellx1440
 * ... \gaph\cellx3456          ... \gaph\cellx2880
 * ... \gaph\cellx5184          ... \gaph\cellx4320
 * ... \gaph\cellx6912          ... \gaph\cellx5760
 * ... \gaph\cellx8640          ... \gaph\cellx7200
 
    ... = \clvertalc \clshdrawnil \clbrdrt\brdrs\brdrw20\brdrcf4 \clbrdrl\brdrs\brdrw20\brdrcf4 \clbrdrb\brdrs\brdrw20\brdrcf4 \clbrdrr\brdrs\brdrw20\brdrcf4 \clpadl100 \clpadr100

                                ... \gaph\cellx8640

 * This pattern seems to repeat for each table row of the table...
 * 
 **/


    auto rtf_file_path = sie_file_path;
    rtf_file_path.replace_extension("rtf");
    std::ofstream rtf_file(rtf_file_path);

    const std::vector<std::string> rtf_template = {
         R"({\rtf1\ansi\ansicpg1252\cocoartf2513)"
        ,R"(\cocoatextscaling0\cocoaplatform0{\fonttbl\f0\froman\fcharset0 Times-Bold;\f1\froman\fcharset0 Times-Roman;})"
        ,R"({\colortbl;\red255\green255\blue255;\red0\green0\blue0;\red191\green191\blue191;})"
        ,R"({\*\expandedcolortbl;;\cssrgb\c0\c0\c0;\csgray\c79525;})"
        ,R"(\paperw11900\paperh16840\margl1440\margr1440\vieww51000\viewh28280\viewkind0)"
        ,R"(\deftab720)"
        ,R"(\pard\pardeftab720\sa240\partightenfactor0)"

        ,R"(\f0\b\fs40 \cf2 \expnd0\expndtw0\kerning0)"
        ,R"(Fler\'e5rs\'f6versikt\)"

        ,R"(\itap1\trowd \taflags1 \trgaph108\trleft-108 \trbrdrt\brdrnil \trbrdrl\brdrnil \trbrdrr\brdrnil )"
        ,R"(... \gaph\cellx1728)"
        ,R"(... \gaph\cellx3456)"
        ,R"(... \gaph\cellx5184)"
        ,R"(... \gaph\cellx6912)"
        ,R"(... \gaph\cellx8640)"
        ,R"(\pard\intbl\itap1\pardeftab720\sa240\partightenfactor0)"

        ,R"(\f1\b0\fs24 \cf2 \cell )"
        ,R"(\pard\intbl\itap1\pardeftab720\sa240\partightenfactor0)"

        ,R"(\f0\b\fs26\fsmilli13333 \cf2 2019-05-01 - 2020-04-30 )"
        ,R"(\f1\b0\fs24 \cell )"
        ,R"(\pard\intbl\itap1\pardeftab720\sa240\partightenfactor0)"

        ,R"(\f0\b\fs26\fsmilli13333 \cf2 2018-05-01 - 2019-04-30)"
        ,R"(\f1\b0\fs24 \cell )"
        ,R"(\pard\intbl\itap1\pardeftab720\sa240\partightenfactor0)"

        ,R"(\f0\b\fs26\fsmilli13333 \cf2 2017-05-01 - 2018-04-30 )"
        ,R"(\f1\b0\fs24 \cell )"
        ,R"(\pard\intbl\itap1\pardeftab720\sa240\partightenfactor0)"

        ,R"(\f0\b\fs26\fsmilli13333 \cf2 2016-05-01 - 2017-04-30 )"
        ,R"(\f1\b0\fs24 \cell \row)"

        ,R"(\itap1\trowd \taflags1 \trgaph108\trleft-108 \trbrdrl\brdrnil \trbrdrr\brdrnil )"
        ,R"(... \gaph\cellx1728)"
        ,R"(... \gaph\cellx3456)"
        ,R"(... \gaph\cellx5184)"
        ,R"(... \gaph\cellx6912)"
        ,R"(... \gaph\cellx8640)"
        ,R"(\pard\intbl\itap1\pardeftab720\sa240\partightenfactor0)"

        ,R"(\fs26\fsmilli13333 \cf2 Nettooms\'e4ttning)"
        ,R"(\fs24 \cell )"
        ,R"(\pard\intbl\itap1\pardeftab720\sa240\partightenfactor0)"
        ,R"(\cf2 \cell )"
        ,R"(\pard\intbl\itap1\pardeftab720\sa240\partightenfactor0)"
        ,R"(\cf2 \cell )"
        ,R"(\pard\intbl\itap1\pardeftab720\sa240\partightenfactor0)"
        ,R"(\cf2 \cell )"
        ,R"(\pard\intbl\itap1\pardeftab720\sa240\partightenfactor0)"
        ,R"(\cf2 \cell \row)"

        ,R"(\itap1\trowd \taflags1 \trgaph108\trleft-108 \trbrdrl\brdrnil \trbrdrr\brdrnil )"
        ,R"(... \gaph\cellx1728)"
        ,R"(... \gaph\cellx3456)"
        ,R"(... \gaph\cellx5184)"
        ,R"(... \gaph\cellx6912)"
        ,R"(... \gaph\cellx8640)"
        ,R"(\pard\intbl\itap1\pardeftab720\sa240\partightenfactor0)"

        ,R"(\fs26\fsmilli13333 \cf2 Resultat efter finansiella poster )"
        ,R"(\fs24 \cell )"
        ,R"(\pard\intbl\itap1\pardeftab720\sa240\partightenfactor0)"
        ,R"(\cf2 \cell )"
        ,R"(\pard\intbl\itap1\pardeftab720\sa240\partightenfactor0)"
        ,R"(\cf2 \cell )"
        ,R"(\pard\intbl\itap1\pardeftab720\sa240\partightenfactor0)"
        ,R"(\cf2 \cell )"
        ,R"(\pard\intbl\itap1\pardeftab720\sa240\partightenfactor0)"
        ,R"(\cf2 \cell \row)"

        ,R"(\itap1\trowd \taflags1 \trgaph108\trleft-108 \trbrdrl\brdrnil \trbrdrt\brdrnil \trbrdrr\brdrnil )"
        ,R"(... \gaph\cellx1728)"
        ,R"(... \gaph\cellx3456)"
        ,R"(... \gaph\cellx5184)"
        ,R"(... \gaph\cellx6912)"
        ,R"(... \gaph\cellx8640)"
        ,R"(\pard\intbl\itap1\pardeftab720\sa240\partightenfactor0)"

        ,R"(\fs26\fsmilli13333 \cf2 Soliditet (%) )"
        ,R"(\fs24 \cell )"
        ,R"(\pard\intbl\itap1\pardeftab720\sa240\partightenfactor0)"
        ,R"(\cf2 \cell )"
        ,R"(\pard\intbl\itap1\pardeftab720\sa240\partightenfactor0)"
        ,R"(\cf2 \cell )"
        ,R"(\pard\intbl\itap1\pardeftab720\sa240\partightenfactor0)"
        ,R"(\cf2 \cell )"
        ,R"(\pard\intbl\itap1\pardeftab720\sa240\partightenfactor0)"
        ,R"(\cf2 \cell \lastrow\row)"
        ,R"(\pard\pardeftab720\sa240\partightenfactor0)"

        ,R"(\f0\b\fs40 \cf2 \)"
        ,R"(})"
    };

   std::cout << "\nGenerating file rtf_file -- BEGIN " << rtf_file_path;
   // rtf_file << R"({\rtf1\ansi{\fonttbl\f0\fswiss Helvetica;}\f0\pard This is some {\b bold} text.\par})";
   int index = 0;
   for (auto const& entry : rtf_template) {
       switch (index++) {
           default: {
               rtf_file << "\n" << entry;
           }
       };
   }
   std::cout << "\nGenerating file rtf_file -- END" << rtf_file_path;
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
    const int no_entries_per_page = 40;
    int page_entry_count = 0;
    for (auto& entry : sie_file_entries) {
        if (++page_entry_count >= no_entries_per_page) {
            std::cout << "\nPage Pause - Press any key...";
            char dummy_char;
            std::cin.get(dummy_char);
            page_entry_count = 0;
        }
        std::cout << entry;
    }

    c_AnnualReport annual_report = create_annual_report(sie_file_entries);

    // Dump the annual report
    std::cout << "\nAnnual Report - BEGIN";
    for (auto const& entry : annual_report) {
        std::cout << "\n" << entry;
    }
    std::cout << "\nAnnual Report - END";

    generate_rtf_file(sie_file_path,annual_report);

    // Exit
    std::cout << '\n';
    return 0;
}