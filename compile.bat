g++ -static -O3 -o keygen -IE:./libsodium-master/src/libsodium/include -L. main.cpp -lsodium -lpthread
pause