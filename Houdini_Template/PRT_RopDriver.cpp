//
//  PRT_RopDriver.cpp
//  PRT_RopDriver
//
//  Created by Ali Nakipoğlu on 1/4/13.
//  Copyright (c) 2013 Ali Nakipoğlu. All rights reserved.
//

#define ENABLE_HALF

#ifdef ENABLE_HALF
    #if defined(_WIN64) || defined(__WIN64__) || defined(WIN64) || defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
        #define OPENEXR_DLL
    #endif
#endif

#include "PRT_RopDriver.h"

#include <algorithm>
#include <string.h>
#include <iostream>

#include <map>

#include <UT/UT_DSOVersion.h>

#include <UT/UT_String.h>
#include <UT/UT_Vector2.h>
#include <UT/UT_Vector3.h>
#include <UT/UT_Vector4.h>

#include <ROP/ROP_Node.h>
#include <ROP/ROP_Error.h>
#include <ROP/ROP_Templates.h>

#include <PRM/PRM_Include.h>
#include <PRM/PRM_SpareData.h>
#include <PRM/PRM_ChoiceList.h>

#include <OP/OP_OperatorTable.h>
#include <OP/OP_Director.h>

#include <SOP/SOP_Node.h>

#include <GU/GU_Detail.h>

#include <GEO/GEO_Point.h>

#include <GA/GA_AIFTuple.h>
#include <GA/GA_Iterator.h>
#include <GA/GA_Types.h>
#include <GA/GA_AttributeDict.h>
#include <GA/GA_AttributeSet.h>
#include <GA/GA_AttributeFilter.h>

#include "PRTFile.h"

enum {
    PRT_ROP_RENDER,
    PRT_ROP_RENDER_CTRL,
    PRT_ROP_TRANGE,
    PRT_ROP_FRANGE,
    PRT_ROP_TAKE,
    PRT_ROP_FILE,
    PRT_ROP_SOPPATH,
    PRT_ROP_REFRESHATTRIBS,
    PRT_ROP_AUTOUPDATEATTRIBS,
    PRT_ROP_ATTRIBS,
    PRT_ROP_TPRERENDER,
    PRT_ROP_PRERENDER,
    PRT_ROP_LPRERENDER,
    PRT_ROP_TPREFRAME,
    PRT_ROP_PREFRAME,
    PRT_ROP_LPREFRAME,
    PRT_ROP_TPOSTFRAME,
    PRT_ROP_POSTFRAME,
    PRT_ROP_LPOSTFRAME,
    PRT_ROP_TPOSTRENDER,
    PRT_ROP_POSTRENDER,
    PRT_ROP_LPOSTRENDER,
    PRT_ROP_MKPATH,
    PRT_ROP_INITSIM,
    PRT_ROP_ALFPROGRESS,
    
    PRT_ROP_MAXPARMS
};


static PRM_Default      FILE_PRM_DEFAULT( 0, "out$F.prt" );
static PRM_Default      PRM_DEFAULT_EMPTY_STR( 0, "" );
static PRM_Default      PRM_DEFAULT_TRUE_INT( 1, "" );
static PRM_Default      PRM_DEFAULT_FALSE_INT( 0, "" );

static PRM_Name         FILE_PRM_NAME( "file", "File" );
static PRM_Name         SOP_PATH_PRM_NAME("soppath",  "SOP Path");
static PRM_Name         MKPATH_PRM_NAME( "mkpath", "Create Intermediate Directories" );
static PRM_Name         INITSIM_PRM_NAME( "initsim", "Initialize Simulation OPs" );
static PRM_Name         ALFPROGRESS_PRM_NAME( "alfprogress", "Alfred Style Progress" );

static PRM_Name         AUTO_UPDATE_ATTRIBS_PRM_NAME("autoAttribs",  "Auto Update Attributes");
static PRM_Name         REFRESH_ATTRIBS_PRM_NAME("refreshAttribs",  "Refresh Attributes");

static PRM_Name         ATTRIBS_PRM_NAME("attribs",  "Attributes");
static PRM_Name         ATTRIBS_PRM_EXP_ATT_NAME("exportAtt#",  "Export");
static PRM_Name         ATTRIBS_PRM_SRC_ATT_NAME("srcAtt#",  "Source Attribute");
static PRM_Name         ATTRIBS_PRM_DST_ATT_NAME("dstAtt#",  "Destination Attribute");
static PRM_Name         ATTRIBS_PRM_TYPE_ATT_NAME("typeAtt#",  "Export Type");

