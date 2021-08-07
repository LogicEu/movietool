#!/bin/bash

name=movietool
comp=gcc
src=*.c

flags=(
    -Wall
    -O2
    -std=c99
)

inc=(
    -I.
    -Iinclude/
)

lib=(
    -Llib/
    -lpdftool
    -lutopia
    -lz
    -lxlsxwriter
)


build() {
    pushd utopia/
    ./build.sh -slib
    popd
    pushd pdftool/
    ./build.sh -s
    popd

    mkdir lib/
    mv utopia/libutopia.a lib/libutopia.a
    mv pdftool/libpdftool.a lib/libpdftool.a
}

comp() {
    echo "Compiling $name"
    $comp ${flags[*]} ${inc[*]} ${lib[*]} $src -o $name
}

clean() {
    rm -r lib/
    rm $name
}

fail() { 
    echo "Use '-build' to build dependencies, '-comp' to compile executable or '-run'"
    exit
}

if [[ $# < 1 ]]; then 
    fail
elif [[ "$1" == "-build" ]]; then
    build
elif [[ "$1" == "-comp" ]]; then
    build
    comp
elif [[ "$1" == "-run" ]]; then 
    shift
    build
    comp
    ./$name "$@"
elif [[ "$1" == "-clean" ]]; then
    clean
else 
    fail
fi
