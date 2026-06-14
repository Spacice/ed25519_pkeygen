# ed25519_pkeygen
## Description

The program is designed to generate a public key for a device in the MeshCore mesh network. The program uses all processor threads, so after launching, your computer may start to stutter.

## Building

**Windows**

- Install Git, GNU GCC compiler.
- Clone [Sodium library](https://github.com/jedisct1/libsodium).
- Enter command `git clone https://github.com/Spacice/ed25519_pkeygen` in the command line.
- Go to [Sodium releases](https://github.com/jedisct1/libsodium/releases/).
- Download the latest release.
- Place the `libsodium.a` of the Sodium library into the folder with the current source code.
- Run `compile.bat`.

**Linux**

Enter commands in the terminal:
- `sudo apt install libsodium-dev`
- `git clone https://github.com/jedisct1/libsodium`
- `git clone https://github.com/Spacice/ed25519_pkeygen`
- `g++ -static -O3 -o keygen -I./libsodium-master/src/libsodium/include -L. main.cpp -lsodium -lpthread`

## Using

Windows command line
`keygen.exe beebee`

Linux terminal
`keygen beebee`

`beebee` is the prefix that needs to be generated.
