/*
    main.cpp
    Scribe2NeoScribe
 
    Created by Jason Stoessel on 21/06/13.
    Copyright (c) 2013-14 The Early Music eResearch System (THEMES). All rights reserved.
 
    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:
 
    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.
 
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
    LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
    OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
    WARNING: THIS IS CURRENTLY DELTA SOFTWARE THAT IS INCOMPLETE, HAS NOT 
    BEEN EXTENSIVELY TESTED AND MAY PRODUCE UNEXPECT RESULTS ON DIFFERENT SYSTEMS.

    This basic terminal/command line tool converts Scribe data to MEI-compliant NeoScribe XML
*/

#include <iostream>
#include <exception>

#include "Header.h"
#include "CScribeCodes.h"
#include "CScribeReaderVisitable.h"
#include "CScribeToNeoScribeXML.h"

std::string     encoder_parameter("-e");
std::string     appName = "Scribe2NeoScribe";
std::string     PROCWORKINGDIRECTORY = "/usr/local/bin/";

int main(int argc, const char * argv[])
{
    
    if (argc < 2)
    {
        std::cout << "Insufficient or incorrect parameters." << std::endl;
        std::cout << "Usage: Scribe2NeoScribe [-e encoder] file ..." << std::endl;
        exit(1); //add help here
    }
    
        
    try {
        
        std::cout << "Welcome to the Scribe2NeoScribe convertor. This this application will\n"
        << "convert your Scribe file to MEI-compliant NeoScribe XML. Your original\n"
        << "file will not be altered." << std::endl;
        
        PROCWORKINGDIRECTORY = argv[0];
        std::cout << "Process directory: "<< PROCWORKINGDIRECTORY << std::endl;
        if (!PROCWORKINGDIRECTORY.empty()) {
            for (int i = 0; i < appName.length(); i++) {
                PROCWORKINGDIRECTORY.pop_back();
            }
        }
        else { std::cerr << "Scribe2NeoScribe: couldnot find process directory" << std::endl; return 0; }
        
        //quick work around for the time being
        if (PROCWORKINGDIRECTORY.empty()) {
            PROCWORKINGDIRECTORY = "/usr/local/bin/";
        }
        
        std::stringstream encoder;
        int i = 1;
        
        //see if encoder name is indicated
        if (argv[i]==encoder_parameter)
        {
            encoder << argv[2];
            i = 3; //first file
        }
        else
            encoder << "Unknown";
        
        if (i>argc-1) { std::cerr << "Scribe2NeoScribe: Bad parameter count" << std::endl; return 0; }
        
        std::string file(argv[i]); //file name currently supplied as call parameter
        
        if (file.empty())  { std::cerr << "Scribe2NeoScribe: Bad file parameter" << std::endl; return 0; }
    
        std::cout << "Converting " << argv[i] << "..." << std::endl;
        
        CScribeReaderVisitable scribe_data(file);
        
        CScribeToNeoScribeXML nsXML(encoder.str());
        
        for ( ; i<argc; i++)
        {
            
            if (scribe_data.PieceCount()==1)
            {
                scribe_data.Accept(nsXML);
                
                //test functions
                //scribe_data.print();
                
                //nsXML.PrintMEIXML();
                
                std::string xml_file = file + ".xml";
                
                nsXML.SaveMEIXML(xml_file);
            }
            else if (scribe_data.PieceCount()>1) 
                nsXML.SegmentScribe2MEIXML(scribe_data);
            
        }
        
        std::cout << scribe_data.PieceCount() << " piece(s) converted to NeoScribeXML." << std::endl;
        
    } catch (std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    
    return 0;
}