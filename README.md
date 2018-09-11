# Netease Cloud Music Copyright Protection File Dump

## **dep**
```
openssl
taglib(with zlib)
jsoncpp
```

上面列出的依赖库在不同平台下名字不同，Debian系列注意包名添加`-dev`后缀

## **build**

> #### 注意：项目使用C++17，全平台使用静态链接，相关配置请看`cmake/ucm.cmake`

### *Windows(MSVC)*

请使用[vcpkg](https://github.com/Microsoft/vcpkg)安装相关依赖（注意是否为安装为静态库），参考vcpkg的文档使用cmake进行configure和build。

### *Linux(gcc/clang)*

直接cmake，无特殊配置，libc++和libstdc++自匹配

### *macOS(clang)*

直接cmake，无特殊配置，已适配libc++

> !!! libc++由于版本更新，头文件`<experimental/filesystem>`可能会并入std，相关链接库不知是否会改变，如有问题，欢迎issue

# 鸣谢

algorithm来自[anonymous5l/ncmdump](https://github.com/anonymous5l/ncmdump)，特别感谢anonymous5l的奉献和其开源精神

## Warning

学习研究

切勿宣传

低调使用

后果自负
