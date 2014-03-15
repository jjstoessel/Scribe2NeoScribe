//
//  CSourceKey.cpp
//  Scribe2NeoScribe
//
//  Created by Jason Stoessel on 28/02/2014.
//  Copyright (c) 2014 The Early Music eResearch System (THEMES). All rights reserved.
//

#include "CSourceKey.h"
#include <vector>
#include <iterator>
#include <map>
#include <exception>
#include <string>
#include <sstream>
#include <utility>
#include <iostream>


CSourceKey::CSourceKey(const std::string &csv_file)
{
    load_csv_table(csv_file);
}

CSourceKey::CSourceKey(const CSourceKey &codes)
{
    *this = codes;
}

CSourceKey::~CSourceKey()
{
}

//  =========================================================================================
// load_csv_table
// input: name of neumcode csv FILE
// output: nil
// This function loads the contents of a neumcode csv file into memory
//  =========================================================================================

void    CSourceKey::load_csv_table(const std::string &csv_file)
{
    std::ifstream               file ( csv_file.c_str() );
    std::vector<std::string>    row;
    std::string                 line;
    std::string                 field;
    
    if (file.is_open())
        GETLINE(file, line); //skip header row
    else
        throw std::runtime_error("Could not open Source Codes file.");
    
    while (file)
    {
        GETLINE(file, line);
        std::stringstream lineStream(line);
        
        while ( std::getline(lineStream, field, '\t') ) //use the standard getline here - safe!
            row.push_back(field);
        
        
        if (!row.empty() )
            code_matrix.push_back(row);
        row.clear();
        line.clear();
    }
    
    // make a quickly accessible maps of code name pairs and pitch code states (pitched or not) for future reference (used by code_to_name);
    
    std::string siglum, source_name, RISM_abbrev;
    
    if (!code_matrix.empty()) {
        for (std::vector<std::vector<std::string>>::iterator i = code_matrix.begin(); i!=code_matrix.end(); i++){
            
            siglum = (*i)[0];
            source_name = (*i)[1];
            RISM_abbrev = (*i)[2];
            //delete first and last char which are ""
            source_name.pop_back();
            source_name.erase(0,1);
            
            siglum_source_key.insert(std::make_pair(siglum, source_name));
            siglum_RISM_key.insert(std::make_pair(siglum, RISM_abbrev));
            
        }
    }
    else
        throw std::runtime_error("Error reading Sources codes file");
    
}



//  =========================================================================================
//  getSourceName
//  input: string reference containing the siglum of MS
//  output: string reference containing the manuscipt details
//  =========================================================================================

const std::string& CSourceKey::getSourceName(const std::string code) const
{
    return siglum_source_key.at(code);
}

//  =========================================================================================
//  getRISMName
//  input: string reference containing the siglum of MS
//  output: String containing RISM abbreviation
//  =========================================================================================

const std::string& CSourceKey::getRISMName(const std::string code) const
{
    return siglum_RISM_key.at(code);
}

//  =========================================================================================
//  contains_code
//  input: string reference containing the siglum of MS
//  output: boolean result
//  =========================================================================================

const bool CSourceKey::contains_code(const std::string& code) const
{
    //std::unordered_map<std::string, std::string>::const_iterator got = code_name_map.find(code);
    
    //return (got!=code_name_map.end()); //if code is not in map, then map::end returned
    
    return siglum_source_key.count(code) > 0;
}


CSourceKey&   CSourceKey::operator=(const CSourceKey& rhs)
{
    code_matrix = rhs.code_matrix;
    siglum_source_key = rhs.siglum_source_key;
    siglum_RISM_key = rhs.siglum_RISM_key;
    
    return *this;
}


