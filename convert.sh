#!/bin/sh
rm tmp/*
mkdir tmp
pdftoppm $1 tmp/image

mkdir $2
convert tmp/image-*.ppm -rotate 90 -resize 1024x1024 -depth 8 -colors 256  $2/image%02d.pcx
