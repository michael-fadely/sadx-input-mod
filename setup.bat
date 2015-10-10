@echo off
git submodule update --init --recursive
cd SDL2
cmake -G "Visual Studio 14 2015" -T "v140_xp"
cd ..