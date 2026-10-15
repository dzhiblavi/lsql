users = (
    SELECT * FROM input.txt WHERE uid = 'aaa'
)

SELECT uid FROM input.txt WHERE uid IN $users

SELECT status FROM $users
