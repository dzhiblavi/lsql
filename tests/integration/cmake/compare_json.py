import json
import sys


def try_unmarshal(s):
    try:
        return json.loads(s)
    except Exception as e:
        print(f"failed to unmarshal line '{s}': {str(e)}")
        sys.exit(1)


def read_block_json_file(f):
    blocks = []
    curr_block = []

    for line in f:
        line = line.strip()
        if not line:
            if curr_block:
                blocks.append(curr_block)
                curr_block = []
            continue
        curr_block.append(try_unmarshal(line))

    if curr_block:
        blocks.append(curr_block)

    return blocks


with open(sys.argv[1], "r") as f:
    expected_blocks = read_block_json_file(f)
actual_blocks = read_block_json_file(sys.stdin)


if len(actual_blocks) != len(expected_blocks):
    print(
        f"Different number of blocks: actual={len(actual_blocks)}, expected={len(expected_blocks)}"
    )


def sort_key(block):
    sorted_block_strings = sorted(
        [json.dumps(item, sort_keys=True, separators=(",", ":")) for item in block],
    )
    return ":".join(sorted_block_strings)


def dump(blocks, path):
    with open(path, "w") as file:
        for i, block in enumerate(blocks):
            for line in block:
                file.write(json.dumps(line))
                file.write('\n')
            file.write('\n')


actual_blocks = sorted(actual_blocks, key=sort_key)
expected_blocks = sorted(expected_blocks, key=sort_key)


for b, (ab, eb) in enumerate(zip(actual_blocks, expected_blocks)):
    for a, e in zip(ab, eb):
        if a != e:
            print(f"Lines differ block #{b}:\n\t{a}\n\t{e}")
            dump(actual_blocks, "/tmp/actual.json")
            dump(expected_blocks, "/tmp/expected.json")
            sys.exit(1)


sys.exit(0)
