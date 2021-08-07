#!/bin/bash

name=movietool
comp=gcc
src=*.c

flags=(
    -Wall
    -Wextra
    -O2
    -std=c99
)

inc=(
    -I.
    -Iinclude/
    -lz
    -lpdftool
    -lutopia
    -lxlsxwriter
)

build() {
    
}

comp() {
    echo "Compiling $name"
    $comp ${flags[*]} ${inc[*]} $src -o $name
}

fail() { 
    echo "Use '-build' to build dependencies, '-comp' to compile executable or '-run'"
    exit
}

if [[ $# < 1 ]]; then 
    fail
elif [[ "$1" == "-comp" ]]; then
    comp
elif [[ "$1" == "-run" ]]; then 
    shift
    comp
    ./$name "$@"
else 
    fail
fi
