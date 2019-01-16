
#include <stdio.h>
#include "emXGUI.h"
#include "x_libc.h"
#include "./led/bsp_led.h"  
#include "GUI_AppDef.h"




/**********************分界线*********************/

/* 各类控件ID */
#define ID_EXIT            0x3000
#define ID_BURN           0x3001
#define ID_RESET           0x3002
#define ID_INFO           0x3003
#define ID_TITLE          0x3004
#define ID_EXIT_INFO       0x3005

//定义控件的私有消息(所有控件的私有消息要从 WM_WIDGET 开始定义)
#define MSG_MYWRITE_RESULT WM_WIDGET+1 //烧录结果

HWND info_textbox ;
HWND res_writer_dialog;

/* 用于标记是否有资源文件无法找到 */
extern BOOL res_not_found_flag;


/**
  * @brief  烧录应用线程
  */
static void App_FLASH_Writer(void )
{
  static int thread=0;
	static rt_thread_t h_flash;
  
   //HDC hdc;
  u32 result;
   
	if(thread==0)
	{  
      h_flash=rt_thread_create("Flash writer",(void(*)(void*))App_FLASH_Writer,NULL,5*1024,1,5);
      thread =1;
      rt_thread_startup(h_flash);//启动线程
      return;
	}
	while(thread) //线程已创建了
	{     
    result = (u32)BurnFile();
    
    //发消息给hwnd,设置成"按下"状态。
    SendMessage(res_writer_dialog,MSG_MYWRITE_RESULT,result,0);

    thread = 0;       

    rt_thread_delete(h_flash);

	}
  return;
}

/**
  * @brief  烧录应用回调函数
  */
