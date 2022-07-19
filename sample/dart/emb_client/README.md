# Emb client for Dart

This is Emb Client for Dart.

A sample command-line application with an entrypoint in `bin/`, library code
in `lib/`, and example unit test in `test/`.

- How to install 
~~~
 sudo apt-get update
 sudo apt-get install apt-transport-https
 wget -qO- https://dl-ssl.google.com/linux/linux_signing_key.pub | sudo gpg --dearmor -o /usr/share/keyrings/dart.gpg
 echo 'deb [signed-by=/usr/share/keyrings/dart.gpg arch=amd64] https://storage.googleapis.com/download.dartlang.org/linux/debian stable main' | sudo tee /etc/apt/sources.list.d/dart_stable.list
 sudo apt-get update
 sudo apt-get install dart
~~~

- How to use

~~~
$ ./broker &
$ dart run
~~~
or 
~~~
$ ./broker &
$ dart compile exe bin/emb_client.dart
$ bin/emb_client.exe
~~~
