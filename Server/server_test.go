package main

import (
	"bufio"
	"fmt"
	"net"
	"testing"
)

func TestHandling(t *testing.T) {
	cn, err := net.Dial("tcp", "localhost:3636")
	if err != nil {
		fmt.Println(err)
		t.FailNow()
	}

	shortMsg := "%d gameStart\n"
	longMsg := "%d gotta type a very long message such that it is over 100 chars such that it will detect it as being a GameEnd event\n"

	for i := 0; i < 2; i++ {
		_, err := cn.Write([]byte(fmt.Sprintf(shortMsg, i)))
		if err != nil {
			fmt.Println(err)
		}
		_, err = bufio.NewReader(cn).ReadString('\n')
		if err != nil {
			fmt.Println(err)
		}
		_, err = cn.Write([]byte(fmt.Sprintf(longMsg, i)))
		if err != nil {
			fmt.Println(err)
		}
		_, err = bufio.NewReader(cn).ReadString('\n')
		if err != nil {
			fmt.Println(err)
		}
	}

	cn.Close()
}
