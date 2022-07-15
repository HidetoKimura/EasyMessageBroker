# Easy Message Broker

## Build
~~~
$ mkdir build
$ cd build
$ cmake ..
$ make
~~~

## Test
~~~
$ ./broker &
$ ./pubsub /tmp/emb_fifo &
$ ./pubsub /tmp/emb_fifo2 &
$ echo "sub <topic>" > /tmp/emb_fifo
output id : ex. sub id = 10000

$ echo "sub <topic>" > /tmp/emb_fifo2
output id : ex. sub id = 10001

$ echo "pub <topic> <message>" > /tmp/emb_fifo
$ echo "pubid <topic> <message> <id>" > /tmp/emb_fifo
$ echo "unsub <id>" > /tmp/emb_fifo
$ echo "stop" > /tmp/emb_fifo
$ echo "stop" > /tmp/emb_fifo2

$ killall broker
~~~
