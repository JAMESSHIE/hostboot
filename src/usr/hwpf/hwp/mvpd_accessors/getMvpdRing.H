/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/usr/hwpf/hwp/mvpd_accessors/getMvpdRing.H $               */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* COPYRIGHT International Business Machines Corp. 2012,2014              */
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
// $Id: RepairRingFunc.H,v 1.1 2012/07/19 22:00:38 mjjones Exp $
/**
 *  @file getMvpdRing.H
 *
 *  @brief Prototype for getMvpdRing() -
 *          get a repair ring from a MVPD record
 */

 #ifndef    _HWP_GETMVPDRING_
 #define    _HWP_GETMVPDRING_

 #include   <fapi.H>

// function pointer typedef definition for HWP call support
typedef fapi::ReturnCode (*getMvpdRing_FP_t)
     (fapi::MvpdRecord,fapi::MvpdKeyword,const fapi::Target &,
      const uint8_t, const uint8_t, uint8_t *, uint32_t &);


extern "C"
{
/**
 * @brief   get specified ring from MVPD for the specified target CPU.
 *
 * A Ring Id Chiplet Id should be unique in the mvpd Record.
 * The code does not validate. No assumption should be made on which would
 * be returned if there are multiple.
 *
 * @param i_record          -   Record enumerator 
 * @param i_keyword         -   Keyword enumerator 
 * Supported Rings are: 
 *                   MVPD_RECORD_CP00 - MVPD_KEYWORD_PDR
 *                   MVPD_RECORD_CP00 - MVPD_KEYWORD_PDG
 * @param i_fapiTarget      -   cpu target
 * @param i_chipletId       -   Chiplet ID
 * @param i_ringId          -   Ring ID
 * @param i_pRingBuf        -   pointer to a buffer allocated by the caller 
 *                              to receive the ring header and data.
 *                              if NULL, the size of the min buffer required
 *                              buffer will be returned in io_rRingBufsize 
 *                              with rc FAPI_RC_SUCCESS.
 * @param io_rRingBufsize   -   in:     size of ring buffer that caller has
 *                                      allocated
 *                              out:    number of BYTES that were copied to the
 *                                      output buffer.
 *                                      If the ring was not found, an error
 *                                      will be returned and this will be 0.
 *                                      If the output buffer is not big enough,
 *                                      an error will be returned and this will
 *                                      be the minimum size required.
 *                              The buffer contains the CompressedScanData
 *                              structure followed by compressed data. The 
 *                              caller does compression and decompression.
 *  Buffer:   io_rRingBufsize returns xNN. 
 *    byte x0  CompressedScanData structure (rs4 header)
 *    byte x18 compressed data (sizeof CompressedScanData is 0x18)
 *    byte xNN last byte of compressed data
 *
 * @return fapi::ReturnCode -   FAPI_RC_SUCCESS if success,
 *                              relevant error code for failure.
 */
fapi::ReturnCode getMvpdRing( fapi::MvpdRecord i_record,
                              fapi::MvpdKeyword i_keyword,
                              const fapi::Target  &i_fapiTarget,
                              const uint8_t       i_chipletId,
                              const uint8_t       i_ringId,
                              uint8_t             *io_pRingBuf,
                              uint32_t            &io_rRingBufsize );

}

#endif
