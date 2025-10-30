/**
 * @file State.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Model-Specific Registers definitions
 *
 * @version 0.2
 * @date 2022-12-01
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				      typedefs         			 //
//////////////////////////////////////////////////

typedef EPT_PML4E     EPT_PML4_POINTER, *PEPT_PML4_POINTER;
typedef EPT_PDPTE     EPT_PML3_POINTER, *PEPT_PML3_POINTER;
typedef EPT_PDPTE_1GB EPT_PML3_ENTRY, *PEPT_PML3_ENTRY;
typedef EPT_PDE_2MB   EPT_PML2_ENTRY, *PEPT_PML2_ENTRY;
typedef EPT_PDE       EPT_PML2_POINTER, *PEPT_PML2_POINTER;
typedef EPT_PTE       EPT_PML1_ENTRY, *PEPT_PML1_ENTRY;

//////////////////////////////////////////////////
//				    Constants					//
//////////////////////////////////////////////////

/**
 * @brief Pending External Interrupts Buffer Capacity
 *
 */
#define PENDING_INTERRUPTS_BUFFER_CAPACITY 64

/**
 * @brief Maximum number of hidden breakpoints in a page
 *
 */
#define MaximumHiddenBreakpointsOnPage 40

//////////////////////////////////////////////////
//					  Enums		    			//
//////////////////////////////////////////////////

/**
 * @brief Types of actions for NMI broadcasting
 *
 */
typedef enum _NMI_BROADCAST_ACTION_TYPE
{
    NMI_BROADCAST_ACTION_NONE = 0,
    NMI_BROADCAST_ACTION_TEST,
    NMI_BROADCAST_ACTION_REQUEST,
    NMI_BROADCAST_ACTION_INVALIDATE_EPT_CACHE_SINGLE_CONTEXT,
    NMI_BROADCAST_ACTION_INVALIDATE_EPT_CACHE_ALL_CONTEXTS,

} NMI_BROADCAST_ACTION_TYPE;

/**
 * @brief Types of last violation happened to EPT hook
 *
 */
typedef enum _EPT_HOOKED_LAST_VIOLATION
{
    EPT_HOOKED_LAST_VIOLATION_READ  = 1,
    EPT_HOOKED_LAST_VIOLATION_WRITE = 2,
    EPT_HOOKED_LAST_VIOLATION_EXEC  = 3

} EPT_HOOKED_LAST_VIOLATION;

//////////////////////////////////////////////////
//			      Memory Layout      			//
//////////////////////////////////////////////////

/**
 * @brief The number of 512GB PML4 entries in the page table
 *
 */
#define VMM_EPT_PML4E_COUNT 512

/**
 * @brief The number of 1GB PDPT entries in the page table per 512GB PML4 entry
 *
 */
#define VMM_EPT_PML3E_COUNT 512

/**
 * @brief Then number of 2MB Page Directory entries in the page table per 1GB
 *  PML3 entry
 *
 */
#define VMM_EPT_PML2E_COUNT 512

/**
 * @brief Then number of 4096 byte Page Table entries in the page table per 2MB PML2
 * entry when dynamically split
 *
 */
#define VMM_EPT_PML1E_COUNT 512

/**
 * @brief Structure for saving EPT Table
 *
 */
typedef struct _VMM_EPT_PAGE_TABLE
{
    /**
     * @brief 28.2.2 Describes 512 contiguous 512GB memory regions each with 512 1GB regions.
     */
    DECLSPEC_ALIGN(PAGE_SIZE)
    EPT_PML4_POINTER PML4[VMM_EPT_PML4E_COUNT];

    /**
     * @brief Describes exactly 512 contiguous 1GB memory regions within a our singular 512GB PML4 region
     * (This entry is used to support the entire address space).
     * @details The reason why there is a minus one here is that the original PML3 is described in the next field.
     */
    DECLSPEC_ALIGN(PAGE_SIZE)
    EPT_PML3_ENTRY PML3_RSVD[VMM_EPT_PML4E_COUNT - 1][VMM_EPT_PML3E_COUNT];

    /**
     * @brief Describes exactly 512 contiguous 1GB memory regions within a our singular 512GB PML4 region.
     */
    DECLSPEC_ALIGN(PAGE_SIZE)
    EPT_PML3_POINTER PML3[VMM_EPT_PML3E_COUNT];

    /**
     * @brief For each 1GB PML3 entry, create 512 2MB entries to map identity.
     * NOTE: We are using 2MB pages as the smallest paging size in our map, so we do not manage individual 4096 byte pages.
     * Therefore, we do not allocate any PML1 (4096 byte) paging structures.
     */
    DECLSPEC_ALIGN(PAGE_SIZE)
    EPT_PML2_ENTRY PML2[VMM_EPT_PML3E_COUNT][VMM_EPT_PML2E_COUNT];

} VMM_EPT_PAGE_TABLE, *PVMM_EPT_PAGE_TABLE;

