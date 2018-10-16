// This software is in the public domain
// It implements a Dell Latitude D630 Laptop Keyboard Controller and PS/2 Touchpad Controller
// using a Teensy 3.2 on a daughterboard with a 32 pin FPC connector. The keyboard part number 
// is DP/N 0DR160. The Touchpad from an HP DV9000 is part number 920-000702-04 Rev A.
// This routine uses the Teensyduino "Micro-Manager Method" to send Normal and Modifier 
// keys over USB. Multi-media keys are not supported by this routine.
// Description of Teensyduino keyboard functions is at www.pjrc.com/teensy/td_keyboard.html
// The PS/2 code was originally from https://playground.arduino.cc/uploads/ComponentLib/mouse.txt
// but the interface to the PC was changed from RS232 serial to USB using the PJRC Mouse functions. 
// A watchdog timer was also added to the "while loops" so the code doesn't hang if the touchpad hiccups.
//
// The test points on the touchpad were wired to a Teensy 3.2 as follows:
// T22 = 5V wired to the Teensy Vin pin
// T23 = Gnd wired to the Teensy Ground pin   It's hard to solder to T23 so I soldered to a bypass cap gnd pad.
// T10 = Clock wired to the Teensy I/O 14 pin  Pull up to 5 volts is in the touchpad
// T11 = Data wired to the Teensy I/O 15 pin   Pull up to 5 volts is in the touchpad
// 
// Revision History
// Initial Release Oct 16, 2018
//
//
#define MODIFIERKEY_FN 0x8f   // give Fn key a HID code 
#define CAPS_LED 13 // Teensy LED shows Caps-Lock
//
#define MDATA 15 // Touchpad ps/2 data connected to Teensy I/O pin 15
#define MCLK 14  // Touchpad ps/2 clock connected to Teensy I/O pin 14
//
//
const byte rows_max = 17; // sets the number of rows in the matrix
const byte cols_max = 8; // sets the number of columns in the matrix
//
// Load the normal key matrix with the Teensyduino key names described at www.pjrc.com/teensy/td_keyboard.html
// A zero indicates no normal key at that location.
//
int normal[rows_max][cols_max] = {
  {0,KEY_INSERT,0,KEY_F12,0,0,0,KEY_RIGHT},
  {0,KEY_DELETE,0,KEY_F11,0,0,0,KEY_DOWN},
  {KEY_UP,KEY_HOME,KEY_MENU,KEY_END,0,0,KEY_PAUSE,KEY_LEFT},
  {0,KEY_F8,KEY_F7,KEY_9,KEY_O,KEY_L,KEY_PERIOD,0},
  {KEY_QUOTE,KEY_MINUS,KEY_LEFT_BRACE,KEY_0,KEY_P,KEY_SEMICOLON,0,KEY_SLASH},
  {KEY_F6,KEY_EQUAL,KEY_RIGHT_BRACE,KEY_8,KEY_I,KEY_K,KEY_COMMA,0},
  {KEY_H,KEY_6,KEY_Y,KEY_7,KEY_U,KEY_J,KEY_M,KEY_N},
  {KEY_F5,KEY_F9,KEY_BACKSPACE,KEY_F10,0,KEY_BACKSLASH,KEY_ENTER,KEY_SPACE},
  {KEY_G,KEY_5,KEY_T,KEY_4,KEY_R,KEY_F,KEY_V,KEY_B},
  {KEY_F4,KEY_F2,KEY_F3,KEY_3,KEY_E,KEY_D,KEY_C,0},
  {0,KEY_F1,KEY_CAPS_LOCK,KEY_2,KEY_W,KEY_S,KEY_X,0},
  {KEY_ESC,KEY_TILDE,KEY_TAB,KEY_1,KEY_Q,KEY_A,KEY_Z,0},
  {0,0,0,KEY_PRINTSCREEN,KEY_NUM_LOCK,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,KEY_PAGE_UP,KEY_PAGE_DOWN,0,0},
  {0,0,0,0,0,0,0,0}
};
// Load the modifier key matrix with key names at the correct row-column location. 
// A zero indicates no modifier key at that location.
int modifier[rows_max][cols_max] = {
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},   
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {MODIFIERKEY_LEFT_ALT,0,0,0,0,0,0,MODIFIERKEY_RIGHT_ALT},    
  {0,0,MODIFIERKEY_LEFT_SHIFT,0,0,0,MODIFIERKEY_RIGHT_SHIFT,0},
  {0,MODIFIERKEY_LEFT_CTRL,0,0,0,0,MODIFIERKEY_RIGHT_CTRL,0},
  {0,0,0,MODIFIERKEY_GUI,0,0,0,0},
  {0,0,0,0,0,MODIFIERKEY_FN,0,0}
};
// Initialize the old_key matrix with one's. 
// 1 = key not pressed, 0 = key is pressed
boolean old_key[rows_max][cols_max] = {
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1}
};
//
// Define the Teensy 3.2 I/O numbers (translated from the FPC pin #)
// Row FPC pin # 02,03,04,05,06,07,08,09,10,11,12,13,14,15,16,17,18
// Teensy I/O  # 00,22,01,21,02,20,03,19,04,18,05,17,06,24,07,25,08
int Row_IO[rows_max] = {0,22,1,21,2,20,3,19,4,18,5,17,6,24,7,25,8}; // Teensy 3.2 I/O numbers for rows
//
// Column FPC pin # 19,20,21,22,23,24,25,26
// Teensy I/O     # 33,09,26,10,27,11,28,12
int Col_IO[cols_max] = {33,9,26,10,27,11,28,12};  // Teensy 3.2 I/O numbers for columns

