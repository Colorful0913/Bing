#include <reg52.h>                                        //包含头文件
#include <intrins.h>

#define uchar unsigned char	                              //以后unsigned char就可以用uchar代替
#define uint  unsigned int	                              //以后unsigned int 就可以用uint 代替

sfr ISP_DATA  = 0xe2;					                  //数据寄存器
sfr ISP_ADDRH = 0xe3;					                  //地址寄存器高八位
sfr ISP_ADDRL = 0xe4;					                  //地址寄存器低八位
sfr ISP_CMD   = 0xe5;					                  //命令寄存器
sfr ISP_TRIG  = 0xe6;					                  //命令触发寄存器
sfr ISP_CONTR = 0xe7;					                  //控制寄存器

sbit RS       = P2^7;                                     //1602液晶的RS管脚       
sbit RW       = P2^6;                                     //1602液晶的RW管脚 
sbit EN       = P2^5;                                     //1602液晶的EN管脚
sbit RST      = P1^3;				                      //时钟芯片DS1302的RST管脚
sbit SDA      = P1^2;				                      //时钟芯片DS1302的SDA管脚
sbit SCK      = P1^1;				                      //时钟芯片DS1302的SCK管脚
sbit Key1     = P3^2;				                      //设置时间按键
sbit Key2     = P3^3;				                      //设置闹钟按键
sbit Key3     = P3^4;				                      //秒表功能按键
sbit Key4     = P3^5;				                      //子功能按键1
sbit Key5     = P3^6;                                     //子功能按键2
sbit Buzzer   = P2^0;				                      //蜂鸣器
sbit DQ       = P1^0;				                      //DS18B20传感器的引脚定义

uchar TimeBuff[7] = {17,9,1,6,18,30,40};				  //时间数组，默认2017年9月1日，星期五，18:30:40
// TimeBuff[0] 代表年份，范围00-99
// TimeBuff[1] 代表月份，范围1-12
// TimeBuff[2] 代表日期，范围1-31
// TimeBuff[3] 代表星期，范围1-7，1是星期天，2是星期一... ...
// TimeBuff[4] 代表小时，范围00-23
// TimeBuff[5] 代表分钟，范围00-59
// TimeBuff[6] 代表秒钟，范围00-59

uchar Clock_Hour;					                      //闹钟的小时
uchar Clock_Minute;				                          //闹钟的分钟
uchar Clock_Swt;					                      //闹钟的开关
uchar Buzzer_Flag = 0;			                          //蜂鸣器工作标志

uchar Stop_Watch_Count = 0;		                          // 用于秒表计数，10毫秒加1
uint  Stop_Watch_Second = 0;	                          // 用于秒表计数，1秒加1

/*********************************************************/
// 单片机内部EEPROM不使能
/*********************************************************/
void ISP_Disable()
{
    ISP_CONTR = 0;                                        //控制寄存器赋值为0
    ISP_ADDRH = 0;                                        //地址寄存器高八位赋值为0
    ISP_ADDRL = 0;                                        //地址寄存器低八位赋值为0
}

/*********************************************************/
// 从单片机内部EEPROM读一个字节
/*********************************************************/
unsigned char ReadE2PROM(unsigned int addr)
{
    ISP_DATA = 0x00;                                      //数据寄存器清零
    ISP_CONTR = 0x83;                                     //允许改变内部E2PROM,存取数据速度为5MHz
    ISP_CMD = 0x01;                                       //读命令
    ISP_ADDRH = (unsigned char)(addr >> 8);               //输入高8位地址
    ISP_ADDRL = (unsigned char)(addr & 0xff);             //输入低8位地址
    ISP_TRIG = 0x46;                                      //先向命令触发寄存器写入0x46
    ISP_TRIG = 0xb9;                                      //再向命令触发寄存器写入0xb9,完成触发
    _nop_();                                              //延时大约1us
    ISP_Disable();                                        //单片机内部EEPROM不使能
    
    return ISP_DATA;                                      //返回读的数据
}

/*********************************************************/
// 从单片机内部EEPROM写一个字节
/*********************************************************/
void WriteE2PROM(unsigned char dat, unsigned int addr)
{
    ISP_CONTR = 0x83;                                     //允许改变内部E2PROM,存取数据速度为5MHz
    ISP_CMD = 0x02;                                       //写命令
    ISP_ADDRH = (unsigned char)(addr >> 8);               //输入高8位地址
    ISP_ADDRL = (unsigned char)(addr & 0xff);             //输入低8位地址
    ISP_DATA = dat;                                       //输入要写的数据
    ISP_TRIG = 0x46;                                      //先向命令触发寄存器写入0x46
    ISP_TRIG = 0xb9;                                      //再向命令触发寄存器写入0xb9,完成触发 
    _nop_();                                              //延时大约1us
    ISP_Disable();                                        //单片机内部EEPROM不使能
}

/*********************************************************/
// 从单片机内部EEPROM扇区擦除
/*********************************************************/
void SectorErase(unsigned int addr)
{
    ISP_CONTR = 0x83;                                     //允许改变内部E2PROM,存取数据速度为5MHz
    ISP_CMD = 0x03;                                       //扇区擦除命令
    ISP_ADDRH = (unsigned char)(addr >> 8);               //输入高8位地址
    ISP_ADDRL = (unsigned char)(addr & 0xff);             //输入低8位地址
    ISP_TRIG = 0x46;                                      //先向命令触发寄存器写入0x46
    ISP_TRIG = 0xb9;                                      //再向命令触发寄存器写入0xb9,完成触发
    _nop_();                                              //延时大约1us
    ISP_Disable();                                        //单片机内部EEPROM不使能
}

