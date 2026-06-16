from datetime import datetime, timedelta
import gzip
from pathlib import Path
import random as random_module
import string


DEFAULT_TIMESTAMP_START = datetime(2026, 5, 6, 12, 0, 0, 123456)
DEFAULT_ALPHABET = string.ascii_lowercase + string.digits


def imap_timestamp(i, start=DEFAULT_TIMESTAMP_START):
    ts = start + timedelta(seconds=i)
    return ts.strftime("[%Y-%b-%d %H:%M:%S.%f]")


def value_of(spec, i, rng, cache):
    if callable(spec):
        key = id(spec)
        if key not in cache:
            cache[key] = spec(i, rng, cache)
        return cache[key]
    return spec


def constant(value):
    return lambda _i, _rng, _cache: value


def random(values):
    values = list(values)
    return lambda _i, rng, _cache: rng.choice(values)


def random_int(min_value, max_value):
    return lambda _i, rng, _cache: rng.randint(min_value, max_value)


def normal(mean, std):
    return lambda rng: rng.gauss(mean, std)


def fixed(value):
    return lambda _rng: value


def random_string(distr, alphabet=DEFAULT_ALPHABET, min_len=0):
    def generate(_i, rng, _cache):
        length = max(min_len, int(round(distr(rng))))
        return "".join(rng.choice(alphabet) for _ in range(length))

    return generate


def format_string(template, **values):
    return lambda i, rng, cache: template.format(
        **{key: value_of(value, i, rng, cache) for key, value in values.items()}
    )


def materialize_fields(fields, i, rng):
    if callable(fields):
        return fields(i)

    cache = {}
    return {key: value_of(value, i, rng, cache) for key, value in fields.items()}


def write_imap_log_to(f, rows, fields, timestamp, seed):
    rng = random_module.Random(seed)

    for i in range(rows):
        line_fields = materialize_fields(fields, i, rng)
        field_text = " ".join(f"{key}={value}" for key, value in line_fields.items())
        if field_text:
            f.write(f"{timestamp(i)} {field_text}\n")
        else:
            f.write(f"{timestamp(i)}\n")


def write_imap_log(path, rows, fields, timestamp=imap_timestamp, seed=0):
    path = Path(path)

    with path.open("w") as f:
        write_imap_log_to(f, rows, fields, timestamp, seed)


def write_imap_log_gzip(path, rows, fields, timestamp=imap_timestamp, seed=0):
    path = Path(path)

    with gzip.open(path, "wt") as f:
        write_imap_log_to(f, rows, fields, timestamp, seed)