// Declare variables that will be used by functions
boolean touchpad_error = LOW; // sent high when touch pad routine times out
boolean slots_full = LOW; // Goes high when slots 1 thru 6 contain normal keys
// slot 1 thru slot 6 hold the normal key values to be sent over USB. 
int slot1 = 0; //value of 0 means the slot is empty and can be used.  
int slot2 = 0; 
int slot3 = 0; 
int slot4 = 0; 
int slot5 = 0; 
int slot6 = 0;
//
int mod_shift_l = 0; // These variables are sent over USB as modifier keys.
int mod_shift_r = 0; // Each is either set to 0 or MODIFIER_ ... 
int mod_ctrl_l = 0;   
int mod_ctrl_r = 0;
int mod_alt_l = 0;
int mod_alt_r = 0;
int mod_gui = 0;
//
// Function to load the key name into the first available slot
void load_slot(int key) {
  if (!slot1)  {
    slot1 = key;
  }
  else if (!slot2) {
    slot2 = key;
  }
  else if (!slot3) {
    slot3 = key;
  }
  else if (!slot4) {
    slot4 = key;
  }
  else if (!slot5) {
    slot5 = key;
  }
  else if (!slot6) {
    slot6 = key;
  }
  if (!slot1 || !slot2 || !slot3 || !slot4 || !slot5 || !slot6)  {
    slots_full = LOW; // slots are not full
  }
  else {
    slots_full = HIGH; // slots are full
  } 
}
//
// Function to clear the slot that contains the key name
void clear_slot(int key) {
  if (slot1 == key) {
    slot1 = 0;
  }
  else if (slot2 == key) {
    slot2 = 0;
  }
  else if (slot3 == key) {
    slot3 = 0;
  }
  else if (slot4 == key) {
    slot4 = 0;
  }
  else if (slot5 == key) {
    slot5 = 0;
  }
  else if (slot6 == key) {
    slot6 = 0;
  }
  slots_full = LOW;
}
//
// Function to load the modifier key name into the appropriate mod variable
void load_mod(int m_key) {
  if (m_key == MODIFIERKEY_LEFT_SHIFT)  {
    mod_shift_l = m_key;
  }
  else if (m_key == MODIFIERKEY_RIGHT_SHIFT)  {
    mod_shift_r = m_key;
  }
  else if (m_key == MODIFIERKEY_LEFT_CTRL)  {
    mod_ctrl_l = m_key;
  }
  else if (m_key == MODIFIERKEY_RIGHT_CTRL)  {
    mod_ctrl_r = m_key;
  }
  else if (m_key == MODIFIERKEY_LEFT_ALT)  {
    mod_alt_l = m_key;
  }
  else if (m_key == MODIFIERKEY_RIGHT_ALT)  {
    mod_alt_r = m_key;
  }
  else if (m_key == MODIFIERKEY_GUI)  {
    mod_gui = m_key;
  }
}
//
// Function to load 0 into the appropriate mod variable
void clear_mod(int m_key) {
  if (m_key == MODIFIERKEY_LEFT_SHIFT)  {
    mod_shift_l = 0;
  }
  else if (m_key == MODIFIERKEY_RIGHT_SHIFT)  {
    mod_shift_r = 0;
  }
  else if (m_key == MODIFIERKEY_LEFT_CTRL)  {
    mod_ctrl_l = 0;
  }
  else if (m_key == MODIFIERKEY_RIGHT_CTRL)  {
    mod_ctrl_r = 0;
  }
  else if (m_key == MODIFIERKEY_LEFT_ALT)  {
    mod_alt_l = 0;
  }
  else if (m_key == MODIFIERKEY_RIGHT_ALT)  {
    mod_alt_r = 0;
  }
  else if (m_key == MODIFIERKEY_GUI)  {
    mod_gui = 0;
  }
}
//
// Function to send the modifier keys over usb
void send_mod() {
  Keyboard.set_modifier(mod_shift_l | mod_shift_r | mod_ctrl_l | mod_ctrl_r | mod_alt_l | mod_alt_r | mod_gui);
  Keyboard.send_now();
}
//
// Function to send the normal keys in the 6 slots over usb
void send_normals() {
  Keyboard.set_key1(slot1);
  Keyboard.set_key2(slot2);
  Keyboard.set_key3(slot3);
  Keyboard.set_key4(slot4);
  Keyboard.set_key5(slot5);
  Keyboard.set_key6(slot6);
  Keyboard.send_now();
}
//
// Function to set a pin to high impedance (acts like open drain output)
void go_z(int pin)
{
  pinMode(pin, INPUT);
  digitalWrite(pin, HIGH);
}
//
// Function to set a pin as an input with a pullup
void go_pu(int pin)
{
  pinMode(pin, INPUT_PULLUP);
  digitalWrite(pin, HIGH);
}
//
// Function to send a pin to a logic low
void go_0(int pin)
{
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}
//
// Function to send a pin to a logic high
void go_1(int pin)
{
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
}
//
// -----------Touchpad Functions--------------
// Function to send the Touchpad a command
void touchpad_write(char data)
{
  char i;
  char parity = 1;
 // put pins in output mode 
  go_z(MDATA);
  go_z(MCLK);
  elapsedMillis watchdog; // set watchdog to zero
  delayMicroseconds(300);
  go_0(MCLK);
  delayMicroseconds(300);
  go_0(MDATA);
  delayMicroseconds(10);
  // start bit 
  go_z(MCLK);
  // wait for touchpad to take control of clock)
  while (digitalRead(MCLK) == HIGH) {
    if (watchdog >= 200) { //check for infinite loop
      touchpad_error = HIGH; // set error flag
      break;
    }
  }
  // clock is low, and we are clear to send data 
  for (i=0; i < 8; i++) {
    if (data & 0x01) {
      go_z(MDATA);
    } 
    else {
      go_0(MDATA);
    }
    // wait for clock cycle 
    while (digitalRead(MCLK) == LOW) {
    if (watchdog >= 200) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break;
    }
  }      
    while (digitalRead(MCLK) == HIGH) {
    if (watchdog >= 200) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break;
    }
  }
    parity = parity ^ (data & 0x01);
    data = data >> 1;
  }  
  // parity 
  if (parity) {
    go_z(MDATA);
  } 
  else {
    go_0(MDATA);
  }
 // wait for clock cycle
  while (digitalRead(MCLK) == LOW) {
  if (watchdog >= 200) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break;
    }
  }  
  while (digitalRead(MCLK) == HIGH) {
    if (watchdog >= 200) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break;
    }
  }  
  // stop bit 
  go_z(MDATA);
  delayMicroseconds(50);
  while (digitalRead(MCLK) == HIGH) {
    if (watchdog >= 200) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break;
    }
  }
  // wait for touchpad to switch modes 
  while ((digitalRead(MCLK) == LOW) || (digitalRead(MDATA) == LOW)) {
    if (watchdog >= 200) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break;
    }
  }
  // put a hold on the incoming data. 
  go_0(MCLK);
}

