#include <msp430.h>
#include <rtthread.h>

void rt_system_power_manager(void)
{
//    __bis_SR_register(LPM3_bits);
    __no_operation();
}

#define TX_DATA     0xFF
#define TX_LEN      10
#define BEGIN_CODE  0x6A            // 0110 1010
#define END_CODE    0x95            // 1001 0101
unsigned char tx_data = 0;
unsigned char tx_len = 0;

unsigned char alarm_conntinue = 1;

static rt_thread_t pH_thread = RT_NULL;     // pH����߳̿��ƿ�
static rt_thread_t alarm_thread = RT_NULL;  // �����߳̿��ƿ�

static void pH_thread_entry(void *param);
static void alarm_thread_entry(void *param);

void SLP_Open(void){
    // ͨ��A0����������P1.0
    P1SEL0 |= BIT0;
    P1SEL1 |= BIT0;

    // ����ADC��11.7ms��1��+0.42725msת1��+0.03051758ms����=12.13ms��������һ�ε�ʱ��
    ADCCTL0 |= ADCSHT_9 | ADCON;                 // ��������ʱ��Ϊ384��Cycle������ADC
    ADCCTL1 |= ADCSSEL_1 | ADCSHP | ADCCONSEQ_0; // ʱ��ѡ��ΪACLK�����������������ͨ�����βɼ�
    ADCCTL2 = ADCRES_2 | ADCSR;                  // ��������Ϊ12λ��14��Cycleת��1��+1��Cycle����������ADC Buffer
    ADCMCTL0 = ADCSREF_0 | ADCINCH_0;            // �ο���ѹVCC��ѡ��ͨ��A0��P1.0��
    ADCIE |= ADCIE0;                             // �����ж�
}


void SLP_Close(void){
    ADCCTL0 = 0;

    P1DIR &= ~BIT0;
    P1SEL0 &= ~BIT0;
    P1SEL1 &= ~BIT0;
}


void SLP_Start(void){
    ADCCTL0 |= ADCENC | ADCSC; // ���ò���ʼת��
}


unsigned int SLP_Get(void){
    return ADCMEM0;
}


#pragma vector = ADC_VECTOR
__interrupt void SLP_ISR(void)
{
    switch(__even_in_range(ADCIV, ADCIV_ADCIFG))
    {
        case ADCIV_ADCIFG: // ת������ж�
            rt_thread_resume(pH_thread);
            ADCIFG = 0;
            break;
    }

}


void CM_Open(void)
{
    /* RX is enabled */
    P2OUT |= BIT3;
    P2DIR |= BIT3;

    /* TX is enabled */
    P1OUT |= BIT6;
    P1DIR |= BIT6;
    P6OUT |= BIT1;
    P6DIR |= BIT1;

    /* RX setting */
    // RX pull-down input
    P6SEL0 |= BIT0;
    P6SEL1 &= ~BIT0;
    P6REN |= BIT0;
    P6OUT &= ~BIT0;
    P6DIR &= ~BIT0;
    // Timer TB3 capture mode
    TB3CTL = TBCLR|TBSSEL__ACLK|MC__UP;
    TB3CCTL1 = CM__RISING|CAP__CAPTURE|CCIS__CCIA|CCIE;
    TB3CCR1 = 0;
}


void CM_SendData(unsigned char data)
{
    tx_data = data;
    TB0CCR0 = 32;  // 1kHz
    TB0CTL = TBSSEL__ACLK |MC_1 | TBIE| ID__1 | TBCLR;
}

char CM_IsEnd(void)
{
    return !TB0CTL;
}


// TIMER3_B1 interrupt service routine
#pragma vector = TIMER3_B1_VECTOR
__interrupt void TIMER3_B1_ISR(void)
{
    switch(__even_in_range(TB3IV, TBIV_14))
    {
    case TBIV__TBCCR1:
    {
    }
    }
}


