// Sparkle  v0.1
// Richard Jackett 2021
// Using Paul Hamshere's SparkPedalOverride code SparkClass.ino as its basis
// There will be things in here that aren't elegant, for which I apologise. I'm not a programmer. The goal is just to get it to work.

// Bugs: too many to count, but the big ones are:
//   Doesn't update parameters changed on amp, and in some other situations
//   Sometimes the left/right gesture doesn't change the effect until the direction is reversed.
//   Under some situations Sparkle can get out of sync with the amp. Need to look into this.


#include <M5Core2.h>
#include "SparkClass.h"
#include "SparkPresets.h"
#include "BluetoothSerial.h" // https://github.com/espressif/arduino-esp32

// Bluetooth vars
#define SPARK_NAME "Spark 40 Audio"
#define MY_NAME    "Sparkle"

#define BACKGROUND TFT_BLACK
#define TEXT_COLOUR TFT_WHITE

#define MIDI   68
#define STATUS 34
#define IN     102
#define OUT    172

#define STD_HEIGHT 30
#define PANEL_HEIGHT 68

#define MAX_EFFECT_NUM  50

BluetoothSerial SerialBT;

// Spark vars
SparkClass sc1, sc2, sc3, sc4, scr, scrp;
SparkClass sc_setpreset7f;

SparkPreset preset;
SparkClass sc_getserial;

// RJ

int pos;
int ret, parse_ret;

const int maxMode = 6;
int currentMode = 3;
int slidervalues[] = { 50,50,50,50,50 };
int currentItembyMode[] = { 0,0,0,0,0,0,0 };
bool currenteffectON = false;
int iExtendedSelection = 0;
int preset_cycler = -1;

const int maxItems[maxMode + 1]{
    sizeof(spark_noisegates) / sizeof(spark_noisegates[0]),
    sizeof(spark_compressors) / sizeof(spark_compressors[0]),
    sizeof(spark_drives) / sizeof(spark_drives[0]),
    sizeof(spark_amps) / sizeof(spark_amps[0]),
    sizeof(spark_modulations) / sizeof(spark_modulations[0]),
    sizeof(spark_delays) / sizeof(spark_delays[0]),
    sizeof(spark_reverbs) / sizeof(spark_reverbs[0])
};

const int maxItemsExtra[maxMode + 1]{
    sizeof(spark_noisegates) / sizeof(spark_noisegates[0]),
    sizeof(spark_compressors) / sizeof(spark_compressors[0]),
    sizeof(spark_drives_extra) / sizeof(spark_drives_extra[0]),
    sizeof(spark_amps_extra) / sizeof(spark_amps_extra[0]),
    sizeof(spark_modulations_extra) / sizeof(spark_modulations_extra[0]),
    sizeof(spark_delays) / sizeof(spark_delays[0]),
    sizeof(spark_reverbs) / sizeof(spark_reverbs[0])
};

char effect_to_return[STR_LEN + 1];

///  normal stuff

int i, j, p;
int pres[]{ 0,0x7f };

int cmd, sub_cmd;
char a_str[STR_LEN + 1];
char b_str[STR_LEN + 1];
int param;
float val;

unsigned long keep_alive;

// RJ
unsigned long last_button_press;
bool button_press;
bool driveON;

char thisPedal[STR_LEN + 1];
char lastPedal[STR_LEN + 1];

String thisSerialString;
String lastSerialString;
bool found_preset;


// ------------------------------------------------------------------------------------------
// Display routintes

// Display vars
#define DISP_LEN 50
char outstr[DISP_LEN+1];
char instr[DISP_LEN+1];
char statstr[DISP_LEN+1];
int bar_pos;
unsigned long bar_count;

// DISPLAY
char AllModes[][4 + 1]{ "GATE","COMP","DRV","AMP","MOD","DLY","RVB" };

// FORMAT DISPLAY
const int dispright = 319;
const int dispbottom = 239;
// sliders
const int sliderleft = 80;
const int sliderdown = 30;
const int sliderspacing = 50;
const int sliderlength = 140;
const int sliderwidth = 30;
// mode selector -- modes are COMP, DRV, AMP, MOD, etc
const int modebarleft = 5;
const int modebardown = 12;
const int modebarwidth = 50;
const int modebarlength = 200;
const int modespacing = 24;
// items selector -- items are the individual effects or amp models
const int itemzonedown = 190;
const int itemtextleft = 15;  // if left aligned
const int itemtextcentre = dispright / 2;  // if centre aligned
const int itemtextdown = 190;
// connection status light
const int statuslightx = 310;
const int statuslighty = 230;
const int statuslightrad = 4;
// normal/extra selection thing
const int selectionx = 55;
const int selectiony = 230;
// preset text
const int presetx = 265;
const int presety = 230;


