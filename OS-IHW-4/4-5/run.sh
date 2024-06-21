if [ $# -ne 1 ] ; 
then
    echo "You should pass 1 arg: number_of_objects"
    exit 1
fi

chmod 755 server.c
chmod 755 Ivanov.c
chmod 755 Petrov.c
chmod 755 Necheporuk.c

cc server.c -o server.out
./server.out "0.0.0.0" 8080 $1 &
cc Ivanov.c -o Ivanov.out
./Ivanov.out "127.0.0.1" 8080 &
cc Petrov.c -o Petrov.out
./Petrov.out "127.0.0.1" 8080 &
cc Necheporuk.c -o Necheporuk.out
./Necheporuk.out "127.0.0.1" 8080 &
