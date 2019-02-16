//motor.h
//***NOTE THIS FIRMWARE SUPPORTS the Eth_Motors_DRV8800_rev02 PCB
#define MAX_NUM_MOTORS 16//20 //Maximum number of motors this MCU can control
//made duty cycle 10ms because <=2ms is too much and mcu gets sys exception
//MotorStatus defines the current motor status for an array of motors
// note that bit#s are PORTB bit number (not header pin#) and start with 0
//could add EnableDriverLeftPin, EnableDriverRightPins
#define MOTOR_CLOCKWISE 0  //motor is set to move in clockwise direction
#define MOTOR_COUNTERCLOCKWISE 1 //motor is set to move in clockwise direction
#define MOTOR_DRIVER_USES_PULSE_PIN 0x01 //the motor driver uses a pulse pin instead of pulsing both direction pins
#define MOTOR_INST_FIRST_RUN 0x02 //to make sure both h-bridge pins are zero for a microsecond before setting one to avoid a short-circuit between top and bottom bridge
typedef struct {
    uint8_t    flags; //flags that define how pulses are sent to motor pins
	uint32_t DirPin; //pin that determine direction
	uint32_t PulsePin; //pin that does pulse width modulation
#if 0 
    volatile unsigned int   *DirCWPort; //the address of the port of the MCU the pin for CW direction is on (LATB,LATC, etc.)
    volatile unsigned int   *DirCCWPort; //the address of the the port of the MCU the pin for CCW direction is on (LATB,LATC, etc.)
    volatile unsigned int   *PulsePort; //the address of the the port of the MCU the pin for Pulsing is on (LATB,LATC, etc.)
    volatile unsigned int   *CSPort; //the address of the the port of the MCU the pin for CS is on (LATB,LATC, etc.)
    volatile unsigned int   *DirCWPortTRIS; //the address of the in/out port of the MCU the pin for CW direction is on (TRISB, TRISC, etc.)
    volatile unsigned int   *DirCCWPortTRIS; //the address of the n/out port of the MCU the pin for CCW direction is on (TRISB, TRISC, etc.)
    volatile unsigned int   *PulsePortTRIS; //the address of the the n/out port of the MCU the pin for Pulsing is on (TRISB, TRISC, etc.)
    volatile unsigned int   *CSPortTRIS; //the address of the the n/out port of the MCU the pin for CS is on (TRISB, TRISC, etc.)-todo: can ignore is always PORTB
    volatile unsigned int   *CSADCBuf; //the address of the ADC1BUF (analog buffer) for CS
#endif
    uint32_t    Direction; //direction this motor is set to move in
    uint16_t	DirectionCWBit; //PORT bit# that moves the motor shaft clockwise (from the perspective of behind the shaft)
    uint16_t	DirectionCCWBit; //PORT bit# that moves the motor shaft counter clockwise (from the perspective of behind the shaft)
    uint16_t    PulseBit; //PORT bit# that gets a pulse
    uint16_t    CurrentSenseBit; // PORT bit# to receive current sense on
    uint16_t    DirectionCWBitMask; //mask to or/and port pins with- for example DirectionCWPin 3 is 2^3=4 (0x00000004)
    uint16_t    DirectionCCWBitMask; //mask to or/and port pins with- for example DirectionCWPin 5 is 2^5=32 0x20 (0x00000020)
    uint16_t    PulseBitMask; //mask to or/and port pins with
    uint16_t    CurrentSenseBitMask; //mask to or/and port pins with
    uint16_t	Strength; //0-7 strength of motor turning (duty cycle 0/7=stop,7/7=full speed)
    uint16_t	StrengthCount; //what part of duty cycle motor is currently on (ex if Strength=3 StrengthCount<3 means the motor pulse is on	>3 means the motor pulse is off)
    uint32_t	Duration; //remaining time in motor movement (0=not moving) in hundredths of a second (10^-2 seconds, 100hs=1 second)
    uint32_t	DurationCount; //counts up to duty cycle and resets each end of duty cycle
} MotorStatus;

#if 0 
#define MAX_NUM_STORED_INSTRUCTIONS 38
#define STORED_PROGRAM_RUNNING 0x01
#define STORED_PROGRAM_STEPPING 0x02
#define STORED_PROGRAM_PAUSED 0x03
//StoredInstructions - a list of stored motor instructions and execution times
typedef struct {
    uint16_t	StartTime; //StartTime of instruction (in ms)-can make Duint16_t?
//	uint16_t	Address; //Motor instruction 16+8=24 bits (motor #, direction, strength, and high uint8_t of duration)
//	uint8_t	Data;  //low uint8_t of duration (big endian)
    uint8_t	Inst[3];  //Motor Instruction (3 uint8_ts) uint8_t1=motor#,direction,strength + uint8_t2=duration(little endian)
} StoredInstructions;
#endif