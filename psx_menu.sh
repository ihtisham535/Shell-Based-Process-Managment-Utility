#!/usr/bin/env bash

pause() {
  echo
  read -rp "Output finished. Press Enter to return to menu..." _
}

require_daemon() {
  if ! pgrep -x psx >/dev/null 2>&1; then
    echo
    echo "psx daemon is not running."
    echo "Start it in another terminal with: ./psx -d"
    pause
    return 1
  fi
  return 0
}

while true; do
  clear
  echo "╔══════════════════════════════╗"
  echo "║      PSX Control Center      ║"
  echo "╠══════════════════════════════╣"
  echo "║ 1) Check daemon status       ║"
  echo "║ 2) List processes            ║"
  echo "║ 3) Show details of a PID     ║"
  echo "║ 4) System stats              ║"
  echo "║ 5) Kill a PID                ║"
  echo "║ 6) Tail logs (Ctrl+C to exit)║"
  echo "║ 0) Exit                      ║"
  echo "╚══════════════════════════════╝"
  read -rp "Select an option: " choice
  case "$choice" in
    1)
      echo "Checking for running psx daemon..."
      ps aux | grep "[p]sx" | head -n 5 || echo "No psx process found."
      pause
      ;;
    2)
      require_daemon || continue
      echo "Updating process table..."
      ./psx update >/dev/null 2>&1
      echo "Listing processes..."
      ./psx list
      pause
      ;;
    3)
      require_daemon || continue
      read -rp "PID: " pid
      ./psx show "$pid"
      pause
      ;;
    4)
      require_daemon || continue
      ./psx stats
      pause
      ;;
    5)
      require_daemon || continue
      read -rp "PID to kill: " pid
      ./psx kill "$pid"
      pause
      ;;
    6)
      # logs may exist even if daemon is stopped, so no daemon check here
      echo "Showing last 30 lines of psx_log.txt..."
      echo "----------------------------------------"
      tail -n 30 psx_log.txt 2>/dev/null || echo "Log file not found yet."
      pause
      ;;
    0)
      clear
      exit 0
      ;;
    *)
      echo "Invalid option"
      sleep 1
      ;;
  esac
done