static PRM_Name         ATTRIBS_PRM_TYPE_ATT_CHOICES[] = {  PRM_Name( "Original", "Original" ),
                                                            PRM_Name( "float64", "float64" ),
                                                            PRM_Name( "float32", "float32" ),
#ifdef ENABLE_HALF
                                                            PRM_Name( "float16", "float16" ),
#endif
                                                            PRM_Name( "int64", "int64" ),
                                                            PRM_Name( "int32", "int32" ),
                                                            PRM_Name( 0 )
                                                        };

static PRM_ChoiceList   ATRIBS_PRM_TYPE_ATT_CHOICE_LIST( PRM_CHOICELIST_SINGLE, ATTRIBS_PRM_TYPE_ATT_CHOICES );

template<typename READ_ATT_TYPE_T, typename TARGET_ATT_TYPET>
static void readAttributeIntoBuffer(    const GU_Detail *gdpPtr,
                                        const GA_Attribute *attributePtr,
                                        const int tupleSize,
                                        typename PRTFile::toRefType< PRTFile::AttributeContainerT<TARGET_ATT_TYPET> >::type attributeContainerRef )
{
    const GA_AIFTuple   *aifTuplePtr        = attributePtr->getAIFTuple();
        
    if( aifTuplePtr )
    {
        READ_ATT_TYPE_T value;
        size_t          index               = 0;
        
        for( GA_Iterator it( gdpPtr->getPointRange() ); !it.atEnd(); it.advance() )
        {
            for( int i = 0; i < tupleSize; ++i )
            {
                aifTuplePtr->get( attributePtr, it.getOffset(), value, i );
                
                attributeContainerRef->getData().at( index ).at( i ) = value;
            }
            
            index++;
        }
    }
};

static std::map<std::string, std::string> & getPRTPreDefinedAttNameMap()
{
    std::map<std::string, std::string> *mapPtr  = NULL;
    
    if( mapPtr )
    {
        return *mapPtr;
    }
    
    mapPtr                                      = new std::map<std::string, std::string>();
    
    std::map<std::string, std::string> & mapRef = *mapPtr;
    
    mapRef[ "P" ]                               = "Position";
    mapRef[ "v" ]                               = "Velocity";
    mapRef[ "N" ]                               = "Normal";
    mapRef[ "Cd" ]                              = "Color";
    
    return mapRef;
};

static std::map<std::string, int> & getPRTPreDefinedAttMaxArityMap()
{
    std::map<std::string, int> *mapPtr          = NULL;
    
    if( mapPtr )
    {
        return *mapPtr;
    }
    
    mapPtr                                      = new std::map<std::string, int>();
    
    std::map<std::string, int> & mapRef         = *mapPtr;
    
    mapRef[ "Position" ]                        = 3;
    mapRef[ "Velocity" ]                        = 3;
    mapRef[ "Normal" ]                          = 3;
    mapRef[ "Color" ]                           = 3;
    
    return mapRef;
};

static void updateAttributeList( PRT_RopDriver *node, int index, fpreal32 time, const PRM_Template *tplate )
{
    UT_String                       sopPath;
    
    node->evalString( sopPath, PRT_ROP_SOPPATH, 0, time );
    
    SOP_Node                        *sopNode        = node->getSOPNode( sopPath, 1 );
    
    if( sopNode != NULL )
    {
        OP_Context                      context(time);
        
        GU_DetailHandle                 gdh             = sopNode->getCookedGeoHandle( context );
        GU_DetailHandleAutoReadLock     gdl( gdh );
        
        const GU_Detail                 *gdpPtr         = gdl.getGdp();
        
        if ( gdpPtr )
        {
            node->setInt( ATTRIBS_PRM_NAME.getToken(), 0, time, gdpPtr->getAttributeDict(GA_ATTRIB_POINT).entries() - 1 );
            
            int attIndex                                = 1;
            
            for (GA_AttributeDict::iterator it = gdpPtr->getAttributeDict(GA_ATTRIB_POINT).begin(GA_SCOPE_PUBLIC); !it.atEnd(); ++it)
            {
                GA_Attribute *attrib = it.attrib();
                
                UT_String attName( attrib->getName() );
                
                node->setStringInst( attName, CH_STRING_LITERAL, ATTRIBS_PRM_SRC_ATT_NAME.getToken() , &attIndex, 0, time );
                
                if( getPRTPreDefinedAttNameMap().count( std::string(attrib->getName()) ) > 0 )
                {
                    UT_String dstName( getPRTPreDefinedAttNameMap()[ std::string(attrib->getName()) ] );
                    
                    node->setStringInst( dstName, CH_STRING_LITERAL, ATTRIBS_PRM_DST_ATT_NAME.getToken() , &attIndex, 0, time );
                    node->setIntInst( 1, ATTRIBS_PRM_EXP_ATT_NAME.getToken(), &attIndex, 0, time );
                } else {
                    node->setStringInst( attName, CH_STRING_LITERAL, ATTRIBS_PRM_DST_ATT_NAME.getToken() , &attIndex, 0, time );
                }
                
                attIndex++;
            }
        }
    } else {
        if( node->evalInt( AUTO_UPDATE_ATTRIBS_PRM_NAME.getToken(), 0, time ) != 0 )
        {
            node->setInt( ATTRIBS_PRM_NAME.getToken(), 0, time, 0 );
        }
    }
};

