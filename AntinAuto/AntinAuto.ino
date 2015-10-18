#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <I2CIO.h>
#include <EEPROM.h>


 #define I2C_ADDR 0x27
#define POSITIVE 1          
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7);  // Set the LCD I2C address

I2CIO ic;

#define BACKLIGHT_PIN     13

#define LCD_BACKLIGHT   0x08
#define LCD_NOBACKLIGHT 0x00

#include <stdarg.h>

int lcdPrint(int line, const char *fmt, ...) {
    static char buffer[4][17];
    static char tmp[17];
    memset(tmp,' ',sizeof(tmp));
    tmp[sizeof(tmp)-1]=0;
    
    va_list args;
    va_start(args, fmt);
    
    int len = vsnprintf(tmp, sizeof(tmp),fmt, args);
    tmp[len] = ' ';
    tmp[sizeof(tmp)-1]=0;
    if(memcmp(tmp,buffer[line],sizeof(tmp))!=0) {
      memcpy(buffer[line],tmp,sizeof(tmp));
      lcd.setCursor (0, line);
      lcd.print(tmp);
     
    }

    return 0;

}



void setup() {

  
  lcd.begin(16,4);               // initialize the lcd 
   
  lcd.backlight();
  lcd.home ();                   // go home
  lcdPrint(0, "*****           ");
  lcdPrint(1, "*   *           ");
  lcdPrint(2, "*****           ");
  lcdPrint(3, "*   *TK-Talkkari");
  
  //Odotetaan 3,5 sekunttia
  while(millis()<3500){
    getAndUpdateDim();
  }
  
  for(int i=0;i<9;i++)  {
    
    getAndUpdateRelayState(i);
  }

}


#define KEY_UP    (1<<0)
#define KEY_DOWN  (1<<1)
#define KEY_ENTER (1<<2)

void setRelayState(int relay, unsigned char on) {
   int pin=-1;
   switch(relay){
     case 0: pin = 9; break;
     case 1: pin = 8; break;
     case 2: pin = 7; break;
     case 3: pin = 6; break;
     case 4: pin = 5; break;
     case 5: pin = 4; break;
     case 6: pin = 3; break;
     case 7: pin = 2; break;

   }; 
   pinMode(pin, OUTPUT);
   digitalWrite(pin, on?HIGH:LOW);
}

void (*menus[])() = {&menuRele0,&menuRele1,&menuRele2,&menuRele3,&menuRele4,&menuRele5,&menuRele6,&menuRele7, &menuDim, &menuAutoDim};
int menuIndex=0;

int rotateMenuAndGetKey() {
  static int last_state = 0;
  int current_state = 0;
  int ret_state = 0;
  
  int pins[] = {A0,A1,A2};
  
  for(int i=0;i<sizeof(pins)/sizeof(int);i++){
    pinMode(pins[i], INPUT);
    digitalRead(pins[i]);
    if(!digitalRead(pins[i])) {
      current_state |= 1<<i;
    } else {
      current_state &= ~(1<<i);
    }
  }

  ret_state = current_state & (~last_state);
  last_state = current_state;
  
  if(ret_state&KEY_UP) {
    menuIndex++;
    if(menuIndex>(sizeof(menus)/sizeof(void*)-1)) menuIndex = 0;
  }
  if(ret_state&KEY_DOWN) {
    menuIndex--;
    if(menuIndex<0) menuIndex = sizeof(menus)/sizeof(void*)-1;
  }
  
 
  return ret_state;
}

#define EEPROM_RELAY_ADD 0
#define EEPROM_DIM_ADD 10
#define EEPROM_DIM_AUTO_ADD 12

unsigned char flipAutoDimState() {
    unsigned char c = !EEPROM.read(EEPROM_DIM_AUTO_ADD);
    EEPROM.write(EEPROM_DIM_AUTO_ADD,c);
}    
unsigned char getAutoDimState() {
    return EEPROM.read(EEPROM_DIM_AUTO_ADD);
}  

unsigned char flipRelayState(int relay) {
    unsigned char c = !EEPROM.read(EEPROM_RELAY_ADD+relay);
    EEPROM.write(EEPROM_RELAY_ADD+relay,c);
    return c;
}
unsigned char getAndUpdateRelayState(int relay) {
    unsigned char state =  EEPROM.read(EEPROM_RELAY_ADD+relay);
    setRelayState(relay,state);
    return state;
}

unsigned char isDark() {
  const unsigned long filteringTime = 5000;
  
  static unsigned long dayTime=0xfffff;
  static unsigned long darkTime=0xfffff;  
  unsigned long ctime = millis();
  static unsigned char dark=0;
  
  if(!digitalRead(12)) { 
    //dark
    darkTime=ctime;
  } else {
    //dark
    dayTime=ctime;
  }
  
  if((ctime-darkTime)>filteringTime) dark=1;
  if((ctime-dayTime)>filteringTime) dark=0;
  
  return dark;
}

