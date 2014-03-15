//
//  CSourceKey.h
//  Scribe2NeoScribe
//
//  Created by Jason Stoessel on 28/02/2014.
//  Copyright (c) 2014 The Early Music eResearch System (THEMES). All rights reserved.
//

#ifndef __Scribe2NeoScribe__CSourceKey__
#define __Scribe2NeoScribe__CSourceKey__

#include <iostream>

#include <fstream>
#include <vector>
#include <unordered_map>
#include <map>

#include "Header.h"


class CSourceKey {
    
public:
    CSourceKey(const std::string &csv_file);
    CSourceKey(const CSourceKey &codes);
    ~CSourceKey();
    
    const std::string&      getSourceName(const std::string code) const;
    const std::string&      getRISMName(const std::string code) const;
    const bool              contains_code(const std::string& code) const;
    CSourceKey&             operator=(const CSourceKey& rhs);
    
   
private:
    void            load_csv_table(const std::string &csv_file);
    
    std::vector<std::vector<std::string> >            code_matrix;
    std::unordered_map<std::string,std::string>       siglum_source_key;
    std::unordered_map<std::string, std::string>      siglum_RISM_key;
    
};


#endif /* defined(__Scribe2NeoScribe__CSourceKey__) */
