#pragma once

//#define DSMSW 0x44534D5357ui64
//#define DSMSW_SIZE 5
#define DSMSW 0x44534D53ui64
#define DSMSW_SIZE 4

enum dsmp_t {DSMP_FILE, DSMP_STREAMINFO, DSMP_MEDIATYPE, DSMP_CHAPTERS, DSMP_SAMPLE, DSMP_SYNCPOINTS};

/*

-----------------------
The .dsm file structure
-----------------------

File + MediaType*N [+ StreamInfo + Chapters] + Sample*M [+ SyncPoints]

Notes: 
- SyncPoints is optional, seeking can be performed by searching for packet syncpoints and their timestamps.
- This layout is fine for streaming. (TODO: introduce NewSegment packet, but _only_ for streaming)
- The resolution of timestamp and duration is 100ns.
- Strings are zero terminated utf-8 strings.

Packet
------

DSMSW (DSMSW_SIZE bytes) (DirectShow Media SyncWord)
enum dsmp_t {DSMP_FILE, DSMP_STREAMINFO, DSMP_MEDIATYPE, DSMP_CHAPTERS, DSMP_SAMPLE} (5 bits)
data size length (3 bits, 1-8 bytes)
data size (1-8 bytes)

[... data ...]

File : extends Packet (dsmp_t: DSMP_FILE)
-----------------------------------------

... repeated n times ...

id (4cc, 4 bytes)
string

... repeated n times ...

Notes:
- Possible values of "id": 
	'TITL': Title
	'AUTH': Author
	'COMM': Comment
	... more to be defined ...
	'NMSP': Namespace (the following ids are part of this namespace and should be interpreted only if the parser knows about it, nested namespaces are not supported)

MediaType : extends Packet (dsmp_t: DSMP_MEDIATYPE)
---------------------------------------------------

stream id (1 byte)

majortype (sizeof(GUID))
subtype (sizeof(GUID))
bFixedSizeSamples (1 bit)
bTemporalCompression (1 bit)
lSampleSize (30 bit)
formattype (sizeof(GUID))

[... format data ...]

StreamInfo : extends Packet (dsmp_t: DSMP_STREAMINFO)
-----------------------------------------------------

stream id (1 byte)

... repeated n times ...

id (4cc, 4 bytes)
string

... repeated n times ...

Notes:
- Possible values of "id": 
	'SGRP': Stream Group (groupped streams can be useful if the splitter is able to group and switch between them, but it's not a strict requirement towards dsm splitters)
	'LANG': Language code (ISO 639-2)
	'COMM': Comment
	... more to be defined ...
	'NMSP': Namespace (the following ids are part of this namespace and should be interpreted only if the parser knows about it, nested namespaces are not supported)

Chapters : extends Packet (dsmp_t: DSMP_CHAPTERS)
------------------------------------------------

... repeated n times ...

timestamp length (3 bits)
reserved (5 bits)
timestamp (0-7 bytes)
string

... repeated n times ...

Sample : extends Packet (dsmp_t: DSMP_SAMPLE)
---------------------------------------------

stream id (1 byte)

syncpoint (1 bit)
timestamp sign (1 bit, <0?)
timestamp length (3 bits -> 0-7 bytes)
duration length (3 bits -> 0-7 bytes)

timestamp (0-7 bytes)
duration (0-7 bytes)

[... data ...]

Notes:
- sign == 1 && timestamps length == 0 -> timestamp and duration is unknown (but for syncpoints it cannot be unknown!)
- sign == 0 && ... length == 0 -> simply means the value is stored on zero bytes and its value is zero too.
- syncpoint timestamps must be strictly in increasing order.

SyncPoints : extends Packet (dsmp_t: DSMP_SYNCPOINTS)
-----------------------------------------------------

... repeated n times ...

timestamp delta sign (1 bit, <0?)
timestamp delta length (3 bits -> 0-7 bytes)
file position delta length (3 bits -> 0-7 bytes)
reserved (1 bits)

timestamp delta (0-7 bytes)
file position delta (0-7 bytes)

... repeated n times ...

Notes: 
- "timestamp delta" / "file position delta" holds the difference to the previous value, both start at 0.

The algorithm of generating SyncPoints
--------------------------------------

stream 1: 1,5,8 (video)
stream 2: 2,3,6,7,9 (audio)
stream 3: 4 (subtitle)

1 ----|              1->2:  1      +2        -> 1 (t 1, fp 1)
      |---- 2        2->3:  1,2    +3 -2     -> 1 
      |---- 3        3->4:  1,3    +4        -> 1
	+-|-- 4 (start)  4->5:  1,3,4  +5 -1     -> 1
5 --|-|              5->6:  3,4,5  +6 -3     -> 3 (t 5, fp 3)
    | |---- 6        6->7:  4,5,6  +7 -6     -> 4 (t 6, fp 4)
	| |---- 7        7->8:  4,5,7  +8 -7 -4  -> 4
	+-|-- 4 (stop)    
      |---- 8        8->9:  5,8    +9 -5     -> 5 (t 8, fp 5)
9 ----|              9->10: 8,9   +10 -8     -> 8 (t 9, fp 8)
      |---- 10       10->:  9,10             -> 9 (t 10, fp 9)

Notice how it is the values of the first and last elements of the queue are used.

In the end it represents the following: (timestamp ranges mapped to file positions)

1->5:  [1]
5->6:  [3]
6->8:  [4]
8->9:  [5]
9->10: [8]
10->:  [9]

Example usage: 

Seeking to 7 would mean we need to start decoding at the file position of 4, which 
makes sure we hit at least one syncpoint from every stream (4,5,6) until we reach 7.

*/
