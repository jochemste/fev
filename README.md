# FEV

Simple tool to log events on files and directories.

## Build

This tool is Linux only. Build instructions are fairly straightforward

```
cd src/ && make
```

## Run

To see the available options, print the usage

```
./fev -h
```

Monitor one or more files
```
./fev somefile.txt
./fev /var/log/somelog.log somefile.txt ../anotherfile.txt
./fe v/var/log/somelog.log somefile.txt ../anotherfile.txt > output.txt
```

Monitor one or more directories
```
./fev somedir
./fev ./
./fev somedir ./ anotherdir/
./fev somedir > output.txt
```

## Modify events to watch

Currently you will have to modify main.c to adjust the events you want to watch.
To reduce the amount of events to watch, simply remove flags from the
`WATCH_ME` definition, which looks like:
```
#define WATCH_ME                                                          \
  IN_OPEN | IN_CLOSE | IN_MOVE | IN_MODIFY | IN_DELETE | IN_DELETE_SELF | \
      IN_ACCESS | IN_ATTRIB | IN_CREATE | IN_MOVE_SELF
```