static int SOP_PATH_PRM_CALLBACK( void *data, int index, fpreal32 time, const PRM_Template *tplate )
{
    PRT_RopDriver                   *node               = (PRT_RopDriver *)data;
 
    if( node->evalInt( AUTO_UPDATE_ATTRIBS_PRM_NAME.getToken(), 0, time ) != 0 )
    {
        updateAttributeList( node, index, time, tplate );
    }
    
    return 0;
};

static int REFRESH_ATT_PRM_CALLBACK( void *data, int index, fpreal32 time, const PRM_Template *tplate )
{
    PRT_RopDriver                   *node               = (PRT_RopDriver *)data;
    
    updateAttributeList( node, index, time, tplate );
    
    return 0;
};

static PRM_Template * getAttPRMTemplates()
{
    static PRM_Template *prmTemplate        = 0;
    
    if( prmTemplate )
    {
        return prmTemplate;
    }
    
    prmTemplate                             = new PRM_Template[4 + 1];
    
    prmTemplate[0]                          = PRM_Template( PRM_TOGGLE, 1, &ATTRIBS_PRM_EXP_ATT_NAME, &PRM_DEFAULT_EMPTY_STR );
    prmTemplate[1]                          = PRM_Template( PRM_STRING, 1, &ATTRIBS_PRM_SRC_ATT_NAME, &PRM_DEFAULT_EMPTY_STR );
    prmTemplate[2]                          = PRM_Template( PRM_STRING, 1, &ATTRIBS_PRM_DST_ATT_NAME, &PRM_DEFAULT_EMPTY_STR );
    prmTemplate[3]                          = PRM_Template( PRM_STRING, 1, &ATTRIBS_PRM_TYPE_ATT_NAME, 0, &ATRIBS_PRM_TYPE_ATT_CHOICE_LIST );
    
    prmTemplate[4]                          = PRM_Template();
    
    return prmTemplate;
};

