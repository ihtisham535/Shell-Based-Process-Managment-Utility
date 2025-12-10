# Running PSX on Kali Linux

## Prerequisites

Kali Linux comes with all the necessary tools pre-installed, but let's verify:

```bash
# Check if GCC is installed
gcc --version

# Check if make is installed
make --version

# If not installed, install them:
sudo apt update
sudo apt install build-essential -y
```

## Building the Project

1. **Navigate to the project directory:**
   ```bash
   cd "/path/to/os project"
   ```

2. **Compile the project:**
   ```bash
   make
   ```

   If successful, you should see the `psx` executable created.

3. **Alternative: Build with debug symbols:**
   ```bash
   make debug
   ```

4. **Alternative: Build optimized release:**
   ```bash
   make release
   ```

## Running PSX

### Method 1: Start as Daemon (Background Process)

```bash
# Start the daemon
./psx -d

# Or simply (defaults to daemon mode if no arguments)
./psx
```

The daemon will run in the background, collecting process information and handling commands.

### Method 2: Interactive Commands

Run commands directly without starting the daemon:

```bash
# List all processes
./psx list

# List all processes including zombies
./psx list -a

# Show details of a specific process
./psx show 1234

# Kill a process
./psx kill 1234

# Suspend a process
./psx suspend 1234

# Resume a process
./psx resume 1234

# Update process table
./psx update

# Show system statistics
./psx stats
```

## Complete Example Session

```bash
# 1. Build the project
cd "/path/to/os project"
make

# 2. Start the daemon (in one terminal)
./psx -d

# 3. In another terminal, use psx commands
cd "/path/to/os project"

# List processes
./psx list

# Show details of init process
./psx show 1

# Check system stats
./psx stats

# Kill a process (if you have permission)
./psx kill <PID>

# 4. Check logs
cat psx_log.txt
cat psx_stats.log

# 5. Stop the daemon
pkill psx
# Or find and kill it
ps aux | grep psx
kill <PID>
```

## Installation (Optional)

To install PSX system-wide:

```bash
# Install to /usr/local/bin
sudo make install

# Now you can run psx from anywhere
psx list
psx show 1

# Uninstall
sudo make uninstall
```

## Troubleshooting

### Permission Errors

Some operations require root privileges:

```bash
# Run specific commands with sudo
sudo ./psx kill <PID>

# Or run the entire daemon as root (not recommended)
sudo ./psx -d
```

### IPC Resources Already Exist

If you get errors about existing shared memory or message queues:

```bash
# List all IPC resources
ipcs -a

# Remove shared memory
ipcs -m  # List shared memory
ipcrm -m <shmid>  # Remove by ID

# Remove message queues
ipcs -q  # List message queues
ipcrm -q <msgid>  # Remove by ID

# Remove semaphores
ipcs -s  # List semaphores
ipcrm -s <semid>  # Remove by ID

# Or clean everything
make clean
# Then remove remaining IPC resources manually if needed
```

### Process Not Found

If a process doesn't appear in the list:

```bash
# Update the process table manually
./psx update

# Check if the process exists
ps aux | grep <process_name>
```

### Compilation Errors

If you encounter compilation errors:

```bash
# Make sure you have all dependencies
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r) -y

# Clean and rebuild
make clean
make
```

### Check System Compatibility

Verify your system supports the required features:

```bash
# Check /proc filesystem
ls /proc

# Check IPC support
ipcs -l

# Check pthread support
gcc -pthread --version
```

## Running as a Service (Advanced)

To run PSX as a systemd service:

1. **Create service file:**
   ```bash
   sudo nano /etc/systemd/system/psx.service
   ```

2. **Add the following content:**
   ```ini
   [Unit]
   Description=PSX Process Management Utility
   After=network.target

   [Service]
   Type=simple
   User=root
   WorkingDirectory=/path/to/os project
   ExecStart=/path/to/os project/psx -d
   Restart=always
   RestartSec=10

   [Install]
   WantedBy=multi-user.target
   ```

3. **Enable and start the service:**
   ```bash
   sudo systemctl daemon-reload
   sudo systemctl enable psx
   sudo systemctl start psx
   ```

4. **Check status:**
   ```bash
   sudo systemctl status psx
   ```

5. **View logs:**
   ```bash
   journalctl -u psx -f
   ```

## Monitoring

### View Real-time Logs

```bash
# Watch log file
tail -f psx_log.txt

# Watch stats file
tail -f psx_stats.log
```

### Check Daemon Status

```bash
# Check if psx is running
ps aux | grep psx

# Check system resources
./psx stats
```

## Security Notes

- Running as root gives full access to all processes
- Be careful when killing processes - some are critical system processes
- The daemon creates IPC resources that persist until cleaned up
- Log files may contain sensitive process information

## Project Structure on Kali Linux

```
os project/
├── psx                    # Compiled executable
├── psx.c                  # Main source file
├── *.c                    # Other source files
├── *.h                    # Header files
├── Makefile               # Build configuration
├── README.md              # Documentation
├── QUICKSTART.md          # Quick start guide
├── KALI_LINUX_GUIDE.md    # This file
├── psx_log.txt            # Log file (created at runtime)
└── psx_stats.log          # Stats file (created at runtime)
```

## Quick Reference

```bash
# Build
make

# Run daemon
./psx -d

# List processes
./psx list

# Show process details
./psx show <PID>

# Kill process
./psx kill <PID>

# Clean build
make clean

# Install
sudo make install
```

Enjoy using PSX on Kali Linux! 🐉

