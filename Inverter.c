/*
 * Inverter.c
 *
 *  Created on: Mar 6, 2015
 *      Author: watchmen
 */

/**
 * Private Function Definitions
 */
#include "SolarExplorer-Includes.h"
#include "Inverter.h"

#include "fsm.h"
#include "inverterFSM.h"
#include "hbridgeFSM.h"
#include "mpptFSM.h"
#include "panelFSM.h"
#include "stdbool.h"
#include "stdio.h"


void DeviceInit(void);

void pwmPhaseConfig(void);

void userInitialization(void);

void netConnections(void);

void controllerCoefficientInits(void);

void netVariableInits(void);

void sigGenInit(void);

void dataLoggingInits(void);

void ISR_Init(void);

void SPI_init(void);

void ADC_Init(void);

//-------------------------------- DPLIB --------------------------------------------
void PWM_1ch_UpDwnCntCompl_CNF(int16 n, int16 period, int16 mode, int16 phase);
void ADC_SOC_CNF(int ChSel[], int Trigsel[], int ACQPS[], int IntChSel, int mode);

// -------------------------------- FRAMEWORK --------------------------------------
// State Machine function prototypes all declared in _FSM.h files
//----------------------------------------------------------------------------------


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// VARIABLE DECLARATIONS - GENERAL
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// -------------------------------- FRAMEWORK --------------------------------------

int16	VTimer0[4];					// Virtual Timers slaved off CPU Timer 0
int16	VTimer1[4];					// Virtual Timers slaved off CPU Timer 1
int16	VTimer2[4];					// Virtual Timers slaved off CPU Timer 2

// Used for running BackGround in flash, and ISR in RAM
extern Uint16 *RamfuncsLoadStart, *RamfuncsLoadEnd, *RamfuncsRunStart;
// Used for copying CLA code from load location to RUN location
extern Uint16 Cla1funcsLoadStart, Cla1funcsLoadEnd, Cla1funcsRunStart;

