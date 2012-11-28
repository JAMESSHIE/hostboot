/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kernel/ipc.C $                                            */
/*                                                                        */
/* IBM CONFIDENTIAL                                                       */
/*                                                                        */
/* COPYRIGHT International Business Machines Corp. 2013                   */
/*                                                                        */
/* p1                                                                     */
/*                                                                        */
/* Object Code Only (OCO) source materials                                */
/* Licensed Internal Code Source Materials                                */
/* IBM HostBoot Licensed Internal Code                                    */
/*                                                                        */
/* The source code for this program is not published or otherwise         */
/* divested of its trade secrets, irrespective of what has been           */
/* deposited with the U.S. Copyright Office.                              */
/*                                                                        */
/* Origin: 30                                                             */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */
#include <arch/ppc.H>
#include <kernel/ipc.H>
#include <kernel/cpu.H>
#include <kernel/intmsghandler.H>
#include <kernel/console.H>

using namespace KernelIpc;

namespace KernelIpc
{
    void send(uint64_t i_q, msg_t * i_msg);
};

/**
 * IPC communication area. Interrupt service provider initializes.
 * @see intrrp.C
 */
KernelIpc::ipc_data_area_t KernelIpc::ipc_data_area;

// Put the IPC message in the other nodes memory space
//    Two potential issues:
//    1. The destination node is not there - memory location is nonexistant.
//    2. The destination node never responds, potentially hanging this thread.
void KernelIpc::send(uint64_t i_q, msg_t * i_msg)
{
    // @note
    // Point to memory in the destination image.
    // All host boot images are assured to be at the same code level.
    // PIR node and physical node are not always the same. For instance a
    // single node system with an alt-master could be designed such that the
    // alt-master were on a different logical (power bus numbering) node.
    // Since it's not in plan to use this IPC mechanism in a single node
    // system, this case will ge ignored for now.
    uint64_t this_node = getPIR()/KERNEL_MAX_SUPPORTED_CPUS_PER_NODE;
    uint64_t hrmor_offset = getHRMOR()-(this_node*(ipc_data_area.hrmor_base));
    uint64_t dest_node = (i_q >> 32) & 0x07; 
    uint64_t dest_hrmor = (ipc_data_area.hrmor_base*dest_node) + hrmor_offset;

    uint64_t dest_addr = reinterpret_cast<uint64_t>(&ipc_data_area);
    dest_addr += dest_hrmor;
    dest_addr |= 0x8000000000000000ul;

    printkd("IPC Dest addr %lx\n",dest_addr);

    // pointer to the ipc_data_area in the destination node
    ipc_data_area_t * p_dest = 
        reinterpret_cast<ipc_data_area_t*>(dest_addr);

    // get lock on IPC data area in other node
    while(false == __sync_bool_compare_and_swap(&(p_dest->msg_queue_id),
                                                0,
                                                0xFFFFFFFFFFFFFFFFul))
    {
        setThreadPriorityLow();
    }
    setThreadPriorityHigh();

    p_dest->msg_payload = *i_msg;  // copy in message
    lwsync();

    p_dest->msg_queue_id = i_q;    // set destination queue id
    lwsync();

    // send IPI
    InterruptMsgHdlr::sendIPI(p_dest->pir);

    // The message allocation is freed here to make msg_send for IPC
    // messages behave the same as non-IPC msg_send; that is, the message
    // is freed by the consumer; however, i_msg was allocated in user space
    // code and freed here in kernel space. The assumption is that this is OK.
    msg_free(i_msg);

    return;
}
