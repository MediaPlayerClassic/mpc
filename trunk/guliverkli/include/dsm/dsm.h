/* 
 *	Copyright (C) 2003-2004 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#define DSMSW 0x44534D53ui64
#define DSMSW_SIZE 4

enum dsmp_t {DSMP_FILEINFO, DSMP_STREAMINFO, DSMP_MEDIATYPE, DSMP_CHAPTERS, DSMP_SAMPLE, DSMP_SYNCPOINTS};

/*

-----------------------
The .dsm file structure
-----------------------

FileInfo + Header Packets + Samples + Footer Packets

Header & Footer Packets:
- MediaType: required
- StreamInfo: optional
- Chapters: optional
- SyncPoints: optional

Notes: 
- SyncPoints is optional because seeking can be performed simply by searching for packet syncpoints and their timestamps.
- This layout is fine for streaming. On connection send everything up to Sample packets, then the rest. (TODO: introduce NewSegment packet, but _only_ for streaming, to support seeking)
- The resolution of timestamp and duration is 100ns.
- Strings are zero terminated utf-8 strings.

Packet
------

DSMSW (DSMSW_SIZE bytes) (DirectShow Media SyncWord)
enum dsmp_t {DSMP_FILEINFO, DSMP_STREAMINFO, DSMP_MEDIATYPE, DSMP_CHAPTERS, DSMP_SAMPLE, DSMP_SYNCPOINTS} (5 bits)
data size length (3 bits -> 1-8 bytes)
data size (1-8 bytes)

[... data ...]

FileInfo : extends Packet (dsmp_t: DSMP_FILEINFO)
-------------------------------------------------

... repeated n times ...

id (4 bytes, alphanum)
string

... repeated n times ...

Notes:
- Suggested values of "id": 
	"TITL": Title
	"AUTH": Author
	"RTNG": Rating
	"CPYR": Copyright
	"DESC": Description
	"APPL": Application
	"MUXR": Muxer
	"DATE": Encoding date
	... more to be defined ...

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

Notes:
- bFixedSizeSamples, bTemporalCompression, lSampleSize are only there to preserve compatibility with dshow's media type structure, they aren't playing a role in anything really.

StreamInfo : extends Packet (dsmp_t: DSMP_STREAMINFO)
-----------------------------------------------------

stream id (1 byte)

... repeated n times ...

id (4 bytes, alphanum)
string

... repeated n times ...

Notes:
- Suggested values of "id": 
	"SGRP": Stream Group (groupped streams can be useful if the splitter is able to group and switch between them, but it's not a strict requirement towards dsm splitters)
	"LANG": Language code (ISO 639-2)
	"DESC": Description
	... more to be defined ...

Chapters : extends Packet (dsmp_t: DSMP_CHAPTERS)
------------------------------------------------

... repeated n times ...

timestamp delta sign (1 bit, <0?)
timestamp delta length (3 bits -> 0-7 bytes)
reserved (4 bits)
timestamp delta (0-7 bytes)
string

... repeated n times ...

Notes: 
- "timestamp delta" holds the difference to the previous value, starts at 0.

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
- sign == 1 && timestamp length == 0 -> timestamp and duration is unknown (but for syncpoints it cannot be unknown!)
- sign == 0 && timestamp length == 0 -> simply means the value is stored on zero bytes and its value is zero too.
- timestamps of syncpoints must be strictly in increasing order.

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
