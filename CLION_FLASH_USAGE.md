# CLion 一键下载说明

工程里已经加入了共享运行配置：

- `Flash STM32`

它的行为是：

1. 运行 `flash.ps1`
2. `flash.ps1` 自动调用 `build.ps1`
3. `build.ps1` 先编译固件
4. 编译成功后调用 `STM32_Programmer_CLI`
5. 通过 `ST-Link` 把程序下载到单片机

## 在 CLion 里怎么用

1. 打开 `diode_measure` 工程
2. 等待 CMake 索引完成
3. 右上角运行配置下拉框里选择 `Flash STM32`
4. 连接好 `ST-Link` 和目标板
5. 点击绿色三角运行按钮

之后每次改完代码，直接再点一次绿色三角即可重新编译并烧录。

## 需要满足的前提

- 已安装 ARM 工具链 `arm-none-eabi-gcc`
- 已安装 `STM32CubeProgrammer`
- `ST-Link` 已接好
- 目标板已经上电

## 如果运行失败

优先检查这几项：

- `ST-Link` 是否被识别
- 目标板 `3.3V` 和 `GND` 是否正常
- `SWDIO / SWCLK / GND / 3.3V` 是否接对
- `BOOT0` 是否处于正常从 Flash 启动状态
- `STM32_Programmer_CLI` 是否已安装

如果 CLion 没立刻显示出 `Flash STM32`，执行一次：

- `File -> Reload CMake Project`

如果还没出现，就重新打开工程目录。
