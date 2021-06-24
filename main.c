/**
  Generated main.c file from MPLAB Code Configurator

  @Company
    Microchip Technology Inc.

  @File Name
    main.c

  @Summary
    This is the generated main.c using PIC24 / dsPIC33 / PIC32MM MCUs.

  @Description
    This source file provides main entry point for system initialization and application code development.
    Generation Information :
        Product Revision  :  PIC24 / dsPIC33 / PIC32MM MCUs - 1.170.0
        Device            :  dsPIC33EV256GM106
    The generated drivers are tested against the following:
        Compiler          :  XC16 v1.61
        MPLAB 	          :  MPLAB X v5.45
*/


/**
  Section: Included Files
*/
#include "mcc_generated_files/mcc.h"
#include "SASE/SASE_Init.h"


long i;

void delay_ms(int delay);
void delay_us(unsigned long delay);
void __delay32(unsigned long delay);
void MachineLogic(void);
void JoystickLogic(void);
void SetDriveSpeed(void);
void SetWheelDirection(void);
void VFDlogic(void);
void CAN_Message(CAN_MSG_OBJ msg);
void RampUpISR(void);


/*
                         Main application
 */
int main(void)
{
    // initialize the device
    SYSTEM_Initialize();
    CAN1_TransmitEnable();
    CAN1_ReceiveEnable();
    //WATCHDOG_TimerSoftwareDisable();
    //TMR1_SetInterruptHandler (RampUpISR);
    
    uint16_t Packet1[8] = {0};
    uint16_t Packet2[8] = {0};
    uint16_t Packet3[8] = {0};
    uint8_t data[8] = {0};
    LED1_SetHigh();
    delay_ms(500);
    LED1_SetLow();
    

    while (1)
    {
        //Main loop
        CAN_MSG_OBJ msg;
        msg.msgId = 0x0000;
        msg.data = data;
        
        CAN1_OperationModeSet(CAN_CONFIGURATION_MODE);
        if(CAN_CONFIGURATION_MODE == CAN1_OperationModeGet())
        {
            if(CAN_OP_MODE_REQUEST_SUCCESS == CAN1_OperationModeSet(CAN_NORMAL_2_0_MODE))
            {
                i = 0;
                while(i < 10000) 
                {
                    
                    if(CAN1_ReceivedMessageCountGet() > 0) 
                    {

                        if(true == CAN1_Receive(&msg))//Error in here, CJR
                        {
                            CAN_Received = true;
                            break;                         
                        } 
                    i++;
                    }
                }
            }
        }

        if(CAN_Received){
            if(msg.msgId == AutecAddress1){
                for(i = 0; i < 8; i++){
                    Packet1[i] = *(msg.data + i);
                }

                Forward = Packet1[0] & 1;
                TurnRight = (Packet1[0] & 2) >> 1;
                Backward = Packet1[1] & 1;
                TurnLeft = (Packet1[1] & 2) >> 1;
                Start = Packet1[2] & 1;
                Reset = (Packet1[2] & 2) >> 1;
                Jog = (Packet1[2] & 4) >> 2;
                TransportMode = (Packet1[2] & 8) >> 3;
                GrindMode = (Packet1[2] & 16) >> 4;
                ForwardS = (Packet1[2] & 32) >> 5;
                ReverseS = (Packet1[2] & 64) >> 6;
                LightSW = (Packet1[2] & 128) >> 7;
            }
            else if(msg.msgId == AutecAddress2){
                for(i = 0; i < 8; i++){
                    Packet2[i] = *(msg.data + i);
                }
                JoystickY = Packet2[0];
                JoystickX = Packet2[1];
                GrindSpeed = Packet2[3];
                Trim = Packet2[4]; 
            }
            else if(msg.msgId == AutecAddress3){
                for(i = 0; i < 8; i++){
                    Packet3[i] = *(msg.data + i);
                }
                DriveSpeed = Packet3[0];
            }
        }
        
        
        //Get CAN message and separate packets 
        //CAN_Message(msg);
                
        //Machine Logic
        MachineLogic(); 
        
        //Joystick Logic
        JoystickLogic();
        
        //Set Wheel Direction
        SetWheelDirection();
        
        //Set Drive Speed
        SetDriveSpeed();
        
        //Set VFD Logic
        VFDlogic();
         
    }
    return 1; 
}


void delay_ms(int delay)
{
    __delay32(delay*39921);
}

void delay_us(unsigned long delay)
{
    __delay32(delay*40);
}

void MachineLogic(void)
{
    //Light switch
    if(LightSW)
        LED1_SetHigh();
    else
        LED1_SetLow();
    
    //Enable Servos
    if((TransportMode || GrindMode) && !Reset){
        L_Enable_SetHigh();
        R2_Enable_SetHigh();
    }
        
    else{
        L_Enable_SetLow();
        R2_Enable_SetLow();
    }
    
    
    //Switch Active
    if(ForwardS || ReverseS)
        SwitchActive = 1;
    else
        SwitchActive = 0;
    

    //Joystick Active
    if(Forward || Backward || TurnRight || TurnLeft)
        JoystickActive = true;
    else
        JoystickActive = false;
    

    //Drive in reverse
    if((ReverseS && !Forward) || Backward)
        DriveBack = 1;
    else
        DriveBack = 0;
    

}

