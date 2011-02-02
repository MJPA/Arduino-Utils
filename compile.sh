#!/bin/sh

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
  echo "${GPP} ${FILE}.c -o ${FILE} -lpthread"
  ${GPP} ${FILE}.c -o ${FILE} -lpthread
  [ $? -ne 0 ] && exit 1
else
  echo "Could not find ${FILE}.c, ignoring..."
fi

FILE=arduino-send
if [ -f "${FILE}.c" ]; then
  echo "${GCC} ${FILE}.c -o ${FILE} -lpthread"
  ${GCC} ${FILE}.c -o ${FILE} -lpthread
  [ $? -ne 0 ] && exit 1
else
  echo "Could not find ${FILE}.c, ignoring..."
fi

FILE=arduino-read
if [ -f "${FILE}.c" ]; then
  echo "${GCC} ${FILE}.c -o ${FILE} -lpthread"
  ${GPP} ${FILE}.c -o ${FILE} -lpthread
  [ $? -ne 0 ] && exit 1
else
  echo "Could not find ${FILE}.c, ignoring..."
fi

echo "Done."
exit 0