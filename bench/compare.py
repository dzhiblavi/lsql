#!/usr/bin/env python3

import argparse
import json
import math
import statistics
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BENCH = ROOT / "bench"
RESULTS = BENCH / "results"


@dataclass(frozen=True)
class BenchKey:
    name: str
    frontend: str


@dataclass
class BenchResult:
    key: BenchKey
    category: str
    target_ms: list[float]
    query_file: str
    output_sha256: str
    output_semantic_sha256: str
    real_ms: float
    user_ms: float
    sys_ms: float
    max_rss_kb: float
    real_ms_min: float
    real_ms_max: float
    repeat: int


def query_metadata(name):
    path = BENCH / "queries" / name / "meta.json"
    if not path.exists():
        return {}

    return load_json(path)


class Colors:
    def __init__(self, enabled):
        self.enabled = enabled

    def wrap(self, s, code):
        return f"\033[{code}m{s}\033[0m" if self.enabled else s

    def bold(self, s):
        return self.wrap(s, "1")

    def dim(self, s):
        return self.wrap(s, "2")

    def green(self, s):
        return self.wrap(s, "32")

    def red(self, s):
        return self.wrap(s, "31")

    def yellow(self, s):
        return self.wrap(s, "33")


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


def meta_label(result_dir, meta):
    tag = meta.get("git_tag") or result_dir.name
    rev = meta.get("git_revision")
    message = meta.get("git_commit_message")
    dirty = "-dirty" if meta.get("git_dirty") else ""
    if rev:
        return f"{tag} ({rev}{dirty}): {message}"
    return tag


def median(samples, key):
    return statistics.median(sample[key] for sample in samples)


def load_results(result_dir):
    meta_path = result_dir / "meta.json"
    meta = load_json(meta_path) if meta_path.exists() else {}
    results = {}

    for path in sorted(result_dir.glob("*.json")):
        if path.name == "meta.json":
            continue

        data = load_json(path)
        name = data["name"]
        metadata = data.get("metadata") or query_metadata(name)
        for item in data["results"]:
            samples = item["samples"]
            key = BenchKey(name=name, frontend=item["frontend"])
            summary = item.get("summary", {})
            results[key] = BenchResult(
                key=key,
                category=metadata.get("category", ""),
                target_ms=metadata.get("target_ms", []),
                query_file=item["query_file"],
                output_sha256=item["output_sha256"],
                output_semantic_sha256=item.get(
                    "output_semantic_sha256", item["output_sha256"]
                ),
                real_ms=summary.get("real_ms_median", median(samples, "real_ms")),
                user_ms=summary.get("user_ms_median", median(samples, "user_ms")),
                sys_ms=summary.get("sys_ms_median", median(samples, "sys_ms")),
                max_rss_kb=median(samples, "max_rss_kb"),
                real_ms_min=summary.get(
                    "real_ms_min", min(sample["real_ms"] for sample in samples)
                ),
                real_ms_max=summary.get(
                    "real_ms_max", max(sample["real_ms"] for sample in samples)
                ),
                repeat=len(samples),
            )

    return meta, results


def pct_delta(base, head):
    if base == 0:
        return math.inf if head != 0 else 0.0
    return (head / base - 1.0) * 100.0


def format_ms(x):
    if x >= 1000:
        return f"{x / 1000:.3f}s"
    if x >= 10:
        return f"{x:.1f}ms"
    return f"{x:.2f}ms"


def format_kb(x):
    if x >= 1024 * 1024:
        return f"{x / 1024 / 1024:.2f}G"
    if x >= 1024:
        return f"{x / 1024:.1f}M"
    return f"{x:.0f}K"


def spread_pct(result):
    if result.real_ms == 0:
        return 0.0
    return (result.real_ms_max - result.real_ms_min) / result.real_ms * 100.0


def target_verdict(result, colors):
    if len(result.target_ms) != 2:
        return ""

    low, high = result.target_ms
    if result.real_ms < low:
        return colors.yellow("low")
    if result.real_ms > high:
        return colors.yellow("high")
    return colors.dim("ok")


def format_pct(x, colors, threshold):
    if math.isinf(x):
        return colors.yellow("+inf")

    s = f"{x:+.1f}%"
    if x <= -threshold:
        return colors.green(s)
    if x >= threshold:
        return colors.red(s)
    return colors.dim(s)


def verdict(delta, threshold):
    if delta <= -threshold:
        return "faster"
    if delta >= threshold:
        return "slower"
    return "same"


def output_verdict(base, head, colors):
    if base.output_semantic_sha256 != head.output_semantic_sha256:
        return colors.yellow("different")
    if base.output_sha256 != head.output_sha256:
        return colors.dim("bytes")
    return "equal"


def pad_rows(rows):
    widths = [0] * len(rows[0])
    for row in rows:
        for i, cell in enumerate(row):
            widths[i] = max(widths[i], len(strip_ansi(cell)))

    result = []
    for row in rows:
        padded = []
        for i, cell in enumerate(row):
            extra = widths[i] - len(strip_ansi(cell))
            if i >= 2:
                padded.append(" " * extra + cell)
            else:
                padded.append(cell + " " * extra)
        result.append("  ".join(padded))
    return result


def strip_ansi(s):
    out = ""
    i = 0
    while i < len(s):
        if s[i : i + 2] == "\033[":
            j = s.find("m", i)
            i = len(s) if j == -1 else j + 1
        else:
            out += s[i]
            i += 1
    return out