/*********************************************************/
// 延时X*10us函数
/*********************************************************/
void DelayX10us(unsigned char t)
{
    do{
        _nop_();                                          //延时1us
        _nop_();                                          //延时1us
        _nop_();                                          //延时1us
        _nop_();                                          //延时1us
        _nop_();                                          //延时1us
        _nop_();                                          //延时1us
        _nop_();                                          //延时1us
        _nop_();                                          //延时1us
        }while(--t);                                      //t先自减，然后判断t的值是否大于0
}
/*********************************************************/
// 复位18B20
/*********************************************************/
bit Reset18B20()
{
    bit ack;                                              //定义一个bit型变量

    DQ = 0;                                               //数据引脚拉低
    DelayX10us(50);                                       //延时500us
    DQ = 1;                                               //数据引脚拉高
    DelayX10us(6);                                        //延时60us
    ack = DQ;                                             //读取复位值
    while(!DQ);                                           //判断是否为高电平
    
    return ack;                                           //返回复位值
}
/*********************************************************/
// 向18B20写数据或命令
/*********************************************************/
void Write18B20(unsigned char dat)
{
    unsigned char mask;                                   //定义一个mask变量
    
    for(mask=0x01; mask!=0; mask<<=1)                     //循环8次，从低到高依次写入
    {
        DQ = 0;                                           //初始电平为0
        _nop_();                                          //延时大约为1us
        _nop_();                                          //延时大约为1us
        if((dat & mask) == 0)                             //判断写入的一位是否为0
            DQ = 0;                                       //为0，数据位为0
        else
            DQ = 1;                                       //否则，数据位为1
        DelayX10us(6);                                    //延时60us
        DQ = 1;                                           //拉高数据位，为下次写作准备
    }
}
/*********************************************************/
// 从18B20读数据或命令
/*********************************************************/
unsigned char Read18B20()
{
    unsigned char mask;                                   //定义一个mask变量
    unsigned char dat;                                    //定义一个dat变量,用于保存读到的数据
    
    for(mask=0x01; mask!=0; mask<<=1)                     //循环8次，从低到高依次读出
    {
        DQ = 0;                                           //初始电平为0
        _nop_();                                          //延时大约为1us
        _nop_();                                          //延时大约为1us
        DQ = 1;                                           //数据电平拉高
        _nop_();                                          //延时大约为1us
        _nop_();                                          //延时大约为1us
        if(DQ == 0)                                       //判读数据引脚是否为0
            dat &= ~mask;                                 //若为0，dat按位与上0xfe,读出0
        else
            dat |= mask;                                  //否则，dat按位或上0x01,读出1
        DelayX10us(6);                                    //延时60us
    }
    
    return dat;                                           //返回读出数据dat
}
/*********************************************************/
// 获取18B20温度数据
/*********************************************************/
int Get18B20Temp()
{
    bit ack;                                             //定义bit型变量
    int temp;                                            //定义有符号整形变量temp,用于保存温度数据
    unsigned char LSB, MSB;                              //定义两个无符号字符型变量，用于保存从18B20中读出的两个数据
    
    ack = Reset18B20();                                  //获取18B20复位位
    if(ack == 0)                                         //判读是否复位
    {
        Write18B20(0xcc);                                //跳过检测ROM
        Write18B20(0x44);                                //启动温度转换指令
    }
    
    ack = Reset18B20();                                  //获取18B20复位位
    if(ack == 0)                                         //判读是否复位
    {
        Write18B20(0xcc);                                //跳过检测ROM
        Write18B20(0xbe);                                //读取温度指令
        LSB = Read18B20();                               //先读第8位
        MSB = Read18B20();                               //再读高8位
        temp = ((int)MSB<<8) + LSB;                      //高8位，第8位整合成一个有符号的整型变量，并把值保存在temp中
        temp = temp * 0.0625 * 10;                       //合成温度值并放大10倍
    }
    
    return temp;                                         //返回读到的温度数值
}

/********************************************************/
// 延时X*ms函数
/********************************************************/
void DelayMs(unsigned int ms)
{
    unsigned int i, j;                                    //定义两个无符号整形变量i,j
    
    for(i=0; i<ms; i++)                                   
        for(j=0; j<112; j++);
}
/*********************************************************/
// 1602液晶写命令函数，cmd就是要写入的命令
/*********************************************************/
void WriteLcdCmd(uchar cmd)
{ 
	RS = 0;                                               //数据命令选择引脚置为低电平，选择写入命令
	RW = 0;                                               //读写选择引脚置为低电平，选择写入
	EN = 0;                                               //使能引脚置为低电平
	P0=cmd;                                               //要写入的命令赋值给P0端口
	DelayMs(2);                                           //延时2ms
	EN = 1;                                               //使能引脚置为高电平
	DelayMs(2);                                           //延时2ms
	EN = 0;	                                              //使能引脚置为低电平
}
/*********************************************************/
// 1602液晶写数据函数，dat就是要写入的数据
/*********************************************************/
void WriteLcdData(uchar dat)
{
	RS = 1;                                               //数据命令选择引脚置为高电平，选择写入数据
	RW = 0;                                               //读写选择引脚置为低电平，选择写入
	EN = 0;                                               //使能引脚置为低电平
	P0=dat;                                               //要写入的数据赋值给P0端口
	DelayMs(2);                                           //延时2ms
	EN = 1;                                               //使能引脚置为高电平 
	DelayMs(2);                                           //延时2ms
	EN = 0;                                               //使能引脚置为低电平
}