//
// Function to get a byte of data from the touchpad
//
char touchpad_read(void)
{
  char data = 0x00;
  int i;
  char bity = 0x01;
  // start the clock 
  elapsedMillis watchdog; // set watchdog to zero
  go_z(MCLK);
  go_z(MDATA);
  delayMicroseconds(50);
  while (digitalRead(MCLK) == HIGH) {
    if (watchdog >= 200) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break;
    }
  }
  delayMicroseconds(5);  // wait for clock ring to settle 
  while (digitalRead(MCLK) == LOW) {  // eat start bit 
    if (watchdog >= 200) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break;
    }
  }
  for (i=0; i < 8; i++) {
    while (digitalRead(MCLK) == HIGH) {
    if (watchdog >= 200) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break;
    }
  }
    if (digitalRead(MDATA) == HIGH) {
      data = data | bity;
    }
    while (digitalRead(MCLK) == LOW) {
    if (watchdog >= 200) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break;
    }
  }
    bity = bity << 1;
  }
  // ignore parity bit  
  while (digitalRead(MCLK) == HIGH) {
    if (watchdog >= 200) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break;
    }
  }
  while (digitalRead(MCLK) == LOW) {
    if (watchdog >= 200) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break;
    }
  }
  // eat stop bit 
  while (digitalRead(MCLK) == HIGH) {
    if (watchdog >= 200) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break;
    }
  }
  while (digitalRead(MCLK) == LOW) {
    if (watchdog >= 200) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break;
    }
  }
  // put a hold on the incoming data. 
  go_0(MCLK);
  return data;
}

