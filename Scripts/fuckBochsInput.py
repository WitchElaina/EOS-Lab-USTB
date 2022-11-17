import time
import pyautogui
import pyperclip

# define
SLEEP_TIME = 3
MENU_LOGO = ''' /$$$$$$$$                  /$$                       
| $$_____/                 | $$                       
| $$    /$$   /$$  /$$$$$$$| $$   /$$                 
| $$$$$| $$  | $$ /$$_____/| $$  /$$/                 
| $$__/| $$  | $$| $$      | $$$$$$/                  
| $$   | $$  | $$| $$      | $$_  $$                  
| $$   |  $$$$$$/|  $$$$$$$| $$ \  $$                 
|__/    \______/  \_______/|__/  \__/                 
                                                      
                                                      
                                                      
 /$$     /$$                                          
|  $$   /$$/                                          
 \  $$ /$$//$$$$$$  /$$   /$$                         
  \  $$$$//$$__  $$| $$  | $$                         
   \  $$/| $$  \ $$| $$  | $$                         
    | $$ | $$  | $$| $$  | $$                         
    | $$ |  $$$$$$/|  $$$$$$/                         
    |__/  \______/  \______/                          
                                                      
                                                      
                                                      
 /$$$$$$$                      /$$                 /$$
| $$__  $$                    | $$                | $$
| $$  \ $$  /$$$$$$   /$$$$$$$| $$$$$$$   /$$$$$$$| $$
| $$$$$$$  /$$__  $$ /$$_____/| $$__  $$ /$$_____/| $$
| $$__  $$| $$  \ $$| $$      | $$  \ $$|  $$$$$$ |__/
| $$  \ $$| $$  | $$| $$      | $$  | $$ \____  $$    
| $$$$$$$/|  $$$$$$/|  $$$$$$$| $$  | $$ /$$$$$$$/ /$$
|_______/  \______/  \_______/|__/  |__/|_______/ |__/'''
MENU_ITMES = '''
Fuck Bochs Inputs
by WitchElaina
----------------------------
c.Input Clipboard Texts.

r.Input 'A:\\eosapp.exe'
a.Input ' A:\\a.txt'
b.Input ' A:\\b.txt'
c.Input ' A:\\c.txt'
d.Input ' A:\\d.txt'

f.Let's Fuck Bochs' JUNK input method!
0.exit
----------------------------
tips: You can input multi items like 'ab' to 'A:\\eosapp.exe A:\\a.txt' :-)
'''
PROMOT = 'Let\'s Fuck>'

def pyagInput(str_in:str):
    time.sleep(SLEEP_TIME)
    pyautogui.typewrite(str_in)
    
def showMenu():
    print(MENU_ITMES)
    
def inputClipboard():
    text = pyperclip.paste()
    text.replace('','\n')
    print('Will input: '+text)
    pyagInput(text)
    
def inputEOSApp():
    print('Will input: A:\\eosapp.exe')
    pyautogui.typewrite('A:\\eosapp.exe')
    
def inputTxt(txt_items:str):
    text = ''
    for i in range(txt_items.__len__()):
        if txt_items[i] == 'r':
            text += 'A:\\eosapp.exe'
        else:
            text += ' A:\\'+txt_items[i]+'.txt'
    print('Will input: '+text)
    pyagInput(text)
    
def fuckBochs():
    print(MENU_LOGO)
    
if __name__ == '__main__':
    showMenu()
    input_cmd = ''
    while(True):
        print(PROMOT, end='')
        input_cmd = input()
        if input_cmd == 'c':
            inputClipboard()
        elif input_cmd == 'f':
            fuckBochs()
        elif input_cmd == 'r':
            inputEOSApp()
        elif input_cmd == 'e' or input_cmd == '0':
            exit(0)
        else:
            inputTxt(input_cmd)