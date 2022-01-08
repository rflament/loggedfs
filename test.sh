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
cat ~/log.txt

# make sure the actions have not been logged in syslog
grep -c grep loggedfs /var/log/syslog
COUNT=$(grep -c grep loggedfs /var/log/syslog)
echo $COUNT
if [ "$COUNT" -ne "0" ]; then
  echo "nothing should have been logged in syslog"
  exit 1
fi