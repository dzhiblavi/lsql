import argparse
import os
import sys
import time


ROWS = 2048
SOURCES = 32


def row_count(source, profile):
    if profile == "empty":
        return 0
    if profile == "balanced":
        return ROWS
    if source % 8 == 0:
        return 0
    if source % 8 == 1:
        return 1
    return ROWS * (1 + source % 3)


def record(source, index):
    return {
        "shard": f"{source:02d}",
        "id": f"{source:02d}{index:08d}",
        "key": f"{index // 3:08d}",
        "payload": f"s{source:02d}r{index:08d}_" + "abcdefgh01234567" * 16,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=int, required=True)
    parser.add_argument("--seed", type=int, default=int(os.environ.get("LSQL_HARDENING_SEED", "0")))
    parser.add_argument("--profile", choices=("balanced", "uneven", "empty"),
                        default=os.environ.get("LSQL_HARDENING_PROFILE", "balanced"))
    parser.add_argument("--rows", type=int)
    parser.add_argument("--batch", type=int)
    parser.add_argument("--pause-ms", type=int)
    parser.add_argument("--start-ms", type=int)
    parser.add_argument("--eof-ms", type=int)
    parser.add_argument("--descending", action="store_true")
    args = parser.parse_args()

    schedule = args.source * 17 + args.seed * 31
    rows = args.rows if args.rows is not None else row_count(args.source, args.profile)
    batch = args.batch if args.batch is not None else (1, 7, 64, 257)[schedule % 4]
    pause = args.pause_ms if args.pause_ms is not None else int(schedule % 7 == 0)
    start = args.start_ms if args.start_ms is not None else schedule % 5
    eof = args.eof_ms if args.eof_ms is not None else schedule % 11
    time.sleep(start / 1000)
    indices = range(rows - 1, -1, -1) if args.descending else range(rows)
    for offset, index in enumerate(indices, 1):
        fields = " ".join(f"{name}={value}" for name, value in record(args.source, index).items())
        sys.stdout.write(f"[2026-Oct-15 00:00:00.000000] {fields}\n")
        if offset % batch == 0:
            sys.stdout.flush()
            if pause:
                time.sleep(pause / 1000)
    sys.stdout.flush()
    time.sleep(eof / 1000)


if __name__ == "__main__":
    try:
        main()
    except BrokenPipeError:
        sys.stdout = open("/dev/null", "w")
