import threading
from selenium import webdriver
from selenium.webdriver.common.keys import Keys
from time import sleep
from selenium.webdriver.chrome.options import Options
import requests
from json import load
from datetime import datetime
import warnings
import requests

warnings.filterwarnings("ignore", category=DeprecationWarning)
HEADLESS = True

settingsPath = "settings.json"
outputPath = "output"
outputExtension = ".txt"
errorPath = "errors"
errorExtension =".txt"
paraUrl = "https://www.bloomberght.com/"
havaUrl = "https://www.mgm.gov.tr/?il=Ankara"

#necessary variables
altin = "e:__gold"
dolar = "e:__usd"
euro = "e:__eur"
hava = "null"
settings = {}
couCurr = 0
couWeat = 0
firstSend = True

# initialize setting.json
try:
    print("loading settings...")
    with open(settingsPath, "r", encoding="utf-8") as fil:
        settings = load(fil)
    print("settings loaded succesfully")
except Exception as ex:
    print(ex)

print("initializing browsers...")
secenek = Options()
if HEADLESS:
    secenek.add_argument("--headless") #pencere görmemek için
secenek.add_argument("--log-level=3") #konsola log basmasın diye
#secenek.add_experimental_option('excludeSwitches', ['enable-logging']) #DevTools listening on ws://127.0..... zamazingosu printlenmesin diye # bunu açınca program kapanınca tarayıcılır açık kalıyor. o yüzden kullanmıyoruz
prefs = {"profile.default_content_setting_values.notifications" : 2}# bildirim engelleme kodu
secenek.add_experimental_option("prefs",prefs)

tarayiciPara = webdriver.Chrome(settings["browserPath"], chrome_options=secenek)
tarayiciHava = webdriver.Chrome(settings["browserPath"], chrome_options=secenek)
tarayiciPara.set_window_size(1920, 900)
tarayiciHava.set_window_size(1280, 720)
print("browsers have been initialized")

# initialize browser for currency scrapping
try:
    tarayiciPara.get(paraUrl)
    print("initializing currency scrapper...")
    sleep(5)
    #1/0
    print("initialization successful")  
except:
    print("initialization failed")
    print(f"try reopening program and be sure the website '{paraUrl}' is not down ")

# initialize browser for weather scrapping
try:
    tarayiciHava.get(havaUrl)
    print("initializing weather scrapper...")
    sleep(5)
    #1/0
    print("initialization successful")  
except:
    print("initialization failed")
    print(f"try reopening program and be sure the website '{havaUrl}' is not down ")

print("#"*50 + '\n')

def scrapCurrency():
    global altin, dolar, euro, couCurr
    while True:
        try:
            altinTemp = ""
            dolarTemp = ""
            euroTemp = ""
            tarih = datetime.now().strftime('%Y-%m-%d') #dosya adı için
            tarih2 = datetime.now().strftime('%d/%m/%Y %H:%M:%S') #ekrana printlemek için
            if not couCurr % 12: # 12 döngüde bir refresh at
                tarayiciPara.refresh()
                print(f"{tarih2}: sayfa yenilendi")
            sleep(3)
            # if not couCurr % 12: #refresh atıldıysa 
                # EMTİA butonuna tıkla
            btnCommodity = tarayiciPara.find_element_by_xpath("/html/body/div[3]/section[1]/div/div[2]/div[2]/div/div/div[3]/div/div/ul/li[3]/a")
            btnCommodity.click()
            sleep(1)
            altinTemp = tarayiciPara.find_element_by_xpath("/html/body/div[3]/section[1]/div/div[2]/div[2]/div/div/div[3]/div/div/div[2]/div[3]/div/table/tbody/tr[2]/td[2]").text

            # DÖVİZ butonuna tıkla
            btnCurrency = tarayiciPara.find_element_by_xpath("/html/body/div[3]/section[1]/div/div[2]/div[2]/div/div/div[3]/div/div/ul/li[2]/a")
            btnCurrency.click()
            sleep(1)
            dolarTemp = tarayiciPara.find_element_by_xpath("/html/body/div[3]/section[1]/div/div[2]/div[2]/div/div/div[3]/div/div/div[2]/div[2]/div/table/tbody/tr[1]/td[2]").text
            euroTemp = tarayiciPara.find_element_by_xpath("/html/body/div[3]/section[1]/div/div[2]/div[2]/div/div/div[3]/div/div/div[2]/div[2]/div/table/tbody/tr[2]/td[2]").text
            

            if altinTemp == "": #string boş gelirse kaydetmeden geç
                print(f"{tarih2}: altın için  null string return edildi")
            else:#dataları kaydet          
                with open(outputPath + " altın " + tarih + outputExtension, "a", encoding="utf-8") as fil:
                    fil.write(f"{tarih2} ---> {altinTemp}\n")
                print(f"{tarih2}: Gram Altın:{altinTemp} TL")
                altin = altinTemp

            if dolarTemp == "":
                print(f"{tarih2}: dolar için  null string return edildi")
            else:
                with open(outputPath + " dolar " + tarih + outputExtension, "a", encoding="utf-8") as fil:
                    fil.write(f"{tarih2} ---> {dolarTemp}\n")
                print(f"{tarih2}: Dolar:{dolarTemp} TL")
                dolar = dolarTemp

            if euroTemp == "":
                print(f"{tarih2}: euro için  null string return edildi")
            else:
                euroTemp = str(euroTemp).replace(",", ".")
                euroTemp = str(round(float(euroTemp), 3))
                with open(outputPath + " euro " + tarih + outputExtension, "a", encoding="utf-8") as fil:
                    fil.write(f"{tarih2} ---> {euroTemp}\n")
                print(f"{tarih2}: Euro:{euroTemp} TL")
                euro = euroTemp
                # print(dolar)
                # print(altin)
                # print(hava)

        except Exception as ex:
            tarih = datetime.now().strftime('%Y-%m-%d') #dosya adı için
            tarih2 = datetime.now().strftime('%d/%m/%Y %H:%M:%S') #ekrana printlemek için
            with open(errorPath + "-currencyScrape" + tarih + errorExtension, "a", encoding="utf-8") as fil:
                fil.write(f"{tarih2} ---> {ex}\n")
            print(f"{tarih2}-Currency: {str(ex)}")

        finally:
            couCurr += 1
            if couCurr >= 5000:
                couCurr = 0

