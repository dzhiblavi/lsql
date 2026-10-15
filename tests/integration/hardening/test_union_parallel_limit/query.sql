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
n01 = ($n00 UNION ALL $s02)
n02 = ($n01 UNION ALL $s03)
n03 = ($n02 UNION ALL $s04)
n04 = ($n03 UNION ALL $s05)
n05 = ($n04 UNION ALL $s06)
n06 = ($n05 UNION ALL $s07)
n07 = ($n06 UNION ALL $s08)
n08 = ($n07 UNION ALL $s09)
n09 = ($n08 UNION ALL $s10)
n10 = ($n09 UNION ALL $s11)
n11 = ($n10 UNION ALL $s12)
n12 = ($n11 UNION ALL $s13)
n13 = ($n12 UNION ALL $s14)
n14 = ($n13 UNION ALL $s15)
n15 = ($n14 UNION ALL $s16)
n16 = ($n15 UNION ALL $s17)
n17 = ($n16 UNION ALL $s18)
n18 = ($n17 UNION ALL $s19)
n19 = ($n18 UNION ALL $s20)
n20 = ($n19 UNION ALL $s21)
n21 = ($n20 UNION ALL $s22)
n22 = ($n21 UNION ALL $s23)
n23 = ($n22 UNION ALL $s24)
n24 = ($n23 UNION ALL $s25)
n25 = ($n24 UNION ALL $s26)
n26 = ($n25 UNION ALL $s27)
n27 = ($n26 UNION ALL $s28)
n28 = ($n27 UNION ALL $s29)
n29 = ($n28 UNION ALL $s30)
n30 = ($n29 UNION ALL $s31)

SELECT shard, id, key, payload, 'limit_1' AS consumer
FROM $n30
LIMIT 1

SELECT shard, id, key, payload, 'limit_17' AS consumer
FROM $n30
LIMIT 17

SELECT shard, id, key, payload, 'limit_1000' AS consumer
FROM $n30
LIMIT 1000

SELECT shard, id, key, payload, 'full' AS consumer
FROM $n30