#pragma vector = TIMER0_B1_VECTOR
__interrupt void CM_ISR(void)
{
    switch(__even_in_range(TB0IV, TBIV_14))
    {
    case TBIV__TBIFG:
    {
        switch(tx_len)
        {
        case 10:
        {
            P6OUT &= ~BIT1;
            --tx_len;
        }break;
        case 1:
        {
            P6OUT |= BIT1;
            --tx_len;
        }break;
        case 0:
        {
            tx_len = TX_LEN;
            tx_data = TX_DATA;
            TB0CTL = 0;
        }break;
        default:
        {
            unsigned char bit = 0x01&tx_data;
            if(bit == 1)
                P6OUT |= BIT1;
            else
                P6OUT &= ~BIT1;
            tx_data = tx_data>>1;
            --tx_len;
        }break;
        }
    }
    }
}


void Control_RFSwitch(int c_1,int c_2,int c_3,int c_4)
{
    if(c_1 == 1)
    {
        P4DIR |= BIT0;
        P4OUT |= BIT0;
    }
    else
    {
        P4DIR |= BIT0;
        P4OUT &= ~BIT0;
    }
    if(c_2 == 1)
    {
        P4DIR |= BIT1;
        P4OUT |= BIT1;
    }
    else
    {
        P4DIR |= BIT1;
        P4OUT &= ~BIT1;
    }
    if(c_3 == 1)
    {
        P1DIR |= BIT2;
        P1OUT |= BIT2;
    }
    else
    {
        P1DIR |= BIT2;
        P1OUT &= ~BIT2;
    }
    if(c_4 == 1)
    {
        P1DIR |= BIT3;
        P1OUT |= BIT3;
    }
    else
    {
        P1DIR |= BIT3;
        P1OUT &= ~BIT3;
    }
}


void PH_Init(void)
{
    SAC2DAT = (unsigned int)(1.65/3.3*(float)4095);
    SAC2PGA = MSEL_1;
    SAC2OA = SACEN|OAPM_0|OAEN|NMUXEN|NSEL_1|PMUXEN|PSEL_1;
    SAC2DAC = DACEN;
    P3SEL0 |= BIT1;
    P3SEL1 |= BIT1;

    P1DIR &= ~BIT0;
    P3DIR &= ~BIT1;
    P5DIR &= ~BIT1;

    // SW_WRK Switch
    P4OUT &= ~BIT4;
    P4DIR |= BIT4;
    // SW_WE Switch
    P4OUT &= ~BIT5;
    P4DIR |= BIT5;
    // SHRT_RECE Switch
    P4OUT |= BIT6;
    P4DIR |= BIT6;

    Control_RFSwitch(1,0,0,0);
}


/**
 * main.c
 */
int main(void)
{
    pH_thread = rt_thread_create("pH", pH_thread_entry, RT_NULL, 256, 3, 5);
    alarm_thread = rt_thread_create("alarm", alarm_thread_entry, RT_NULL, 256, 1, 5);
    if (pH_thread != RT_NULL)
        rt_thread_startup(pH_thread);
    else
        return -1;
    if (alarm_thread != RT_NULL)
        rt_thread_startup(alarm_thread);
    else
        return -1;
}


static void pH_thread_entry(void *param)
{
    int i = 0;
    unsigned int data = 0;

    while(1)
    {
        PH_Init();
        SLP_Open();
        SLP_Start();
        rt_thread_suspend(pH_thread);
        data = SLP_Get();
        SLP_Close();

        data = 0;
        if(data>2100 || data<1900)
            rt_thread_resume(alarm_thread);
        else
            alarm_conntinue = 0;

        CM_Open();
        for(i=0; i<100; i++)
        {
            CM_SendData(data);
            rt_thread_mdelay(500);
        }

        rt_thread_mdelay(5000);
    }
}

static void alarm_thread_entry(void *param)
{
    while(1)
    {
        rt_thread_suspend(alarm_thread);

        P1DIR |= BIT4;
        P1OUT &= ~BIT4;

        while(alarm_conntinue == 1)
        {
            CM_SendData(0x55);
            P1OUT ^= BIT4;

            rt_thread_mdelay(500);
        }
        P1OUT &= ~BIT4;
    }
}
