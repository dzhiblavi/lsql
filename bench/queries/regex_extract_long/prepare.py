from lib.generate import fixed, format_string, random_int, random_string, write_imap_log


rows = 80_000
user = random_int(0, 50_000)
session = random_int(0, 10_000)
noise = random_string(distr=fixed(220))

fields = {
    "uid": format_string("user{user}", user=user),
    "message": format_string(
        "prefix{noise} user={user} session=s{session} suffix{tail}",
        noise=noise,
        user=user,
        session=session,
        tail=random_string(distr=fixed(64)),
    ),
}

write_imap_log("input.1.txt", rows, fields)