void touchpad_init()
{
  touchpad_error = LOW; // start with no error
  go_z(MCLK); // float the clock and data to touchpad
  go_z(MDATA);
  //  Sending reset to touchpad
  touchpad_write(0xff);
  touchpad_read();  // ack byte
  //  Read ack byte
  touchpad_read();  // blank 
  touchpad_read();  // blank 
  // Default resolution is 4 counts/mm which is too small
  //  Sending resolution command
  touchpad_write(0xe8);
  touchpad_read();  // ack byte
  touchpad_write(0x03); // value of 03 gives 8 counts/mm resolution
  touchpad_read();  // ack byte
  //  Sending remote mode code so the touchpad will send data only when polled
  touchpad_write(0xf0);  // remote mode 
  touchpad_read();  // Read ack byte 
  delayMicroseconds(100);
}
//----------------------------------Setup-------------------------------------------
void setup() {
  for (int a = 0; a < cols_max; a++) {  // loop thru all column pins 
    go_pu(Col_IO[a]); // set each column pin as an input with a pullup
  }
//
  for (int b = 0; b < rows_max; b++) {  // loop thru all row pins 
    go_z(Row_IO[b]); // set each row pin as a floating output
  }  
  touchpad_init(); // reset touchpad, then set it's resolution and put it in remote mode 
  if (touchpad_error) {
    touchpad_init(); // try one more time to initialize the touchpad
  }
}
//
boolean Fn_pressed = HIGH; // Initialize Fn key to HIGH = "not pressed"
extern volatile uint8_t keyboard_leds; // 8 bits sent from Host to Teensy that give keyboard LED status. Caps lock is bit D1.
//
// declare and initialize touchpad variables  
  char mstat; // touchpad status reg = Y overflow, X overflow, Y sign bit, X sign bit, Always 1, Middle Btn, Right Btn, Left Btn
  char mx; // touchpad x movement = 8 data bits. The sign bit is in the status register to 
           // make a 9 bit 2's complement value. Left to right on the touchpad gives a positive value. 
  char my; // touchpad y movement also 8 bits plus sign. Touchpad movement away from the user gives a positive value.
  boolean over_flow; // set if x or y movement values are bad due to overflow
  boolean left_button = 0; // on/off variable for left button = bit 0 of mstat
  boolean right_button = 0; // on/off variable for right button = bit 1 of mstat
  boolean old_left_button = 0; // on/off variable for left button status the previous polling cycle
  boolean old_right_button = 0; // on/off variable for right button status the previous polling cycle
  boolean button_change = 0; // Active high, shows when a touchpad left or right button has changed since last polling cycle

