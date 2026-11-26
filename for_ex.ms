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
几乎与C++一样，以下为更多扩展用法，需要使用pick中的ms.msh as __array

**
__array(c,arr,10); //将arr数组手动扩大10个
__array(c,arr,-10); //将arr数组手动缩小10个
__array(d,arr,8); //将arr数组的第8位删去
__array(s,arr,8,7); //将arr数组的第8位设为7


