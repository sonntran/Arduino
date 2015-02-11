const int NLEDS = 4;
const int LEDPINS[NLEDS] = {
  3,5,6,11}; // Need to be PWM pins!
const int NSWITCHES = 4;
const int SWITCHPINS[NSWITCHES] = {
  8,9,10,12}; // skip 11 because we need it as a PWM LED pin
const int STATUSLEDPIN = 13; // onboard LED
const int SPEEDPOTPIN = 5; // analog potentiometer pin

const int NuSTEPS = 4; // microsteps between pattern steps

const int MINuSTEPSIZE = 15; // when POTPIN is 255
const int MAXuSTEPSIZE = 30; // when POTPIN is 0
int uStepSize;

const int NPATTERNS = 2*(NSWITCHES-1); // SWITCHPINS[0] is "shift key", so we have 6 instead of 4 :)
const int PATTERNLOOKUP[NPATTERNS+1] = {
  // indices into PATTERNDATA below
  0,4,8,12,16,24,25}; // one extra index to tell the length of last pattern

const int PATTERNDATA[][NLEDS]={
  // 0: 0-3 chase
  {
    255,0,0,0  }
  ,{
    0,255,0,0  }
  ,{
    0,0,255,0  }
  ,{
    0,0,0,255  }
  ,
  // 1: 4-7 fade in
  {
    255,255,255,255  }
  ,{
    0,0,0,0  }
  ,{
    0,0,0,0  }
  ,{
    0,0,0,0  }
  ,
  // 2: 8-11 chase+fade
  {
    0,0,0,255  }
  ,{
    0,0,255,127  }
  ,{
    0,255,127,63  }
  ,{
    255,127,63,0  }
  ,   
  // 3: 12-15 explode
  {
    0,255,255,0  }
  ,{
    127,63,63,127  }
  ,{
    63,0,0,63  }
  ,{
    0,0,0,0  }
  ,

  // 4: 16-23 slow fade
  {
    255,255,255,255  }
  ,{
    191,191,191,191  }
  ,{
    127,127,127,127  }
  ,{
    0,0,0,0  }
  ,
  {
    0,0,0,0  }
  ,{
    127,127,127,127  }
  ,{
    191,191,191,191  }
  ,{
    255,255,255,255  }
  ,
  // 5: 24 dark (for my wife's sake)
  {
    0,0,0,0  }
  ,
};

const int DEFAULTPAT = 5; // dark
int curPat;

const int SHIFTUP = 0;
const int SHIFTDOWN = 1;
const int SHIFTUSED = 2;
int shiftState;
int switchesFree; // true if no switch was pressed (except maybe the shift)

unsigned long timeBase; // used for syncing the patterns when switch is pressed
unsigned long seqBase; // absolute start of sequencer record or playback
unsigned long shiftDownTime; // time shift key went down. *Might* become seqBase

int recordMode; // true if we're recording
const int MAXSEQSIZE = 32;
int seqPats[MAXSEQSIZE];
int seqEnds[MAXSEQSIZE]; // millis from start of seq, cummulative
int seqSize; // length of the recorded sequence
int seqIndex; // where we are at the playback

void setup() {
  //Serial.begin(9600);
  for (int i=0 ; i<NLEDS ; i++) {
    pinMode(LEDPINS[i],OUTPUT);
  }
  pinMode(STATUSLEDPIN,OUTPUT);
  curPat = DEFAULTPAT;
  shiftState = SHIFTUP;
  switchesFree = seqSize = seqBase = shiftDownTime = 0; // will be set by checkSwitches if true
  setRecordMode(0);
}

void loop() {
  uStepSize=map(analogRead(SPEEDPOTPIN),0,1023,MAXuSTEPSIZE,MINuSTEPSIZE);
  setLeds();
  checkSwitches();
  checkSeqPlayback();
  delay(uStepSize);
}

