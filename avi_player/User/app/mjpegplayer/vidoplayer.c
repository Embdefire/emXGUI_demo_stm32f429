#include <stdio.h>
#include <string.h>
#include "stm32f4xx.h"
#include "ff.h"
#include "./mjpegplayer/avifile.h"
#include "./mjpegplayer/vidoplayer.h"
#include "./Bsp/lcd/bsp_lcd.h"
#include "./Bsp/wm8978/bsp_wm8978.h"  
#include "emXGUI.h"
#include "emXGUI_JPEG.h"
#include "rtthread.h"

FIL       fileR;
UINT      BytesRD;
uint8_t   Frame_buf[1024*30];

static volatile uint8_t audiobufflag=0;
uint8_t   Sound_buf[4][1024*5]={0};

uint8_t   *pbuffer;

uint32_t  mid;
uint32_t  Strsize;
uint16_t  Strtype;



static volatile uint8_t timeout;
extern WAVEFORMAT*   wavinfo;
extern avih_TypeDef* avihChunk;

void MUSIC_I2S_DMA_TX_Callback(void);
extern void mjpegdraw(uint8_t *mjpegbuffer,uint32_t size);
static void TIM3_Config(uint16_t period,uint16_t prescaler);
extern void App_DecodeMusic(HWND hwnd, const void *dat, int cbSize, JPG_DEC *dec);
extern char tiimg[];
extern unsigned int timgsize(void);
extern HDC hdc_AVI;
extern HWND hwnd_AVI;

void JPEG_Out(HDC hdc,int x,int y,u8 *mjpegbuffer,s32 size);

static volatile int frame=0;
static volatile int t0=0;
volatile int avi_fps=0;

static  JPG_DEC *dec=NULL;