static PRM_Template * getTemplates()
{
    static PRM_Template *prmTemplate        = 0;
    
    if( prmTemplate )
    {
        return prmTemplate;
    }
    
    prmTemplate                             = new PRM_Template[PRT_ROP_MAXPARMS + 1];
    
    prmTemplate[PRT_ROP_RENDER]             = theRopTemplates[ROP_RENDER_TPLATE];
    prmTemplate[PRT_ROP_RENDER_CTRL]        = theRopTemplates[ROP_RENDERDIALOG_TPLATE];
    prmTemplate[PRT_ROP_TRANGE]             = theRopTemplates[ROP_TRANGE_TPLATE];
    prmTemplate[PRT_ROP_FRANGE]             = theRopTemplates[ROP_FRAMERANGE_TPLATE];
    prmTemplate[PRT_ROP_TAKE]               = theRopTemplates[ROP_TAKENAME_TPLATE];
    
    prmTemplate[PRT_ROP_FILE]               = PRM_Template( PRM_FILE, 1, &FILE_PRM_NAME, &FILE_PRM_DEFAULT );
    prmTemplate[PRT_ROP_SOPPATH]            = PRM_Template( PRM_STRING, PRM_TYPE_DYNAMIC_PATH, 1, &SOP_PATH_PRM_NAME, 0, 0, 0, &SOP_PATH_PRM_CALLBACK, &PRM_SpareData::sopPath );
    prmTemplate[PRT_ROP_REFRESHATTRIBS]     = PRM_Template( PRM_CALLBACK, 1, &REFRESH_ATTRIBS_PRM_NAME, 0, 0, 0, &REFRESH_ATT_PRM_CALLBACK );
    prmTemplate[PRT_ROP_AUTOUPDATEATTRIBS]  = PRM_Template( PRM_TOGGLE, 1, &AUTO_UPDATE_ATTRIBS_PRM_NAME, &PRM_DEFAULT_TRUE_INT );
    prmTemplate[PRT_ROP_ATTRIBS]            = PRM_Template( PRM_MULTITYPE_LIST, getAttPRMTemplates(), 0, &ATTRIBS_PRM_NAME, 0, 0, 0 );
    
    prmTemplate[PRT_ROP_TPRERENDER]         = theRopTemplates[ROP_TPRERENDER_TPLATE];
    prmTemplate[PRT_ROP_PRERENDER]          = theRopTemplates[ROP_PRERENDER_TPLATE];
    prmTemplate[PRT_ROP_LPRERENDER]         = theRopTemplates[ROP_LPRERENDER_TPLATE];
    prmTemplate[PRT_ROP_TPREFRAME]          = theRopTemplates[ROP_TPREFRAME_TPLATE];
    prmTemplate[PRT_ROP_PREFRAME]           = theRopTemplates[ROP_PREFRAME_TPLATE];
    prmTemplate[PRT_ROP_LPREFRAME]          = theRopTemplates[ROP_LPREFRAME_TPLATE];
    prmTemplate[PRT_ROP_TPOSTFRAME]         = theRopTemplates[ROP_TPOSTFRAME_TPLATE];
    prmTemplate[PRT_ROP_POSTFRAME]          = theRopTemplates[ROP_POSTFRAME_TPLATE];
    prmTemplate[PRT_ROP_LPOSTFRAME]         = theRopTemplates[ROP_LPOSTFRAME_TPLATE];
    prmTemplate[PRT_ROP_TPOSTRENDER]        = theRopTemplates[ROP_TPOSTRENDER_TPLATE];
    prmTemplate[PRT_ROP_POSTRENDER]         = theRopTemplates[ROP_POSTRENDER_TPLATE];
    prmTemplate[PRT_ROP_LPOSTRENDER]        = theRopTemplates[ROP_LPOSTRENDER_TPLATE];
    prmTemplate[PRT_ROP_MKPATH]             = theRopTemplates[ROP_MKPATH_TPLATE];
    prmTemplate[PRT_ROP_INITSIM]            = theRopTemplates[ROP_INITSIM_TPLATE];
    prmTemplate[PRT_ROP_ALFPROGRESS]        = PRM_Template( PRM_TOGGLE, 1, &ALFPROGRESS_PRM_NAME, PRMzeroDefaults );
    
    prmTemplate[PRT_ROP_MAXPARMS]           = PRM_Template();
    
    return prmTemplate;
};

/******* PRT_RopDriver IMPL ******/

OP_TemplatePair * PRT_RopDriver::getTemplatePair()
{
    static OP_TemplatePair *ropPair     = 0;
    
    if( ropPair )
    {
        return ropPair;
    }
    
    ropPair = new OP_TemplatePair( getTemplates() );
    
    return ropPair;
};

OP_VariablePair * PRT_RopDriver::getVariablePair()
{
    static OP_VariablePair *varPair     = 0;
    
    if ( varPair )
    {
        return varPair;
    }
    
    varPair = new OP_VariablePair( ROP_Node::myVariableList );
    
    return varPair;
};

OP_Node * PRT_RopDriver::nodeConstructor( OP_Network *net, const char *name, OP_Operator *op )
{
    return new PRT_RopDriver( net, name, op );
};

PRT_RopDriver::PRT_RopDriver( OP_Network *net, const char *name, OP_Operator *op )

:ROP_Node( net, name, op )
,endTime( 0 )

{};

PRT_RopDriver::~PRT_RopDriver()
{};

int PRT_RopDriver::startRender( int nframes, fpreal s, fpreal e )
{
    UT_String sopPath;
    UT_String filePath;
    
    evalString( sopPath, PRT_ROP_SOPPATH, 0, s );
    evalString( filePath, PRT_ROP_FILE, 0, s );

    if( evalInt("initsim", 0, 0) )
    {
        initSimulationOPs();
        OPgetDirector()->bumpSkipPlaybarBasedSimulationReset(1);
    }

    if( !sopPath.isstring() )
    {
        addError( ROP_COOK_ERROR, "Invalid Sop Path" );
        
        return 0;
    }
    
    if( !filePath.isstring() )
    {
        addError( ROP_COOK_ERROR, "Invalid File Path" );
        
        return 0;
    }
    
    SOP_Node    *sopNode    = getSOPNode( sopPath, 1 );
    
    if( !sopNode )
    {
        addError( ROP_COOK_ERROR, "Sop Not Found" );
        
        return 0;
    }

    startTime = s;
    endTime = e;
    
    if( error() < UT_ERROR_ABORT )
    {
        executePreRenderScript( s );
    }
    
    return 1;
};

