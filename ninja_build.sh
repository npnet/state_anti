#!/bin/bash
clear
rm -rf out
. tools/core_launch.sh&&cout&&cmake ../.. -G Ninja&&ninja
