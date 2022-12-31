set -e
set -x

pushd ./Animations/vendor/GLFW/
  rm -rf build
  mkdir build
  cmake -B build -D GLFW_USE_WAYLAND=0 -G "Unix Makefiles"
  cmake --build build -j4
popd

# # Very slow
# pushd Animations/vendor/ffmpeg/
#   rm -rf ./build/
#   ./configure --prefix=./build --disable-doc --disable-x86asm --disable-xlib
#   make -j4 install
# popd

# pushd Animations/vendor/openal/build/
#   rm -rf {Debug,Release}

#   mkdir Debug
#   pushd Debug
#     cmake ../.. -DCMAKE_BUILD_TYPE=Debug
#     make -j4
#   popd

#   mkdir Release
#   pushd Release
#     cmake ../.. -DCMAKE_BUILD_TYPE=Release
#     make -j4
#   popd
# popd

# pushd Animations/vendor/onigurama/
#   if test -f "Makefile"; then
#     make distclean
#   fi
#   ./configure --prefix=$PWD/build
# popd

# pushd Animations/vendor/luau/
#   rm -rf build
#   mkdir build
#   cd build

#   mkdir Debug
#   pushd Debug
#     cmake ../.. -DCMAKE_BUILD_TYPE=RelWithDebInfo

#     cmake --build . -j4 --target Luau.Repl.CLI --config Debug
#     cmake --build . -j4 --target Luau.Analyze.CLI --config Debug
#   popd

#   mkdir Release
#   pushd Release
#     cmake ../.. -DCMAKE_BUILD_TYPE=RelWithDebInfo

#     cmake --build . -j4 --target Luau.Repl.CLI --config Release
#     cmake --build . -j4 --target Luau.Analyze.CLI --config Release
#   popd  

#   # rename 's/(.*).a/lib$1.lib' Debug/*.a
#   # rename 's/(.*).a/lib$1.lib' Release/*.a
# popd

# make clean
rm -f *.make Makefile
premake5 --cc=clang gmake
make Animations