//
//  PRT_RopDriver.h
//  PRT_RopDriver
//
//  Created by Ali Nakipoğlu on 1/10/13.
//  Copyright (c) 2013 Ali Nakipoğlu. All rights reserved.
//
#pragma once
#ifndef PRT_RopDriver_PRT_RopDriver_h
#define PRT_RopDriver_PRT_RopDriver_h

#include <ROP/ROP_Node.h>
#include <OP/OP_OperatorPair.h>

class PRT_RopDriver : public ROP_Node
{
    
public:

    static OP_TemplatePair* getTemplatePair();
    static OP_VariablePair* getVariablePair();
    
    static OP_Node *nodeConstructor( OP_Network *net, const char *name, OP_Operator *op );
    
protected:
    
    PRT_RopDriver( OP_Network *net, const char *name, OP_Operator *op );
    
    virtual ~PRT_RopDriver();
    
protected:
    
    virtual int             startRender(int nframes, fpreal s, fpreal e);
    virtual ROP_RENDER_CODE renderFrame(fpreal time, UT_Interrupt *boss);
    virtual ROP_RENDER_CODE endRender();
    
private:

    fpreal  startTime;
    fpreal  endTime;
};

#endif
