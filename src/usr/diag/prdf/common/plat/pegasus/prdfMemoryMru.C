/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/usr/diag/prdf/common/plat/pegasus/prdfMemoryMru.C $       */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* COPYRIGHT International Business Machines Corp. 2008,2014              */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */

/** @file prdfMemoryMru.C */

#include <prdfMemoryMru.H>

#include <prdfCalloutUtil.H>
#include <prdfTrace.H>
#include <prdfCenMemUtils.H>

using namespace TARGETING;

namespace PRDF
{

using namespace MemoryMruData;
using namespace PlatServices;
using namespace CEN_SYMBOL;

MemoryMru::MemoryMru() :
    iv_mbaTarget(NULL), iv_rank(), iv_special(NO_SPECIAL_CALLOUT)
{
    iv_memMruMeld.u = 0;
}

//------------------------------------------------------------------------------

MemoryMru::MemoryMru( uint32_t i_memMru ) :
    iv_mbaTarget(NULL), iv_rank(), iv_special(NO_SPECIAL_CALLOUT)
{
    #define PRDF_FUNC "[MemoryMru::MemoryMru] "

    iv_memMruMeld.u = i_memMru;

    iv_memMruMeld.s.valid = 0; // initially invalidated

    do
    {
        // Get MBA target using node, proc, membuff and mba position
        TargetHandle_t system = getSystemTarget();

        if ( NULL == system )
        {
            PRDF_ERR( PRDF_FUNC"Could not find system target" );
            break;
        }

        TargetHandle_t node = getConnectedChild( system, TYPE_NODE,
                                                 iv_memMruMeld.s.nodePos );
        if ( NULL == node )
        {
            PRDF_ERR( PRDF_FUNC"Could not find functional node attached to "
                      "system at pos: %u", iv_memMruMeld.s.nodePos );
            break;
        }

        TargetHandle_t proc = getConnectedChild( node, TYPE_PROC,
                                                 iv_memMruMeld.s.procPos );
        if ( NULL == proc )
        {
            PRDF_ERR( PRDF_FUNC"Could not find functional  proc attached to "
                      "node 0x%08X at pos: %u", getHuid( node ),
                       iv_memMruMeld.s.procPos );
            break;
        }

        TargetHandle_t membuff = getConnectedChild( proc, TYPE_MEMBUF,
                                                   iv_memMruMeld.s.cenPos );
        if ( NULL == membuff )
        {
            PRDF_ERR( PRDF_FUNC"Could not find functional  membuff attached to "
                      "proc 0x%08X at pos: %u", getHuid( proc ),
                       iv_memMruMeld.s.cenPos );
            break;
        }

        iv_mbaTarget = getConnectedChild( membuff, TYPE_MBA,
                                          iv_memMruMeld.s.mbaPos );
        if ( NULL == iv_mbaTarget )
        {
            PRDF_ERR( PRDF_FUNC"Could not find functional MBA attached to "
                      "membuff 0x%08X at pos: %u", getHuid( membuff ),
                       iv_memMruMeld.s.mbaPos );
            break;
        }


        // Get the rank
        iv_rank = (0 == iv_memMruMeld.s.srankValid)
                    ? CenRank( iv_memMruMeld.s.mrank )
                    : CenRank( iv_memMruMeld.s.mrank, iv_memMruMeld.s.srank );

        // Get the symbol or special callout
        if ( (FIRST_SPECIAL_CALLOUT <= iv_memMruMeld.s.symbol) &&
             (iv_memMruMeld.s.symbol <= LAST_SPECIAL_CALLOUT) )
        {
            iv_special = (MemoryMruData::Callout)iv_memMruMeld.s.symbol;
        }
        else
        {
            if ( SYMBOLS_PER_RANK <= iv_memMruMeld.s.symbol )
            {
                PRDF_ERR( PRDF_FUNC"Invalid symbol value :%u",
                                    iv_memMruMeld.s.symbol );
                break;
            }

            iv_symbol = CenSymbol::fromSymbol( iv_mbaTarget, iv_rank,
                                               iv_memMruMeld.s.symbol,
                                               iv_memMruMeld.s.pins );
            if ( !iv_symbol.isValid() )
            {
                PRDF_ERR( PRDF_FUNC"fromSymbol() failed" );
                break;
            }

            if ( iv_memMruMeld.s.dramSpared ) iv_symbol.setDramSpared();
            if ( iv_memMruMeld.s.eccSpared  ) iv_symbol.setEccSpared();

            // Validation checks
            CEN_SYMBOL::WiringType type = iv_symbol.getWiringType();

            if ( type != CEN_SYMBOL::WiringType (iv_memMruMeld.s.wiringType))
            {
                PRDF_ERR( PRDF_FUNC"Wiring Type does not match type:%u "
                                    "iv_memMruMeld.s.wiringType :%u",
                                    type, iv_memMruMeld.s.wiringType);
                break;
            }

        }

        // If the code gets to this point the MemoryMru is valid.
        iv_memMruMeld.s.valid = 1;

    } while (0);

    #undef PRDF_FUNC
}

//------------------------------------------------------------------------------

MemoryMru::MemoryMru( TARGETING::TargetHandle_t i_mbaTarget,
                      const CenRank & i_rank, const CenSymbol & i_symbol ) :
    iv_mbaTarget(i_mbaTarget), iv_rank(i_rank),
    iv_special(NO_SPECIAL_CALLOUT), iv_symbol( i_symbol )
{
    #define PRDF_FUNC "[MemoryMru::MemoryMru] "

    iv_memMruMeld.u = 0;

    do
    {
        TargetHandle_t node = getConnectedParent( iv_mbaTarget, TYPE_NODE );
        if ( NULL == node )
        {
            PRDF_ERR( PRDF_FUNC"Could not find node attached to MBA 0x%08x",
                      getHuid(iv_mbaTarget) );
            break;
        }

        TargetHandle_t proc = getConnectedParent( iv_mbaTarget, TYPE_PROC );
        if ( NULL == proc )
        {
            PRDF_ERR( PRDF_FUNC"Could not find proc attached to MBA 0x%08x",
                      getHuid(iv_mbaTarget) );
            break;
        }

        TargetHandle_t memBuff = getConnectedParent( iv_mbaTarget,
                                                     TYPE_MEMBUF );
        if ( NULL == memBuff )
        {
            PRDF_ERR( PRDF_FUNC"Could not find memBuff attached to MBA 0x%08x",
                      getHuid(iv_mbaTarget) );
            break;
        }

        iv_memMruMeld.s.nodePos    = getTargetPosition( node );
        iv_memMruMeld.s.procPos    = getTargetPosition( proc );
        iv_memMruMeld.s.cenPos     = getTargetPosition( memBuff );
        iv_memMruMeld.s.mbaPos     = getTargetPosition( iv_mbaTarget );
        iv_memMruMeld.s.mrank      = iv_rank.getMaster();
        iv_memMruMeld.s.srank      = iv_rank.getSlave();
        iv_memMruMeld.s.srankValid = iv_rank.isSlaveValid();
        iv_memMruMeld.s.symbol     = iv_symbol.getSymbol();
        iv_memMruMeld.s.pins       = iv_symbol.getPins();
        iv_memMruMeld.s.dramSpared = iv_symbol.isDramSpared() ? 1 : 0;
        iv_memMruMeld.s.eccSpared  = iv_symbol.isEccSpared()  ? 1 : 0;
        iv_memMruMeld.s.wiringType = iv_symbol.getWiringType();
        // If the code gets to this point the MemoryMru is valid.
        iv_memMruMeld.s.valid = 1;

    } while (0);

    #undef PRDF_FUNC
}

//------------------------------------------------------------------------------

MemoryMru::MemoryMru( TARGETING::TargetHandle_t i_mbaTarget,
                      const CenRank & i_rank,
                      MemoryMruData::Callout i_specialCallout ) :
    iv_mbaTarget(i_mbaTarget), iv_rank(i_rank), iv_special(i_specialCallout)
{
    #define PRDF_FUNC "[MemoryMru::MemoryMru] "

    iv_memMruMeld.u = 0;

    do
    {
        TargetHandle_t node = getConnectedParent( iv_mbaTarget, TYPE_NODE );
        if ( NULL == node )
        {
            PRDF_ERR( PRDF_FUNC"Could not find node attached to MBA 0x%08x",
                      getHuid(iv_mbaTarget) );
            break;
        }

        TargetHandle_t proc = getConnectedParent( iv_mbaTarget, TYPE_PROC );
        if ( NULL == proc )
        {
            PRDF_ERR( PRDF_FUNC"Could not find proc attached to MBA 0x%08x",
                      getHuid(iv_mbaTarget) );
            break;
        }

        TargetHandle_t memBuff = getConnectedParent( iv_mbaTarget,
                                                     TYPE_MEMBUF );
        if ( NULL == memBuff )
        {
            PRDF_ERR( PRDF_FUNC"Could not find memBuff attached to MBA 0x%08x",
                      getHuid(iv_mbaTarget) );
            break;
        }

        iv_memMruMeld.s.nodePos    = getTargetPosition( node );
        iv_memMruMeld.s.procPos    = getTargetPosition( proc );
        iv_memMruMeld.s.cenPos     = getTargetPosition( memBuff );
        iv_memMruMeld.s.mbaPos     = getTargetPosition( iv_mbaTarget );
        iv_memMruMeld.s.mrank      = iv_rank.getMaster();
        iv_memMruMeld.s.srank      = iv_rank.getSlave();
        iv_memMruMeld.s.srankValid = iv_rank.isSlaveValid();
        iv_memMruMeld.s.symbol     = iv_special;

        // If the code gets to this point the MemoryMru is valid.
        iv_memMruMeld.s.valid = 1;

    } while (0);

    #undef PRDF_FUNC
}

//------------------------------------------------------------------------------

TargetHandleList MemoryMru::getCalloutList() const
{
    #define PRDF_FUNC "[MemoryMru::getCalloutList] "

    TargetHandleList o_list;

    if ( 0 == iv_memMruMeld.s.valid )
    {
        PRDF_ERR( PRDF_FUNC"MemoryMru 0x%08x not valid.", iv_memMruMeld.u );
    }
    else
    {
        if ( NO_SPECIAL_CALLOUT != iv_special )
        {
            switch ( iv_special )
            {
                case CALLOUT_RANK:
                    o_list = CalloutUtil::getConnectedDimms( iv_mbaTarget,
                                                             iv_rank );
                    break;
                case CALLOUT_RANK_AND_MBA:
                    o_list = CalloutUtil::getConnectedDimms( iv_mbaTarget,
                                                             iv_rank );
                    o_list.push_back( iv_mbaTarget );
                    break;
                case CALLOUT_ALL_MEM:
                    o_list = CalloutUtil::getConnectedDimms( iv_mbaTarget );
                    break;
                default:
                    PRDF_ERR( PRDF_FUNC"MemoryMruData::Callout 0x%02x not "
                              "supported", iv_special );
            }
        }
        else
        {
            // Add DIMM represented by symbol
            uint8_t ps = iv_symbol.getPortSlct();
            if ( iv_memMruMeld.s.eccSpared ) ps = 1; // Adjust for ECC spare
            o_list = CalloutUtil::getConnectedDimms( iv_mbaTarget,
                                                     iv_rank, ps );
        }
    }

    return o_list;

    #undef PRDF_FUNC
}

} // end namespace PRDF

