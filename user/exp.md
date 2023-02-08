* user.h主要实现类主要文件，user/ulib.c, user/printf.c, user/umalloc.c

# sleep
1. 添加必要的头文件, 系统调用kernel sleep， 实际调用在usys.pl产生的usys.S，汇编代码
2. 修改makefile文件，加入sleep

# pingpong
1. pipe的使用，主进程等子进程写入并且返回，然后再读取pipe，否则有并发错误


# primes
1. 注意管道的关闭，主进程等待子进程返回
2. data -> | filter(data) -> | filter(data) -> |

# find
1. 借鉴ls.c的代码，递归遍历目录

# xargs
1. 拼接参数，exec, 注意参数的顺序，每次从标准输入中读出一个参数，拼接到xargs的参数后面