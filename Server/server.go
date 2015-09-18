package main

import (
	"bufio"
	"fmt"
	"net"
)

func listenAndServe(conn net.Conn) {
	//Close connection when done reading
	defer func() {
		conn.Close()
		fmt.Println("Connection closed.")
	}()

	for {
		message, err := bufio.NewReader(conn).ReadString('\n')
		if err != nil {
			fmt.Println("Error detected reading from connection.")
			break
		}
		fmt.Print(string(message))
		conn.Write([]byte("\n"))
	}
}

func main() {
	//	go func() {

	//		http.Handle("/foo", fooHandler)

	//		http.HandleFunc("/bar", func(w http.ResponseWriter, r *http.Request) {
	//			fmt.Fprintf(w, "Hello, %q", html.EscapeString(r.URL.Path))
	//		})

	//		log.Fatal(http.ListenAndServe(":8080", nil))
	//	}()
	fmt.Println("Starting device server...")

	ln, err := net.Listen("tcp", ":3636")
	if err != nil {
		fmt.Println(err)
		return
	}

	for {
		conn, err := ln.Accept()
		if err != nil {
			fmt.Println("Failed to accept connection.", err)
			continue
		}

		//Start go routine to handle messages on this connection
		go listenAndServe(conn)
	}
}
