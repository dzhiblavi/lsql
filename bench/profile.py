#!/usr/bin/env python3

import argparse
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RUN = ROOT / "bench" / "run.py"


def main():
    parser = argparse.ArgumentParser(description="Run one benchmark with profile artifacts")
    parser.add_argument("query", help="benchmark query name")
    parser.add_argument("--binary", required=True, help="SQL frontend binary path")
    parser.add_argument("--pipe-binary", default=None, help="pipe frontend binary path")
    parser.add_argument("--warmup", type=int, default=1)
    parser.add_argument("--tag", default=None)
    args = parser.parse_args()

    command = [
        sys.executable,
        str(RUN),
        "--query",
        args.query,
        "--binary",
        args.binary,
        "--repeat",
        "1",
        "--warmup",
        str(args.warmup),
        "--dump-profiles",
    ]
    if args.pipe_binary:
        command += ["--pipe-binary", args.pipe_binary]
    if args.tag:
        command += ["--tag", args.tag]

    subprocess.run(command, cwd=ROOT, check=True)


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"error: {e}", file=sys.stderr)
        sys.exit(1)
