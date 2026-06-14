# ed25519_pkeygen
## Description

The program is designed to generate a public key for a device in the MeshCore mesh network. The program uses all processor threads, so after launching, your computer may start to stutter.

## Building

**Windows**

- Install Git, and MinGW compiler.
- Enter command `git clone https://github.com/Spacice/ed25519_pkeygen` in the command line.
- Go to [Sodium releases](https://github.com/jedisct1/libsodium/releases/).
- Download the latest Sodium release.
- Unzip the Sodium library release archive into the source code folder.
- Run `compile.bat`.

**Linux**

- `sudo apt install libsodium-dev`
- `git clone https://github.com/Spacice/ed25519_pkeygen`
- `cd ./ed25519_pkeygen`
- `g++ -static -O3 -o keygen main.cpp -lsodium -lpthread`

## Using

Windows command line
`keygen.exe beebee`*

or using the Windows batch file `start.bat`:
- Edit `start.bat`
- Replace `beebee`* with the required prefix
- Run `start.bat`

Linux terminal
`keygen beebee`*

*`beebee` is the prefix that needs to be generated (maximum 8 characters).
