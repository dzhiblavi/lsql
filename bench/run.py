#!/usr/bin/env python3

import argparse
import hashlib
import json
import os
import platform
import resource
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BENCH = ROOT / "bench"
QUERIES = BENCH / "queries"
RESULTS = BENCH / "results"


def run_command(args, cwd=ROOT, capture=False):
    if capture:
        return subprocess.run(args, cwd=cwd, check=True, text=True, capture_output=True).stdout

    subprocess.run(args, cwd=cwd, check=True)


def current_revision():
    return run_command(["git", "rev-parse", "--short=12", "HEAD"], capture=True).strip()


def is_dirty():
    result = subprocess.run(["git", "diff", "--quiet"], cwd=ROOT)
    return result.returncode != 0


def build(build_type, skip_build):
    if skip_build:
        return

    run_command(["make", "build", f"BUILD_TYPE={build_type}", "TESTS=OFF"])


def binary_for(frontend, build_type):
    name = "lsql" if frontend == "sql" else "lpipe"
    path = ROOT / "output" / f"{name}-{build_type}"
    if not path.exists():
        raise RuntimeError(f"binary not found: {path}")

    return path


def discover_queries():
    queries = []
    for query_dir in sorted(p for p in QUERIES.iterdir() if p.is_dir()):
        has_sql = (query_dir / "query.sql").exists()
        has_pipe = (query_dir / "query.pipe").exists()
        if not has_sql and not has_pipe:
            continue

        queries.append(
            {
                "name": query_dir.name,
                "source_dir": query_dir,
                "frontends": [
                    ("sql", "query.sql") if has_sql else None,
                    ("pipe", "query.pipe") if has_pipe else None,
                ],
            }
        )

    return queries


def prepare_query(query, query_work_dir):
    shutil.copytree(query["source_dir"], query_work_dir)
    prepare = query_work_dir / "prepare.py"
    if prepare.exists():
        run_command([sys.executable, str(prepare)], cwd=query_work_dir)


def timed_run(command, cwd):
    started = time.perf_counter()
    with tempfile.TemporaryFile() as stdout, tempfile.TemporaryFile() as stderr:
        proc = subprocess.Popen(command, cwd=cwd, stdout=stdout, stderr=stderr)
        _, status, usage = os.wait4(proc.pid, 0)
        returncode = os.waitstatus_to_exitcode(status)
        stdout.seek(0)
        stderr.seek(0)
        stdout_bytes = stdout.read()
        stderr_bytes = stderr.read()

    elapsed = time.perf_counter() - started
    max_rss_kb = usage.ru_maxrss / 1024.0 if platform.system() == "Darwin" else usage.ru_maxrss

    return {
        "returncode": returncode,
        "stdout": stdout_bytes,
        "stderr": stderr_bytes,
        "real_ms": elapsed * 1000.0,
        "user_ms": usage.ru_utime * 1000.0,
        "sys_ms": usage.ru_stime * 1000.0,
        "max_rss_kb": max_rss_kb,
    }


def ensure_success(query, frontend, result):
    if result["returncode"] == 0:
        return

    raise RuntimeError(
        f"{query['name']} ({frontend}) failed with exit code {result['returncode']}\n"
        f"stdout:\n{result['stdout'].decode(errors='replace')}\n"
        f"stderr:\n{result['stderr'].decode(errors='replace')}"
    )


def summarize(samples):
    def median(key):
        return statistics.median(sample[key] for sample in samples)

    return {
        "real_ms_median": median("real_ms"),
        "user_ms_median": median("user_ms"),
        "sys_ms_median": median("sys_ms"),
        "real_ms_min": min(sample["real_ms"] for sample in samples),
        "real_ms_max": max(sample["real_ms"] for sample in samples),
    }


def cleanup_profile_artifacts(query_work_dir):
    for path in query_work_dir.glob("prof.*.folded"):
        path.unlink()

    for path in [query_work_dir / "prof.dot"]:
        if path.exists():
            path.unlink()


def collect_profile_artifacts(query_work_dir, profile_dir, result):
    profile_dir.mkdir(parents=True, exist_ok=True)

    (profile_dir / "stdout.json").write_bytes(result["stdout"])
    (profile_dir / "stderr.txt").write_bytes(result["stderr"])

    for path in query_work_dir.glob("prof.*.folded"):
        shutil.move(str(path), profile_dir / path.name)

    dot = query_work_dir / "prof.dot"
    if dot.exists():
        shutil.move(str(dot), profile_dir / dot.name)