def scrapWeather():
    global hava, couWeat
    while True:
        try:
            tarih = datetime.now().strftime('%Y-%m-%d') #dosya adı için
            tarih2 = datetime.now().strftime('%d/%m/%Y %H:%M:%S') #ekrana printlemek için
            tarayiciHava.refresh()
            sleep(5)
            havaTemp = tarayiciHava.find_element_by_xpath("/html/body/section[1]/div/div[2]/div[2]/div/h3/span[1]/ziko").text
            if havaTemp == "":
                print(f"{tarih2}: hava için  null string return edildi")
            else:
                with open(outputPath + " hava " + tarih + outputExtension, "a", encoding="utf-8") as fil:
                    fil.write(f"{tarih2} ---> {havaTemp}\n")
                print(f"{tarih2}: Hava Sıcaklığı: {havaTemp}°C")
                hava = havaTemp
        except Exception as ex:
            tarih = datetime.now().strftime('%Y-%m-%d') #dosya adı için
            tarih2 = datetime.now().strftime('%d/%m/%Y %H:%M:%S') #ekrana printlemek için
            with open(errorPath + "-weatherScrap" + tarih + errorExtension, "a", encoding="utf-8") as fil:
                fil.write(f"{tarih2} ---> {ex}\n")

        finally:
            couWeat += 1
            if couWeat >= 5000:
                couWeat = 0
def send():
    global firstSend
    global altin, dolar, euro, hava
    if firstSend:
        sleep(15)
        firstSend = False
    while True:
        tarih = datetime.now().strftime('%Y-%m-%d') #dosya adı için
        tarih2 = datetime.now().strftime('%d/%m/%Y %H:%M:%S') #ekrana printlemek için
        if altin != '*' and dolar != '*' and hava != '*':
            try: 
                requests.get(f'http://192.168.1.102/setText2/$_a{altin}?_d{dolar}?_e{euro}?_h{hava}?$')
                print(f"{tarih2}: data matrixe gönderildi")
            except Exception as ex:
                with open(errorPath + "-send" + tarih + errorExtension, "a", encoding="utf-8") as fil:
                    fil.write(f"{tarih2} ---> {ex}\n")
                    print(f"data gönderilirken bir hata oluştu: {ex}")
            sleep(settings['delayTime'])

#run threads
workerCurr = threading.Thread(target=scrapCurrency, daemon=True)
workerWeat = threading.Thread(target=scrapWeather, daemon=True)
workerSend = threading.Thread(target=send, daemon=True)
workerCurr.start()
workerWeat.start()
workerSend.start()
workerCurr.join()
workerWeat.join()
workerSend.join()

# tarayiciPara.quit()
# tarayiciHava.quit
# print("goodbye")
# sleep(3)
