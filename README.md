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
---
```
psi-intersection -e element size -p path result -a path to a -b path to b -s path to buckets -n number of buckets -q queue buffer size -r read buffer size -t number of threads
```