def run_profile(query, frontend, command, query_work_dir, profile_dir):
    cleanup_profile_artifacts(query_work_dir)
    result = timed_run(command + ["--profile", "--dot-graph", "--flamegraph"], cwd=query_work_dir)
    ensure_success(query, frontend, result)
    collect_profile_artifacts(query_work_dir, profile_dir, result)

    return {
        "real_ms": result["real_ms"],
        "user_ms": result["user_ms"],
        "sys_ms": result["sys_ms"],
        "max_rss_kb": result["max_rss_kb"],
        "output_sha256": hashlib.sha256(result["stdout"]).hexdigest(),
        "output_bytes": len(result["stdout"]),
        "profile_dir": str(profile_dir.relative_to(RESULTS.parent)),
    }


def run_query(
    query,
    frontend,
    query_file_name,
    query_work_dir,
    build_type,
    repeat,
    warmup,
    dump_profiles,
    result_dir,
):
    binary = binary_for(frontend, build_type)
    query_file = query_work_dir / query_file_name
    command = [str(binary), "-f", "JSON", str(query_file)]
    samples = []
    output_hash = None

    for _ in range(warmup):
        result = timed_run(command, cwd=query_work_dir)
        ensure_success(query, frontend, result)

    profile = None
    if dump_profiles:
        profile = run_profile(
            query,
            frontend,
            command,
            query_work_dir,
            result_dir / "profiles" / query["name"] / frontend,
        )
        output_hash = profile["output_sha256"]

    for i in range(repeat):
        result = timed_run(command, cwd=query_work_dir)
        ensure_success(query, frontend, result)

        sample_hash = hashlib.sha256(result["stdout"]).hexdigest()
        if output_hash is None:
            output_hash = sample_hash
        elif output_hash != sample_hash:
            raise RuntimeError(f"{query['name']} ({frontend}) produced unstable output")

        samples.append(
            {
                "iteration": i,
                "real_ms": result["real_ms"],
                "user_ms": result["user_ms"],
                "sys_ms": result["sys_ms"],
                "max_rss_kb": result["max_rss_kb"],
                "output_bytes": len(result["stdout"]),
            }
        )

    return {
        "frontend": frontend,
        "query_file": query_file_name,
        "warmup": warmup,
        "output_sha256": output_hash,
        "profile": profile,
        "summary": summarize(samples),
        "samples": samples,
    }


def write_json(path, obj):
    path.write_text(json.dumps(obj, indent=2, sort_keys=True) + "\n")


def main():
    parser = argparse.ArgumentParser(description="Run logsql benchmark queries")
    parser.add_argument("--git-tag", default=None, help="result directory tag; defaults to HEAD")
    parser.add_argument("--build-type", default="Release")
    parser.add_argument("--repeat", type=int, default=7)
    parser.add_argument("--warmup", type=int, default=0)
    parser.add_argument("--dump-profiles", action="store_true")
    parser.add_argument("--skip-build", action="store_true")
    args = parser.parse_args()

    if args.repeat <= 0:
        raise RuntimeError("--repeat should be positive")
    if args.warmup < 0:
        raise RuntimeError("--warmup should be non-negative")

    revision = current_revision()
    dirty = is_dirty()
    tag = args.git_tag or (revision + ("-dirty" if dirty else ""))
    result_dir = RESULTS / tag
    work_root = result_dir / "_work"

    result_dir.mkdir(parents=True, exist_ok=True)
    if work_root.exists():
        shutil.rmtree(work_root)
    work_root.mkdir(parents=True)

    build(args.build_type, args.skip_build)

    meta = {
        "git_revision": revision,
        "git_dirty": dirty,
        "git_tag": tag,
        "build_type": args.build_type,
        "repeat": args.repeat,
        "warmup": args.warmup,
        "dump_profiles": args.dump_profiles,
        "started_at": datetime.now(timezone.utc).isoformat(),
        "platform": platform.platform(),
        "machine": platform.machine(),
        "python": sys.version,
    }
    write_json(result_dir / "meta.json", meta)

    for query in discover_queries():
        query_work_dir = work_root / query["name"]
        prepare_query(query, query_work_dir)

        frontend_results = []
        for item in query["frontends"]:
            if item is None:
                continue

            frontend, query_file_name = item
            frontend_results.append(
                run_query(
                    query,
                    frontend,
                    query_file_name,
                    query_work_dir,
                    args.build_type,
                    args.repeat,
                    args.warmup,
                    args.dump_profiles,
                    result_dir,
                )
            )

        write_json(
            result_dir / f"{query['name']}.json",
            {
                "name": query["name"],
                "meta": meta,
                "results": frontend_results,
            },
        )

    print(f"wrote benchmark results to {result_dir}")


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"error: {e}", file=sys.stderr)
        sys.exit(1)
