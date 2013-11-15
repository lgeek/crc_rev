echo "Running tests..."

../crc_rev 16 random1 0x5257 random2 0xc18b random3 0xabe6 random4 0x5c37 |grep Found | cmp -s - o1
if [ $? != 0 ]; then echo "Test 1 failed"; exit; fi

../crc_rev 16 random1 0xeffc random2 0xff5 random3 0xefb1 random4 0x1f3b |grep Found | cmp -s - o2
if [ $? != 0 ]; then echo "Test 2 failed"; exit; fi

../crc_rev 16 random1 0x3ebb random2 0xc768 random3 0xed1f random4 0xbd53 |grep Found | cmp -s - o3
if [ $? != 0 ]; then echo "Test 3 failed"; exit; fi

../crc_rev 16 random1 0x85d5 random2 0x4609 random3 0xf43e random4 0x3772 |grep Found | cmp -s - o4
if [ $? != 0 ]; then echo "Test 4 failed"; exit; fi

echo "Completed successfully"
