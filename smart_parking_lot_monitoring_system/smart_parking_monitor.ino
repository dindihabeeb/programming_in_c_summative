/**
 *  SMART PARKING LOT MONITOR
 *  Arduino Uno - 4 bays, gate entry/exit buttons
 *  Hardware
 *    D2       ENTRY button  (INPUT_PULLUP, other leg to GND)
 *    D3       EXIT  button  (INPUT_PULLUP, other leg to GND)
 *    D8..D11  Bay 1..4 LEDs, 220R to GND  (lit = occupied)
 *    D12      Status LED, 220R to GND
 *               solid      = lot full
 *               fast blink = last operation rejected
 *    D13      Heartbeat (onboard) - proves the loop never blocks
 */

//config
const uint8_t LED_PINS[]    = {8, 9, 10, 11};
const uint8_t NUM_SPACES    = sizeof(LED_PINS) / sizeof(LED_PINS[0]);
const uint8_t STATUS_LED    = 12;
const uint8_t HEARTBEAT_LED = 13;

const uint8_t BTN_PINS[]    = {2, 3};   //0 = ENTRY, 1 = EXIT
const uint8_t BTN_ENTRY     = 0;
const uint8_t BTN_EXIT      = 1;
const uint8_t NUM_BUTTONS   = sizeof(BTN_PINS) / sizeof(BTN_PINS[0]);

const uint16_t DEBOUNCE_MS   = 40;    //contacts must settle this long
const uint16_t BLINK_MS      = 150;   //status LED blink half-period
const uint16_t HEARTBEAT_MS  = 500;
const uint16_t REPORT_MS     = 2000;  //periodic statistics dump
const uint16_t ERROR_HOLD_MS = 1500;  //how long an error stays shown

//data structs
typedef struct {
  uint8_t  id;         //human-facing bay number
  uint8_t  ledPin;
  bool     occupied;
  uint32_t since;      //millis() at arrival; picks the longest-parked bay
  uint16_t served;     //lifetime arrivals at this bay
} Space;

//All the state one debounced button needs.
typedef struct {
  uint8_t  pin;
  bool     stable;     //debounced state: true = pressed
  bool     lastRaw;    //previous raw sample, to spot an edge
  uint32_t lastEdge;   //when the raw sample last changed
} Button;

Space  *lot = NULL;            //heap
Button  btns[NUM_BUTTONS];     //index with BTN_ENTRY / BTN_EXIT

//software timers - one variable each, no nesting.
uint32_t tBlink = 0, tBeat = 0, tReport = 0, tError = 0;
bool beatOn = false, blinkOn = false;

//memory

//Allocate and initialise the records. false if the heap is exhausted.
bool allocLot() {
  lot = (Space *) malloc(NUM_SPACES * sizeof(Space));
  if (lot == NULL) return false;

  for (uint8_t i = 0; i < NUM_SPACES; i++) {
    Space *s = lot + i;
    s->id       = i + 1;
    s->ledPin   = LED_PINS[i];
    s->occupied = false;
    s->since    = 0;
    s->served   = 0;
  }
  return true;
}

void freeLot() {
  free(lot);
  lot = NULL;   //to avoid dangling pointer
}

//input

//True exactly once, on the transition into "pressed".
//k is BTN_ENTRY or BTN_EXIT. Call every pass of loop().
bool pressed(uint8_t k, uint32_t now) {
  Button *b = &btns[k];
  bool raw = (digitalRead(b->pin) == LOW);

  if (raw != b->lastRaw) {   //still bouncing - restart wait
    b->lastRaw  = raw;
    b->lastEdge = now;
    return false;
  }
  if (now - b->lastEdge >= DEBOUNCE_MS && raw != b->stable) {
    b->stable = raw;     //settled, and genuinely changed
    return b->stable;    //true on press, false on release
  }
  return false;
}

//parking ops

uint8_t occupiedCount() {
  uint8_t n = 0;
  if (lot == NULL) return 0;
  for (uint8_t i = 0; i < NUM_SPACES; i++)
    if ((lot + i)->occupied) n++;
  return n;
}

//Fill the first free bay. Returns its index, -1 means the lot is full.
int8_t park(uint32_t now) {
  if (lot == NULL) return -1;
  for (uint8_t i = 0; i < NUM_SPACES; i++) {
    Space *s = lot + i;
    if (!s->occupied) {
      s->occupied = true;
      s->since    = now;
      s->served++;
      return (int8_t) i;
    }
  }
  return -1;   //no space
}

