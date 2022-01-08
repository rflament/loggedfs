#!/bin/bash

TEST_DIR=/tmp/test-loggedfs
mkdir $TEST_DIR

####################################################################
## Test 1: log actions in a file
####################################################################

# launch loggedfs
./loggedfs -l ~/log.txt $TEST_DIR

# create file
echo "bar" >> $TEST_DIR/foo && ls $TEST_DIR
sudo umount $TEST_DIR

# make sure the actions have been logged in the log file
COUNT=$(grep -c "write 4 bytes to $TEST_DIR/foo at offset 0" ~/log.txt)
if [ "$COUNT" -ne "1" ]; then
  echo "write should have been logged in file"
  exit 1
fi

# make sure the actions have not been logged in syslog
COUNT=$(grep -c "write 4 bytes to $TEST_DIR/foo at offset 0" /var/log/syslog)
if [ "$COUNT" -ne "0" ]; then
  echo "nothing should have been logged in syslog"
  exit 1
fi

rm ~/log.txt

####################################################################
## Test 2: log actions to syslog
####################################################################

# launch loggedfs
./loggedfs $TEST_DIR

# create file
echo "bar" >> $TEST_DIR/foo2 && ls $TEST_DIR
sudo umount $TEST_DIR

# make sure the actions have been logged in syslog
COUNT=$(grep -c "write 4 bytes to $TEST_DIR/foo2 at offset 0" /var/log/syslog)
if [ "$COUNT" -ne "1" ]; then
  echo "write should have been logged in syslog"
  exit 1
fi