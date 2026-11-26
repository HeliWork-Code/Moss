Moss 26创意稿


for(1->n,>>+) //从1到N循环，递增
for(int i=1,i->n,>>-) //i到n递减
for(1->n,>>*2) //1到n，每次乘2

while(1->n) //1到N循环，无法设置递增或递减

**
__go(1,1->n,(ex*2-1)) //__go循环，从1开始，默认为int ex，1到n，每次ex*2-1


高级用法：

**
fn go ms(args int a){
	return __go_if(a*2>=10 && a*2<=20);
}
__go(1,1->n,ex++,ms(ex))

//依旧__go循环，但是可以同时起到if语句的作用

if语句和C++一样即可




数组定义：
array int arr[10]={}; //这是最普通的数组表达

**
array int arr[10>>100]={}; //这是限制一个动态数组大小的
array int arr[n]={}; //可以使用常量或变量作为大小，但是必须保证该量非空
array int arr[10>>]={} //可以将数组初始开到10个，如果超出则无限放大
array int arr[>>] //没有数据则空，有数据则无限放大
array int arr[>>10]//没有数据则空，有数据则最多放大到10个

二、三维数组同理

数组使用：
几乎与C++一样，以下为更多扩展用法，需要使用pick中的array.msh as array_manage

**
__array(c,arr,10); //将arr数组手动扩大10个
__array(c,arr,-10); //将arr数组手动缩小10个
__array(d,arr,8); //将arr数组的第8位删去
__array(s,arr,8,7); //将arr数组的第8位设为7


错误处理：
大部分与C++保持一致

内存管理：
需要使用pick中memory.msh as mem

__RAM add str1: {string,"This is in RAM!"}; //类似于C++的add关键字
__RAM del str1; //类似于C++的delete关键字


动态库？
动态库对于一个程序来说是不可或缺的！
所以！26.1.x-26.2.0必须端来动态库！
后缀：.msdl / .dll
在源代码中，可以通过pick dl来链接动态库！
pick dl{
	//用法和导入head一样
}

动态库管理：
在代码的最开始中可以使用pick管理用到的库
但是这样可能会占用内存
所以 我们可以在最开始引用一个 cap_manage.msh

__PICK add memory.msdl:mem; //临时链接memory.msh的mem
__PICK del memory.msdl:mem; //取消链接memory.msh的mem
__PICK list; //输出目前已经链接的动态库库


动态库开头格式：
moss 1000;
moss dynlib:memory; //声明一个memory的动态库，方便编译器打包！！

fn void example1(){
	//此处是你的自定义函数
}


常量：（26.0.x）
const int n=1; //声明一个普通的常量
__const array int a[5]={1,2,3,4,5}; //声明一个常量数组
