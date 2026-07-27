

## Build Process

### 1. Configure
```bash
cmake -S . -B build
```

### 2. Build
```bash
cmake --build build # normal building
# debug and building
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

## Install Dependencies

### 1. update g++
```bash
sudo apt install g++
```

### 2. install openssl
```bash
sudo apt-get install libssl-dev
```