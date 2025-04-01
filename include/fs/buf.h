#ifndef XV6_BUF_H_
#define XV6_BUF_H_

#include "config/basic_types.h"
#include "fs/fs.h"
#include "lock/sleeplock.h"

struct buf {
    int valid; // has data been read from disk?
    int disk;  // does disk "own" buf?
    uint dev;
    uint blockno;
    struct sleeplock lock;
    uint refcnt;
    struct buf *prev; // LRU cache list
    struct buf *next;
    uchar data[BSIZE];
};

#endif
