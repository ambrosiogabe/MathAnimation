# Math Anim

This is a tool I use to create animations for my videos on my YouTube channel. You can see an example of a video created using this tool [here](https://www.youtube.com/watch?v=iydG-e1dQGA). My goal is to have nearly identical animations to those produced by [Manim](https://www.manim.community), except in realtime with a GUI+audio preview to enhance the editing process.

This is a small GIF showcasing some of the capabilities of this tool:

![GIF](.github/images/app-showcase.gif)

## Supported Platforms

* Windows

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

### Compiling FFmpeg

In order to compile this manually, you need to build static binaries for FFmpeg so they can be copied to the final build and statically linked into the application.

#### Setting up Environment for ffmpeg

I'm only writing instructions for Windows and MSVC. For information on compiling ffmpeg in a different environment, please see the [ffmpeg documentation](https://ffmpeg.org/platform.html#Windows) for further details and make the appropriate changes.

Unfortunately, ffmpeg is a particularly wild beast, so compiling is non-trivial.

(_The following instructions are modified from [ffmpeg documentation](https://ffmpeg.org/platform.html#Windows)_)

First, make sure to have these tools installed:

* [MSYS2](https://www.msys2.org)
* [NASM](https://www.nasm.us)

Next, follow these steps:

1. Place `nasm.exe` in your `PATH`.
2. To set up a proper environment in MSYS2, you need to run `msys_shell.bat` from the Visual Studio or Intel Compiler command prompt. To do this:
    * First type in `x86_x64 Cross Tools Command Prompt for VS 2022` in your windows search bar.
      * NOTE: If you're compiling for 32-bit architecture, you'll have to open the x86 version and modify the instructions below to use 32 bit architecture information
    * Run the command prompt.
    * Change directories to where you installed msys2.
        * The default directory for me is `cd C:\tools\msys64`
    * Run `msys2_shell.cmd -use-full-path` to launch msys2.
3. Make sure `cl` works. Running `cl` should print something starting with: `Microsoft (R) C/C++...`
4. Make sure `NASM` is available. Running `nasm -v` should print the version.
5. Change into the directory where you have this repo installed.
    * You may need to install some dependencies in order to compile this:
        * `pacman -S diffutils`
        * `pacman -S make`
6. Finally, to compile ffmpeg, run this command in the terminal that you launched from step 2:

```bash
# NOTE This will take quite some time to compile
# To compile it faster you can use `make -j{core count}` instead of `make` where
# core count is 2 cores less than the number of cores available on your machine
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

7. Verify that you compiled everything correctly. There should be a file named `build` in the directory `./Animations/vendor/ffmpeg/build`. Inside this directory you should see several files with a `.lib` extension, these are the ffmpeg binaries.
    * If this is correct, then you're done compiling ffmpeg.

</details>

### Compiling

> _NOTE:_ Make sure that you have completed the first time setup instructions if this is your first time compiling this project. Click the dropdown above to get the full instructions.

<br/>

> _NOTE:_ CMake is required to build this. Make sure you have CMake 3.15 or newer installed.

Run the following commands:

```batch
mkdir build 
pushd build 
cmake ..
popd 
```

Then open the project `build/MathAnimationsPrj.sln` or compile it from the command line using the MSVC developer's prompt.

## Current Features

Project Management:

* Create/Open projects using the project splash screen
* Create/Open different scenes inside a project using `Scene Manager` tab.
* Create/Open different scripts using `Asset Manager` tab.

Console Output:

* Any scripts added in the `Asset Manager` will dump any errors here
* Any logs written from a script will end up here
* Click on a log to open the file that logged it

Animation Editor View:

* Use this view to edit scenes as you're working on them
* Gizmos are available in this view to drag objects around
* You can click on objects here to pull up their properties in the inspector
* Mouse scroll wheel to zoom
* Click mouse scroll wheel to pan

Animation View:

* Final rendered view of the current scene
* Press play/stop to start the animation
  * Press space at any time to play/stop the animation as well

Export Video:

* Export the final animation as an mp4 file

Timeline (can be found in the `Timeline` tab):

* Zoom timeline in out using the zoom bar in the top center
* Enable snapping of timeline clips by clicking the magnet icon
  * Red means it's on, white means it's off
* Reset zoom by pressing the `Undo-All` button to the right of the zoom bar
* Create new timeline tracks by right-clicking the left panel
* Add/Remove audio waveform preview by right-clicking the left panel
* Drag/drop animations onto the timeline
* Drag cursor by clicking the ruler and dragging
* Delete clips by selecting and pressing `Delete` on your keyboard
* Resize clips by clicking edge of clips
* Move clips by clicking and dragging the clips

Animations (can be found in the `Animations` tab):

* Move To animation (moves an object to the specified position)
* Create Animation (renders the outline of the object/children and then fills in the shapes)
* Un-Create (Create in reverse)
* Fade In
* Fade Out
* Replacement Transform (morphs one object/children into another object/children)
* Rotate To (NOT IMPLEMENTED)
* Animate Stroke Color (NOT IMPLEMENTED)
* Animate Fill Color (NOT IMPLEMENTED)
* Animate Stroke Width (NOT IMPLEMENTED)
* Shift (BROKEN)
* Circumscribe (plays an animation where a line surrounds the object/children then disappears)
* Animate Scale

Scene Tab:

* Object is green if it's animating, gray if it's inactive, and white if it's active
* Right-Click to add animation object
* Select object and press `Delete` on your keyboard to delete it
* Rearrange the scene heirarchy by clicking and dragging the objects around

Animation Objects (can be found in the `Scene` tab):

* Camera
* Text Block (add text using any font installed on your machine)
* LaTeX Object (add LaTeX as an object in the scene)
* Square
* Circle
* Cube (STATUS UNKNOWN)
* Axis (NOT IMPLEMENTED)
* SVG File Object (import SVG files as objects in the scene)
* Script Object (drag scripts created in the assets panel here to generate custom objects)
* Code Block (like text block, except it highlights the code with the language/theme selected)
* Arrow

Animation Object Inspector:

* Click on any object in your scene tab to pull up its properties here
* If there is a button with a `Copy` symbol to the right of it, clicking that will apply the change to all children
* SVG scale determines the resolution to render to the SVG cache. Increasing this increases the fidelity, but you have limited space so don't increase it too much.

Animation Inspector:

* Click on any segment in your `Timeline` to open its properties here
* `Anim Objects` dropdown allows you to add animation objects to this animation. Just drag the animation object from your `Scene` panel into the drop area after pressing `Add Anim Object`
* Change the animation curve or whether an animation is `Synchronous` or `Lagged`

App Metrics:

* If you feel like the app is being slow, open this to get more detailed information such as FPS and the load on the app

Editor Settings:

* Change the scene view from wireframe/filled
* Adjust editor camera sensitivity

## Licensing

Please see the attached EULA for information on what is and isn't allowed. I'm not opposed to people forking this library and continuing solo development on a new project based off of this source code, however reach out through Github issues or other communication methods with me before doing so. This library is free for you to compile and modify for your own personal use, but it is not free for you to distribute any binary copies (paid or free). If you have any questions, please reach out to me through Github issues, Discord, or any other communication method you find appropriate.