// Used for ADC Configuration
int 	ChSel[16] =   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int		TrigSel[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int     ACQPS[16] =   {8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8};

//---------------------------------------------------------------------------
// Used to indirectly access all EPWM modules
volatile struct EPWM_REGS *ePWM[] =
 				  { &EPwm1Regs,			//intentional: (ePWM[0] not used)
				  	&EPwm1Regs,
					&EPwm2Regs,
					&EPwm3Regs,
					&EPwm4Regs,
					#if (!DSP2802x_DEVICE_H)
					&EPwm5Regs,
					&EPwm6Regs,
					#if (DSP2803x_DEVICE_H || DSP2804x_DEVICE_H)
					&EPwm7Regs,
					#if (DSP2804x_DEVICE_H)
					&EPwm8Regs
					#endif
					#endif
					#endif
				  };

// Used to indirectly access all Comparator modules
volatile struct COMP_REGS *Comp[] =
 				  { &Comp1Regs,			//intentional: (Comp[0] not used)
					&Comp1Regs,
					&Comp2Regs,
					#if (DSP2803x_DEVICE_H)
					&Comp3Regs
					#endif
				  };
// ---------------------------------- USER -----------------------------------------
// ---------------------------- DPLIB Net Pointers ---------------------------------
// Declare net pointers that are used to connect the DP Lib Macros  here

// ADCDRV_1ch
extern volatile long *ADCDRV_1ch_Rlt1;	//instance #1
extern volatile long *ADCDRV_1ch_Rlt5; 	// instance #5
extern volatile long *ADCDRV_1ch_Rlt6; 	// instance #6
extern volatile long *ADCDRV_1ch_Rlt7; 	// instance #7

// PWMDRV_1ch
//extern volatile long *PWMDRV_1ch_Duty3;	// instance #3, EPWM3
extern volatile long *PWMDRV_1ch_UpDwnCntCompl_Duty3;
extern volatile long PWMDRV_1ch_UpDwnCntCompl_Period3;

// CONTROL_2P2Z
extern volatile long *CNTL_2P2Z_Ref1;	// instance #1
extern volatile long *CNTL_2P2Z_Out1;	// instance #1
extern volatile long *CNTL_2P2Z_Fdbk1;	// instance #1
extern volatile long *CNTL_2P2Z_Coef1; 	// instance #1
extern volatile long CNTL_2P2Z_DBUFF1[5];

// CONTROL_2P2Z
extern volatile long *CNTL_2P2Z_Ref2;	// instance #1
extern volatile long *CNTL_2P2Z_Out2;	// instance #1
extern volatile long *CNTL_2P2Z_Fdbk2;	// instance #1
extern volatile long *CNTL_2P2Z_Coef2; 	// instance #1
extern volatile long CNTL_2P2Z_DBUFF2[5];

//MATH_EMAVG - instance #1
extern volatile long *MATH_EMAVG_In1;
extern volatile long *MATH_EMAVG_Out1;
extern volatile long MATH_EMAVG_Multiplier1;

//MATH_EMAVG - instance #2
extern volatile long *MATH_EMAVG_In2;
extern volatile long *MATH_EMAVG_Out2;
extern volatile long MATH_EMAVG_Multiplier2;

// ---------------------------- DPLIB Variables ---------------------------------
// Declare the net variables being used by the DP Lib Macro here

volatile long Duty3A,Duty3A_fixed;
volatile long VboostRead;
volatile long VpvRef;
volatile long VpvRead, IpvRead;
volatile long VpvRead_EMAVG, IpvRead_EMAVG;
volatile long IboostswRead;
volatile long IboostSwRef;
volatile long VpvRef_MPPT;

#pragma DATA_SECTION(CNTL_2P2Z_CoefStruct1, "CNTL_2P2Z_Coef");
struct CNTL_2P2Z_CoefStruct CNTL_2P2Z_CoefStruct1;

#pragma DATA_SECTION(CNTL_2P2Z_CoefStruct2, "CNTL_2P2Z_Coef");
struct CNTL_2P2Z_CoefStruct CNTL_2P2Z_CoefStruct2;

long Pgain_V,Igain_V,Dgain_V,Dmax_V;
long Pgain_I,Igain_I,Dgain_I,Dmax_I;

int16 UpdateCoef;

// DC-DC MPPT Variables
int16 PanelBoostConnect;	// 0 when Panel is connected to battery input
							// 1 when panel is connected to boost input

//-------------------------- MPPT tracking PnO and Incc ---------------------------
mppt_incc mppt_incc1 = mppt_incc_DEFAULTS;
mppt_pno  mppt_pno1 = mppt_pno_DEFAULTS;

int16 MPPT_slew;
int16 Run_MPPT;
int16 MPPT_ENABLE;
//-----------------------------Inverter Variables ----------------------------------

//--------------------------------- SINE GEN LIB ------------------------------------
SGENHP_2 sgen = SGENHP_2_DEFAULTS;

//-------------------------------- PWM DAC driver -----------------------------------
int16 PwmDacCh1=0;
int16 PwmDacCh2=0;
int16 PwmDacCh3=0;
int16 PwmDacCh4=0;

PWMDAC pwmdac1 = PWMDAC_DEFAULTS;

//-------------- PID GRANDO INSTANCE Voltage Loop and Current Loop   -----------------

PID_GRANDO_CONTROLLER	pidGRANDO_Iinv = {PID_TERM_DEFAULTS, PID_PARAM_DEFAULTS, PID_DATA_DEFAULTS};
PID_GRANDO_CONTROLLER	pidGRANDO_Vinv = {PID_TERM_DEFAULTS, PID_PARAM_DEFAULTS, PID_DATA_DEFAULTS};

// ------------- Solar Sine Analyzer Block to measure RMS, frequency and ZCD ---------
SineAnalyzer_diff sine_mainsV = SineAnalyzer_diff_DEFAULTS;

// ------------- Software PLL for Grid Tie Applications ------------------------------
SPLL_1ph spll1;

// ------------- Data Logger --------------------------------------------------------
// Datalogger options and instance creation
int16 DlogCh1 = 0;
int16 DlogCh2 = 0;
int16 DlogCh3 = 0;
int16 DlogCh4 = 0;

DLOG_4CH dlog = DLOG_4CH_DEFAULTS;

_iq24 inv_ref_vol_inst,inv_meas_vol_inst;
_iq24 inv_ref_cur_inst;
_iq24 inv_meas_cur_diff_inst;
_iq24 inv_meas_cur_lleg1_inst;
_iq24 inv_meas_cur_lleg2_inst;

int16 CloseVloopInv;
int16 CloseIloopInv;

// for open loop operation of the inverter
_iq24	InvModIndex;				// Q15 format - 0..1.99 ( > 1 is overmodulation)

_iq24 inv_Iset;

_iq15 InvSine;

int16 ClearInvTrip;

int16 ResetPLL;

int32 Grid_Freq=GRID_FREQ;
int32 Vac_in;
int32 Offset_Volt;

_iq15 VrmsReal, VavgReal, Temp2;
_iq15 ScaleFactor;

int16 PVInverterState;

// Stand Alone Flash Image Instrumentation, GPIO toggles for different states
int16  LedBlinkCnt,LedBlinkCnt2;
int16 timer1;

int16 TransmitData;
Uint16 sdata[2];     // Send data buffer
// Display Values

// Monitor ("Get")						// Display as:
_iq	Gui_Vpv;							// Q10
_iq Gui_Ipv;							// Q12
_iq	Gui_Vboost;							// Q9
_iq	Gui_PanelPower;						// Q9
_iq Gui_PanelPower_Theoretical;         //
_iq Gui_MPPTTrackingEff;
_iq	Gui_Light;						// Q13
_iq	Gui_LightRatio;					// Q13
_iq	Gui_LightRatio_Avg;				// Q13

_iq  Gui_LightCommand;				//Q13
Uint16  Gui_MPPTEnable;
Uint16  Gui_InvStart;
Uint16  Gui_InvStop;




Uint16  Gui_LightCommand_Prev;

// History arrays are used for Running Average calculation (boxcar filter)
// Used for CCS display and GUI only, not part of control loop processing
int16	Hist_Vpv[HistorySize];
int16	Hist_Ipv[HistorySize];
int16	Hist_Light[HistorySize];
int16	Hist_Vboost[HistorySize];

//Scaling Constants (values found via spreadsheet; exact value calibrated per board)
int16	K_Vpv;							// Q15
int16	K_Ipv;							// Q15
int16	K_Vboost;						// Q15
int16	K_Light;						// Q15

int16	iK_Vboost;						// Q15
int16	iK_Ipv;						// Q15

// Variables for background support only (no need to access)
int16	i;								// common use incrementer
Uint32	HistPtr, temp_Scratch; 			// Temp here means Temporary

Uint16 VloopTicker=0;

Uint16 ZCDDetect=0;
int16 sine_prev=0;

void initializeInverter(void){


	/**
	 * State Machine Declarations
	 */
	//Declare the variable 'inverter' to be of the type 'Inverter', where 'Inverter' is the class
	//wrapping the FSM object. Note that the FSM object is a pointer to the current state.
	Inverter inverter;
	hBridge hBridge;

	//Take the instance of the 'Inverter' class inverter, get the FSM it contains, and point it to an initialization state
	InverterCtor(&inverter);
	hBridgeCtor(&hBridge);

	//Derference the Fsm object pointed to by inverter, initialize it with an initial event
	//@TODO: need to figure the best initial event to send the machine
	FsmInit((Fsm *)&inverter, 0);
	FsmInit((Fsm *)&hBridge, 0);


	/**
	 * Device and Variable Inits
	 */

		// The DeviceInit() configures the clocks and pin mux registers
		// The function is declared in {ProjectName}-DevInit_F2803/2x.c,
		// Please ensure/edit that all the desired components pin muxes
		// are configured properly that clocks for the peripherals used
		// are enabled, for example the individual PWM clock must be enabled
		// along with the Time Base Clock

		DeviceInit();	// Device Life support & GPIO


		// Only used if running from FLASH
		// Note that the variable FLASH is defined by the compiler with -d FLASH

		#ifdef FLASH
		// Copy time critical code and Flash setup code to RAM
		// The  RamfuncsLoadStart, RamfuncsLoadEnd, and RamfuncsRunStart
		// symbols are created by the linker. Refer to the linker files.
			MemCopy(&RamfuncsLoadStart, &RamfuncsLoadEnd, &RamfuncsRunStart);

		// Call Flash Initialization to setup flash waitstates
		// This function must reside in RAM
			InitFlash();	// Call the flash wrapper init function
		#endif //(FLASH)


		/**
		 * Timing sync for background loops
		 * Timer period definitions found in PeripheralHeaderIncludes.h
		 */
		CpuTimer0Regs.PRD.all =  mSec5;		// A tasks
		CpuTimer1Regs.PRD.all =  mSec50;	// B tasks
		CpuTimer2Regs.PRD.all =  mSec1000;	// C tasks

		/**
		 * These virtual timers are incrememtned in the 'Alpha states' used on the SE
		 */
		VTimer0[0] = 0;
		VTimer1[0] = 0;
		VTimer2[0] = 0;
		LedBlinkCnt = 5;
		LedBlinkCnt2 = 5;


		/**
		 * Init State variables here
		 */


		/**
		 * User Initialization
		 */
		userInitialization();

		/**
		 * Configure PWM3, Boost Loop, for 100Khz switching Frequency
		 *
		 *PWM_1ch_UpDwnCntCompl_CNF(int16 n, int16 period, int16 mode, int16 phase)	 *
		 *
		 * PWM3, configured with a period of 600 sysclks, i.e. 600*(1/(60MHz)) = 1/(100kHz)
		 * Configured in 'slave mode', clock synchronization from EPWM x, the master.
		 */
		PWM_1ch_UpDwnCntCompl_CNF(3,600,0,30);

		/**
		 * PWM_1phInv_unipolar_CNF(n,period,deadband_rising,deadband_falling)
		 *
		 * PWM1, configured with a period of 1500 sysclks, i.e. 1500*(1/(60MHz)) = 1/(40kHz)
		 * Deadband, rising and falling, set to 20, I presume this to be 20 sysclks
		 * In this case we have 20*(1/(60MHz)) = 33uS
		 * @TODO verify the deadband assumption above
		 */
		PWM_1phInv_unipolar_CNF(1,1500,20,20);

		/**
		 * ADC Varible Inits
		 */
		ADC_Init();

		/**
		 * Implements phase synchronization to avoid ADC and ISR conflicts
		 *
		 * @TODO: find out more about how this works, make a timing diagram
		 */
		pwmPhaseConfig();

		/**
		 * Digital Power CLA(DP) library initialization
		 */
		DPL_Init();

		/**
		 * Connect structures to variables used in code
		 */
		netConnections();

		/**
		 * Set the coefficients used in 2P2Z compensators
		 */
		controllerCoefficientInits();

		/**
		 * Net Variable Inits - set nets to known initial conditions
		 */
		netVariableInits();

		/**
		 * Initialize Signal Generator module for Sine wave generation
		 */
		sigGenInit();

		/**
		 * Initialize the DATALOG module
		 */
		dataLoggingInits();


		// Initialize PWMDAC module
		pwmdac1.PeriodMax = 500;   // 3000->10kHz, 1500->20kHz, 1000-> 30kHz, 500->60kHz
		pwmdac1.PwmDacInPointer0 = &PwmDacCh1;
		pwmdac1.PwmDacInPointer1 = &PwmDacCh2;
		pwmdac1.PwmDacInPointer2 = &PwmDacCh3;
		pwmdac1.PwmDacInPointer3 = &PwmDacCh4;

		PWMDAC_INIT_MACRO(pwmdac1)

		/**
		 * Software PLL for grid tie
		 * @TODO: figure out if I should comment this out, or if leaving it will have adverse effects
		 * since we're not planning on grid-tie for the moment.
		 */
		SPLL_1ph_init(60,_IQ21(0.00005),&spll1);

		PanelBoostConnect=0;
		MPPT_ENABLE = 0;
		Run_MPPT=0;
		MPPT_slew=0;
		VpvRef_MPPT=_IQ24(0.0);

		CloseVloopInv = 0;
	 	CloseIloopInv = 0;
		ClearInvTrip=0;
		InvModIndex = 0;
		ScaleFactor = _IQ15(1.0);
		inv_Iset = _IQ24(0.0);
		InvSine=0;
		inv_meas_cur_diff_inst=0;
		VrmsReal=0;



		UpdateCoef=0;
		timer1=0;
		Offset_Volt=_IQ21(0.5); // the input sinusoid is offset with 1.65 V

		PVInverterState=0;
		Gui_MPPTEnable=0;
		Gui_InvStart=0;
		Gui_LightRatio_Avg=0;
		Gui_InvStop=0;
		Gui_PanelPower_Theoretical=_IQ9(0.0);

		// keep the PLL in reset by default
		ResetPLL=1;

		Gui_LightCommand=_IQ13(0.0);
		Gui_LightCommand_Prev=_IQ13(0.0);
		/**
		 * Init Virtual Timers 0-2 here
		 */


		#ifdef FLASH
		// Initiate Commros
			InitCommros();
		#endif

		/**
		 * Finally, initialize the ISRs, clear flags, setup PIE,
		 */
		ISR_Init();


		/**
		 * Hardware init of SPI function - works in tandem with Rx/Tx FIFO ISR declarations
		 */
		SPI_init();
}


void runInverter(void){

#ifdef FLASH
		ServiceRoutine(&commros);
		Datalogger(&commros.m_datalogger,0);
#endif

}

void pwmPhaseConfig(void){

	/**
	 * TBCTL: Time-Base Control Register
	 * 		PHSEN: Counter Register Load From Phase Register Enable
	 * 		SYNCOSEL: Synchronization Output Select. These bits select the source of the EPWMxSYNCO signal.
	 * TBPHS: Time-Base Phase Register. These bits set time-base counter phase of the selected ePWM relative
	 * to the time-base that is supplying the synchronization input signal.
	 * Example: If TBPRD (time-base period) is 600, and TBPHS is set to 200 for the 'slave', then we have 200/600 X 360° = 120°
	 * So we have 120° phase lead for the slave. See Figure 58 in SPRUG04A for an example of this.
	 */

	//EPWM1
	EPwm1Regs.TBCTL.bit.PHSEN   = TB_DISABLE;		//Do not load from the time-base counter from time-base phase register
	EPwm1Regs.TBCTL.bit.SYNCOSEL = TB_CTR_ZERO;		//CTR = zero: Time-base counter equal to zero (TBCTR = 0x0000)

	//EPWM2
	EPwm2Regs.TBCTL.bit.PHSEN   = TB_ENABLE;		//Load the time-base counter with the phase register when an EPWMxSYNCI
													//input signal occurs or when a software synchronization is forced by the SWFSYNC bit
	EPwm2Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;		//Choose EPWM2SYNC signal as source for synchronization
	EPwm2Regs.TBCTL.bit.PHSDIR = TB_UP;				//Count up on synchronization event
	EPwm2Regs.TBPHS.half.TBPHS = 2;					//@TODO: Figure out what the phase delay from this struct corresponds to!

	//EPWM3
	EPwm3Regs.TBCTL.bit.PHSEN=TB_ENABLE;			//Load the time-base counter with the phase register when an EPWMxSYNCI
													//input signal occurs or when a software synchronization is forced by the SWFSYNC bit
	EPwm3Regs.TBCTL.bit.SYNCOSEL=TB_SYNC_IN;		//Choose EPWM3SYNC signal as source for synchronization
	EPwm3Regs.TBCTL.bit.PHSDIR=TB_UP;				//Count up on synchronization event
	EPwm3Regs.TBPHS.half.TBPHS=4;					//@TODO: Figure out what the phase delay from this struct corresponds to!
}

void userInitialization(void){
	 /**
	  * User Initialization and Variable Definitions
	  * @TODO: Commment all of these variables with brief description of usages
	  */

	/**
	* These are used a in a moving average filter, to be output to user via GUI
	*/
	for(i=0;i<HistorySize;i++)
	{
		Hist_Vpv[i]=0;
		Hist_Ipv[i]=0;
		Hist_Light[i]=0;
		Hist_Vboost[i]=0;
	}

	HistPtr=0;

	K_Vpv=17050;							// Q15
	K_Ipv=17329;							// Q15
	K_Vboost=20463;							// Q15
	iK_Vboost=26236;						// Q15
	iK_Ipv=30981;
	K_Light=27034;							//Q15

	/**
	 * Initialize GUI variables
	 */
	Gui_Vpv=0;
	Gui_Ipv=0;
	Gui_Vboost=0;

	/**
	 * SPI send flag
	 */
	TransmitData=0;
	sdata[0]=0;     // Send data buffer
	sdata[1]=1;
}
/**
 * @brief Lib Module connection to nets
 * Connects the PWM, EMAVG and 2P2Z structures to variables used in code
 */
void netConnections(void){

	// Connect the PWM Driver input to an input variable, Open Loop System
	PWMDRV_1ch_UpDwnCntCompl_Duty3 = &Duty3A; //&Duty3A; //&Duty3A_fixed;

	ADCDRV_1ch_Rlt1=&IboostswRead;
	ADCDRV_1ch_Rlt5=&VboostRead;
	ADCDRV_1ch_Rlt6=&IpvRead;
	ADCDRV_1ch_Rlt7=&VpvRead;

	// MATH_EMAVG1 block connections
	MATH_EMAVG_In1=&IpvRead;
	MATH_EMAVG_Out1=&IpvRead_EMAVG;
	MATH_EMAVG_Multiplier1=_IQ30(0.001); // a 1000 point moving average filter

	// MATH_EMAVG2 block connections
	MATH_EMAVG_In2=&VpvRead;
	MATH_EMAVG_Out2=&VpvRead_EMAVG;
	MATH_EMAVG_Multiplier2=_IQ30(0.001); // a 1000 point moving average filter

	//connect the 2P2Z connections, for the inner current Loop
	CNTL_2P2Z_Ref2 = &IboostSwRef;
	CNTL_2P2Z_Out2 = &Duty3A;
	CNTL_2P2Z_Fdbk2= &IboostswRead;
	CNTL_2P2Z_Coef2 = &CNTL_2P2Z_CoefStruct2.b2;

	//connect the 2P2Z connections, for the outer Voltage Loop
	CNTL_2P2Z_Ref1 = &VpvRead;
	CNTL_2P2Z_Fdbk1 = &VpvRef;
	CNTL_2P2Z_Out1 = &IboostSwRef;
	CNTL_2P2Z_Coef1 = &CNTL_2P2Z_CoefStruct1.b2;

}

void controllerCoefficientInits(void){

	//connect the 2P2Z connections, for the inner current Loop
	CNTL_2P2Z_Ref2 = &IboostSwRef;
	CNTL_2P2Z_Out2 = &Duty3A;
	CNTL_2P2Z_Fdbk2= &IboostswRead;
	CNTL_2P2Z_Coef2 = &CNTL_2P2Z_CoefStruct2.b2;

	//connect the 2P2Z connections, for the outer Voltage Loop
	CNTL_2P2Z_Ref1 = &VpvRead;
	CNTL_2P2Z_Fdbk1 = &VpvRef;
	CNTL_2P2Z_Out1 = &IboostSwRef;
	CNTL_2P2Z_Coef1 = &CNTL_2P2Z_CoefStruct1.b2;

	// Coefficients for Outer Voltage Loop
	// PID coefficients & Clamping - Current loop (Q26)
	Dmax_V  = _IQ24(0.9);
	Pgain_V = _IQ26(0.015);
	Igain_V = _IQ26(0.00005);
	Dgain_V =_IQ26(0.0);

	// Coefficient init	--- Coeeficient values in Q26
	CNTL_2P2Z_CoefStruct1.b2   =Dgain_V;                            // B2
    CNTL_2P2Z_CoefStruct1.b1   =(Igain_V-Pgain_V-Dgain_V-Dgain_V);  // B1
    CNTL_2P2Z_CoefStruct1.b0   =(Pgain_V + Igain_V + Dgain_V);      // B0
    CNTL_2P2Z_CoefStruct1.a2   =0.0;                              	// A2 = 0
    CNTL_2P2Z_CoefStruct1.a1   =_IQ26(1.0);                       	// A1 = 1
    CNTL_2P2Z_CoefStruct1.max  =Dmax_V;					  		  	//Clamp Hi
    CNTL_2P2Z_CoefStruct1.min  =_IQ24(0.0); 					  	//Clamp Min

	// Coefficients for Inner Current Loop
	// PID coefficients & Clamping - Current loop (Q26)
	Dmax_I  = _IQ24(0.9);
	Pgain_I = _IQ26(0.015);
	Igain_I = _IQ26(0.00005);
	Dgain_I =_IQ26(0.0);

	// Coefficient init	--- Coeeficient values in Q26
	CNTL_2P2Z_CoefStruct2.b2   =Dgain_I;                            // B2
    CNTL_2P2Z_CoefStruct2.b1   =(Igain_I-Pgain_I-Dgain_I-Dgain_I);  // B1
    CNTL_2P2Z_CoefStruct2.b0   =(Pgain_I + Igain_I + Dgain_I);      // B0
    CNTL_2P2Z_CoefStruct2.a2   =0.0;                              	// A2 = 0
    CNTL_2P2Z_CoefStruct2.a1   =_IQ26(1.0);                       	// A1 = 1
    CNTL_2P2Z_CoefStruct2.max  =Dmax_I;					  		  	//Clamp Hi
    CNTL_2P2Z_CoefStruct2.min  =_IQ24(0.0); 					  	//Clamp Min
}

void netVariableInits(){

	// Initialize the net variables
	Duty3A =_IQ24(0.0);
	VboostRead=_IQ24(0.0);
	IboostswRead=_IQ24(0.0);
	VpvRef=_IQ24(0.9);	// to increase current, we need to reduce VpvRef, thus initailize it with a high value.
	IboostSwRef=_IQ24(0.0);
	Duty3A_fixed=_IQ24(0.2);
	VpvRead_EMAVG=_IQ24(0.0);
	IpvRead_EMAVG=_IQ24(0.0);

  	// MPPT testing related code
  	// mppt incc
	mppt_incc1.IpvH = _IQ(0.0001);
	mppt_incc1.IpvL = _IQ(-0.0001);
	mppt_incc1.VpvH = _IQ(0.0001);
	mppt_incc1.VpvL = _IQ(-0.0001);
	mppt_incc1.MaxVolt = _IQ(0.9);
	mppt_incc1.MinVolt = _IQ(0.0);
	mppt_incc1.Stepsize = _IQ(0.01);
	mppt_incc1.mppt_first=1;
	mppt_incc1.mppt_enable=0;

	//mppt pno
	mppt_pno1.DeltaPmin = _IQ(0.00001);
	mppt_pno1.MaxVolt = _IQ(0.9);
	mppt_pno1.MinVolt = _IQ(0.0);
	mppt_pno1.Stepsize = _IQ(0.01);

}

void sigGenInit(){

  	/* Signal Generator module initialisation           */
	sgen.offset=0;
    sgen.gain=0x7fff;       /* gain=1 in Q15                              */
	sgen.freq=0x14F8CF92; 		/* freq = (Required Freq/Max Freq)*2^31 */
								/* = (50/305.17)*2^31 = 0x14f8cf92 */
	sgen.step_max=0x3E7FB26;	/* Max Freq= (step_max * sampling freq)/2^32 */
								/* =(0x3E7FB26*20k)/2^32 = 305.17 */
	sgen.phase=0x80000000;      /* Phase= (required Phase)/180 in Q31 format  */
	       		                /*      =  (+90/180) in Q31 = 8000h          */

    //sine analyzer initialization
	sine_mainsV.Vin=0;
    sine_mainsV.SampleFreq=_IQ15(20000.0);
    sine_mainsV.Threshold=_IQ15(0.0);

}

void dataLoggingInits(void){

	// Initialize DATALOG module
    dlog.iptr1 = &DlogCh1;
    dlog.iptr2 = &DlogCh2;
    dlog.iptr3 = &DlogCh3;
    dlog.iptr4 = &DlogCh4;
    dlog.trig_value = _IQ15(0.08);
    dlog.size = 0x64;
    dlog.prescalar = 10;
    dlog.init(&dlog);

}

/**
 * @brief INTERRUPTS & ISR INITIALIZATION (best to run this section after other initialization)
 *
 * @TODO: document this! look into how the pie operates in the C2000 family
 * @TODO: Why are EDIS and EALLOW being called so many times in between configs?
 */
void ISR_Init(void){

// Set up C28x Interrupt

//Also Set the appropriate # define's in the {ProjectName}-Settings.h
//to enable interrupt management in the ISR
	EALLOW;
    PieVectTable.EPWM3_INT = &DPL_ISR;      	// DP Lib interrupt for boost closed loop control (100Khz)
    PieVectTable.ADCINT1 = &Inv_ISR;		// Inverter Control Interrupt (20Khz)

   	PieCtrlRegs.PIEIER3.bit.INTx3 = 1;      	// PIE level enable, Grp3 / Int3
   	PieCtrlRegs.PIEIER1.bit.INTx1 	= 1;		// Enable ADCINT1 in PIE group 1
   	PieCtrlRegs.PIEIER6.bit.INTx1 	= 1;		// Enable INT1 in PIE group 6
   	PieCtrlRegs.PIEIER6.bit.INTx2 	= 1;		// Enable INT1 in PIE group 6

   	EPwm3Regs.ETSEL.bit.INTSEL = ET_CTR_PRD;  	// INT on PRD event
   	EPwm3Regs.ETSEL.bit.INTEN = 1;              // Enable INT
    EPwm3Regs.ETPS.bit.INTPRD = ET_2ND;         // Generate INT on every event

    //ADC interrupt already enabled by ADC SOC Cnf

    IER |= M_INT3;                          	// Enable CPU INT3 connected to EPWM1-6 INTs:
    IER |= M_INT1;								// Enable CPU INT1 for ADCINT1,ADCINT2,ADCINT9,TripZone

    EINT;                                   	// Enable Global interrupt INTM
    ERTM;                                   	// Enable Global realtime interrupt DBGM
	EDIS;

// Protections

	EALLOW;

	Comp1Regs.COMPCTL.bit.COMPDACEN  =0x1;
	Comp1Regs.COMPCTL.bit.SYNCSEL    =0x0;	// asynchronous version of the COMP signal is passed to the EPWM/GPIO module
	Comp1Regs.COMPCTL.bit.CMPINV     =0x0;	// Output of the comparator is passed directly
	Comp1Regs.COMPCTL.bit.COMPSOURCE =0x0;	// inverting input of the comparator is connected to the internal DAC
	Comp1Regs.DACVAL.bit.DACVAL		 =950;	// set DAC input to peak trip point

	AdcRegs.COMPHYSTCTL.bit.COMP1_HYST_DISABLE = 0x1;

// Cycle by cycle trip for overvoltage protection
	/**
	 * DCTRIPSEL Register: Digital Trip Compare Select
	 *
	 * DCAHCOMPSEL: Digital Compare A High Input Select
	 * 		Defines the source for the DCAH input. The TZ signals, when used as trip signals,
	 * 		are treated as normal inputs and can be defined as active high or active low.
	 * 		Here, DC_COMP1OUT = 0x8 which will use COMP1OUT as input
	 *
	 * TZDCSEL: Trip Zone Digital Compare Event Select Register
	 * 		DCAEVT2: Digital Compare Output A Event 2 Selection,
	 * 		in this case is set to 'TZ_DCAH_HI' where TZ_DCAH_HI = 0x2.
	 * 		This option corresponds to DCAH = high, DCAL = don't care,
	 * 		where these are Digital Compare A high and Low resprectively.
	 * 		These indicate when a trip event has occured.
	 */
	EPwm3Regs.DCTRIPSEL.bit.DCAHCOMPSEL=DC_COMP1OUT;
	EPwm3Regs.TZDCSEL.bit.DCAEVT2=TZ_DCAH_HI;

	EPwm3Regs.DCACTL.bit.EVT2SRCSEL = DC_EVT2;
	EPwm3Regs.DCACTL.bit.EVT2FRCSYNCSEL=DC_EVT_ASYNC;

	EPwm3Regs.TZSEL.bit.DCAEVT2=0x1;

	EDIS;


	EALLOW;

// Cycle by cycle interrupt for CPU halt trip

/**
 * TZSEL Register: Trip Zone Select:
 *
 * TZCTL Register: Trip Zone Controller:
 *
 * TZCLR Register: Trip Zone Clear Register:
 *
 * TZFRC Register: Trip Zone Force Register:
 */
	EPwm3Regs.TZSEL.bit.CBC6=0x1;
	EPwm1Regs.TZSEL.bit.CBC6=0x1;
	EPwm2Regs.TZSEL.bit.CBC6=0x1;
	EPwm1Regs.TZSEL.bit.OSHT1=0x1;
	EPwm2Regs.TZSEL.bit.OSHT1=0x1;

// What do we want the OST/CBC events to do?
// TZA events can force EPWMxA
// TZB events can force EPWMxB
	EPwm3Regs.TZCTL.bit.TZA = TZ_FORCE_LO; // EPWMxA will go low
	EPwm3Regs.TZCTL.bit.TZB = TZ_FORCE_LO; // EPWMxB will go low

	EPwm1Regs.TZCTL.bit.TZA = TZ_FORCE_LO; // EPWMxA will go low
	EPwm2Regs.TZCTL.bit.TZB = TZ_FORCE_LO; // EPWMxB will go low

	EPwm1Regs.TZCTL.bit.TZA = TZ_FORCE_LO; // EPWMxA will go low
	EPwm2Regs.TZCTL.bit.TZB = TZ_FORCE_LO; // EPWMxB will go low

	EDIS;

	//clear any spurious trips
	EPwm3Regs.TZCLR.bit.OST=1;
	EPwm3Regs.TZCLR.bit.DCAEVT1=1;

	// software force the trip of inverter to disable the inverter completely
	EALLOW;
	EPwm1Regs.TZFRC.bit.OST=0x1;
	EPwm2Regs.TZFRC.bit.OST=0x1;
	EDIS;
}

/**
 * @brief SPI init function
 *
 * Setting the registers to get SPI serial comm up
 *
 * @TODO find out what SPI is being used for
 * @TODO consider the impact of moving this function to DevInit file
 */
void SPI_init(void)
{
	// Initialize SPI FIFO registers
   SpibRegs.SPICCR.bit.SPISWRESET=0; // Reset SPI
   SpibRegs.SPICCR.bit.SPILBK=0;	// No Loopback mode
   SpibRegs.SPICCR.bit.SPICHAR=0xF;	// 16 bit SPI word

   SpibRegs.SPICTL.bit.OVERRUNINTENA=0;
   SpibRegs.SPICTL.bit.CLK_PHASE=0;
   SpibRegs.SPICTL.bit.MASTER_SLAVE=1; // configure as a master
   SpibRegs.SPICTL.bit.SPIINTENA=0;
   SpibRegs.SPICTL.bit.TALK=1;

   SpibRegs.SPISTS.all=0x0000;

   SpibRegs.SPIBRR=0x0063;           // Baud rate

   SpibRegs.SPIRXBUF=0x0;
   SpibRegs.SPITXBUF=0x0;

   SpibRegs.SPIFFTX.all=0xC022;      // Enable FIFO's, set TX FIFO level to 4
   SpibRegs.SPIFFRX.all=0x0022;      // Set RX FIFO level to 4

   SpibRegs.SPIFFCT.all=0x00;
   SpibRegs.SPIPRI.all=0x0010;

   SpibRegs.SPICCR.bit.SPISWRESET=1;  // Enable SPI

   SpibRegs.SPIFFTX.bit.TXFIFO=1;
   SpibRegs.SPIFFRX.bit.RXFIFORESET=1;

   SpibRegs.SPIPRI.bit.FREE=1;
}

void ADC_Init(void){

	/**
	 * ADC configuration vars
	 */
	int 	ChSel[16] =   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int		TrigSel[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int     ACQPS[16] =   {8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8};

	/**
	 * Map channel to ADC Pin the dummy reads are to account for first sample issue in Rev 0 silicon
	 * Please refer to the Errata and the datasheet, this would be fixed in later versions of the silicon
	 */
	ChSel[0] = 14;						 // B6 - Iboostsw-FB, DC-DC Boost switch current, not routed on Rev 1, dummy read
	ChSel[1] = 14;						 // B6 - Iboostsw-FB, DC-DC Boost switch current, not routed on Rev 1
	ChSel[2] = 4;						 // A4 - Ileg1,
	ChSel[3] = 4;						 // A4 - Ileg1,
	ChSel[4] = 6;						 // A6 - Ileg2,
	ChSel[5] = 2;                        // A2 - Vb_FB, DC DC Boost Output Voltage
	ChSel[6] = 0;						 // A0 - Ipv_FB, Panel input current
	ChSel[7] = 1;						 // A1 - Vpv_FB, Panel input Voltage
	ChSel[8] = 7;						 // A7 - Vac-fb
	ChSel[9] = 5;						 // A5 - VN-fb
	ChSel[10] = 9;						 // B1 - VL-fb
	ChSel[11] = 8;						 // B0 - Light-fb

	// Select Trigger Event
	TrigSel[0]= ADCTRIG_EPWM3_SOCA;
	TrigSel[1]= ADCTRIG_EPWM3_SOCA;
	TrigSel[2]= ADCTRIG_EPWM1_SOCA;
	TrigSel[3]= ADCTRIG_EPWM1_SOCA;
	TrigSel[4]= ADCTRIG_EPWM1_SOCA;
	TrigSel[5]= ADCTRIG_EPWM3_SOCA;
	TrigSel[6]= ADCTRIG_EPWM3_SOCA;
	TrigSel[7]= ADCTRIG_EPWM3_SOCA;
	TrigSel[8]= ADCTRIG_EPWM1_SOCA;
	TrigSel[9]= ADCTRIG_EPWM1_SOCA;
	TrigSel[10]= ADCTRIG_EPWM1_SOCA;
	TrigSel[11]= ADCTRIG_EPWM1_SOCA;

	ADC_SOC_CNF(ChSel,TrigSel,ACQPS,3,0); // use auto clr ADC int flag mode, end of conversion 2 trigger ADC INT 1


	/**
	 * Configure the Start of Conversion for the ADC.
	 */

	// SOC for DCDC Boost MPPT
	EPwm3Regs.ETSEL.bit.SOCAEN 	= 1;
	EPwm3Regs.ETSEL.bit.SOCASEL = ET_CTR_PRD;	// Use PRD event as trigger for ADC SOC
    EPwm3Regs.ETPS.bit.SOCAPRD 	= ET_2ND;        // Generate pulse on 2nd event

	//SOC for DCAC Inverter,SOCA is configured by the PWM CNF macro
}



/**
 * State Machines
 */

void Panel_Dashboard(Panel *self, Event *e) {

    if (e->transition == true) {
	// Dashboard measurement calculated by:
	//	Gui_Vbout = VboutAvg * K_Vbout, where VboutAvg = sum of 8 Vbout samples
	//	Gui_Iinb = IinbAvg * K_Iinb, where IinbAvg = sum of 8 Iinb samples

		HistPtr++;
		if (HistPtr >= HistorySize)	HistPtr = 0;

		// BoxCar Averages - Input Raw samples into BoxCar arrays
		//----------------------------------------------------------------
		Hist_Vpv[HistPtr]    = 	Vpv_FB;
		Hist_Ipv[HistPtr]    = 	Ipv_FB;
		Hist_Vboost[HistPtr] = 	Vboost_FB;
		Hist_Light[HistPtr]  =  LIGHT_FB;

		temp_Scratch=0;
		for(i=0; i<8; i++)	temp_Scratch = temp_Scratch + Hist_Vpv[i];
		Gui_Vpv = ( (long) temp_Scratch * (long) K_Vpv ) >> 15;

		temp_Scratch=0;
		for(i=0; i<8; i++)	temp_Scratch = temp_Scratch + Hist_Ipv[i];
		Gui_Ipv = ( (long) temp_Scratch * (long) K_Ipv ) >> 15;

		temp_Scratch=0;
		for(i=0; i<8; i++)	temp_Scratch = temp_Scratch + Hist_Vboost[i];
		Gui_Vboost = ( (long) temp_Scratch * (long) K_Vboost ) >> 15;

		temp_Scratch=0;
		for(i=0; i<8; i++)	temp_Scratch = temp_Scratch + Hist_Light[i];
		Gui_Light = ( (long) temp_Scratch * (long) K_Light ) >> 15;

		Gui_LightRatio =_IQ13mpy(Gui_Light,LIGHTSENSOR_MAX_INV);

		Gui_PanelPower=_IQ9mpy(((Gui_Vpv-_IQ9(0.20))>_IQ9(0.0)?(Gui_Vpv-_IQ9(0.20)):_IQ9(0.0)),(((long)(Gui_Ipv-_IQ12(0.03))>>3)>_IQ9(0.0)?((long)(Gui_Ipv-_IQ12(0.03))>>3):_IQ9(0.0)));

		if(Gui_PanelPower_Theoretical!=0)
			Gui_MPPTTrackingEff =_IQ9mpy(_IQ9div(Gui_PanelPower,Gui_PanelPower_Theoretical),_IQ9(100.0));
		else
			Gui_MPPTTrackingEff=0;

        e->transition = false;
    }

    switch (e->signal) {
        case TIMER:
            printf("Disable");
            _FsmTran_((Fsm *) self, &Panel_Connect);
            break;

        case NO_EVENT_PANEL:
            printf("defaultNOEVENT");
            break;

        default:
            break;
    }
}

void Panel_Connect(Panel *self, Event *e) {

    if (e->transition == true) {

    	if(PanelBoostConnect==0)
    	{
    		GpioDataRegs.GPACLEAR.bit.GPIO12=0x1;
    	}
    	else if (PanelBoostConnect==1)
    	{
    		GpioDataRegs.GPASET.bit.GPIO12=0x1;
    	}

        e->transition = false;
    }

    switch (e->signal) {

        case TIMER:
            printf("Disable");
            _FsmTran_((Fsm *) self, &Panel_Emulator);
            break;

        case NO_EVENT_PANEL:
            printf("defaultNOEVENT");
            break;

        default:
            break;

    }
}

void Panel_Emulator(Panel *self, Event *e) {

    if (e->transition == true) {
    	if(Gui_LightCommand != Gui_LightCommand_Prev)
    	{
    	 sdata[0]=1; //1 indicates it is the command for the light value

    	 // saturate the light command to 0.8 because of power capacity of the DC power supply shipped with the kit
    	 if(Gui_LightCommand>_IQ13(0.8))
    	 	Gui_LightCommand=_IQ13(0.8);

    	  if(Gui_LightCommand<_IQ13(0.0))
    	 	Gui_LightCommand=_IQ13(0.0);

    	 sdata[1]=Gui_LightCommand;	// Value of light that needs to be sent to the emulator

    	 Gui_LightCommand_Prev=Gui_LightCommand;

    	 SpibRegs.SPITXBUF=sdata[0];      // Send data
    	 SpibRegs.SPITXBUF=sdata[1];

    	 SpiaRegs.SPIFFTX.bit.TXFIFO=1;

    	 Gui_PanelPower_Theoretical=_IQ9mpy(_IQ9(36.02),((long)Gui_LightCommand>>4)); // Panel Max power * Luminance Ratio

    	}
    	e->transition = false;
    }

    switch (e->signal) {
        case TIMER:
            printf("H-bridge to negVDC");
            _FsmTran_((Fsm *) self, &Panel_Dashboard);
            break;

        case NO_EVENT_PANEL:
            printf("defaultNOEVENT");
            break;

        default:;
            break;
    }
}





/**
 * MPPT FSM
 */

void Mppt_Execute(Mppt *self, Event *e) {

    if (e->transition == true) {
    	if(Run_MPPT==1)
    	{
    		// MPPT routine
    		mppt_incc1.Ipv = IpvRead_EMAVG; //IpvRead;
    		mppt_incc1.Vpv = VpvRead_EMAVG; //VpvRead;

    		mppt_incc_MACRO(mppt_incc1);

    		VpvRef_MPPT = mppt_incc1.VmppOut;

    		mppt_pno1.Ipv = IpvRead_EMAVG; //IpvRead;
    		mppt_pno1.Vpv = VpvRead_EMAVG; //VpvRead;

    		mppt_pno_MACRO(mppt_pno1);

    		//VpvRef_MPPT = mppt_pno1.VmppOut;

    		if(VpvRef_MPPT<_IQ24(0.0))
    		{
    			VpvRef_MPPT=_IQ24(0.0);
    		}
    		else if(VpvRef_MPPT>_IQ24(0.9))
    		{
    			VpvRef_MPPT=_IQ24(0.9);
    		}

    		VpvRef=VpvRef_MPPT;

    		Run_MPPT=0;
    	}

    //MPPT is a slow task, the following code enables to modulate the rate at which the MPPT is called

    	if(MPPT_slew==0)
    	{
    		if(MPPT_ENABLE==1)
    		{
    			Run_MPPT=1;
    			mppt_incc1.mppt_enable=1;
    		}
    		MPPT_slew=0;
    	}
    	else
    		MPPT_slew--;

    	// Toggle LD2 on the control card if MPPT enabled
    	if(MPPT_ENABLE==1)
    	{
    		if(LedBlinkCnt2==0)
    			{
    				GpioDataRegs.GPATOGGLE.bit.GPIO31 = 1;	//turn on/off LD2 on the controlCARD
    				LedBlinkCnt2=1;
    			}
    		else
    			LedBlinkCnt2--;
    	}
        e->transition = false;
    }

    switch (e->signal) {
        case DISABLE:
            printf("Disable");
            _FsmTran_((Fsm *) self, &Mppt_Disable);
            break;

        case NO_EVENT:
            printf("defaultNOEVENT");
            break;

        default:
            _FsmTran_((Fsm *) self, &Mppt_Blink);
            break;
    }
}

void Mppt_Blink(Mppt *self, Event *e) {

    if (e->transition == true) {
    	// Toggle LD3 on control card to show execution of code
    	if(LedBlinkCnt==0)
    	{
    		GpioDataRegs.GPBTOGGLE.bit.GPIO34 = 1;	//turn on/off LD3 on the controlCARD
    		LedBlinkCnt=4;
    	}
    	else
    		LedBlinkCnt--;
		e->transition = false;
    }

    switch (e->signal) {

        case DISABLE:
            printf("Disable");
            _FsmTran_((Fsm *) self, &Mppt_Disable);
            break;

        case NO_EVENT:
            printf("defaultNOEVENT");
            break;

        default:
            printf("Blink");
            _FsmTran_((Fsm *) self, &Mppt_Execute);
            break;

    }
}

// @TODO: Figure out whta needs to be done with this. Dont think we need a seperate state for MPPT disable, this i smore on the macro level...
void Mppt_Disable(Mppt *self, Event *e) {

    if (e->transition == true) {
		#if (INCR_BUILD == 2)
			// Inverter State ==0 , wait for the command to start production of power
			// Inverter State ==1 , Check if panel voltage is available, i.e. Vpv > 5V
			//						if true enable MPPT
			// Inverter State ==2 , Check if Gui_Vboost>33V
			//						enable closed voltage loop regulation and current loop regulation
			// Inverter State ==3, wait for stop command, if stop, trip all PWM's, shut down MPPT and return to state 0, reset all values

			switch(PVInverterState)
			{
				case 0: // wait for the command to start the inverter
						if(Gui_InvStart==1)
						{
							PVInverterState=1;
							Gui_InvStart=0;
						}
						break;
				case 1: // check if PV is present
						if(Gui_Vpv>_IQ9(3.0))
						{
							PVInverterState=2;
							//Enable MPPT
							MPPT_ENABLE=1;
							ClearInvTrip=1;
							InvModIndex=_IQ24(0.0);
							pidGRANDO_Vinv.term.Fbk = VdcRef; // 30V/ 39.97
							mppt_incc1.Stepsize = _IQ(0.02);
						}

						if(Gui_InvStop==1)
						{
							PVInverterState=3;
						}
						break;
				case 2: // Check if DC Bus is greater than 33V , as currently the inverter is off this woudl happen quickly
						if(Gui_Vboost>_IQ9(31.0))
						{
							PVInverterState=3;
							CloseVloopInv=1;
							CloseIloopInv=1;
							mppt_incc1.Stepsize = _IQ(0.005);
						}
						if(Gui_InvStop==1)
						{
							PVInverterState=3;
						}
						break;
				case 3: // Wait for shut down sequence
						if(Gui_InvStop==1)
						{
							// switch off MPPT and also open the loop for current and voltage
							// the open loop index on the inverter is used to discharge the boost till it is close to the input panle voltage

							MPPT_ENABLE=0;
							mppt_incc1.mppt_first=1;
							mppt_pno1.mppt_first=1;

							VpvRef=_IQ24(0.9);

							// Run the reset sequence
							// Trip the PWM for the inverter
							// software force the trip of inverter to disable the inverter completely
							EALLOW;
							EPwm1Regs.TZFRC.bit.OST=0x1;
							EPwm2Regs.TZFRC.bit.OST=0x1;
							EDIS;

							// Wait for the command to be restarted
							PVInverterState=0;

							CloseVloopInv=0;
							CloseIloopInv=0;

							Gui_InvStop=0;

						}

						break;
				default:
						break;
			}

		#endif

		#if (INCR_BUILD == 3)

			// Inverter State ==1 , Check grid voltage, i.e. Vrms Real > _IQ15(12)
			// Inverter State ==2 , Check if panel voltage is available, i.e. Vpv > 3V
			//						if true enable MPPT
			// Inverter State ==3 , Check if Gui_Vboost>31V
			//						enable closed voltage loop regulation and current loop regulation
			// Inverter State == 4, Check if inv_Iset > 0.1 pu, Clear Inverter Trip
			// Inverter State == 5, wait for stop command, if stop, trip all PWM's, shut down MPPT and return to state 0, reset all values

			switch(PVInverterState)
			{
				case 0: // wait for the command to start the inverter
						if(Gui_InvStart==1)
						{
							PVInverterState=1;
							Gui_InvStart=0;
						}
						break;
				case 1: // once the inverter command is issued check if the grid is present
						if(VrmsReal>_IQ15(12.0))
						{
							PVInverterState=2;
							// take the PLL out of reset
						}
						break;
				case 2: // AC is present, check if PV is present
						if(Gui_Vpv>_IQ9(3.0))
						{
							PVInverterState=3;
							//Enable MPPT
							MPPT_ENABLE=1;
							mppt_incc1.Stepsize = _IQ(0.02);
						}
						break;
				case 3: // Check if DC Bus is greater than 31V , as currently the inverter is off this woudl happen quickly
						if(Gui_Vboost>_IQ9(31.0))
						{
							PVInverterState=4;
							CloseVloopInv=1;
							CloseIloopInv=1;
							mppt_incc1.Stepsize = _IQ(0.005);
						}
						break;
				case 4: // Check if there is enough current command then only connect the grid
						// this is for safety, as the board does not have reverse current sense,
						if(inv_Iset>_IQ24(0.1))
						{
							PVInverterState=5;
							ClearInvTrip=1;
						}
						break;
				case 5: // Wait for shut down sequence
						if(Gui_InvStop==1)
						{
							Gui_InvStop=0;
							// Run the reset sequence
							// Trip the PWM for the inverter
							// software force the trip of inverter to disable the inverter completely
							EALLOW;
							EPwm1Regs.TZFRC.bit.OST=0x1;
							EPwm2Regs.TZFRC.bit.OST=0x1;
							EDIS;

							MPPT_ENABLE=0;
							mppt_incc1.mppt_first=1;
							mppt_pno1.mppt_first=1;

							VpvRef=_IQ24(0.9);

							// Wait for the command to be restarted
							PVInverterState=0;

							CloseVloopInv=0;
							CloseIloopInv=0;

						}
						break;
				case 6: // wait for the power sequence
						break;
				default:
						break;
			}

			if(VrmsReal<_IQ15(10.0) && PVInverterState<3)
			{
				ResetPLL=1;
			}
			else
				ResetPLL=0;

		#endif

			if(timer1<20)
				timer1++;

			if(timer1==20)
			{
				PanelBoostConnect=1;
				Gui_LightCommand=_IQ13(0.2);
				timer1++;
			}

			e->transition = false;
    }

    switch (e->signal) {
        case EXECUTE:
            printf("H-bridge to negVDC");
            _FsmTran_((Fsm *) self, &Mppt_Execute);
            break;

        case NO_EVENT:
            printf("defaultNOEVENT");
            break;

        default:;
            break;
    }
}





/**
 * Transition Functions
 */









