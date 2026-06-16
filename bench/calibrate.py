#!/usr/bin/env python3

import argparse
import json
import re
import statistics
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BENCH = ROOT / "bench"
QUERIES = BENCH / "queries"
RESULTS = BENCH / "results"


@dataclass(frozen=True)
class Suggestion:
    name: str
    category: str
    current_rows: int | None
    suggested_rows: int | None
    median_ms: float
    target_ms: float
    factor: float
    prepare_path: Path | None


def load_json(path):
    return json.loads(path.read_text())


def resolve_result_dir(s):
    path = Path(s)
    if path.exists():
        return path.resolve()

    path = RESULTS / s
    if path.exists():
        return path.resolve()

    raise RuntimeError(f"result directory not found: {s}")


def median(samples, key):
    return statistics.median(sample[key] for sample in samples)


def query_metadata(name, result_data):
    metadata = result_data.get("metadata") or {}
    path = QUERIES / name / "meta.json"
    if path.exists():
        metadata = {**metadata, **load_json(path)}
    return metadata


def read_rows(name):
    path = QUERIES / name / "prepare.py"
    if not path.exists():
        return None, None

    match = re.search(r"(?m)^rows\s*=\s*([0-9_]+)\s*$", path.read_text())
    if not match:
        return None, path

    return int(match.group(1).replace("_", "")), path


def round_rows(value):
    if value < 10_000:
        step = 1_000
    elif value < 100_000:
        step = 5_000
    elif value < 1_000_000:
        step = 10_000
    else:
        step = 50_000

    return max(step, int(round(value / step)) * step)


def collect_suggestions(result_dir):
    suggestions = []
    for path in sorted(result_dir.glob("*.json")):
        if path.name == "meta.json":
            continue

        data = load_json(path)
        name = data["name"]
        metadata = query_metadata(name, data)
        target = metadata.get("target_ms")
        if not target or len(target) != 2:
            continue

        current_rows, prepare_path = read_rows(name)
        if current_rows is None:
            continue

        target_ms = (target[0] + target[1]) / 2
        result = data["results"][0]
        summary = result.get("summary", {})
        samples = result["samples"]
        median_ms = summary.get("real_ms_median", median(samples, "real_ms"))
        factor = target_ms / median_ms if median_ms else 1.0
        suggested_rows = round_rows(current_rows * factor)

        suggestions.append(
            Suggestion(
                name=name,
                category=metadata.get("category", ""),
                current_rows=current_rows,
                suggested_rows=suggested_rows,
                median_ms=median_ms,
                target_ms=target_ms,
                factor=factor,
                prepare_path=prepare_path,
            )
        )

    return suggestions


def format_int(value):
    return f"{value:_}"


def format_ms(value):
    if value >= 1000:
        return f"{value / 1000:.3f}s"
    if value >= 10:
        return f"{value:.1f}ms"
    return f"{value:.2f}ms"


def print_table(suggestions):
    rows = [["query", "category", "current", "suggested", "median", "target", "factor"]]
    for item in suggestions:
        rows.append(
            [
                item.name,
                item.category,
                format_int(item.current_rows),
                format_int(item.suggested_rows),
                format_ms(item.median_ms),
                format_ms(item.target_ms),
                f"{item.factor:.2f}x",
            ]
        )

    widths = [0] * len(rows[0])
    for row in rows:
        for i, cell in enumerate(row):
            widths[i] = max(widths[i], len(cell))

    for idx, row in enumerate(rows):
        padded = []
        for i, cell in enumerate(row):
            if i >= 2:
                padded.append(cell.rjust(widths[i]))
            else:
                padded.append(cell.ljust(widths[i]))
        print("  ".join(padded))
        if idx == 0:
            print("-" * (sum(widths) + 2 * (len(widths) - 1)))


def print_edits(suggestions):
    print()
    print("Suggested edits:")
    for item in suggestions:
        if item.prepare_path is None or item.current_rows == item.suggested_rows:
            continue
        rel = item.prepare_path.relative_to(ROOT)
        print(f"{rel}: rows = {format_int(item.suggested_rows)}")


def main():
    parser = argparse.ArgumentParser(
        description="Suggest benchmark row counts from a result directory"
    )
    parser.add_argument("result", help="result directory or tag under bench/results")
    args = parser.parse_args()

    result_dir = resolve_result_dir(args.result)
    suggestions = collect_suggestions(result_dir)
    if not suggestions:
        raise RuntimeError("no calibratable benchmark results found")

    print(f"Calibration suggestions from {result_dir}")
    print_table(suggestions)
    print_edits(suggestions)


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"error: {e}", file=sys.stderr)
        sys.exit(1)
