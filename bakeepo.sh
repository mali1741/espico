#!/bin/bash

if [ -z $1 ] ; then
  echo "bakeepo.sh <bin> <p8>"
fi
if [ -z $2 ] ; then
  echo "bakeepo.sh <bin> <p8>"
fi

cat $2
echo "__epo__"
xxd -p $1