//TFT_eSprite disp(&M5.Lcd);
//TFT_eSprite disp = TFT_eSprite(&M5.Lcd);
bool first_scroll = true;
bool show_move = false;
bool long_press = true;
bool key_repeat = false;


ButtonColors onCol = { BLACK, WHITE, WHITE };
ButtonColors offCol = { RED, WHITE, WHITE };

// Buttons
Button slider0(sliderleft, sliderdown, sliderwidth, sliderlength, false, "0", onCol, offCol);
Button slider1(sliderleft + 1 * sliderspacing, sliderdown, sliderwidth, sliderlength, false, "1", onCol, offCol);
Button slider2(sliderleft + 2 * sliderspacing, sliderdown, sliderwidth, sliderlength, false, "2", onCol, offCol);
Button slider3(sliderleft + 3 * sliderspacing, sliderdown, sliderwidth, sliderlength, false, "3", onCol, offCol);
Button slider4(sliderleft + 4 * sliderspacing, sliderdown, sliderwidth, sliderlength, false, "4", onCol, offCol);

// Gestures
Zone modezone(0, 0, sliderleft - 5, modebarlength);
Zone itemzone(0, itemzonedown, 319, 239);

Gesture nextMode(modezone, modezone, "Next Mode", 20, DIR_DOWN, 30, false, 1000U);
Gesture lastMode(modezone, modezone, "Last Mode", 20, DIR_UP, 30, false, 1000U);

Gesture nextItem(itemzone, itemzone, "Next Item", 20, DIR_RIGHT, 30, false, 1000U);
Gesture lastItem(itemzone, itemzone, "Last Item", 20, DIR_LEFT, 30, false, 1000U);


void display_bar()
{
 /*  if (millis() - bar_count > 400) {
      bar_count = millis();
      M5.Lcd.fillRoundRect(15 + bar_pos*30, STATUS + STD_HEIGHT + 10 , 15, 15, 4, BACKGROUND);
      bar_pos ++;
      bar_pos %= 10;
      M5.Lcd.fillRoundRect(15 + bar_pos*30, STATUS + STD_HEIGHT + 10, 15, 15, 4, TEXT_COLOUR);
   }*/
}

void display_val(float val)
{
   //int dist;

   //dist = int(val * 290);
   //M5.Lcd.fillRoundRect(15 + dist, IN + STD_HEIGHT + 10 , 290 - dist, 15, 4, BACKGROUND);
   //M5.Lcd.drawRoundRect(15, IN + STD_HEIGHT + 10 , 290 , 15, 4, TEXT_COLOUR);
   //M5.Lcd.fillRoundRect(15, IN + STD_HEIGHT + 10 , dist, 15, 4, TEXT_COLOUR);
}

void display_str(const char *a_str, int y)
{
   //char b_str[30];

   //strncpy(b_str, a_str, 25);
   //if (strlen(a_str) < 25) strncat(b_str, "                         ", 25-strlen(a_str));
   //M5.Lcd.setCursor(8,y+8);
   //M5.Lcd.print(b_str);

    Serial.println(a_str);

}

// ------------------------------------------------------------------------------------------
// Bluetooth routines

bool connected;
int bt_event;

void btEventCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param){
  // On BT connection close
  if (event == ESP_SPP_CLOSE_EVT ){
    // TODO: Until the cause of connection instability (compared to Pi version) over long durations 
    // is resolved, this should keep your pedal and amp connected fairly well by forcing reconnection
    // in the main loop
    connected = false;
  }
  if (event != ESP_SPP_CLOSE_EVT ) {
     bt_event = event;
  }
}

void start_bt() {
   SerialBT.register_callback(btEventCallback);
   
   if (!SerialBT.begin (MY_NAME, true)) {
      display_str("Bluetooth init fail", STATUS);
      while (true);
   }    
   connected = false;
   bt_event = 0;
}

void connect_to_spark() {
   int rec;

   status_light(ORANGE);
   while (!connected) {
      update_itemtext("   Connecting to Spark40... ", WHITE, BLACK);
      display_str("Connecting to Spark", STATUS);
      connected = SerialBT.connect(SPARK_NAME);
      if (connected && SerialBT.hasClient()) {
         update_itemtext("Connected to Spark40", WHITE, BLACK);
         display_str("Connected to Spark", STATUS);
         status_light(GREEN);
      }
      else {
         connected = false;
         delay(3000);
      }
   }
   // flush anything read from Spark - just in case

   while (SerialBT.available())
      rec = SerialBT.read(); 
}

// ----------------------------------------------------------------------------------------
// Send messages to the Spark (and receive an acknowledgement where appropriate

