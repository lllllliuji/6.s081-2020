# simplify copyin
1. 重写mappages使得p->k_pagetable可以重新remap，因为exec会替换原来进程的文本页 + 栈
