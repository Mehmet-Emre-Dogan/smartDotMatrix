import requests
from selenium import webdriver
from selenium.webdriver.common.keys import Keys
from time import sleep
from selenium.webdriver.chrome.options import Options
import warnings
from datetime import datetime
from json import load
warnings.filterwarnings("ignore", category=DeprecationWarning)

settingsPath = "settings.json"
outputPath = "output"
outputExtension = ".txt"
errorPath = "errors"
errorExtension =".txt"
url1 = "https://www.mgm.gov.tr/tahmin/il-ve-ilceler.aspx?il=ANKARA&ilce="
url2 = "https://www.mgm.gov.tr/?il=Ankara"

dogus = ""
batis = ""
havaDurumu = ""

gunler = ["pazartesi", "salı", "çarşamba", "perşembe", "cuma", "cumartesi", "pazar"]
sGunler = ["pzt", "sal", "çrş", "prş", "cum", "cmt", "pzr"]

# initialize setting.json
try:
    print("loading settings...")
    with open(settingsPath, "r", encoding="utf-8") as fil:
        settings = load(fil)
    print("settings loaded succesfully")
except Exception as ex:
    print(ex)

print("initializing browser(s)...")
secenek = Options()
secenek.add_argument("--headless") #pencere görmemek için
# secenek.add_argument("--window-size=1280,720")
secenek.add_argument("--log-level=3") #konsola log basmasın diye
#secenek.add_experimental_option('excludeSwitches', ['enable-logging']) #DevTools listening on ws://127.0..... yazısını printlenmesin diye # bunu açınca program kapanınca tarayıcılır açık kalıyor. o yüzden kullanmıyoruz
prefs = {"profile.default_content_setting_values.notifications" : 2}# bildirim engelleme kodu
secenek.add_experimental_option("prefs",prefs)

tarayici1 = webdriver.Chrome(settings["browserPath"], chrome_options=secenek)
tarayici2 =webdriver.Chrome(settings["browserPath"], chrome_options=secenek)

print("browser(s) have been initialized")

# initialize browser 1 to scrap
for i in range(10):
    try:
        tarayici1.get(url1)
        print("initializing scrapper 1...")
        sleep(5)
        print("initialization successful")
        break
    except:
        print("initialization failed")
        print(f"try reopening program and be sure the website '{url1}' is not down ")

# initialize browser 3 to scrap
for i in range(10):
    try:
        tarayici2.get(url2)
        print("initializing mgm...")
        sleep(5)
        print("initialization successful")
        break
    except:
        print("initialization failed")
        print(f"try reopening program and be sure the website '{url2}' is not down ")

print("#"*50)

for i in range(10):
    tarihName = datetime.now().strftime('%Y-%m-%d') #dosya adı için
    tarihPrint = datetime.now().strftime('%d/%m/%Y %H:%M:%S') #ekrana printlemek için
    try:
        dogus = " -->Gün Doğumu: " + tarayici1.find_element_by_xpath("/html/body/div[2]/div[2]/div/section/div[3]/div[4]/span").text + " "
        batis = " -->Gün Batımı: " + tarayici1.find_element_by_xpath("/html/body/div[2]/div[2]/div/section/div[3]/div[5]/span").text + " "
        break
    except Exception as ex:
        print(ex)
        with open(errorPath + "-meteoroloji" + tarihName + errorExtension, "a", encoding="utf-8") as fil:
            fil.write(f"{tarihPrint} ---> {ex}\n")
print(dogus)
print(batis)
print("#"*50 )

for i in range(10):
    tarihName = datetime.now().strftime('%Y-%m-%d') #dosya adı için
    tarihPrint = datetime.now().strftime('%d/%m/%Y %H:%M:%S') #ekrana printlemek için
    try:
        gun1 = tarayici2.find_element_by_xpath("/html/body/section[1]/div/div[2]/div[4]/div/div[1]/div/div[1]/div[1]").text.lower()
        gun2 = tarayici2.find_element_by_xpath("/html/body/section[1]/div/div[2]/div[4]/div/div[2]/div/div[1]/div[1]").text.lower()
        tahmin1 = tarayici2.find_element_by_xpath("/html/body/section[1]/div/div[2]/div[4]/div/div[1]/div/div[1]/div[3]").text.lower()
        tahmin2 = tarayici2.find_element_by_xpath("/html/body/section[1]/div/div[2]/div[4]/div/div[2]/div/div[1]/div[3]").text.lower()
        try:
            gun1 = sGunler[gunler.index(gun1)]
        except:
            gun1 = "idk"
        try:
            gun2 = sGunler[gunler.index(gun2)]
        except:
            gun2 = "idk"
        havaDurumu = f"->{gun1}: {tahmin1} ->{gun2}: {tahmin2} "
        break
    except Exception as ex:
        print(ex)
        with open(errorPath + "-meteoroloji-hava" + tarihName + errorExtension, "a", encoding="utf-8") as fil:
            fil.write(f"{tarihPrint} ---> {ex}\n")
print(havaDurumu)
print("#"*50 )

tarih = datetime.now().strftime('%d-%m')
txt = f"{havaDurumu}{dogus}{batis} ({tarih})"
txt = txt.replace(" ", "__").replace(">", "_!B").replace("<", "_!K").replace("<", "_!K").replace("Ğ", "_!G").replace("Ü", "_!U").replace("Ş", "_!S").replace("İ", "_!I").replace("Ö", "_!O").replace("Ç", "_!C").replace("ğ", "_!g").replace("ü", "_!u").replace("ş", "_!s").replace("ı", "_!i").replace("ö", "_!o").replace("ç", "_!c")
#print(txt)
requests.get(f"http://192.168.1.102/setText3/${txt}$")
print("Ayarlandı!")
tarayici1.close()
tarayici1.quit()
tarayici2.close()
tarayici2.quit()