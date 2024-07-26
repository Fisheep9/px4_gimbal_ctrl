# PX4 Gimbal Ctrl

使用mavros mount control进行云台控制的一个示例：

> Status： 正式版

## 节点流程
使用之前需要启动mavros，启动节点后，通过`/mavros/mount_control/orientation`获取一次当前云台的位置，并从当前位置运动到目标位置`gimbal_ctrl/mag`，运行过程中可通过改变该参数来改变云台的当前位置，改变之后会以恒定的角速度运动到目标位置，该速度可通过`gimbal_ctrl/rate`进行设置，单位为deg/s。

## 参数
```xml
<!-- mag 参数表示目标位置随节点以50 Hz的频率轮询，因此支持动态修改-->
<param name="mag" type="double" value="45" />
<!-- type 参数默认为固定值fixed，其他模式是用来debug的-->
<param name="type" type="string" value="fixed">
<!-- rate 参数表示运动的角速度，单位为deg/s-->
<param name="rate" type="double" value="50"/>
<!-- bias 参数表示偏移量，在期望的角度加上偏移量以修正云台的潜在虚位，单位为deg-->
<param name="bias" type="double" value="0.0"/>
<!-- range 参数表示云台的运动范围，为0 - [range] deg，超出范围终端会发出警告并自动限制rosparam -->
<param name="range" type="double" value="45.0" />
```

> **目前云台的运动范围为0 - 45 deg**

## 特性

- 可以将飞控的Prearm选项打开，具体见PX4文档，这样在上锁模式下仍可以改变云台位置。
https://docs.px4.io/main/en/advanced_config/prearm_arm_disarm.html#COM_PREARM_MODE

- 由于PX4 Mount Control的特性，在节点退出后仍然会保持当前位置，只有在飞控重启后进行复位


