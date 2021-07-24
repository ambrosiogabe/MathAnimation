# Generating Projects

First we will clone this repository. Create a new directory where you would like your project to live. For example I'll create a directory called `MyCoolProject`. Open a command prompt in this directory and execute:

```bat
git clone https://github.com/ambrosiogabe/opengl-template-cpp ./
```

Then we will pull all our dependencies and create the file structure for our project. Type in:

```bat
build.bat create MyCoolProject
```

where `MyCoolProject` is the name of your project. Finally we will build the actual project files using premake. Type in:

```bat
build.bat vs2019
```

You can use vs2015, vs2017, codelite, xcode, etc. To see all options just type in `build.bat` and hit enter. Once you have a generated project file click onto the project file then compile and run your project and you should be greeted with a window and a console window printing out 'Hello OpenGL'.

## Requirements

In order to use this build script you must have python and git installed. This means you should be able to run:

```bat
python --version
git --version
```

and not encounter any errors. If this works then you should be able to follow the instructions above and generate an OpenGL project.
