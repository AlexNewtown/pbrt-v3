<!--

光的加色(光源), 减色(物体反射)模型
-->

# pbrt-v3 阅读笔记(二): 物体的形状与材质

在场景中有光源和相机以后, 我们很自然的会加入物体进去. 本篇的主要内容是分析光在物体表面上的行为, 为了方便分析, 我们就只先加入一个球体吧.

## 场景二: 加入单个物体



#### 点光源

虽然图中画了一个比较大的光源, 但它只是作为示意, 为了简单起见, 我们先从点光源开始分析, 实际上我们把它作是理想的**点光源**来处理(怎么说呢, 能看见的光源都不是点光源, 不过它本来就是理想化的光源), 那光源射向相机的, 也就只有一条**光线**.


#### 几何光学

这里我们先把光看作是沿着一条直线传播的(这也是初中的几何光学里用的模型), 细节之后再讲
