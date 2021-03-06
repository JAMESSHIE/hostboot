/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/usr/initservice/baseinitsvc/initservice.C $               */
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

/**
 * @file    initservice.C
 *
 *  Implements Initialization Service for Host boot.
 *  See initservice.H for details
 *
 */
#define __HIDDEN_SYSCALL_SHUTDOWN

#include    <kernel/console.H>                  // printk status

#include    <sys/vfs.h>
#include    <vfs/vfs.H>
#include    <sys/task.h>
#include    <sys/misc.h>
#include    <trace/interface.H>
#include    <errl/errlentry.H>
#include    <errl/errlmanager.H>
#include    <sys/sync.h>
#include    <sys/mm.h>
#include    <vmmconst.h>
#include    <sys/time.h>
#include    <hwas/common/hwas_reasoncodes.H>


#include    <errl/errludstring.H>
#include    <errl/errludprintk.H>

#include    <initservice/taskargs.H>        // TASK_ENTRY_MACRO

#include    "initservice.H"
#include    "initsvctasks.H"


//  -----   namespace   SPLESS  -----------------------------------------------
namespace   SPLESS
{
    //  allocate space for SPLess Command regs in the base image.
    uint64_t    g_SPLess_Command_Reg    =   0;
    uint64_t    g_SPLess_Status_Reg     =   0;

}
//  -----   end namespace   SPLESS  ---------------------------------------


