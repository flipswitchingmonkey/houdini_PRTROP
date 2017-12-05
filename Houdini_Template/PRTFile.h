//
//  PRTFile.h
//  PRTFile
//
//  Created by Ali Nakipoğlu on 1/13/13.
//  Copyright (c) 2013 Ali Nakipoğlu. All rights reserved.
//
#pragma once
#ifndef PRTFile_PRTFile_h
#define PRTFile_PRTFile_h

#include <string>
#include <map>
#include <fstream>

#include <hboost/variant.hpp>

#include <zlib.h>

#include "Types.h"
#include "AttributeContainer.h"
#include "FileSections.h"
#include "Utils.h"

#define PRT_HEADER_MAGIC_NUM_0  ((UINT8)192)
#define PRT_HEADER_MAGIC_NUM_1  ((UINT8)'P')
#define PRT_HEADER_MAGIC_NUM_2  ((UINT8)'R')
#define PRT_HEADER_MAGIC_NUM_3  ((UINT8)'T')
#define PRT_HEADER_MAGIC_NUM_4  ((UINT8)'\r')
#define PRT_HEADER_MAGIC_NUM_5  ((UINT8)'\n')
#define PRT_HEADER_MAGIC_NUM_6  ((UINT8)26)
#define PRT_HEADER_MAGIC_NUM_7  ((UINT8)'\n')

#define PRT_VERSION             1

#define PRT_SIGNITURE           "Extensible Particle Format"

#define PRT_RESERVED_BYTE_VALUE 4

namespace PRTFile
{
    
    class PRTFile
    {
      
    public:
        typedef hboost::variant< toRefType<AttributeContainer_INT8T>::type,
                                toRefType<AttributeContainer_INT16T>::type,
                                toRefType<AttributeContainer_INT32T>::type,
                                toRefType<AttributeContainer_INT64T>::type,
                                toRefType<AttributeContainer_UINT8T>::type,
                                toRefType<AttributeContainer_UINT16T>::type,
                                toRefType<AttributeContainer_UINT32T>::type,
                                toRefType<AttributeContainer_UINT64T>::type,
#ifdef ENABLE_HALF
                                toRefType<AttributeContainer_FLOAT16T>::type,
#endif
                                toRefType<AttributeContainer_FLOAT32T>::type,
                                toRefType<AttributeContainer_FLOAT64T>::type
                                >
                                                                        AttributeContainerVariantType;
        
        typedef std::map<std::string, AttributeContainerVariantType>    AttributeMapType;
        typedef std::map<std::string, INT32>                            OffsetMapType;
        
    protected:
        
        class AttributeRefDataTypeIDVisitor: public hboost::static_visitor<INT32>
        {
            
        public:
            
            template<typename T>
            INT32 operator()( T &operand )
            {
                return (INT32)toTypeID<typename toElementType<T>::type::Type>::ID;
            };
        };
        
        class AttributeRefComponentCountVisitor: public hboost::static_visitor<INT32>
        {
            
        public:
            
            template<typename T>
            INT32 operator()( T &operand )
            {
                return (INT32)operand->getComponentCount();
            };
        };
        
        class BufferTransferVisitor: public hboost::static_visitor<>
        {
            
        public:
            
            BufferTransferVisitor( UINT8 *bufferPtr, const INT32 elementSize_, const INT32 offset_ )
            
            :bufferPtr( bufferPtr )
            ,elementSize( elementSize_ )
            ,offset( offset_ )
            
            {};
            
        public:
            
            template<typename T>
            void operator()( T &operand )
            {
                for( typename toElementType<T>::type::ElementIterator eIt = operand->getData().begin(); eIt != operand->getData().end(); ++eIt )
                {
                    INT64 elementIndex          = std::distance( operand->getData().begin(), eIt );
                    INT64 currentStartOffset    = elementIndex * (INT64)elementSize + (INT64)offset;
                    
                    memcpy( &bufferPtr[ currentStartOffset ], &( eIt->at( 0 ) ), operand->getComponentCount() * sizeof( typename toElementType<T>::type::Type ) );
                }
            };
            
        protected:
            
            UINT8   *bufferPtr;
            INT32   elementSize;
            INT32   offset;
        };
        
    public:
        
        PRTFile( const INT64 elementCount_ )
        
        :elementCount( elementCount_ )
        ,currentElementSize( 0 )
        
        {};
        
        ~PRTFile()
        {};
        
    public:
        
        template<typename TypeT>
        void addAttribute( const std::string &name,  const size_t component_count )
        {
            typedef AttributeContainerT<TypeT>                          AttributeContainerTypeT;
            typedef typename toRefType<AttributeContainerTypeT>::type AttributeContainerRefTypeT;
            
            attributeMap[ name ]    = AttributeContainerRefTypeT( new AttributeContainerTypeT( elementCount, component_count ) );
            offsetMap[ name ]       = currentElementSize;
            
            currentElementSize      += sizeof( TypeT ) * component_count;
        };
        