void send_bt(SparkClass& spark_class)
{
   int i;

   // if multiple blocks then send all but the last - the for loop should only run if last_block > 0
   for (i = 0; i < spark_class.last_block; i++) {
      SerialBT.write(spark_class.buf[i], BLK_SIZE);
 
   }
      
   // and send the last one      
   SerialBT.write(spark_class.buf[spark_class.last_block], spark_class.last_pos+1);

}


// Helper functions to send an acknoweledgement and to send a requesjames luit for a preset

void send_ack(int seq, int cmd)
{
   byte ack[]{  0x01, 0xfe, 0x00, 0x00, 0x41, 0xff, 0x17, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0xf0, 0x01, 0xff, 0x00, 0x04, 0xff, 0xf7};
   ack[18] = seq;
   ack[21] = cmd;            

   SerialBT.write(ack, sizeof(ack));
}

void send_preset_request(int preset)
{
   byte req[]{0x01, 0xfe, 0x00, 0x00, 0x53, 0xfe, 0x3c, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0xf0, 0x01, 0x04, 0x00, 0x02, 0x01,
              0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0xf7};
 
   req[24] = preset;
   SerialBT.write(req, sizeof(req));      
}

void send_current_state_request()
{
    byte req[]{ 0x01, 0xfe, 0x00, 0x00, 0x53, 0xfe, 0x3c, 0x00,
               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
               0xf0, 0x01, 0x04, 0x00, 0x02, 0x01,
               0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
               0x00, 0x00, 0x00, 0x00, 0x00, 0xf7 };

    SerialBT.write(req, sizeof(req));
}

void expect_ack()
{
    int ret;
    int parse_ret;
    int i;

    // this is complex because it is checking the data received - you really only
    // need to do scr.get_data()

    display_str("Waiting for Ack", STATUS);
    ret = scr.get_data();
    if (ret >= 0) {
        display_str("Got responses", STATUS);
        parse_ret = scr.parse_data();
        if (parse_ret < 0)
            display_str("Parse failed", STATUS);
        else
            if (scr.num_messages > 0)
                for (i = 0; i < scr.num_messages; i++)
                    if (scr.messages[i].cmd == 0x04)
                        display_str("Got ack", STATUS);
                    else {
                        snprintf(statstr, DISP_LEN - 1, "Got something %2.2X %2.2X", scr.messages[i].cmd, scr.messages[i].sub_cmd);
                        display_str(statstr, STATUS);
                    }
    }
    else
        display_str("Bad block received", STATUS);
}

void send_receive_bt(SparkClass& spark_class)
{
   int i;
   int rec;

   // if multiple blocks then send all but the last - the for loop should only run if last_block > 0
   for (i = 0; i < spark_class.last_block; i++) {
      SerialBT.write(spark_class.buf[i], BLK_SIZE);
      // read the ACK                                     COULD REPLACE WITH A CALL TO expect_ack() HERE
      rec = scr.get_data();
      if (rec <0) {
         Serial.print("Receive error "); 
         Serial.println(rec);
      }
   }
      
   // and send the last one      
   SerialBT.write(spark_class.buf[spark_class.last_block], spark_class.last_pos+1); 
   
   // read the ACK                                      COULD REPLACE WITH A CALL TO expect_ack() HERE
      rec = scr.get_data();
      if (rec <0) {
         Serial.print("Receive error "); 
         Serial.println(rec);
      } 
}


// ------------------------------------------------------------------------------------------

void setup() {
   M5.begin();
   Serial.begin(9600);
   Serial.println("\n\n*** Sparkle - A Tiny Spark40 Interface ***");
         
   // SETUP DISPLAY
   show_splash();
   //setup_disp();
   setup_events();


   // slider handlers
   slider0.addHandler(SliderEvent, E_ALL);
   slider1.addHandler(SliderEvent, E_ALL);
   slider2.addHandler(SliderEvent, E_ALL);
   slider3.addHandler(SliderEvent, E_ALL);
   slider4.addHandler(SliderEvent, E_ALL);

   // gestures
   nextMode.addHandler(modeHandler, E_GESTURE);
   lastMode.addHandler(modeHandler, E_GESTURE);
   nextItem.addHandler(itemHandler, E_GESTURE);
   lastItem.addHandler(itemHandler, E_GESTURE);

   // SETUP CONNECTION
   start_bt();
   connect_to_spark();

   pos = 0;

   keep_alive = millis();
   last_button_press = millis();
   
   // set up the change to 7f message for when we send a full preset
   sc_setpreset7f.change_hardware_preset(0x7f);
   sc_getserial.get_serial();

   // draw stuff on the screen now
   setup_disp();
   status_light(GREEN);
   draw_ModeBar();
   //update_itemtext("   Connecting to Spark40... ", WHITE, BLACK);
   M5.Buttons.draw();
   update_selection_text();
   //DisplayParameters();

   // kick things off with a request for current amp state
   send_current_state_request();

}

