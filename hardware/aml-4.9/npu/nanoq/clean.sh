#!/bin/bash

find -name *.o|xargs rm -rf
find -name *.d|xargs rm -rf
find -name *.cmd|xargs rm -rf