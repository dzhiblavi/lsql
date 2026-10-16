import argparse
from collections import Counter
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile

from producer import record, row_count, SOURCES


FIELDS = ("shard", "id", "key", "payload")
THREADS = 16
TIMEOUT = 120


def expected_rows(profile):
    for source in range(SOURCES):
        for index in range(row_count(source, profile)):
            value = record(source, index)
            yield tuple(value[field] for field in FIELDS)


def check_output(path, profile, order, limits):
    expected = Counter(expected_rows(profile))
    total = sum(expected.values())
    merge = order != "unordered"
    descending = order.endswith("desc")
    key_columns = (2, 1) if order.startswith("key-id") else (2,)

    def sort_key(row):
        return tuple(row[column] for column in key_columns)

    consumers = {consumer: [] for consumer in limits}
    with path.open() as output:
        for line_number, line in enumerate(output, 1):
            if not line.strip():
                continue
            value = json.loads(line)
            if set(value) != set(FIELDS) | {"consumer"}:
                raise AssertionError(
                    f"line {line_number}: unexpected fields: {list(value)}"
                )
            consumer = value["consumer"]
            if consumer not in consumers:
                raise AssertionError(
                    f"line {line_number}: unexpected consumer: {consumer}"
                )
            consumers[consumer].append(tuple(value[field] for field in FIELDS))

    for consumer, rows in consumers.items():
        actual = Counter(rows)
        limit = limits[consumer]
        if limit is None:
            missing = expected - actual
            extra = actual - expected
            if missing or extra:
                raise AssertionError(
                    f"{consumer}: missing {sum(missing.values())}, extra {sum(extra.values())}; "
                    f"example IDs: missing {[row[1] for row in list(missing)[:3]]}, "
                    f"extra {[row[1] for row in list(extra)[:3]]}"
                )
        else:
            if len(rows) != min(limit, total):
                raise AssertionError(
                    f"{consumer}: expected {min(limit, total)} rows, got {len(rows)}"
                )
            if actual - expected:
                raise AssertionError(f"{consumer}: unexpected or duplicate rows")
            if merge:
                prefix = sorted(
                    expected.elements(),
                    key=sort_key,
                    reverse=descending,
                )[:limit]
                if rows != prefix:
                    raise AssertionError(f"{consumer}: incorrect sorted prefix")
        if merge:
            keys = [sort_key(row) for row in rows]
            if keys != sorted(keys, reverse=descending):
                raise AssertionError(f"{consumer}: merge output is out of order")


def run_case(cli, query, profile, seed, order, limits):
    source_directory = Path(__file__).resolve().parent
    parameters = dict(
        query=str(query),
        seed=seed,
        profile=profile,
        order=order,
        consumers=limits,
        threads=THREADS,
        timeout=TIMEOUT,
    )
    directory = Path(
        tempfile.mkdtemp(
            prefix=f"{query.stem}-{profile}-{seed}-", dir=Path.cwd()
        )
    )
    process = None
    try:
        shutil.copyfile(query, directory / "query.sql")
        shutil.copyfile(source_directory / "producer.py", directory / "producer.py")
        (directory / "parameters.json").write_text(
            json.dumps(parameters, indent=2) + "\n"
        )
        environment = dict(
            os.environ, LSQL_HARDENING_SEED=str(seed), LSQL_HARDENING_PROFILE=profile
        )
        with (directory / "stdout.jsonl").open("w") as stdout, (
            directory / "stderr.txt"
        ).open("w") as stderr:
            process = subprocess.Popen(
                [cli, "query.sql", "-f", "JSON", "-j", str(THREADS)],
                cwd=directory,
                env=environment,
                stdout=stdout,
                stderr=stderr,
                start_new_session=True,
            )
            process.wait(timeout=TIMEOUT)
        if process.returncode != 0:
            raise AssertionError(f"CLI exited with code {process.returncode}")
        try:
            os.killpg(process.pid, 0)
        except ProcessLookupError:
            pass
        else:
            raise AssertionError("STREAM processes remain after CLI exit")
        check_output(directory / "stdout.jsonl", profile, order, limits)
    except Exception as error:
        raise RuntimeError(f"{error}; artifacts: {directory}") from error
    finally:
        if process is not None:
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            process.wait()
    shutil.rmtree(directory)
    print(f"passed: {query=}, {seed=}, {profile=}", flush=True)


def parse_consumer(value):
    name, separator, limit = value.partition("=")
    if not name:
        raise argparse.ArgumentTypeError("consumer name must not be empty")
    if not separator:
        return name, None
    try:
        count = int(limit)
    except ValueError:
        raise argparse.ArgumentTypeError("consumer limit must be an integer")
    if count < 0:
        raise argparse.ArgumentTypeError("consumer limit must not be negative")
    return name, count


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cli", required=True)
    parser.add_argument("--query", type=Path, required=True)
    parser.add_argument("--profile", choices=("balanced", "uneven", "empty"), required=True)
    parser.add_argument("--seed", type=int, required=True)
    parser.add_argument(
        "--order",
        choices=("unordered", "key-desc", "key-id-asc", "key-id-desc"),
        required=True,
    )
    parser.add_argument("--consumer", type=parse_consumer, action="append", required=True)
    args = parser.parse_args()
    cli = str(Path(args.cli).resolve())
    run_case(
        cli, args.query.resolve(), args.profile, args.seed, args.order, dict(args.consumer)
    )


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(error, file=sys.stderr)
        sys.exit(1)
