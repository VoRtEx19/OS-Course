if [ $# -ne 3 ] ; 
then
    echo "You should pass 3 args: port1, port2, number_of_objects"
    exit 1
fi

chmod 755 server.c
chmod 755 Ivanov.c
chmod 755 Petrov.c
chmod 755 Necheporuk.c
chmod 755 observer.c

cc server.c -o server.out
./server.out "0.0.0.0" $1 $2 $3 &
cc Ivanov.c -o Ivanov.out
./Ivanov.out "127.0.0.1" $1 &
cc Petrov.c -o Petrov.out
./Petrov.out "127.0.0.1" $1 &
cc Necheporuk.c -o Necheporuk.out
./Necheporuk.out "127.0.0.1" $1 &
cc observer.c -o observer.out
./observer.out "127.0.0.01" $2 &
