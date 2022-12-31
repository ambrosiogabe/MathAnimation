set -e
set -x

git submodule update --init --recursive

pushd ./Animations/vendor/GLFW/
  rm -rf build
  mkdir -p build
  cmake -B build -D GLFW_USE_WAYLAND=0 -G "Unix Makefiles"
  cmake --build build -j4
popd

# Very slow
pushd Animations/vendor/ffmpeg/
  rm -rf ./build/
  ./configure --prefix=./build --disable-doc --disable-x86asm
  make -j4 install
popd

pushd Animations/vendor/openal/build/
  rm -rf {Debug,Release}

  mkdir -p Debug
  pushd Debug
    cmake ../.. -DCMAKE_BUILD_TYPE=Debug
    make -j4
  popd

  mkdir -p Release
  pushd Release
    cmake ../.. -DCMAKE_BUILD_TYPE=Release
    make -j4
  popd
popd

pushd Animations/vendor/onigurama/
  if test -f "Makefile"; then
    make distclean
  fi

  mkdir -p build
  pushd build
    cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
    make -j4
  popd
popd

pushd Animations/vendor/luau/
  rm -rf build
  mkdir -p build
  cd build

  mkdir -p Debug
  pushd Debug
    cmake ../.. -DCMAKE_BUILD_TYPE=RelWithDebInfo

    cmake --build . -j4 --target Luau.Repl.CLI --config Debug
    cmake --build . -j4 --target Luau.Analyze.CLI --config Debug
  popd

  mkdir -p Release
  pushd Release
    cmake ../.. -DCMAKE_BUILD_TYPE=RelWithDebInfo

    cmake --build . -j4 --target Luau.Repl.CLI --config Release
    cmake --build . -j4 --target Luau.Analyze.CLI --config Release
  popd
popd

if test -f "Makefile"; then
  make clean
  rm -f *.make Makefile
fi
premake5 --cc=gcc gmake
make -j4 DearImGui
make -j4 TinyXml2
make -j4 plutovg
make -j4 Oniguruma
make -j4 Animations