/*********************************************************/
// 液晶坐标设置函数
/*********************************************************/
void SetLcdCursor(unsigned char line, unsigned char column)
{
    if(line == 0)                                         //判断是否为第一行
        WriteLcdCmd(column + 0x80);                       //若是，写入第一行列坐标
    if(line == 1)                                         //判断是否为第二行
        WriteLcdCmd(column + 0x80 + 0x40);                //若是，写入第二行列坐标
}
/*********************************************************/
// 液晶显示字符串函数
/*********************************************************/
void ShowLcdStr(unsigned char *str)
{
    while(*str != '\0')                                  //当没有指向结束符
        WriteLcdData(*str++);                            //字符指针加1
}
/*********************************************************/
// 液晶初始化函数
/*********************************************************/
void LcdInit()
{
    WriteLcdCmd(0x38);                                    //16*2显示，5*7点阵，8位数据口
    WriteLcdCmd(0x06);                                    //地址加1，当写入数据后光标右移
    WriteLcdCmd(0x0c);                                    //开显示，不显示光标
    WriteLcdCmd(0x01);                                    //清屏
}
/*********************************************************/
// 液晶显示内容的初始化
/*********************************************************/
void ShowLcdInit()
{
    SetLcdCursor(0, 0);                                   //设置坐标第一行，第一列
    ShowLcdStr("20  -  -        ");                       //显示"20  -  -        "
    SetLcdCursor(1, 0);                                   //设置坐标第二行，第一列
    ShowLcdStr("  :  :         C");                       //显示"  :  :         C"
    SetLcdCursor(1, 14);                                  //设置坐标第2行，第15列
    WriteLcdData(0xdf);                                   //温度单位摄氏度上面的圆圈符号
}
/*********************************************************/
// 液晶输出数字
/*********************************************************/
void ShowLcdNum(unsigned char num)
{
    WriteLcdData(num/10 + 48);                            //分离出十位
    WriteLcdData(num%10 + 48);                            //分离出个位
}
/*********************************************************/
// 液晶显示星期
/*********************************************************/
void ShowLcdWeek(uchar week)
{
    switch(week)
    {
        case 1: ShowLcdStr("Sun");  break;                //week为1，显示Sun
        case 2: ShowLcdStr("Mon");  break;                //week为2，显示Mon
        case 3: ShowLcdStr("Tue");  break;                //week为3，显示Tue
        case 4: ShowLcdStr("Wed");  break;                //week为4，显示Wed
        case 5: ShowLcdStr("Thu");  break;                //week为5，显示Thu
        case 6: ShowLcdStr("Fri");  break;                //week为6，显示Fri
        case 7: ShowLcdStr("Sat");  break;                //week为7，显示Sat
        default:                    break;                //结束swithc判断
    }
}
/*********************************************************/
// 在液晶上显示温度
/*********************************************************/
void ShowLcdTemp(int temp)
{
	if(temp < 0)	                                      //如果温度为负数		 					
	{
		WriteLcdData('-');							      //显示负号	
		temp = 0 - temp;								  //负数转为正数
	}
	else if(temp >= 1000)                                 //温度值大于等于100℃				   			
	{
		WriteLcdData(temp/1000 + 0x30);		              //分离出百位	
	}
    else
	{
		WriteLcdData(' ');                                //加个空格
	}
	WriteLcdData(temp%1000/100 + 0x30);	                  //分离出十位
	WriteLcdData(temp%100/10 + 0x30);		              //分离出个位
	WriteLcdData('.');			 					      //添加小数点
	WriteLcdData(temp%10+0x30);				              //小数后一位
}
/*********************************************************/
// 刷新时间显示
/*********************************************************/
void FlashTime()
{
    SetLcdCursor(0, 2);                                   //设置坐标第1行，第3列
    ShowLcdNum(TimeBuff[0]);                              //显示年份
    
    SetLcdCursor(0, 5);                                   //设置坐标第1行，第6列
    ShowLcdNum(TimeBuff[1]);                              //显示月份
    
    SetLcdCursor(0, 8);                                   //设置坐标第1行，第9列
    ShowLcdNum(TimeBuff[2]);                              //显示日期
    
    SetLcdCursor(1, 0);                                   //设置坐标第2行，第1列
    ShowLcdNum(TimeBuff[4]);                              //显示小时
    
    SetLcdCursor(1, 3);                                   //设置坐标第2行，第4列
    ShowLcdNum(TimeBuff[5]);                              //显示分钟
    
    SetLcdCursor(1, 6);                                   //设置坐标第2行，第7列
    ShowLcdNum(TimeBuff[6]);                              //显示秒钟
    
    SetLcdCursor(0, 12);                                  //设置坐标第1行，第13列
    ShowLcdWeek(TimeBuff[3]);                             //显示星期	
}

