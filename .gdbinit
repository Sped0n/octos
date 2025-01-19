# Load the ELF file
file ./build/octos.elf

# Connect to OpenOCD's GDB server
target extended-remote :3333

# Set up the source code layout
layout src

# Move focus to command window
focus cmd

# Set breakpoint at assert
break OCTOS_ASSERT_CALLED

# Optional: Some useful settings
set print pretty on
set confirm off
