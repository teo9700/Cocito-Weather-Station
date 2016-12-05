/*
 * Original version made by deba168
 * http://www.instructables.com/id/ARDUINO-SOLAR-CHARGE-CONTROLLER-Version-20/
 *
 *
 * Released under CC BY-NC-SA 3.0 https://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 * MORE INFO ON THE PROJECT PAGE:
 * https://hackaday.io/project/11278-cocito-weather-station
 *
 */
#define SOL_ADC A2     // Solar panel side voltage divider is connected to pin A0 
#define BAT_ADC A1    // Battery side voltage divider is connected to pin A1
#define AVG_NUM 10    // number of iterations of the adc routine to average the adc readings
#define BAT_MIN 10.5  // minimum battery voltage for 12V system
#define BAT_MAX 15.0  // maximum battery voltage for 12V system
#define BULK_CH_SP 14 // bulk charge set point for sealed lead acid battery // flooded type set it to 14.6V
#define FLOAT_CH_SP 13.5  //float charge set point for lead acid battery
#define LVD 12.2          //Low voltage disconnect setting for a 12V system (you could set it lower, i just wanted to be safe)
#define PWM_PIN 10         // pin-10 is used to control the charging MOSFET //the default frequency is 490.20Hz
#define LOAD_PIN 9       // pin-9 is used to control the load



//--------------------------------------------------------------------------------------------------------------------------
///////////////////////DECLARATION OF ALL GLOBAL VARIABLES//////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------------------------------------------
float solar_volt = 0;
float bat_volt = 0;
float load_current = 0;
float system_volt = 0;
float charge_status = 0;
float load_status = 0;
float error = 0;
float Ep = 0;
int duty = 0;
float msec = 0;
float last_msec = 0;
float elasped_msec = 0;
float elasped_time = 0;
float bulk_charge_sp = 0;
float float_charge_sp = 0;
float lvd = 0;
int flag = 0;

//******************************************************* MAIN PROGRAM START ************************************************
void setup()
{

  pinMode(PWM_PIN, OUTPUT);
  pinMode(LOAD_PIN, OUTPUT);
  digitalWrite(PWM_PIN, LOW); // default value of pwm duty cycle
  digitalWrite(LOAD_PIN, HIGH); // default load state is ON
  //Serial.begin(9600);        //used to calibrate adc
}

void loop()
{
  read_data();             // read different sensors data from analog pin of arduino
  system_voltage();        // detect the system voltage according to battery voltage
  setpoint();
  charge_cycle();         // pwm charging of battery
  load_control();         //control the load
  //calibration();        //used to calibrate adc
}
//************************************************************ PROGRAM END *************************************************


//------------------------------------------------------------------------------------------------------
////////////////// READS AND AVERAGES THE ANALOG INPUTS (SOLAR VOLTAGE,BATTERY VOLTAGE)////////////////
//------------------------------------------------------------------------------------------------------
int read_adc(int adc_parameter)
{

  int sum = 0;
  int sample ;
  for (int i = 0; i < AVG_NUM; i++)
  { // loop through reading raw adc values AVG_NUM number of times
    sample = analogRead(adc_parameter);    // read the input pin
    sum += sample;                        // store sum for averaging
    delayMicroseconds(50);              // pauses for 50 microseconds
  }
  return (sum / AVG_NUM);               // divide sum by AVG_NUM to get average and return it
}
//-------------------------------------------------------------------------------------------------------------
////////////////////////////////////READ THE DATA//////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------------------------------------------
void read_data(void)
{
  //5V = ADC value 1024 => 1 ADC value = (5/1024)Volt= 0.0048828Volt
  // Vout=Vin*R2/(R1+R2) => Vin = Vout*(R1+R2)/R2   R1=100 and R2=20
  //Solar panel R1=81k R2=14.81k   Batt R1=81k R2=15k         (values measured with multimeter)
  solar_volt = read_adc(SOL_ADC) * 0.0049 * (95.81 / 14.81); //tweak these numbers to get the perfect value
  bat_volt   = read_adc(BAT_ADC) * 0.0054 * (96 / 15);

}


