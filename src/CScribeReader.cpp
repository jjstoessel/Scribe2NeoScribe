/*---------------------------------------------------------------------------------------------------------
    File:       CScribeReader.cpp
    Project:    NeoScribe
 
    Created by Jason Stoessel on 21/06/13.
    Copyright (c) 2013 eMirabilis. All rights reserved.
 
    History
 
    21 JUNE 2013:   Commenced - scribe parser
    12 JAN 2913     Scribe2MEIXML function split into sub routines to permit addition call functions

    To do:
    Split into ScribeReader and NeoScribe class using visitor model
---------------------------------------------------------------------------------------------------------*/

#include "CScribeReader.h"
#include "CScribeCodes.h"
#include "tinyxml.h"
#include <exception>
#include <sstream>
#include <iterator>
#include "Header.h"



//static class instantiation - once only
const CScribeCodes* CScribeReader::trecento_codes = new CScribeCodes(TRECENTO_CODES_CSV); //new CScribeCodes("trecento_neumcode.csv");
const CScribeCodes* CScribeReader::chant_codes = new CScribeCodes(CHANT_CODES_CSV); //new CScribeCodes("chant_neumcode.csv");


CScribeReader::CScribeReader(const std::string& scribe_file_name)
: file(scribe_file_name)
{
    if (file.is_open())
    {
        read_header();
        if (is_scribe_file()) load_scribe_file();
    }
    else
        std::cout << "Could not open nominated Scribe file.";
}

CScribeReader::~CScribeReader()
{
    file.close();
}

scribe_type  CScribeReader::read_header()
{
    std::string h_line;
    GETLINE(file,h_line);
    //GETLINE(file,h_line); //9 January testing on John's recent exports - additional empty line appears that should be ignored
    //set type and point at codes
    if (h_line == scribe_chant)
    {
        type = chant;
        codes = chant_codes;
    }
    else if (h_line == scribe_trecento)
    {
        type = trecento;
        codes = trecento_codes;
    }
    else
    {
        throw "bad file type";
    }
    
    return type;
}

//Primary function that reads in header and calls a function to read each row of a scribe file
int CScribeReader::load_scribe_file()
{
    int numparts = 0;
    scribe_part part;
    std::string row;
    std::string field;
    
    //read in second row containing metadata; header has already been read so file pointer should be pre-line 2 (checking required)
    GETLINE(file,row);
    
    while (file.good())
    {
        std::stringstream lineStream(row);
        
        if (lineStream.get()!='>') throw "metadata not present."; //make sure metadata is present
        
        std::streampos  pos = 1; //allow for leading '>'
        
        if (type == trecento){ //read trecento header - NB not tab delimited, but standard char widths
            
            std::string nvox_str;
            
            //auditing will be required for each
            part.rep_num =  read_part_header_field(row, pos,rep_num_length);
            part.title = read_part_header_field(row, pos, title_length);
            part.composer = read_part_header_field(row, pos, composer_length);
            part.genre = read_part_header_field(row, pos, genre_length);
            nvox_str = read_part_header_field(row, pos, voice_count_length);
            part.num_voices = std::atoi(nvox_str.c_str());
            part.abbrev_ms = read_part_header_field(row, pos, ms_abbrev_length);
            part.folios = read_part_header_field(row, pos, folio_length);
            part.voice_type = read_part_header_field(row, pos, 1).c_str()[0];
        }
        else if (type == chant) { // read in chant header, again using standard width fields, not tab delimited.
            
            std::string cao_str;
            
            part.abbrev_ms = read_part_header_field(row, pos,ms_abbrev_length);
            part.feast  = read_part_header_field(row, pos,title_length);
            part.office  = read_part_header_field(row, pos, office_length);
            part.genre  = read_part_header_field(row, pos, chant_type_length); //genre holds the item data for chant type
            part.folios  = read_part_header_field(row, pos, folio_length);
            cao_str = read_part_header_field(row, pos, cao_num_length);
            part.cao_num = std::atoi(cao_str.c_str());

        }
        row.clear();
        
        // read in next row and pass to parser
        // next line will be a LINE token if staff has more or less than the default four lines
        // otherwise is will be a clef token
        GETLINE(file,row);
        
        //pre-fetch number of staff lines, so this event is not stored except in part::staff_lines
        if (row.find("LINE")!=std::string::npos) {
            scribe_row line = read_scribe_row(row);
            part.initial_staff_data.staff_lines = line.events[0].pitch_num[0]; //should countain number of lines
            GETLINE(file,row);
        }
        
        //we don't want empty rows, header row, or rows that don't at least contain a suffix and prefix
        while (!file.eof() && !row.empty() && row[0]!='>' ) {
            scribe_row line = read_scribe_row(row);
            //only store filled rows
            if (line.is_comment || !line.events.empty())
            {
                part.rows.push_back(line);
            }
            row.clear();
            GETLINE(file,row);
        }
        
        //find first clef
        if (!part.rows.empty())
        {
            for (std::vector<scribe_row>::iterator r = part.rows.begin(); r!=part.rows.end(); r++) {
                if (!r->is_comment && (r->events[0].code=="C" || r->events[0].code=="F" || r->events[0].code=="G") ) {
                    char temp_clef = r->events[0].code[0];
                    part.initial_staff_data.clef = temp_clef;
                    part.initial_staff_data.clef_line = r->events[0].pitch_num[0];
                    break;
                }
            }
        }
        
        parts.push_back(part);
        part.clear();
        numparts++;
    }
    
    return numparts;
}

