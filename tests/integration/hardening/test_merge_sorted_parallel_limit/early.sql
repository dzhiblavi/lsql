s00 = STREAM 'exec python3 producer.py --source 0 --descending'
s01 = STREAM 'exec python3 producer.py --source 1 --descending'
s02 = STREAM 'exec python3 producer.py --source 2 --descending'
s03 = STREAM 'exec python3 producer.py --source 3 --descending'
s04 = STREAM 'exec python3 producer.py --source 4 --descending'
s05 = STREAM 'exec python3 producer.py --source 5 --descending'
s06 = STREAM 'exec python3 producer.py --source 6 --descending'
s07 = STREAM 'exec python3 producer.py --source 7 --descending'
s08 = STREAM 'exec python3 producer.py --source 8 --descending'
s09 = STREAM 'exec python3 producer.py --source 9 --descending'
s10 = STREAM 'exec python3 producer.py --source 10 --descending'
s11 = STREAM 'exec python3 producer.py --source 11 --descending'
s12 = STREAM 'exec python3 producer.py --source 12 --descending'
s13 = STREAM 'exec python3 producer.py --source 13 --descending'
s14 = STREAM 'exec python3 producer.py --source 14 --descending'
s15 = STREAM 'exec python3 producer.py --source 15 --descending'
s16 = STREAM 'exec python3 producer.py --source 16 --descending'
s17 = STREAM 'exec python3 producer.py --source 17 --descending'
s18 = STREAM 'exec python3 producer.py --source 18 --descending'
s19 = STREAM 'exec python3 producer.py --source 19 --descending'
s20 = STREAM 'exec python3 producer.py --source 20 --descending'
s21 = STREAM 'exec python3 producer.py --source 21 --descending'
s22 = STREAM 'exec python3 producer.py --source 22 --descending'
s23 = STREAM 'exec python3 producer.py --source 23 --descending'
s24 = STREAM 'exec python3 producer.py --source 24 --descending'
s25 = STREAM 'exec python3 producer.py --source 25 --descending'
s26 = STREAM 'exec python3 producer.py --source 26 --descending'
s27 = STREAM 'exec python3 producer.py --source 27 --descending'
s28 = STREAM 'exec python3 producer.py --source 28 --descending'
s29 = STREAM 'exec python3 producer.py --source 29 --descending'
s30 = STREAM 'exec python3 producer.py --source 30 --descending'
s31 = STREAM 'exec python3 producer.py --source 31 --descending'

n00 = ($s00 UNION ALL SORTED BY key, id DESC $s01)
n01 = ($s02 UNION ALL SORTED BY key, id DESC $s03)
n02 = ($s04 UNION ALL SORTED BY key, id DESC $s05)
n03 = ($s06 UNION ALL SORTED BY key, id DESC $s07)
n04 = ($s08 UNION ALL SORTED BY key, id DESC $s09)
n05 = ($s10 UNION ALL SORTED BY key, id DESC $s11)
n06 = ($s12 UNION ALL SORTED BY key, id DESC $s13)
n07 = ($s14 UNION ALL SORTED BY key, id DESC $s15)
n08 = ($s16 UNION ALL SORTED BY key, id DESC $s17)
n09 = ($s18 UNION ALL SORTED BY key, id DESC $s19)
n10 = ($s20 UNION ALL SORTED BY key, id DESC $s21)
n11 = ($s22 UNION ALL SORTED BY key, id DESC $s23)
n12 = ($s24 UNION ALL SORTED BY key, id DESC $s25)
n13 = ($s26 UNION ALL SORTED BY key, id DESC $s27)
n14 = ($s28 UNION ALL SORTED BY key, id DESC $s29)
n15 = ($s30 UNION ALL SORTED BY key, id DESC $s31)

n16 = ($n00 UNION ALL SORTED BY key, id DESC $n01)
n17 = ($n02 UNION ALL SORTED BY key, id DESC $n03)
n18 = ($n04 UNION ALL SORTED BY key, id DESC $n05)
n19 = ($n06 UNION ALL SORTED BY key, id DESC $n07)
n20 = ($n08 UNION ALL SORTED BY key, id DESC $n09)
n21 = ($n10 UNION ALL SORTED BY key, id DESC $n11)
n22 = ($n12 UNION ALL SORTED BY key, id DESC $n13)
n23 = ($n14 UNION ALL SORTED BY key, id DESC $n15)

n24 = ($n16 UNION ALL SORTED BY key, id DESC $n17)
n25 = ($n18 UNION ALL SORTED BY key, id DESC $n19)
n26 = ($n20 UNION ALL SORTED BY key, id DESC $n21)
n27 = ($n22 UNION ALL SORTED BY key, id DESC $n23)

n28 = ($n24 UNION ALL SORTED BY key, id DESC $n25)
n29 = ($n26 UNION ALL SORTED BY key, id DESC $n27)

n30 = ($n28 UNION ALL SORTED BY key, id DESC $n29)


SELECT shard, id, key, payload, 'limit_1' AS consumer
FROM $n30
LIMIT 1
