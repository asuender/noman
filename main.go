package main

import (
	"flag"
	"fmt"
	"io/fs"
	"log"
	"os"
	"path"
)

const (
	default_sub_dir = ".noman"
	path_usage      = "Path where to look for the specified note"
)

var (
	default_path string
	_path        string
)

func init() {
	home_dir, err := os.UserHomeDir()
	CheckErrWithMsg(err, "The home directory could not be accessed. Please make sure it can be accessed as the default notes directory lies there.")

	default_path = path.Join(home_dir, default_sub_dir)

	flag.StringVar(&_path, "path", default_path, path_usage+".")
	flag.StringVar(&_path, "p", default_path, path_usage+" (shorthand).")

	flag.Usage = func() {
		fmt.Fprintln(os.Stderr, "Flags:")
		flag.PrintDefaults()
	}

	log.SetFlags(0x0)
}

func main() {
	flag.Parse()

	var args = flag.Args()

	if len(args) == 0 {
		fmt.Fprintln(os.Stderr, "Please provide a note you would like to search for.")
		os.Exit(1)
	} else if len(args) > 1 {
		log.Print("Note: you passed more than one note; noman will only consider the first one.")
	}

	if _, err := os.Stat(_path); err != nil {
		if os.IsNotExist(err) {
			switch {
			case _path == default_path:
				log.Fatalf("The default notes directory ('%s') does not exist. Please create it first.", default_path)
			default:
				log.Fatal("The specified notes directory does not exist.")
			}
			os.Exit(1)
		}
	}

	var note = args[0]
	var root = os.DirFS(default_path)
	_files, err := fs.Glob(root, fmt.Sprintf("%s.md", note))
	CheckErr(err)

	if len(_files) == 0 {
		log.Fatalf("Could not find note '%s'.", note)
	}

	dat, err := os.ReadFile(path.Join(_path, _files[0]))
	fmt.Print(string(dat))
}

func CheckErr(err error) {
	if err != nil {
		log.Fatal(err)
		os.Exit(1)
	}
}

func CheckErrWithMsg(err error, message string) {
	if err != nil {
		log.Fatal(message)
		os.Exit(1)
	}
}