/*********************************************************/
// 初始化DS1302
/*********************************************************/
void DS1302_Init(void)
{
    RST = 0;                                              //RST脚置低
    SCK = 0;                                              //SCK脚置低
    SDA = 0;                                              //SDA脚置低	
}
/*********************************************************/
// 向DS1302写入一字节数据
/*********************************************************/
void DS1302_Write_Byte(uchar addr, uchar dat)
{
    uchar i;                                              //定义无符号变量i，for循环用  
    
    RST = 1;                                              //RST引脚拉为高电平 
    
    /* 写入目标地址：addr*/
    for(i=0; i<8; i++)                                    //循环8次 
    {
        if(addr & 0x01)                                   //判断addr最后一位是否为1 
            SDA = 1;                                      //若为1，SDA引脚输出为1 
        else                                              //否则
            SDA = 0;                                      //SDA引脚输出为0 
        SCK = 1;                                          //SCK拉为高电平 
        _nop_();                                          //延时1us 
        SCK = 0;                                          //SCK拉为低电平 
        _nop_();                                          //延时1us 
        
        addr = addr >> 1;                                 //addr向右移动一位 
    }
    
    /* 写入数据：dat*/
    for(i=0; i<8; i++)                                    //循环8次 
    {
        if(dat & 0x01)                                    //判断dat最后一位是否为1  
            SDA = 1;                                      //若为1，SDA引脚输出为1 
        else                                              //否则 
            SDA = 0;                                      //SDA引脚输出为0 
        SCK = 1;                                          //SCK拉为高电平 
        _nop_();                                          //延时1us 
        SCK = 0;                                          //SCK拉为低电平 
        _nop_();                                          //延时1us 
        
        dat = dat >> 1;                                   //addr向右移动一位 
    }
    
    RST = 0;                                              //RST引脚拉为低电平 
}
/*********************************************************/
// 从DS1302读出一字节数据
/*********************************************************/
uchar DS1302_Read_Byte(uchar addr)
{
    uchar i;                                              //定义无符号变量i，for循环用 
    uchar temp;                                           //temp用来保存读到的数据 
    
    RST = 1;                                              //RST引脚拉为高电平 
    
    /* 写入目标地址：addr*/
    for(i=0; i<8; i++)                                    //循环8次  
    {
        if(addr & 0x01)                                   //判断addr最后一位是否为1  
            SDA = 1;                                      //若为1，SDA引脚输出为1  
        else                                              //否则  
            SDA = 0;                                      //SDA引脚输出为0  
        SCK = 1;                                          //SCK拉为高电平 
        _nop_();                                          //延时1us 
        SCK = 0;                                          //SCK拉为低电平 
        _nop_();                                          //延时1us 
        
        addr = addr >> 1;                                 //addr向右移动一位 
    }
    
    /* 读出该地址的数据 */
    for(i=0; i<8; i++)                                    //循环8次 
    {
        temp = temp >> 1;                                 //temp向右移动一位 
        
        if(SDA == 1)                                      //判断SDA是否为1 
            temp |= 0x80;                                 //若为1，temp按位或上0x80 
        else                                              //否则
            temp &= 0x7f;                                 //temp按位与上0x7f 
        SCK = 1;                                          //SCK拉为高电平 
        _nop_();                                          //延时1us 
        SCK = 0;                                          //SCK拉为低电平 
        _nop_();                                          //延时1us 
    }
    
    RST = 0;                                              //RST引脚拉为低电平 
    
    return temp;                                          //返回读到的数据 
}
/*********************************************************/
// 向DS1302写入时间数据
/*********************************************************/
void DS1302_Write_Time()
{
    uchar i;                                              //定义无符号字符型变量i，for循环使用
    uchar temp1;                                          //temp1用来保存转换成的二进制的十位
    uchar temp2;                                          //temp2用来保存转换成的二进制的个位
    
    for(i=0; i<7; i++)                                    //循环7次
    {
        temp1 = (TimeBuff[i]/10) << 4;                    //分离出十位并左移4位
        temp2 = TimeBuff[i] % 10;                         //分离出个位
        TimeBuff[i] = temp1 + temp2;                      //十位与个位整合
    }
    
    DS1302_Write_Byte(0x8E, 0x00);                        //关闭写保护
    DS1302_Write_Byte(0x80, 0x80);                        //暂停时钟
    DS1302_Write_Byte(0x8C, TimeBuff[0]);                 //年
    DS1302_Write_Byte(0x88, TimeBuff[1]);                 //月
    DS1302_Write_Byte(0x86, TimeBuff[2]);                 //日
    DS1302_Write_Byte(0x8A, TimeBuff[3]);                 //星期
    DS1302_Write_Byte(0x84, TimeBuff[4]);                 //时
    DS1302_Write_Byte(0x82, TimeBuff[5]);                 //分
    DS1302_Write_Byte(0x80, TimeBuff[6]);                 //秒
    DS1302_Write_Byte(0x80, TimeBuff[6]&0x7F);            //运行时钟
    DS1302_Write_Byte(0x8E, 0x80);                        //打开写保护
}
/*********************************************************/
// 从DS1302读出时间数据
/*********************************************************/
void DS1302_Read_Time()
{
    uchar i;                                              //定义无符号字符型变量i，for循环使用
    
    TimeBuff[0] = DS1302_Read_Byte(0x8D);				  //年 
	TimeBuff[1] = DS1302_Read_Byte(0x89);			      //月 
	TimeBuff[2] = DS1302_Read_Byte(0x87);				  //日 
	TimeBuff[3] = DS1302_Read_Byte(0x8B);				  //星期
	TimeBuff[4] = DS1302_Read_Byte(0x85);				  //时 
	TimeBuff[5] = DS1302_Read_Byte(0x83);				  //分 
	TimeBuff[6] = (DS1302_Read_Byte(0x81)) & 0x7F;		  //秒 
    
    for(i=0;i<7;i++)		                              //BCD转十进制
	{           
		TimeBuff[i] = (TimeBuff[i]/16)*10 + TimeBuff[i]%16;
	}
}

/*********************************************************/
// 按键按下去提示声
/*********************************************************/
void KeySound()
{
    Buzzer = 0;                                           //蜂鸣器响
    DelayMs(40);                                          //延时40ms
    Buzzer = 1;                                           //蜂鸣器不响
}

