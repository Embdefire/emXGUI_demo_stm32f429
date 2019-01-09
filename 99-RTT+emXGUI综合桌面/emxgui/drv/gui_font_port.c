/**
  *********************************************************************
  * @file    gui_font_port.c
  * @version V1.0
  * @date    2018-xx-xx
  * @brief   字体管理接口
  *********************************************************************
  * @attention
  * 官网    :www.emXGUI.com
  *
  **********************************************************************
  */ 

#include	"emXGUI.h"
#include  "gui_drv_cfg.h"

#include "gui_mem_port.h"
#include "gui_font_port.h"
#include "gui_resource_port.h"

/*===================================================================================*/
/*===================================================================================*/


#include <GUI_Font_XFT.h>

/**
  * @note  若要把字体加入到内部FLASH，
  * 把相应的字体数组文件添加到工程，然后在此处加入声明
  * 并使用XFT_CreateFont创建字体
  */
extern const char GB2312_16_1BPP[];
extern const char GB2312_16_2BPP[];
extern const char GB2312_20_2BPP[];

extern const char GB2312_16_4BPP[];
extern const char GB2312_20_4BPP[];

extern const char ASCII_16_4BPP[];
extern const char ASCII_20_4BPP[];
extern const char ASCII_24_4BPP[];
extern const char ASCII_32_4BPP[];
extern const char LOGO_50_4BPP[];
extern const char ICON_100_4BPP[];
extern const char CONTROL_50_4BPP[];
extern const char CONTROL_60_8BPP[];
extern const char CONTROL_70_8BPP[];
extern const char CONTROL_80_8BPP[];



/* 默认字体 */
HFONT defaultFont =NULL;

#if(GUI_ICON_LOGO_EN)  
/* logo字体 */
HFONT logoFont =NULL;
/* 图标字体 */
HFONT iconFont_100 =NULL;
/* 控制图标字体 */
HFONT controlFont_48 =NULL;
HFONT controlFont_64 =NULL;
HFONT controlFont_72 =NULL;
#endif

/* 其它 */
HFONT GB2312_32_Font =NULL;




/*===================================================================================*/
#if (GUI_USE_EXTERN_FONT && (!GUI_FONT_LOAD_TO_RAM))

/**
  * @brief  从流媒体加载内容的回调函数
  * @param  buf[out] 存储读取到的数据缓冲区
  * @param  offset 要读取的位置
  * @param  size 要读取的数据大小
  * @param  lParam 调用函数时的自定义参数（用户参数）
  * @retval 读取到的数据大小
  */
static int font_read_data_exFlash(void *buf,int offset,int size,LONG lParam)
{
  /* 本例子中offset是具体字符数据在字体文件中的偏移 
   * lParam 是字体文件在FLASH中的基地址
  */
	offset += lParam;
  
  /* 读取具体的字模数据内容 */
	RES_DevRead(buf,offset,size);
	return size;
}
#endif

/**
  * @brief  初始化外部FLASH字体
  * @param  res_name 字体资源名字
  * @retval 返回默认字体的句柄
  */
HFONT GUI_Init_Extern_Font(const char* res_name)
{
  /* 使用流设备加载字体，按需要读取 */

  int font_base;
  HFONT hFont;
  CatalogTypeDef dir;

  font_base =RES_GetInfo_AbsAddr(res_name, &dir);
  if(font_base > 0)
  {
    hFont =XFT_CreateFontEx(font_read_data_exFlash,font_base);
  }
  else
  {
    GUI_ERROR("Can not find RES:%s",res_name);
  }
  
  if(hFont==NULL)  
     GUI_ERROR("%s font create failed",res_name);

  return hFont;
}


/**
  * @brief  GUI默认字体初始化
  * @param  无
  * @retval 返回默认字体的句柄
  */
