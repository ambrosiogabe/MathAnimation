# Math Anim

This is a quick and dirty math animation library I am creating for personal use. It should work on any Windows machine, but I am not testing it on any other platforms as of right now. It's main purpose will be to quickly and easily render mathematical animations in real time as well as render mathematical animations to video files.

## Compiling on Windows

First clone the repository and the submodules by running:

```batch
git clone --recursive https://github.com/ambrosiogabe/MathAnimation
```

### First Time Setup

These instructions only need to be followed the first time you ever compile this library.

<details>

<summary>
Click to See First Time Setup Instructions
</summary>

 First we need to setup ffmpeg.

I'm only writing instructions for Windows and MSVC. For information on compiling ffmpeg in a different environment, please see [ffmpeg documentation](https://ffmpeg.org/platform.html#Windows) for further details and make the appropriate changes.

Unfortunately, ffmpeg is a particularly wild beast, so compiling is non-trivial.

#### Setting up Environment for ffmpeg

(_The following instructions are modified from [ffmpeg documentation](https://ffmpeg.org/platform.html#Windows)_)

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

7. Verify that you compiled everything correctly. There should be a file named `build` in the current directory `./Animations/vendor/ffmpeg/build`. Inside this file you should see several directories with and a `lib` folder with the ffmpeg binaries.
    * If this is correct, then you're done compiling ffmpeg.

#### Setting up Environment for freetype

Thankfully, freetype is much simpler to set up than ffmpeg. To compile on windows, I'll be using cmake and MSVC. You can change use a different build system if you like, just ensure that at the end you have two directories for a release and debug version of freetype at the locations:

```bash
./Animations/vendor/freetype/build/Debug/freetyped.lib
./Animations/vendor/freetype/build/Release/freetype.lib
```

To build with CMake and MSVC:

1. Open up a developer command prompt for MSVC.
2. Change into your local directory for this animations library.
3. Run the following commands to compile freetype:

```batch
pushd .\Animations\vendor\freetype
mkdir build
pushd build
cmake ..
msbuild freetype.sln /property:Configuration=Debug
msbuild freetype.sln /property:Configuration=Release
popd
popd
```

4. If this all succeeds, you should see a build directory in `./Animations/vendor/freetype` that contains a Debug directory and Release directory with the appropriate DLLs.

#### Setting up Environment for OpenAL

To compile OpenAL on windows, I'll be using cmake and MSVC. You can change use a different build system if you like, just ensure that at the end you have two directories for a release and debug version of freetype at the locations:

```bash
./Animations/vendor/openal/build/Debug/OpenAL32.dll
./Animations/vendor/openal/build/Release/OpenAL32.dll
```

To build with CMake and MSVC:

1. Open up a developer command prompt for MSVC.
2. Change into your local directory for this animations library.
3. Run the following commands to compile freetype:

```batch
pushd .\Animations\vendor\openal\build
cmake ..
msbuild OpenAL.sln /property:Configuration=Debug
msbuild OpenAL.sln /property:Configuration=Release
popd
```

</details>

### Compiling

_NOTE:_ Make sure that you have completed the first time setup instructions if this is your first time compiling this project. Click the dropdown above to get the full instructions.

Build the VisualStudio project using premake:

```batch
build.bat vs2022
```

Then open the project or compile it from the command line using MSVC developer's prompt.

_NOTE_: For a more comprehensive list of the build options supported, just type `build.bat`.

## Current Features

Right now this library can

* Animate any parametric equation over time
* Draw a grid of arbitrary width
* Draw basic lines
* And much more. One of my TODO's is to update this list with some cool GIF's and/or pictures of the features currently available.

## Planned Features

This library is planned to support

- [ ] Add LaTex support using MicroTex (https://github.com/NanoMichael/MicroTeX)    
- [ ] Export arbitrary video formats (currently only H264 MP4 is supported right now)
- [ ] Swap to st AV1 instead of ffmpeg for video encoding (they have much nicer licensing and they are the future of video codecs)
  > https://gitlab.com/AOMediaCodec/SVT-AV1
- [ ] Add cascade effects when editing timeline
  > For example, if you move a segment to the right, it should "push" the other segments further to the right if the cascade effect is enabled
- [ ] Add a split segment mode
- [ ] Add support for interpolating between SVG objects with different number of paths and contours
  > To do this, I'll have to auto generate the in between points for the contours and then interpolate regularly from there
- [ ] Add follow path animation. Should allow you to plot a custom path and the object will follow that
- [ ] Add math grid animation. Should create a numbered grid.
- [ ] 3D Bezier curve approximations
- [ ] 3D animation objects
- [ ] 3D blending (potentially OIT)
- [ ] Gizmos
- [ ] Textured 2D objects
- [ ] Textured 3D objects
- [ ] Animatable Camera movements
- [ ] Scriptable Animations
- [ ] Multiple Animations on one animation segment
  > I may need to rethink the entire architecture here in order to get this to work
- [ ] Rethink the architecture
  > Right now the way rendering works is I go through each animation up until the current
    frame. If the animation was already completed, I "apply" the animation to the object.
    If an animation is in progress at the current frame, I send an interpolated t value to
    the animation, and it figures out how to render itself. This means that if your at frame
    60,000 in a video, then it must loop through all the previous objects and apply the animations
    before it can render this particular frame. This may or may not be a performance issue, it   remains to be seen
- [ ] Export/Import custom scenes (the plumbing is all there, I just need a file picker UI)
- [ ] Project hub when starting the app
  > Kind of like DaVinci Resolve's startup project display. This way you can just click into the most recent
    project and create a new one without having to do anything complicated
- [ ] Animation preview when hovering over a question mark
- [x] ~~Font previews when selecting a font~~
- [x] ~~View audio waveform preview~~
  - [x] ~~View an audio wave form at the bottom of the timeline editor(and hear the audio as the clip is played) to sync~~
    ~~animations up with an audio clip~~
- [x] ~~3D Lines~~

### Planned Features (nice-to-have, but not necessary)

- [ ] Offload rendering to background thread so that event processing doesn't block render
- [ ] Improve styling for the whole editor. Make it all cohesive and add option to export/import themes.

## Bugs

* The timeline should only accept key events when no other element is focused.
  * For example, if you click a segment, then hit the delete key while entering text in an input box, it
  will delete the segment. This should not happen.

## Licensing

Please see the attached EULA for information on what is and isn't allowed. I'm not opposed to people forking this library and continuing solo development on a new project based off of this source code, however reach out through Github issues or other communication methods with me before doing so. This library is free for you to compile and modify for your own personal use, but it is not free for you to distribute any binary copies (paid or free). If you have any questions, please reach out to me through Github issues, Discord, or any other communication method you find appropriate.