/*********************************************************/
// 按键扫描(设置时间)
/*********************************************************/
void KeyScanf1()
{
    if(Key1 == 0)                                         //检测设置键是否被按下
    {
        KeySound();                                       //按键按下去发出提示音
        WriteLcdCmd(0x0f);			                      //启动光标闪烁
        SetLcdCursor(0,3);					              //定位光标到年份闪烁
        DelayMs(10);						              //延时等待，消除按键按下的抖动
        while(!Key1);				                      //等待按键释放
        DelayMs(10);						              //延时等待，消除按键松开的抖动
        
        /* 调整年份 */
        while(1)
        {
            if(Key4 == 0)							      //如果减按键被按下去
            {
                if(TimeBuff[0] > 0)						  //判断年份是否大于0
                    TimeBuff[0]--;                        //是的话就减去1
                KeySound();
                SetLcdCursor(0, 2);						  //光标定位到年份的位置
                ShowLcdNum(TimeBuff[0]);		          //刷新显示改变后的年份
                SetLcdCursor(0, 3);                       //定位光标到年份闪烁
                DelayMs(300);							  //延时0.3秒左右
            }
            if(Key5 == 0)							      //如果加按键被下去
            {
                if(TimeBuff[0] < 99)					  //判断年份是否小于99
                    TimeBuff[0]++;                        //是的话就加上1
                KeySound();                               //按键按下去发出提示音
                SetLcdCursor(0, 2);						  //光标定位到年份的位置
                ShowLcdNum(TimeBuff[0]);		          //刷新显示改变后的年份
                SetLcdCursor(0, 3);                       //定位光标到年份闪烁
                DelayMs(300);							  //延时0.3秒左右
            }
            if(Key1 == 0)                                 //如果设置键被按下
			{
                KeySound();                               //按键按下去发出提示音
				break;                                    //退出此while(1)循环
			}    
        } 

        SetLcdCursor(0,6);					              //定位光标到月份闪烁           
        DelayMs(10);						              //延时等待，消除按键按下的抖动
        while(!Key1);				                      //等待按键释放
        DelayMs(10);						              //延时等待，消除按键松开的抖动

        /* 调整月份 */
        while(1)
        {
            if(Key4 == 0)							      //如果减按键被按下去
            {
                if(TimeBuff[1] > 1)						  //判断月份是否大于1
                    TimeBuff[1]--;                        //是的话就减去1
                KeySound();                               //按键按下去发出提示音
                SetLcdCursor(0, 5);						  //光标定位到月份的位置
                ShowLcdNum(TimeBuff[1]);		          //刷新显示改变后的月份
                SetLcdCursor(0, 6);                       //定位光标到月份闪烁
                DelayMs(300);							  //延时0.3秒左右
            }
            if(Key5 == 0)							      //如果加按键被下去
            {
                if(TimeBuff[1] < 12)					  //判断月份是否小于12
                    TimeBuff[1]++;                        //是的话就加上1
                KeySound();                               //按键按下去发出提示音
                SetLcdCursor(0, 5);						  //光标定位到月份的位置
                ShowLcdNum(TimeBuff[1]);		          //刷新显示改变后的月份
                SetLcdCursor(0, 6);                       //定位光标到月份闪烁
                DelayMs(300);							  //延时0.3秒左右
            }
            if(Key1 == 0)                                 //如果设置键被按下
			{
                KeySound();                               //按键按下去发出提示音
				break;                                    //退出此while(1)循环
			}    
        } 
        
        SetLcdCursor(0,9);					              //定位光标到日期闪烁           
        DelayMs(10);						              //延时等待，消除按键按下的抖动
        while(!Key1);				                      //等待按键释放
        DelayMs(10);						              //延时等待，消除按键松开的抖动
        /* 调整日期 */
        while(1)
        {
            if(Key4 == 0)							      //如果减按键被按下去
            {
                if(TimeBuff[2] > 1)						  //判断日期是否大于1
                    TimeBuff[2]--;                        //是的话就减去1
                KeySound();                               //按键按下去发出提示音
                SetLcdCursor(0, 8);						  //光标定位到日期的位置
                ShowLcdNum(TimeBuff[2]);		          //刷新显示改变后的日期
                SetLcdCursor(0, 9);                       //定位光标到日期闪烁
                DelayMs(300);							  //延时0.3秒左右
            }
            if(Key5 == 0)							      //如果加按键被下去
            {
                if(TimeBuff[2] < 31)					  //判断日期是否小于31
                    TimeBuff[2]++;                        //是的话就加上1
                KeySound();                               //按键按下去发出提示音
                SetLcdCursor(0, 8);						  //光标定位到日期的位置
                ShowLcdNum(TimeBuff[2]);		          //刷新显示改变后的日期
                SetLcdCursor(0, 9);                       //定位光标到日期闪烁
                DelayMs(300);							  //延时0.3秒左右
            }
            if(Key1 == 0)                                 //如果设置键被按下
			{
                KeySound();                               //按键按下去发出提示音
				break;                                    //退出此while(1)循环
			}    
        }
        
        SetLcdCursor(0,14);					              //定位光标到星期闪烁           
        DelayMs(10);						              //延时等待，消除按键按下的抖动
        while(!Key1);				                      //等待按键释放
        DelayMs(10);						              //延时等待，消除按键松开的抖动
        
        /* 调整星期 */
        while(1)
        {
            if(Key4 == 0)							      //如果减按键被按下去
            {
                if(TimeBuff[3] > 1)						  //判断星期是否大于1
                    TimeBuff[3]--;                        //是的话就减去1
                KeySound();                               //按键按下去发出提示音
                SetLcdCursor(0, 12);					  //光标定位到星期的位置
                ShowLcdWeek(TimeBuff[3]);		          //刷新显示改变后的星期
                SetLcdCursor(0, 14);                      //定位光标到星期闪烁
                DelayMs(300);							  //延时0.3秒左右
            }
            if(Key5 == 0)							      //如果加按键被按下去
            {
                if(TimeBuff[3] < 7)						  //判断星期是否小于7
                    TimeBuff[3]++;                        //是的话就加上1
                KeySound();                               //按键按下去发出提示音
                SetLcdCursor(0, 12);					  //光标定位到日期的位置
                ShowLcdWeek(TimeBuff[3]);		          //刷新显示改变后的星期
                SetLcdCursor(0, 14);                      //定位光标到星期闪烁
                DelayMs(300);							  //延时0.3秒左右
            }
            if(Key1 == 0)                                 //如果设置键被按下
			{
                KeySound();                               //按键按下去发出提示音
				break;                                    //退出此while(1)循环
			}    
        }
        
        SetLcdCursor(1,1);					              //定位光标到小时闪烁           
        DelayMs(10);						              //延时等待，消除按键按下的抖动
        while(!Key1);				                      //等待按键释放
        DelayMs(10);						              //延时等待，消除按键松开的抖动
        /* 调整小时 */
        while(1)
        {
            if(Key4 == 0)							      //如果减按键被按下去
            {
                if(TimeBuff[4] > 0)						  //判断小时是否大于0
                    TimeBuff[4]--;                        //是的话就减去1
                KeySound();                               //按键按下去发出提示音
                SetLcdCursor(1, 0);						  //光标定位到小时的位置
                ShowLcdNum(TimeBuff[4]);		          //刷新显示改变后的小时
                SetLcdCursor(1, 1);                       //定位光标到小时闪烁
                DelayMs(300);							  //延时0.3秒左右
            }
            if(Key5 == 0)							      //如果加按键被按下去
            {
                if(TimeBuff[4] < 23)					  //判断小时是否小于23
                    TimeBuff[4]++;                        //是的话就加上1
                KeySound();                               //按键按下去发出提示音
                SetLcdCursor(1, 0);						  //光标定位到小时的位置
                ShowLcdNum(TimeBuff[4]);		          //刷新显示改变后的小时
                SetLcdCursor(1, 1);                       //定位光标到小时闪烁
                DelayMs(300);							  //延时0.3秒左右
            }
            if(Key1 == 0)                                 //如果设置键被按下
			{
                KeySound();                               //按键按下去发出提示音
				break;                                    //退出此while(1)循环
			}    
        }
        
        SetLcdCursor(1,4);					              //定位光标到分钟闪烁           
        DelayMs(10);						              //延时等待，消除按键按下的抖动
        while(!Key1);				                      //等待按键释放
        DelayMs(10);						              //延时等待，消除按键松开的抖动
        /* 调整分钟 */
        while(1)
        {
            if(Key4 == 0)							      //如果减按键被按下去
            {
                if(TimeBuff[5] > 0)						  //判断分钟是否大于0
                    TimeBuff[5]--;                        //是的话就减去1
                KeySound();                               //按键按下去发出提示音
                SetLcdCursor(1, 3);						  //光标定位到分钟的位置
                ShowLcdNum(TimeBuff[5]);		          //刷新显示改变后的分钟
                SetLcdCursor(1, 4);                       //定位光标到分钟闪烁
                DelayMs(300);							  //延时0.3秒左右
            }
            if(Key5 == 0)							      //如果加按键被按下去
            {
                if(TimeBuff[5] < 59)					  //判断分钟是否小于59
                    TimeBuff[5]++;                        //是的话就加上1
                KeySound();                               //按键按下去发出提示音
                SetLcdCursor(1, 3);						  //光标定位到分钟的位置
                ShowLcdNum(TimeBuff[5]);		          //刷新显示改变后的分钟
                SetLcdCursor(1, 4);                       //定位光标到分钟闪烁
                DelayMs(300);							  //延时0.3秒左右
            }
            if(Key1 == 0)                                 //如果设置键被按下
			{
                KeySound();                               //按键按下去发出提示音
				break;                                    //退出此while(1)循环
			}    
        }
        
        SetLcdCursor(1,7);					              //定位光标到秒钟闪烁           
        DelayMs(10);						              //延时等待，消除按键按下的抖动
        while(!Key1);				                      //等待按键释放
        DelayMs(10);						              //延时等待，消除按键松开的抖动
        /* 调整秒钟 */
        while(1)
        {
            if(Key4 == 0)							      //如果减按键被按下去
            {
                if(TimeBuff[6] > 0)						  //判断秒钟是否大于0
                    TimeBuff[6]--;                        //是的话就减去1
                KeySound();                               //按键按下去发出提示音
                SetLcdCursor(1, 6);						  //光标定位到秒钟的位置
                ShowLcdNum(TimeBuff[6]);		          //刷新显示改变后的秒钟
                SetLcdCursor(1, 7);                       //定位光标到秒钟闪烁
                DelayMs(300);							  //延时0.3秒左右
            }
            if(Key5 == 0)							      //如果加按键被按下去
            {
                if(TimeBuff[6] < 59)					  //判断秒钟是否小于59
                    TimeBuff[6]++;                        //是的话就加上1
                KeySound();                               //按键按下去发出提示音
                SetLcdCursor(1, 6);						  //光标定位到秒钟的位置
                ShowLcdNum(TimeBuff[6]);		          //刷新显示改变后的秒钟
                SetLcdCursor(1, 7);                       //定位光标到秒钟闪烁
                DelayMs(300);							  //延时0.3秒左右
            }
            if(Key1 == 0)                                 //如果设置键被按下
			{
                KeySound();                               //按键按下去发出提示音
				break;                                    //退出此while(1)循环
			}    
        }
        /* 退出前的设置 */
		WriteLcdCmd(0x0C);			                      //关闭光标闪烁
		DS1302_Write_Time();		                      //把新设置的时间值存入DS1302芯片
		DelayMs(10);						              //延时等待，消除按键按下的抖动
		while(!Key1);				                      //等待按键释放
		DelayMs(10);						              //延时等待，消除按键松开的抖动        
    }
}
/*********************************************************/
// 按键扫描(设置闹钟)
/*********************************************************/
void KeyScanf2()
{
   if(Key2 == 0)                                          //判断闹钟引脚是否按下
   {
       KeySound();                                        //按键按下去发出提示音
       SetLcdCursor(0,0);                                 // 设置坐标第1行，第1列
       ShowLcdStr("Alarm Clock Set ");                    //液晶显示为闹钟设置的界面
       SetLcdCursor(1,0);                                 //设置坐标第2行，第1列
       ShowLcdStr("     :          ");                    //显示字符串"     :          "
       SetLcdCursor(1,3);                                 // 设置坐标第2行，第4列
       ShowLcdNum(Clock_Hour);                            //显示闹钟的小时
       SetLcdCursor(1,6);                                 //设置坐标第2行，第7列
       ShowLcdNum(Clock_Minute);                          //显示闹钟的分钟
       SetLcdCursor(1,10);                                //设置坐标第2行，第11列
       if(Clock_Swt==0)                                   //判断闹钟是否关闭
		{
			ShowLcdStr("OFF");                            //若关闭显示"OFF"
		}
		else                                              //否则
		{
			ShowLcdStr(" ON");                            //显示" ON"
		}
        SetLcdCursor(1,4);								  //光标定位
        WriteLcdCmd(0x0f);								  //光标闪烁
        DelayMs(10);									  //延时等待，消除按键按下的抖动
		while(!Key2);								      //等待按键释放
		DelayMs(10);									  //延时等待，消除按键松开的抖动
        
        /* 调整闹钟小时 */
        while(1)
		{
			if(Key4 == 0)							      //如果减按键被按下去
			{
				if(Clock_Hour > 0)						  //判断闹钟小时是否大于0
					Clock_Hour--;						  //是的话就减去1
                KeySound();                               //按键按下去发出提示音
				SetLcdCursor(1,3);						  //光标定位到闹钟小时的位置
				ShowLcdNum(Clock_Hour);		              //刷新显示改变后的闹钟小时
				SetLcdCursor(1,4);						  //定位光标到闹钟小时闪烁
				DelayMs(300);							  //延时0.3秒左右
			}
			
			if(Key5 == 0)								  //如果加按键被按下去
			{
				if(Clock_Hour < 23)						  //判断闹钟小时是否小于23
					Clock_Hour++;						  //是的话就加上1
                KeySound();                               //按键按下去发出提示音
				SetLcdCursor(1,3);						  //光标定位到闹钟小时的位置
				ShowLcdNum(Clock_Hour);		              //刷新显示改变后的闹钟小时
				SetLcdCursor(1,4);						  //定位光标到闹钟小时闪烁
				DelayMs(300);							  //延时0.3秒左右
			}
			
			if(Key2 == 0)                                 //如果设置键被按下
			{
                KeySound();                               //按键按下去发出提示音
				break;                                    //退出此while(1)循环
			}
		}
		        
		SetLcdCursor(1,7);					              //定位光标到闹钟分钟的闪烁
		DelayMs(10);						              //延时等待，消除按键按下的抖动
		while(!Key2);			                          //等待按键释放
		DelayMs(10);						              //延时等待，消除按键松开的抖动
		
		/* 调整分钟 */
		while(1)
		{
			if(Key4 == 0)							      //如果减按键被按下去
			{
				if(Clock_Minute > 0)					  //判断闹钟分钟是否大于0
					Clock_Minute--;						  //是的话就减去1
                KeySound();                               //按键按下去发出提示音
				SetLcdCursor(1,6);						  //光标定位到闹钟分钟的位置
				ShowLcdNum(Clock_Minute);	              //刷新显示改变后的闹钟分钟
				SetLcdCursor(1,7);						  //定位光标到闹钟分钟闪烁
				DelayMs(300);							  //延时0.3秒左右
			}
			
			if(Key5 == 0)								  //如果加按键被按下去
			{
				if(Clock_Minute < 59)					  //判断闹钟分钟是否小于59
					Clock_Minute++;						  //是的话就加上1
                KeySound();
				SetLcdCursor(1,6);						  //光标定位到闹钟分钟的位置
				ShowLcdNum(Clock_Minute);	              //刷新显示改变后的闹钟分钟
				SetLcdCursor(1,7);						  //定位光标到闹钟分钟闪烁
				DelayMs(300);							  //延时0.3秒左右
			}
			
			if(Key2 == 0)                                 //如果设置键被按下
			{
                KeySound();                               //按键按下去发出提示音
				break;                                    //退出此while(1)循环
			}
		}
        
		SetLcdCursor(1,12);				                  //定位光标到闹钟开关的位置闪烁
		DelayMs(10);						              //延时等待，消除按键按下的抖动
		while(!Key2);			                          //等待按键释放
		DelayMs(10);						              //延时等待，消除按键松开的抖动
        /* 闹钟开关 */
		while(1)
		{
			if(Key4 == 0)							      //如果减按键被按下去
			{
				if(Clock_Swt == 1)						  //判断闹钟是否开启
					Clock_Swt = 0;						  //关闭闹钟
                KeySound();                               //按键按下去发出提示音
				SetLcdCursor(1,10);						  //光标定位到秒钟开关的位置
				ShowLcdStr("OFF");					      //液晶显示“OFF”
				SetLcdCursor(1,12);						  //定位光标到闹钟开关的位置闪烁
				DelayMs(300);							  //延时0.3秒左右
			}
			
			if(Key5 == 0)								  //如果加按键被按下去
			{
				if(Clock_Swt == 0)						  //判断闹钟是否关闭
					Clock_Swt = 1;						  //启动闹钟
                KeySound();                               //按键按下去发出提示音
				SetLcdCursor(1,10);						  //光标定位到秒钟开关的位置
				ShowLcdStr(" ON");					      //液晶显示“ ON”
				SetLcdCursor(1,12);						  //定位光标到闹钟开关的位置闪烁
				DelayMs(300);							  //延时0.3秒左右
			}			
			if(Key2 == 0)                                 //如果设置键被按下
			{
                KeySound();                               //按键按下去发出提示音
				break;                                    //退出此while(1)循环
			}
		}
        
        /* 退出前的设置 */
		WriteLcdCmd(0x0C);			                      //关闭光标闪烁
		ShowLcdInit();					                  //液晶显示内容恢复为检测界面的
		DelayMs(10);						              //延时等待，消除按键按下的抖动
		while(!Key2);			                          //等待按键释放
		DelayMs(10);						              //延时等待，消除按键松开的抖动
		SectorErase(0x2000);
		WriteE2PROM(Clock_Hour, 0x2000);			      //往0x2000这个地址写入闹钟的小时 
		WriteE2PROM(Clock_Minute, 0x2001);		          //往0x2001这个地址写入闹钟的分钟
		WriteE2PROM(Clock_Swt, 0x2002);				      //往0x2002这个地址写入闹钟的开关      
   }
}
/*********************************************************/
// 进入/退出 秒表
/*********************************************************/
void KeyScanf3()
{
    if(Key3 == 0)                                         //判断秒表设置按键是否被按下
    {
        KeySound();                                       //按键按下去发出提示音
        SetLcdCursor(0, 0);                               //设置坐标第1行，第1列
        ShowLcdStr("   Stop_Watch   ");                   //显示字符串"   Stop-Watch   "
        SetLcdCursor(1, 0);                               //设置坐标第1行，第2列
        ShowLcdStr("  00:00:00 00   ");                   //显示字符串"  00:00:00 00   "
        
        DelayMs(10);                                      //延时等待，消除按键按下的抖动
        while(!Key3);                                     //等待按键释放
        DelayMs(10);                                      //延时等待，消除按键松开的抖动
        
        /* 秒表控制 */
        while(Key3)                                       //判断按键3是弹起
        {
            // 秒表的开始和暂停切换
            if(Key4 == 0)                                 //判断按键4是否被按下
            {
                KeySound();                               //按键按下去发出提示音
                if(TR1 == 0)                              //关变开
                {
                    TR1 = 1;
                }
                else                                      //开变关
                {
                    TR1 = 0;
                }
                DelayMs(10);                              //延时等待，消除按键按下的抖动
                while(!Key4);                             //等待按键释放
                DelayMs(10);                              //延时等待，消除按键松开的抖动
            }
            
            // 秒表清零
            if(Key5 == 0)                                 //判断按键5是否被按下
            {
                KeySound();                               //按键按下去发出提示音
                Stop_Watch_Count = 0;                     //计数变量1清零
                Stop_Watch_Second = 0;                    //计数变量2清零
                SetLcdCursor(1, 0);                       //设置坐标第2行，第1列
                ShowLcdStr("  00:00:00 00   ");           //显示清零
                DelayMs(10);							  //延时等待，消除按键按下的抖动
				while(!Key5);							  //等待按键释放
				DelayMs(10);                              //延时等待，消除按键松开的抖动
            }
            
            if(TR1 == 1)
            {
                SetLcdCursor(1, 2);                       //设置坐标第2行，第3列
                ShowLcdNum(Stop_Watch_Second/3600);       //小时
                SetLcdCursor(1, 5);                       //设置坐标第2行，第6列
                ShowLcdNum(Stop_Watch_Second%3600/60);    //分钟
                SetLcdCursor(1, 8);                       //设置坐标第2行，第9列
                ShowLcdNum(Stop_Watch_Second%60);         //秒钟
                SetLcdCursor(1, 11);                      //设置坐标第2行，第12列
                ShowLcdNum(Stop_Watch_Count);             //毫秒
            }
        }
        // 退出秒表，回到时钟模式
        KeySound();                                       //按键按下去发出提示音
        TR1 = 0;                                          //停止定时器
        Stop_Watch_Count = 0;                             //计数变量1清零
        Stop_Watch_Second = 0;                            //计数变量2清零
        ShowLcdInit();                                    //液晶显示时钟界面
        DelayMs(10);                                      //延时等待，消除按键按下的抖动
        while(!Key3);                                     //等待按键释放
        DelayMs(10);                                      //延时等待，消除按键按下的抖动       
    }
}
/*********************************************************/
// 闹钟判断
/*********************************************************/
void ClockJudge()
{
	if(Clock_Swt == 1)			                          //判断闹钟的开关是否开启
	{
		if((Clock_Hour==TimeBuff[4]) && (Clock_Minute==TimeBuff[5]))		//当前小时和分钟，和闹钟的小时和分钟是否一致
		{
			if(TimeBuff[6] == 0)						  //秒数是否等于0
			{
				Buzzer_Flag = 1;						  //开启蜂鸣器报警标志
			}
		}
	}
	
	if(TimeBuff[6] == 59)								  //如果当前秒数为59秒
	{
		Buzzer_Flag = 0;							      //关闭蜂鸣器报警标志
	}
	
	if((Key4==0)||(Key5==0))			                  //如果Key4或Key5被按下，关闭蜂鸣器
	{
		Buzzer_Flag = 0;							      //关闭蜂鸣器报警标志
	}
	
	if(Buzzer_Flag == 1)								  //如果蜂鸣器报警标志为启动
	{
		Buzzer = 0;										  //启动蜂鸣器
		DelayMs(50);									  //延时0.05秒
		Buzzer = 1;										  //关闭蜂鸣器
		DelayMs(50);									  //延时0.05秒
	}
}
/*********************************************************/
// 定时器初始化，用于秒表
/*********************************************************/
void Timer1Init()
{
	TMOD = 0x10;				                          // 使用定时器1，工作方式是1	 
	ET1  = 1;						                      // 定时器1中断使能
	EA   = 1;						                      // 打开总中断
}
/*********************************************************/
 //主函数
