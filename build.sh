#!/bin/bash

name=movietool
cc=gcc
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

build_lib() {
    pushd $1/ && ./build.sh $2 && mv *.a ../lib/ && popd
}

build() {
    mkdir lib/
    build_lib utopia -slib
    build_lib pdftool -s
}

comp() {
    echo "Compiling $name"
    $cc ${flags[*]} ${inc[*]} ${lib[*]} $src -o $name
}

clean() {
    rm -r lib/
    rm $name
}

fail() { 
    echo "Use '-build' to build dependencies, '-comp' to compile executable or '-run'"
    exit
}

case "$1" in
    "-build")
        build;;
    "-comp")
        build && comp;;
    "-run")
        shift 
        build && comp && ./$name "$@";;
    "-clean")
        clean;;
    *)
        fail;;
esac