void loop() {
   int av;
   int ret;
   int ct;
   
   M5.update();

   // this will connect if not already connected
   if (!connected) {
       connect_to_spark();
       send_current_state_request();
   }

   // see if this keeps the connection alive
   if (millis() - keep_alive  > 10000) {
      keep_alive = millis();
      send_receive_bt(sc_getserial);
   }
   
    // RECIEVE INFO FROM SPARK
    // This is mostly still Paul's code. Thanks Paul!

    if (SerialBT.available()) {
        display_str("Getting data from Spark", STATUS);
        ret = scr.get_data();
        if (ret < 0) {
            snprintf(statstr, DISP_LEN - 1, "Corrupted data %d", ret);
            display_str(statstr, STATUS);
        }
        if (ret >= 0) {
            display_str("Parsing the data", STATUS);
            parse_ret = scr.parse_data();
            if (parse_ret >= 0) {
                if (scr.num_messages > 0) {
                    snprintf(statstr, DISP_LEN - 1, "Processing %d messages", scr.num_messages);
                    display_str(statstr, STATUS);
                }
                for (i = 0; i < scr.num_messages; i++) {
                    Serial.printf("Msg %i Cmd %2.2X %2.2X\n", i, scr.messages[i].cmd, scr.messages[i].sub_cmd);
                    cmd = scr.messages[i].cmd;
                    sub_cmd = scr.messages[i].sub_cmd;
                    if (cmd == 0x03 && sub_cmd == 0x37) {
                        scr.get_effect_parameter(i, a_str, &param, &val);
                        // need to put val into my slider array
                        if (!strncmp(a_str, current_effects[currentMode], STR_LEN-1)) {
                            Serial.println("Param is current effect");
                            slidervalues[param] = val * 100;
                            //update_display();  // this sends sparkle and amp into infinite loop!
                        }
                        //update_display();
                        snprintf(instr, DISP_LEN - 1, "%s %d %0.2f", a_str, param, val);
                        display_str(instr, IN);
                    }
                    else if (cmd == 0x03 && sub_cmd == 0x06) {
                        // it can only be an amp change if received from the Spark
                        scr.get_effect_change(i, a_str, b_str);
                        strncpy(current_effects[3], b_str, STR_LEN);  // was STR_LEN-1 but should be +1?
                        currentMode = 3;
                        Serial.println("Changed amp selection on Spark40");
                        //snprintf(instr, DISP_LEN - 1, "-> %s", b_str);
                        //display_str(instr, IN);
                        update_display();

                    }
                    else if (cmd == 0x03 && sub_cmd == 0x01) {
                        scr.get_preset(i, &preset);
                        Serial.printf("Got Preset: %s\n",preset.Name);
                        // update the current effects list
                        for (j = 0; j < 7; j++) {
                            strncpy(current_effects[j], preset.effects[j].EffectName, STR_LEN);   // was STR_LEN-1 but should be +1?
                            //Serial.printf("%s (%d): param0 = %.2f\n",preset.effects[j].EffectName, preset.effects[j].OnOff, preset.effects[j].Parameters[0]);
                        }
                        // update display
                        update_display();

                        for (j = 0; j < 7; j++) {
                            Serial.printf("%s %s [%.2f,%.2f,%.2f,%.2f,%.2f,%.2f]\n", preset.effects[j].EffectName, preset.effects[j].OnOff ? "ON" : "OFF",
                                preset.effects[j].Parameters[0], preset.effects[j].Parameters[1], preset.effects[j].Parameters[2], preset.effects[j].Parameters[3],
                                preset.effects[j].Parameters[4], preset.effects[j].Parameters[5]);
                        }

                    }
                    else {
                        //snprintf(instr, DISP_LEN - 1, "Command %2.2X %2.2X", scr.messages[i].cmd, scr.messages[i].sub_cmd);
                        //display_str(instr, IN);
                        //for (j = 0; j < 7; j++) Serial.printf("Effect %i = %s\n", j, current_effects[j]);
                        //update_display();
                    }
                }
            }
            display_str("Done", STATUS);
        }
    }

    // BUTTON PUSH  -- these could have been events but they aren't. No reason.

    if (M5.BtnB.wasPressed()) {  // Turn current effect ON/OFF
        
        char* onoff;

        /// Find current state
        if (!preset.effects[currentMode].OnOff) { // turn on
            onoff = "On";
            update_itemtext(current_effects[currentMode], YELLOW, BLACK);
        }
        else {
            update_itemtext(current_effects[currentMode], WHITE, BLACK);;
            onoff = "Off";
        }
          
        sc2.turn_effect_onoff(current_effects[currentMode], onoff);
        send_bt(sc2);

        Serial.printf("Turn %s to %s\n", current_effects[currentMode], onoff);

        send_current_state_request();

    }

    if (M5.BtnA.wasPressed()) {  // Change selection mode between standard and extra

        iExtendedSelection++;
        if (iExtendedSelection > 1) {
            iExtendedSelection = 0;
        }

        update_selection_text();

    }

    if (M5.BtnC.wasPressed()) {  // switch between hardware presets

        preset_cycler++;

        if (preset_cycler > 3) {preset_cycler = 0;}

        sc2.change_hardware_preset(preset_cycler);
        send_receive_bt(sc2);
        send_receive_bt(sc_setpreset7f);

        update_preset_text();

        // Behaviour is quite unsatisfactory. It doesn't update the display well.
        // It can't tell the difference between this instruction and the one to get current state.
        

        //// This just outputs current state.
        //Serial.println(preset.Name);
        //for (j = 0; j < 7; j++) {
        //    Serial.printf("%s %s [%.2f,%.2f,%.2f,%.2f,%.2f,%.2f]\n", preset.effects[j].EffectName, preset.effects[j].OnOff ? "ON" : "OFF",
        //        preset.effects[j].Parameters[0], preset.effects[j].Parameters[1], preset.effects[j].Parameters[2], preset.effects[j].Parameters[3],
        //        preset.effects[j].Parameters[4], preset.effects[j].Parameters[5]);
        //}

        // update_display();
    }
      
  if (Serial.available() > 0) {   // this has been repurposed to change to a preset name given over Serial
                                  // No error checking here!

      lastSerialString = thisSerialString;
      
      thisSerialString = Serial.readString();

      Serial.println(thisSerialString);

      lastSerialString.toCharArray(lastPedal, STR_LEN + 1);
      thisSerialString.toCharArray(thisPedal, STR_LEN + 1);

      int num_presets = sizeof(presets) / sizeof(SparkPreset*);
      int k;

      
        // find the current[] effect of this type
        found_preset = false;

        for (j = 0; j < num_presets; j++) {
            if (!strncmp(presets[j]->Name, thisPedal, STR_LEN - 1)) {
                found_preset = true;
                break;
            }
        }
        if (found_preset) {
            sc2.create_preset(*presets[j]);
            send_receive_bt(sc2);
            // and send the select hardware 7f message too
            send_receive_bt(sc_setpreset7f);

            Serial.printf("Preset Activated: %s\n", presets[j]->Name);
            
            for (k = 0; k < 7; k++) strncpy(current_effects[k], presets[j]->effects[k].EffectName, STR_LEN - 1);
        }
      
        send_current_state_request();

      //display_str(outstr, OUT);
      //Serial.println(outstr);

      //sc2.change_effect(lastPedal,thisPedal);

      //send_receive_bt(sc2);
      //ret = scr.get_effect_change(i, a_str, b_str);

      //snprintf(instr, DISP_LEN - 1, "%s -> %s", a_str, b_str);
      //display_str(instr, IN);
      //Serial.println(instr);

  }

}

