#!/usr/bin/bash -e


if [[ $# -eq 0 ]]; then
    echo "Usage: $0 [dev|release]"
    exit 1
fi

build_dev() {
    rm -rf build
    meson setup build --buildtype debug -Duse_sdl=true
    meson compile -C build
}


build_rel() {
    rm -rf build-rpi
    meson setup build-rpi --cross-file toolchain-rpi.txt --buildtype release --strip -Duse_drm=true
    meson compile -C build-rpi/
}


case "$1" in
    dev)
        build_dev
        ;;
    release)
        build_rel
        ;;
    *)
        echo "Invalid option: $1"
        echo "Usage: $0 [dev|release]"
        exit 1
        ;;
esac
