#!/bin/bash

RANDOM_FILE=`ls | sort -R | tail -n 1`
echo ${RANDOM_FILE}
