#include <ch.h>
#include <hal.h>

#include <chprintf.h>

#define ADC1_NUM_CHANNELS   1 
#define ADC1_BUF_DEPTH      1 

static adcsample_t adc_buffer[ADC1_NUM_CHANNELS * ADC1_BUF_DEPTH]; 


static const GPTConfig gpt4cfg1 = { 
  .frequency =  100000,
  .callback  =  NULL,
  .cr2       =  TIM_CR2_MMS_1,  /* MMS = 010 = TRGO on Update Event.        */
  .dier      =  0U          //
  /* .dier field is direct setup of register, we don`t need to set anything here until now */
};


static void adccallback(ADCDriver *adcp, adcsample_t *buffer, size_t n)
{
	adcp = adcp; n = n;
}


static void adcerrorcallback(ADCDriver *adcp, adcerror_t err) {

  (void)adcp;
  (void)err;
}

static const ADCConversionGroup adcgrpcfg1 = {
  .circular     = true,                    
  /* Buffer will continue writing to the beginning when it come to the end */
  .num_channels = ADC1_NUM_CHANNELS,       
  .end_cb       = adccallback,              
  .error_cb     = adcerrorcallback,        
  .cr1          = 0,
  .cr2          = ADC_CR2_EXTEN_RISING | ADC_CR2_EXTSEL_SRC(12),  // Commutated from GPT
  /* 12 means 0b1100, and from RM (p.449) it is GPT4 */
  /* ADC_CR2_EXTEN_RISING - means to react on the rising signal (front) */
  .smpr1        = ADC_SMPR1_SMP_AN10(ADC_SAMPLE_144),       // for AN10 - 144 samples
  .smpr2        = 0, //ADC_SMPR2_SMP_AN3(ADC_SAMPLE_144),        // for AN3  - 144 samples
  .sqr1         = ADC_SQR1_NUM_CH(ADC1_NUM_CHANNELS),   //
  /* Usually this field is set to 0 as config already know the number of channels (.num_channels) */
  .sqr2         = 0,
  .sqr3         = ADC_SQR3_SQ1_N(ADC_CHANNEL_IN10)// |         // sequence of channels
                  //ADC_SQR3_SQ2_N(ADC_CHANNEL_IN10)
  /* If we can macro ADC_SQR2_SQ... we need to write to .sqr2 */
};

static const SerialConfig sdcfg = {
    .speed  = 115200,
    .cr1    = 0,
    .cr2    = 0,
    .cr3    = 0
};

void sd_set(void)
{
    sdStart( &SD7, &sdcfg );
    palSetPadMode( GPIOE, 8, PAL_MODE_ALTERNATE(8) );    // TX
    palSetPadMode( GPIOE, 7, PAL_MODE_ALTERNATE(8) );    // RX
}

void matlab_msg(void)
{
	uint16_t pot_value[1] = {buffer[0]};
	msg_t msg = sdGetTimeout( &SD7, MS2ST( 30 ) );
	if ( msg >= 0 )
		{
			sdWrite( &SD7, (uint8_t *)pot_value, sizeof( pot_value ));
		}
}

void adc_set_and_start(void)
{
    adcStart(&ADCD1, NULL);
    adcStartConversion(&ADCD1, &adcgrpcfg1, adc_buffer, ADC1_BUF_DEPTH);
    palSetLineMode( LINE_ADC123_IN10, PAL_MODE_INPUT_ANALOG );  // PC0

    gptStart(&GPTD4, &gpt4cfg1);
    gptStartContinuous(&GPTD4, gpt4cfg1.frequency/1000);
}

PWMConfig pwm1conf = {
    .frequency = 400000,
    .period    = 4000,
    .callback  = NULL,
    .channels  = {
                  {PWM_OUTPUT_DISABLED, NULL},
                  {PWM_OUTPUT_DISABLED, NULL},
                  {PWM_OUTPUT_ACTIVE_HIGH, NULL},
                  {PWM_OUTPUT_DISABLED, NULL}
                  },
    .cr2        = 0,
    .dier       = 0
};

void pwm_set_and_start(void)
{
    palSetLineMode( LINE_LED1, PAL_MODE_ALTERNATE(2) );
    pwmStart(&PWMD3 , &pwm1conf );
}

int main(void)
{
    chSysInit();
    halInit();
    sd_set();
    adc_set_and_start();
    pwm_set_and_start();

    while (true)
    {
    	matlab_msg();
    	pwmEnableChannel(&PWMD3, 2 , buffer[0]);
    	chThdSleepMilliseconds(5);
    }
}