//----------------------------------------------------------------------------------------------------------------------
//////////////////////////////////SYSTEM VOLTAGE AUTO DETECT ///////////////////////////////////////////////////////////
//----------------------------------------------------------------------------------------------------------------------
void system_voltage(void)
{
  if ((bat_volt > BAT_MIN) && (bat_volt < BAT_MAX))
  {
    system_volt = 12;
  }
  else if ((bat_volt > BAT_MIN / 2 ) && (bat_volt < BAT_MAX / 2))
  {
    system_volt = 6;
  }

}

//---------------------------------------------------------------------------------------------------------------------------
////////////////////////////////////CHARGE SET POINT ///////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------------------------------------------------------

void setpoint(void)
{
  if (system_volt == 12)
  {
    bulk_charge_sp = BULK_CH_SP;
    float_charge_sp = FLOAT_CH_SP;
    lvd = LVD;
  }

  else if (system_volt == 6)
  {
    bulk_charge_sp = (BULK_CH_SP / 2) ;
    float_charge_sp = (FLOAT_CH_SP / 2) ;
    lvd = LVD / 2;
  }
}

//--------------------------------------------------------------------------------------------------------------------------------
///////////////////////////////////////////////////PWM CHARGE CYCLE @490.2 HZ //////////////////////////////////////////////////
//-------------------------------------------------------------------------------------------------------------------------------
void charge_cycle(void)
{
  if (solar_volt > bat_volt && bat_volt <= bulk_charge_sp)
  {


    if (bat_volt <= float_charge_sp) // charging start
    {
      charge_status = 1; // indicate the charger is in BULK mode
      duty = 252.45;
      analogWrite(PWM_PIN, duty); // 99 % duty cycle // rapid charging


    }
    else if (bat_volt > float_charge_sp && bat_volt <= bulk_charge_sp)
    {
      charge_status = 2; // indicate the charger is in FLOAT mode
      error  = (bulk_charge_sp - bat_volt);      // duty cycle reduced when the battery voltage approaches the charge set point
      Ep = error * 100 ; //Ep= error* Kp // Assume  Kp=100

      if (Ep < 0)
      {
        Ep = 0;
      }
      else if (Ep > 100)
      {
        Ep = 100;
      }
      else if (Ep > 0 && Ep <= 100) // regulating
      {
        duty = (Ep * 255) / 100;
      }
      analogWrite(PWM_PIN, duty);
    }
  }
  else
  {
    charge_status = 0; // indicate the charger is OFF
    duty = 0;
    analogWrite(PWM_PIN, duty);
  }
}
//----------------------------------------------------------------------------------------------------------------------
/////////////////////////////////////////////LOAD CONTROL/////////////////////////////////////////////////////
//----------------------------------------------------------------------------------------------------------------------

void load_control() {
  if ((bat_volt > lvd) && (flag == 0)) // check if battery is healthy
  {
    load_status = 1;
    digitalWrite(LOAD_PIN, HIGH); // load is ON
  }
  else if (bat_volt < lvd)
  {
    load_status = 0;
    flag = 1;
    digitalWrite(LOAD_PIN, LOW); //load is OFF
  }
  if (bat_volt > (lvd + 0.3)) {
    flag = 0;
  }
}
void calibration() {
  delay(100);
  Serial.print("Solar Panel Voltage: ");
  Serial.print(solar_volt);
  Serial.println("V");
  Serial.print("Battery Voltage: ");
  Serial.print(bat_volt);
  Serial.println("V");
  Serial.print("Syestem Voltage: ");
  Serial.print(system_volt);
  Serial.println("V");
  Serial.print("Charge Set Point:");
  Serial.println(bulk_charge_sp);
  Serial.print("Duty Cycle :");
  if (charge_status == 1)
  {
    Serial.println("99%");
    Serial.println("BULK CHARGING");
  }
  else if (charge_status == 2)
  {
    Serial.print(Ep);
    Serial.println("%");
    Serial.println("FLOAT CHARGING");
  }
  else
  {
    Serial.println("0%");
    Serial.println("NOT CHARGING");
  }
  if (load_status == 1)
  {
    Serial.println("LOAD IS CONNECTED");
  }
  else
  {
    Serial.println("LOAD IS DISCONNECTED");
  }

  Serial.println("***************************");
}

