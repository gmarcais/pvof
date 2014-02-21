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


## License

This code is release under the GPL 3 license.
