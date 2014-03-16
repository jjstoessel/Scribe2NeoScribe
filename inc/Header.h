//
//  Header.h
//  Scribe2NeoScribe
//
//  Created by Jason Stoessel on 25/06/13.
//  Copyright (c) 2013-14 The Early Music eResearch System (THEMES). All rights reserved.
//

#ifndef Scribe2NeoScribe_Header_h
#define Scribe2NeoScribe_Header_h

#include <sstream>
#include <iomanip> 


const std::string   TRECENTO_CODES_CSV = "data/neumcode_trecento.csv";
const std::string   CHANT_CODES_CSV = "data/neumcode_chant.csv";
const std::string   SOURCE_KEY_CSV = "data/sourcekey.tab";
extern  std::string  PROCWORKINGDIRECTORY;


enum scribe_type {undefined, chant, trecento};
enum coloration_type { default_color, full_black = 3, full_red, void_red, void_black, full_blue };
enum code_t  { note, rest, ligature, uneume, ineume, mensuration, clef, barline, dot, accidental, other };
enum rest_types { minim_rest = 1, semibreve_rest = -1, breve_rest = 2, long_rest = 4, perf_long_rest = 6 };
enum rest_names {
    generic_rest = 'R',
    semiminim_rest = 'RSM'
    };

enum voice_type { unlabelled = 0, cantus = 1, triplum = 2, contratenor = 3, tenor = 4, tenor2 = 5 };

const std::string scribe_trecento = "S^C^R^I^B^E^S";    //Trecento notation file
const std::string scribe_chant = "S^C^R^I^B^E^L";       //Chant notation file

const size_t rep_num_length = 10, genre_length = 10, ms_abbrev_length = 10;
const size_t title_length = 40;
const size_t composer_length = 20, folio_length = 20, office_length = 20, chant_type_length = 20;
const size_t voice_count_length = 1;
const size_t cao_num_length = 8;

//utility function for converting floats, doubles, etc. to strings with particular precision after decimal point
template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 6)
{
    std::ostringstream oss;
    oss << std::setprecision(n) << a_value;
    return oss.str();
}


std::string ZeroPadNumber(int num, int width);

//GETLINE is a marco which calls either a safeGetline function suitable for Mac, Window, and Unix line ends; std::end is platform specific, ie. lf on unix and new mac, cr+lf on windows. Undefine _SAFEGETLINE_ if only unix files are to be processed
#define _SAFEGETLINE_
#ifdef _SAFEGETLINE_

std::istream& safeGetline(std::istream& is, std::string& t);

#define GETLINE(is, t) safeGetline(is, t)

#else

#define GETLINE(is, t) std::getline(is,t)

#endif

#endif