        template<typename TypeT>
        typename toRefType< AttributeContainerT<TypeT> >::type getAttributeContainerRef( const std::string &name )
        {
            return hboost::get< typename toRefType< AttributeContainerT<TypeT> >::type >( attributeMap[ name ] );
        };
        
        void save( const std::string &filePath )
        {
            // Init Header Section
            
            memset( &headerSection, 0, sizeof( HeaderSection ) );
            
            headerSection.magicNumber[0]            = PRT_HEADER_MAGIC_NUM_0;
            headerSection.magicNumber[1]            = PRT_HEADER_MAGIC_NUM_1;
            headerSection.magicNumber[2]            = PRT_HEADER_MAGIC_NUM_2;
            headerSection.magicNumber[3]            = PRT_HEADER_MAGIC_NUM_3;
            headerSection.magicNumber[4]            = PRT_HEADER_MAGIC_NUM_4;
            headerSection.magicNumber[5]            = PRT_HEADER_MAGIC_NUM_5;
            headerSection.magicNumber[6]            = PRT_HEADER_MAGIC_NUM_6;
            headerSection.magicNumber[7]            = PRT_HEADER_MAGIC_NUM_7;
            
            headerSection.length                    = sizeof( HeaderSection );
            
            headerSection.version                   = PRT_VERSION;
            
            headerSection.count                     = elementCount;
            
            sprintf( (char *)headerSection.signiture, "%s", PRT_SIGNITURE );
            
            // Init Reserved Byte Section
            
            reservedBytesSection.value              = PRT_RESERVED_BYTE_VALUE;
            
            // Init Channels Header Section
            
            channelsHeaderSection.length            = sizeof( ChannelDefinition );
            channelsHeaderSection.count             = (INT32)attributeMap.size();
            
            INT64   attributeBufferSize             = elementCount * currentElementSize;
            uLongf  attributeBufferCompressedSize   = compressBound( attributeBufferSize );
            
            UINT8   *attributeBufferData            = (UINT8*)malloc( attributeBufferSize );
            Bytef   *compressedAttributeBuffer      = (Bytef *)malloc( attributeBufferCompressedSize );
            
            for( AttributeMapType::const_iterator it = attributeMap.begin(); it != attributeMap.end(); ++it )
            {
                BufferTransferVisitor bufferTransferVisitor( attributeBufferData, currentElementSize, offsetMap[ it->first ] );
                
                hboost::apply_visitor( bufferTransferVisitor, it->second );
            }

            if( compress2( compressedAttributeBuffer, &attributeBufferCompressedSize, reinterpret_cast<Bytef*>( attributeBufferData ), attributeBufferSize, 6 ) == Z_OK )
            {
                std::ofstream filestream( filePath.c_str(), std::ios::binary | std::ios_base::out );
                
                if ( filestream.good() )
                {
                    filestream.write( (char *)&headerSection, sizeof( HeaderSection ) );
                    filestream.write( (char *)&reservedBytesSection, sizeof( ReservedBytesSection ) );
                    filestream.write( (char *)&channelsHeaderSection, sizeof( ChannelsHeaderSection ) );
                    
                    for( AttributeMapType::const_iterator it = attributeMap.begin(); it != attributeMap.end(); ++it )
                    {
                        ChannelDefinition                   channelDefinition;
                        AttributeRefDataTypeIDVisitor       typeIDVisitor;
                        AttributeRefComponentCountVisitor   componentCountVisitor;
                        
                        memset( &channelDefinition, 0, sizeof( ChannelDefinition ) );
                        
                        sprintf( (char *)channelDefinition.name, "%s", it->first.c_str() );
                        
                        channelDefinition.dataType  = hboost::apply_visitor( typeIDVisitor, it->second );
                        channelDefinition.arity     = hboost::apply_visitor( componentCountVisitor, it->second );
                        channelDefinition.offset    = offsetMap[ it->first ];

                        filestream.write( (char *)&(channelDefinition), sizeof( ChannelDefinition ) );
                    }
                    
                    filestream.write( (char *)compressedAttributeBuffer, attributeBufferCompressedSize );
                    
                    filestream.close();
                    
                    free( attributeBufferData );
                    free( compressedAttributeBuffer );
                }
            }
        };
        
    protected:
        
        HeaderSection           headerSection;
        ReservedBytesSection    reservedBytesSection;
        ChannelsHeaderSection   channelsHeaderSection;
        
        AttributeMapType        attributeMap;
        OffsetMapType           offsetMap;
        
        INT64                   elementCount;
        
        INT32                   currentElementSize;
        
    };
}

#endif
