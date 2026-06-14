from lib.generate import format_string, random_int, write_imap_log


rows = 200_000

user = random_int(0, 49_999)
session = random_int(0, 9_999)

fields = {
    "uid": format_string("user{user}", user=user),
    "status": "ok",
    "request": format_string(
        "/api/search?user={user}&session=s{session}&q=item{item}",
        user=user,
        session=session,
        item=random_int(0, 999),
    ),
}

write_imap_log("input.1.txt", rows, fields)
