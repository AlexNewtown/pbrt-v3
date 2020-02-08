<!--
-->

# pbrt-v3 �Ķ��ʼ�(��): ����ϸ��

tree_crown.jpg

��ƪ������Ǳʼ���, ��¼��ǰ��ƪ����δ�ἰ�Ĳ����Ķ�ϸ�ںͱ��������������, ��(Ӧ�û�)��������. ���µĸ������ֿ���˵�����໥��ϵ���ָ��Զ�����, �����԰��Լ�����Ȥ�ֱ��Ķ���������.



- Interaction ������ Geometry ��ϵͳ����������, ��ͨ�� ray casting �ڳ������ҵ�һ�������, ���Ǿ���ʱ����Ҫ�ٹ�ע�����е� shapes ��.
- BRDF ������ Shading �� Integrator, ��ͨ�� Interaction �Ϲ����� Material �������Ӧ�� BRDF ��, ͬ����, ���ǲ����ٹ�ע����, ��� Interaction �ϵ�������Ϣ, ���ǲ����ٹ�ע primitives ��.

���Ƶ�, sampler, camera, light �ȶ�ͨ�����ԵĽӿ���������ϸ��, �����ǿ��԰�ע����������ĳһ����, �������˽�����ϵͳ������ϸ��
ͨ���ӿڰ�ϸ����������, ��Ȼû������ȥ


# ���ۺ�ģ�黮��

��һ�µ� 1.7 С���ÿһ�µ� Further Reading ���������һ�����ݵ���ʷ��չ, ��������, �ǳ�������

## ��Դ������

[]()

# ��һ����: Geometry

## ���� & �任

### Transform

(����)�任��ʵʱ��Ⱦ��Ӧ���ܸ�����⺬��

unity.Transform, unreal.Transform

## shape/primitive

### �������

todo

## ���߼��ٽṹ

## ��Դ������

[]()


# �ڶ�����: spectrum/radiance


## ��Դ������

[]()

# ��������: sample/filter/denoise


## ��Դ������

[]()

# ���Ĳ���: Camera/Light

## Camera

## Light

### �����Դ AreaLight

�� pbrt-v3 ��, ���ĳ�� shape �� AreaLight �Ļ�, ��������һ�������� AreaLight. 

## ��Դ������

[]()


# ���岿��: Shading

## BRDFs

�� xxx �����ǽ����� BRDF �Ļ�������, ���ｫ������չ

���ھ���, ����, ������

## ����

## ����

### �������, Բ׶����, ����΢��


Բ׶�ʹ����� => �����ʹ�����

## ��Դ������

[����������ɫ��BRDF](https://zhuanlan.zhihu.com/p/21376124)

[BRDF - Wakapon](http://wiki.nuaj.net/index.php?title=BRDF)

[More DETAILS! PBR����һ����չ�����](https://zhuanlan.zhihu.com/p/32209554)

[]()

[]()

[]()

[]()



# ��������: Rendering

## ��Դ������

[]()



# more



# ��Դ������

## pbrt

- code: https://github.com/mmp/pbrt-v3
- book: http://www.pbr-book.org/3ed-2018/contents.html
- errata: https://www.pbrt.org/errata-3ed.html
- courses: https://www.pbrt.org/courses.html

## ���ķ���&�ʼ�

- https://github.com/zq317157782/raiden
- https://book.douban.com/subject/26974215/
- https://zhuanlan.zhihu.com/wayonpbrt
- https://www.zhihu.com/people/lou-jia-jie-95/posts
- https://www.qiujiawei.com/

# detail

- Moving Frostbite to PBR
- https://www.scratchapixel.com/lessons/digital-imaging/colors

## light sources

## light transport

- https://www.scratchapixel.com/lessons/3d-basic-rendering/global-illumination-path-tracing

## monte carlo integration

- https://www.scratchapixel.com/lessons/mathematics-physics-for-computer-graphics/monte-carlo-methods-in-practice/monte-carlo-methods

## ray�Cobject intersections, acceleration structures and more

- �����Դ���֪ʶ������ߺ������εĽ�������: https://www.qiujiawei.com/triangle-intersect/

- ΢�ּ�������Ⱦ(1): https://www.qiujiawei.com/partial-derivatives/

- https://www.qiujiawei.com/bvh-1/
- https://www.qiujiawei.com/bvh-2/

## material and texture

- https://zhuanlan.zhihu.com/p/53086060
- https://belcour.github.io/blog/research/2018/05/05/brdf-realtime-layered.html

## surface scattering

- ����������ɫ: https://zhuanlan.zhihu.com/p/20091064
- ����������ɫ��BRDF https://zhuanlan.zhihu.com/p/21376124

## camera

## sampling and reconstruction

- �Ͳ������У�һ��- �������еĶ��弰����: https://zhuanlan.zhihu.com/p/20197323
- �Ͳ������У�����- ��Чʵ���Լ�Ӧ��: https://zhuanlan.zhihu.com/p/20374706
- http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/
