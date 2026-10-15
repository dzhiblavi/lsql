s00 = STREAM 'exec python3 producer.py --source 0'
s01 = STREAM 'exec python3 producer.py --source 1'
s02 = STREAM 'exec python3 producer.py --source 2'
s03 = STREAM 'exec python3 producer.py --source 3'
s04 = STREAM 'exec python3 producer.py --source 4'
s05 = STREAM 'exec python3 producer.py --source 5'
s06 = STREAM 'exec python3 producer.py --source 6'
s07 = STREAM 'exec python3 producer.py --source 7'
s08 = STREAM 'exec python3 producer.py --source 8'
s09 = STREAM 'exec python3 producer.py --source 9'
s10 = STREAM 'exec python3 producer.py --source 10'
s11 = STREAM 'exec python3 producer.py --source 11'
s12 = STREAM 'exec python3 producer.py --source 12'
s13 = STREAM 'exec python3 producer.py --source 13'
s14 = STREAM 'exec python3 producer.py --source 14'
s15 = STREAM 'exec python3 producer.py --source 15'
s16 = STREAM 'exec python3 producer.py --source 16'
s17 = STREAM 'exec python3 producer.py --source 17'
s18 = STREAM 'exec python3 producer.py --source 18'
s19 = STREAM 'exec python3 producer.py --source 19'
s20 = STREAM 'exec python3 producer.py --source 20'
s21 = STREAM 'exec python3 producer.py --source 21'
s22 = STREAM 'exec python3 producer.py --source 22'
s23 = STREAM 'exec python3 producer.py --source 23'
s24 = STREAM 'exec python3 producer.py --source 24'
s25 = STREAM 'exec python3 producer.py --source 25'
s26 = STREAM 'exec python3 producer.py --source 26'
s27 = STREAM 'exec python3 producer.py --source 27'
s28 = STREAM 'exec python3 producer.py --source 28'
s29 = STREAM 'exec python3 producer.py --source 29'
s30 = STREAM 'exec python3 producer.py --source 30'
s31 = STREAM 'exec python3 producer.py --source 31'

n00 = ($s00 UNION ALL $s01)
n01 = ($s02 UNION ALL $s03)
n02 = ($s04 UNION ALL $s05)
n03 = ($s06 UNION ALL $s07)
n04 = ($s08 UNION ALL $s09)
n05 = ($s10 UNION ALL $s11)
n06 = ($s12 UNION ALL $s13)
n07 = ($s14 UNION ALL $s15)
n08 = ($s16 UNION ALL $s17)
n09 = ($s18 UNION ALL $s19)
n10 = ($s20 UNION ALL $s21)
n11 = ($s22 UNION ALL $s23)
n12 = ($s24 UNION ALL $s25)
n13 = ($s26 UNION ALL $s27)
n14 = ($s28 UNION ALL $s29)
n15 = ($s30 UNION ALL $s31)

n16 = ($n00 UNION ALL $n01)
n17 = ($n02 UNION ALL $n03)
n18 = ($n04 UNION ALL $n05)
n19 = ($n06 UNION ALL $n07)
n20 = ($n08 UNION ALL $n09)
n21 = ($n10 UNION ALL $n11)
n22 = ($n12 UNION ALL $n13)
n23 = ($n14 UNION ALL $n15)

n24 = ($n16 UNION ALL $n17)
n25 = ($n18 UNION ALL $n19)
n26 = ($n20 UNION ALL $n21)
n27 = ($n22 UNION ALL $n23)

n28 = ($n24 UNION ALL $n25)
n29 = ($n26 UNION ALL $n27)

n30 = ($n28 UNION ALL $n29)


SELECT shard, id, key, payload, 'full' AS consumer
FROM $n30