int get_item(int mode, int effectnum) {
    
    if (effectnum < 0) {
        effectnum = maxItems[mode] - 1;
    }
    else if (effectnum > maxItems[mode] - 1) {
        effectnum = 0;
    }

    switch (mode)
    {
        case 0:
            strncpy(effect_to_return, spark_noisegates[effectnum], STR_LEN + 1);
            Serial.printf("0-%i:%s\n", effectnum, spark_noisegates[effectnum]);
            return effectnum;
        case 1:
            strncpy(effect_to_return, spark_compressors[effectnum], STR_LEN + 1);
            Serial.printf("1-%i:%s\n", effectnum, spark_compressors[effectnum]);
            return effectnum;
        case 2:
            strncpy(effect_to_return, spark_drives[effectnum], STR_LEN + 1);
            Serial.printf("2-%i:%s\n", effectnum, spark_drives[effectnum]); // remember I experiment here
            return effectnum;
        case 3:
            strncpy(effect_to_return, spark_amps[effectnum], STR_LEN + 1);
            Serial.printf("3-%i:%s\n", effectnum, spark_amps[effectnum]);
            return effectnum;
        case 4:
            strncpy(effect_to_return, spark_modulations[effectnum], STR_LEN + 1);
            Serial.printf("4-%i:%s\n", effectnum, spark_modulations[effectnum]);
            return effectnum;
        case 5:
            strncpy(effect_to_return, spark_delays[effectnum], STR_LEN + 1);
            Serial.printf("5-%i:%s\n", effectnum, spark_delays[effectnum]);
            return effectnum;
        case 6:
            strncpy(effect_to_return, spark_reverbs[effectnum], STR_LEN + 1);
            Serial.printf("6-%i:%s\n", effectnum, spark_reverbs[effectnum]);
            return effectnum;
    }

    Serial.printf("get_item.effect_to_return = %s, mode = %i, effectnum = %i\n",effect_to_return,mode,effectnum);
    //Serial.printf("Test get_item.effect_to_return = %s, mode = %i, effectnum = %i\n", effect_to_return, mode, effectnum);

    return -1;
}

