#define CTRLPIN 9
#define CTRLPINBIT BIT9

#define LED1 12
#define LED2 13

inline uint32_t __j;
#define DELAY_NANOSECS(_XNANOS) for (__j = _XNANOS; __j > 0; __j--);

inline boolean crowbar = true;
boolean serialstate = false;
boolean serialNewline = true;
boolean randomb = false;
boolean state = 0;
long dropTime = 100; 
int tinc = 10;


void setup() {
  delay(2000);
  pinMode(CTRLPIN, OUTPUT);  
  digitalWrite(CTRLPIN, 0);
  pinMode(LED1, OUTPUT);     
  pinMode(LED2, OUTPUT);     
  digitalWrite(LED1,LOW);
  digitalWrite(LED2,LOW);
  Serial.begin(115200);
  Serial.println("Initializing...");
  Serial.println("Calculating nops time...");
  int64_t startus = esp_timer_get_time();
  DELAY_NANOSECS(100000);
  int64_t stopus = esp_timer_get_time();
  Serial.print("100k nops took: ");
  Serial.print(stopus-startus,DEC);
  Serial.println(" microseconds");
  Serial.print(100000.0/(stopus-startus),DEC);
  Serial.println(" nops/usec");
  Serial.print(1000/(100000.0/(stopus-startus)),DEC);
  Serial.println(" nanoseconds resolution (minimum delay)");

}

void printMenu(){
  Serial.println("Seringe v0.0.1");
  Serial.print("State: ");
  Serial.print(state);
  Serial.print("  Crowbar mode: ");
  Serial.print(crowbar);
  Serial.print("  RandomBlast: ");
  Serial.println(randomb);
  Serial.print("Voltage drop time(nops): ");
  Serial.println(dropTime);
  Serial.println("h -> help");
  Serial.println("p -> serial passthrough (pin 2)");
  Serial.println("c -> toggle crowbar (inverse logic)");
  Serial.println("t -> toggle power (force on/off)");
  Serial.println("t -> toggle random blast");
  Serial.println("u -> toogle units (1,10,100,1000)");
  Serial.println("+ -> increase voltage drop time");
  Serial.println("- -> decrease voltage drop time");
  Serial.println("f -> FIRE!");
 }



void fire() {
  if (randomb) fireRandomBlast();
  else if (crowbar) fireCrowbar();
  else fireNormal();
}

void setState(boolean s){
  if (s) REG_WRITE(GPIO_OUT_W1TS_REG, CTRLPINBIT);
  else  REG_WRITE(GPIO_OUT_W1TC_REG, CTRLPINBIT);
  digitalWrite(LED1,s);
}

void toggleState(){
  state = !state;
  setState(state);
}

void fireNormal() {
  digitalWrite(LED1,0);
  delayMicroseconds(100);
  digitalWrite(LED1,1);
  REG_WRITE(GPIO_OUT_W1TC_REG, CTRLPINBIT);
  DELAY_NANOSECS(dropTime);
  REG_WRITE(GPIO_OUT_W1TS_REG, CTRLPINBIT);
  setState(1);  
}

void fireCrowbar() {
  digitalWrite(LED1,0);
  delayMicroseconds(100);
  digitalWrite(LED1,1);
  REG_WRITE(GPIO_OUT_W1TS_REG, CTRLPINBIT);
  DELAY_NANOSECS(dropTime);
  REG_WRITE(GPIO_OUT_W1TC_REG, CTRLPINBIT);
  setState(0);  
}

void fireRandomBlast(){
  unsigned int totalDelay = 0;
  int64_t startus = esp_timer_get_time();
  digitalWrite(LED1,1);
  int randomTime;
  //drop time here is in usecs
  while(esp_timer_get_time() - startus < dropTime){
   randomTime = (long) random(0,30);
   REG_WRITE(GPIO_OUT_W1TS_REG, CTRLPINBIT);
   DELAY_NANOSECS(randomTime);
   randomTime = (long) random(0,30);
   REG_WRITE(GPIO_OUT_W1TC_REG, CTRLPINBIT);
   DELAY_NANOSECS(randomTime);
  }
  setState(!crowbar);  
  digitalWrite(LED1,0);
}



void toggleCrowbar(){
  crowbar = !crowbar;
  digitalWrite(LED2,crowbar);
}

void toggleRandom(){
  randomb = !randomb;
}

void toggleSerial(){
  serialstate = !serialstate;
  if (serialstate) {
    Serial1.begin(115200, SERIAL_8N1, 2, 3);
    serialNewline = 1;
  }
  else Serial1.end();
}

char last = 0;
void loop() {
  if (Serial.available() > 0){
    char cmd = Serial.read();  
    switch(cmd) {
      case '\n' : Serial.println(); break;
      case 'h' : printMenu(); break;
      case 'e' : Serial.println("echo!"); break;
      case 'c' : Serial.println("toggle crowbar!"); toggleCrowbar(); Serial.println(crowbar);break;
      case 'r' : Serial.println("toggle random blast!"); toggleRandom(); Serial.println(randomb);break;
      case 't' : Serial.println("toggle power!"); toggleState();Serial.println(state); break;
      case 'p' : Serial.println("toggle serial passthrough!"); toggleSerial();Serial.println(serialstate); break;
      case 'f' : Serial.println("fire!"); fire(); break;
      case 'u' : Serial.println("toggle time increase!"); tinc *= 10; if (tinc > 1000) tinc = 1; Serial.println(tinc);break;
      case '+' : Serial.println("increase!"); dropTime += tinc; Serial.println(dropTime); break;
      case '-' : Serial.println("decrease!"); dropTime -= tinc; if (dropTime <0) dropTime = 0; Serial.println(dropTime);break;    
    }
    last = cmd;
  }
  if (serialstate && Serial1.available() > 0) {
    char b = Serial1.read();
    if (serialNewline) { Serial.print("* "); serialNewline = false;}
    Serial.print(b);
    if (b == '\n') serialNewline = true;
  }
  delay(1);
}
