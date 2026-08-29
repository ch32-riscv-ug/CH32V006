# 暴力风扇

目前支持15万转

## 开环启动

可以通过升压升频法进行强托启动

## 闭环功能

支持低速滤波，高速自动关闭。

## 保护功能

- 堵转保护

## 保护/阈值参数（config.h）

| 参数名称 | 参数说明 | 参考设定值 |
|---|---|---|
| OVER_CURRENT1 | 过流保护1，单位：A | 80 |
| OVER_CURRENT1_TIME | 过流保护1持续时间，单位：ms | 200 |
| OVER_CURRENT2 | 过流保护2，单位：A | 100 |
| OVER_CURRENT2_TIME | 过流保护2持续时间，单位：ms | 50 |
| OVER_VOLTAGE | 过压，单位：V | 25 |
| UNDER_VOLTAGE | 欠压，单位：V | 12 |
| VOLTAGE_PROTECT_TIME | 电压保护时间 | 200 |
| STALL_TIME | 闭环堵转时间，单位：ms | 500 |
| PHASE_LOSS_THRESHOLD | 缺项保护阈值（ADC采样值） | 25 |
| BRAKE_TIME | 电机刹车时间，单位：ms | 5000 |