//Free the longest-parked bay (FIFO). Returns its index, -1 means empty.
int8_t leaveOldest() {
  int8_t best = -1;
  if (lot == NULL) return -1;

  for (uint8_t i = 0; i < NUM_SPACES; i++) {
    Space *s = lot + i;
    if (s->occupied && (best < 0 || s->since < (lot + best)->since))
      best = (int8_t) i;
  }
  if (best < 0) return -1;   //empty lot guard

  Space *s = lot + best;
  s->occupied = false;
  s->since    = 0;
  return best;
}

//output

void showLeds() {
  if (lot == NULL) return;
  for (uint8_t i = 0; i < NUM_SPACES; i++) {
    Space *s = lot + i;
    digitalWrite(s->ledPin, s->occupied ? HIGH : LOW);
  }
}

//One line summary printed after every accepted event.
void printCounts() {
  uint8_t used = occupiedCount();
  Serial.print(F("   occupied "));
  Serial.print(used);
  Serial.print('/');
  Serial.print(NUM_SPACES);
  Serial.print(F("  available "));
  Serial.println(NUM_SPACES - used);
}

//Full statistics table, printed on a timer.
void report(uint32_t now) {
  uint8_t used = occupiedCount();

  Serial.print(F("-- STATUS  occupied "));
  Serial.print(used);
  Serial.print('/');
  Serial.print(NUM_SPACES);
  Serial.print(F("  available "));
  Serial.println(NUM_SPACES - used);

  for (uint8_t i = 0; i < NUM_SPACES; i++) {
    Space *s = lot + i;
    Serial.print(F("   Bay "));
    Serial.print(s->id);
    Serial.print(s->occupied ? F(" OCCUPIED ") : F(" FREE     "));
    if (s->occupied) {
      Serial.print((now - s->since) / 1000);
      Serial.print('s');
    }
    Serial.print(F("   served="));
    Serial.println(s->served);
  }
}

//setup / loop

void setup() {
  Serial.begin(9600);

  for (uint8_t i = 0; i < NUM_SPACES; i++) pinMode(LED_PINS[i], OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  pinMode(HEARTBEAT_LED, OUTPUT);

  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    btns[i].pin      = BTN_PINS[i];
    btns[i].stable   = false;
    btns[i].lastRaw  = false;
    btns[i].lastEdge = 0;
    pinMode(BTN_PINS[i], INPUT_PULLUP);
  }

  if (!allocLot()) {
    Serial.println(F("FATAL: cannot allocate parking records"));
    digitalWrite(STATUS_LED, HIGH);
    while (1) { }   //nothing safe to do
  }

  showLeds();
  Serial.print(F("Parking monitor ready - "));
  Serial.print(NUM_SPACES);
  Serial.print(F(" bays, "));
  Serial.print(NUM_SPACES * sizeof(Space));
  Serial.println(F(" bytes on the heap"));
}

void loop() {
  uint32_t now = millis();

  //vehicle arrives
  if (pressed(BTN_ENTRY, now)) {
    int8_t k = park(now);
    if (k >= 0) {
      Space *s = lot + k;
      showLeds();
      Serial.print(F("IN  -> Bay "));
      Serial.print(s->id);
      printCounts();
    } else {
      tError = now;
      Serial.println(F("ERROR: entry rejected - LOT FULL"));
    }
  }

  //vehicle departs
  if (pressed(BTN_EXIT, now)) {
    int8_t k = leaveOldest();
    if (k >= 0) {
      Space *s = lot + k;
      showLeds();
      Serial.print(F("OUT <- Bay "));
      Serial.print(s->id);
      printCounts();
    } else {
      tError = now;
      Serial.println(F("ERROR: exit rejected - LOT EMPTY"));
    }
  }

  //serial command: reset
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'R' || c == 'r') {
      freeLot();   //release, then rebuild
      if (!allocLot()) {
        Serial.println(F("FATAL: reallocation failed"));
        while (1) { }
      }
      showLeds();
      Serial.println(F("RESET: records freed and reallocated"));
    }
  }

  //timer: status LED (error blink wins over full indicator)
  if (now - tBlink >= BLINK_MS) {
    tBlink  = now;
    blinkOn = !blinkOn;
    bool err = (tError != 0) && (now - tError < ERROR_HOLD_MS);
    digitalWrite(STATUS_LED,
                 err ? blinkOn : (occupiedCount() == NUM_SPACES));
  }

  //timer: heartbeat
  if (now - tBeat >= HEARTBEAT_MS) {
    tBeat  = now;
    beatOn = !beatOn;
    digitalWrite(HEARTBEAT_LED, beatOn);
  }

  //timer: periodic statistics
  if (now - tReport >= REPORT_MS) {
    tReport = now;
    report(now);
  }
}