int get_item_extra(int mode, int effectnum) {

    if (effectnum < 0) {
        effectnum = maxItemsExtra[mode] - 1;
    }
    else if (effectnum > maxItemsExtra[mode] - 1) {
        effectnum = 0;
    }

    switch (mode)
    {
    case 0:
        strncpy(effect_to_return, spark_noisegates[effectnum], STR_LEN + 1);
        Serial.printf("0-%i:%s\n", effectnum, spark_noisegates[effectnum]);
        return effectnum;
    case 1:
        strncpy(effect_to_return, spark_compressors[effectnum], STR_LEN + 1);
        Serial.printf("1-%i:%s\n", effectnum, spark_compressors[effectnum]);
        return effectnum;
    case 2:
        strncpy(effect_to_return, spark_drives_extra[effectnum], STR_LEN + 1);
        Serial.printf("2-%i:%s\n", effectnum, spark_drives_extra[effectnum]); 
        return effectnum;
    case 3:
        strncpy(effect_to_return, spark_amps_extra[effectnum], STR_LEN + 1);
        Serial.printf("3-%i:%s\n", effectnum, spark_amps_extra[effectnum]);
        return effectnum;
    case 4:
        strncpy(effect_to_return, spark_modulations_extra[effectnum], STR_LEN + 1);
        Serial.printf("4-%i:%s\n", effectnum, spark_modulations_extra[effectnum]);
        return effectnum;
    case 5:
        strncpy(effect_to_return, spark_delays[effectnum], STR_LEN + 1);
        Serial.printf("5-%i:%s\n", effectnum, spark_delays[effectnum]);
        return effectnum;
    case 6:
        strncpy(effect_to_return, spark_reverbs[effectnum], STR_LEN + 1);
        Serial.printf("6-%i:%s\n", effectnum, spark_reverbs[effectnum]);
        return effectnum;
    }

    Serial.printf("ERROR with mode in get_item_extra.effect_to_return = %s, mode = %i, effectnum = %i\n", effect_to_return, mode, effectnum);

    return -1;
}

void setup_disp() {
    //disp.createSprite(1, 1);
    //disp.setScrollRect(0, 0, 320, 240);
    ////disp.fillSprite(WHITE);
    //disp.setTextFont(2);
    //disp.setTextSize(2);
    //disp.setTextColor(WHITE, BLACK);
    //disp.setCursor(0, 0);
    
    M5.Lcd.clearDisplay(BLACK);
    M5.Lcd.setTextFont(1);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE, BLACK);

    //disp.pushSprite(0, 0);
}

// Remove/replace event handlers, since list of evetns could change.
//
void setup_events() { 
    M5.background.delHandlers();
    // uint16_t events = (show_move) ? E_ALL : (E_ALL - E_MOVE); // Show all events, or everything but E_MOVE? Controlled with A button.
    M5.background.longPressTime = (long_press) ? 500 : 0;                // Detect long presses (500mS) or not? Controlled with B button.
    M5.background.repeatDelay = (key_repeat) ? 200 : 0;                // Repeat press events every 200mS or not? Controlled with the C button.
}

void modeHandler(Event& e) {
    // Mode is the type of effect that is active in the UI (e.g. Amp, Drive, Mod, etc)
    // The state of currentMode is only determined and used by this app...
    // ...the Spark never sees or sends a current Mode change
    
    // is it next or previous mode?
    if (!strcmp(e.gesture->getName(), "Next Mode")) {
        Serial.printf("NM: %i to ", currentMode);
        currentMode++;
        Serial.printf("%i to ", currentMode);
        if (currentMode > maxMode) { currentMode = 0; } // wrap
        Serial.printf("%i\n", currentMode);
    }

    if (!strcmp(e.gesture->getName(), "Last Mode")) {
        Serial.printf("LM: %i to ", currentMode);
        currentMode--;
        Serial.printf("%i to ", currentMode);
        if (currentMode < 0) { currentMode = maxMode; } // wrap
        Serial.printf("%i\n", currentMode);
    }
    
    // update display
    draw_ModeBar();

    Serial.printf("%s to %i %s: %s\n", e.gesture->getName(), currentMode, AllModes[currentMode], current_effects[currentMode]);

    // better update the display with the current settings...
    send_current_state_request();

}

