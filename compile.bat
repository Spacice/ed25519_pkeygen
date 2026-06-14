g++ -static -O3 -o keygen -IE:./libsodium/src/libsodium/include -L. main.cpp -lsodium -lpthread
pause