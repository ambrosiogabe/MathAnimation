# Math Anim

This is a quick and dirty math animation library I am creating for personal use. It should work on any Windows machine, but I am not testing it on any other platforms as of right now. It's main purpose will be to quickly and easily render mathematical animations in real time as well as render mathematical animations to video files.

## Compiling on Windows

First clone the repository and the submodules by running:

```batch
git clone --recursive https://github.com/ambrosiogabe/MathAnimation
```

### First Time Setup

These instructions only need to be followed the first time you ever compile this library.

<details open>

<summary>
Click to See First Time Setup Instructions
</summary>

 First we need to setup ffmpeg.

I'm only writing instructions for Windows and MSVC. For information on compiling ffmpeg in a different environment, please see [ffmpeg documentation](https://ffmpeg.org/platform.html#Windows) for further details and make the appropriate changes.

Unfortunately, ffmpeg is a particularly wild beast, so compiling is non-trivial.

#### Setting up Environment for ffmpeg

(_The following instructions are modified from ffmpeg documentation_)

* [MSYS2](https://www.msys2.org)
* [NASM](https://www.msys2.org)

Next, make sure that the following are completed.

1. Place `nasm.exe` in your `PATH`.
2. To set up a proper environment in MSYS2, you need to run `msys_shell.bat` from the Visual Studio or Intel Compiler command prompt. To do this:
    * First type in `Developer Command Prompt for VS` in your windows search bar.
    * Run the command prompt.
    * Change directories to where you installed msys2.
        * The default dir for me is `cd C:\tools\msys64`
    * Run `msys2_shell.cmd -use-full-path` to launch msys2.
3. Make sure `cl` works. Running `cl` should print something starting with: `Microsoft (R) C/C++...`
4. Make sure `NASM` is available. Running `nasm -v` should print the version.
5. Change into the directory where you have this repo installed.
6. Finally, to compile ffmpeg, run:

```bash
# NOTE This will take quite some time to compile
pushd ./Animations/vendor/ffmpeg
./configure \
    --toolchain=msvc \
    --prefix=./build \
    --disable-doc \
    --arch=x86_64 \
    --disable-x86asm 
make 
make install

# Rename the files to .lib extension to make premake happy
mv ./build/lib/libavcodec.a ./build/lib/libavcodec.lib
mv ./build/lib/libavdevice.a ./build/lib/libavdevice.lib
mv ./build/lib/libavfilter.a ./build/lib/libavfilter.lib
mv ./build/lib/libavformat.a ./build/lib/libavformat.lib
mv ./build/lib/libavutil.a ./build/lib/libavutil.lib
mv ./build/lib/libswresample.a ./build/lib/libswresample.lib
mv ./build/lib/libswscale.a ./build/lib/libswscale.lib
popd
```

7. Verify that you compiled everything correctly. There should be a file named `bin` in the current directory `./Animations/vendor/ffmpeg/bin`. Inside this file you should see 7 files labeled `libx.a` and they should match the name of the directories in `./Animations/vendor/ffmpeg`.
    * If this is correct, then you're done compiling ffmpeg.

</details>

### Compiling

_NOTE:_ Make sure that you have completed the first time setup instructions if this is your first time compiling this project. Click the dropdown above to get the full instructions.

Some more stuff...

## Current Features

Right now this library can

* Animate any parametric equation over time
* Draw a grid of arbitrary width
* Draw basic lines

## Planned Features

This library is planned to support

* Render to video files (.mp4, .mov, etc)
* Draw coordinate marks
* Animate and draw text in any font format
* Animate text over time
* Animate interpolations transitioning from one curve to another
* Draw arbitrary shapes
* Draw textured quads
* Animate camera movements