// returns substring field and pos incremented by field length
std::string CScribeReader::read_part_header_field(const std::string& row, std::streampos& start,  const size_t field_length)
{
    std::string field;
    
    field = row.substr(start, field_length);
    
    //trim trailing spaces
    while (std::isspace(field.back())) {
        field.pop_back();
    }
    
    start+=field_length;
    
    return field;
}

//Takes a line of Scribe data and parses it into events
scribe_row    CScribeReader::read_scribe_row(std::string raw_row)
{
    scribe_row      s_row;
    scribe_event    s_event;
    
    std::stringstream lineStream(raw_row);
    //get row length
    lineStream.seekg(-4,lineStream.end);
    std::streampos suffix_pos = lineStream.tellg();
    lineStream.seekg(0, lineStream.beg);
    
    //a row is either a comment or a set of events with a syllable
    if (lineStream.peek()=='*')
    { //comment row
        lineStream.get(); //get rid of '*'
        s_row.comment = lineStream.str().substr(lineStream.tellg()); // read comment (w/o) asterisk into row
        s_row.is_comment = true; // toggle default
    }
    else if (lineStream.tellg()==lineStream.beg) //a bit silly, but makes sure were are beginning of row
    {
        //read in four char prefix
        s_row.prefix.notation_colour = static_cast<enum coloration_type>(lineStream.get());
        s_row.prefix.staff_lines = lineStream.get();
        s_row.prefix.clef = lineStream.get();
        s_row.prefix.clef_line = lineStream.get();

        //process one or more events - all events are alphabetical or punctuation, or one or more chars
        std::string token;
        std::string p_number;
        char c = lineStream.get();
        
        //11 Jan 2014 deal extraneous space(s) between prefix and first event
        //while (isspace(c)) {
        //    c = lineStream.get();
        //}
        
        coloration_type current_color = s_event.local_coloration;
        
        //search in space between prefix and suffix for events/syllables
        while ( !lineStream.eof() && c!='\0' )
        {
            //first we find event prefix signs, these being those that specify gaps and coloration
            switch (c) {
                case '!':
                    s_event.preceding_gap = true;
                    c = lineStream.get();
                    if (c=='!') c = lineStream.get(); //sometimes '!' doubled - does it have a specific meaning?
                    if (isspace(c)) c = lineStream.get(); //sometimes '!' followed by a space - this seems to be because it is suffix to previous event.
                    break;
                case '@':
                    s_event.preceding_gap = false;
                    c = lineStream.get();
                    if (isspace(c)) {
                        c = lineStream.get(); //gobble up space if it follows this attribute "token"
                    }
                    break;
                case '+':
                    c = lineStream.get();
                    if (c=='-') { // '+-' and '-+' indicates void red coloration
                        c = lineStream.get();
                        s_event.local_coloration = void_red;
                    }
                    else
                        s_event.local_coloration = full_red;
                    break;
                case '-':
                    c = lineStream.get();
                    if (c=='+') { // '+-' and '-+' indicates void red coloration
                        c = lineStream.get();
                        s_event.local_coloration = void_red;
                    }
                    else
                        s_event.local_coloration = void_black;
                    break;
                case '=':
                    s_event.local_coloration = full_black; //change to see to default coloration
                    c = lineStream.get();
                    if (isspace(c)) {
                        c = lineStream.get(); //gobble up space if it follows this attribute "token"
                    }
                    break;
                case ';':
                    {
                        size_t length = suffix_pos-lineStream.tellg();
                        
                        try {
                            
                            char  *buffer = new char[length+1];
                            lineStream.read(&buffer[0],length);
                            buffer[length] = '\0'; //zero terminate to make into a real c string before passing to C++ string class to stop weirdness/buggyness
                            s_row.syllable = buffer;
                            c = lineStream.get();
                            delete buffer;
                        } catch (std::exception& e) {
                            throw e;
                        }
                        
                    }
                    break;
                default:
                    s_event.local_coloration = current_color;
                    if (isspace(c)) {
                        c = lineStream.get(); //two events without attributes will be separated by a space.
                    }
                    break;
            }
            
            current_color = s_event.local_coloration;
            
            //find tokens
            //tokens are composed only of upper case chars and a limted number of punctuation signs; '#' is the one exception
            while (isupper(c) || (c=='.') || (c=='\'') || (c=='#'))
            {
                token.push_back(c); //store token chars
                c = lineStream.get();
                if (lineStream.eof()) break;
            }
            //store token and get associated numbers (pitchs or staff locations)
            if (!token.empty() && codes->contains_code(token))
            {
                s_event.code = token;
                s_event.pitch_num = get_pitch_code(c, lineStream); //search recursively for number codes after a token.
                //store event if populated
                if (!s_event.code.empty())
                {
                    s_row.events.push_back(s_event); //must be finished with event; push event
                }
            }
            
            //get suffix
            if (lineStream.tellg() >= suffix_pos )
            {
                if (isspace(c)) c = lineStream.get(); //for some tokens like DBAR, a space is inserted between token and suffix. Gobble, gobble.
                s_row.suffix.notation_colour = static_cast<enum coloration_type>(c);
                s_row.suffix.staff_lines = lineStream.get();
                s_row.suffix.clef = lineStream.get();
                s_row.suffix.clef_line = lineStream.get();
                c = lineStream.get();
                //if (!lineStream.eof()) //in case of problem with newline character
                //    lineStream.setstate(std::ios::eofbit); //force loop termination
                break; //if we get to there, it's all over for a row.
            }
            
            s_event.clear();
            token.clear();
            p_number.clear();
        } 
    }
    
    return s_row;
}

