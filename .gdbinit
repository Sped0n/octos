# Helpers
define oinfo
    set $tcb = (TCB_t*)current_tcb
    set $max_priorities = sizeof(ready_list)/sizeof(ready_list[0])
    
    printf "\n=== OCTOS Task Information ===\n"
    printf "Current TCB: "
    output (TCB_t*)$tcb
    printf "\n"
    if $tcb != 0
        printf "Current Task ID: %d\n", $tcb->TCBNumber
        printf "Current Priority: %d\n", $tcb->Priority
    end
    
    printf "\nScheduler Status:\n"
    printf "Scheduler Suspended: %d\n", scheduler_suspended
    printf "Yield Pending: %d\n", yield_pending
    printf "Number of Tasks: %d\n", current_number_of_tasks
    
    printf "\nTask Lists:\n"
    set $i = 0
    while $i < $max_priorities
        if ready_list[$i].Length > 0
            printf "Ready List[%d] length: %d\n", $i, ready_list[$i].Length
        end
        set $i = $i + 1
    end
    
    printf "Pending Ready List length: %d\n", pending_ready_list.Length
    printf "Delayed List length: %d\n", delayed_list->Length
    printf "Delayed Overflow List length: %d\n", delayed_list_overflow->Length
    printf "Suspended List length: %d\n", suspended_list.Length
    printf "Terminated List length: %d\n", terminated_list.Length
end

document oinfo
Displays OCTOS RTOS task information including:
- Current task details
- Scheduler status
- Task list lengths
end

define otasks
    set $tcb = (TCB_t*)current_tcb
    
    printf "\n=== OCTOS Task Details ===\n"
    printf "ID\tPriority\tStack Top\tState|Event\n"
    printf "-------------------------------------------------------------------------------\n"
    
    define _print_task_state
        if $arg0 == current_tcb
            printf "RUNNING\n"
            printf "\n"
        else
            output $arg0->StateListItem.Parent
            printf "\n\t\t\t\t\t"
            output $arg0->EventListItem.Parent
            printf "\n"
        end
    end

    define _print_list
        set $item = $arg0->End->Next
        set $j = 0
        while $j < $arg0->Length
            set $t = (TCB_t*)($item->Owner)
            printf "%d\t%d\t\t", $t->TCBNumber, $t->Priority
            printf "%p\t", $t->StackTop
            _print_task_state $t
            set $item = $item->Next
            set $j = $j + 1
        end
    end
    
    set $max_priorities = sizeof(ready_list)/sizeof(ready_list[0])
    
    set $i = 0
    while $i < $max_priorities
        set $item = ready_list[$i].End->Next
        set $j = 0
        while $j < ready_list[$i].Length
            set $t = (TCB_t*)($item->Owner)
            printf "%d\t%d\t\t", $t->TCBNumber, $t->Priority
            printf "%p\t", $t->StackTop
            _print_task_state $t
            set $item = $item->Next
            set $j = $j + 1
        end
        set $i = $i + 1
    end

    set $item = delayed_list->End->Next
    set $j = 0
    while $j < delayed_list->Length
        set $t = (TCB_t*)($item->Owner)
        printf "%d\t%d\t\t", $t->TCBNumber, $t->Priority
        printf "%p\t", $t->StackTop
        _print_task_state $t
        set $item = $item->Next
        set $j = $j + 1
    end

    set $item = delayed_list_overflow->End->Next
    set $j = 0
    while $j < delayed_list_overflow->Length
        set $t = (TCB_t*)($item->Owner)
        printf "%d\t%d\t\t", $t->TCBNumber, $t->Priority
        printf "%p\t", $t->StackTop
        _print_task_state $t
        set $item = $item->Next
        set $j = $j + 1
    end

    set $item = suspended_list.End->Next
    set $j = 0
    while $j < suspended_list.Length
        set $t = (TCB_t*)($item->Owner)
        printf "%d\t%d\t\t", $t->TCBNumber, $t->Priority
        printf "%p\t", $t->StackTop
        _print_task_state $t
        set $item = $item->Next
        set $j = $j + 1
    end
    
    dont-repeat
    delete _print_task_state
end

document otasks
Displays detailed information about all OCTOS tasks including:
- Task ID
- Priority
- Current state
- Stack pointer
end


# Load the ELF file
file ./build/octos.elf

# Connect to OpenOCD's GDB server
target extended-remote :3333

# Set up the layout
layout src
layout asm
layout next

# Move focus to command window
focus cmd

# Increase cmd winheight
winheight cmd +7

# Set breakpoint at assert
break OCTOS_ASSERT_CALLED

# Optional: Some useful settings
set print pretty on
set confirm off