void itemHandler(Event& e) {
    
    if (!strcmp(e.gesture->getName(), "Next Item")) {
        currentItembyMode[currentMode]++;
        //if (currentItembyMode[currentMode] > maxItems[currentMode]-1) currentItembyMode[currentMode] = 0; // wrap -- this is now handled inside get_item()
    }

    if (!strcmp(e.gesture->getName(), "Last Item")) {
        currentItembyMode[currentMode]--;
        //if (currentItembyMode[currentMode] < 0) currentItembyMode[currentMode] = maxItems[currentMode]-1; // wrap
    }

    if (iExtendedSelection > 0) {  // this might be terrible practice, but it stores the next effect in: effect_to_return but doesn't pass this, even as a pointer. Eek.
        currentItembyMode[currentMode] = get_item_extra(currentMode, currentItembyMode[currentMode]);
    }
    else {
        currentItembyMode[currentMode] = get_item(currentMode, currentItembyMode[currentMode]);
    }

    if (currentItembyMode[currentMode] > -1) {
        sc2.change_effect(current_effects[currentMode], effect_to_return);
        send_receive_bt(sc2);
        Serial.printf("Change Item from %s to %s\n", current_effects[currentMode], effect_to_return);
    }

    // Update Display
    //update_itemtext(current_effects[currentMode],WHITE, BLACK);  // might be unnecessary now that I am getting current state
    //DisplayParameters();

    send_current_state_request();
    
}

void SliderEvent(Event& e) {

    if (e.type & E_MOVE) {

        if (e.isDirection(0, 20)) {
            Serial.println(printf("%s +%i %i", e.objName(), e.distance(), e.button->instanceIndex()));
            slidervalues[e.button->instanceIndex() - 4] += e.distance();
        }
        else if (e.isDirection(180, 20)) {
            Serial.println(printf("%s -%i %i", e.objName(), e.distance(), e.button->instanceIndex()));
            slidervalues[e.button->instanceIndex() - 4] -= e.distance();
        }

        if (slidervalues[e.button->instanceIndex() - 4] < 0) slidervalues[e.button->instanceIndex() - 4] = 0;
        if (slidervalues[e.button->instanceIndex() - 4] > 100) slidervalues[e.button->instanceIndex() - 4] = 100;

        DisplayParameters();

    }

    if (e.type & E_DBLTAP) {

        if (slidervalues[e.button->instanceIndex() - 4] == 0) {
            slidervalues[e.button->instanceIndex() - 4] = 100;
        }
        else {
            slidervalues[e.button->instanceIndex() - 4] = 0;
        }
        
        Serial.println(printf("E_DBLTAP %s %i %i", e.objName(), e.button->instanceIndex(), slidervalues[e.button->instanceIndex() - 4]));
        
    }

    // print values on screen
    //DisplayParameters();

    // send the value to Spark only on lifting finger
    if (e.type & E_RELEASE) {
        // need to change a parameter value
        // need to know the current item but not mode?
        // the parameter number is: e.button->instanceIndex() - 4
        // the new value is: slidervalues[e.button->instanceIndex() - 4]/100

        sc2.change_effect_parameter(current_effects[currentMode],
            e.button->instanceIndex() - 4,
            static_cast<float>(slidervalues[e.button->instanceIndex() - 4]) / static_cast<float>(100));

        Serial.printf("Change Param: %s (%i) = %i\n", current_effects[currentMode], e.button->instanceIndex() - 4, slidervalues[e.button->instanceIndex() - 4]);
        
        send_bt(sc2);

        //send_current_state_request();

    }

    //send_current_state_request();

}

void DisplayParameters() {
    // these are the values printed above the sliders
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setTextFont(1);
    M5.Lcd.setTextSize(2);

    M5.Lcd.fillRect(sliderleft - 10, 0, 319 - sliderleft + 10, sliderdown - 1, BLACK);
    M5.Lcd.drawNumber(slidervalues[0], sliderleft + 15, sliderdown - 20);
    M5.Lcd.drawNumber(slidervalues[1], sliderleft + 1 * sliderspacing + 15, sliderdown - 20);
    M5.Lcd.drawNumber(slidervalues[2], sliderleft + 2 * sliderspacing + 15, sliderdown - 20);
    M5.Lcd.drawNumber(slidervalues[3], sliderleft + 3 * sliderspacing + 15, sliderdown - 20);
    M5.Lcd.drawNumber(slidervalues[4], sliderleft + 4 * sliderspacing + 15, sliderdown - 20);

}

void status_light(uint32_t colour) {
    // the connection status
    M5.Lcd.fillCircle(statuslightx, statuslighty, statuslightrad, colour);
}