void JoystickLogic(void)
{
    //Normalize X and Y Speed
    DriveSpeed = (DriveSpeed  - 0x7F) * 2;
    DriveSpeedPer = DriveSpeed/255;
    YSpeed = abs(JoystickY - 0x007F)*2*DriveSpeedPer;
    XSpeed = abs(JoystickX - 0x007F)*2*DriveSpeedPer;
    
    
    //Set Y direction speed
    if((TransportMode || GrindMode) && (Forward || Backward)){
        LeftDrive = YSpeed;
        RightDrive = YSpeed;
    }
    
    //Set drive speed for normal grinding operation
    else if(GrindMode && !JoystickActive && SwitchActive){
        LeftDrive = DriveSpeed;
        RightDrive = DriveSpeed;
    }
    
    //Set turning speed for grinding 
    else if(GrindMode && !Forward && !Backward && (TurnRight || TurnLeft)){
        if((ForwardS && TurnRight) || (ReverseS && TurnLeft)){
            LeftDrive = DriveSpeed;
            RightDrive = DriveSpeed - XSpeed*2;
            
            if(RightDrive < 0){
                RightDrive = abs(RightDrive);
                GrindChangeDirR = true;
            }
            else
                GrindChangeDirR = false;
        }
        
        else if((ForwardS && TurnLeft) || (ReverseS && TurnRight)){
            RightDrive = DriveSpeed;
            LeftDrive = DriveSpeed - XSpeed*2;
            
            
            if(LeftDrive < 0){
                LeftDrive = abs(LeftDrive);
                GrindChangeDirL = true;
            }
            else
                GrindChangeDirL = false;
        }
    }
    
    //Set turning speed
    else if(!Forward && !Backward && TransportMode){
        if(TurnRight)
            LeftDrive = XSpeed;
        else if(TurnLeft)
            RightDrive = XSpeed;
        else{
        LeftDrive = 0x00;
        RightDrive = 0x00;
        }
    } 
       
    //Set speed back to 0
    else{
        LeftDrive = 0x00;
        RightDrive = 0x00;
    }
           
    
    
    //Set multi direction speed
    if((Forward && TurnRight) || (Backward && TurnLeft)){
        LeftDrive = YSpeed + XSpeed;
        if(LeftDrive > 250)
            RightDrive = 250 - XSpeed;
    }
    
    else if((Forward && TurnLeft) || (Backward && TurnRight)){
        RightDrive = YSpeed + XSpeed;
        if(RightDrive > 250)
            LeftDrive = 250 - XSpeed;
    }
    
    if(LeftDrive < 0)
        LeftDrive = 0;
    if(RightDrive < 0)
        RightDrive = 0;
       
    
}

void SetDriveSpeed(void)
{    
        
    //Left Drive
    if((TransportMode || GrindMode) && LeftDrive > 1){
        if(LeftDrive > 250)
            LeftDrive = 250;
        PWM_DutyCycleSet(LeftMotor_PWM, LeftDrive);
    }
    else
        PWM_DutyCycleSet(LeftMotor_PWM, 0x00);
    
    //Right Drive
    if((TransportMode || GrindMode) && RightDrive > 1){
        if(RightDrive > 250)
            RightDrive = 250;
        PWM_DutyCycleSet(RightMotor_PWM, RightDrive);
    }
    else
        PWM_DutyCycleSet(RightMotor_PWM, 0x00);
    
}

void SetWheelDirection(void)
{
    if((DriveBack && !GrindChangeDirR) || (ForwardS && GrindChangeDirR))
        RightWheelDir_SetLow();
    else
        RightWheelDir_SetHigh(); 
    

    if((DriveBack && !GrindChangeDirL) || (ForwardS && GrindChangeDirL))
        LeftWheelDir_SetLow();
    else
        LeftWheelDir_SetHigh();
}

void VFDlogic(void)
{
    //Normalize HeadSpeed
    HeadSpeed = abs(GrindSpeed-127);
    PWM_DutyCycleSet(HeadSpeed_PWM, 0xaa);
    
    if(GrindMode || Jog)
        FWD_SetHigh(); //HeadSpeed = abs(GrindSpeed-127);
    else
        FWD_SetLow();//HeadSpeed = 0;
    
    //Convert HeadSpeed to Hz
    if(Grinding && HeadSpeed > 1)
        HeadSpeedHz = HeadSpeed * 63; //HeadSpeed/127*8000 -> HeadSpeed*63
    else if(Jog)
        HeadSpeedHz = 5;
    else
        HeadSpeedHz = 0;
    
}


void RampUpISR(void)
{
//    if(GrindMode && !GrindModeOld){
//        StartRamp = 1;
//        RampTM = 0;
//    }
//    GrindModeOld = GrindMode;
//        
//    if(StartRamp){
//        RampTM++;
//        if(RampTM > 1000)
//            Nop();
//            //DriveSpeed = DriveSpeed*(RampTM-1000)/4000;
//        
//        if(RampTM > 4000)
//            StartRamp = 0;
//        
//    }
    LED1_Toggle();
    
    
}


/**
 End of File
*/