//Recursively searches for codes (not necessarily pitch codes) after a token
//input: char last taken from lineStream, stringstream& linestream containing the row being parsed.
//output: vector of pitch numbers as ints
std::vector<int> CScribeReader::get_pitch_code(char& c, std::stringstream& lineStream)
{
    std::string number_str;
    std::vector<int> pitches;
    
    if (isnumber(c) || (c=='-' && isnumber(lineStream.peek())) || (isspace(c) && isnumber(lineStream.peek())))  //negative numbers may be used for ???; sometimes a token might have a space before the first number, though it shouldn't
    {
        do {
            number_str.push_back(c);
            c = lineStream.get();
        } while (isnumber(c) && !lineStream.eof());
        
        if (!number_str.empty())
        {
            pitches.push_back(std::atoi(number_str.c_str()));
            
            if (isspace(c)) {
                
                if (isspace(lineStream.peek())) c = lineStream.get(); //sometimes two spaces may appear before a token number
                std::vector<int> temp_pitches;
                c = lineStream.get();
                temp_pitches = get_pitch_code(c, lineStream);
                for (size_t i = 0; i < temp_pitches.size(); ++i) {
                    pitches.push_back(temp_pitches[i]);
                }
            }
        }
        
    }
    
    return pitches;
}

//===================================================================================================
//
//  Function: Print
//  Purpose: outputs Scribe data to screen
//  Used by: none - public function call
//
//===================================================================================================
void CScribeReader::Print()
{
    for (std::vector<scribe_part>::iterator part = parts.begin(); part != parts.end() ; part++)
    {
        //print metadata
        for (std::vector<scribe_row>::iterator row = part->rows.begin(); row != part->rows.end(); row++) {
            
            //print row prefix
            
            //print comment or row contents (events, syllables)
            if (row->is_comment)
            {
                std::cout << '*' << row->comment;
            }
            else
            {
                for (std::vector<scribe_event>::iterator event = row->events.begin(); event != row->events.end(); event++) {
                    std::cout << event->code;
                    size_t n = event->pitch_num.size();
                    for (std::vector<int>::iterator pn = event->pitch_num.begin() ; pn != event->pitch_num.end(); pn++ ) {
                        std::cout << *pn;
                        --n;
                        if (n > 0) {
                            std::cout << ' ';
                        }
                    }
                }
                //print row syllable
                if (!row->syllable.empty()) {
                    std::cout << ';' << row->syllable;
                }
            }
            
            //print row suffix
            
            //add newline to end of suffix
            std::cout << std::endl;
        }
    }
    
}


//===================================================================================================
//
//  Function: SaveMEIXML
//  Purpose: Outputs XML representation of a Scribe file converted to MEI compliant format with extensions
//  Input: encoder name (std::string)
//  Output: screen print
//  Used by: none - public function call
//
//===================================================================================================
void CScribeReader::PrintMEIXML(const std::string& encoder_name)
{
    TiXmlDocument* doc = Scribe2MEIXML(encoder_name);
    
    doc->Print();
    
    delete doc;
}

//===================================================================================================
//
//  Function: SaveMEIXML
//  Purpose: Converts in memory data from Scribe file to NeoScribe XML and saves to named file
//  Input: name of file to save to, name of original Scribe encoder
//  Output: file containing NeoScribe MEI-compliant XML
//  Used by: none - public function call
//
//===================================================================================================

void CScribeReader::SaveMEIXML(const std::string& file_name, const std::string& encoder_name)
{
    TiXmlDocument* doc = Scribe2MEIXML(encoder_name);
    
    doc->SaveFile(file_name.c_str());
    
    delete doc;
    
}
//===================================================================================================
//
//  Function:   Scribe2MEIXML
//  Purpose:    Builds MEI XML representation of a Scribe file converted to MEI-compliant format with extensions.
//              Produces a single file reflecting the content of the file input into CScribeReader. Works
//              best for Scribe files containing one part, but will convert composite Scribe data
//  Used by:    SaveMEIXML
//  Inputs:     encoder_name, string naming original Scribe encoder
//
//===================================================================================================

TiXmlDocument* CScribeReader::Scribe2MEIXML(const std::string& encoder_name)
{
    
    TiXmlDocument* doc = new TiXmlDocument();
    
    TiXmlElement* mei = MEIXMLDeclaration(doc);
    
    //Create MEIhead get file descriptor
    TiXmlElement* fileDesc = Scribe2MEIXMLHeader(encoder_name, doc, mei);
    
    //Create the MEIscore, part of music data
    TiXmlElement* score = Scribe2MEIXMLScore(mei);
    
    //start adding score definitions - child of score
    TiXmlElement* scoredef = new TiXmlElement("scoreDef");
    score->LinkEndChild(scoredef);
    TiXmlElement* staffgrp = new TiXmlElement("staffGrp");
    scoredef->LinkEndChild(staffgrp);
    staffgrp->SetAttribute("id", "all");
    
    int i=0;
    
    for (std::vector<scribe_part>::iterator partit = parts.begin(); partit!=parts.end(); partit++)
    {
        ++i;
        if (i==1)
        {
            MEIXMLFileData(fileDesc, *partit, encoder_name );
        }
        
        //add section - child of score
        TiXmlElement* section = new TiXmlElement("section");
        score->LinkEndChild(section);
        
        //handle staff and link to section
        TiXmlElement* staff = Scribe2MEIXMLStaff(*partit, staffgrp, i);
        section->LinkEndChild(staff); //or TiXmlElement* layer;?

    }

    return doc;
}

