//
//  CScribeReader.h
//  NeoScribe
//
//  Created by Jason Stoessel on 21/06/13.
//  Copyright (c) 2013 eMirabilis. All rights reserved.
//
//  This class reads SCRIBE files into memory in preparation for modification and/or conversion to other formats
//  To do: transform class into a scribe file i/o class for two-way conversion
//

#ifndef NeoScribe_CScribeReader_h
#define NeoScribe_CScribeReader_h

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "Header.h"
#include "tinyxml.h"
//#include "CScribeCodes.h"
class CScribeCodes; //forward declaration of class

//  Notes on four char row prefix/suffix in Scribe by John A. Stinson (20 June 2013)
//    The first and last four codes on each line are
//    1. The colour of the notation
//    2. The number of lines (4 being the default)
//    3. The clef
//    4. The line on which the clef appears.
//    The four characters at the end of the line should be the same as those at the beginning unless there is a change noted in the line. Thus the third character of the line is 'C' and within the line is 'F5' this indicates a change of clef from C to F on the second line from the bottom (lines are numbered from 1 (below the staff), then 3 5 7 9 on a four-line staff.
struct scribe_clef {
public:
    char clef;
    int clef_line;
    int staff_lines;
    
    const char get_pitch_name(const int pitch_loc);
    const int  get_octave(const int pitch_loc);
};

class scribe_staff_data
{
public:
    coloration_type     notation_colour = full_black; //full black notation default
    int     staff_lines = 4;
    char    clef;
    int     clef_line;
    
    const bool operator==(const scribe_staff_data& rhs) { return ((staff_lines==rhs.staff_lines) & (notation_colour==rhs.notation_colour) && (clef==rhs.clef) && (clef_line==rhs.clef_line)); }
    const bool operator!=(const scribe_staff_data& rhs) { if (*this==rhs) return false; else return true; }
};

//A scribe code can have several pitch numbers assigned to it;
//for a simple note, this indicates that same type of note appears for each pitch
//for a ligature, this indicate respective pitches of a ligature
class scribe_event {
public:
    std::string     code;
    bool            preceding_gap = true;
    coloration_type    local_coloration = full_black;
    std::vector<int>     pitch_num;
    
    void            clear() { code.clear(); preceding_gap = true; local_coloration = full_black; pitch_num.clear(); }
};

//A scribe row/line may contain several events, but only one syllable (although compound syllables also occur).
//A row may consist of either one or more events or comment. A comment always has * as the first char in the line).
struct scribe_row {
    scribe_staff_data           prefix;
    std::vector<scribe_event>   events;
    std::string                 syllable;
    std::string                 comment;
    bool                        is_comment = false;
    scribe_staff_data           suffix;
};


class scribe_part {
public:
    
    scribe_part();
    scribe_part(const scribe_part& part) { *this=part; }
    
    scribe_part&   operator=(const scribe_part& rhs);
    void        clear();
    
    std::string rep_num;            //trecento only - must be string (can have leading zeros)
    std::string title;              //trecento only
    std::string composer;           //trecento only
    std::string genre;              //trecento only
    int         num_voices;         //trecento only
    int         voice_type = voice_type::unlabelled; //trecento only
    std::string abbrev_ms;          //common
    std::string folios;             //common
    std::string feast;              //chant only
    std::string office;             //chant only
    int         cao_num = 0;        //chant only
    std::vector<scribe_row>  rows; //common
    
    
    scribe_staff_data   initial_staff_data;
    bool        initial_staff_data_set = false;
};

class CScribeReader {
public:
    
    CScribeReader(const std::string& scribe_file_name);
    ~CScribeReader();
    
    const bool          is_scribe_file() const { return (type==chant || type==trecento); }
    void                Print();
    void                PrintMEIXML(const std::string& encoder_name="John A. Stinson");
    void                SaveMEIXML(const std::string& file_name, const std::string& encoder_name="John A. Stinson");
    void                SegmentScribe2MEIXML(const std::string& encoder_name="John A. Stinson");
    
private:
    TiXmlDocument*      Scribe2MEIXML(const std::string& encoder_name);
    void                MEIXMLFileData(TiXmlElement* fileDesc, const scribe_part& partit, const std::string& encoder_name );
    TiXmlElement*       MEIXMLDeclaration(TiXmlDocument* doc);
    TiXmlElement*       Scribe2MEIXMLScore(TiXmlElement* mei);
    TiXmlElement*       Scribe2MEIXMLHeader (const std::string& encoder_name, TiXmlDocument* doc, TiXmlElement* mei);
    TiXmlElement*       Scribe2MEIXMLStaff(const scribe_part& partit, TiXmlElement* staffgrp, const int i);
    scribe_type         read_header();
    int                 load_scribe_file();
    std::string         read_part_header_field(const std::string& row, std::streampos& start, const size_t field_length);
    scribe_row          read_scribe_row(std::string raw_row);
    std::vector<int>    get_pitch_code(char& c, std::stringstream& lineStream);
    const std::string   get_ineume_part(const std::string& code, const int i, int& neume_notes);
    scribe_type         file_type() { return type; }
    
    static const        CScribeCodes*       trecento_codes;
    static const        CScribeCodes*       chant_codes;
    const               CScribeCodes*       codes;
    std::ifstream       file;
    
    scribe_type         type;

    std::vector<scribe_part>   parts; //all voices or parts
};

#endif
