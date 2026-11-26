$moss.v=1000; //设定moss，让编译器认出来
pick head{ //从pick方法传入需要的头文件
moss.msh;
}

fn int add(args int a,b,c;){
	return a+b+c;
}

fn int main(args string arg){ //主函数，通过args类型设为参数
	int a,b,c;
	input("%d"&a);//通过类似于scanf的方法传入a
	ms.in@b@","@c;//通过ms.in，用@的方式格式化传入东西，如果这样的话，输入就应为:1,3
	int ans;
	ans=add(a,b,c);
	output("%d",ans);
	ms.out@ans;
}
