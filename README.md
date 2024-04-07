## Avalanche Game Tools
An application containing a series of tools required for modding Avalanche Engine games, including the Just Cause series, Generation Zero and Mad Max.

We support all the common file formats used in Avalanche Engine games (from 2015 to present), with the ability to edit files and repack within the application - you don't have to learn command line tools to use this.

<p align="center"><img src="https://kirkh.am/jc-model-renderer/window-v2.png" alt="Avalanche Game Main Window" title="Main Window"></p>

### Build Requirements
 - Visual Studio 2022 or later
 - Python 3.1 or later (only needed if you want to build the asset lookup files)
 
### Installation
Download a version from [releases](https://github.com/aaronkirkham/jc-model-renderer/releases) __or__ build it yourself:
 - Clone this repository
 - Run `git submodule update --init --recursive`
 - Run `./build_and_run.sh` to build and run, or run `python build.py` manually.
 - After the first initial run, you can open and build `out/jc-model-renderer.sln` in Visual Studio if preferred.

### Contributions
Code contributions are welcomed and encouraged - if you have an idea for a feature or simply want to improve the code, feel free to create a Pull Request!

### License
MIT.