//===================================================================================================
//
//  Function:   SegmentScribe2MEIXML
//  Purpose:    Builds MEI XML representation of a Scribe file converted to MEI-compliant format with extensions.
//              Produces a series of files reflecting the content of the file input into CScribeReader. Works
//              best for Scribe files containing more than one part or piece
//  Used by:    none - public function call
//  Inputs:     encoder_name, string naming original Scribe encoder
//
//===================================================================================================

void CScribeReader::SegmentScribe2MEIXML(const std::string& encoder_name)
{

    for (std::vector<scribe_part>::iterator part = parts.begin(); part != parts.end(); part++)
    {
        int i = 0;
        
        //save record of first part
        scribe_part first_part = *part;
        
        //create an instance of XML doc representation, etc.
        TiXmlDocument* doc = new TiXmlDocument();
        
        TiXmlElement *mei = MEIXMLDeclaration(doc);
        
        //Create MEIhead get file descriptor
        TiXmlElement* fileDesc = Scribe2MEIXMLHeader(encoder_name, doc, mei);
        
        //Create the MEIscore, part of music data
        TiXmlElement* score = Scribe2MEIXMLScore(mei);
        
        //start adding score definitions - child of score
        TiXmlElement* scoredef = new TiXmlElement("scoreDef");
        score->LinkEndChild(scoredef);
        TiXmlElement* staffgrp = new TiXmlElement("staffGrp");
        scoredef->LinkEndChild(staffgrp);
        staffgrp->SetAttribute("id", "all");
        
        //skip along and collect parts - all have the same REPNUM or TITLE
        do
        {
            ++i;
            
            if (i==1)
            {
                MEIXMLFileData(fileDesc, *part, encoder_name );
            }
            //add section - child of score
            TiXmlElement* section = new TiXmlElement("section");
            score->LinkEndChild(section);
            
            //handle staff and link to section
            TiXmlElement* staff = Scribe2MEIXMLStaff(*part, staffgrp, i);
            section->LinkEndChild(staff); //or TiXmlElement* layer;?
            
            part++;
            
        } while ( part->rep_num==first_part.rep_num && part != parts.end()); //} while ( part->title==first_part.title && part != parts.end());
        
        std::string xml_file_name = first_part.rep_num + ".xml";
        
        doc->SaveFile(xml_file_name);
        
        delete doc; // destructs all associated objects created herein
        
        part--; // step back to last part in piece
    }
    
}

TiXmlElement* CScribeReader::MEIXMLDeclaration(TiXmlDocument* doc)
{
    TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
    doc->LinkEndChild(decl);
    TiXmlElement* mei = new TiXmlElement( "mei");
    doc->LinkEndChild(mei);
    mei->SetAttribute("xmlns", "http://www.music-encoding.org/ns/mei" );
    mei->SetAttribute("xmlns:xlink", "http://www.w3.org/1999/xlink");
    mei->SetAttribute("meiversion", "2013");

    return mei;
}

//===================================================================================================
//
//  Function: Scribe2MEIXMLHeader
//  Purpose: utility function to create MEI header structure
//  Used by: Scribe2MEIXML
//
//===================================================================================================
TiXmlElement* CScribeReader::Scribe2MEIXMLHeader (const std::string& encoder_name, TiXmlDocument* doc, TiXmlElement* mei)
{
  
    //<meiHead>
    TiXmlElement* mei_head = new TiXmlElement("meiHead");
    mei->LinkEndChild(mei_head);
    //mei_head - <fileDesc>
    TiXmlElement* fileDesc = new TiXmlElement("fileDesc");
    mei_head->LinkEndChild(fileDesc);
    
    //fileDesc - <pubStmt>
    TiXmlElement* pubStmt = new TiXmlElement("pubStmt");
    fileDesc->LinkEndChild(pubStmt);
    //fileDesc - <seriesStmt>
    TiXmlElement* seriesStmt = new TiXmlElement("seriesStmt");
    fileDesc->LinkEndChild(seriesStmt);
    
    //encodingDesc
    TiXmlElement* encodingDesc = new TiXmlElement("encodingDesc");
    mei_head->LinkEndChild(encodingDesc);
    TiXmlElement* appInfo = new TiXmlElement("appInfo");
    encodingDesc->LinkEndChild(appInfo);
    TiXmlElement* application = new TiXmlElement("application");
    appInfo->LinkEndChild(application);
    application->SetAttribute("xml:id", "xsl_scribe2neoscribexml");
    application->SetAttribute("version", "0.1");
    TiXmlElement* app_name = new TiXmlElement("name");
    application->LinkEndChild(app_name);
    TiXmlText* name = new TiXmlText("Scribe2NeoScribeXML");
    app_name->LinkEndChild(name);
    
    //workDesc
    TiXmlElement* workDesc = new TiXmlElement("workDesc");
    mei_head->LinkEndChild(workDesc);
    TiXmlElement* work = new TiXmlElement("work");
    workDesc->LinkEndChild(work);
    
    //</meiHead> - not really at this stage - other elements completed in main routine
    
    return fileDesc;

}

