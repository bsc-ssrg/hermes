#!/usr/bin/env bash

function help() {
  echo "Usage:"
  echo "  $(basename "$0") COMMAND"
  echo ""
  echo "  Where COMMAND is one of:"
  echo "    start PIDFILE PROGRAM ARGS...  Run PROGRAM and record its PID in PIDFILE"
  echo "    stop SIGNAL PIDFILE            Send SIGNAL to the PID contained in PIDFILE"
  echo "    help                      Print this message"
}

function readpid() {

  if [ $# -eq 0 ]; then
    echo "FATAL: readpid(): Missing pifile" >&2
    exit 1
  fi

  pidfile="$1"

  read -r pid <"$pidfile"
  echo "$pid"
}

function run() {

  if [ $# -eq 0 ]; then
    echo "FATAL: missing program pidfile" >&2
  elif [ $# -eq 1 ]; then
    echo "FATAL: missing program to run" >&2
    help
    exit 1
  fi

  pidfile="$1"
  shift

  if [ -e "$pidfile" ]; then
    pid=$(readpid "$pidfile")
    echo "$pid"

    if pgrep --pidfile "$pidfile"; then
      exit 1
    fi
  fi

  "$@" 2>/dev/null 1>/dev/null 0</dev/null &
  pid=$!
  echo $pid >"$pidfile"
  sleep 1

  if ! kill -0 $pid; then
    echo "Process $pid does not seem to exist." >&2
    echo "The program below may not exist or may have crashed while starting:" >&2
    echo "  $*" >&2
    exit 1
  fi

  exit 0
}

function stop() {

  if [ $# -eq 0 ]; then
    echo "FATAL: missing signal" >&2
    exit 1
  elif [ $# -eq 1 ]; then
    echo "FATAL: missing pidfile" >&2
    exit 1
  fi

  signal="$1"
  pidfile="$2"

  if [ ! -e "$pidfile" ]; then
    echo "FATAL: pidfile '$pidfile' does not exist" >&2
    exit 1
  fi

  if pkill "-$signal" --pidfile "$pidfile"; then
    rm "$pidfile"
    exit 0
  fi
  exit 1
}

if [ $# -eq 0 ]; then
  echo "FATAL: missing arguments" >&2
  help
  exit 1
fi

case $1 in

start)
  shift
  run "$@"
  ;;

stop)
  shift
  stop "$@"
  ;;

help)
  help
  exit 0
  ;;
esac
