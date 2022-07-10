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
$ ./pubsub sub topic_a &
$ ./pubsub sub topic_b &
$ ./pubsub pub topic_b hello &

$ killall broker
$ killall pubsub
~~~
