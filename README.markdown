crc_rev
=======

Fast, efficient CRC bruteforcer.

Finds the parameters of a specific CRC implementation:

* poly
* reverse in
* reverse out
* xor in
* xor out

based on two or more samples (captured data + CRC).


Build
-----

    make
    

Usage
-----

    crc_rev width file1 crc1 file2 crc2 [... fileN crcN]
    
    Example: see test/test.sh.
    
    
Performance
-----------

Bruteforcing 4 CRCs based on 4 data samples of 512 bytes each:

    time make test
    [...]
    real	0m15.216s