def print_table(base_results, head_results, colors, threshold):
    rows = [
        [
            "query",
            "cat",
            "front",
            "base",
            "head",
            "delta",
            "b-spread",
            "h-spread",
            "s-delta",
            "target",
            "user",
            "sys",
            "rss",
            "output",
            "result",
        ]
    ]

    shared = sorted(
        set(base_results) & set(head_results), key=lambda k: (k.name, k.frontend)
    )
    for key in shared:
        base = base_results[key]
        head = head_results[key]
        real_delta = pct_delta(base.real_ms, head.real_ms)
        user_delta = pct_delta(base.user_ms, head.user_ms)
        sys_delta = pct_delta(base.sys_ms, head.sys_ms)
        rss_delta = pct_delta(base.max_rss_kb, head.max_rss_kb)
        base_spread = spread_pct(base)
        head_spread = spread_pct(head)

        rows.append(
            [
                key.name,
                head.category or base.category,
                key.frontend,
                format_ms(base.real_ms),
                format_ms(head.real_ms),
                format_pct(real_delta, colors, threshold),
                format_pct(base_spread, colors, 15.0),
                format_pct(head_spread, colors, 15.0),
                format_pct(head_spread - base_spread, colors, 10.0),
                target_verdict(head, colors),
                format_pct(user_delta, colors, threshold),
                format_pct(sys_delta, colors, threshold),
                format_pct(rss_delta, colors, threshold),
                output_verdict(base, head, colors),
                verdict(real_delta, threshold),
            ]
        )

    lines = pad_rows(rows)
    print(colors.bold(lines[0]))
    print(colors.dim("-" * len(strip_ansi(lines[0]))))
    for line in lines[1:]:
        print(line)


def print_summary(base_results, head_results, colors, threshold):
    shared = sorted(
        set(base_results) & set(head_results), key=lambda k: (k.name, k.frontend)
    )
    if not shared:
        print(colors.yellow("No matching benchmark results."))
        return

    faster = []
    slower = []
    same = []
    total_base = 0.0
    total_head = 0.0
    output_equal = []
    output_bytes = []
    output_different = []
    target_low = []
    target_high = []

    for key in shared:
        base = base_results[key]
        head = head_results[key]
        delta = pct_delta(base.real_ms, head.real_ms)
        total_base += base.real_ms
        total_head += head.real_ms

        if base.output_semantic_sha256 != head.output_semantic_sha256:
            output_different.append(key)
        elif base.output_sha256 != head.output_sha256:
            output_bytes.append(key)
        else:
            output_equal.append(key)

        if delta <= -threshold:
            faster.append((key, delta))
        elif delta >= threshold:
            slower.append((key, delta))
        else:
            same.append((key, delta))

        if len(head.target_ms) == 2:
            low, high = head.target_ms
            if head.real_ms < low:
                target_low.append(key)
            elif head.real_ms > high:
                target_high.append(key)

    total_delta = pct_delta(total_base, total_head)
    print()
    print(colors.bold("Summary"))
    print(f"shared benchmarks: {len(shared)}")
    print(
        "sum of medians: "
        f"{format_ms(total_base)} -> {format_ms(total_head)} "
        f"({format_pct(total_delta, colors, threshold)})"
    )
    print(f"faster: {len(faster)}, slower: {len(slower)}, same/noisy: {len(same)}")
    print(
        "output: "
        f"equal: {len(output_equal)}, "
        f"bytes: {len(output_bytes)}, "
        f"different: {len(output_different)}"
    )
    print(f"target band: low: {len(target_low)}, high: {len(target_high)}")

    if output_different:
        print(colors.yellow(f"semantic output differences: {len(output_different)}"))
        for key in output_different:
            print(colors.yellow(f"  {key.name} ({key.frontend})"))

    if target_low or target_high:
        print(colors.yellow("target band misses:"))
        for key in target_low:
            print(colors.yellow(f"  {key.name} ({key.frontend}): low"))
        for key in target_high:
            print(colors.yellow(f"  {key.name} ({key.frontend}): high"))

    missing_base = sorted(
        set(head_results) - set(base_results), key=lambda k: (k.name, k.frontend)
    )
    missing_head = sorted(
        set(base_results) - set(head_results), key=lambda k: (k.name, k.frontend)
    )
    if missing_base:
        print(
            colors.yellow(
                f"only in head: {', '.join(f'{k.name}/{k.frontend}' for k in missing_base)}"
            )
        )
    if missing_head:
        print(
            colors.yellow(
                f"only in base: {', '.join(f'{k.name}/{k.frontend}' for k in missing_head)}"
            )
        )


def main():
    parser = argparse.ArgumentParser(
        description="Compare two logsql benchmark result directories"
    )
    parser.add_argument("base", help="base result directory or tag under bench/results")
    parser.add_argument("head", help="head result directory or tag under bench/results")
    parser.add_argument(
        "--threshold",
        type=float,
        default=3.0,
        help="percent delta considered meaningful",
    )
    parser.add_argument("--no-color", action="store_true")
    args = parser.parse_args()

    if args.threshold < 0:
        raise RuntimeError("--threshold should be non-negative")

    colors = Colors(enabled=not args.no_color and sys.stdout.isatty())
    base_dir = resolve_result_dir(args.base)
    head_dir = resolve_result_dir(args.head)
    base_meta, base_results = load_results(base_dir)
    head_meta, head_results = load_results(head_dir)

    print(colors.bold("Benchmark Comparison"))
    print(f"base: {meta_label(base_dir, base_meta)}")
    print(f"head: {meta_label(head_dir, head_meta)}")
    print()

    print_table(base_results, head_results, colors, args.threshold)
    print_summary(base_results, head_results, colors, args.threshold)


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"error: {e}", file=sys.stderr)
        sys.exit(1)
