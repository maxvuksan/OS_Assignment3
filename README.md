## Multi-Threaded Network Server for Pattern Analysis

#### compile

`gcc -pthread assignment3.c -o assignment3`

#### start server 

if pattern is left blank, will run without analysis thread 

`./assignment3 -l PORT -p PATTERN`

#### add clients

`nc localhost PORT -i DELAY < FILE`