//////////////////////////////////////////////////
//					  Structure	    			//
//////////////////////////////////////////////////

/**
 * @brief The status of transparency of each core after and before VMX
 *
 */
typedef struct _VM_EXIT_TRANSPARENCY
{
    UINT64 PreviousTimeStampCounter;

    HANDLE  ThreadId;
    UINT64  RevealedTimeStampCounterByRdtsc;
    BOOLEAN CpuidAfterRdtscDetected;

} VM_EXIT_TRANSPARENCY, *PVM_EXIT_TRANSPARENCY;

/**
 * @brief Save the state of core in the case of VMXOFF
 *
 */
typedef struct _VMX_VMXOFF_STATE
{
    BOOLEAN IsVmxoffExecuted; // Shows whether the VMXOFF executed or not
    UINT64  GuestRip;         // Rip address of guest to return
    UINT64  GuestRsp;         // Rsp address of guest to return

} VMX_VMXOFF_STATE, *PVMX_VMXOFF_STATE;

/**
 * @brief Structure to save the state of each hooked pages
 *
 */
typedef struct _EPT_HOOKED_PAGE_DETAIL
{
    DECLSPEC_ALIGN(PAGE_SIZE)
    CHAR FakePageContents[PAGE_SIZE];

    /**
     * @brief Linked list entries for each page hook.
     */
    LIST_ENTRY PageHookList;

    /**
     * @brief The virtual address from the caller perspective view (cr3)
     */
    UINT64 VirtualAddress;

    /**
     * @brief The virtual address of it's entry on g_EptHook2sDetourListHead
     * this way we can de-allocate the list whenever the hook is finished
     */
    UINT64 AddressOfEptHook2sDetourListEntry;

    /**
     * @brief The base address of the page. Used to find this structure in the list of page hooks
     * when a hook is hit.
     */
    SIZE_T PhysicalBaseAddress;

    /**
     * @brief Start address of the target physical address.
     */
    SIZE_T StartOfTargetPhysicalAddress;

    /**
     * @brief End address of the target physical address.
     */
    SIZE_T EndOfTargetPhysicalAddress;

    /**
     * @brief Tag used for notifying the caller.
     */
    UINT64 HookingTag;

    /**
     * @brief The base address of the page with fake contents. Used to swap page with fake contents
     * when a hook is hit.
     */
    SIZE_T PhysicalBaseAddressOfFakePageContents;

    /**
     * @brief The original page entry. Will be copied back when the hook is removed
     * from the page.
     */
    EPT_PML1_ENTRY OriginalEntry;

    /**
     * @brief The original page entry. Will be copied back when the hook is remove from the page.
     */
    EPT_PML1_ENTRY ChangedEntry;

    /**
     * @brief The buffer of the trampoline function which is used in the inline hook.
     */
    PCHAR Trampoline;

    /**
     * @brief This field shows whether the hook contains a hidden hook for execution or not
     */
    BOOLEAN IsExecutionHook;

    /**
     * @brief If TRUE shows that this is the information about
     * a hidden breakpoint command (not a monitor or hidden detours)
     */
    BOOLEAN IsHiddenBreakpoint;

    /**
     * @brief This field shows whether the hook is for MMIO shadowing or not
     */
    BOOLEAN IsMmioShadowing;

    /**
     * @brief Temporary context for the post event monitors
     * It shows the context of the last address that triggered the hook
     * Note: Only used for read/write trigger events
     */
    EPT_HOOKS_CONTEXT LastContextState;

    /**
     * @brief This field shows whether the hook should call the post event trigger
     * after restoring the state or not
     */
    BOOLEAN IsPostEventTriggerAllowed;

    /**
     * @brief This field shows the last violation happened to this EPT hook
     */
    EPT_HOOKED_LAST_VIOLATION LastViolation;

    /**
     * @brief Address of hooked pages (multiple breakpoints on a single page)
     * this is only used in hidden breakpoints (not hidden detours)
     */
    UINT64 BreakpointAddresses[MaximumHiddenBreakpointsOnPage];

    /**
     * @brief Character that was previously used in BreakpointAddresses
     * this is only used in hidden breakpoints (not hidden detours)
     */
    CHAR PreviousBytesOnBreakpointAddresses[MaximumHiddenBreakpointsOnPage];

    /**
     * @brief Count of breakpoints (multiple breakpoints on a single page)
     * this is only used in hidden breakpoints (not hidden detours)
     */
    UINT64 CountOfBreakpoints;

    /**
     * @brief Flag to indicate if the hook target is in kernel space (R0) or user space (R3)
     */
    BOOLEAN IsKernelAddress;

    /**
     * @brief Process ID where the R3 trampoline was allocated (only for R3 hooks)
     */
    HANDLE ProcessId;

} EPT_HOOKED_PAGE_DETAIL, *PEPT_HOOKED_PAGE_DETAIL;

