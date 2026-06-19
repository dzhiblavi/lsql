#!/usr/bin/env python3

import argparse
import hashlib
import json
import os
import platform
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


def discover_queries(names=None):
    names = set(names or [])
    queries = []
    for query_dir in sorted(p for p in QUERIES.iterdir() if p.is_dir()):
        if names and query_dir.name not in names:
            continue

        has_sql = (query_dir / "query.sql").exists()
        has_pipe = (query_dir / "query.pipe").exists()
        if not has_sql and not has_pipe:
            continue

        queries.append(
            {
                "name": query_dir.name,
                "source_dir": query_dir,
                "metadata": query_metadata(query_dir),
                "frontends": [
                    ("sql", "query.sql") if has_sql else None,
                    ("pipe", "query.pipe") if has_pipe else None,
                ],
            }
        )

    return queries


def query_metadata(query_dir):
    path = query_dir / "meta.json"
    if not path.exists():
        return {}

    return json.loads(path.read_text())


def prepare_query(query, query_work_dir):
    shutil.copytree(query["source_dir"], query_work_dir)
    prepare = query_work_dir / "prepare.py"
    if prepare.exists():
        env = os.environ.copy()
        env["PYTHONPATH"] = (
            str(BENCH)
            if not env.get("PYTHONPATH")
            else str(BENCH) + os.pathsep + env["PYTHONPATH"]
        )
        subprocess.run(
            [sys.executable, str(prepare)], cwd=query_work_dir, env=env, check=True
        )


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
    max_rss_kb = (
        usage.ru_maxrss / 1024.0 if platform.system() == "Darwin" else usage.ru_maxrss
    )

    return {
        "returncode": returncode,
        "stdout": stdout_bytes,
        "stderr": stderr_bytes,
        "real_ms": elapsed * 1000.0,
        "user_ms": usage.ru_utime * 1000.0,
        "sys_ms": usage.ru_stime * 1000.0,
        "max_rss_kb": max_rss_kb,
    }


def output_semantic_hash(stdout):
    try:
        rows = []
        for line in stdout.splitlines():
            if not line:
                continue
            rows.append(json.loads(line))

        normalized = b"\n".join(
            json.dumps(row, sort_keys=True, separators=(",", ":")).encode()
            for row in rows
        )
        return hashlib.sha256(normalized).hexdigest()
    except json.JSONDecodeError:
        return hashlib.sha256(stdout).hexdigest()


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


def check_output_stability(
    query, frontend, result, output_hash, output_semantic_hash_value
):
    sample_hash = hashlib.sha256(result["stdout"]).hexdigest()
    sample_semantic_hash = output_semantic_hash(result["stdout"])
    if output_hash is None:
        return sample_hash, sample_semantic_hash
    if output_hash != sample_hash:
        raise RuntimeError(f"{query['name']} ({frontend}) produced unstable output")
    return output_hash, output_semantic_hash_value


def append_sample(samples, result):
    samples.append(
        {
            "iteration": len(samples),
            "real_ms": result["real_ms"],
            "user_ms": result["user_ms"],
            "sys_ms": result["sys_ms"],
            "max_rss_kb": result["max_rss_kb"],
            "output_bytes": len(result["stdout"]),
        }
    )


def auto_repeat_count(pilot_ms, time_limit_s):
    if pilot_ms <= 0:
        return 1

    return max(1, int(time_limit_s * 1000.0 / pilot_ms))


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
    result = timed_run(
        command + ["--profile", "--dot-graph", "--flamegraph"], cwd=query_work_dir
    )
    ensure_success(query, frontend, result)
    collect_profile_artifacts(query_work_dir, profile_dir, result)

    return {
        "real_ms": result["real_ms"],
        "user_ms": result["user_ms"],
        "sys_ms": result["sys_ms"],
        "max_rss_kb": result["max_rss_kb"],
        "output_sha256": hashlib.sha256(result["stdout"]).hexdigest(),
        "output_semantic_sha256": output_semantic_hash(result["stdout"]),
        "output_bytes": len(result["stdout"]),
        "profile_dir": str(profile_dir.relative_to(RESULTS.parent)),
    }


def run_query(
    query,
    frontend,
    query_file_name,
    query_work_dir,
    binaries,
    repeat,
    warmup,
    time_limit_s,
    dump_profiles,
    result_dir,
):
    binary = binaries.get(frontend)
    if binary is None:
        raise RuntimeError(f"no binary provided for {frontend} frontend")

    query_file = query_work_dir / query_file_name
    command = [str(binary), "-f", "JSON", str(query_file)]
    samples = []
    output_hash = None
    output_semantic_hash_value = None
    pilot_ms = None

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
        output_semantic_hash_value = profile["output_semantic_sha256"]

    if repeat is None:
        result = timed_run(command, cwd=query_work_dir)
        ensure_success(query, frontend, result)
        output_hash, output_semantic_hash_value = check_output_stability(
            query, frontend, result, output_hash, output_semantic_hash_value
        )
        append_sample(samples, result)
        pilot_ms = result["real_ms"]
        repeat = auto_repeat_count(pilot_ms, time_limit_s)

    while len(samples) < repeat:
        result = timed_run(command, cwd=query_work_dir)
        ensure_success(query, frontend, result)
        output_hash, output_semantic_hash_value = check_output_stability(
            query, frontend, result, output_hash, output_semantic_hash_value
        )
        append_sample(samples, result)

    return {
        "frontend": frontend,
        "query_file": query_file_name,
        "warmup": warmup,
        "repeat": repeat,
        "auto_repeat": pilot_ms is not None,
        "time_limit_s": time_limit_s if pilot_ms is not None else None,
        "pilot_ms": pilot_ms,
        "output_sha256": output_hash,
        "output_semantic_sha256": output_semantic_hash_value,
        "profile": profile,
        "summary": summarize(samples),
        "samples": samples,
    }


