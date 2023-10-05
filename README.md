# SoftRenderer
Personal Renderer Learning

QA From TinyRenderer&TinyRenderer问题汇总:https://www.cnblogs.com/xkyl/p/17660819.html
- Version1 lesson0-4 旧头文件
- Version1_1 lesson0-4 新头文件 tip:注意使用新头文件读取的是切线空间法线贴图记得更改。_nn_tangent.tga -> _nm.tga
- Version1_2 lesson5-6 各种Shader，视口变换，视口变换不是自动进行的。
- Version1_3 lesson7 ShadowMapping 视口变换在光栅化进行 

- Version2-1 采用SDL库，加入实时渲染窗口 将lesson6的内容在窗口中显示 600*600大小 目前5帧左右
    - 一开始一帧需要运行2.5s，优化思路主要针对在像素着色器多次渲染的运算进行优化 因为是单线程运行
    - 优化项：
        - 1. 重心坐标方程
        - 2. 重复计算的坐标变换在外面直接用uniform变量代替
        - 3. 像素着色器的中间变量简化
        - 4. 编译器优化

Ref:
https://github.com/ssloy/tinyrenderer
https://github.com/SunXLei/SRender
