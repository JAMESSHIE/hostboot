/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/usr/errl/errlprvt.H $                             */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* COPYRIGHT International Business Machines Corp. 2011,2014              */
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
#ifndef ERRLPRVT_H
#define ERRLPRVT_H

/**
 *  @file errlprvt.H
 *
 *  @brief A class for the private header 'PH' section of an error log.
 *  Manages the data destined for the PEL PH section.
 *
*/

#include <errl/errlsctnhdr.H>


namespace ERRORLOG
{

class ErrlPrvt
{

    // ErrlEntry may access private elements of this class.  Data items that
    // you would expect to be part of ErrlEntry are actually instance data
    // in this class.
    friend class ErrlEntry;


private:

    enum constants
    {
        SLEN            = 40,  // section length w/o sizeof(ErrlSctnHdr)
        SST             = 0,   // section type
        VER             = 1    // section version
    };

    /**
     *  @brief Constructor will initialize as much data in the private
     *  header as it can, including asking the ErrlManager for the error
     *  log ID. Also sets the time of creation.
     *
     *  @param[in] i_CreatorCompId   Creator of this error log.
     */
    ErrlPrvt( compId_t  i_CreatorCompId );


    /**
     *  @brief Destructor
     */
    ~ErrlPrvt();


    /**
      * @brief   Disable copy constructor and assignment operator.
      */
    ErrlPrvt(const ErrlPrvt& i_right);
    ErrlPrvt& operator=(const ErrlPrvt& i_right);


    /**
     *  @brief  Calculates the flattened size of this.
     *
     *  @return Count of bytes needed to flatten this.
     */
    uint64_t flatSize() const;


    /**
     *  @brief   Flatten this to the output buffer given.
     *
     *  @param[out]  o_pBuffer   Pointer to memory buffer where data will go.
     *  @param[in]   i_cbBuffer  Count of bytes in memory buffer.
     *                           Call flatSize() first to know how big the
     *                           buffer should be before calling flatten()
     *
     *  @return  Count of bytes flattened, or zero on error. The likely error
     *           is that the buffer is of insufficient size.
     */
    uint64_t flatten( void * o_pBuffer, const uint64_t i_cbBuffer );

    /**
     * @brief Import flattened data
     * @parm[in] i_buf, pointer to the flattened data
     * @return The number of bytes consumed
     */
    uint64_t unflatten( const void * i_buf );



    // Instance data

    ErrlSctnHdr	iv_header;    // section header for any/all sections

    uint64_t    iv_created;   // TODO RTC 35258 Expects BCD_time8_t, but using
                              // timebase
    uint64_t    iv_committed; // TODO RTC 35258 Expects BCD_time8_t, but using
                              // timebase
    uint8_t     iv_cid;       // Creator 'B' (enum errlCreator)
    uint8_t     iv_sctns;     // count of sections
    uint32_t    iv_plid;      // platform log id
    uint32_t    iv_eid;       // Error Log ID
};



/*****************************************************************************/
// Destructor

inline ErrlPrvt::~ErrlPrvt()
{
}


/*****************************************************************************/
// Export data size

inline uint64_t ErrlPrvt::flatSize() const
{
    return ( iv_header.flatSize() + ErrlPrvt::SLEN );
}




} // namespace


#endif //ERRLPRVT_H