def write_json(path, obj):
    path.write_text(json.dumps(obj, indent=2, sort_keys=True) + "\n")


def main():
    parser = argparse.ArgumentParser(description="Run logsql benchmark queries")
    parser.add_argument(
        "--tag",
        default=None,
        help="result directory tag; defaults to '<binary-name>-<timestamp>'",
    )
    parser.add_argument(
        "--binary",
        type=Path,
        default=None,
        help="SQL frontend binary path; shorthand for --sql-binary",
    )
    parser.add_argument(
        "--sql-binary",
        type=Path,
        default=None,
        help="lsql binary path for SQL benchmarks",
    )
    parser.add_argument(
        "--pipe-binary",
        type=Path,
        default=None,
        help="lpipe binary path for pipe benchmarks",
    )
    parser.add_argument(
        "--repeat",
        type=int,
        default=None,
        help="fixed recorded sample count; by default repeat count is estimated from --time-limit",
    )
    parser.add_argument(
        "--time-limit",
        type=float,
        default=5.0,
        help="target seconds spent on recorded samples per query when --repeat is omitted",
    )
    parser.add_argument("--warmup", type=int, default=0)
    parser.add_argument("--dump-profiles", action="store_true")
    parser.add_argument(
        "--query",
        action="append",
        default=[],
        help="benchmark query name to run; can be passed multiple times",
    )
    args = parser.parse_args()

    if args.repeat is not None and args.repeat <= 0:
        raise RuntimeError("--repeat should be positive")
    if args.time_limit <= 0:
        raise RuntimeError("--time-limit should be positive")
    if args.warmup < 0:
        raise RuntimeError("--warmup should be non-negative")

    sql_binary = args.sql_binary or args.binary
    binaries = {
        "sql": sql_binary.resolve() if sql_binary else None,
        "pipe": args.pipe_binary.resolve() if args.pipe_binary else None,
    }
    binaries = {
        frontend: path for frontend, path in binaries.items() if path is not None
    }
    if not binaries:
        raise RuntimeError(
            "at least one of --binary, --sql-binary, or --pipe-binary is required"
        )
    for frontend, path in binaries.items():
        if not path.exists():
            raise RuntimeError(f"{frontend} binary not found: {path}")
        if not path.is_file():
            raise RuntimeError(f"{frontend} binary is not a file: {path}")

    queries = discover_queries(args.query)
    if args.query and len(queries) != len(set(args.query)):
        found = {query["name"] for query in queries}
        missing = sorted(set(args.query) - found)
        raise RuntimeError(f"benchmark query not found: {', '.join(missing)}")

    missing_frontends = sorted(
        {
            frontend
            for query in queries
            for item in query["frontends"]
            if item is not None
            for frontend, _ in [item]
            if frontend not in binaries
        }
    )
    if missing_frontends:
        raise RuntimeError(
            "missing binaries for frontend(s): " + ", ".join(missing_frontends)
        )

    default_tag_binary = binaries.get("sql") or next(iter(binaries.values()))
    timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    tag = args.tag or f"{default_tag_binary.name}-{timestamp}"
    result_dir = RESULTS / tag
    work_root = result_dir / "_work"

    result_dir.mkdir(parents=True, exist_ok=True)
    if work_root.exists():
        shutil.rmtree(work_root)
    work_root.mkdir(parents=True)

    meta = {
        "tag": tag,
        "binaries": {
            frontend: str(path) for frontend, path in sorted(binaries.items())
        },
        "repeat": args.repeat,
        "auto_repeat": args.repeat is None,
        "time_limit_s": args.time_limit if args.repeat is None else None,
        "warmup": args.warmup,
        "dump_profiles": args.dump_profiles,
        "started_at": datetime.now(timezone.utc).isoformat(),
        "platform": platform.platform(),
        "machine": platform.machine(),
        "python": sys.version,
    }
    write_json(result_dir / "meta.json", meta)

    for idx, query in enumerate(queries):
        name = query["name"]
        print(f"running query #{idx}: '{name}'")
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
                    binaries,
                    args.repeat,
                    args.warmup,
                    args.time_limit,
                    args.dump_profiles,
                    result_dir,
                )
            )

        write_json(
            result_dir / f"{query['name']}.json",
            {
                "name": query["name"],
                "metadata": query["metadata"],
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
