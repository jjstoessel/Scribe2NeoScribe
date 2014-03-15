//
//  CNeoScribeXML.h
//  Scribe2NeoScribe
//
//  Created by Jason Stoessel on 8/02/2014.
//  Copyright (c) 2014 The Early Music eResearch System (THEMES). All rights reserved.
//
//  CScribeToNeoScribeXML converts an in-memory representation of Scribe data contained in CScribeReaderVisitable
//  in Music Encoding Initiative-compliant XML

#ifndef __Scribe2NeoScribe__CScribeToNeoScribeXML__
#define __Scribe2NeoScribe__CScribeToNeoScribeXML__

#include <iostream>
#include <string>
#include <mei/mei.h>
#include <mei/header.h>
#include <mei/shared.h>

#include "Visitor.h"
#include "CSourceKey.h"

using namespace Loki;
using namespace mei;

class CScribeReaderVisitable;
class scribe_part;

class CScribeToNeoScribeXML : public BaseVisitor, public Visitor<CScribeReaderVisitable>
{
public:
    CScribeToNeoScribeXML(const std::string& encoder_name);
    ~CScribeToNeoScribeXML();
    
    void                PrintMEIXML(const std::string& encoder_name="John A. Stinson");
    void                SaveMEIXML(const std::string& file_name, const std::string& encoder_name="John A. Stinson");
    void                SegmentScribe2MEIXML(const CScribeReaderVisitable& scribe_data);
    
    void                Visit(CScribeReaderVisitable& scribe_data);
    
private:
    
    std::string         encoder = "Unknown";
    static const CSourceKey*   sourcekey;
    
    MeiDocument*        Scribe2MEIXML(const CScribeReaderVisitable& scribe_data);
    void                Scribe2MEIXMLFileData(FileDesc* fileDesc, const scribe_part& partit );
    void                Scribe2MEIXMLWorkData(WorkDesc* workDesc, const scribe_part& partit );
    FileDesc*           Scribe2MEIFileDesc();
    EncodingDesc*       Scribe2MEIEncoderDesc();
    WorkDesc*           Scribe2MEIWorkDesc();
    Score*              Scribe2MEIXMLScore();
    Staff*              Scribe2MEIXMLStaff(const CScribeReaderVisitable& scribe_data, const scribe_part& partit, StaffGrp* staffgrp, const int i);
    
    MeiDocument*      doc;
};

#endif /* defined(__NeoScribe__CScribeToNeoScribeXML__) */