//---------------------------------Main Loop---------------------------------------------
//
void loop() {   
// Scan keyboard matrix with an outer loop that drives each row low and an inner loop that reads every column (with pull ups).
// The routine looks at each key's present state (by reading the column input pin) and also the previous state from the last scan
// that was 30msec ago. The status of a key that was just pressed or just released is sent over USB and the state is saved in the old_key matrix. 
// The keyboard keys will read as logic low if they are pressed (negative logic).
// The old_key matrix also uses negative logic (low=pressed). 
//
  for (int x = 0; x < rows_max; x++) {   // loop thru the rows
    go_0(Row_IO[x]); // Activate Row (send it low)
    delayMicroseconds(10); // give the row time to go low and settle out
    for (int y = 0; y < cols_max; y++) {   // loop thru the columns
// **********Modifier keys including the Fn special case
      if (modifier[x][y] != 0) {  // check if modifier key exists at this location in the array (a non-zero value)
        if (!digitalRead(Col_IO[y]) && (old_key[x][y])) {  // Read column to see if key is low (pressed) and was previously not pressed
          if (modifier[x][y] != MODIFIERKEY_FN) {   // Exclude Fn modifier key  
            load_mod(modifier[x][y]); // function reads which modifier key is pressed and loads it into the appropriate mod_... variable   
            send_mod(); // function sends the state of all modifier keys over usb including the one that just got pressed
            old_key[x][y] = LOW; // Save state of key as "pressed"
          }
          else {   
            Fn_pressed = LOW; // Fn status variable is active low
            old_key[x][y] = LOW; // old_key state is "pressed" (active low)
          }
        }
        else if (digitalRead(Col_IO[y]) && (!old_key[x][y])) {  //check if key is not pressed and was previously pressed
          if (modifier[x][y] != MODIFIERKEY_FN) { // Exclude Fn modifier key 
            clear_mod(modifier[x][y]); // function reads which modifier key was released and loads 0 into the appropriate mod_... variable
            send_mod(); // function sends all mod's over usb including the one that just released
            old_key[x][y] = HIGH; // Save state of key as "not pressed"
          }
          else {
            Fn_pressed = HIGH; // Fn is no longer active
            old_key[x][y] = HIGH; // old_key state is "not pressed" 
          }
        }
      } 
// ***********end of modifier section
//
// ***********Normal keys section
      else if (normal[x][y] != 0) {  // check if normal key exists at this location in the array (a non-zero value)
        if (!digitalRead(Col_IO[y]) && (old_key[x][y]) && (Fn_pressed)) { // check if key is pressed and was not previously pressed and no Fn pressed
          old_key[x][y] = LOW; // Save state of key as "pressed"
          load_slot(normal[x][y]); //update first available slot with normal key name
          send_normals(); // send all slots over USB including the key that just got pressed 
        }          
        else if (digitalRead(Col_IO[y]) && (!old_key[x][y])) { //check if key is not pressed, but was previously pressed 
          old_key[x][y] = HIGH; // Save state of key as "not pressed"
          clear_slot(normal[x][y]); //clear the slot that contains the normal key name
          send_normals(); // send all slots over USB including the key that was just released 
        }
      } 
// **************end of normal section
//
// ******Add Multi-Media key coding here if needed
//
    }
    go_z(Row_IO[x]); // De-activate Row (send it to hi-z)
  }
//
// **********keyboard scan complete
//
// Turn on the LED on the Teensy for Caps Lock based on bit 1 in the keyboard_leds variable controlled by the USB host computer
//
  if (keyboard_leds & 1<<1) {  // mask off all bits but D1 and test if set
    go_1(CAPS_LED); // turn on the LED
  }
  else {
    go_0(CAPS_LED); // turn off the LED
  }
//
// poll the touchpad for new movement data
  over_flow = 0; // assume no overflow until status is received 
  touchpad_error = LOW; // start with no error
  touchpad_write(0xeb);  // request data
  touchpad_read();      // ignore ack
  mstat = touchpad_read(); // save into status variable
  mx = touchpad_read(); // save into x variable
  my = touchpad_read(); // save into y variable
  if (((0x80 & mstat) == 0x80) || ((0x40 & mstat) == 0x40))  {   // x or y overflow bits set?
    over_flow = 1; // set the overflow flag
  }   
// change the x data from 9 bit to 8 bit 2's complement
  mx = mx >> 1; // convert to 7 bits of data by dividing by 2
  mx = mx & 0x7f; // don't allow sign extension
  if ((0x10 & mstat) == 0x10) {   // move the sign into 
    mx = 0x80 | mx;              // the 8th bit position
  } 
// change the y data from 9 bit to 8 bit 2's complement and then take the 2's complement 
// because y movement on ps/2 format is opposite of touchpad.move function
  my = my >> 1; // convert to 7 bits of data by dividing by 2
  my = my & 0x7f; // don't allow sign extension
  if ((0x20 & mstat) == 0x20) {   // move the sign into 
    my = 0x80 | my;              // the 8th bit position
  } 
  my = (~my + 0x01); // change the sign of y data by taking the 2's complement (invert and add 1)
// zero out mx and my if over_flow or touchpad_error is set
  if ((over_flow) || (touchpad_error)) { 
    mx = 0x00;       // data is garbage so zero it out
    my = 0x00;
  } 
// send the x and y data back via usb if either one is non-zero
  if ((mx != 0x00) || (my != 0x00)) {
    Mouse.move(mx,my);
  }
//
// send the touchpad left and right button status over usb if no error
  if (!touchpad_error) {
    if ((0x01 & mstat) == 0x01) {   // if left button set 
      left_button = 1;   
    }
    else {   // clear left button
      left_button = 0;   
    }
    if ((0x02 & mstat) == 0x02) {   // if right button set 
      right_button = 1;   
    } 
    else {   // clear right button
      right_button = 0;  
    }
// Determine if the left or right touch pad buttons have changed since last polling cycle
    button_change = (left_button ^ old_left_button) | (right_button ^ old_right_button);
// Don't send button status if there's no change since last time. 
    if (button_change){
      Mouse.set_buttons(left_button, 0, right_button); // send button status
    }
    old_left_button = left_button; // remember new button status for next polling cycle
    old_right_button = right_button;
  }
//
// End of touchpad routine
 
  delay(22); // The overall keyboard scanning rate is about 30ms
}