void AVI_play(char *filename, HWND hwnd)
{
	FRESULT  res;
  uint32_t offset;
  uint16_t audiosize;
  uint8_t avires=0;
  uint8_t audiosavebuf;

  pbuffer=Frame_buf;
  static U16 pic_width,pic_height;
  res=f_open(&fileR,filename,FA_READ);
  if(res!=FR_OK)
  {
    return;    
  }
  
  res=f_read(&fileR,pbuffer,20480,&BytesRD);	  
  avires=AVI_Parser(pbuffer);//解析AVI文件格式
  if(avires)
  {
    return;    
  }
  
  avires=Avih_Parser(pbuffer+32);//解析avih数据块
  if(avires)
  {
    return;    
  }
  
  avires=Strl_Parser(pbuffer+88);//解析strh数据块
  if(avires)
  {
    return;    
  }
  
  avires=Strf_Parser(pbuffer+164);//解析strf数据块
  if(res!=FR_OK)
  {
    return;    
  }
  
  mid=Search_Movi(pbuffer);//寻找movi ID		
  if(mid==0)
  {
    return;    
  }
  
  Strtype=MAKEWORD(pbuffer+mid+6);//流类型
  Strsize=MAKEDWORD(pbuffer+mid+8);//流大小
  if(Strsize%2)Strsize++;//奇数加1
  f_lseek(&fileR,mid+12);//跳过标志ID  
  
  offset=Search_Auds(pbuffer);
  if(offset==0)
  {
    return;    
  }  
  audiosize=*(uint8_t *)(pbuffer+offset+4)+256*(*(uint8_t *)(pbuffer+offset+5));
  if(audiosize==0)
  {
    offset=(uint32_t)pbuffer+offset+4;
    mid=Search_Auds((uint8_t *)offset);
    if(mid==0)
    {
      return;    
    }
    audiosize=*(uint8_t *)(mid+offset+4)+256*(*(uint8_t *)(mid+offset+5));
  }
  
   I2S_Stop();			/* 停止I2S录音和放音 */
	wm8978_Reset();		/* 复位WM8978到复位状态 */	
  	/* 配置WM8978芯片，输入为DAC，输出为耳机 */
	wm8978_CfgAudioPath(DAC_ON, EAR_LEFT_ON | EAR_RIGHT_ON);

	/* 调节音量，左右相同音量 */
	wm8978_SetOUT1Volume(15);

	/* 配置WM8978音频接口为飞利浦标准I2S接口，16bit */
	wm8978_CfgAudioIF(I2S_Standard_Phillips, 16);
  I2S_GPIO_Config();
  I2Sx_Mode_Config(I2S_Standard_Phillips, I2S_DataFormat_16b, wavinfo->SampleRate);
  I2S_DMA_TX_Callback=MUSIC_I2S_DMA_TX_Callback;			//回调函数指wav_i2s_dma_callback
  I2S_Play_Stop();
  I2Sx_TX_DMA_Init((uint16_t *)Sound_buf[1],(uint16_t *)Sound_buf[2],audiosize/2);
  audiobufflag=0;	    
  timeout=0;
  audiosavebuf=0;
  audiobufflag=0;
  TIM3_Config((avihChunk->SecPerFrame/100)-1,9000-1);
  I2S_Play_Start();  
	
	t0= GUI_GetTickCount();
  while(1)//播放循环
  {					
		int t1;
		
		if(Strtype==T_vids)//显示帧
    {    
			RECT rc;  
			
			frame++;
			t1 =GUI_GetTickCount();
			if((t1 - t0) >= 1000)
			{
				
				avi_fps =frame;
				t0 =t1;
				frame =0;
			}

      //HDC hdc_mem,hdc;
      pbuffer=Frame_buf;
      f_read(&fileR,Frame_buf,Strsize+8,&BytesRD);//读入整帧+下一数据流ID信息
      //hdc = GetDC(hwnd);
      //TextOut(hdc,10,10,L"Hello",-1); //输出文字 
			
  #if 0     
      /*******文件解码*************************/ 
      dec = JPG_Open(Frame_buf, BytesRD);
 //     JPG_GetImageSize(&pic_width, &pic_height,dec);
      //hdc_mem = CreateMemoryDC(SURF_SCREEN,pic_width,pic_height); 
      if(BytesRD>10)
			{			
        //JPG_Draw(hdc_mem, 0, 0, dec);  
				//BitBlt(hdc, 400, 0, pic_width,pic_height,hdc_mem,0,0,SRCCOPY);
				JPG_Draw(hdc_AVI,0,0,dec); //绘制到MEMDC里.
				
			}
      JPG_Close(dec);
	#endif	
			
			timeout=0;
		
			if(frame&1)
			{	
				
	#if 0 //MEMDC方式.
				ClrDisplay(hdc_AVI,NULL,0);
				//JPG_Draw(hdc_AVI,0,0,dec); //绘制到MEMDC里.
				JPEG_Out(hdc_AVI,0,0,Frame_buf,BytesRD);
				rc.x =0;
				rc.y =0;
				rc.w =480;
				rc.h =272;	
				InvalidateRect(hwnd_AVI,&rc,FALSE); //触发窗口刷新.
				UpdateWindow(hwnd_AVI);
	#endif
	
	#if 1		//直接写到窗口方式.	
				HDC hdc;
				
				hdc =GetDC(hwnd_AVI);
				JPEG_Out(hdc,0,0,Frame_buf,BytesRD);			
				ReleaseDC(hwnd_AVI,hdc);
	#endif
			
			}
			
      while(timeout==0)
      {   
				rt_thread_delay(1); //不要死等，最好用信号量.				
        //GUI_msleep(5);
      }      
      
      //DeleteDC(hdc_mem);
      //ReleaseDC(hwnd, hdc);
      timeout=0;
    }//显示帧
    else if(Strtype==T_auds)//音频输出
    { 
      uint8_t i;
      audiosavebuf++;
      if(audiosavebuf>3)
			{
				audiosavebuf=0;
			}
			
		
      do
      {
				//rt_thread_delay(1); 
        i=audiobufflag;
        if(i)
					i--;
        else 
					i=3; 

      }while(audiobufflag==i);
			

			f_read(&fileR,Sound_buf[audiosavebuf],Strsize+8,&BytesRD);//读入整帧+下一数据流ID信息
			pbuffer=Sound_buf[audiosavebuf];      
    }
    else break;
    Strtype=MAKEWORD(pbuffer+Strsize+2);//流类型
    Strsize=MAKEDWORD(pbuffer+Strsize+4);//流大小									
    if(Strsize%2)Strsize++;//奇数加1							   	
  }
  
  I2S_Play_Stop();
  I2S_Stop();		/* 停止I2S录音和放音 */
	wm8978_Reset();	/* 复位WM8978到复位状态 */
  TIM_ITConfig(TIM3,TIM_IT_Update,DISABLE); //允许定时器3更新中断
	TIM_Cmd(TIM3,DISABLE); //使能定时器3
  f_close(&fileR);
}

void MUSIC_I2S_DMA_TX_Callback(void)
{
  audiobufflag++;
  if(audiobufflag>3)
	{
		audiobufflag=0;
	}
	
	if(DMA1_Stream4->CR&(1<<19)) //当前读取Memory1数据
	{
		DMA_MemoryTargetConfig(DMA1_Stream4,(uint32_t)Sound_buf[audiobufflag], DMA_Memory_0);
	}
	else
	{
		DMA_MemoryTargetConfig(DMA1_Stream4,(uint32_t)Sound_buf[audiobufflag], DMA_Memory_1); 
	}
}

/**
  * @brief  通用定时器3中断初始化
  * @param  period : 自动重装值。
  * @param  prescaler : 时钟预分频数
  * @retval 无
  * @note   定时器溢出时间计算方法:Tout=((period+1)*(prescaler+1))/Ft us.
  *          Ft=定时器工作频率,为SystemCoreClock/2=90,单位:Mhz
  */
static void TIM3_Config(uint16_t period,uint16_t prescaler)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);  ///使能TIM3时钟
	
	TIM_TimeBaseInitStructure.TIM_Prescaler=prescaler;  //定时器分频
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseInitStructure.TIM_Period=period;   //自动重装载值
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);
	
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE); //允许定时器3更新中断
	TIM_Cmd(TIM3,ENABLE); //使能定时器3
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn; //定时器3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x00; //抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x02; //子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

/**
  * @brief  定时器3中断服务函数
  * @param  无
  * @retval 无
  */
void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET) //溢出中断
	{    
    timeout=1;
	}
  TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //清除中断标志位	
}