/*********************************************************/
void main()
{
    int temp;											  //保存温度值
	
    Timer1Init();
	LcdInit();											  //执行液晶初始化	
	DS1302_Init();										  //时钟芯片的初始化
	ShowLcdInit();										  //液晶显示内容的初始化
   
	if(DS1302_Read_Byte(0x81) >= 128)		              //判断时钟芯片是否正在运行
	{
		DS1302_Write_Time();						      //如果没有，则初始化一个时间
	}	
    
	Clock_Hour = ReadE2PROM(0x2000);		              //读取0x2000这个地址的内容，赋值给闹钟的小时变量
	if(Clock_Hour > 23)									  //如果读取到的闹钟小时数值不正常，则重新赋值
	{
		Clock_Hour = 12;
	}
    Clock_Minute = ReadE2PROM(0x2001);	                  //读取0x2001这个地址的内容，赋值给闹钟的分钟变量
	if(Clock_Minute > 59)								  //如果读取到的闹钟分钟数值不正常，则重新赋值
	{ 
		Clock_Minute = 30;
	}
	Clock_Swt = ReadE2PROM(0x2002);		                  //读取0x2002这个地址的内容，赋值给闹钟的开关变量
	if(Clock_Swt > 1)									  //如果读取到的闹钟开关数值不正常，则重新赋值
	{
		Clock_Swt = 0;
	}
	
	while(Get18B20Temp() == 850)		                  //等待温度传感器初始化完成
	{
		DelayMs(10);
	}
    
    while(1)
    {
        DS1302_Read_Time();				                  //获取当前时钟芯片的时间，存在数组time_buf中
		FlashTime();							          //刷新时间显示
		ClockJudge();							          //闹钟工作的判断
		
		temp = Get18B20Temp();	                          //读取温度
		SetLcdCursor(1,9);						          //定位到显示温度的地方
		ShowLcdTemp(temp);				                  //显示温度

		KeyScanf1();							          //按键扫描1(时间的设置)
		KeyScanf2();							          //按键扫描2(闹钟的设置)
		KeyScanf3();                                      //按键扫描3(进入和退出秒表)
        
		DelayMs(100);							          //延时0.1秒
    }
}

/*********************************************************/
// 定时器1服务程序，用于秒表
/*********************************************************/
void Timer1(void) interrupt 3
{
    TH1 = 216;                                            // 给定时器1的TH0装初值，定时10ms
    TL1 = 240;                                            // 给定时器1的TL0装初值
    
    Stop_Watch_Count++;                                   //Stop_Watch_Count加1
    if(Stop_Watch_Count >= 100)                           //判断如果Stop_Watch_Count是否加到100
    {
        Stop_Watch_Count = 0;                             //加到100之后Stop_Watch_Count清0
        Stop_Watch_Second++;                              //Stop_Watch_Second加1，既加1秒
    }
}










