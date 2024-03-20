/*  INTERLOCKER  by Don Willsmer 
you are welcome to use the software for your personal use with an acknowledgement.
To drive the PCA9685 boards I have used the Hobby Components Library.
 For commercial use please contact me
This program acts between a leverframe fitted with Microswitches oor just plain switches
to provide for interlocking between the various Levers/switches.
Instead of providingphysical locking pulling a locked lever will cause a red led to flash and no  action will be taken. The system will not respond until the lever has been restored.levers

The initial release drives servos connected to a PCA9685 board which means I could use a Nano Every which would not have enough pins for a 16 lever frame and the servos. However the essential code can operate a mixture of devices. I will be looking at LED panel lights, DCC accessories, CBUS 
accessories, etc. for future releases.
It also allows for linked devices to be operated so pulling a Lever could operate a servo and change LEDs on a panel  */



#include <HCPCA9685.h> // Hobby Components 


#define  I2CAdd1 0x40    //PCA9685 board address
#define num_levers 6
#define Locked_pin 20
#define num_devices 7
#define QueSize 16


// Quevariables
int head=0;
int tail=0;
bool QueEmpty=true;
bool QueFull=false;
unsigned long leverchecktime;

struct {
  int device;
  bool direction;
} Queue[QueSize]; 


HCPCA9685 PCA_1(I2CAdd1);

void Applylocks(int leverno, bool reversed);

struct Eservo {
//  int PCA_1Board;
  byte channel;
  int Normal_pos;
  int Reverse_pos;

  int target;
  int current_pos;

  void Setup()  {
    current_pos=Normal_pos;
    PCA_1.Servo(channel, Normal_pos);
    delay(20);
  }

  void EOperate(bool Direction) {
    PCA_1.Sleep(false);
    if (Direction) {
      target=Reverse_pos;
    }
    else {
      target=Normal_pos;
    }
    while (current_pos<target) {
      current_pos=current_pos+3;
      if (current_pos>target) {
        current_pos=target;
      }
      PCA_1.Servo(channel, current_pos);
      delay(20);
    }
    while (current_pos>target) {
      current_pos=current_pos-3;
      if (current_pos<target)  {
        current_pos=target;
      }
      PCA_1.Servo(channel, current_pos);
      delay(20);  
    }
    PCA_1.Sleep(true);

  }
};

/*set up the Eservos format [num of servos]={{channel , normal position, reversed position}, */ 
Eservo Eservos[6]={{0,150,300},    
                             {1,150,250},
                             {2,150,250},
                             {3,180,300},
                             {4,120,280},
                             {5,150,350}};


struct Devices {
  char outputtype;
  int output_no;
  int linked_device;

  int Operate_Device(bool direction) {
     switch (outputtype) {
       case 'L':         
        //Lservos[output_no].Operate(direction);
        break;
        case 'E':
        Eservos[output_no].EOperate(direction);
        break;  
        case 'P':
        break;
       case 'Z':
       break;
     }
     return linked_device;
}
};

/* setting up devices format {{ type of device, output_no, linked_device},*/
Devices Device[num_devices]= {{'E',0,0},
                              {'E',1,0},
                              {'E',2,4},
                              {'E',3,0},
                              {'E',4,0},
                              {'E',5,0},
                              {'E',0,0}};


 struct Lever{
  int leverpin;
  int locked;
  int device_no;
  int linked_device;
  int locks[10];
  bool leverreverse;

  void Setup() {
    pinMode(leverpin, INPUT_PULLUP);
    leverreverse=false;
    }

   void checklever() {
    bool newleverstate=false;
    bool pinread=(digitalRead(leverpin));
    if (pinread==LOW) {
      newleverstate=true;
      }
    else {
      newleverstate=false;
    }
    if (newleverstate!=leverreverse) {
      if (locked>0) {
      //Locked pin routine
      digitalWrite ( Locked_pin, HIGH );     //lights LED to indicate lever is locked
      bool pinout=LOW;
        while (digitalRead(leverpin)== pinread) {
        delay (400);
        digitalWrite(Locked_pin, pinout);
        pinout=!pinout;
      //  delay (400);
      //  digitalWrite(Locked_pin, HIGH);   
        }
      digitalWrite (Locked_pin, LOW); 
      }
      else {
        leverreverse=newleverstate;
        //Apply locks
        int x=0;
        int leverno;
        while (locks[x]!=0) {
          leverno=locks[x];
          Applylocks(leverno, leverreverse);
          x++;
        }
     

        Queue[tail].device=device_no;
        Queue[tail].direction=leverreverse;
        tail++;
        QueEmpty=false;
        if (tail==QueSize) tail=0;
        if (tail==head) {
          QueFull=true;        
        }
          
  }
 }
   } 
};

Lever levers[num_levers]= {{2,2,0,0,{2,6,0}},
                             {3,0,1,4,{-1,3,0}},
                             {4,0,2,0,{2,-4,0}},
                             {5,1,3,0,{3,0}},
                             {6,0,4,0,{0}},
                             {7,0,5,0,{-1,0}}};


void Applylocks(int leverno, bool reversed) {
      if (reversed) {

          if (leverno>0) {
            leverno--;
            levers[leverno].locked= levers[leverno].locked+1;
          }
          else {
            leverno*=-1;
            leverno--;
            levers[leverno].locked=levers[leverno].locked-1;

          }
      } 
      else {
          if (leverno>0) {
            leverno--;
            levers[leverno].locked= levers[leverno].locked-1;
          }
          else {
            leverno*=-1;
            leverno--;
            levers[leverno].locked=levers[leverno].locked+1;

          }
      }
}


void Lever_Process() {  
  if (QueFull==false) {
    if (millis()>leverchecktime) {
     for (int l=0; l<num_levers; l++) {
     levers[l].checklever(); 
     }
    leverchecktime= millis()+100;
    }
  }
   
}


void Queue_Process() {
if (QueEmpty==false) {
  int next=0;
  int device_no=Queue[head].device;
  int direction=Queue[head].direction;
  next=Device[device_no].Operate_Device(direction);
  head++;
  if (head==QueSize) {
    head=0;
  }
  if (head==tail) {
    QueEmpty=true;
  }
  while(next>0) {
    Lever_Process();
    device_no=next;
    next=Device[device_no].Operate_Device(direction);
  }
}
}

void setup() {
  // put your setup code here, to run once:
  for (int x=0; x<num_levers; x++) {
    levers[x].Setup();
  }
  PCA_1.Init(SERVO_MODE);
  PCA_1.Sleep(false);
  for (int s=0; s<6; s++) {
    Eservos[s].Setup();
  }
} 

void loop () {
  if (QueFull==false) Lever_Process();
  if (QueEmpty==false) Queue_Process();
}