/**
 * @brief The status of NMI broadcasting in VMX
 *
 */
typedef struct _NMI_BROADCASTING_STATE
{
    volatile NMI_BROADCAST_ACTION_TYPE NmiBroadcastAction; // The broadcast action for NMI

} NMI_BROADCASTING_STATE, *PNMI_BROADCASTING_STATE;

/**
 * @brief The status of each core after and before VMX
 *
 */
typedef struct _VIRTUAL_MACHINE_STATE
{
    BOOLEAN          IsOnVmxRootMode;                                               // Detects whether the current logical core is on Executing on VMX Root Mode
    BOOLEAN          IncrementRip;                                                  // Checks whether it has to redo the previous instruction or not (it used mainly in Ept routines)
    BOOLEAN          HasLaunched;                                                   // Indicate whether the core is virtualized or not
    BOOLEAN          IgnoreMtfUnset;                                                // Indicate whether the core should ignore unsetting the MTF or not
    BOOLEAN          WaitForImmediateVmexit;                                        // Whether the current core is waiting for an immediate vm-exit or not
    BOOLEAN          EnableExternalInterruptsOnContinue;                            // Whether to enable external interrupts on the continue  or not
    BOOLEAN          EnableExternalInterruptsOnContinueMtf;                         // Whether to enable external interrupts on the continue state of MTF or not
    BOOLEAN          RegisterBreakOnMtf;                                            // Registered Break in the case of MTFs (used in instrumentation step-in)
    BOOLEAN          IgnoreOneMtf;                                                  // Ignore (mark as handled) for one MTF
    BOOLEAN          NotNormalEptp;                                                 // Indicate that the target processor is on the normal EPTP or not
    BOOLEAN          MbecEnabled;                                                   // Indicate that the target processor is on MBEC-enabled mode or not
    PUINT64          PmlBufferAddress;                                              // Address of buffer used for dirty logging
    BOOLEAN          Test;                                                          // Used for test purposes
    UINT64           TestNumber;                                                    // Used for test purposes (Number)
    GUEST_REGS *     Regs;                                                          // The virtual processor's general-purpose registers
    GUEST_XMM_REGS * XmmRegs;                                                       // The virtual processor's XMM registers
    UINT32           CoreId;                                                        // The core's unique identifier
    UINT32           ExitReason;                                                    // The core's exit reason
    UINT32           ExitQualification;                                             // The core's exit qualification
    UINT64           LastVmexitRip;                                                 // RIP in the current VM-exit
    UINT64           VmxonRegionPhysicalAddress;                                    // Vmxon region physical address
    UINT64           VmxonRegionVirtualAddress;                                     // VMXON region virtual address
    UINT64           VmcsRegionPhysicalAddress;                                     // VMCS region physical address
    UINT64           VmcsRegionVirtualAddress;                                      // VMCS region virtual address
    UINT64           VmmStack;                                                      // Stack for VMM in VM-Exit State
    UINT64           MsrBitmapVirtualAddress;                                       // Msr Bitmap Virtual Address
    UINT64           MsrBitmapPhysicalAddress;                                      // Msr Bitmap Physical Address
    UINT64           IoBitmapVirtualAddressA;                                       // I/O Bitmap Virtual Address (A)
    UINT64           IoBitmapPhysicalAddressA;                                      // I/O Bitmap Physical Address (A)
    UINT64           IoBitmapVirtualAddressB;                                       // I/O Bitmap Virtual Address (B)
    UINT64           IoBitmapPhysicalAddressB;                                      // I/O Bitmap Physical Address (B)
    UINT32           QueuedNmi;                                                     // Queued NMIs
    UINT32           PendingExternalInterrupts[PENDING_INTERRUPTS_BUFFER_CAPACITY]; // This list holds a buffer for external-interrupts that are in pending state due to the external-interrupt
                                                                                    // blocking and waits for interrupt-window exiting
                                                                                    // From hvpp :
                                                                                    // Pending interrupt queue (FIFO).
                                                                                    // Make storage for up-to 64 pending interrupts.
                                                                                    // In practice I haven't seen more than 2 pending interrupts.
    VMX_VMXOFF_STATE        VmxoffState;                                            // Shows the vmxoff state of the guest
    NMI_BROADCASTING_STATE  NmiBroadcastingState;                                   // Shows the state of NMI broadcasting
    VM_EXIT_TRANSPARENCY    TransparencyState;                                      // The state of the debugger in transparent-mode
    PEPT_HOOKED_PAGE_DETAIL MtfEptHookRestorePoint;                                 // It shows the detail of the hooked paged that should be restore in MTF vm-exit
    UINT8                   LastExceptionOccurredInHost;                            // The vector of last exception occurred in host
    UINT64                  HostIdt;                                                // host Interrupt Descriptor Table (actual type is SEGMENT_DESCRIPTOR_INTERRUPT_GATE_64*)
    UINT64                  HostGdt;                                                // host Global Descriptor Table (actual type is SEGMENT_DESCRIPTOR_32* or SEGMENT_DESCRIPTOR_64*)
    UINT64                  HostTss;                                                // host Task State Segment (actual type is TASK_STATE_SEGMENT_64*)
    UINT64                  HostInterruptStack;                                     // host interrupt RSP

    //
    // EPT Descriptors
    //
    EPT_POINTER         EptPointer;   // Extended-Page-Table Pointer
    PVMM_EPT_PAGE_TABLE EptPageTable; // Details of core-specific page-table

} VIRTUAL_MACHINE_STATE, *PVIRTUAL_MACHINE_STATE;