static	LRESULT	win_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	RECT rc,rc0;
	HWND wnd;
  res_writer_dialog = hwnd;
	//static RECT rc_R, rc_G, rc_B;//RGB分量指示框
  
  const WCHAR no_res_info[] = L"It's seems that the FLASH is missing some resources.\r\n\
Follow the instructions below:\r\n\r\n\
1.Insert an SD card with [srcdata] resource.\r\n\
3.Power up again the board.\r\n\
2.Click the button below to load the resources.";
  
  const WCHAR normal_res_info[] = L"This app is use to reload resources!\r\n\
If you really want to reload resources:\r\n\r\n\
1.Insert an SD card with [srcdata] resource.\r\n\
3.Power up again the board.\r\n\
2.Click the button below to load the resources.";

   //HDC hdc_mem2pic;
	switch (msg)
	{
    case WM_CREATE: {          
          
          /* 默认显示信息 */
          const WCHAR *pStr = normal_res_info;
      
          /* 找不到资源时显示信息 */
          if(res_not_found_flag)
            pStr = no_res_info;

          
          GetClientRect(hwnd,&rc); //获得窗口的客户区矩形.
      
          /* 标题 */
          rc0.x = 5;
          rc0.y = 10;
          rc0.w = rc.w-10;
          rc0.h = 40;
          
          wnd = CreateWindow(TEXTBOX,L"GUI FLASH Writer" ,WS_VISIBLE,
                                rc0.x, rc0.y, rc0.w, rc0.h, hwnd, ID_TITLE, NULL, NULL); 

          SendMessage(wnd,TBM_SET_TEXTFLAG,0,DT_SINGLELINE|DT_CENTER|DT_VCENTER|DT_BKGND);

          /* 退出提示 */
          OffsetRect(&rc0,0,rc0.h+10);  
          rc0.w = 700;
          
          if(!res_not_found_flag)
          {
            /* 若本身能找到资源文件，显示这个提示 */
            CreateWindow(TEXTBOX,L"Please [Exit] if you don't know what you are doing! --->" ,WS_VISIBLE,
                        rc0.x, rc0.y, rc0.w, rc0.h, hwnd, ID_EXIT_INFO, NULL, NULL); 
          
          
            OffsetRect(&rc0,rc0.w+10,0);  
            
            /* 退出按钮 */
            rc0.w = 70;
            CreateWindow(BUTTON, L"Exit",WS_VISIBLE,
                          rc0.x, rc0.y, rc0.w, rc0.h, hwnd, ID_EXIT, NULL, NULL); 
          }
          
          /* 提示信息 */
          OffsetRect(&rc0,0,rc0.h+10);  
          rc0.x = 5;
          rc0.w = rc.w-10;
          rc0.h = 250;
      
          CreateWindow(TEXTBOX,pStr ,WS_VISIBLE,
                        rc0.x, rc0.y, rc0.w, rc0.h, hwnd, ID_INFO, NULL, NULL); 

          /* 烧录按钮 */
          OffsetRect(&rc0,0,rc0.h+10);  
          rc0.w = 350;
          rc0.h = 70;
          
          CreateWindow(BUTTON, L"Click me to load resources",WS_VISIBLE,
                        rc0.x, rc0.y, rc0.w, rc0.h, hwnd, ID_BURN, NULL, NULL); 

          /* 复位按钮 */
          OffsetRect(&rc0,rc0.w+50,0);  
          CreateWindow(BUTTON, L"Click me to reset system",0,
                        rc0.x, rc0.y, rc0.w, rc0.h, hwnd, ID_RESET, NULL, NULL); 
      break;
	}
    
   case MSG_MYWRITE_RESULT:
    {
      u32 result = wParam;

      /* 烧录失败 */
      if(result)
      {
        EnableWindow(GetDlgItem(hwnd,ID_BURN),ENABLE);

      }
      else
      {
        /* 烧录成功 */
        ShowWindow(GetDlgItem(hwnd,ID_RESET),SW_SHOW);
        ShowWindow(GetDlgItem(hwnd,ID_BURN),SW_HIDE);
        
        SetWindowText(info_textbox,L"Load resources success!\r\n\r\nClick the button below to reset system!");

      }  
      

      break;
    }
 
	case WM_NOTIFY: {
      NMHDR *nr;
      WCHAR wbuf[128];
      u16 ctr_id; 
      u16 code,  id;
      id  =LOWORD(wParam);//获取消息的ID码
      code=HIWORD(wParam);//获取消息的类型
      ctr_id = LOWORD(wParam); //wParam低16位是发送该消息的控件ID.
      nr = (NMHDR*)lParam; //lParam参数，是以NMHDR结构体开头.
		
      if(id == ID_EXIT && code == BN_CLICKED)
      {
         PostCloseMessage(hwnd);
      }
      
      if(id == ID_BURN && code == BN_CLICKED)
      {
//         rt_enter_critical();
         
         info_textbox = GetDlgItem(hwnd,ID_INFO);
         App_FLASH_Writer();
        
         EnableWindow(GetDlgItem(hwnd,ID_BURN),DISABLE);
         ShowWindow(GetDlgItem(hwnd,ID_EXIT),SW_HIDE);
         ShowWindow(GetDlgItem(hwnd,ID_EXIT_INFO),SW_HIDE);

//         rt_exit_critical();

      }
      
      if(id == ID_RESET && code == BN_CLICKED)
      {
         NVIC_SystemReset();
      }      
      
      
		break;
	}
	case WM_DRAWITEM:
	{
		DRAWITEM_HDR *ds;
		ds = (DRAWITEM_HDR*)lParam;

//      if(ds->ID == ID_EXIT)
//      {
//			exit_owner_draw(ds);
//			return TRUE;      
//      }

	}
   case	WM_CTLCOLOR:
   {
      /* 控件在绘制前，会发送 WM_CTLCOLOR到父窗口.
       * wParam参数指明了发送该消息的控件ID;lParam参数指向一个CTLCOLOR的结构体指针.
       * 用户可以通过这个结构体改变控件的颜色值.用户修改颜色参数后，需返回TRUE，否则，系统
       * 将忽略本次操作，继续使用默认的颜色进行绘制.
       *
       */
			u16 id;
			id =LOWORD(wParam);
			if(id== ID_INFO )
			{
				CTLCOLOR *cr;
				cr =(CTLCOLOR*)lParam;
				cr->TextColor =RGB888(0,255,0);//文字颜色（RGB888颜色格式)
				cr->BackColor =RGB888(0,0,0);//背景颜色（RGB888颜色格式)
				cr->BorderColor =RGB888(255,10,10);//边框颜色（RGB888颜色格式)
				return TRUE;
			}
      else if(id == ID_TITLE)
      {
      	CTLCOLOR *cr;
				cr =(CTLCOLOR*)lParam;
				cr->TextColor =RGB888(255,255,0);//文字颜色（RGB888颜色格式)
				cr->BackColor =RGB888(0,0,0);//背景颜色（RGB888颜色格式)
				cr->BorderColor =RGB888(255,10,10);//边框颜色（RGB888颜色格式)
				return TRUE;

      }
      else if(id == ID_EXIT_INFO)
      {
      	CTLCOLOR *cr;
				cr =(CTLCOLOR*)lParam;
				cr->TextColor =RGB888(255,0,0);//文字颜色（RGB888颜色格式)
				cr->BackColor =RGB888(0,0,0);//背景颜色（RGB888颜色格式)
				cr->BorderColor =RGB888(255,10,10);//边框颜色（RGB888颜色格式)
				return TRUE;

      }
			else
			{
				return FALSE;
			}
      
   }   
   case WM_ERASEBKGND:
   {
      HDC hdc =(HDC)wParam;
      RECT rc;
      RECT rc_text = {0, 0, 100, 40};
      GetClientRect(hwnd, &rc);
      
      SetBrushColor(hdc, MapRGB(hdc, 0, 0, 0));
      FillRect(hdc, &rc);
      
      
      return TRUE;
      
   }

	case	WM_PAINT: //窗口需要重绘制时，会自动收到该消息.
	{	
      PAINTSTRUCT ps;
      HDC hdc, hdc_mem, hdc_mem1;//屏幕hdc
      RECT rc = {0,0,72,72};
      RECT rc_cli = {0,0,72,72};
      GetClientRect(hwnd, &rc_cli);
      hdc = BeginPaint(hwnd, &ps); 

		EndPaint(hwnd, &ps);
		return	TRUE;
	}
	default:
		return	DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return	WM_NULL;
}