namespace   INITSERVICE
{

trace_desc_t *g_trac_initsvc = NULL;
TRAC_INIT(&g_trac_initsvc, "INITSVC", 2*KILOBYTE );

/**
 *  @brief  start() task entry procedure
 *  This one is "special" since we do not return anything to the kernel/vfs
 */
extern "C"
void* _start(void *ptr)
{
    TRACFCOMP( g_trac_initsvc,
            "Executing Initialization Service module." );

    // initialize the base modules in Hostboot.
    InitService::getTheInstance().init( ptr );

    TRACFCOMP( g_trac_initsvc,
            "return from Initialization Service module." );

    return NULL;
}



errlHndl_t  InitService::checkNLoadModule( const TaskInfo *i_ptask ) const
{
    errlHndl_t      l_errl  =   NULL;
    const   char    *l_modulename   =   NULL;

    assert(i_ptask->taskflags.task_type   ==  START_FN );

    do  {

        //  i_ptask->taskflags.task_type   ==  STARTFN
        l_modulename  =   VFS::module_find_name(
                reinterpret_cast<void*>(i_ptask->taskfn) );
        if  ( l_modulename  ==   NULL )
        {
            /*@     errorlog tag
             *  @errortype      ERRL_SEV_CRITICAL_SYS_TERM
             *  @moduleid       BASE_INITSVC_MOD_ID
             *  @reasoncode     INITSVC_LOAD_MODULE_FAILED
             *  @userdata1      0
             *  @userdata2      0
             *
             *  @devdesc        Initialization Service failed to load a
             *                  module needed to load a function or task.
             *                  UserDetails will contain the name of the
             *                  function or task.
             */
            const bool hbSwError = true;
            l_errl = new ERRORLOG::ErrlEntry(
                    ERRORLOG::ERRL_SEV_CRITICAL_SYS_TERM,
                    INITSERVICE::BASE_INITSVC_MOD_ID,
                    INITSERVICE::INITSVC_LOAD_MODULE_FAILED,
                    0, 0, hbSwError);
            break;
        }

        TRACDCOMP( g_trac_initsvc,
                "checkNLoadModule: found %s in module %s",
                i_ptask->taskname,
                ((l_modulename!=NULL)?l_modulename:"NULL???") );

        if (  !VFS::module_is_loaded( l_modulename )
        )
        {
            TRACDCOMP( g_trac_initsvc,
                    "loading module %s",
                    l_modulename );
            l_errl = VFS::module_load( l_modulename );
            if ( l_errl )
            {
                //  load module returned with errl set
                TRACFCOMP( g_trac_initsvc,
                        "module_load( %s ) returned with an error.",
                        l_modulename );

                //  break out of do block
                break;
            }
        }

    }   while( 0 );     // end do() block


    return  l_errl;
}


errlHndl_t InitService::startTask(
        const TaskInfo       *i_ptask,
        void                 *io_pargs ) const
{
    tid_t       l_tidlnchrc     =   0;
    tid_t       l_tidretrc      =   0;
    errlHndl_t  l_errl          =   NULL;
    int         l_childsts      =   0;
    void        *l_childerrl    =   NULL;


    assert( i_ptask != NULL );

    do  {
        // Base modules have already been loaded and initialized,
        // extended modules have not.
        if ( i_ptask->taskflags.module_type == EXT_IMAGE )
        {
            // load module if necessary
            l_errl = VFS::module_load( i_ptask->taskname );
        }
        if ( l_errl )
        {
            TRACFCOMP(g_trac_initsvc,
                    "ERROR: failed to load module for task '%s'",
                    i_ptask->taskname);

            // drop out of do block with errl set
            break;
        }

        // launch a task and wait for it.
        l_tidlnchrc = task_exec( i_ptask->taskname, io_pargs );
        TRACDCOMP( g_trac_initsvc,
                "launch task %s returned %d",
                i_ptask->taskname,
                l_tidlnchrc );

        //  process the return - kernel returns a 16-bit signed # as a
        //  threadid/error
        if ( static_cast<int16_t> (l_tidlnchrc) < 0 )
        {
            // task failed to launch, post an errorlog and dump some trace
            TRACFCOMP(g_trac_initsvc,
                    "ERROR 0x%x: starting task '%s'",
                    l_tidlnchrc,
                    i_ptask->taskname);

            /*@     errorlog tag
             *  @errortype      ERRL_SEV_CRITICAL_SYS_TERM
             *  @moduleid       BASE_INITSVC_MOD_ID
             *  @reasoncode     START_TASK_FAILED
             *  @userdata1      0
             *  @userdata2      task id or task return code
             *
             *  @devdesc        Initialization Service failed to start a task.
             *
             */
            const bool hbSwError = true;
            l_errl = new ERRORLOG::ErrlEntry(
                    ERRORLOG::ERRL_SEV_CRITICAL_SYS_TERM,
                    INITSERVICE::BASE_INITSVC_MOD_ID,
                    INITSERVICE::START_TASK_FAILED,
                    0, l_tidlnchrc, hbSwError);
            break;
        } // endif tidlnchrc

        TRACDCOMP(g_trac_initsvc,
                            "Wait for tid %d '%s'",
                            l_tidlnchrc,
                            i_ptask->taskname);

        //  wait here for the task to end.
        //  status of the task ( OK or Crashed ) is returned in l_childsts
        //  if the task returns an errorlog, it will be returned
        //  in l_childerrl
        l_tidretrc  =   task_wait_tid(
                                l_tidlnchrc,
                                &l_childsts,
                                &l_childerrl );
        if (    ( static_cast<int16_t>(l_tidretrc) < 0 )
             || ( l_childsts != TASK_STATUS_EXITED_CLEAN )
        )
        {
            // the launched task failed or crashed,
            //  post an errorlog and dump some trace
            /*@     errorlog tag
             *  @errortype      ERRL_SEV_CRITICAL_SYS_TERM
             *  @moduleid       BASE_INITSVC_MOD_ID
             *  @reasoncode     WAIT_TASK_FAILED
             *  @userdata1      task id or task return code
             *  @userdata2      returned status from task
             *
             *  @devdesc        Initialization Service launched a task and
             *                  the task returned an error.
             *
             */
            const bool hbSwError = true;
            l_errl = new ERRORLOG::ErrlEntry(
                                    ERRORLOG::ERRL_SEV_CRITICAL_SYS_TERM,
                                    INITSERVICE::BASE_INITSVC_MOD_ID,
                                    INITSERVICE::WAIT_TASK_FAILED,
                                    l_tidretrc, l_childsts, hbSwError);

            // Add Printk Buffer for FFDC.
            ERRORLOG::ErrlUserDetailsPrintk().addToLog(l_errl);

            // If the task crashed, then the l_childerrl is either NULL or
            // contains an RC indicating that the issue causing the child
            // to crash has already been reported.  Therefore, reduce the
            // severity to informational.
            if ((l_childsts == TASK_STATUS_CRASHED) &&
                (NULL != l_childerrl))
            {
                l_errl->setSev(ERRORLOG::ERRL_SEV_INFORMATIONAL,
                               true);  //set severity "final"
            }
            break;
        } // endif tidretrc

        //  check for returned errorlog
        if ( l_childerrl != NULL )
        {
            // cast to the correct type and return
            l_errl  =   reinterpret_cast<errlHndl_t>(l_childerrl);

            // break out of do block
            break;
        }

    }   while(0);   // end do block

    if ( l_errl )
    {
        // Add the task name as user detail data to any errorlog
        ERRORLOG::ErrlUserDetailsString(i_ptask->taskname).addToLog(l_errl);
    }

    //  return any errorlog to the caller
    return l_errl;
}   // startTask()


errlHndl_t InitService::executeFn(
                        const TaskInfo  *i_ptask,
                        void            *io_pargs ) const
{
    tid_t       l_tidlnchrc     =   0;
    tid_t       l_tidretrc      =   0;
    errlHndl_t  l_errl          =   NULL;
    int         l_childsts      =   0;
    void        *l_childerrl    =   NULL;

    assert( i_ptask != NULL );
    assert( i_ptask->taskfn != NULL ) ;


    do  {

        // If the module is not in the base image, then we must ensure that
        // the module has been loaded already.
        if (BASE_IMAGE != i_ptask->taskflags.module_type)
        {
            l_errl = checkNLoadModule( i_ptask );
        }
        if ( l_errl )
        {
            TRACFCOMP(g_trac_initsvc,
                    "ERROR: failed to load module for task '%s'",
                    i_ptask->taskname);

            //  break out with errorlog set
            break;
        }

        //  valid function, launch it
        l_tidlnchrc = task_create( i_ptask->taskfn, io_pargs);
        if (static_cast<int16_t> (l_tidlnchrc) < 0)
        {
            TRACFCOMP(g_trac_initsvc,
                    "ERROR %d: starting function in task'%s'",
                    l_tidlnchrc,
                    i_ptask->taskname);

            /*@     errorlog tag
             *  @errortype      ERRL_SEV_CRITICAL_SYS_TERM
             *  @moduleid       BASE_INITSVC_MOD_ID
             *  @reasoncode     START_FN_FAILED
             *  @userdata1      task return code
             *  @userdata2      0
             *
             *  @devdesc        Initialization Service attempted to start a
             *                  function within a module but the function
             *                  failed to launch
             */
            const bool hbSwError = true;
            l_errl = new ERRORLOG::ErrlEntry(
                                    ERRORLOG::ERRL_SEV_CRITICAL_SYS_TERM,
                                    INITSERVICE::BASE_INITSVC_MOD_ID,
                                    INITSERVICE::START_FN_FAILED,
                                    l_tidlnchrc, 0, hbSwError);
            break;
        } // endif tidlnchrc

        //  wait here for the task to end.
        //  status of the task ( OK or Crashed ) is returned in l_childsts
        //  if the task returns an errorlog, it will be returned
        //  in l_childerrl
        l_tidretrc  =   task_wait_tid(
                                l_tidlnchrc,
                                &l_childsts,
                                &l_childerrl );
        if (    ( static_cast<int16_t>(l_tidretrc) < 0 )
             || ( l_childsts != TASK_STATUS_EXITED_CLEAN )
            )
        {
            // the launched task failed or crashed
            //  post an errorlog and dump some trace
            /*@     errorlog tag
             *  @errortype      ERRL_SEV_CRITICAL_SYS_TERM
             *  @moduleid       BASE_INITSVC_MOD_ID
             *  @reasoncode     WAIT_FN_FAILED
             *  @userdata1      task id or task return code
             *  @userdata2      returned status from task
             *
             *  @devdesc        Initialization Service launched a function and the task returned an error.
             *
             *
             */
            const bool hbSwError = true;
            l_errl = new ERRORLOG::ErrlEntry(
                                    ERRORLOG::ERRL_SEV_CRITICAL_SYS_TERM,
                                    INITSERVICE::BASE_INITSVC_MOD_ID,
                                    INITSERVICE::WAIT_FN_FAILED,
                                    l_tidretrc, l_childsts, hbSwError);

            // Add Printk Buffer for FFDC.
            ERRORLOG::ErrlUserDetailsPrintk().addToLog(l_errl);

            // If the task crashed, then the l_childerrl is either NULL or
            // contains an RC indicating that the issue causing the child
            // to crash has already been reported.  Therefore, reduce the
            // severity to informational.
            if ((l_childsts == TASK_STATUS_CRASHED) &&
                (NULL != l_childerrl))
            {
                l_errl->setSev(ERRORLOG::ERRL_SEV_INFORMATIONAL,
                               true);  //set severity "final"
            }

            TRACFCOMP(g_trac_initsvc,
                    "ERROR : task_wait_tid(0x%x). '%s', l_tidretrc=0x%x, l_childsts=0x%x",
                    l_tidlnchrc,
                    i_ptask->taskname,
                    l_tidretrc,
                    l_childsts );

            //  break out of do block
            break;
        } // endif tidretrc

        //  check for returned errorlog
        if ( l_childerrl != NULL )
        {
            TRACFCOMP(g_trac_initsvc,
                    "ERROR : task_wait_tid(0x%x). '%s', l_childerrl=%p",
                    l_tidlnchrc,
                    i_ptask->taskname,
                    l_childerrl  );

            // cast to the correct type and return
            l_errl  =   reinterpret_cast<errlHndl_t>(l_childerrl);

            // break out of do block
            break;
        }

    }   while( 0 );     // end do block

    if ( l_errl )
    {
        // Add the task name as user detail data to any errorlog that was
        //  posted.
        ERRORLOG::ErrlUserDetailsString(i_ptask->taskname).addToLog(l_errl);
    }

    return l_errl;
}

errlHndl_t  InitService::dispatchTask( const TaskInfo   *i_ptask,
                                       void             *io_pargs ) const
{
    errlHndl_t   l_errl  =   NULL;


    //  dispatch tasks...
    switch ( i_ptask->taskflags.task_type)
    {
    case NONE:
        //  task is a place holder, skip
        TRACDCOMP( g_trac_initsvc,
                   "task_type==NONE : %s",
                   i_ptask->taskname );
        break;
    case START_TASK:
        TRACDCOMP( g_trac_initsvc,
                  "task_type==START_TASK: %s",
                  i_ptask->taskname );
        l_errl = startTask( i_ptask,
                            io_pargs );
        break;
    case INIT_TASK:
        TRACDCOMP( g_trac_initsvc,
                   "task_type==INIT_TASK: %s",
                   i_ptask->taskname);

        l_errl = VFS::module_load( i_ptask->taskname );
        break;
    case START_FN:
        TRACDCOMP( g_trac_initsvc,
                   "task_type==START_FN : %s %p",
                   i_ptask->taskname,
                   i_ptask->taskfn );
        l_errl = executeFn( i_ptask,
                            io_pargs );
        break;
    case UNINIT_TASK:
        TRACDCOMP( g_trac_initsvc,
                  "task_type=UNINIT_TASK : %s ",
                  i_ptask->taskname );
        l_errl = VFS::module_unload( i_ptask->taskname );
        break;
    default:
        /**
         * @note    If there is a bad TaskInfo struct we just stop here.
         */
        TRACFCOMP( g_trac_initsvc,
                   "Invalid task_type %d: ABORT",
                   i_ptask->taskflags.task_type );
        assert( 0 );
        break;

    } //  endswitch

    return  l_errl;
}


void InitService::init( void *io_ptr )
{
    // Detach this task from the parent
    task_detach();

    errlHndl_t      l_errl              =   NULL;
    uint64_t        l_task              =   0;
    const TaskInfo  *l_ptask            =   NULL;
    //  init shutdown status to good.
    uint64_t        l_shutdownStatus    =   SHUTDOWN_STATUS_GOOD;

    printk( "InitService entry.\n" );

    TRACFCOMP( g_trac_initsvc,
               "Initialization Service is starting, io_ptr=%p.", io_ptr );

    //  loop through the task list and start up any tasks necessary
    for ( l_task=0;
          l_task <  ( sizeof(g_taskinfolist)/sizeof(TaskInfo) ) ;
          l_task++ )
    {
        //  make a local copy of the base image task
        l_ptask = &(g_taskinfolist[ l_task]);
        if ( l_ptask->taskflags.task_type == END_TASK_LIST )
        {
            TRACDCOMP( g_trac_initsvc,
                    "End of Initialization Service task list." );
            break;
        }

        //  dispatch the task and return good or errorlog
        l_errl  =   dispatchTask( l_ptask ,
                                  NULL );

        //  process errorlogs returned from the task that was launched
        if ( l_errl )
        {
            TRACFCOMP( g_trac_initsvc,
                       "ERROR: dispatching task, errorlog=0x%p",
                       l_errl );
            //  drop out with the error
            break;
        }

    } // endfor

    //  die if we drop out with an error
    if ( l_errl )
    {

        //  commit the log first, then shutdown.
        TRACFCOMP( g_trac_initsvc,
                "InitService: Committing errorlog %p",
                l_errl );

        // Catch this particular error type so it can be returned
        //   as a reasoncode the FSP can catch instead of a plid
        if (l_errl->reasonCode() == HWAS::RC_SYSAVAIL_INSUFFICIENT_HW)
        {
            TRACFCOMP( g_trac_initsvc,
                "InitService: Returning reasoncode 0x%08x to the FSP",
                HWAS::RC_SYSAVAIL_INSUFFICIENT_HW);
            l_shutdownStatus = HWAS::RC_SYSAVAIL_INSUFFICIENT_HW;
        }
        else
        {
            // Set the shutdown status to be the plid to force a TI
            l_shutdownStatus = l_errl->plid();
        }

        errlCommit( l_errl, INITSVC_COMP_ID );

    }

    //  =====================================================================
    //  -----   Shutdown all CPUs   -----------------------------------------
    //  =====================================================================

    TRACFCOMP( g_trac_initsvc,
            "InitService finished, shutdown = 0x%x.",
            l_shutdownStatus );

    //  Tell kernel to perform shutdown sequence
    INITSERVICE::doShutdown( l_shutdownStatus );

    // return to _start() to exit the task.
}


InitService& InitService::getTheInstance( )
{
    return Singleton<InitService>::instance();
}


InitService::InitService( ) :
    iv_shutdownInProgress(false)
{
    mutex_init(&iv_registryMutex);
}


InitService::~InitService( )
{ }

void registerBlock(void* i_vaddr, uint64_t i_size, BlockPriority i_priority)
{
    Singleton<InitService>::instance().registerBlock(i_vaddr,i_size,i_priority);
}

void InitService::registerBlock(void* i_vaddr, uint64_t i_size,
                                BlockPriority i_priority)
{
    mutex_lock(&iv_registryMutex);

    if (!iv_shutdownInProgress)
    {

        //Order priority from largest to smallest upon inserting
        std::vector<regBlock_t*>::iterator regBlock_iter = iv_regBlock.begin();
        for (; regBlock_iter!=iv_regBlock.end(); ++regBlock_iter)
        {
            if ((uint64_t)i_priority >= (*regBlock_iter)->priority)
            {
                iv_regBlock.insert(regBlock_iter,
                        new regBlock_t(i_vaddr,i_size,
                            (uint64_t)i_priority));
                regBlock_iter=iv_regBlock.begin();
                break;
            }
        }
        if (regBlock_iter == iv_regBlock.end())
        {
            iv_regBlock.push_back(new regBlock_t(i_vaddr,i_size,
                        (uint64_t)i_priority));
        }
    }

    mutex_unlock(&iv_registryMutex);
}


void doShutdown(uint64_t i_status,
                bool i_inBackground,
                uint64_t i_payload_base,
                uint64_t i_payload_entry,
                uint64_t i_payload_data,
                uint64_t i_masterHBInstance)
{
    class ShutdownExecute
    {
        public:
            ShutdownExecute(uint64_t i_status,
                            uint64_t i_payload_base,
                            uint64_t i_payload_entry,
                            uint64_t i_payload_data,
                            uint64_t i_masterHBInstance)
                : status(i_status),
                  payload_base(i_payload_base),
                  payload_entry(i_payload_entry),
                  payload_data(i_payload_data),
                  masterHBInstance(i_masterHBInstance)
            { }

            void execute()
            {
                Singleton<InitService>::instance().doShutdown(status,
                                                              payload_base,
                                                              payload_entry,
                                                              payload_data,
                                                              masterHBInstance);
            }
            void startThread()
            {
                task_create(ShutdownExecute::run, this);
            }

        private:
            uint64_t status;
            uint64_t payload_base;
            uint64_t payload_entry;
            uint64_t payload_data;
            uint64_t masterHBInstance;

            static void* run(void* _self)
            {
                task_detach();

                ShutdownExecute* self =
                    reinterpret_cast<ShutdownExecute*>(_self);

                self->execute();

                return NULL;
            }
    };

    ShutdownExecute* s = new ShutdownExecute(i_status, i_payload_base,
                                             i_payload_entry, i_payload_data,
                                             i_masterHBInstance);

    if (i_inBackground)
    {
        s->startThread();
    }
    else
    {
        s->execute();
        while(1) nanosleep(1,0);
    }
}

void InitService::doShutdown(uint64_t i_status,
                             uint64_t i_payload_base,
                             uint64_t i_payload_entry,
                             uint64_t i_payload_data,
                             uint64_t i_masterHBInstance)
{
    int l_rc = 0;
    errlHndl_t l_err = NULL;
    static volatile uint64_t worst_status = 0;

    TRACFCOMP(g_trac_initsvc, "doShutdown(i_status=%.16X)",i_status);

    // Hostboot PLIDs always start with 0x9 (32-bit)
    static const uint64_t PLID_MASK = 0x0000000090000000;

    // Ensure no one is manpulating the registry lists and that only one
    // thread actually executes the shutdown path.
    mutex_lock(&iv_registryMutex);
    if (iv_shutdownInProgress)
    {
        // switch the failing status if an RC comes in after
        //  a plid fail because we'd rather have the RC
        if( ((worst_status & 0x00000000F0000000) == PLID_MASK)
            && ((i_status & 0x00000000F0000000) != PLID_MASK)
            && (i_status > SHUTDOWN_STATUS_GOOD) )
        {
            worst_status = i_status;
        }
        mutex_unlock(&iv_registryMutex);
        return;
    }
    worst_status = i_status; //first thread in is the default
    iv_shutdownInProgress = true;
    mutex_unlock(&iv_registryMutex);

    TRACFCOMP(g_trac_initsvc, "doShutdown> status=%.16X",worst_status);

    // Call registered services and notify of shutdown
    msg_t * l_msg = msg_allocate();
    l_msg->data[1] = 0;
    l_msg->extra_data = 0;

    for(EventRegistry_t::iterator i = iv_regMsgQ.begin();
        i != iv_regMsgQ.end();
        ++i)
    {
        l_msg->type = i->msgType;
        l_msg->data[0] = worst_status;
        msg_sendrecv(i->msgQ,l_msg);
    }

    msg_free(l_msg);

    std::vector<regBlock_t*>::iterator l_rb_iter = iv_regBlock.begin();
    //FLUSH each registered block in order
    while (l_rb_iter!=iv_regBlock.end())
    {
        l_rc = mm_remove_pages(FLUSH,(*l_rb_iter)->vaddr,(*l_rb_iter)->size);
        if (l_rc)
        {
            TRACFCOMP(g_trac_initsvc, "ERROR: flushing virtual memory");
            /*
             * @errorlog tag
             * @errortype       ERRL_SEV_CRITICAL_SYS_TERM
             * @moduleid        BASE_INITSVC_MOD_ID
             * @reasoncode      SHUTDOWN_FLUSH_FAILED
             * @userdata1       returncode from mm_remove_pages()
             * @userdata2       0
             *
             * @defdesc         Could not FLUSH virtual memory.
             *
             */
            const bool hbSwError = true;
            l_err = new ERRORLOG::ErrlEntry(
                        ERRORLOG::ERRL_SEV_CRITICAL_SYS_TERM,
                        INITSERVICE::BASE_INITSVC_MOD_ID,
                        INITSERVICE::SHUTDOWN_FLUSH_FAILED,
                        l_rc, 0, hbSwError);
            //Commit and attempt flushing other registered blocks
            errlCommit( l_err, INITSVC_COMP_ID );
            l_err = NULL;
        }
        l_rb_iter++;
    }

    // Wait a little bit to give other tasks a chance to call shutdown
    // before moving on in case we want to modify the status we are
    // failing with.  This is after shutting down all daemons and flushing
    // out memory to make it most likely for the important work to have
    // a chance to log a better RC (e.g. pnor errors).
    task_yield();
    nanosleep(0,TEN_CTX_SWITCHES_NS);
    TRACFCOMP(g_trac_initsvc, "doShutdown> Final status=%.16X",worst_status);

    shutdown(worst_status,
             i_payload_base,
             i_payload_entry,
             i_payload_data,
             i_masterHBInstance);
}

bool InitService::registerShutdownEvent(msg_q_t i_msgQ,
                                        uint32_t i_msgType,
                                        EventPriority_t i_priority)
{
    bool result = true;

    mutex_lock(&iv_registryMutex);

    if (!iv_shutdownInProgress)
    {

        EventRegistry_t::iterator in_pos = iv_regMsgQ.end();

        for(EventRegistry_t::iterator r = iv_regMsgQ.begin();
                r != iv_regMsgQ.end();
                ++r)
        {
            if(r->msgQ == i_msgQ)
            {
                result = false;
                break;
            }

            if(r->msgPriority <= (uint32_t)i_priority)
            {
                in_pos = r;
                break;
            }
        }

        if(result)
        {
            in_pos = iv_regMsgQ.insert(in_pos,
                            regMsgQ_t(i_msgQ, i_msgType, i_priority));
        }
    }

    mutex_unlock(&iv_registryMutex);

    return result;
}

bool InitService::unregisterShutdownEvent(msg_q_t i_msgQ)
{
    bool result = false;

    mutex_lock(&iv_registryMutex);

    if (!iv_shutdownInProgress)
    {

        for(EventRegistry_t::iterator r = iv_regMsgQ.begin();
                r != iv_regMsgQ.end();
                ++r)
        {
            if(r->msgQ == i_msgQ)
            {
                result = true;
                iv_regMsgQ.erase(r);
                break;
            }
        }
    }

    mutex_unlock(&iv_registryMutex);

    return result;
}

/**
 * @see src/include/usr/initservice/initservicif.H
 */
bool registerShutdownEvent(msg_q_t i_msgQ,
                           uint32_t i_msgType,
                           EventPriority_t i_priority)
{
    return
    Singleton<InitService>::instance().registerShutdownEvent(i_msgQ,
                                                             i_msgType,
                                                             i_priority);
}

bool unregisterShutdownEvent(msg_q_t i_msgQ)
{
    return Singleton<InitService>::instance().unregisterShutdownEvent(i_msgQ);
}

} // namespace