void    CScribeReader::MEIXMLFileData(TiXmlElement* fileDesc, const scribe_part& partit, const std::string& encoder_name )
{
    //title element and dependencies - part of fileDesc area of mei header
    //at this point of time, title and creator elements reside in the header and must be filled in the part iteration.
    //titleStmt
    TiXmlElement* titleSmt = new TiXmlElement("titleSmt");
    fileDesc->LinkEndChild(titleSmt);
    TiXmlElement* title = new TiXmlElement("Title"); //will fill below in clunky fashion
    titleSmt->LinkEndChild(title);
    TiXmlElement* respStmt = new TiXmlElement("respStmt");
    titleSmt->LinkEndChild(respStmt);
    TiXmlElement* creator = new TiXmlElement("persName");
    creator->SetAttribute("role", "creator");
    respStmt->LinkEndChild(creator);
    TiXmlElement* encoder = new TiXmlElement("persName");
    encoder->SetAttribute("role", "encoder");
    TiXmlText* name = new TiXmlText(encoder_name);
    encoder->LinkEndChild(name);
    respStmt->LinkEndChild(encoder);
    
    //fileDesc - <sourceDesc> - part of fileDesc area of mei header
    //ultimately source Desc must be linked to the music division element to permit multiple sources in the one file. Currently they reside in the header. New handing needed that is more multisource oriented
    TiXmlElement* sourceDesc = new TiXmlElement("sourceDesc");
    fileDesc->LinkEndChild(sourceDesc);
    TiXmlElement* source = new TiXmlElement("source");
    sourceDesc->LinkEndChild(source);
    TiXmlElement* physLoc = new TiXmlElement("physLoc");
    source->LinkEndChild(physLoc); //place for placing city, archive, shelfnumber
    //</meihead>
    
    TiXmlText* text = new TiXmlText(partit.title);
    title->LinkEndChild(text);
    TiXmlText* creator_name = new TiXmlText(partit.composer);
    creator->LinkEndChild(creator_name);
    TiXmlText* ms = new TiXmlText(partit.abbrev_ms); //this is temp fix - full ms detail here
    source->SetAttribute("n", partit.abbrev_ms); //this wil need to handle multiple sources
    source->SetAttribute("label", "manuscript");
    physLoc->LinkEndChild(ms);
    
}

//===================================================================================================
//
//  Function: Scribe2MEIXMLScore
//  Purpose: utility function to create MEI score structure
//  Used by: Scribe2MEIXML
//
//===================================================================================================

TiXmlElement* CScribeReader::Scribe2MEIXMLScore(TiXmlElement* mei)
{
    //music - contains all music data
    TiXmlElement* music = new TiXmlElement("music");
    mei->LinkEndChild(music);
	TiXmlElement* mdiv = new TiXmlElement("mdiv"); //for chant source this needs to be specified repeatedly, with n and type attributes
    music->LinkEndChild(mdiv);
    TiXmlElement* score = new TiXmlElement("score");
    mdiv->LinkEndChild(score);
    
    return score;
}


//===================================================================================================
//
//  Function: Scribe2MEIXMLStaff
//  Purpose: utility function to create MEI staff structure from scribe parts
//  Used by: Scribe2MEIXML
//  Inputs: scribe_part, staffgrp XML element pointer, i = count of parts in processed score
//
//===================================================================================================

