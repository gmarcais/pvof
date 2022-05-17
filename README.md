## Synopsis

The `pvof` command displays the file open and the speed at which the files are read from/writtne to.

For example, to follow a given process:

```
$ pvof -p 50050
    0 /   -  :    0 /s:   -    /data/file1
 37.7G/ 37.8G:  199M/s:  < 1s  /data/file2
 14.3G/ 37.4G:  170M/s: 2.27m  /data/file3
```

For a file being read (file `file2` and `file3` in our example), the
columns are in order the amount of data read, the size of the file,
the estimated speed of reading, the ETA until the file is finished and
the path of the file.

For a file being written, the size of the ETA is a -.

`pvof` can monitor easier a process that is already running (the PID is given with `-p` as above). Alternatively, a command is passed as argument to `pvof`.

`pvof` should have a little to no impact on the performance of the process being monitored. It does not use strace/ptrace, but rather reads the information from `/proc/<pid>/fdinfo` or `/proc/<pid>/io`.

## Installation

### From the release tarball

Download the latest [release tar ball](https://github.com/gmarcais/pvof/releases/download/v0.0.6/pvof-0.0.6.tar.gz) and compile it with:

```Shell
./configure
make
make install # may need sudo
```

By default, it installs in `/usr/local`. To install in another location, use `--prefix` with configure.

### From the git tree

Use this method only if you plan to develop and improve pvof. There are more dependencies: `autoconf`, `automake` and [yaggo](https://github.com/gmarcais/yaggo). Compile with

```Shell
autoreconf -fi
./configure
make
make install # may need sudo
```

## License

This code is release under the GPL 3 license.
