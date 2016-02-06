#!/bin/sh

ps aux | grep srun | grep -v grep | awk '{ print "kill -12", $2 }' | sh