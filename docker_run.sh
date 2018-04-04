#!/usr/bin/bash

TARGET=$1

docker run --rm -it -v $(pwd):/workdir/sherlock.lv2 ventosus/${TARGET}
