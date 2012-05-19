#!/bin/bash
test -d jpg || mkdir -p jpg
for a in xbm/*.xbm
do
   convert xbm/${a#xbm} jpg/${a//xbm}jpg
done

for a in xbm/protocols/*.xbm
do
   convert xbm/${a#xbm} jpg/${a//xbm}jpg
done