void	GUI_RES_WRITER_DIALOG(void)
{
	HWND	hwnd;
	WNDCLASS	wcex;
	MSG msg;

  /* 重设默认字体为ASCII 内部flash字库，防止擦除时出错 */
  GUI_SetDefFont(defaultFontEn);  //设置默认的字体

	wcex.Tag = WNDCLASS_TAG;

	wcex.Style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = win_proc; //设置主窗口消息处理的回调函数.
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = NULL;//hInst;
	wcex.hIcon = NULL;//LoadIcon(hInstance, (LPCTSTR)IDI_WIN32_APP_TEST);
	wcex.hCursor = NULL;//LoadCursor(NULL, IDC_ARROW);
	//创建主窗口
	hwnd = CreateWindowEx(NULL,
                        &wcex,
                        L"GUI FLASH Writer",
                        WS_CLIPCHILDREN,
                        0, 0, GUI_XSIZE, GUI_YSIZE,
                        NULL, NULL, NULL, NULL);
   //显示主窗口
	ShowWindow(hwnd, SW_SHOW);
	//开始窗口消息循环(窗口关闭并销毁时,GetMessage将返回FALSE,退出本消息循环)。
	while (GetMessage(&msg, hwnd))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

  /* 恢复中文默认字体 */
  GUI_SetDefFont(defaultFont);  

}