TiXmlElement* CScribeReader::Scribe2MEIXMLStaff(const scribe_part& partit, TiXmlElement* staffgrp, const int i)
{
    
    std::string staffnum("s");
    staffnum += std::to_string(i); //autogenerate staff ids
    
    //finish score definitions
    //add staff definition for current part
    //NB. look to alternatively embedding these in staffs
    TiXmlElement* staffdef = new TiXmlElement("staffDef");
    
    //define staff from data
    staffgrp->LinkEndChild(staffdef);
    staffdef->SetAttribute("id", staffnum);
    staffdef->SetAttribute("lines", partit.initial_staff_data.staff_lines);
    staffdef->SetAttribute("label", CScribeCodes::voice_labels[partit.voice_type].c_str());
    
    //define clef from data
    TiXmlElement* clef = new TiXmlElement("clef");
    staffdef->LinkEndChild(clef);
    
    scribe_clef loc_clef;
    loc_clef.clef_line = partit.initial_staff_data.clef_line;
    loc_clef.clef = partit.initial_staff_data.clef;
    //convert scribe-based clef position to mei
    int mei_clef_line = ((loc_clef.clef_line + 1)/2) - 1;
    //4-line staves are numbered 3, 5, 7, 9 in scribe
    //if (partit->staff_lines<=4) mei_clef_line -= (5 - partit->staff_lines);
    
    clef->SetAttribute("line", mei_clef_line); //these need to be set for each staff/part
    clef->SetAttribute("shape", std::string(&(partit.initial_staff_data.clef),1).c_str()); //the first event in stored memory should be the initial clef
    
    TiXmlElement* staff = new TiXmlElement("staff");

    staff->SetAttribute("id", staffnum);
    staff->SetAttribute("source", partit.abbrev_ms);
    //folio on which part appears
    TiXmlElement* pb = new TiXmlElement("pb");
    staff->LinkEndChild(pb);
    pb->SetAttribute("n", partit.folios);
    //first staff on which part appears
    TiXmlElement* sb = new TiXmlElement("sb");
    sb->SetAttribute("n", "0"); //set to "0" since this isn't encoded in scribe
    staff->LinkEndChild(sb);
    
    coloration_type current_color = coloration_type::full_black; // this needs to be better handled with a default coloration in a part
    
    for (std::vector<scribe_row>::const_iterator rowit = partit.rows.begin(); rowit!=partit.rows.end(); rowit++)
    {
        if (rowit->is_comment) {
            //check it this is the correct way to handle a comment
            TiXmlComment* comment = new TiXmlComment(rowit->comment.c_str());
            staff->LinkEndChild(comment);
            //NB. syl can have a type (eg. initial) attribute and also encode color as <rend> child element
        } else {
            
            //syllable container (holds syllables, notes, neumes, and ligatures)
            TiXmlElement* syllable = new TiXmlElement("syllable");
            staff->LinkEndChild(syllable);
            
            //add actual syllable if present
            if (!rowit->syllable.empty()) {
                TiXmlElement* syl = new TiXmlElement("syl");
                TiXmlText* text = new TiXmlText(rowit->syllable);
                syl->LinkEndChild(text);
                syllable->LinkEndChild(syl);
            }
            
            //extract events - notes, rests ligatures, uneumes and/or ligatures
            for (std::vector<scribe_event>::const_iterator eventit = rowit->events.begin(); eventit!=rowit->events.end(); eventit++ )
            {
                current_color = eventit->local_coloration;
                
                //use temp TiXML pointer which is either syllable, uneume/ineume or ligature - add notes to this, but make sure that uneume/inueme/ligature pointer is preinserted into syllable
                //handle events for each row
                code_t event_type = codes->get_code_type(eventit->code);
                //foster parent will change roles according to child elements that need to be added
                TiXmlElement* foster = syllable;
                
                switch (event_type)
                {
                    case code_t::ineume:
                    {
                        TiXmlElement* ineume = new TiXmlElement("ineume");
                        ineume->SetAttribute("name", codes->code_to_name(eventit->code));
                        foster->LinkEndChild(ineume);
                        foster = ineume;
                        goto do_note;
                        break;
                    }
                    case code_t::uneume:
                    {
                        TiXmlElement* uneume = new TiXmlElement("uneume");
                        uneume->SetAttribute("name", codes->code_to_name(eventit->code));
                        foster->LinkEndChild(uneume);
                        foster = uneume;
                        goto do_note;
                        break;
                    }
                    case code_t::ligature:
                    {
                        TiXmlElement* ligature = new TiXmlElement("ligature");
                        ligature->SetAttribute("name", codes->code_to_name(eventit->code));
                        foster->LinkEndChild(ligature);
                        foster = ligature;
                        if (current_color!=coloration_type::full_black)
                        {
                            switch (current_color) {
                                case full_red:
                                    ligature->SetAttribute("color", "red");
                                    break;
                                case void_red:
                                    ligature->SetAttribute("color", "red");
                                    ligature->SetAttribute("void", "true"); //this seems a sensible solution for this problem and a suitable additional attribute to the MEI description
                                    break;
                                case void_black:
                                    ligature->SetAttribute("void", "true"); //**
                                    break;
                                case full_blue:
                                    ligature->SetAttribute("color", "blue");
                                    break;
                                default:
                                    break;
                            }
                        }
                        //goto do_note;
                        //break;
                    }
                    case code_t::note:
                    do_note:
                        for (auto i = eventit->pitch_num.begin(); i!=eventit->pitch_num.end(); i++)
                        {
                            int note_count = 1;
                            TiXmlElement* temp_foster = foster;
                            
                            //handle note attributes according to type
                            if ( foster->ValueStr()=="ineume" )
                            {
                                TiXmlElement* temp_uneume = new TiXmlElement("uneume");  //handle ineumes
                                //find uneume names
                                temp_uneume->SetAttribute("name", get_ineume_part(eventit->code, i - eventit->pitch_num.begin(), note_count));
                                foster->LinkEndChild(temp_uneume);
                                temp_foster = temp_uneume; //allow notes to be children of uneume
                            } else if ((eventit->code=="B" || eventit->code=="V" || eventit->code=="L") && eventit->pitch_num.size()>1 && type==chant && i!=eventit->pitch_num.begin()) //codes like virga and punctum may be followed by several pitch numbers, indicating a sequence of simple neumes
                            {
                                TiXmlElement* temp_uneume = new TiXmlElement("uneume");
                                temp_uneume->SetAttribute("name", codes->code_to_name(eventit->code));
                                foster->Parent()->LinkEndChild(temp_uneume); //link to syllable element, not uneume!
                                temp_foster = temp_uneume;
                            }
                            //also handle ligatures
                            auto j = 0;
                            //insert note or notes for unneumes in ineumes
                            for (; j < note_count ; j++)
                            {
                                //convert note location to pitch name
                                // need to handle dots as element rather than attribute?
                                char pitch_name = loc_clef.get_pitch_name(*i);
                                int octave = loc_clef.get_octave(*i);
                                
                                TiXmlElement* note = new TiXmlElement("note");
                                if (foster->ValueStr()=="syllable")
                                {
                                    note->SetAttribute("dur", codes->code_to_name(eventit->code));
                                }
                                note->SetAttribute("pname", std::string(&pitch_name,1));
                                note->SetAttribute("oct", octave);
                                if (current_color!=coloration_type::full_black)
                                {
                                    switch (current_color) {
                                        case full_red:
                                            note->SetAttribute("color", "red");
                                            break;
                                        case void_red:
                                            note->SetAttribute("color", "red");
                                            note->SetAttribute("void", "true"); //this seems a sensible solution for this problem and a suitable additional attribute to the MEI description
                                            break;
                                        case void_black:
                                            note->SetAttribute("void", "true"); //**
                                            break;
                                        case full_blue:
                                            note->SetAttribute("color", "blue");
                                            break;
                                        default:
                                            break;
                                    }
                                }
                                temp_foster->LinkEndChild(note);
                            }
                            i+=j-1;
                        }
                        break;
                        //dot needs to be handled as a unique element in our extended definition
                        /*Stinson, 7 July 2013: Further notes on DOT
                         The code has two arguments: the first is the substantive position on the staff; the second refines that position up or down and is capable of five variants: 0 = exactly on the line or exactly in the middle of the space between the lines; -1 = 0.1 of the space between the lines below (or for +, above) the space or line; -2= 0.2 of the space below the normal position for that line or space; -3 = 0.3 below the normal position; -4 = 0.4 below the normal position.
                         Only values between 0 and 4 are permitted as the second argument as 5 would be the equivalent of having the dot on the next numbered line or space, e.g. '7 -5', if it were permitted, would be the same as '6 0'.*/
                    case code_t::dot:
                    {
                        TiXmlElement* dot = new TiXmlElement("dot");
                        char pitch_name = loc_clef.get_pitch_name(eventit->pitch_num[0]);
                        int octave = loc_clef.get_octave(eventit->pitch_num[0]);
                        dot->SetAttribute("ploc", std::string(&pitch_name,1));
                        dot->SetAttribute("oloc", octave);
                        /*vo: records the vertical adjustment of a feature's programmatically-determined location in terms of staff interline distance; that is, in units of 1/2 the distance between adjacent staff lines. (MEI2013)*/
                        //only set for non-defult positions
                        if (eventit->pitch_num[1]!=0)
                        {
                            float v_pos = eventit->pitch_num[1]/10.0*2;
                            dot->SetAttribute("vo", to_string_with_precision(v_pos,1));
                        }
                        foster->LinkEndChild(dot);
                    }
                        break;
                    case code_t::rest:
                    {
                        TiXmlElement* rest = new TiXmlElement("rest");
                        foster->LinkEndChild(rest);
                        switch (*(eventit->code.c_str())) {
                                //rests of type 'R' has two associated pitch numbers from which we might infer the type
                            case generic_rest:
                            {
                                int end = eventit->pitch_num[0];
                                int start = eventit->pitch_num[1];
                                int rest_type = start-end;
                                switch (rest_type) {
                                    case minim_rest:
                                        rest->SetAttribute("type", "minima");
                                        break;
                                    case semibreve_rest:
                                        rest->SetAttribute("type", "semibrevis");
                                        break;
                                    case breve_rest:
                                        rest->SetAttribute("type", "brevis");
                                        break;
                                    case long_rest:
                                        rest->SetAttribute("type", "longa imperfecta");
                                        break;
                                    case perf_long_rest:
                                        rest->SetAttribute("type", "long perfecta");
                                        break;
                                    default:
                                        break;
                                }
                                char ploc = loc_clef.get_pitch_name(start);
                                rest->SetAttribute("ploc", std::string(&ploc,1));
                                rest->SetAttribute("oloc", loc_clef.get_octave(start));
                                break;
                                //also process 'RSM', semiminim rest
                            }
                                //case semiminim_rest:
                                //rest->SetAttribute("type", "semiminim");
                                //    break;
                            default:
                                rest->SetAttribute("type", codes->code_to_name(eventit->code));
                                if (!eventit->pitch_num.empty()) {
                                    char pitch_name = loc_clef.get_pitch_name(eventit->pitch_num[0]);
                                    int octave = loc_clef.get_octave(eventit->pitch_num[0]);
                                    rest->SetAttribute("ploc", std::string(&pitch_name,1));
                                    rest->SetAttribute("oloc", octave);
                                }
                                break;
                        }
                    }
                        break;
                    case code_t::mensuration:
                    {
                        TiXmlElement* mensuration_sign = new TiXmlElement("mensur");
                        foster->LinkEndChild(mensuration_sign);
                        
                        if (eventit->code == "MO" || eventit->code == "MC" || eventit->code == "MO." || eventit->code == "MC.")
                        {
                            char the_sign = eventit->code[1];
                            mensuration_sign->SetAttribute("sign", std::string(&the_sign,1));
                            if (eventit->code.size()==3 && eventit->code[2]=='.') {
                                mensuration_sign->SetAttribute("dot", "true");
                            }
                            //also able to set attribute 'orient' to reversed for reversed signs; and slash attribute for cut signs
                        }
                        if (eventit->code == ".D." || eventit->code == ".Q." || eventit->code == ".SI." || eventit->code == ".P." || eventit->code == ".N." || eventit->code == ".O." || eventit->code == ".I.") //also .SG.?
                        {
                            //these are wholly new to the MEI schema; the whole dot-letter-dot sign is encoded
                            mensuration_sign->SetAttribute("sign", eventit->code);
                        }
                        break;
                    }
                    case code_t::barline:
                    {
                        TiXmlElement* barline = new TiXmlElement("barline");
                        foster->LinkEndChild(barline);
                        if (eventit->code == "QBAR") {
                            
                            barline->SetAttribute("rend", "quarter"); //non-standard data type for rend.
                        }
                        if (eventit->code == "HBAR") {
                            
                            barline->SetAttribute("rend", "half");
                        }
                        if (eventit->code == "WBAR") {
                            
                            barline->SetAttribute("rend", "single");
                        }
                        if (eventit->code == "DBAR") {
                            
                            barline->SetAttribute("rend", "dbl");
                        }
                        
                        //modern bar editorial - ignore?
                        if (eventit->code == "MBAR") {
                            
                            barline->SetAttribute("barplace", "takt");
                            barline->SetAttribute("taktplace", 9);
                        }
                        
                        //also able to see rend attribute
                        
                        break;
                    }
                    case code_t::clef:
                    {
                        //NB. old clefchange element superceded; clefGrp used for simultaneous clefs
                        loc_clef.clef_line = *(eventit->pitch_num.begin());
                        loc_clef.clef = *(eventit->code.c_str());
                        TiXmlElement* clef = new TiXmlElement("clef");
                        clef->SetAttribute("line", ((loc_clef.clef_line + 1)/2) - 1);
                        clef->SetAttribute("shape", std::string(&(loc_clef.clef),1).c_str());
                        foster->LinkEndChild(clef);
                        break;
                    }
                        //The MEI accidental names are not used: diesis, b-rotundum and b-quadratum are. Other variants may be added. All ms accidentials in Scribe (as they should be in NeoScribe) are independent elements
                    case code_t::accidental:
                    {
                        TiXmlElement* accid = new TiXmlElement("accid");
                        accid->SetAttribute("accidental", codes->code_to_name(eventit->code));
                        if (!eventit->pitch_num.empty()) {
                            char ploc = loc_clef.get_pitch_name(eventit->pitch_num[0]);
                            int oloc = loc_clef.get_octave(eventit->pitch_num[0]);
                            accid->SetAttribute("ploc", std::string(&ploc,1));
                            accid->SetAttribute("oloc", oloc);
                        }
                        foster->LinkEndChild(accid);
                        break;
                    }
                    case code_t::other:
                        //lucunae
                    default:
                        break;
                }
                
            }
            
        }
    }
    
    return staff;
}

