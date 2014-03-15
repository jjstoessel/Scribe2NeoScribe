//
//  CScribeCodes.cpp
//  Scribe2NeoScribe
//
//  Created by Jason Stoessel on 21/06/13.
//  Copyright (c) 2013 The Early Music eResearch System (THEMES). All rights reserved.
//
//  This class reads in a csv file containing the codes used by John Stinson's Scribe software.
//
//  The neumcode.csv file has the following columes:
//      ascii - the corresponding ascii code for the scribe font glyph
//      character - the character corresponding to the font glyph
//      arguments - the number of pitches associated with the neume
//      code - the Scribe code used for entering data
//      pitchcode - True or false, some codes (e.g. '@'='switch gap off', i.e. form a compound neume. Gap is switched ON at the end of each                 syllable) and 'WBAR' %u2013 whole bar are formatting codes and are not related to pitches.
//      scoretype - Code for Score glyph type
//      scorecode - Score codenumbers for chant glyphs
//      name - the name of the element
//      lilycode - code used in lilypond
//      numspace - horizontal space of character, to assist in alignment of mensural parts in Score.
//      whitecode - Score codenumbers for white mensural glyphs
//      blackcode - Score codenumbers for black mensural glyphs
//      cursneume - reference to EPS encodings of neumes usually St Gall neumes
//      hilneume - reference to EPS encodings of "Hildegard" neumes
//      duration - these are for mensural notation, the unit is the brevis.
//      Confirmed by J.A Stinson 22 June 2013

#include "CScribeCodes.h"
#include <vector>
#include <iterator>
#include <map>
#include <exception>
#include <string>
#include <sstream>
#include <utility>
#include <iostream>

std::unordered_map<int,std::string> CScribeCodes::voice_labels({{ voice_type::unlabelled, "unlabelled" }, { voice_type::cantus, "cantus" }, { voice_type::contratenor,"contratenor" }, { voice_type::tenor,"tenor" }, { voice_type::tenor2, "tenor 2"}, { voice_type::triplum, "triplum" } });


CScribeCodes::CScribeCodes(const std::string &csv_file)
{
    load_csv_table(csv_file);
}

CScribeCodes::CScribeCodes(const CScribeCodes &codes)
{
    *this = codes;
}

CScribeCodes::~CScribeCodes()
{
}

//  =========================================================================================
// load_csv_table
// input: name of neumcode csv FILE
// output: nil
// This function loads the contents of a neumcode csv file into memory
//  =========================================================================================

void    CScribeCodes::load_csv_table(const std::string &csv_file)
{
    std::ifstream               file ( csv_file.c_str() );
    std::vector<std::string>    row;
    std::string                 line;
    std::string                 field;
    
    if (file.is_open())
        GETLINE(file, line); //skip header row
    else
        throw "Could not open CSV file.";
    
    while (file)
    {
        GETLINE(file, line); 
        std::stringstream lineStream(line);
        
        while ( std::getline(lineStream, field, ',') ) //use the standard getline here - safe!
            row.push_back(field);
        
        
        if (!row.empty() )
            code_matrix.push_back(row);
        row.clear();
        line.clear();
    }
    
    // make a quickly accessible maps of code name pairs and pitch code states (pitched or not) for future reference (used by code_to_name);
    
    std::string key, event_name, mei_type;
    bool is_pitch_code = false;
    
    if (!code_matrix.empty()) {
        for (std::vector<std::vector<std::string>>::iterator i = code_matrix.begin(); i!=code_matrix.end(); i++){
            
            key = (*i)[3];
            event_name = (*i)[7];
            mei_type = (*i)[15];
            
            code_name_map.insert(std::make_pair(key, event_name));
            if ((*i)[4]=="TRUE")
                is_pitch_code = true;
            else is_pitch_code = false;
            code_pitchcode_map.insert(std::make_pair(key, is_pitch_code));
            
            if (mei_type.empty()) {
                std::cout << "Error. Empty mei type" << std::endl;
            }
            
            std::pair<std::string, std::string> code_type_pair = std::make_pair(key, mei_type);
            code_types.insert(code_type_pair);
            
        }
    }
    else
        throw "Error reading in csv file";
    
}



//  =========================================================================================
//  code_to_name
//  input: string reference containing the SCRIBE code for a notational element
//  output: string reference contain the name of the notational element for use in XML
//  =========================================================================================

const std::string& CScribeCodes::code_to_name(const std::string code) const
{
    return code_name_map.at(code);
}

//  =========================================================================================
//  contains_code
//  input: string reference containing the SCRIBE code for a notational element
//  output: string reference contain the name of the notational element for use in XML
//  =========================================================================================

const bool CScribeCodes::contains_code(const std::string& code) const
{
    //std::unordered_map<std::string, std::string>::const_iterator got = code_name_map.find(code);

    //return (got!=code_name_map.end()); //if code is not in map, then map::end returned
    
    return code_name_map.count(code) > 0;
}

const bool CScribeCodes::is_pitched_code(const std::string& code) const
{
    bool is_pitched = false;
    
    is_pitched = code_pitchcode_map.at(code);
    
    return is_pitched;
}

CScribeCodes&   CScribeCodes::operator=(const CScribeCodes& rhs)
{
    code_matrix = rhs.code_matrix;
    code_name_map = rhs.code_name_map;
    code_types = rhs.code_types;
    
    return *this;
}

const code_t CScribeCodes::get_code_type(const std::string& code) const
{
    code_t type = code_t::other;
    
    if (code_types.count(code))
    {
        std::string type_name = code_types.find(code)->second;
        
        if (type_name=="note") type = code_t::note;
        if (type_name=="rest") type = code_t::rest;
        if (type_name=="ligature") type = code_t::ligature;
        if (type_name=="uneume") type = code_t::uneume;
        if (type_name=="ineume") type = code_t::ineume;
        if (type_name=="mensuration") type = code_t::mensuration;
        if (type_name=="clef") type = code_t::clef;
        if (type_name=="dot") type = code_t::dot;
        if (type_name=="barline") type = code_t::barline;
        if (type_name=="accidental") type = code_t::accidental;
    }
    
    return type;

}