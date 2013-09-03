////////////////////////////////////////////////////////////////////////////
//                           BrewShark v1                                 //
// Change value of hltSIZE to match the max volume in your HLT            //  
//                                                                        //
////////////////////////////////////////////////////////////////////////////

#include <LiquidCrystal.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

int lcd_key     = 0;
int adc_key_in  = 0;
float current_volume = 0;                //Current total volume passed though flow meter
float target_volume = 23;                //Total desired volume
float adjustment_volume = 0;             //Adjustment volume to take into account delays in solenoid closing. Subtracted from the current volume.

#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

#define hltSIZE   50                       //Size of the Hot Liquor Tun in litres

#define SOLENOIDPIN 1                      //Solenoid relay on Pin 1
#define FLOWSENSORPIN 2                    //Flow sensore on Pin 2

// count how many pulses
volatile uint16_t pulses = 0;
// track the state of the pulse pin
volatile uint8_t lastflowpinstate;
// you can try to keep time of how long it is between pulses
volatile uint32_t lastflowratetimer = 0;
// and use that to calculate a flow rate
volatile float flowrate;
// Interrupt is called once a millisecond, looks for any pulses from the sensor!
SIGNAL(TIMER0_COMPA_vect) {
  uint8_t x = digitalRead(FLOWSENSORPIN);
  
  if (x == lastflowpinstate) {
    lastflowratetimer++;
    return; // nothing changed!
  }
  
  if (x == HIGH) {
    //low to high transition!
    pulses++;
  }
  lastflowpinstate = x;
  flowrate = 1000.0;
  flowrate /= lastflowratetimer; // in hertz
  lastflowratetimer = 0;
}

void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
  }
}

void setup()
{
    
  Serial.begin(9600);
  Serial.print("Flow sensor test!");
  
  digitalWrite(FLOWSENSORPIN, HIGH);
  digitalWrite(SOLENOIDPIN,HIGH); //ACTIVE LOW RELAY PUT THIS FIRST TO ENSURE PIN IS IN CORRECT STATE BEFORE WE INIT IT
  
  pinMode(FLOWSENSORPIN, INPUT);
  pinMode(SOLENOIDPIN, OUTPUT);
  
  lcd.begin(16, 2);                        // start the library

  printBanner(2000);                       // show banner for 2 seconds
  
  clearLCD();
  
  printLabels();
  
  lastflowpinstate = digitalRead(FLOWSENSORPIN);
  useInterrupt(true);

}

void loop()
{

  while(current_volume < (target_volume - adjustment_volume))
  { 
    dowork();
  }

  lcd.setCursor(0,0);
  lcd.print("  VALVE CLOSED  ");
  lcd.setCursor(0,1);
  lcd.print("   TARGET HIT   ");
  
  //Debug Messaging
  Serial.println("  VALVE CLOSED  ");
  Serial.println("   TARGET HIT   ");
  
  digitalWrite(SOLENOIDPIN,HIGH);      //Close solenoid
}

void clearLCD()
{
  lcd.setCursor(0,0);                      
  lcd.print("                ");
  lcd.setCursor(0,1);
  lcd.print("                ");
}

void printBanner(int bannerdelay)
{
  lcd.setCursor(0,0);                     
  lcd.print("        ^       ");
  lcd.setCursor(0,1);  
  lcd.print("  >=BrewShark>  ");
  delay(bannerdelay);
}

void printLabels()
{
  lcd.setCursor(0,0);
  lcd.print("Set Vol: ");
  lcd.setCursor(0,1);
  lcd.print("Cur Vol: ");
}

// read the buttons
int read_LCD_buttons()
{
  adc_key_in = analogRead(0);
  if (adc_key_in > 1000) return btnNONE;
  if (adc_key_in < 50)   return btnRIGHT; 
  if (adc_key_in < 195)  return btnSELECT;
  if (adc_key_in < 380)  return btnUP;
  if (adc_key_in < 555)  return btnDOWN;
  if (adc_key_in < 790)  return btnLEFT;  

  return btnNONE;
}

void dowork()
{

  //Serial.print("Freq: "); Serial.println(flowrate);        //Uncomment these two lines to monitor the freq/pulses
  //Serial.print("Pulses: "); Serial.println(pulses, DEC);
  float liters = pulses;
  liters /= 7.5;
  liters /= 60.0;
  
  current_volume = liters;
  
  //Debug Messaging
  Serial.print("Cur: "); Serial.println(current_volume);
  Serial.print("Tar: "); Serial.println(target_volume);
  
  //Print the target volume value in litres
  lcd.setCursor(11,0);
  lcd.print(target_volume,2);
  lcd.setCursor(15,0);
  lcd.print("L");

  //Print the current volume value in litres
  lcd.setCursor(11,1);            
  lcd.print(current_volume,2);
  lcd.setCursor(15,1);
  lcd.print("L");

  lcd_key = read_LCD_buttons();    // read the buttons
  

  switch (lcd_key)               // depending on which button was pushed, we perform an action
  {
  case btnRIGHT:
    {
      current_volume += 0.01;       //manually simulate flow
      break;
    }

  case btnLEFT:
    {
      digitalWrite(SOLENOIDPIN,LOW);       //START FLOW
      break;
    }

  case btnUP:
    {
      if (target_volume >= hltSIZE)  //don't go larger than the size of the vessel
      {
        target_volume = hltSIZE;
        break;
      }
      else
      {
        target_volume += 0.1;
        break;
      }

      break;
    }

  case btnDOWN:
    {
      if (target_volume <= 0)      //don't go below zero
      {
        target_volume = 0;
        break; 
      }
      else
      {
        target_volume -= 0.1;
      }
      break;
    }

  case btnNONE:
    {
      break;
    }
  }

}