const std::string CScribeReader::get_ineume_part(const std::string& code, const int i, int& neume_notes)
{
    std::string return_code("rhomboid");
    neume_notes = 1;

    if (code == "CM" || code == "CMS" || code == "CMSS" || code == "CM6" || code == "CM7" || code == "CM7" || code == "CM8") {
        if (i==0) {
            return_code = "virga"; //better to call these from the code table
            return return_code;
        }
    }
    
    
    if (code == "PS" || code == "PSS" || code == "PSSS") {
        if (i<2) {
            return_code = "podatus";
            neume_notes = 2;
            return return_code;
        }
    }

    //if (code == "SC") {
    
    //}

    if (code == "SC'") {
        if (i<2) {
            return_code = "podatus";
            neume_notes = 2;
        }
        if (i==2) {
            return_code = "virga";
        }
        return return_code;
    }

    if (code == "SQ") {
        if (i==1){
            return_code = "quilisma";
        }
        return return_code;
    }

    if (code == "TQ4") { //torculus subpunctus
        if (i<3) {
            return_code = "torculus";
            neume_notes = 3;
        }
        return return_code;
    }

    if (code == "PR6") {
        
    }

    if (code == "PR5") {
        
    }

    if (code == "PS7") {
        
    }

    if (code == "PS6") {
        
    }

    if (code == "PS8") {
        
    }

    if (code == "PS9") {
        
    }

    if (code == "CM2") {
        
    }

    if (code == "PS3") {
        
    }

    return return_code;
    
}

