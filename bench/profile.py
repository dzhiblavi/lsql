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
    parser.add_argument("--build-type", default="Release")
    parser.add_argument("--warmup", type=int, default=1)
    parser.add_argument("--git-tag", default=None)
    parser.add_argument("--skip-build", action="store_true")
    args = parser.parse_args()

    command = [
        sys.executable,
        str(RUN),
        "--query",
        args.query,
        "--build-type",
        args.build_type,
        "--repeat",
        "1",
        "--warmup",
        str(args.warmup),
        "--dump-profiles",
    ]
    if args.git_tag:
        command += ["--git-tag", args.git_tag]
    if args.skip_build:
        command += ["--skip-build"]

    subprocess.run(command, cwd=ROOT, check=True)


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"error: {e}", file=sys.stderr)
        sys.exit(1)
