#!/usr/bin/env bash
binary="jc-model-renderer.exe"

if [[ "$@" == '-d' ]] || [[ "$@" == "--debug" ]]
then
  binary="jc-model-renderer-d.exe"
fi

python build.py $@ && ./$binary $@