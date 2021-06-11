import os
import json

DIR = '/mnt/temp/data/1_free/'
l = os.listdir(DIR)
cnt = 0
for i, name in enumerate(l):
    f = open(DIR + name)
    data = json.load(f)
    le = len(data['time'])
    if le > 50 and le < 150 :
        print(name, le)
        break
    if "_234109952177_58" in name:
        print(name)
        cnt += 1
print(cnt)