ROP_RENDER_CODE PRT_RopDriver::renderFrame( fpreal time, UT_Interrupt *boss )
{
    UT_String sopPath;
    UT_String filePath;
    
    evalString( sopPath, PRT_ROP_SOPPATH, 0, time );
    evalString( filePath, PRT_ROP_FILE, 0, time );
    
    OP_Context                      context(time);
    
    SOP_Node                        *sopNode    = getSOPNode( sopPath, 1 );
    
    GU_DetailHandle                 gdh         = sopNode->getCookedGeoHandle( context );
    GU_DetailHandleAutoReadLock     gdl(gdh);
        
    const GU_Detail                 *gdpPtr     = gdl.getGdp();
    
    if ( !gdpPtr )
    {
        addError( ROP_COOK_ERROR, "Invalid Sop" );
        
        return ROP_ABORT_RENDER;
    }

    if( evalInt("alfprogress", 0, 0) && startTime<endTime )
    {
        // (from ROP_Field3D.C)
        fpreal  fpercent = (time - startTime) / (endTime - startTime);
        int     percent = (int)SYSrint(fpercent * 100);
        percent = SYSclamp(percent, 0, 100);
        fprintf(stdout, "ALF_PROGRESS %d%%\n", percent);
        fflush(stdout);
    }
 
    if( evalInt("mkpath", 0, 0) )
    {
        ROP_Node::makeFilePathDirs(filePath);
    }

    //PRTFile::PRTFile                    prtFile( gdpPtr->points().entries() );
	PRTFile::PRTFile                    prtFile( gdpPtr->getPointRange().getEntries() );

    int attributeCount  = evalInt( ATTRIBS_PRM_NAME.getToken(), 0, time );
    int attStartIndex   = 1;
    
    for( int i = 0; i < attributeCount; ++i )
    {
        int index   = attStartIndex + i;
        
        if( evalIntInst( ATTRIBS_PRM_EXP_ATT_NAME.getToken(), &index, 0, time ) )
        {
            UT_String srcAttName;
            UT_String dstAttName;
            
            evalStringInst( ATTRIBS_PRM_SRC_ATT_NAME.getToken(), &index, srcAttName, 0, time );
            evalStringInst( ATTRIBS_PRM_DST_ATT_NAME.getToken(), &index, dstAttName, 0, time );
            
            GA_Attribute * attribute    = gdpPtr->getAttributeDict(GA_ATTRIB_POINT).find( GA_SCOPE_PUBLIC, srcAttName );
            
            GA_AttributeFilter  numericFilter( GA_AttributeFilter::selectNumeric() );
            
            if( attribute && numericFilter.match( attribute ) )
            {
                const GA_AIFTuple   *aifTuplePtr    = attribute->getAIFTuple();
                
                if( aifTuplePtr )
                {
                    
                    PRTFile::TypeID     attributeChannelType    = PRTFile::PRT_UNKNOWN;
                    GA_Storage          storageType             = aifTuplePtr->getStorage( attribute );
                    
                    int                 arity                   = 0;
                    
                    UT_String           attType;
                    
                    evalStringInst( ATTRIBS_PRM_TYPE_ATT_NAME.getToken(), &index, attType, 0, time );
                    
                    if( attType != "Original" )
                    {
                        if( attType == "float64" )
                        {
                            storageType = GA_STORE_REAL64;
                        }
                        
                        if( attType == "float32" )
                        {
                            storageType = GA_STORE_REAL32;
                        }
#ifdef ENABLE_HALF
                        if( attType == "float16" )
                        {
                            storageType = GA_STORE_REAL16;
                        }
#endif
                        if( attType == "int64" )
                        {
                            storageType = GA_STORE_INT64;
                        }
                        
                        if( attType == "int32" )
                        {
                            storageType = GA_STORE_INT32;
                        }
                    }
                    
                    if( storageType == GA_STORE_INT32 )
                    {
                        attributeChannelType    = PRTFile::PRT_INT_32;
                    }
                    
                    if( storageType == GA_STORE_INT64 )
                    {
                        attributeChannelType    = PRTFile::PRT_INT_64;
                    }
#ifdef ENABLE_HALF
                    if( storageType == GA_STORE_REAL16 )
                    {
                        attributeChannelType    = PRTFile::PRT_FLOAT_16;
                    }
#endif
                    if( storageType == GA_STORE_REAL32 )
                    {
                        attributeChannelType    = PRTFile::PRT_FLOAT_32;
                    }
                    
                    if( storageType == GA_STORE_REAL64 )
                    {
                        attributeChannelType    = PRTFile::PRT_FLOAT_64;
                    }
                    
                    if( attributeChannelType != PRTFile::PRT_UNKNOWN )
                    {
                        if( getPRTPreDefinedAttMaxArityMap().count( dstAttName.toStdString() ) > 0 )
                        {
                            arity               = std::min<int>( attribute->getTupleSize(), getPRTPreDefinedAttMaxArityMap()[ dstAttName.toStdString() ] );
                        } else {
                            arity               = attribute->getTupleSize();
                        }
                        
                        if( attributeChannelType == PRTFile::PRT_INT_32 )
                        {
                            prtFile.addAttribute<PRTFile::INT32>( dstAttName.toStdString(), arity );
                            
                            readAttributeIntoBuffer<int32, PRTFile::INT32>( gdpPtr, attribute, arity, prtFile.getAttributeContainerRef<PRTFile::INT32>( dstAttName.toStdString() ) );
                        }
                        
                        if( attributeChannelType == PRTFile::PRT_INT_64 )
                        {
                            prtFile.addAttribute<PRTFile::INT64>( dstAttName.toStdString(), arity );
                            
                            readAttributeIntoBuffer<int64, PRTFile::INT64>( gdpPtr, attribute, arity, prtFile.getAttributeContainerRef<PRTFile::INT64>( dstAttName.toStdString() ) );
        
                        }
#ifdef ENABLE_HALF
                        if( attributeChannelType == PRTFile::PRT_FLOAT_16 )
                        {
                            prtFile.addAttribute<PRTFile::FLOAT16>( dstAttName.toStdString(), arity );
                            
                            readAttributeIntoBuffer<fpreal32, PRTFile::FLOAT16>( gdpPtr, attribute, arity, prtFile.getAttributeContainerRef<PRTFile::FLOAT16>( dstAttName.toStdString() ) );
                        }
#endif
                        
                        if( attributeChannelType == PRTFile::PRT_FLOAT_32 )
                        {
                            prtFile.addAttribute<PRTFile::FLOAT32>( dstAttName.toStdString(), arity );
                            
                            readAttributeIntoBuffer<fpreal32, PRTFile::FLOAT32>( gdpPtr, attribute, arity, prtFile.getAttributeContainerRef<PRTFile::FLOAT32>( dstAttName.toStdString() ) );
                        }
                        
                        if( attributeChannelType == PRTFile::PRT_FLOAT_64 )
                        {
                            prtFile.addAttribute<PRTFile::FLOAT64>( dstAttName.toStdString(), arity );
                            
                            readAttributeIntoBuffer<fpreal64, PRTFile::FLOAT64>( gdpPtr, attribute, arity, prtFile.getAttributeContainerRef<PRTFile::FLOAT64>( dstAttName.toStdString() ) );
                        }
                    }
                }
            }
        }
    }
    
    prtFile.save( filePath.toStdString() );
    
    if( error() < UT_ERROR_ABORT )
    {
        executePostFrameScript( time );
    }
    
    return ROP_CONTINUE_RENDER;
};

ROP_RENDER_CODE PRT_RopDriver::endRender()
{
    if( evalInt("initsim", 0, 0) )
    {
        initSimulationOPs();
    OPgetDirector()->bumpSkipPlaybarBasedSimulationReset(-1);
    }

    if( error() < UT_ERROR_ABORT )
    {
        executePostRenderScript( endTime );
    }
    
    return ROP_CONTINUE_RENDER;
};

/******* Operator Interface ******/

void newDriverOperator( OP_OperatorTable *table )
{
    table->addOperator(new OP_Operator("PRT_ROPDriver",
                                       "PRT ROPDriver",
                                       PRT_RopDriver::nodeConstructor,
                                       PRT_RopDriver::getTemplatePair(),
                                       0,
                                       0,
                                       PRT_RopDriver::getVariablePair(),
                                       OP_FLAG_GENERATOR));
};

