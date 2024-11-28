# UDP_server_client

1. To compile packages run the following commands:
```
mkdir build
cd build
cmake ..
make
```
2. In a separate terminal window open "/*project_folder*/build
To run a server:
```
cp ../config.txt ./
./Server
```
To run Client in the build folder:
```
cp ../config_client.txt ./
./Client
```
To test multiple client connections in project folder run : 
```
chmod +x run_client_in_folders.sh
./run_client_in_folders.sh
```
This script generates 10 folders with a copy of the config file and runs Client with root in each folder.
