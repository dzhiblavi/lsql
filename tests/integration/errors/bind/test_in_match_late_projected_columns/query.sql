users = input.txt

SELECT uid IN $users AS is_known FROM input.txt

SELECT uid, status FROM $users
