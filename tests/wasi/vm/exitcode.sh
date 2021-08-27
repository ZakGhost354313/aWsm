#!/bin/bash

cd "$(dirname "${BASH_SOURCE[0]}")" || exit
mkdir -p data

./exitcode_vm
if (($? != 120)); then
	echo "Fail!"
	exit 1
else
	exit 0
fi