#!/bin/sh
MANUAL_PATH=/home/markus/midieditor-tags/midieditor-manual
D=${PWD}
cd $MANUAL_PATH
qcollectiongenerator midieditor-collection.qhcp -o midieditor-collection.qhc
cd $D
cp $MANUAL_PATH/midieditor-collection.qhc ../run_environment/assistant/midieditor-collection.qhc
cp $MANUAL_PATH/midieditor-manual.qch ../run_environment/assistant/midieditor-manual.qch