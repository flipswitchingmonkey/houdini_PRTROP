//
//  AttributeContainer.h
//  PRTFile
//
//  Created by Ali Nakipoğlu on 1/14/13.
//  Copyright (c) 2013 Ali Nakipoğlu. All rights reserved.
//
#pragma once
#ifndef PRTFile_AttributeContainer_h
#define PRTFile_AttributeContainer_h

#include <vector>

#include "Types.h"

namespace PRTFile {
    
    template<typename TypeT>
    class AttributeContainerT
    {
        
    public:
        
        typedef TypeT                                       Type;
        typedef std::vector<Type>                           ElementVectorType;
        typedef std::vector<ElementVectorType>              DataVector;
        
        typedef typename DataVector::iterator               ElementIterator;
        typedef typename DataVector::const_iterator         ElementConstIterator;
        
        typedef typename ElementVectorType::iterator        ComponentIterator;
        typedef typename ElementVectorType::const_iterator  ComponentConstIterator;
        
    public:
        
        AttributeContainerT( const size_t element_count_, const size_t component_count_ )
        
        :element_count( element_count_ )
        ,component_count( component_count_ )
        
        {
            ElementVectorType elementVector;
            
            elementVector.resize( component_count );
            
            data.resize( element_count, elementVector );
        };
        
        ~AttributeContainerT(){};
        
    public:
        
        const DataVector & getData() const
        {
            return data;
        };
        
        DataVector & getData()
        {
            return data;
        };
        
    public:
        
        const TypeT & operator ()( size_t offset, size_t component_index ) const
        {
            return data[ offset ][ component_index ];
        };
        
        TypeT & operator ()( size_t offset, size_t component_index )
        {
            return data[ offset ][ component_index ];
        };
        
        const ElementVectorType & operator ()( size_t offset ) const
        {
            return data[ offset ];
        };
        
        ElementVectorType & operator ()( size_t offset )
        {
            return data[ offset ];
        };
        
    public:
        
        const size_t getElementCount() const
        {
            return element_count;
        };
        
        const size_t getComponentCount() const
        {
            return component_count;
        }
        
    protected:
        
        DataVector  data;
        size_t      element_count;
        size_t      component_count;
    };
    
    typedef AttributeContainerT<INT8>       AttributeContainer_INT8T;
    typedef AttributeContainerT<INT16>      AttributeContainer_INT16T;
    typedef AttributeContainerT<INT32>      AttributeContainer_INT32T;
    typedef AttributeContainerT<INT64>      AttributeContainer_INT64T;
    
    typedef AttributeContainerT<UINT8>      AttributeContainer_UINT8T;
    typedef AttributeContainerT<UINT16>     AttributeContainer_UINT16T;
    typedef AttributeContainerT<UINT32>     AttributeContainer_UINT32T;
    typedef AttributeContainerT<UINT64>     AttributeContainer_UINT64T;
#ifdef ENABLE_HALF
    typedef AttributeContainerT<FLOAT16>    AttributeContainer_FLOAT16T;
#endif
    typedef AttributeContainerT<FLOAT32>    AttributeContainer_FLOAT32T;
    typedef AttributeContainerT<FLOAT64>    AttributeContainer_FLOAT64T;
    
}

#endif
