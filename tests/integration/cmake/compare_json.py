# compare two naked arrays of json (i.e. without '[]')

import json
import sys


def try_unmarshal(s):
    try:
        return json.loads(line)
    except Exception as e:
        print(f"failed to unmarshal line '{line}': {str(e)}")
        sys.exit(1)


actual = []


for line in sys.stdin:
    line = line.strip()
    if not line:
        continue
    actual.append(try_unmarshal(line))


expected = []
with open(sys.argv[1]) as f:
    for line in f:
        expected.append(try_unmarshal(line))


def sort_key(d):
    return sorted(d.items())


if sorted(expected, key=sort_key) == sorted(actual, key=sort_key):
    sys.exit(0)


print(f"{expected} != {actual}", file=sys.stderr)
sys.exit(1)
