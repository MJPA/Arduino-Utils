#!/bin/sh

# Help?
if [ "$1" == "-h" ]; then
  echo "Usage: $0 [arduino_device] [socket_location]"
  echo
  echo "arduino_device: The device handle that will be used to read from the Arduino. Defaults to /dev/arduino"
  echo "socket_location: The socket to use for communication between the utilities. Defaults to /tmp/arduino.sock"
  exit 0
fi

# Check for device / socket locations
DARGS=
if [ ! -z $1 ]; then
  DARGS="${DARGS}-DARDUINO=\"$1\" "
fi
if [ ! -z $2 ]; then
  DARGS="${DARGS}-DSOCKET_PATH=\"$2\" "
fi

GCC=`which gcc 2>/dev/null`
GPP=`which g++ 2>/dev/null`

# Compiler check...
if [ "x${GCC}" == "x" ] || [ "x${GPP}" == "x" ]; then
  echo "Missing required compilers (gcc and g++), exiting..."
  exit 1
fi

# Build the files, warn if we don't find one
FILE=arduino
if [ -f "${FILE}.c" ]; then
  echo "${GPP} ${FILE}.c -o ${FILE} -lpthread ${DARGS}"
  ${GPP} ${FILE}.c -o ${FILE} -lpthread ${DARGS}
  [ $? -ne 0 ] && exit 1
else
  echo "Could not find ${FILE}.c, ignoring..."
fi

FILE=arduino-send
if [ -f "${FILE}.c" ]; then
  echo "${GCC} ${FILE}.c -o ${FILE} -lpthread ${DARGS}"
  ${GCC} ${FILE}.c -o ${FILE} -lpthread ${DARGS}
  [ $? -ne 0 ] && exit 1
else
  echo "Could not find ${FILE}.c, ignoring..."
fi

FILE=arduino-read
if [ -f "${FILE}.c" ]; then
  echo "${GCC} ${FILE}.c -o ${FILE} -lpthread ${DARGS}"
  ${GPP} ${FILE}.c -o ${FILE} -lpthread ${DARGS}
  [ $? -ne 0 ] && exit 1
else
  echo "Could not find ${FILE}.c, ignoring..."
fi

echo "Done."
exit 0