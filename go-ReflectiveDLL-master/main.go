package main

import "C"

import (
	"fmt"
	"os"
	gsq "github.com/kballard/go-shellquote"
)



//export test
func test(arg string) {

	args, err := gsq.Split(arg)
	if err == nil {
		fmt.Println("Golang ReflectiveDLL")
		os.Args = args
		fmt.Printf("Args Count %d\n",len(os.Args))
		for i := 0; i < len(os.Args); i++ {
			fmt.Printf("[%d] %s\n",i,os.Args[i])
		}
	}
}

func main()  {

}
