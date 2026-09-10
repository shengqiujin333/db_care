示例目的：
          UART1中断方式接收数据超时检测示例。

硬件资源：
          1. CW32L010 StartKit
          2. 时钟HSIOSC
          3. 系统时钟设置为HSIOSC时钟6分频，8MHz，PCLK、HCLK不分频，PCLK=HCLK=SysClk=8MHz
          4. UART的传输时钟设置为PCLK

演示说明：
          PC发送数据，UART中断方式接收数据，并回传至PC，超时时间为传输两个2个数据的时间，若发生超时，打印timeout。

硬件连接：
          UART1_TXD (PA6) -- PCRX
          UART1_RXD (PA5) -- PCTX  

使用说明：
+ EWARM
          1. 打开project.eww文件
          2. 编译所有文件：Project->Rebuild all
          3. 载入工程镜像：Project->Debug
          4. 运行程序：Debug->Go(F5)

+ MDK-ARM
          1. 打开project.uvproj文件
          2. 编译所有文件：Project->Rebuild all target files
          3. 载入工程镜像：Debug->Start/Stop Debug Session
          4. 运行程序：Debug->Run(F5)