// DEBUGER
/**
 * @file State.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Model-Specific Registers definitions
 *
 * @version 0.2
 * @date 2022-12-01
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

 //////////////////////////////////////////////////
 //				    Structures					//
 //////////////////////////////////////////////////

 /**
  * @brief Use to modify Msrs or read MSR values
  *
  */
typedef struct _PROCESSOR_DEBUGGING_MSR_READ_OR_WRITE
{
    UINT64 Msr;   // Msr (ecx)
    UINT64 Value; // the value to write on msr

} PROCESSOR_DEBUGGING_MSR_READ_OR_WRITE, * PPROCESSOR_DEBUGGING_MSR_READ_OR_WRITE;

/**
 * @brief Use to trace the execution in the case of instrumentation step-in
 * command (i command)
 *
 */
typedef struct _DEBUGGEE_INSTRUMENTATION_STEP_IN_TRACE
{
    UINT16 CsSel; // the cs value to trace the execution modes

} DEBUGGEE_INSTRUMENTATION_STEP_IN_TRACE, * PDEBUGGEE_INSTRUMENTATION_STEP_IN_TRACE;

/**
 * @brief Structure to save the state of adding trace for threads
 * and processes
 *
 */
typedef struct _DEBUGGEE_PROCESS_OR_THREAD_TRACING_DETAILS
{
    BOOLEAN InitialSetProcessChangeEvent;
    BOOLEAN InitialSetThreadChangeEvent;

    BOOLEAN InitialSetByClockInterrupt;

    //
    // For threads
    //
    UINT64  CurrentThreadLocationOnGs;
    BOOLEAN DebugRegisterInterceptionState;
    BOOLEAN InterceptClockInterruptsForThreadChange;

    //
    // For processes
    //
    BOOLEAN IsWatingForMovCr3VmExits;
    BOOLEAN InterceptClockInterruptsForProcessChange;

} DEBUGGEE_PROCESS_OR_THREAD_TRACING_DETAILS, * PDEBUGGEE_PROCESS_OR_THREAD_TRACING_DETAILS;

/**
 * @brief The structure of storing breakpoints
 *
 */
