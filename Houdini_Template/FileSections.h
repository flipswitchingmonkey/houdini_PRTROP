//
//  FileSections.h
//  PRTFile
//
//  Created by Ali Nakipoğlu on 1/14/13.
//  Copyright (c) 2013 Ali Nakipoğlu. All rights reserved.
//
#pragma once
#ifndef PRTFile_FileSections_h
#define PRTFile_FileSections_h

#include "Types.h"

namespace PRTFile {
    
#pragma pack(push)
#pragma pack(1)
    struct HeaderSection
    {
        UINT8   magicNumber[8];
        INT32   length;
        UINT8   signiture[32];
        INT32   version;
        INT64   count;
    };

    struct ReservedBytesSection
    {
        INT32   value;
    };

    struct ChannelsHeaderSection
    {
        INT32   count;
        INT32   length;
    };
    
    struct ChannelDefinition
    {
        UINT8   name[32];
        INT32   dataType;
        INT32   arity;
        INT32   offset;
    };
#pragma pack(pop)
    
}

#endif
