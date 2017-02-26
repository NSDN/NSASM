# NSASM MANUAL
NyaSama Assembly Script Module Manual
## Instruction (zh_CN)

### 操作符不区分大小写

#### 大概是8086的风格

<code>

r#      # = 0 to (REG_CNT - 1)，通用寄存器

mov     数据传送

push    入栈

pop     出栈

in      输入，0x00为标准输入流，仍在开发中

out     输出，地址0x00为标准输出流，0xFF带有[DEBUG]前缀输出

add     加法

inc     加一

sub     减法

dec     减一

mul     乘法

div     除法

cmp     比较，仅此操作符能改变跳转状态寄存器

jz      零则跳转

jnz     非零则跳转

jg      大于则跳转

jl      小于则跳转

and     与

or      或

xor     异或

not     非

shl     左移

shr     右移

end     程序结束，或子程序结束

run     外部加载子程序文件，带提示

call    外部加载子程序文件，无提示

nop     空指令

rst     复位，PC不可用，暂未实现

rem     注释行前缀


</code>

### 基本结构

<code>
.conf {

    stack 栈大小

    heap 堆大小

}


.data {

    rem 这是注释

    var 变量名 = 值

    var a = 1       整数

    var b = 0x1     整数

    var c = 2h      整数

    var d = 1.0     浮点

    var e = 2F      浮点

    var f = 3.0F    浮点

    var g = 'a'     字符

    var h = "ho\n"  字符串，只读

}


.code {

    rem 这是注释

    ...

    [TAG]

    rem 标号必须使用中括号[]包括起来

    ...

    jmp [TAG]

    ...

    cmp r0, r1

    jz [TAG]

    rem 寄存器为r0~r7，r不区分大小写

    rem 目前仅cmp指令影响跳转标志

    rem "cmp a, b"相当于判断a - b的结果，是0还是大于0还是小于0

    end
    
}
</code>

### 如何运行

编译后得到可执行文件，比如nsasm.exe，则：

nsasm.exe [c/r] [FILE]

c为编译，r为运行

其中c目前暂不可用，缺省参数为r

#### Copyright © NSDN 2014 - 2017
