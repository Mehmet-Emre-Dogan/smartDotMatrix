import requests
from datetime import datetime
from time import sleep
from msvcrt import getch
data = datetime.now()
requests.get(f"http://192.168.1.102/setRtc/sec/?{data.second}?")
requests.get(f"http://192.168.1.102/setRtc/min/?{data.minute}?")
requests.get(f"http://192.168.1.102/setRtc/hour/?{data.hour}?")
requests.get(f"http://192.168.1.102/setRtc/dow/?{data.weekday()+1}?")
requests.get(f"http://192.168.1.102/setRtc/day/?{data.day}?")
requests.get(f"http://192.168.1.102/setRtc/mon/?{data.month}?")
requests.get(f"http://192.168.1.102/setRtc/yr/?{data.year}?")
print(f"saat: {data.hour}\ndakika: {data.minute}\nsaniye: {data.second}\nhaftanın günü: {data.weekday()+1}\ngün: {data.day}\nay: {data.month}\nyıl: {data.year}")
print("Olarak ayarlandı!")
print("Çıkış için bir tuşa basınız")
getch()