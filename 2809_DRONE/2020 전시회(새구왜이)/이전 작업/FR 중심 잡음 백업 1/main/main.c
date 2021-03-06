#define _MAIN_
#define __STRUCT__


#include "DSP280x_Device.h"     // DSP281x Headerfile Include File
#include "DSP280x_Examples.h"  // DSP281x Examples Include File

void System_Init(void)
{
	DINT;				// 전체 인터럽트 금지
	InitSysCtrl();		// 워치록 금지, PLL 초기화, 주변 장치 클럭설정
	InitGpio();			// 입출력 포트 초기화	
	InitCpuTimers();
	
	MemCopy(&RamfuncsLoadStart, 	&RamfuncsLoadEnd, 		&RamfuncsRunStart); 
	MemCopy(&RamfuncsLoadStart1, 	&RamfuncsLoadEnd1, 	&RamfuncsRunStart1);
	
	InitSci();			
	InitSpi();
	InitPieCtrl();		// PIE 제어 레지스터 초기화 동작
	IER = 0x0000;		// 인터럽트 인에이블 레지스터 클리어
	IFR = 0x0000;		// 인터럽트 플래그 레지스터 클리어
	InitPieVectTable();
	InitAdc();

	Init_ISR();

	InitEPWM( &EPwm1Regs );
	InitEPWM( &EPwm2Regs );
	
	InitEQep( &LeftQepRegs );
	InitEQep( &RightQepRegs );
	
}

void Variable_Init( void )
{
	COMMEND_SCIA = 0;

	//-------------------------------------------------------//
	
	memset((void *)IMU_UC_buf,0x00,sizeof(unsigned char)*25);

	IMU_f32_roll = 0.0;
	IMU_f32_pitch = 0.0;	
	IMU_f32_yaw = 0.0;

	IMU_iq17_roll = _IQ17(0.0);
	IMU_iq17_pitch = _IQ17(0.0);
	IMU_iq17_yaw = _IQ17(0.0);

	//-------------------------------------------------------//
	Kp_count = Kp_FR;
	Ki_count = Ki_FR;
	Kd_count = Kd_FR;

	Error_iq17_roll = _IQ17(0.0);
	Error_iq17_pitch = _IQ17(0.0);
	Error_iq17_yaw = _IQ17(0.0);

	Radian = _IQtoF(_IQdiv(_IQ(PI), _IQ(180.0)));
	
	Speed_count = 0;
	Motor_Speed = 0;
	Correct_count = Correct_FR;

	offset_iq17_roll = _IQ17(0.0);
	offset_iq17_pitch = _IQ17(0.0);
	offset_iq17_yaw = _IQ17(0.0);

	Error_f32_FrontLeft = 0.0;
	Error_f32_FrontRight = 0.0;

	Kp_iq17_FL = _IQ17(0.0);
	Kp_iq17_FR = _IQ17(0.0);
	
	Ki_iq17_FL = _IQ17(0.0);
	Ki_iq17_FR = _IQ17(0.0);
	
	Kd_iq17_FL = _IQ17(0.0);
	Kd_iq17_FR = _IQ17(0.0);

	memset((void *)Err_f32_FL,0x00,sizeof(float32)*3);
	memset((void *)Err_f32_FR,0x00,sizeof(float32)*3);

	//-------------------------------------------------------//

	UltraSonic_U32_distance = 0;
	UltraSonic_U16_error = 0;
}

void main(void)
{	
	System_Init();
	Variable_Init();
	
	//LOAD
	SCIA_ISR_ON = ON;
	SCIB_ISR_ON = ON;
	
	MOTOR_calibration();
	IMU_offset();

	//주행시작
	StartCpuTimer2();

	while( 1 )
	{
		//ULTRASONIC_SENSOR();
		MOTOR_Reboot(Err_f32_FL[0], Err_f32_FR[0]);
		
		//TxPrintf("roll : %lf | pitch : %lf | yaw : %lf\n", _IQtoF(offset_iq17_roll - IMU_iq17_roll), _IQtoF(offset_iq17_pitch - IMU_iq17_pitch), _IQtoF(offset_iq17_yaw - IMU_iq17_yaw));
		//TxPrintf("roll : %u | pitch : %u | yaw : %u | ", (Uint16)(Kp_out * (_IQabs(offset_iq17_roll - IMU_iq17_roll) >> 17)), (Uint16)(Kp_out * (_IQabs(offset_iq17_pitch - IMU_iq17_pitch) >> 17)), (Uint16)(Kp_out * (_IQabs(offset_iq17_yaw - IMU_iq17_yaw) >> 17)));
		//TxPrintf("roll : %lf | pitch : %lf | FL_err : %lf | FR_err : %lf | RL_err : %lf | RR_err : %lf\n", IMU_f32_roll, IMU_f32_pitch, Error_f32_FrontLeft, Error_f32_FrontRight, Error_f32_RearLeft, Error_f32_RearRight);
		TxPrintf("Pitch : %+3.2lf | roll: %+3.2lf  | FR_PI : %+3.4lf | FR _D: %+3.4lf   ||   FR : %5u | RL : %5u   ||   Kp_count : %2.3lf | Ki_count : %1.5lf | Kd_count : %4.1lf |", _IQtoF(Error_iq17_pitch), _IQtoF(Error_iq17_roll), Error_f32_FrontRight, Err_f32_FR[0], test_pwm2, test_pwm3, Kp_count, Ki_count, Kd_count);
		TxPrintf("speed level : %2u   ||   kp-FR : %+9ld | ki-FR : %+9ld | kd-FR : %+9ld\n", Speed_count, Kp_iq17_FR, Ki_iq17_FR, Kd_iq17_FR);

		if(COMMEND_SCIA == 'w')
		{
			MOTOR_Speed(COMMEND_SCIA);
			COMMEND_SCIA = 0;
		}
		else if(COMMEND_SCIA == 's')
		{
			MOTOR_Speed(COMMEND_SCIA);	
			COMMEND_SCIA = 0;
		}
		else if(COMMEND_SCIA == 'i')
		{
			COMMEND_SCIA = 0;
			IMU_offset();
		}
#if ON	//각도 조절
		else if(COMMEND_SCIA == 'a')
		{
			COMMEND_SCIA = 0;
			Error_iq17_pitch += _IQ(5.0);
			Error_iq17_roll += _IQ(5.0);
		}
		else if(COMMEND_SCIA == 'd')
		{
			COMMEND_SCIA = 0;
			Error_iq17_pitch -= _IQ(5.0);
			Error_iq17_roll -= _IQ(5.0);
		}
#endif
		else;
		
#if ON
		if(COMMEND_SCIA == 'y')
		{
			COMMEND_SCIA = 0;
			//Correct_count++;
			Kd_count += 1;
		}
		else if(COMMEND_SCIA == 'h')
		{
			COMMEND_SCIA = 0;
			//Correct_count--;
			Kd_count -= 1;
		}
		else if(COMMEND_SCIA == 'r')
		{
			COMMEND_SCIA = 0;
			Kp_count += 0.01;
		}
		else if(COMMEND_SCIA == 'f')
		{
			COMMEND_SCIA = 0;
			Kp_count -= 0.01;
		}
		else if(COMMEND_SCIA == 't')
		{
			COMMEND_SCIA = 0;
			Ki_count += 0.00001;
		}
		else if(COMMEND_SCIA == 'g')
		{
			COMMEND_SCIA = 0;
			Ki_count -= 0.00001;
		}
		else;
#endif

	}
}

void Delay(Uint32 Cnt)
{
	while(Cnt--)
	{
		asm("		nop");
		
		asm("	nop");	
	}
}


