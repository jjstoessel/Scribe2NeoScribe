//
//  Helpers.c
//  Scribe2NeoScribe
//
//  Created by Jason Stoessel on 23/07/13.
//  Copyright (c) 2013 The Early Music eResearch System (THEMES). All rights reserved.
//

#include "Header.h"

//getline replacement to deal with files from different platforms - currently works for windows and unix line ends
//http://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf

std::istream& safeGetline(std::istream& is, std::string& t)
{
    t.clear();
    
    // The characters in the stream are read one-by-one using a std::streambuf.
    // That is faster than reading them one-by-one using the std::istream.
    // Code that uses streambuf this way must be guarded by a sentry object.
    // The sentry object performs various tasks,
    // such as thread synchronization and updating the stream state.
    
    std::istream::sentry se(is, true);
    std::streambuf* sb = is.rdbuf();
    
    for(;;) {
        int c = sb->sbumpc();
        switch (c) {
            case '\n':
                return is;
            case '\r':
                if(sb->sgetc() == '\n')
                 sb->sbumpc();
                return is;
            case EOF:
                // Also handle the case when the last line has no line ending
                if(t.empty())
                    is.setstate(std::ios::eofbit);
                return is;
            default:
                t += (char)c;
        }
    }
}

//converts a number to a string with leading zeros
//http://www.cplusplus.com/forum/general/15952/

std::string ZeroPadNumber(int num, int width)
{
    std::ostringstream ss;
    ss << std::setw(width) << std::setfill('0') << num;
    std::string result = ss.str();
    if (result.length() > width)
    {
        result.erase(0, result.length() - width);
    }
    return result;
}
