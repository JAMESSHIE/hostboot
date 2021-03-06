/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/usr/hwpf/hwp/dram_initialization/mss_extent_setup/mss_extent_setup.C $ */
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
// $Id: mss_extent_setup.C,v 1.8 2012/07/17 13:24:10 bellows Exp $
//------------------------------------------------------------------------------
// Don't forget to create CVS comments when you check in your changes!
//------------------------------------------------------------------------------
//Owner :- Girisankar paulraj
//Back-up owner :- Mark bellows
//
// CHANGE HISTORY:
//------------------------------------------------------------------------------
// Version:|  Author: |  Date:  | Comment:
//---------|----------|---------|-----------------------------------------------
//  1.8    | bellows  |16-Jul-12| added in Id tag
//  1.7    | bellows  |15-Jun-12| Updated for Firmware
//  1.3    | gpaulraj |11-Nov-11| modified according HWPF format
//  1.2    | gpaulraj |02-oct-11| supported for MCS loop - SIM model. compiled in the ecmd & FAPI calls included.
//  1.1    | gpaulraj |31-jul-11| First drop for centaur
//----------------------------------------------------------------------
//  Includes
//----------------------------------------------------------------------
//#include <prcdUtils.H>
//#include <verifUtils.H>
//#include <cen_maintcmds.H>
#include <mss_extent_setup.H>
//----------------------------------------------------------------------
//  eCMD Includes
//----------------------------------------------------------------------
//#include <ecmdClientCapi.H>
//#include <ecmdDataBuffer.H>
//#include <ecmdUtils.H>
//#include <ecmdSharedUtils.H>
//#include <fapiClientCapi.H>
//#include <croClientCapi.H>
//#include <fapi.H>
//#include <fapiSystemConfig.H> 
//#include <fapiSharedUtils.H>
// attributes listing
// FAPI procedure calling
//
extern "C" {

using namespace fapi;


ReturnCode mss_extent_setup(){

    ReturnCode rc;
	    if(rc){
		FAPI_ERR(" Calling Extent function Error ");
		return rc;
	    }
    return rc;
}


} //end extern C