scribe_part::scribe_part()
{
    
}

scribe_part&   scribe_part::operator=(const scribe_part& rhs)
{
    rep_num = rhs.rep_num;
    title = rhs.title;
    composer = rhs.composer;
    genre = rhs.genre;
    num_voices = rhs.num_voices;
    voice_type = rhs.voice_type;
    abbrev_ms = rhs.abbrev_ms;
    folios = rhs.folios;
    feast = rhs.feast;
    office = rhs.office;
    cao_num = rhs.cao_num;
    rows = rhs.rows;
    initial_staff_data = rhs.initial_staff_data;
    initial_staff_data_set = true;
    
    return *this;
}

void scribe_part::clear()
{
    rep_num.clear();
    title.clear();
    composer.clear();
    genre.clear();
    num_voices = 0;
    abbrev_ms.clear();
    folios.clear();
   // staff_lines = 4;
    feast.clear();
    office.clear();
    cao_num = 0;
    rows.clear();
    
    initial_staff_data = scribe_staff_data();
    initial_staff_data_set = false;
}

const char scribe_clef::get_pitch_name(const int pitch_loc)
{
    char pitch_name = 'a';
    int a_pos = 0;
    
    switch (clef)
    {
        case 'C':
            a_pos = clef_line - 2;
            break;
        case 'F':
            a_pos = clef_line + 2;
            break;
        case 'G':
            a_pos = clef_line - 6;
            break;
        default:
            break;
    }
    
    int displacement = 0;
    
    int small_pitch_loc = pitch_loc % 7;
    if (small_pitch_loc >= a_pos)
    {
        displacement = small_pitch_loc - a_pos;
    }
    else
    {
        displacement = (small_pitch_loc + 7) - a_pos;
        while (displacement < 0) displacement += 7;
    }
    
    pitch_name += displacement;
    
    
    return pitch_name;
}

// uses standards of Acoustical Society of America, c-b, middle c = C4, F clef = F3, treble g = G3
const int  scribe_clef::get_octave(const int pitch_loc)
{
    int octave = 4;
    
    int c_pos = 0;
    
    switch (clef)
    {
        case 'C':
            c_pos = clef_line;
            break;
        case 'F':
            c_pos = clef_line + 4;
            break;
        case 'G':
            c_pos = clef_line - 4;
            break;
        default:
            break;
    }
    
    if ((pitch_loc-c_pos)<0) octave--;
    octave += (pitch_loc-c_pos)/7;
    
    return octave;
}

