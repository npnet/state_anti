#!/bin/bash
clear
. tools/core_launch.sh&&cout&&cmake ../.. -G Ninja&&ninja
