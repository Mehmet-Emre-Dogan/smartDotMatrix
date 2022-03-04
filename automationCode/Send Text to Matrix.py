import requests
txt = "*"
print("çıkmak için '#' yazın")
while True:
    txt = input("Printlenecek metini giriniz:\n--> ")
    if txt == "#":
        break
    txt = txt.replace(" ", "__").replace(">", "_!B").replace("<", "_!K").replace("<", "_!K").replace("Ğ", "_!G").replace("Ü", "_!U").replace("Ş", "_!S").replace("İ", "_!I").replace("Ö", "_!O").replace("Ç", "_!C").replace("ğ", "_!g").replace("ü", "_!u").replace("ş", "_!s").replace("ı", "_!i").replace("ö", "_!o").replace("ç", "_!c")
    #print(txt)
    requests.get(f"http://192.168.1.102/setText/${txt}$")
    print("Ayarlandı!")