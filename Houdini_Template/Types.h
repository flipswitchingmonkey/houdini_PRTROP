//
//  Types.h
//  PRTFile
//
//  Created by Ali Nakipoğlu on 1/14/13.
//  Copyright (c) 2013 Ali Nakipoğlu. All rights reserved.
//
#pragma once
#ifndef PRTFile_Types_h
#define PRTFile_Types_h

#include <cstdint>

#ifdef ENABLE_HALF
    // This is taken from the OpenEXR Porting guide https://openexr.com/en/latest/PortingGuide.html#id2:
    // The version can reliably be found in this header file from OpenEXR,
    // for both 2.x and 3.x:
    #include <OpenEXR/OpenEXRConfig.h>
    #define COMBINED_OPENEXR_VERSION ((10000*OPENEXR_VERSION_MAJOR) + \
                                      (100*OPENEXR_VERSION_MINOR) + \
                                      OPENEXR_VERSION_PATCH)

    // There's just no easy way to have an ``#include`` that works in both
    // cases, so we use the version to switch which set of include files we
    // use.
    #if COMBINED_OPENEXR_VERSION >= 20599 /* 2.5.99: pre-3.0 */
    #   include <Imath/half.h>
    #else
        // OpenEXR 2.x, use the old locations
    #   include <OpenEXR/half.h>
    #endif
#endif

namespace PRTFile {
    
    typedef int8_t   INT8;
    typedef int16_t  INT16;
    typedef int32_t  INT32;
    typedef int64_t  INT64;
    
    typedef uint8_t  UINT8;
    typedef uint16_t UINT16;
    typedef uint32_t UINT32;
    typedef uint64_t UINT64;
    
#ifdef ENABLE_HALF
    typedef half            FLOAT16;
#endif
    typedef float           FLOAT32;
    typedef double          FLOAT64;
    
    enum TypeID
    {
        PRT_INT_8       = 9,
        PRT_INT_16      = 0,
        PRT_INT_32      = 1,
        PRT_INT_64      = 2,
        PRT_UINT_8      = 10,
        PRT_UINT_16     = 6,
        PRT_UINT_32     = 7,
        PRT_UINT_64     = 8,
#ifdef ENABLE_HALF
        PRT_FLOAT_16    = 3,
#endif
        PRT_FLOAT_32    = 4,
        PRT_FLOAT_64    = 5,
        PRT_UNKNOWN     = -1
    };
    
    inline static unsigned int getTypeSize( const TypeID typeID )
    {
        switch ( typeID )
        {
            case PRT_INT_8      : return sizeof( INT8 )     ; break;
            case PRT_INT_16     : return sizeof( INT16 )    ; break;
            case PRT_INT_32     : return sizeof( INT32 )    ; break;
            case PRT_INT_64     : return sizeof( INT64 )    ; break;
            case PRT_UINT_8     : return sizeof( UINT8 )    ; break;
            case PRT_UINT_16    : return sizeof( UINT16 )   ; break;
            case PRT_UINT_32    : return sizeof( UINT32 )   ; break;
            case PRT_UINT_64    : return sizeof( UINT64 )   ; break;
#ifdef ENABLE_HALF
            case PRT_FLOAT_16   : return sizeof( FLOAT16 )  ; break;
#endif
            case PRT_FLOAT_32   : return sizeof( FLOAT32 )  ; break;
            case PRT_FLOAT_64   : return sizeof( FLOAT64 )  ; break;
            
            case PRT_UNKNOWN    : return 0; break;
        }
    };
    
    template<typename TypeT> class toTypeID { public: enum{ ID  = PRT_UNKNOWN }; };
    
    template<> class toTypeID<INT8> { public: enum{ ID  = PRT_INT_8 }; };
    template<> class toTypeID<INT16> { public: enum{ ID  = PRT_INT_16 }; };
    template<> class toTypeID<INT32> { public: enum{ ID  = PRT_INT_32 }; };
    template<> class toTypeID<INT64> { public: enum{ ID  = PRT_INT_64 }; };
    
    template<> class toTypeID<UINT8> { public: enum{ ID  = PRT_UINT_8 }; };
    template<> class toTypeID<UINT16> { public: enum{ ID  = PRT_UINT_16 }; };
    template<> class toTypeID<UINT32> { public: enum{ ID  = PRT_UINT_32 }; };
    template<> class toTypeID<UINT64> { public: enum{ ID  = PRT_UINT_64 }; };
    
#ifdef ENABLE_HALF
    template<> class toTypeID<FLOAT16> { public: enum{ ID  = PRT_FLOAT_16 }; };
#endif
    template<> class toTypeID<FLOAT32> { public: enum{ ID  = PRT_FLOAT_32 }; };
    template<> class toTypeID<FLOAT64> { public: enum{ ID  = PRT_FLOAT_64 }; };
}

#endif
