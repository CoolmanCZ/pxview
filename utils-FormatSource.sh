#!/bin/bash

path="app/PxView"
files=`find ${path} -path ${path}/lib -prune -type f -o -name "*.cpp" -o -name "*.h"`

for i in ${files}; do
	if [[ "${i}" =~ "Common.h" ]]; then
		continue;
	fi
	#echo "${i}"
	clang-format -style="file" -i "${i}"
done

