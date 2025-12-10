# PSX - Shell-Based Process Management Utility

A comprehensive operating system project implementing a custom shell command `psx` for listing and managing processes. This project demonstrates various OS concepts including threads, shared memory, message queues, semaphores, memory management, I/O operations, file handling, and process supervision.

## Features

### Core Components

1. **Custom Shell Command (`psx`)** - Command-line interface for process management
2. **Thread-Based Process Reading** - Multiple threads concurrently collect process information from `/proc`
3. **Shared Memory** - Caches the current process table for fast access
4. **Message Queues** - IPC mechanism for sending control commands (kill, suspend, resume)
5. **Memory Allocator** - Custom memory management for process info buffers
6. **I/O Operations** - Fetches CPU and memory usage statistics from `/proc`
7. **File Logging** - Stores historical resource usage logs
8. **Semaphores** - Protects process table access with mutual exclusion
9. **Dynamic Scheduler** - Assigns update frequency based on process priority
10. **Zombie Supervisor** - Ensures zombie process cleanup

## Project Structure

```
.
├── common.h              # Common definitions and structures
├── process_table.h/c     # Shared memory and semaphore management
├── message_queue.h/c     # Message queue IPC implementation
├── memory_allocator.h/c  # Custom memory allocator
├── proc_reader.h/c       # Thread-based /proc reading
├── stats.h/c             # CPU and memory statistics
├── logger.h/c            # File logging system
├── scheduler.h/c         # Dynamic update frequency scheduler
├── supervisor.h/c        # Zombie process cleanup
├── psx.c                 # Main shell command implementation
├── Makefile              # Build configuration
└── README.md             # This file
```

## Building

### Prerequisites

- Linux operating system (uses `/proc` filesystem and System V IPC)
- GCC compiler with pthread support
- Make utility

### Compilation

```bash
# Build the project
make

# Build with debug symbols
make debug

# Build optimized release version
make release

# Clean build artifacts
make clean
```

## Installation

```bash
# Install to /usr/local/bin
sudo make install

# Uninstall
sudo make uninstall
```

## Usage

### Starting the Daemon

The daemon mode runs background services for process monitoring:

```bash
./psx -d
```

Or simply:

```bash
./psx
```

### Commands

#### List Processes

```bash
# List all active processes
./psx list

# List all processes including zombies
./psx list -a
```

#### Show Process Details

```bash
./psx show <PID>
```

Example:
```bash
./psx show 1234
```

#### Process Management

```bash
# Kill a process (default: SIGTERM)
./psx kill <PID>

# Kill with specific signal
./psx kill <PID> <SIGNAL>

# Suspend a process (SIGSTOP)
./psx suspend <PID>

# Resume a process (SIGCONT)
./psx resume <PID>
```

#### Update Process Table

```bash
./psx update
```

#### Show System Statistics

```bash
./psx stats
```

## Architecture

### Threads

- **Process Reader Threads**: Multiple threads scan `/proc` directory concurrently to collect process information
- **Scheduler Thread**: Dynamically adjusts update frequency based on process CPU usage
- **Supervisor Thread**: Monitors and cleans up zombie processes
- **Command Server Thread**: Handles incoming control commands via message queue

### Shared Memory

The process table is stored in shared memory (System V IPC), allowing multiple processes to access it. Semaphores provide mutual exclusion for thread-safe operations.

### Message Queues

Control commands are sent via System V message queues:
- `MSG_KILL`: Terminate a process
- `MSG_SUSPEND`: Suspend a process
- `MSG_RESUME`: Resume a process
- `MSG_UPDATE`: Update the process table
- `MSG_SHUTDOWN`: Shutdown the daemon

### Memory Allocator

Custom memory allocator with:
- Fixed-size pool (10MB)
- First-fit allocation algorithm
- Block coalescing on free
- Statistics tracking

### Scheduler

The scheduler assigns update priorities based on CPU usage:
- **High Priority** (CPU > 50%): Update every 1 second
- **Medium Priority** (CPU > 10%): Update every 3 seconds
- **Low Priority** (CPU ≤ 10%): Update every 5 seconds

### File Logging

Two log files are created:
- `psx_log.txt`: General operations and events
- `psx_stats.log`: Historical resource usage statistics

## Technical Details

### Process Information Sources

- `/proc/<pid>/stat`: Process statistics (CPU times, memory, state)
- `/proc/<pid>/status`: Process status information (name, state)
- `/proc/<pid>/cmdline`: Command-line arguments
- `/proc/stat`: System-wide CPU statistics
- `/proc/uptime`: System uptime

### IPC Mechanisms

1. **Shared Memory**: Process table cache (key: 0x12345)
2. **Message Queues**: Command communication (key: 0x54321)
3. **Semaphores**: Mutual exclusion (key: 0xABCDE)

### Signal Handling

- `SIGTERM`: Default kill signal
- `SIGSTOP`: Suspend process
- `SIGCONT`: Resume process

## Limitations

- Linux-specific (uses `/proc` filesystem)
- Maximum 4096 processes in table
- Requires root privileges for some operations
- Memory pool limited to 10MB

## Cleanup

If the program terminates abnormally, you may need to clean up IPC resources:

```bash
# Find and remove shared memory segments
ipcs -m
ipcrm -m <shmid>

# Find and remove message queues
ipcs -q
ipcrm -q <msgid>

# Find and remove semaphores
ipcs -s
ipcrm -s <semid>
```

Or use the provided cleanup:
```bash
make clean
```

## License

This project is created for educational purposes as part of an operating systems course.

## Author

Operating Systems Project - Process Management Utility

