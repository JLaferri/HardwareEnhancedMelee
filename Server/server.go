package main

import (
	"bufio"
	"fmt"
	"net"
	"os"
)

const summarySizeThreshold = 150

const (
	logName     = "Log.txt"
	infoName    = "Info.json"
	summaryMask = "Game%d.json"
)

var streamRoot string
var localRoot string
var results []string

func init() {
	localRoot = "C:/HardwareEnhancedMelee"
	streamRoot = "C:/HardwareEnhancedMelee/Stream"
	results = make([]string, 0, 5)
}

func listenAndServe(conn net.Conn) {
	//Close connection when done reading
	defer func() {
		conn.Close()
		fmt.Println("Connection closed.")
	}()

	for {
		b, err := bufio.NewReader(conn).ReadByte()
		if err != nil {
			fmt.Println("Error detected reading from connection.")
			break
		}
		fmt.Printf("[%x]", b)

		//		//Append log file
		//		err = os.Chdir(localRoot)
		//		if err != nil {
		//			fmt.Printf("Failed to change to local directory %s\n", localRoot)
		//			continue
		//		}

		//		file, err := os.OpenFile(logName, os.O_APPEND, 0700)
		//		if err != nil {
		//			fmt.Println("Failed to open log file.", err)
		//			if os.IsNotExist(err) {
		//				file, err = os.Create(logName)
		//				if err != nil {
		//					fmt.Printf("Failed to create log file\n")
		//				}
		//			}
		//		}

		//		if file != nil {
		//			file.WriteString(fmt.Sprintf("%v|%s", time.Now().String(), s))
		//			err = file.Close()
		//			if err != nil {
		//				fmt.Println("Failed to close log file. ", err)
		//			}
		//		}

		//		//Change directory to stream root
		//		err = os.Chdir(streamRoot)
		//		if err != nil {
		//			fmt.Printf("Failed to change to stream directory %s\n", streamRoot)
		//			continue
		//		}

		//		//Check size threshold to determine if this is a GameStart of GameEnd message
		//		if len(s) > summarySizeThreshold {
		//			//This is a GameEnd message
		//			//Maintain a buffer of the 5 most recent GameEnd summaries
		//			if len(results) >= 5 {
		//				results = results[1:]
		//			}
		//			results = append(results, s)

		//			//Write out game summary files for stream
		//			num := 0
		//			for i := len(results) - 1; i >= 0; i-- {
		//				num++
		//				fileName := fmt.Sprintf(summaryMask, num)
		//				file, err = os.Create(fileName)
		//				if err != nil {
		//					fmt.Printf("Failed to create file %s in directory %s\n", fileName, streamRoot)
		//					continue
		//				}

		//				_, err = file.WriteString(results[i])
		//				if err != nil {
		//					fmt.Printf("Failed to write file %s\n", fileName)
		//				}

		//				err = file.Close()
		//				if err != nil {
		//					fmt.Println("Failed to close summary file. ", err)
		//				}
		//			}
		//		} else {
		//			//This is a GameStart message
		//			//Write out info
		//			file, err := os.Create(infoName)
		//			if err != nil {
		//				fmt.Printf("Failed to create file %s in directory %s\n", infoName, streamRoot)
		//				continue
		//			}

		//			_, err = file.WriteString(s)
		//			if err != nil {
		//				fmt.Printf("Failed to write file %s\n", infoName)
		//			}

		//			err = file.Close()
		//			if err != nil {
		//				fmt.Println("Failed to close info file. ", err)
		//			}
		//		}

		//conn.Write([]byte("\n"))
	}
}

func main() {
	err := os.MkdirAll(localRoot, 0700)
	if err != nil {
		fmt.Println("Failed to make local directory ", localRoot)
		return
	}

	err = os.MkdirAll(streamRoot, 0700)
	if err != nil {
		fmt.Println("Failed to make stream directory ", streamRoot)
		return
	}

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

		fmt.Println("Connection accepted. ", conn.RemoteAddr(), " | ", conn.LocalAddr())

		//Start go routine to handle messages on this connection
		go listenAndServe(conn)
	}
}
