# ed25519_pkeygen
## Description

The program is designed to generate a public key ed25519 with a given prefix for a node of the MeshCore mesh network. The program uses all processor threads, so after launching, your computer may start to stutter.

## Building

**Windows**

- Install [Git](https://git-scm.com/install/windows), and [MinGW (GCC)](https://www.mingw-w64.org/getting-started/msys2/).
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

## Usage

### General
`keygen <prefix>`
- `<prefix>` is the prefix that needs to be generated (maximum 8 characters).

### Windows
Command line
`keygen.exe <prefix>`

or using the Windows batch file `start.bat`:
- Edit `start.bat`
- Replace `beebee` with the required prefix
- Run `start.bat`

### Linux
Terminal `keygen <prefix>`

## Performance
On an `AMD Ryzen 5 5600X` processor, the key search speed is about **450'000** keys per second. At this speed, the average search time is:
- 1 hex character - instant
- 2 hex characters - instant
- 3 hex characters - instant
- 4 hex characters - fractions of a second
- 5 hex characters - 2.3 seconds
- 6 hex characters - 37 seconds
- 7 hex characters - 9 minutes 56 seconds
- 8 hex characters - 2 hours 39 minutes

On an `Intel Core i7 14700K` about **1'200'000** keys per second. This is about x2.7 faster than `AMD Ryzen 5 5600X`.
