//
//  CScribeCodes.h
//  Scribe2NeoScribe
//
//  Created by Jason Stoessel on 21/06/13.
//  Copyright (c) 2013 The Early Music eResearch System (THEMES). All rights reserved.
//

#ifndef Scribe2NeoScribe_CScribeCodes_h
#define Scribe2NeoScribe_CScribeCodes_h

#include <fstream>
#include <vector>
#include <unordered_map>
#include <map>

#include "Header.h"


class CScribeCodes {
    
public:
                    CScribeCodes(const std::string &csv_file);
                    CScribeCodes(const CScribeCodes &codes);
                    ~CScribeCodes();
    
    const std::string&    code_to_name(const std::string code) const;
    const bool      contains_code(const std::string& code) const;
    const bool      is_pitched_code(const std::string& code) const;
    const bool      is_ligature(const std::string& code) const;
    const code_t    get_code_type(const std::string& code) const;
    
    CScribeCodes&   operator=(const CScribeCodes& rhs);
    
    static std::unordered_map<int,std::string>       voice_labels;
private:
    void            load_csv_table(const std::string &csv_file);
    
    std::vector<std::vector<std::string> >          code_matrix;
    std::unordered_map<std::string, std::string>    code_name_map;
    std::unordered_map<std::string, bool>           code_pitchcode_map;
    std::multimap<std::string, std::string>         code_types;
    
};


#endif