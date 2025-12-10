# Quick Start Guide

## Building the Project

1. Open a terminal in the project directory
2. Compile the project:
   ```bash
   make
   ```

## Running PSX

### Starting the Daemon

Start the background daemon to enable process monitoring:

```bash
./psx -d
```

Or simply:
```bash
./psx
```

The daemon will:
- Start multiple threads to collect process information
- Initialize shared memory for the process table
- Start the scheduler for dynamic updates
- Start the zombie cleanup supervisor
- Begin logging to `psx_log.txt` and `psx_stats.log`

### Basic Commands

**List all processes:**
```bash
./psx list
```

**List processes including zombies:**
```bash
./psx list -a
```

**Show details of a specific process:**
```bash
./psx show 1234
```

**Kill a process:**
```bash
./psx kill 1234
```

**Kill with specific signal:**
```bash
./psx kill 1234 9  # SIGKILL
```

**Suspend a process:**
```bash
./psx suspend 1234
```

**Resume a process:**
```bash
./psx resume 1234
```

**Update process table:**
```bash
./psx update
```

**Show system statistics:**
```bash
./psx stats
```

## Example Session

```bash
# Build the project
make

# Start the daemon (in background)
./psx -d &

# List processes
./psx list

# Show details of process 1
./psx show 1

# Check statistics
./psx stats

# Stop the daemon (find PID first)
ps aux | grep psx
kill <psx_pid>
```

## Troubleshooting

### Permission Errors

Some operations may require root privileges:
```bash
sudo ./psx kill <pid>
```

### IPC Resources Not Cleared

If you get errors about existing shared memory or message queues:

```bash
# List IPC resources
ipcs -a

# Remove shared memory
ipcrm -m <shmid>

# Remove message queues  
ipcrm -q <msgid>

# Remove semaphores
ipcrm -s <semid>
```

Or simply:
```bash
make clean
```

### Check Logs

View operation logs:
```bash
cat psx_log.txt
```

View statistics logs:
```bash
cat psx_stats.log
```

## Architecture Overview

- **Threads**: 4 reader threads scan `/proc` concurrently
- **Shared Memory**: Process table cached for fast access
- **Semaphores**: Protect shared memory access
- **Message Queues**: IPC for control commands
- **Scheduler**: Updates high-CPU processes more frequently
- **Supervisor**: Automatically cleans up zombie processes

## Next Steps

- Read the full README.md for detailed documentation
- Explore the source code to understand implementation
- Modify parameters in `common.h` to customize behavior