void show_splash() {

    //M5.Lcd.setTextFont(1);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setTextColor(YELLOW, BLACK);
    M5.Lcd.drawCentreString("Sparkle",itemtextcentre,90,1);
    M5.Lcd.setTextFont(1);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(WHITE, BLACK);

}

void update_selection_text() {

    uint32_t textcolour = M5.Lcd.textcolor;
    uint32_t textfont = M5.Lcd.textfont;
    uint32_t textsize = M5.Lcd.textsize;

    M5.Lcd.setTextColor(BLACK, BLACK);
    M5.Lcd.setTextFont(1);
    M5.Lcd.setTextSize(1);

    // kill the old stuff
    M5.Lcd.drawCentreString("Normal", selectionx, selectiony, 1);
    M5.Lcd.drawCentreString("Extra", selectionx, selectiony, 1);

    // write the new stuff
    if (iExtendedSelection == 0) {
        M5.Lcd.setTextColor(LIGHTGREY);
        M5.Lcd.drawCentreString("Normal", selectionx, selectiony, 1);
    }
    else if (iExtendedSelection == 1) {
        M5.Lcd.setTextColor(PURPLE);
        M5.Lcd.drawCentreString("Extra", selectionx, selectiony, 1);
    }

    M5.Lcd.setTextColor(textcolour, BLACK);
    M5.Lcd.setTextFont(textfont);
    M5.Lcd.setTextSize(textsize);

}

void update_preset_text() {

    if (preset_cycler < 0) return;

    uint32_t textcolour = M5.Lcd.textcolor;
    uint32_t textfont = M5.Lcd.textfont;
    uint32_t textsize = M5.Lcd.textsize;

    M5.Lcd.setTextColor(BLACK, BLACK);
    M5.Lcd.setTextFont(1);
    M5.Lcd.setTextSize(1);

    // kill the old stuff
    M5.Lcd.fillRect(presetx-30, presety-10, 60,20,BLACK);
    
    // write the new stuff
    M5.Lcd.setTextColor(LIGHTGREY);
    char printtext[4+1];
    sprintf(printtext, "HW %i", preset_cycler);
    
    M5.Lcd.drawCentreString(printtext, presetx, presety, 1);
    M5.Lcd.setTextColor(textcolour, BLACK);
    M5.Lcd.setTextFont(textfont);
    M5.Lcd.setTextSize(textsize);

}

void update_display() {  // fonts and sizes have been a complete pain. It is working now, but look into using a better font and actually understanding how fonts work.

    //disp.setTextFont(2);
    //disp.setTextSize(2);
    int colour1 = WHITE;
    int colour2 = BLACK;

    // Redraw mode bar
    draw_ModeBar();

    // Effect Name and state
    if (preset.effects[currentMode].OnOff) { colour1 = YELLOW; }
    update_itemtext(preset.effects[currentMode].EffectName, colour1, colour2);

    // Parameter values
    for (i = 0; i < 5; i++) {
        slidervalues[i] = static_cast<int>(preset.effects[currentMode].Parameters[i] * 100);
        // could do something special for inactive sliders?
        if (i >= preset.effects[currentMode].NumParameters) { slidervalues[i] = -1; }
    }

    DisplayParameters();

    // reduce number of sliders?
    // extra sliders for effects that need them?
    // buttons instead of sliders?
    // maybe later

    Serial.printf("update_display Mode=%i Effect=%s %s NumParams=%i\n",
        currentMode, current_effects[currentMode], preset.effects[currentMode].OnOff ? "ON" : "OFF", preset.effects[currentMode].NumParameters);

}

void draw_ModeBar() {

    M5.Lcd.setTextFont(1);
    M5.Lcd.setTextSize(2);

    M5.Lcd.setTextColor(WHITE, BLACK);

    // draw all mode names
    for (int i = 0; i < maxMode + 1; i++) {
        M5.Lcd.drawString(AllModes[i], 5, modebardown + i * modespacing);
    }

    M5.Lcd.setTextColor(BLACK, WHITE);
    M5.Lcd.drawString(AllModes[currentMode], 5, modebardown + currentMode * modespacing);

    M5.Lcd.setTextColor(WHITE, BLACK);

}

void update_itemtext(const char* newtext, uint16_t colour1, uint16_t colour2) {
    M5.Lcd.setTextColor(colour1, colour2);
    M5.Lcd.setTextFont(1);
    M5.Lcd.setTextSize(2);

    // black out the prevoius item
    M5.Lcd.fillRect(0, itemtextdown - 5, 319, 25, BLACK);

    // write the new item / colour
    M5.Lcd.drawCentreString(newtext, itemtextcentre, itemtextdown, 1);

    M5.Lcd.setTextColor(WHITE, BLACK);
    //M5.Lcd.setTextSize(2);

    Serial.printf("update_itemtext.newtext = %s\n", newtext);
}