# pbrt-v3 ����&�鼮�Ķ��ʼ�

## 3 shapes

### 3.1 basic shape interface

3.1.5 Sidedness

 The potential for improved performance is reduced when using this technique with ray tracing, however, 
 since it is often necessary to perform the ray�Cobject intersection before determining the surface normal to do the back-facing test. 
 Furthermore, this feature can lead to a physically inconsistent scene description if one-sided objects are not in fact closed. 
 ֧�ֱ����޳��ᵼ��Ǳ�ڵ�������ʧ???, ��ԷǷ�յ�ͼԪ���������Ⱦ�Ĳ���ʵ��. 
 ��ͳ�Ĺ�դ����Ⱦ��ˮ�߼���������Ǿֲ�����, ��ʹ��ȫ�ֹ���ģ��ʱ(real time raytracing)Ҳ��Ҫ��ʼ������������.