# UDP_server_client

1. To compile packages run the following commands:
g++ udp_server.cpp -o udp_server
g++ udp_client.cpp -o udp_client

To test multiple client connections: 

1. Create config.txt with the  following content: "port=8080" | Config file will be added in the following commits
2. Run ```./udp_server``` to set up the server
3. In separate terminal window run ```for i in {1..10}; do ./udp_client 127.0.0.1 55 & echo 1 ; done;``` to simulate several client connections.
4. Server Log will be displayed in the corresponding terminal and written to ```server.log```
5. After each successful transfer, the server_data_for_######:####.csv file will contain the generated array on the server's side. TODO: Change the file type to binary and write down the same array on the client's side. Create script to compare two.
