import json

with open("holidays.json", "r", encoding="utf-8") as file:
    data = json.load(file)

# 日付だけのリストを作成
dates = list(data.keys())

# 日付を表示
print(dates)

s = ""
for date in dates:
    s = s + date + "\n"

with open("holidays.txt", "a") as file:
    file.write(s)
