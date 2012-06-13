/*  IBM_PROLOG_BEGIN_TAG
 *  This is an automatically generated prolog.
 *
 *  $Source: src/include/usr/hwas/hostbootIstep.H $
 *
 *  IBM CONFIDENTIAL
 *
 *  COPYRIGHT International Business Machines Corp. 2012
 *
 *  p1
 *
 *  Object Code Only (OCO) source materials
 *  Licensed Internal Code Source Materials
 *  IBM HostBoot Licensed Internal Code
 *
 *  The source code for this program is not published or other-
 *  wise divested of its trade secrets, irrespective of what has
 *  been deposited with the U.S. Copyright Office.
 *
 *  Origin: 30
 *
 *  IBM_PROLOG_END_TAG
 */
/**
 *  @file hostbootIstep.H
 *
 *  @brief Defines hostboot functions that get called by the istep dispatcher
 *         and call into the HWAS common interface
 */

#ifndef HOSTBOOTISTEP_H_
#define HOSTBOOTISTEP_H_

namespace HWAS
{

// hostboot only functions that are called by the istep dispatcher
//  and then call the HWAS common functions

/**
 * @brief   host_init_fsi   Setup the FSI links to slave chips
 *
 *  Call into the hwas platform-specific initHardware() function
 *  to init the FSI hardware.
 *
 *  param[in,out] io_pArgs  -   (normally) a pointer to a TaskArgs struct,
 *                              or NULL.
 *  return  none
 */
void host_init_fsi( void *io_pArgs );

/**
 * @brief   host_set_ipl_parms  Build ipl parameters
 *
 *  TBD
 *
 *  param[in,out] io_pArgs  -   (normally) a pointer to a TaskArgs struct,
 *                              or NULL.
 *  return  none
 */
void host_set_ipl_parms( void *io_pArgs );

/**
 * @brief   host_discover_targets   Builds targeting
 *
 *  This routine will walk through all the targets and initialize HWAS STATE
 *  to a known default value.  
 *
 *  Currently everything is initialized to powered off, etc.
 *
 *  Call into the hwas platform-specific presenceDetect() function to
 *  read the hardware information, and apply it to the target states.
 *
 *  param[in,out] io_pArgs  -   (normally) a pointer to a TaskArgs struct,
 *                              or NULL.
 *  return  none
 */
void host_discover_targets( void *io_pArgs );

/**
 * @brief   host_gard   Do Gard
 *
 * Collect GARD information and apply it to the target states.
 *
 *  param[in,out] io_pArgs  -   (normally) a pointer to a TaskArgs struct,
 *                              or NULL.
 *  return  none
 */
void host_gard( void *io_pArgs );

/**
 * @brief   host_cancontinue_clear  Clear deconfigured status
 *
 *  TBD
 *
 *  param[in,out] io_pArgs  -   (normally) a pointer to a TaskArgs struct,
 *                              or NULL.
 *  return  none
 */
void host_cancontinue_clear( void *io_pArgs );

/**
 * @brief   proc_check_slave_sbe_seeprom_complete   Check Slave SBE Complete
 *
 *  TBD
 *
 *  param[in,out] io_pArgs  -   (normally) a pointer to a TaskArgs struct,
 *                              or NULL.
 *  return  none
 */
void proc_check_slave_sbe_seeprom_complete( void *io_pArgs );

/**
 * @brief   proc_xmit_sbe   vSBE Init of Slave Chips
 *
 *  TBD
 *
 *  param[in,out] io_pArgs  -   (normally) a pointer to a TaskArgs struct,
 *                              or NULL.
 *  return  none
 */
void proc_xmit_sbe( void *io_pArgs );

} // namespace HWAS
#endif // HOSTBOOTISTEP_H_