/*---------------------------------------------------------------------------------------------------------
    File:       CScribeReader.cpp
    Project:    Scribe2NeoScribe
 
    Created by Jason Stoessel on 21/06/13.
    Copyright (c) 2013 The Early Music eResearch System (THEMES). All rights reserved.
 
    History
 
    21 JUNE 2013:   Commenced - scribe parser
    12 JAN  2014:   Scribe2MEIXML function split into sub routines to permit addition call functions
    9  FEB  2014:   Converted into a visitable class, removing NeoScribeXML extraction routines to 
                    CNeoScribeXML (a visitor)

    To do:
    
---------------------------------------------------------------------------------------------------------*/

#include "CScribeReaderVisitable.h"
#include "CScribeCodes.h"
#include <exception>
#include <sstream>
#include <iterator>
#include "Header.h"


CScribeReaderVisitable::CScribeReaderVisitable(const std::string& scribe_file_name)
: file(scribe_file_name)
{
    trecento_codes = new CScribeCodes(TRECENTO_CODES_CSV);
    chant_codes = new CScribeCodes(CHANT_CODES_CSV);
    if (file.is_open())
    {
        read_header();
        if (is_scribe_file()) load_scribe_file();
    }
    else
        std::cout << "Could not open nominated Scribe file.";
}

CScribeReaderVisitable::~CScribeReaderVisitable()
{
    delete trecento_codes;
    delete chant_codes;
    file.close();
}

scribe_type  CScribeReaderVisitable::read_header()
{
    std::string h_line;
    GETLINE(file,h_line);
    
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
        throw std::range_error("bad file: header not found");
    }
    
    return type;
}

//Primary function that reads in header and calls a function to read each row of a scribe file
int CScribeReaderVisitable::load_scribe_file()
{
    int numparts = 0;
    scribe_part part;
    std::string row;
    std::string field;
    std::string temp_title("");
    
    //read in second row containing metadata; header has already been read so file pointer should be pre-line 2 (checking required)
    GETLINE(file,row);
    
    while (file.good())
    {
        std::stringstream lineStream(row);
        
        
        if (lineStream.get()!='>') throw std::logic_error("metadata not present."); //make sure metadata is present
        
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
        
        //Reading in a title from syllables
        if (!part.rows.empty() && type == chant)
        {
            for (std::vector<scribe_row>::iterator u = part.rows.begin(); u!=part.rows.end() &&  part.title.size()<16 ; u++)
            {
                if (!u->is_comment && !u->syllable.empty() && (u->events[0].code!="C" || u->events[0].code!="F" || u->events[0].code!="G"))
                {
                    part.title += u->syllable;
                    
                    
                    if ( part.title.back() == '-')
                    {
                        do {
                            part.title.pop_back();
                            u++;
                            part.title += u->syllable;
                        } while (u->syllable.back() == '-');
                        
                    }
                    
                    if (part.title.back() == '.')
                    {
                        part.title.pop_back();
                        break;
                    }
                    part.title.push_back(' ');
                    
                }
                
            }
            
            if (part.title.back() == '.' || part.title.back() == '-' || std::isspace(part.title.back()) )
            {
                part.title.pop_back();
            }
        }

        if (temp_title != part.title)
        {
            pieceCount++;
            temp_title=part.title;
        }
        numparts++;
        part.partID = numparts;
        
        parts.push_back(part);
        part.clear();
        
    }
    
    return numparts;
}

// returns substring field and pos incremented by field length
std::string CScribeReaderVisitable::read_part_header_field(const std::string& row, std::streampos& start,  const size_t field_length)
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
scribe_row    CScribeReaderVisitable::read_scribe_row(std::string raw_row)
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
std::vector<int> CScribeReaderVisitable::get_pitch_code(char& c, std::stringstream& lineStream)
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
void CScribeReaderVisitable::Print()
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


const std::string CScribeReaderVisitable::get_ineume_part(const std::string& code, const int i, int& neume_notes)
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

std::string CScribeReaderVisitable::get_ligature_part(const std::string& code, const int note_count) const
{
    std::string note_name = codes->code_to_name("B");
    
    
    if (
        (code == "LL" || code == "VL") ||                           //with propriety and with perfection
        ( (code == "PD" || code == "CL") && note_count == 1) ||     //without propriety and with perfection
        ( code == "OB" && note_count == 0 ) ||                      //without propriety and without perfection
        ( code == "TQ" && note_count == 2 ) ||                      // ternaria with propriety and perfection
        ( (code == "PR" || code == "PR'") && ( note_count == 0 || note_count == 2)) // ternaria without propriety and with perfection
        )
    {
        note_name = codes->code_to_name("L");
    } else if ( code == "OB'")
    {
    } else if ( code == "COB" || code == "OP" )
    {
        note_name = codes->code_to_name("S");
    }
  
    return note_name;
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
    partID = rhs.partID;
    
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

