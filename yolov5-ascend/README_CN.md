中文|[English](README.md)

**本样例为大家学习昇腾软件栈提供参考，非商业目的！**

**本README只提供命令行方式运行样例的指导，如需在Mindstudio下运行样例，请参考[Mindstudio运行图片样例wiki](https://gitee.com/ascend/samples/wikis/Mindstudio%E8%BF%90%E8%A1%8C%E5%9B%BE%E7%89%87%E6%A0%B7%E4%BE%8B?sort_id=3164874)。**

## jpegd样例
功能：调用dvpp的jpegd/png接口和resize，实现图片解码加缩放串接的图像预处理功能，默认缩放到原图宽高各一半的大小，输入图片的路径可以在sample_run.sh中修改。
样例输入：待解码的jpeg图片或png图片。    
样例输出：解码、缩放后的YUV图片。    

### 前置条件
请检查以下条件要求是否满足，如不满足请按照备注进行相应处理。如果CANN版本升级，请同步检查第三方依赖是否需要重新安装（5.0.4及以上版本第三方依赖和5.0.4以下版本有差异，需要重新安装）。
| 条件 | 要求 | 备注 |
|---|---|---|
| CANN版本 | >=5.0.4 | 请参考CANN样例仓介绍中的[安装步骤](https://gitee.com/ascend/samples#%E5%AE%89%E8%A3%85)完成CANN安装，如果CANN低于要求版本请根据[版本说明](https://gitee.com/ascend/samples/blob/master/README_CN.md#%E7%89%88%E6%9C%AC%E8%AF%B4%E6%98%8E)切换samples仓到对应CANN版本 |
| 硬件要求 | Atlas200DK/Atlas300([ai1s](https://support.huaweicloud.com/productdesc-ecs/ecs_01_0047.html#ecs_01_0047__section78423209366))  | 当前已在Atlas200DK和Atlas300测试通过，产品说明请参考[硬件平台](https://ascend.huawei.com/zh/#/hardware/product) ，其他产品可能需要另做适配|
| 第三方依赖 | 安装准备 | 需要按照第三方依赖的[安装准备](../../../environment)，完成环境变量的设置 |


### 样例部署

执行以下命令，执行编译脚本，开始样例编译。   
```
cd /scripts    
bash sample_build.sh
```

### 样例运行

执行运行脚本，开始样例运行。

```
bash sample_run.sh
```

### 查看结果

运行完成后，会在运行环境样例目录下的out/output中生成解码和缩放后的YUV图片。

### 常见错误
请参考[常见问题定位](https://gitee.com/ascend/samples/wikis/%E5%B8%B8%E8%A7%81%E9%97%AE%E9%A2%98%E5%AE%9A%E4%BD%8D/%E4%BB%8B%E7%BB%8D)对遇到的错误进行排查。如果wiki中不包含，请在samples仓提issue反馈。