unsigned char getAndUpdateDim() {
  unsigned char dimValue = EEPROM.read(EEPROM_DIM_ADD);
  unsigned char autoDim = EEPROM.read(EEPROM_DIM_AUTO_ADD); 
  unsigned char setDim = dimValue;
  static unsigned char dimLast = 0;

  static unsigned long detectedDarkTime = 0;
  if(autoDim) { 
    if(!isDark()) {
        setDim = 255;      
    }
  } 
  if(dimLast!=setDim) {
    static unsigned long t = 0;
    unsigned long c = millis();
    if(c-t>10) { //himmennys :)
      t = c;
      if(dimLast>setDim) setDim = dimLast - 1;
      else  setDim = dimLast + 1;
      dimLast = setDim;
      pinMode(11, OUTPUT);
      analogWrite(11, setDim);
    }
  }
  return dimValue;
}

void setDim(unsigned char dim) {
    EEPROM.write(EEPROM_DIM_ADD, dim); 
}

void menuRele0(){
  const int relay = 0;
  lcdPrint(0,"RELE0        %s", getAndUpdateRelayState(relay)?"ON":"OFF");
  lcdPrint(1,"              ");

  if(rotateMenuAndGetKey()&KEY_ENTER) {
    flipRelayState(relay);
  }
}

void menuRele1(){
  const int relay = 1;
  lcdPrint(0,"RELE1        %s", getAndUpdateRelayState(relay)?"ON ":"OFF");
  lcdPrint(1,"              ");
  
  if(rotateMenuAndGetKey()&KEY_ENTER) {
    flipRelayState(relay);
  }
}

void menuRele2(){
  const int relay = 2;
  lcdPrint(0,"RELE2        %s", getAndUpdateRelayState(relay)?"ON ":"OFF");
  lcdPrint(1,"              ");
  
  if(rotateMenuAndGetKey()&KEY_ENTER) {
    flipRelayState(relay);
  }
}

void menuRele3(){
  const int relay = 3;
  lcdPrint(0,"RELE3        %s", getAndUpdateRelayState(relay)?"ON ":"OFF");
  lcdPrint(1,"              ");
  
  if(rotateMenuAndGetKey()&KEY_ENTER) {
    flipRelayState(relay);
  }
}

void menuRele4(){
  const int relay = 4;
  lcdPrint(0,"RELE4        %s", getAndUpdateRelayState(relay)?"ON ":"OFF");
  lcdPrint(1,"              ");
  
  if(rotateMenuAndGetKey()&KEY_ENTER) {
    flipRelayState(relay);
  }
}

void menuRele5(){
  const int relay = 5;
  lcdPrint(0,"RELE5        %s", getAndUpdateRelayState(relay)?"ON ":"OFF");
  lcdPrint(1,"              ");
  
  if(rotateMenuAndGetKey()&KEY_ENTER) {
    flipRelayState(relay);
  }
}

void menuRele6(){
  const int relay = 6;
  lcdPrint(0,"RELE6        %s", getAndUpdateRelayState(relay)?"ON ":"OFF");
  lcdPrint(1,"              ");
  
  if(rotateMenuAndGetKey()&KEY_ENTER) {
    flipRelayState(relay);
  }
}

void menuRele7(){
  const int relay = 7;
  lcdPrint(0,"RELE7        %s", getAndUpdateRelayState(relay)?"ON ":"OFF");
  lcdPrint(1,"              ");
  
  if(rotateMenuAndGetKey()&KEY_ENTER) {
    flipRelayState(relay);
  }
}

void menuDim(){
  unsigned char dim = getAndUpdateDim();
  lcdPrint(0,"Kirkkaus      ");
  
  const unsigned char dims[6] = {25,51,102,153,204,255};

  if(dim==dims[0]) lcdPrint(1,"*         "); 
  if(dim==dims[1]) lcdPrint(1,"**        ");
  if(dim==dims[2]) lcdPrint(1,"****      ");
  if(dim==dims[3]) lcdPrint(1,"******    ");
  if(dim==dims[4]) lcdPrint(1,"********  ");
  if(dim==dims[5]) lcdPrint(1,"**********");
  
  if(rotateMenuAndGetKey()&KEY_ENTER) {
    for(int i=0;i<sizeof(dims);i++){
      if(dim<dims[i]) {
        setDim(dims[i]); 
        return;     
      }
    }
    setDim(dims[0]);  
  }
}

void menuAutoDim(){
  unsigned char state = getAutoDimState();
  lcdPrint(0,"Automaattinen ");
  lcdPrint(1,"himmennys %s", state?"ON":"OFF");
  
  if(rotateMenuAndGetKey()&KEY_ENTER) {
    flipAutoDimState();
  }
}


void infoLine() {
  char tmp[] = "               ";
  char tmp2[] = "               ";
  for(int i=0;i<8;i++) {
    if(getAndUpdateRelayState(i)) {
       tmp[i*2] = '1'; 
    } else {
      tmp[i*2] = 'O';
    }
    if(menuIndex==i) {
      tmp2[i*2] = 'v'; 
    }
  }
  lcdPrint(2,tmp2);
  lcdPrint(3,tmp);
}

void loop(){
  menus[menuIndex]();
  getAndUpdateDim();
  infoLine();
}