void setLeds() {
  int patBase = PATTERNLOOKUP[curPat];
  int patLen = PATTERNLOOKUP[curPat+1]-patBase;
  int absuStep = (millis()-timeBase)/uStepSize;
  int uStep = absuStep%(patLen*NuSTEPS);
  int thisStep = uStep/NuSTEPS;
  int nextStep = (thisStep+1)%patLen;
  int patuStep = uStep%NuSTEPS;
  for (int l=0; l<NLEDS; l++) {
    int theVal = PATTERNDATA[patBase+thisStep][l];
    if (patuStep) {
      int nextVal=PATTERNDATA[patBase+nextStep][l];
      if (nextVal!=theVal) { // interpolate
        theVal = (theVal*(NuSTEPS-patuStep)+nextVal*patuStep)/NuSTEPS;
      }
    };
    analogWrite(LEDPINS[l],theVal);
  }
}

void checkSwitches() {
  int isShift = !digitalRead(SWITCHPINS[0]);
  int firstSwitch = 0;
  for (int s=1; s<NSWITCHES; s++) {
    if (!digitalRead(SWITCHPINS[s])) {
      firstSwitch = s;
      break;
    }
  }
  if (firstSwitch) {
    if (switchesFree) {
      int thePat = isShift? firstSwitch+NSWITCHES-2:firstSwitch-1;
      curPat = thePat;
      if (recordMode) {
        recordPat(thePat,millis());
      } 
      else { // switching to manual
        seqSize = 0;
      }
      // sync pattern timing      
      timeBase = millis();
      // A short bit of darkness would give the illusion of sync :)
      for (int l=0; l<NLEDS; l++) {
        analogWrite(LEDPINS[l],0);
      }
      switchesFree = 0;
      if (shiftState==SHIFTDOWN) {
        shiftState = SHIFTUSED;
      };
    }
  } 
  else {
    switchesFree = 1;
  }
  if (isShift) {
    if (shiftState==SHIFTUP) {
      shiftState = SHIFTDOWN;
      shiftDownTime = millis(); // if we switch recordMode later, this will become seqBase
    }
  } 
  else {
    if (shiftState==SHIFTDOWN) { // "shift" was used as recordMode toggle
      setRecordMode(!recordMode);
    }
    shiftState = SHIFTUP;
  }
}

void setRecordMode(int mode) {
  unsigned long now = millis();
  //Serial.print(now,DEC);
  //Serial.print(":setRecordMode ");
  //Serial.print(mode,DEC);
  //Serial.print(" shiftDownTime: ");
  //Serial.println(shiftDownTime,DEC);
  recordMode = mode;
  digitalWrite(STATUSLEDPIN,recordMode);
  if (recordMode) { // start recording
    seqBase = shiftDownTime; // recording started when "shift" went down
    seqSize = -1; // recordPat will bump it to 0
    recordPat(curPat,now);
  } 
  else {
    recordPat(-1,shiftDownTime); // doesn't matter what we record, just the "when"
    seqBase = shiftDownTime; // Should be *after* recordPat.
    seqIndex = -1;
    for (int i=0; i<seqSize; i++) {
      //Serial.print(seqPats[i],DEC);
      //Serial.print(":");
      //Serial.print(seqEnds[i],DEC);
      //Serial.print(" ");
    }
    //Serial.print("\n");
  }
}

void recordPat(int pat, unsigned long endTime) {
  if (seqSize>=0) {
    seqEnds[seqSize] = endTime-seqBase;
  }
  if (seqSize<MAXSEQSIZE-1) {
    seqPats[++seqSize] = pat; // might be -1, but then we'll never play it
  }
}

void checkSeqPlayback() {
  if (recordMode || (seqSize<2)) {
    return;
  };
  unsigned long absMillis = millis();
  int seqMillis = (absMillis-seqBase)%seqEnds[seqSize-1];
  for (int i=0; i<seqSize; i++) {
    if (seqMillis<=seqEnds[i]) {
      if (i!=seqIndex) {
        seqIndex = i;
        curPat = seqPats[i];
        // sync pattern timing      
        timeBase = millis();
        // A short bit of darkness would give the illusion of sync :)
        for (int l=0; l<NLEDS; l++) {
          analogWrite(LEDPINS[l],0);
        }
        //Serial.print(absMillis,DEC);
        //Serial.print("~");
        //Serial.print(seqMillis,DEC);
        //Serial.print("<=");
        //Serial.print(seqEnds[i],DEC);
        //Serial.print("(");
        //Serial.print(i,DEC);
        //Serial.print(")->");
        //Serial.println(curPat,DEC);
      }
      break;
    }
  }
}