HFONT GUI_Default_FontInit(void)
{

#if (GUI_FONT_LOAD_TO_RAM  )
  {  
    /* 整个字体文件加载至RAM */
    
    int font_base;
    
    /* 指向缓冲区的指针 */
    static u8 *pFontData_XFT=NULL;
    CatalogTypeDef dir;
    
    /* RES_GetInfo读取到的dir.offset是资源的绝对地址 */
    font_base =RES_GetInfo_AbsAddr(GUI_DEFAULT_EXTERN_FONT, &dir);

    if(font_base > 0)
    {
    	pFontData_XFT =(u8*)GUI_VMEM_Alloc(dir.size);
      if(pFontData_XFT!=NULL)
      {
        RES_DevRead(pFontData_XFT, font_base, dir.size);

        defaultFont = XFT_CreateFont(pFontData_XFT);
      }
    }
  }
#elif (GUI_USE_EXTERN_FONT)   
  {
    /* 使用流设备加载字体，按需要读取 */
    if(defaultFont==NULL)
    { 
    	defaultFont =GUI_Init_Extern_Font(GUI_DEFAULT_EXTERN_FONT);
    }
  }
#endif

    /* 若前面的字体加载失败，使用内部FLASH中的数据（工程中的C语言数组）
    *  添加字体数据时，把数组文件添加到工程，在本文件头添加相应字体数组的声明，
    *  然后调用XFT_CreateFont函数创建字体即可
    */
    if(defaultFont==NULL)
    { 
      /* 从本地加载(本地数组数据) */    	
      defaultFont =XFT_CreateFont(GUI_DEFAULT_FONT);  /*ASCii字库,20x20,4BPP抗锯齿*/
      
      /* 中文字库存储占用空间非常大，不推荐放在内部FLASH */
    	//defaultFont =XFT_CreateFont(GB2312_16_2BPP); /*GB2312字库,16x16,2BPP抗锯齿*/
    	//defaultFont =XFT_CreateFont(GB2312_20_4BPP); /*GB2312字库,20x20,4BPP抗锯齿*/
    }
#if(GUI_ICON_LOGO_EN)    
  #if 0 
    /* 内部字体 */
    /* 创建logo字体 */  
    logoFont =  XFT_CreateFont(GUI_LOGO_FONT);
    /* 创建图标字体 */  
    iconFont_100 =  XFT_CreateFont(GUI_ICON_FONT_100);
      
    /* 创建控制图标字体 */  
    controlFont_48 =  XFT_CreateFont(GUI_CONTROL_FONT_48);
    /* 创建控制图标字体 */  
    controlFont_64 =  XFT_CreateFont(GUI_CONTROL_FONT_64);
    /* 创建控制图标字体 */  
    controlFont_72 =  XFT_CreateFont(GUI_CONTROL_FONT_72);

      
    if(logoFont==NULL)  
      GUI_ERROR("logoFont create failed");
        
    if(iconFont_100 ==NULL) 
      GUI_ERROR("iconFont_100 create failed");
    
    if(controlFont_64 ==NULL) 
      GUI_ERROR("controlFont_64 create failed");
  #else
    /*放到外部flash*/    
    {      
      /* 创建logo字体 */  
      logoFont =  GUI_Init_Extern_Font(GUI_LOGO_FONT);
      /* 创建图标字体 */  
      iconFont_100 =  GUI_Init_Extern_Font(GUI_ICON_FONT_100);        
      /* 创建控制图标字体 */  
      controlFont_48 =  GUI_Init_Extern_Font(GUI_CONTROL_FONT_48); 
      /* 创建控制图标字体 */  
      controlFont_64 =  GUI_Init_Extern_Font(GUI_CONTROL_FONT_64); 
      /* 创建控制图标字体 */  
      controlFont_72 =  GUI_Init_Extern_Font(GUI_CONTROL_FONT_72); 

    }     

  #endif
    
#endif
   
#if 0    
  /* 其它 */  
  {
    /* 使用流设备加载字体，按需要读取 */
    	int font_base;
      CatalogTypeDef dir;

    	font_base =RES_GetInfo_AbsAddr("GB2312_32_4BPP.xft", &dir);
    	if(font_base > 0)
    	{
    		GB2312_32_Font =XFT_CreateFontEx(font_read_data_exFlash,font_base);
    	}  
      
      if(GB2312_32_Font ==NULL) 
        GUI_ERROR("GB2312_32_4BPP create failed");
  }
#endif  

	return defaultFont;
}

/********************************END OF FILE****************************/

