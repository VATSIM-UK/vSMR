# vSMR Plugin - VATSIM UK

A complete re-write of vSMR from [pierr3](https://github.com/pierr3/vSMR), refactoring the code to use modern C++

## Local Development

### Requirements

vSMR now uses [conan](https://conan.io/) to manage dependencies. 

To install conan:

`$ pip install conan`

Setup a default profile

`$ conan profile detect`

You will also need a `C++` compiler. For windows `MSVC` is recommended. You can install `MSVC` through the [Visual Studio Installer](https://visualstudio.microsoft.com/downloads/)

To compile the program run the provided batch file `build.bat` from the root directory. Optionally pass `-c` to automatically copy the build DLL to `%APPDATA%\EuroScope\UK\Data\Plugin\vSMR`

## Thanks

With thanks to vSMR's original creator @pierr3

Special thanks to Daniel Lange, Even Rognlien, Juha Holopainen, Lionel Bischof and Wenjun Zhou for their help with the code, thanks to Sam White and Theo Bearman for their help on the wiki, and thanks to Jonas Kuster for the countless bug hunts and help on issue tracking!