typedef struct _DEBUGGEE_BP_DESCRIPTOR
{
    UINT64     BreakpointId;
    LIST_ENTRY BreakpointsList;
    BOOLEAN    Enabled;
    UINT64     Address;
    UINT64     PhysAddress;
    UINT32     Pid;
    UINT32     Tid;
    UINT32     Core;
    UINT16     InstructionLength;
    BYTE       PreviousByte;
    BOOLEAN    AvoidReApplyBreakpoint;
    BOOLEAN    RemoveAfterHit;
    BOOLEAN    CheckForCallbacks;

} DEBUGGEE_BP_DESCRIPTOR, * PDEBUGGEE_BP_DESCRIPTOR;

/**
 * @brief The status of NMI in the kernel debugger
 *
 */
typedef struct _KD_NMI_STATE
{
    volatile BOOLEAN NmiCalledInVmxRootRelatedToHaltDebuggee;
    volatile BOOLEAN WaitingToBeLocked;

} KD_NMI_STATE, * PKD_NMI_STATE;

/**
 * @brief The thread/process information
 *
 */
typedef struct _DEBUGGER_PROCESS_THREAD_INFORMATION
{
    union
    {
        UINT64 asUInt;

        struct
        {
            UINT32 ProcessId;
            UINT32 ThreadId;
        } Fields;
    };

} DEBUGGER_PROCESS_THREAD_INFORMATION, * PDEBUGGER_PROCESS_THREAD_INFORMATION;

/**
 * @brief The status of RFLAGS.TF masking
 * @details Used for masking trap flags on threads
 *
 */
typedef struct _DEBUGGER_TRAP_FLAG_STATE
{
    UINT32                              NumberOfItems;
    DEBUGGER_PROCESS_THREAD_INFORMATION ThreadInformation[MAXIMUM_NUMBER_OF_THREAD_INFORMATION_FOR_TRAPS];

} DEBUGGER_TRAP_FLAG_STATE, * PDEBUGGER_TRAP_FLAG_STATE;

/**
 * @brief Details of setting tasks for the locked (halted) cores
 *
 */
typedef struct _DEBUGGEE_HALTED_CORE_TASK
{
    BOOLEAN PerformHaltedTask;
    BOOLEAN LockAgainAfterTask;
    UINT64  TargetTask;
    PVOID   Context;
    UINT64  KernelStatus;

} DEBUGGEE_HALTED_CORE_TASK, * PDEBUGGEE_HALTED_CORE_TASK;

/**
 * @brief Timer for the core
 *
 */
typedef struct _DATE_TIME_HOLDER
{
    TIME_FIELDS TimeFields;
    CHAR        TimeBuffer[14];
    CHAR        DateBuffer[12];

} DATE_TIME_HOLDER, * PDATE_TIME_HOLDER;

/**
 * @brief Saves the debugger state
 * @details Each logical processor contains one of this structure which describes about the
 * state of debuggers, flags, etc.
 *
 */
typedef struct _PROCESSOR_DEBUGGING_STATE
{
    volatile LONG                              Lock;
    volatile BOOLEAN                           MainDebuggingCore;
    GUEST_REGS* Regs;
    UINT32                                     CoreId;
    BOOLEAN                                    ShortCircuitingEvent;
    BOOLEAN                                    IgnoreDisasmInNextPacket;
    BOOLEAN                                    BreakStarterCore;
    BOOLEAN                                    Test; // Used for testing purposes
    BOOLEAN                                    DoNotNmiNotifyOtherCoresByThisCore;
    BOOLEAN                                    TracingMode; // Indicate that the target processor is on the tracing mode or not
    PROCESSOR_DEBUGGING_MSR_READ_OR_WRITE      MsrState;
    DATE_TIME_HOLDER                           DateTimeHolder;
    PDEBUGGEE_BP_DESCRIPTOR                    SoftwareBreakpointState;
    DEBUGGEE_INSTRUMENTATION_STEP_IN_TRACE     InstrumentationStepInTrace;
    DEBUGGEE_PROCESS_OR_THREAD_TRACING_DETAILS ThreadOrProcessTracingDetails;
    KD_NMI_STATE                               NmiState;
    DEBUGGEE_HALTED_CORE_TASK                  HaltedCoreTask;
    UINT16                                     InstructionLengthHint;
    UINT64                                     HardwareDebugRegisterForStepping;
    UINT64* ScriptEngineCoreSpecificStackBuffer;
    PKDPC                                      KdDpcObject;                       // DPC object to be used in kernel debugger
    CHAR                                       KdRecvBuffer[MaxSerialPacketSize]; // Used for debugging buffers (receiving buffers from serial devices)

} PROCESSOR_DEBUGGING_STATE, PPROCESSOR_DEBUGGING_STATE;
