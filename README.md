# Private Set Intersection (PSI)
###Intersection###
---
__Install:__
```
sudo make install
```
__Clean:__ 
```
sudo make clean
```

A memory-efficient implementation of the plain intersection protocol.

Two files with binary elements of fixed size are used to calculate their 
intersection. Output is then saved to the output file.

There is some given number of buckets, which divide hashing domain into smaller 
domains.

Input files are read in sequential order. Every element is saved into the bucket 
queue according to its hash value. After queue buffer gets full the queue buffer 
will be saved to the according bucket file. If the end of the input file is 
reached all bucket queues will be saved.

Now intersection between bucket pairs can be calculated in RAM in parallel, 
results of which are concatenated.


__Install:__ 
```
sudo make install
```
__Remove:__ 
```
sudo make remove
```

###Dependencies:
---
 * libglib2.0-dev 
 * lpsi-util
 * libssl-dev

###Usage: 
```
psi-intersection
```
* -e _element size_ Byte size of the elements
* -p _"path result"_ Path to the output file
* -a _"path to file a"_ Path to file A
* -b _"path to file b"_ Path to file B
* -s _"path to bucket folder"_ Path to the temporary folder to store buckets
* -n _number of buckets_ Number of buckets
* -q _queue buffer size_ Buffer size of the wait queues
* -r _read buffer size_ Size of the read buffer
* -t _thread number_ Thread number
* -x _"OT" or "NH"_ Lookup protocol to use. Currently two possible choises: 
Oblivious Transfer or Naive Hashing.
* -l _path to lookup file_ Path to the lookup file in NH protocol.
* -m _path to masks_ Path to the client's masks in OT protocol
* -c _path to cuckoo table_  Path to the original Cuckoo Hashing table.